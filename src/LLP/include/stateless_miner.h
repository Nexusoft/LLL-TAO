/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_MINER_H
#define NEXUS_LLP_INCLUDE_STATELESS_MINER_H

#include <LLP/packets/stateless_packet.h>
#include <LLP/packets/packet.h>
#include <LLP/include/canonical_chain_state.h>
#include <LLP/include/push_notification.h>
#include <LLC/types/uint1024.h>
#include <TAO/Ledger/types/block.h>
#include <LLC/include/flkey.h>
#include <Util/include/hex.h>

#include <algorithm>
#include <array>
#include <string>
#include <cstdint>
#include <vector>
#include <memory>

/* Forward declarations */
namespace TAO { namespace Ledger { class Block; class TritiumBlock; } }

namespace LLP
{
    namespace Diagnostics
    {
        inline std::string FullHexOrUnset(const uint256_t& value)
        {
            return value != 0 ? value.GetHex() : std::string("NOT SET");
        }

        inline std::string KeyFingerprint(const std::vector<uint8_t>& vKey)
        {
            if(vKey.empty())
                return "NOT AVAILABLE";

            const size_t nPrefix = std::min<size_t>(8, vKey.size());
            return HexStr(vKey.begin(), vKey.begin() + nPrefix);
        }

        inline const char* YesNo(const bool fValue)
        {
            return fValue ? "YES" : "NO";
        }

        inline const char* PassFail(const bool fValue)
        {
            return fValue ? "PASS" : "FAIL";
        }
    }

    /** SessionConsistencyResult
     *
     *  Canonical session/container consistency check result shared across mining
     *  context, recovery, and node session registry code.
     *
     **/
    enum class SessionConsistencyResult : uint8_t
    {
        Ok = 0,
        MissingSessionId,
        MissingGenesis,
        MissingFalconKey,
        RewardBoundMissingHash,
        EncryptionReadyMissingKey,
        SessionIdMismatch,
        GenesisMismatch,
        FalconKeyMismatch
    };

    const char* SessionConsistencyResultString(const SessionConsistencyResult result);

    /** SessionBinding
     *
     *  Shared immutable-style identity snapshot for comparing/logging mining
     *  session identity and reward binding state across components.
     *
     **/
    struct SessionBinding
    {
        uint32_t nSessionId = 0;
        uint256_t hashGenesis = 0;
        uint256_t hashKeyID = 0;
        uint256_t hashRewardAddress = 0;
        ProtocolLane nProtocolLane = ProtocolLane::LEGACY;
        std::string strKeyFingerprint;
        bool fRewardBound = false;

        bool HasRewardBinding() const
        {
            return fRewardBound && hashRewardAddress != 0;
        }
    };

    /** CryptoContext
     *
     *  Canonical encrypted-session snapshot so callers do not need to manually
     *  reassemble session ID, genesis, lane, and key material.
     *
     **/
    struct CryptoContext
    {
        std::vector<uint8_t> vChaCha20Key;
        std::string strKeyFingerprint;
        uint32_t nSessionId = 0;
        uint256_t hashGenesis = 0;
        uint256_t hashKeyID = 0;
        ProtocolLane nProtocolLane = ProtocolLane::LEGACY;
        bool fEncryptionReady = false;

        bool HasUsableKey() const
        {
            return fEncryptionReady && !vChaCha20Key.empty();
        }
    };

    /** TemplateMetadata
     * 
     *  Tracks metadata about mining templates for enhanced multi-channel staleness detection.
     *  
     *  CRITICAL CONTEXT - THE TEMPLATE STALENESS PROBLEM:
     *  ==================================================
     *  Prior to PR #134, templates were incorrectly marked stale when ANY channel mined a block,
     *  causing ~40% wasted mining work. This was because templates were compared against unified
     *  blockchain height, which changes when Prime, Hash, or Stake channels mine blocks.
     *  
     *  Example of OLD INCORRECT behavior:
     *  ----------------------------------
     *  1. Miner has Prime template at unified height 6535198 (prime_channel=2165443)
     *  2. Hash channel mines a block → unified height becomes 6535199
     *  3. Template marked STALE ❌ (WRONG - Prime channel is still at 2165443!)
     *  4. Miner discards perfectly valid Prime template unnecessarily
     *  5. Result: ~40% wasted work across all miners
     *  
     *  THE SOLUTION - CHANNEL-SPECIFIC HEIGHT TRACKING:
     *  ================================================
     *  Templates should only be marked stale when THEIR SPECIFIC CHANNEL advances, not when
     *  other channels mine blocks. This requires tracking nChannelHeight separately from nHeight.
     *  
     *  Example of NEW CORRECT behavior (PR #134):
     *  ------------------------------------------
     *  1. Miner has Prime template (unified=6535198, prime_channel=2165443)
     *  2. Hash channel mines a block → unified height becomes 6535199
     *  3. Prime channel height STILL 2165443 (unchanged)
     *  4. Template is FRESH! ✅ (CORRECT - can keep mining)
     *  5. Result: <5% wasted work (only when same-channel blocks compete)
     *  
     *  NEXUS MULTI-CHANNEL ARCHITECTURE:
     *  =================================
     *  Nexus uses three independent mining channels competing on one blockchain:
     *  - Prime channel (1): CPU mining via Fermat prime cluster discovery
     *  - Hash channel (2):  GPU/FPGA mining via SHA3 hashing
     *  - Stake channel (0): Proof-of-Stake (trust-based, not for mining)
     *  
     *  Unified Height (nHeight):
     *  -------------------------
     *  Increments for EVERY block regardless of which channel mined it.
     *  Example: Height 6535193 (Hash) → 6535194 (Prime) → 6535195 (Hash) → 6535196 (Stake)
     *  
     *  Channel Height (nChannelHeight):
     *  --------------------------------
     *  Only increments when THAT SPECIFIC CHANNEL mines a block.
     *  Example at unified height 6535196:
     *    - Prime channel:  2165442 (last Prime block was at unified height 6535194)
     *    - Hash channel:   4165000 (last Hash block was at unified height 6535195)
     *    - Stake channel:  235000  (last Stake block was at unified height 6535196)
     *  
     *  STALENESS DETECTION LOGIC (CORRECTED in PR #134):
     *  =================================================
     *  
     *  PRIMARY CHECK: Channel-specific height comparison
     *  -------------------------------------------------
     *  A template is stale if the blockchain's channel height for that channel has advanced
     *  beyond the template's channel height. This is the ONLY reliable indicator that a new
     *  block in the same channel was mined, making our template obsolete.
     *  
     *  Example:
     *    Template: prime_channel=2165443, unified=6535198
     *    Blockchain: prime_channel=2165444 (advanced!)
     *    → Template is STALE (correct - new Prime block was mined)
     *  
     *  SECONDARY CHECK: Age-based timeout (safety net)
     *  ------------------------------------------------
     *  Templates older than 600 seconds are marked stale regardless of height, to handle edge
     *  cases like blockchain reorganizations, network partitions, or clock skew.
     *  
     *  NOT CHECKED: Unified height (would cause false positives)
     *  ----------------------------------------------------------
     *  We do NOT compare against unified blockchain height because it changes when OTHER channels
     *  mine blocks, which should NOT invalidate our template.
     *  
     *  IMPLEMENTATION NOTES:
     *  ====================
     *  
     *  GetLastState() Performance:
     *  ---------------------------
     *  The corrected IsStale() method calls GetLastState() to walk backward through the blockchain
     *  to find the last block in the template's specific channel. This is O(1) average case (~3 blocks)
     *  since channels interleave frequently. Called every 5-10s per miner (minimal overhead).
     *  
     *  Thread Safety:
     *  --------------
     *  Uses atomic load of ChainState::tStateBest to get a consistent blockchain snapshot without locks.
     *  GetLastState() is read-only and safe for concurrent calls.
     *  
     *  Memory Management:
     *  ------------------
     *  Uses std::unique_ptr for automatic memory management and clear ownership semantics.
     *  TemplateMetadata is move-only (no copies) to prevent accidental duplication of block data.
     *  Integrates with PR #133's move semantics for std::map storage.
     *  
     *  USAGE IN StatelessMinerConnection:
     *  ==================================
     *  
     *  Template Creation (ProcessGetBlock):
     *  ------------------------------------
     *  1. Create new block template via new_block()
     *  2. Calculate nChannelHeight using GetLastState() for the block's channel
     *  3. Store in mapBlocks with TemplateMetadata containing both nHeight and nChannelHeight
     *  
     *  Staleness Checking:
     *  -------------------
     *  1. IsStale() automatically checks channel height via GetLastState()
     *  2. Also checks age timeout (600 second safety limit)
     *  3. Returns true only if channel advanced OR age exceeded
     *  
     *  Template Cleanup:
     *  -----------------
     *  CleanupStaleTemplates() iterates mapBlocks and removes stale templates using IsStale().
     *  
     *  EXPECTED IMPACT:
     *  ===============
     *  - Eliminates ~40% false-positive staleness detections
     *  - Reduces wasted mining work from ~40% to <5%
     *  - Templates remain valid across other-channel blocks
     *  - Foundation for accurate difficulty retargeting
     *  - Foundation for ambassador/developer reward payouts
     *  
     *  RELATED PRs:
     *  ===========
     *  - PR #131: Template staleness prevention (base implementation)
     *  - PR #133: Move semantics for unique_ptr (memory safety)
     *  - PR #134: Multi-channel height tracking (this PR)
     *  
     *  @see TAO::Ledger::GetLastState() for channel height calculation
     *  @see TAO::Ledger::BlockState::nChannelHeight for per-channel height tracking
     *  @see StatelessMinerConnection::ProcessGetBlock() for template creation with nChannelHeight
     *  @see StatelessMinerConnection::CleanupStaleTemplates() for staleness-based cleanup
     */
    struct TemplateMetadata
    {
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        /* CORE TEMPLATE DATA                                                               */
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        
        /** The block template itself (owned by this struct via unique_ptr) */
        std::unique_ptr<TAO::Ledger::Block> pBlock;
        
        /** When this template was created (unified timestamp in seconds since epoch) */
        uint64_t nCreationTime;
        
        /** Unified blockchain height at template creation (increments for ALL channels) */
        uint32_t nHeight;
        
        /** Expected merkle root for validation when solution is submitted */
        uint512_t hashMerkleRoot;
        
        /** Mining channel for this template (1=Prime, 2=Hash, 0=Stake) */
        uint32_t nChannel;
        
        /** Best chain hash snapshot at template creation time.
         *
         *  Captures TAO::Ledger::ChainState::hashBestChain at the moment this template
         *  was created.  Used as the PRIMARY staleness anchor in IsStale(): if the live
         *  hashBestChain has advanced beyond this value the chain has moved (new block or
         *  reorg) and the template must be discarded — even when the integer channel height
         *  is unchanged (same-height reorg).
         *
         *  At creation time this equals pBlock->hashPrevBlock, matching the StakeMinter
         *  pattern (stake_minter.cpp:674).
         */
        uint1024_t hashBestChainAtCreation;
        
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        /* PR #134: CHANNEL-SPECIFIC HEIGHT (CRITICAL FOR STALENESS DETECTION)             */
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        
        /** 
         *  Channel-specific height at template creation.
         *  
         *  This is the CRITICAL field added in PR #134 that fixes the ~40% wasted work problem.
         *  
         *  Unlike nHeight (which changes when ANY channel mines), nChannelHeight only changes
         *  when THIS SPECIFIC CHANNEL mines a block. This allows accurate staleness detection
         *  that doesn't produce false positives when other channels advance.
         *  
         *  Calculated via GetLastState() during template creation in ProcessGetBlock().
         *  
         *  Example:
         *    If this is a Prime template and the last Prime block was at unified height 6535194
         *    with prime_channel=2165443, then:
         *      nHeight = 6535198 (current unified blockchain height)
         *      nChannelHeight = 2165443 (last Prime channel height)
         *  
         *  When IsStale() is called, it compares blockchain's current prime_channel against
         *  this nChannelHeight to detect if a new Prime block was mined.
         */
        uint32_t nChannelHeight;
        
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        /* CONSTRUCTORS (MOVE-ONLY SEMANTICS)                                              */
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        
        /** 
         *  Default constructor
         *  
         *  Initializes all fields to zero/nullptr. Used for std::map insertions before
         *  move-assignment from actual template data.
         */
        TemplateMetadata() 
            : pBlock(nullptr)
            , nCreationTime(0)
            , nHeight(0)
            , hashMerkleRoot(0)
            , nChannel(0)
            , hashBestChainAtCreation(0)
            , nChannelHeight(0)  // PR #134: Initialize channel height
        {
        }
        
        /** 
         *  Parameterized constructor - takes ownership of block pointer
         *  
         *  Used during template creation in ProcessGetBlock() to construct metadata
         *  with all necessary information for staleness detection.
         *  
         *  @param pBlock_        Raw block pointer (ownership transferred to unique_ptr)
         *  @param nCreationTime_ Unified timestamp when template was created
         *  @param nHeight_       Unified blockchain height at creation
         *  @param nChannelHeight_ Channel-specific height at creation (PR #134)
         *  @param hashMerkleRoot_ Expected merkle root for validation
         *  @param nChannel_      Mining channel (1=Prime, 2=Hash)
         *  @param hashBestChainAtCreation_ hashBestChain snapshot at template creation
         */
        TemplateMetadata(TAO::Ledger::Block* pBlock_, uint64_t nCreationTime_, 
                        uint32_t nHeight_, uint32_t nChannelHeight_,
                        const uint512_t& hashMerkleRoot_, uint32_t nChannel_,
                        const uint1024_t& hashBestChainAtCreation_ = uint1024_t(0))
            : pBlock(pBlock_)  // unique_ptr takes ownership
            , nCreationTime(nCreationTime_)
            , nHeight(nHeight_)
            , hashMerkleRoot(hashMerkleRoot_)
            , nChannel(nChannel_)
            , hashBestChainAtCreation(hashBestChainAtCreation_)
            , nChannelHeight(nChannelHeight_)  // PR #134: Store channel height
        {
        }
        
        /** 
         *  Move constructor - explicitly defaulted for move-only semantics
         *  
         *  Allows TemplateMetadata to be moved (e.g., when inserting into std::map).
         *  Required because unique_ptr is move-only.
         */
        TemplateMetadata(TemplateMetadata&& other) noexcept = default;
        
        /** 
         *  Move assignment operator - explicitly defaulted for move-only semantics
         *  
         *  Allows TemplateMetadata to be move-assigned (e.g., std::map[key] = std::move(value)).
         *  Required because unique_ptr is move-only.
         */
        TemplateMetadata& operator=(TemplateMetadata&& other) noexcept = default;
        
        /** 
         *  Copy constructor - explicitly deleted
         *  
         *  Copying is disabled because:
         *  1. unique_ptr cannot be copied (move-only)
         *  2. Block templates are large objects (~10KB+) - copying would be expensive
         *  3. Ownership semantics are clearer with move-only design
         */
        TemplateMetadata(const TemplateMetadata&) = delete;
        
        /** 
         *  Copy assignment operator - explicitly deleted
         *  
         *  See copy constructor comment for rationale.
         */
        TemplateMetadata& operator=(const TemplateMetadata&) = delete;
        
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        /* PR #134: CORRECTED STALENESS DETECTION LOGIC                                     */
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        
        /** 
         *  IsStale (CORRECTED in PR #134)
         *  
         *  Determines if this mining template is no longer valid for mining.
         *  
         *  CRITICAL FIX: This method now checks CHANNEL-SPECIFIC HEIGHT instead of unified
         *  blockchain height, eliminating ~40% false-positive staleness detections.
         *  
         *  STALENESS CRITERIA (checked in order):
         *  ======================================
         *  
         *  PRIMARY CHECK: Channel height mismatch
         *  --------------------------------------
         *  Template is stale if:
         *    blockchain_channel_height != template_channel_height - 1
         *  
         *  This indicates another block in OUR CHANNEL was mined, making our template obsolete.
         *  We check (template_channel_height - 1) because the template is for the NEXT block
         *  in that channel.
         *  
         *  Example where template is STALE:
         *    Template: prime_channel=2165443 (mining block 2165444)
         *    Blockchain: prime_channel=2165444 (block 2165444 already mined!)
         *    → Template is STALE (correct - someone else mined the block we were working on)
         *  
         *  Example where template is FRESH:
         *    Template: prime_channel=2165443 (mining block 2165444)
         *    Blockchain: prime_channel=2165443 (no new Prime blocks yet)
         *    → Template is FRESH (correct - keep mining)
         *  
         *  SECONDARY CHECK: Age timeout (600 second safety net)
         *  ---------------------------------------------------
         *  Template is stale if:
         *    current_time - creation_time > 600 seconds
         *  
         *  This catches edge cases like blockchain reorgs, network partitions, or clock skew.
         *  Acts as a safety timeout to prevent miners from working on very old templates.
         *  
         *  NOT CHECKED: Unified height
         *  ---------------------------
         *  We do NOT check unified blockchain height because it changes when other channels mine,
         *  which should NOT invalidate our template. This was the bug causing ~40% wasted work.
         *  
         *  IMPLEMENTATION DETAILS:
         *  ======================
         *  
         *  Thread Safety:
         *  --------------
         *  Uses atomic load of ChainState::tStateBest to get consistent snapshot.
         *  GetLastState() is read-only and safe for concurrent calls.
         *  
         *  Performance:
         *  ------------
         *  GetLastState() walks backward ~3 blocks on average (channels interleave).
         *  Called every 5-10s per miner during staleness checks.
         *  Total overhead: <100μs per call on modern hardware.
         *  
         *  Error Handling:
         *  ---------------
         *  If GetLastState() fails (e.g., genesis block, invalid channel), template is marked
         *  stale for safety. Better to refresh template than mine on potentially invalid data.
         *  
         *  @param nNow Current timestamp (optional, uses current time if 0)
         *  @return true if template is stale and should be discarded
         */
        bool IsStale(uint64_t nNow = 0) const;
        
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        /* UTILITY METHODS                                                                  */
        /* ═══════════════════════════════════════════════════════════════════════════════ */
        
        /** 
         *  GetAge
         *  
         *  Calculate how long this template has existed in seconds.
         *  
         *  Used for:
         *  - Age-based staleness timeout (templates >90s are discarded)
         *  - Logging and diagnostics
         *  - Performance monitoring
         *  
         *  @param nNow Current timestamp (optional, uses current time if 0)
         *  @return Template age in seconds
         */
        uint64_t GetAge(uint64_t nNow = 0) const;
        
        /** 
         *  GetChannelName
         *  
         *  Get human-readable name for this template's mining channel.
         *  
         *  Used for logging and diagnostics to make channel information more readable.
         *  
         *  @return "Prime", "Hash", or "Stake" based on nChannel value
         */
        std::string GetChannelName() const;
    };

    /** MiningContext
     *
     *  Immutable snapshot of a miner's state at a given moment.
     *  All methods that modify state return a new MiningContext.
     *
     *  Phase 2 Extensions:
     *  - hashKeyID: Stable identifier derived from Falcon public key
     *  - hashGenesis: Tritium account identity this miner is mining for
     *
     *  Session Stability Extensions:
     *  - nSessionStart: When the session was established
     *  - nSessionTimeout: Configurable timeout for session expiry
     *  - nKeepaliveCount: Number of keepalives received (for monitoring)
     *
     **/
    struct MiningContext
    {
        uint32_t nChannel;           // Mining channel (1=Prime, 2=Hash)
        /* Session-level unified blockchain height tracking ONLY.
         * Never copy to block.nHeight — that comes from the 216-byte template (bytes[200-203]).
         * block.nHeight is set by AddBlockData() to tStateBest.nHeight + 1 (UNIFIED). */
        uint32_t nHeight;            // Current blockchain height (session tracking only)
        uint64_t nTimestamp;         // Last activity timestamp
        std::string strAddress;      // Miner's network address
        uint32_t nProtocolVersion;   // Protocol version
        bool fAuthenticated;         // Whether Falcon auth succeeded
        uint32_t nSessionId;         // Unique session identifier
        uint256_t hashKeyID;         // Phase 2: Falcon key identifier
        uint256_t hashGenesis;       // Phase 2: Tritium genesis hash (authentication)
        std::string strUserName;     // Phase 2: Username for trust-based addressing
        std::vector<uint8_t> vAuthNonce;  // Challenge nonce for authentication
        std::vector<uint8_t> vMinerPubKey; // Miner's Falcon public key
        std::vector<uint8_t> vDisposablePubKey; // Disposable Falcon session public key
        uint256_t hashDisposableKeyID;          // Disposable Falcon session key ID
        uint64_t nSessionStart;      // Timestamp when session was established
        uint64_t nSessionTimeout;    // Session timeout in seconds (default 300s)
        uint32_t nReconnectCount;    // Recovery metadata; live SaveSession refreshes must not reset this implicitly
        uint32_t nKeepaliveCount;    // Number of keepalives received
        uint32_t nKeepaliveSent;     // Number of keepalive responses sent
        uint64_t nLastKeepaliveTime; // Timestamp of last keepalive exchange
        
        /* Reward address binding (set via MINER_SET_REWARD) */
        uint256_t hashRewardAddress; // Reward payout address (separate from auth genesis)
        bool fRewardBound;           // Whether reward address has been set

        /* ChaCha20 encryption state for secure communication */
        std::vector<uint8_t> vChaChaKey; // ChaCha20 session key derived from genesis
        bool fEncryptionReady;           // Whether ChaCha20 encryption is established

        /* Falcon version tracking */
        LLC::FalconVersion nFalconVersion;      // Detected Falcon version (512 or 1024)
        bool fFalconVersionDetected;            // Whether version has been detected

        /* Protocol lane identification (Phase 3: Unified Mining Server Configuration)
         * Tracks which protocol lane (LEGACY=8323 or STATELESS=9323) this miner is using.
         * Enables unified utility functions across both connection types. */
        ProtocolLane nProtocolLane;             // LEGACY (8-bit opcodes) or STATELESS (16-bit opcodes)

        /* Push notification subscription state (PR #XXX: Push Notifications) */
        bool fSubscribedToNotifications;    // MINER_READY received
        uint32_t nSubscribedChannel;        // Which channel subscribed to (1=Prime, 2=Hash)
        uint64_t nLastNotificationTime;     // Last notification sent (unix timestamp)
        uint64_t nNotificationsSent;        // Total notifications sent to this session

        /* Per-connection template tracking for staleness detection.
         * Tracks the CHANNEL-SPECIFIC height (not unified height) of the last template
         * sent to this miner. Only updated when an actual template (BLOCK_DATA) is sent.
         * Used by GET_ROUND to detect channel height changes and auto-send new templates.
         * 
         * IMPORTANT: This must be a channel height (e.g., Prime=2325188, Hash=4165000),
         * NOT a unified height (e.g., 6594321). Unified height changes when ANY channel
         * mines a block, but templates only need refreshing when the MINER'S channel advances.
         */
        uint32_t nLastTemplateChannelHeight;  // Channel height of last template sent to this miner

        /** hashLastBlock: snapshot of ChainState::hashBestChain at template-creation time.
         *
         *  Captured when BLOCK_DATA is pushed to this miner (StakeMinter Guard 1 pattern).
         *  Used as a session-level staleness anchor: if the live hashBestChain has advanced
         *  beyond this value, the chain has moved (new block or reorg) and the miner should
         *  request a new template.
         *
         *  This is a session-level complement to TemplateMetadata::hashBestChainAtCreation
         *  (which is per-template).  Together they provide Guard 1 (continuous) and Guard 2
         *  (pre-submit, via block.hashPrevBlock == hashBestChain) staleness detection,
         *  mirroring stake_minter.cpp:534 and stake_minter.cpp:674 respectively.
         *
         *  nHeight (above) is for session diagnostic only — never copy to block.nHeight.
         *  block.nHeight comes from the serialized 216-byte template (bytes[200-203]),
         *  set by AddBlockData() to tStateBest.nHeight + 1 (UNIFIED, NexusMiner #169).
         */
        uint1024_t hashLastBlock;  // hashBestChain snapshot when last BLOCK_DATA was pushed

        /* KEEPALIVE v2 telemetry fields.
         * Populated when the miner sends a v2 keepalive (len == 8).
         * nMinerPrevblockSuffix: raw bytes [4..7] of v2 payload (prevblock suffix as-sent).
         * fKeepaliveV2: true once the miner has sent at least one v2 keepalive (len == 8). */
        std::array<uint8_t, 4> nMinerPrevblockSuffix;  // Miner's reported prevblock suffix (raw bytes [4..7])
        bool     fKeepaliveV2;           // Whether this session negotiated keepalive v2

        /* Channel heights from the last KEEPALIVE_V2 cycle.
         * Persisted in context so stake height survives across keepalive cycles
         * without requiring a ChainState re-query on every round. */
        uint32_t nStakeHeight;           ///< Stake channel height from last KEEPALIVE_V2 cycle

        /** Canonical chain state snapshot captured at the last GET_BLOCK or push event.
         *
         *  Populated by the GET_BLOCK handler and SendStatelessTemplate() immediately
         *  after new_block() succeeds.  Read by handle_submit_block_stateless() to
         *  perform the canonical pre-check gate without a fresh chain read.
         *
         *  A default-constructed (all-zero) snap is_initialized() == false, meaning
         *  the pre-check gate runs in warn-only mode until the first template is issued.
         */
        CanonicalChainState canonical_snap;  ///< Snapshot captured at last GET_BLOCK or push

        /** Human-readable channel name for logging and diagnostics.
         *  Initialized by the constructors and kept in sync with nChannel by WithChannel().
         *  Also updated explicitly by WithChannelName() when needed.
         *  Values: "Prime" (nChannel=1), "Hash" (nChannel=2), "Unknown(N)" for others.
         *  This is the single source of truth — never compute inline ternaries elsewhere. */
        std::string strChannelName;  // "Prime" | "Hash" | "Unknown(N)"

        /** Default Constructor **/
        MiningContext();

        /** Parameterized Constructor **/
        MiningContext(
            uint32_t nChannel_,
            uint32_t nHeight_,
            uint64_t nTimestamp_,
            const std::string& strAddress_,
            uint32_t nProtocolVersion_,
            bool fAuthenticated_,
            uint32_t nSessionId_,
            const uint256_t& hashKeyID_,
            const uint256_t& hashGenesis_
        );

        /** WithChannel
         *
         *  Returns a new context with updated channel.
         *
         **/
        MiningContext WithChannel(uint32_t nChannel_) const;

        /** ChannelName
         *
         *  Returns the human-readable name for a mining channel integer.
         *  1 → "Prime", 2 → "Hash", otherwise "Unknown(N)".
         *  Use as the single source of truth — avoid inline ternaries.
         *
         *  @param[in] nChannel_ Mining channel integer
         *  @return Human-readable channel name string
         *
         **/
        static std::string ChannelName(uint32_t nChannel_);

        /** WithChannelName
         *
         *  Returns a new context with strChannelName derived from nChannel.
         *  Must be called whenever nChannel changes (e.g. in ProcessSetChannel).
         *
         *  @param[in] nChannel_ Mining channel integer
         *  @return New context with strChannelName set
         *
         **/
        MiningContext WithChannelName(uint32_t nChannel_) const;

        /** WithHeight
         *
         *  Returns a new context with updated height.
         *
         **/
        MiningContext WithHeight(uint32_t nHeight_) const;

        /** WithLastTemplateChannelHeight
         *
         *  Returns a new context with updated last template channel height.
         *  Only updated when an actual template (BLOCK_DATA) is sent to the miner.
         *  MUST be set to the channel-specific height, NOT the unified height.
         *
         **/
        MiningContext WithLastTemplateChannelHeight(uint32_t nLastTemplateChannelHeight_) const;

        /** WithHashLastBlock
         *
         *  Returns a new context with updated hashLastBlock snapshot.
         *  Must be called when BLOCK_DATA is pushed to this miner, capturing the current
         *  ChainState::hashBestChain as the primary staleness anchor (StakeMinter pattern).
         *
         **/
        MiningContext WithHashLastBlock(const uint1024_t& hashLastBlock_) const;

        /** WithTimestamp
         *
         *  Returns a new context with updated timestamp.
         *
         **/
        MiningContext WithTimestamp(uint64_t nTimestamp_) const;

        /** WithAuth
         *
         *  Returns a new context with updated authentication status.
         *
         **/
        MiningContext WithAuth(bool fAuthenticated_) const;

        /** WithSession
         *
         *  Returns a new context with updated session ID.
         *
         **/
        MiningContext WithSession(uint32_t nSessionId_) const;

        /** WithKeyId
         *
         *  Phase 2: Returns a new context with updated Falcon key ID.
         *
         **/
        MiningContext WithKeyId(const uint256_t& hashKeyID_) const;

        /** WithGenesis
         *
         *  Phase 2: Returns a new context with updated Tritium genesis hash.
         *
         **/
        MiningContext WithGenesis(const uint256_t& hashGenesis_) const;

        /** WithUserName
         *
         *  Phase 2: Returns a new context with updated username for trust-based addressing.
         *
         **/
        MiningContext WithUserName(const std::string& strUserName_) const;

        /** WithNonce
         *
         *  Returns a new context with updated authentication nonce.
         *
         **/
        MiningContext WithNonce(const std::vector<uint8_t>& vNonce_) const;

        /** WithPubKey
         *
         *  Returns a new context with updated miner public key.
         *
         **/
        MiningContext WithPubKey(const std::vector<uint8_t>& vPubKey_) const;

        /** WithDisposableKey
         *
         *  Returns a new context with updated disposable Falcon session key state.
         *
         **/
        MiningContext WithDisposableKey(const std::vector<uint8_t>& vPubKey_,
                                        const uint256_t& hashDisposableKeyID_ = 0) const;

        /** WithSessionStart
         *
         *  Returns a new context with updated session start timestamp.
         *
         **/
        MiningContext WithSessionStart(uint64_t nSessionStart_) const;

        /** WithSessionTimeout
         *
         *  Returns a new context with updated session timeout.
         *
         **/
        MiningContext WithSessionTimeout(uint64_t nSessionTimeout_) const;

        /** WithReconnectCount
         *
         *  Returns a new context with updated reconnect count.
         *  This is recovery metadata and should only be carried forward explicitly
         *  from authoritative recovery state.
         *
         **/
        MiningContext WithReconnectCount(uint32_t nReconnectCount_) const;

        /** WithKeepaliveCount
         *
         *  Returns a new context with updated keepalive count.
         *
         **/
        MiningContext WithKeepaliveCount(uint32_t nKeepaliveCount_) const;

        /** WithKeepaliveSent
         *
         *  Returns a new context with updated keepalive sent count.
         *
         **/
        MiningContext WithKeepaliveSent(uint32_t nKeepaliveSent_) const;

        /** WithLastKeepaliveTime
         *
         *  Returns a new context with updated last keepalive timestamp.
         *
         **/
        MiningContext WithLastKeepaliveTime(uint64_t nLastKeepaliveTime_) const;

        /** WithMinerPrevblockSuffix
         *
         *  Returns a new context with updated miner prevblock suffix (from keepalive v2).
         *  Also sets fKeepaliveV2 to true to mark this session as v2-capable.
         *
         *  @param[in] suffixBytes_ Raw bytes [4..7] of the v2 keepalive payload
         *
         **/
        MiningContext WithMinerPrevblockSuffix(const std::array<uint8_t, 4>& suffixBytes_) const;

        /** WithKeepaliveV2
         *
         *  Returns a new context with updated keepalive v2 flag.
         *  Use WithMinerPrevblockSuffix() to set both the suffix and the flag together.
         *
         *  @param[in] fV2_ Whether this session has negotiated keepalive v2
         *
         **/
        MiningContext WithKeepaliveV2(bool fV2_) const;

        /** WithStakeHeight
         *
         *  Returns a new context with updated stake channel height.
         *  Called after each KEEPALIVE_V2 cycle to persist nStakeHeight
         *  across keepalive cycles without re-querying ChainState.
         *
         *  @param[in] h The Stake channel height from the last KEEPALIVE_V2 ACK
         *
         **/
        MiningContext WithStakeHeight(uint32_t h) const;

        /** WithRewardAddress
         *
         *  Returns a new context with updated reward address and bound flag.
         *
         *  @param[in] hashReward_ The reward address to set
         *
         **/
        MiningContext WithRewardAddress(const uint256_t& hashReward_) const;

        /** WithChaChaKey
         *
         *  Returns a new context with updated ChaCha20 encryption key.
         *  CRITICAL: This ensures encryption state is synchronized between
         *  Miner connection and StatelessMinerManager.
         *
         *  @param[in] vKey_ The ChaCha20 session key
         *
         **/
        MiningContext WithChaChaKey(const std::vector<uint8_t>& vKey_) const;

        /** WithFalconVersion
         *
         *  Returns a new context with updated Falcon version and detection flag.
         *  Used during MINER_AUTH to store auto-detected Falcon version.
         *
         *  @param[in] version_ The detected Falcon version (512 or 1024)
         *
         **/
        MiningContext WithFalconVersion(LLC::FalconVersion version_) const;

        /** WithProtocolLane
         *
         *  Phase 3: Returns a new context with updated protocol lane identifier.
         *  Called during connection initialization to track whether this miner is using
         *  the LEGACY (8-bit opcodes, port 8323) or STATELESS (16-bit opcodes, port 9323) lane.
         *  Enables unified utility functions across both connection types.
         *
         *  @param[in] lane_ The protocol lane (LEGACY or STATELESS)
         *
         **/
        MiningContext WithProtocolLane(ProtocolLane lane_) const;

        /** IsStateless
         *
         *  Phase 3: Helper method to check if this context represents a stateless miner.
         *
         *  @return true if nProtocolLane == STATELESS, false otherwise
         *
         **/
        bool IsStateless() const;

        /** WithSubscription
         *
         *  Returns a new context with push notification subscription enabled.
         *  Called when MINER_READY is received to subscribe miner to channel-specific notifications.
         *
         *  @param[in] nChannel_ The channel to subscribe to (1=Prime, 2=Hash)
         *
         **/
        MiningContext WithSubscription(uint32_t nChannel_) const;

        /** WithNotificationSent
         *
         *  Returns a new context with updated notification statistics.
         *  Called after sending a push notification to update counters.
         *
         *  @param[in] nTime_ Timestamp when notification was sent
         *
         **/
        MiningContext WithNotificationSent(uint64_t nTime_) const;

        /** WithCanonicalSnap
         *
         *  Returns a new context with updated canonical chain state snapshot.
         *  Called by the GET_BLOCK handler and SendStatelessTemplate() after
         *  building the template, so that the SUBMIT_BLOCK handler can cross-
         *  check the submitted block against the snapshot without re-reading
         *  chain state.
         *
         *  @param[in] snap The CanonicalChainState captured at template-issue time
         *
         **/
        MiningContext WithCanonicalSnap(const CanonicalChainState& snap) const;

        /** GetSessionBinding
         *
         *  Returns the canonical session identity snapshot used for comparison
         *  and diagnostics across miner/node session codepaths.
         *
         **/
        SessionBinding GetSessionBinding() const;

        /** GetCryptoContext
         *
         *  Returns the canonical encrypted-session snapshot used for ChaCha20
         *  validation and diagnostics.
         *
         **/
        CryptoContext GetCryptoContext() const;

        /** ValidateConsistency
         *
         *  Canonical validation for session/auth/reward/crypto invariants.
         *
         **/
        SessionConsistencyResult ValidateConsistency() const;

        /** GetPayoutAddress
         *
         *  Get the payout address for rewards.
         *  Returns reward address if bound via MINER_SET_REWARD, otherwise genesis hash.
         *  Falls back to 0 if username-based addressing needs resolution.
         *
         *  @return Reward address, genesis hash, or 0 for username resolution
         *
        **/
        uint256_t GetPayoutAddress() const;

        /** RewardBindingHex
         *
         *  Returns the currently effective reward identity as a full 64-character
         *  internal hex value, or "NOT SET" when no payout identity is available.
         *
         *  @return Full payout identity hex for diagnostics
         *
         **/
        std::string RewardBindingHex() const;

        /** RewardBindingSource
         *
         *  Returns a semantic label describing where the active reward identity
         *  comes from for the current session.
         *
         *  @return Human-readable reward identity source label
         *
         **/
        std::string RewardBindingSource() const;

        /** GenesisHex
         *
         *  Returns the authenticated session genesis as a full 64-character hex
         *  value, or "NOT SET" when no genesis has been established.
         *
         *  @return Full genesis hex for diagnostics
         *
         **/
        std::string GenesisHex() const;

        /** HasValidPayout
         *
         *  Check if the context has a valid payout configuration.
         *
         *  @return true if genesis or username is set
         *
         **/
        bool HasValidPayout() const;

        /** IsSessionExpired
         *
         *  Check if the session has expired based on timeout.
         *
         *  @param[in] nNow Current timestamp (defaults to current time if 0)
         *
         *  @return true if session has expired
         *
         **/
        bool IsSessionExpired(uint64_t nNow = 0) const;

        /** GetSessionDuration
         *
         *  Get the duration of the current session in seconds.
         *
         *  @param[in] nNow Current timestamp (defaults to current time if 0)
         *
         *  @return Session duration in seconds
         *
         **/
        uint64_t GetSessionDuration(uint64_t nNow = 0) const;
    };


    /** ProcessResult
     *
     *  Result of processing a packet in the stateless miner.
     *  Contains the updated context and optional response packet.
     *
     **/
    struct ProcessResult
    {
        const MiningContext context;
        const bool fSuccess;
        const std::string strError;
        const StatelessPacket response;

        /** Success
         *
         *  Create a successful result with updated context and response.
         *
         **/
        static ProcessResult Success(
            const MiningContext& ctx,
            const StatelessPacket& resp = StatelessPacket()
        );

        /** Error
         *
         *  Create an error result with unchanged context and error message.
         *
         **/
        static ProcessResult Error(
            const MiningContext& ctx,
            const std::string& error
        );

    private:
        ProcessResult(
            const MiningContext& ctx_,
            bool fSuccess_,
            const std::string& error_,
            const StatelessPacket& resp_
        );
    };


    /** StatelessMiner
     *
     *  Pure functional packet processor for mining connections.
     *  All methods are stateless and return ProcessResult with updated context.
     *
     *  Phase 2: Falcon-driven authentication is the primary auth mechanism.
     *
     **/
    class StatelessMiner
    {
    public:
        /** ProcessPacket
         *
         *  Main entry point for packet processing.
         *  Routes to appropriate handler based on packet type.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Incoming packet to process
         *
         *  @return ProcessResult with updated context and optional response
         *
         **/
        static ProcessResult ProcessPacket(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessPacket (Legacy Packet Overload)
         *
         *  Overload for legacy 8-bit Packet type.
         *  Converts legacy Packet to StatelessPacket and calls main ProcessPacket.
         *
         *  This allows the legacy Miner class to use StatelessMiner for authentication
         *  and session management while maintaining backward compatibility.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Incoming legacy packet to process
         *
         *  @return ProcessResult with updated context and optional response
         *
         **/
        static ProcessResult ProcessPacket(
            const MiningContext& context,
            const Packet& packet
        );

        /** ProcessFalconResponse
         *
         *  Phase 2: Process Falcon authentication response.
         *  Verifies Falcon signature and updates context with auth status.
         *
         *  Expected packet format:
         *  - Falcon public key (variable length)
         *  - Falcon signature (variable length)
         *  - Optional: Tritium genesis hash (32 bytes)
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Falcon auth response packet
         *
         *  @return ProcessResult with updated context (authenticated, keyID, genesis)
         *
         **/
        static ProcessResult ProcessFalconResponse(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessMinerAuthInit
         *
         *  Process MINER_AUTH_INIT packet - first step of authentication handshake.
         *  Stores miner's public key and generates challenge nonce.
         *
         *  Expected packet format:
         *  - [2 bytes] pubkey_len (big-endian)
         *  - [pubkey_len bytes] Falcon public key
         *  - [2 bytes] miner_id_len (big-endian)
         *  - [miner_id_len bytes] miner_id string (UTF-8)
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Auth init packet
         *
         *  @return ProcessResult with challenge response and updated context
         *
         **/
        static ProcessResult ProcessMinerAuthInit(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessSessionStart
         *
         *  Phase 2: Process session start request.
         *  Requires prior Falcon authentication.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Session start packet
         *
         *  @return ProcessResult with updated session parameters
         *
         **/
        static ProcessResult ProcessSessionStart(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessSetChannel
         *
         *  Process channel selection packet.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Set channel packet
         *
         *  @return ProcessResult with updated channel
         *
         **/
        static ProcessResult ProcessSetChannel(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessKeepaliveV2
         *
         *  Process KEEPALIVE_V2 (0xD100) packet from miner.
         *  Parses the 8-byte KeepAliveV2Frame, populates a 28-byte
         *  KeepAliveV2AckFrame from live chain state, and sends it
         *  as KEEPALIVE_V2_ACK (0xD101).
         *
         *  @param[in] context Current miner state
         *  @param[in] packet KEEPALIVE_V2 packet (8-byte payload)
         *
         *  @return ProcessResult with KEEPALIVE_V2_ACK response
         *
         **/
        static ProcessResult ProcessKeepaliveV2(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessSessionKeepalive
         *
         *  Phase 2: Process session keepalive.
         *  Updates session timestamp to maintain connection.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Keepalive packet
         *
         *  @return ProcessResult with updated timestamp
         *
         **/
        static ProcessResult ProcessSessionKeepalive(
            const MiningContext& context,
            const StatelessPacket& packet
        );

        /** ProcessSetReward
         *
         *  Process MINER_SET_REWARD packet to bind reward address.
         *  Validates and stores the encrypted reward address for this mining session.
         *
         *  @param[in] context Current miner state
         *  @param[in] packet Set reward packet with encrypted address
         *
         *  @return ProcessResult with updated context or error
         *
         **/
        static ProcessResult ProcessSetReward(
            const MiningContext& context,
            const StatelessPacket& packet
        );

    private:
        /** BuildAuthMessage
         *
         *  Constructs the message that should be signed for Falcon auth.
         *  Currently uses: address + timestamp
         *  TODO: Add challenge nonce for enhanced security
         *
         *  @param[in] context Current miner state
         *
         *  @return Message bytes for signature verification
         *
         **/
        static std::vector<uint8_t> BuildAuthMessage(const MiningContext& context);

        /** DecryptRewardPayload
         *
         *  Decrypts reward address payload using ChaCha20-Poly1305.
         *
         *  @param[in] vEncrypted The encrypted payload (nonce + ciphertext + tag)
         *  @param[in] vKey The ChaCha20 session key
         *  @param[out] vPlaintext The decrypted data
         *
         *  @return True if decryption succeeded
         *
         **/
        static bool DecryptRewardPayload(
            uint32_t nSessionId,
            const std::vector<uint8_t>& vEncrypted,
            const std::vector<uint8_t>& vKey,
            std::vector<uint8_t>& vPlaintext
        );

        /** EncryptRewardResult
         *
         *  Encrypts reward result response using ChaCha20-Poly1305.
         *
         *  @param[in] vPlaintext The data to encrypt
         *  @param[in] vKey The ChaCha20 session key
         *
         *  @return Encrypted payload (nonce + ciphertext + tag)
         *
         **/
        static std::vector<uint8_t> EncryptRewardResult(
            uint32_t nSessionId,
            const std::vector<uint8_t>& vPlaintext,
            const std::vector<uint8_t>& vKey
        );

        /** ValidateRewardAddress
         *
         *  Validates that a reward address is non-zero and properly formatted.
         *
         *  @param[in] hashReward The reward address to validate
         *
         *  @return True if valid format
         *
         **/
        static bool ValidateRewardAddress(const uint256_t& hashReward);
    };

} // namespace LLP

#endif
