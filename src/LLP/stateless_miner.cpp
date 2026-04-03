/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_miner.h>
#include <LLP/packets/packet.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/falcon_verify.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/genesis_constants.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/stateless_opcodes.h>
#include <LLP/include/session_start_packet.h>
#include <LLP/include/keepalive_v2.h>
#include <LLP/include/colin_mining_agent.h>

#include <LLD/include/global.h>

#include <LLC/include/random.h>
#include <LLC/include/flkey.h>
#include <LLC/include/falcon_constants_v2.h>
#include <LLC/include/encrypt.h>
#include <LLC/include/chacha20_helpers.h>
#include <LLC/include/mining_session_keys.h>
#include <LLC/hash/SK.h>

#include <TAO/Ledger/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/retarget.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/config.h>
#include <Util/include/hex.h>

#include <openssl/sha.h>  // For SHA256() function - must match NexusMiner's implementation

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

namespace LLP
{
    namespace
    {
        using Diagnostics::FullHexOrUnset;
        using Diagnostics::KeyFingerprint;
        using Diagnostics::YesNo;
        using Diagnostics::PassFail;

        static const char* const SESSION_CONSISTENCY_RESULT_STRINGS[] =
        {
            "Ok",
            "MissingSessionId",
            "MissingGenesis",
            "MissingFalconKey",
            "RewardBoundMissingHash",
            "EncryptionReadyMissingKey",
            "SessionIdMismatch",
            "GenesisMismatch",
            "FalconKeyMismatch"
        };

        static_assert(
            (static_cast<size_t>(SessionConsistencyResult::FalconKeyMismatch) + 1)
                == (sizeof(SESSION_CONSISTENCY_RESULT_STRINGS) / sizeof(SESSION_CONSISTENCY_RESULT_STRINGS[0])),
            "SessionConsistencyResult string table must stay aligned with enum ordering");
    }

    /* Default session timeout in seconds for mining sessions.
     * This is the inactivity timeout - sessions expire if no keepalive
     * is received within this window. Set to 24 hours: miners send
     * 2 KeepAliveV2 per 24 hours by default, and only 1 is required
     * to keep the session alive. Network-only disconnect policy:
     * only disconnect on actual TCP errors, not idle time. */
    static const uint64_t DEFAULT_SESSION_TIMEOUT = 86400;

    /* AAD (Additional Authenticated Data) strings for ChaCha20-Poly1305 AEAD
     * These provide domain separation between different packet types */
    static const std::vector<uint8_t> AAD_REWARD_ADDRESS{'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};
    static const std::vector<uint8_t> AAD_REWARD_RESULT{'R','E','W','A','R','D','_','R','E','S','U','L','T'};

    const char* SessionConsistencyResultString(const SessionConsistencyResult result)
    {
        const auto nIndex = static_cast<size_t>(result);
        const auto nCount = sizeof(SESSION_CONSISTENCY_RESULT_STRINGS) / sizeof(SESSION_CONSISTENCY_RESULT_STRINGS[0]);

        if(nIndex < nCount)
            return SESSION_CONSISTENCY_RESULT_STRINGS[nIndex];

        return "Unknown";
    }

    /* Default constructor */
    MiningContext::MiningContext()
    : nChannel(0)
    , nHeight(0)
    , nTimestamp(0)
    , strAddress("")
    , nProtocolVersion(0)
    , fAuthenticated(false)
    , nSessionId(0)
    , hashKeyID(0)
    , hashGenesis(0)
    , strUserName("")
    , vAuthNonce()
    , vMinerPubKey()
    , vDisposablePubKey()
    , hashDisposableKeyID(0)
    , nSessionStart(0)
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nReconnectCount(0)
    , nKeepaliveCount(0)
    , nKeepaliveSent(0)
    , nLastKeepaliveTime(0)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , vChaChaKey()
    , fEncryptionReady(false)
    , nFalconVersion(LLC::FalconVersion::FALCON_512)
    , fFalconVersionDetected(false)
    , nProtocolLane(ProtocolLane::LEGACY)  // Default to LEGACY lane
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastNotificationTime(0)
    , nNotificationsSent(0)
    , nLastTemplateChannelHeight(0)
    , hashLastBlock(0)
    , nMinerPrevblockSuffix({})
    , nStakeHeight(0)
    , strChannelName("Unknown")
    {
    }

    /* Parameterized constructor */
    MiningContext::MiningContext(
        uint32_t nChannel_,
        uint32_t nHeight_,
        uint64_t nTimestamp_,
        const std::string& strAddress_,
        uint32_t nProtocolVersion_,
        bool fAuthenticated_,
        uint32_t nSessionId_,
        const uint256_t& hashKeyID_,
        const uint256_t& hashGenesis_
    )
    : nChannel(nChannel_)
    , nHeight(nHeight_)
    , nTimestamp(nTimestamp_)
    , strAddress(strAddress_)
    , nProtocolVersion(nProtocolVersion_)
    , fAuthenticated(fAuthenticated_)
    , nSessionId(nSessionId_)
    , hashKeyID(hashKeyID_)
    , hashGenesis(hashGenesis_)
    , strUserName("")
    , vAuthNonce()
    , vMinerPubKey()
    , vDisposablePubKey()
    , hashDisposableKeyID(0)
    , nSessionStart(0)
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nReconnectCount(0)
    , nKeepaliveCount(0)
    , nKeepaliveSent(0)
    , nLastKeepaliveTime(0)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , vChaChaKey()
    , fEncryptionReady(false)
    , nFalconVersion(LLC::FalconVersion::FALCON_512)
    , fFalconVersionDetected(false)
    , nProtocolLane(ProtocolLane::LEGACY)  // Default to LEGACY lane
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastNotificationTime(0)
    , nNotificationsSent(0)
    , nLastTemplateChannelHeight(0)
    , hashLastBlock(0)
    , nMinerPrevblockSuffix({})
    , nStakeHeight(0)
    , strChannelName(MiningContext::ChannelName(nChannel_))
    {
    }

    /* Immutable update methods */
    MiningContext MiningContext::WithChannel(uint32_t nChannel_) const
    {
        MiningContext c = *this;
        c.nChannel = nChannel_;
        c.strChannelName = ChannelName(nChannel_);
        return c;
    }

    /* static */
    std::string MiningContext::ChannelName(uint32_t nChannel_)
    {
        switch(nChannel_)
        {
            case 1: return "Prime";
            case 2: return "Hash";
            default: return "Unknown(" + std::to_string(nChannel_) + ")";
        }
    }

    MiningContext MiningContext::WithChannelName(uint32_t nChannel_) const
    {
        MiningContext c = *this;
        c.strChannelName = ChannelName(nChannel_);
        return c;
    }

    MiningContext MiningContext::WithHeight(uint32_t nHeight_) const
    {
        MiningContext c = *this;
        c.nHeight = nHeight_;
        return c;
    }

    MiningContext MiningContext::WithLastTemplateChannelHeight(uint32_t nLastTemplateChannelHeight_) const
    {
        MiningContext c = *this;
        c.nLastTemplateChannelHeight = nLastTemplateChannelHeight_;
        return c;
    }

    MiningContext MiningContext::WithHashLastBlock(const uint1024_t& hashLastBlock_) const
    {
        MiningContext c = *this;
        c.hashLastBlock = hashLastBlock_;
        return c;
    }

    MiningContext MiningContext::WithTimestamp(uint64_t nTimestamp_) const
    {
        MiningContext c = *this;
        c.nTimestamp = nTimestamp_;
        return c;
    }

    MiningContext MiningContext::WithAuth(bool fAuthenticated_) const
    {
        MiningContext c = *this;
        c.fAuthenticated = fAuthenticated_;
        return c;
    }

    MiningContext MiningContext::WithSession(uint32_t nSessionId_) const
    {
        MiningContext c = *this;
        c.nSessionId = nSessionId_;
        return c;
    }

    MiningContext MiningContext::WithKeyId(const uint256_t& hashKeyID_) const
    {
        MiningContext c = *this;
        c.hashKeyID = hashKeyID_;
        return c;
    }

    MiningContext MiningContext::WithGenesis(const uint256_t& hashGenesis_) const
    {
        MiningContext c = *this;
        c.hashGenesis = hashGenesis_;
        return c;
    }

    MiningContext MiningContext::WithUserName(const std::string& strUserName_) const
    {
        MiningContext c = *this;
        c.strUserName = strUserName_;
        return c;
    }

    MiningContext MiningContext::WithNonce(const std::vector<uint8_t>& vNonce_) const
    {
        MiningContext c = *this;
        c.vAuthNonce = vNonce_;
        return c;
    }

    MiningContext MiningContext::WithPubKey(const std::vector<uint8_t>& vPubKey_) const
    {
        MiningContext c = *this;
        c.vMinerPubKey = vPubKey_;
        return c;
    }

    MiningContext MiningContext::WithDisposableKey(
        const std::vector<uint8_t>& vPubKey_,
        const uint256_t& hashDisposableKeyID_) const
    {
        MiningContext c = *this;
        c.vDisposablePubKey = vPubKey_;
        c.hashDisposableKeyID = (!vPubKey_.empty() && hashDisposableKeyID_ == 0)
            ? LLC::SK256(vPubKey_)
            : hashDisposableKeyID_;
        return c;
    }

    MiningContext MiningContext::WithSessionStart(uint64_t nSessionStart_) const
    {
        MiningContext c = *this;
        c.nSessionStart = nSessionStart_;
        return c;
    }

    MiningContext MiningContext::WithSessionTimeout(uint64_t nSessionTimeout_) const
    {
        MiningContext c = *this;
        c.nSessionTimeout = nSessionTimeout_;
        return c;
    }

    MiningContext MiningContext::WithReconnectCount(uint32_t nReconnectCount_) const
    {
        MiningContext c = *this;
        c.nReconnectCount = nReconnectCount_;
        return c;
    }

    MiningContext MiningContext::WithKeepaliveCount(uint32_t nKeepaliveCount_) const
    {
        MiningContext c = *this;
        c.nKeepaliveCount = nKeepaliveCount_;
        return c;
    }

    MiningContext MiningContext::WithKeepaliveSent(uint32_t nKeepaliveSent_) const
    {
        MiningContext c = *this;
        c.nKeepaliveSent = nKeepaliveSent_;
        return c;
    }

    MiningContext MiningContext::WithLastKeepaliveTime(uint64_t nLastKeepaliveTime_) const
    {
        MiningContext c = *this;
        c.nLastKeepaliveTime = nLastKeepaliveTime_;
        return c;
    }

    MiningContext MiningContext::WithMinerPrevblockSuffix(const std::array<uint8_t, 4>& suffixBytes_) const
    {
        MiningContext c = *this;
        c.nMinerPrevblockSuffix = suffixBytes_;
        return c;
    }

    MiningContext MiningContext::WithStakeHeight(uint32_t h) const
    {
        MiningContext c = *this;
        c.nStakeHeight = h;
        return c;
    }

    MiningContext MiningContext::WithRewardAddress(const uint256_t& hashReward_) const
    {
        MiningContext c = *this;
        c.hashRewardAddress = hashReward_;
        c.fRewardBound = true;
        return c;
    }

    MiningContext MiningContext::WithChaChaKey(const std::vector<uint8_t>& vKey_) const
    {
        MiningContext c = *this;
        c.vChaChaKey = vKey_;
        c.fEncryptionReady = !vKey_.empty();
        return c;
    }

    MiningContext MiningContext::WithFalconVersion(LLC::FalconVersion version_) const
    {
        MiningContext c = *this;
        c.nFalconVersion = version_;
        c.fFalconVersionDetected = true;
        return c;
    }

    MiningContext MiningContext::WithProtocolLane(ProtocolLane lane_) const
    {
        MiningContext c = *this;
        c.nProtocolLane = lane_;
        return c;
    }

    bool MiningContext::IsStateless() const
    {
        return nProtocolLane == ProtocolLane::STATELESS;
    }

    MiningContext MiningContext::WithSubscription(uint32_t nChannel_) const
    {
        MiningContext c = *this;
        c.fSubscribedToNotifications = true;
        c.nSubscribedChannel = nChannel_;
        c.nLastNotificationTime = 0;
        c.nNotificationsSent = 0;
        return c;
    }

    MiningContext MiningContext::WithNotificationSent(uint64_t nTime_) const
    {
        MiningContext c = *this;
        c.nLastNotificationTime = nTime_;
        c.nNotificationsSent = c.nNotificationsSent + 1;
        return c;
    }

    MiningContext MiningContext::WithCanonicalSnap(const CanonicalChainState& snap) const
    {
        MiningContext c = *this;
        c.canonical_snap = snap;
        return c;
    }

    SessionBinding MiningContext::GetSessionBinding() const
    {
        SessionBinding binding;
        binding.nSessionId = nSessionId;
        binding.hashGenesis = hashGenesis;
        binding.hashKeyID = hashKeyID;
        binding.hashRewardAddress = hashRewardAddress;
        binding.nProtocolLane = nProtocolLane;
        binding.strKeyFingerprint = Diagnostics::KeyFingerprint(vMinerPubKey);
        binding.fRewardBound = fRewardBound;
        return binding;
    }

    CryptoContext MiningContext::GetCryptoContext() const
    {
        CryptoContext crypto;
        crypto.vChaCha20Key = vChaChaKey;
        crypto.strKeyFingerprint = Diagnostics::KeyFingerprint(vChaChaKey);
        crypto.nSessionId = nSessionId;
        crypto.hashGenesis = hashGenesis;
        crypto.hashKeyID = hashKeyID;
        crypto.nProtocolLane = nProtocolLane;
        crypto.fEncryptionReady = fEncryptionReady;
        return crypto;
    }

    SessionConsistencyResult MiningContext::ValidateConsistency() const
    {
        if(fAuthenticated && nSessionId == 0)
            return SessionConsistencyResult::MissingSessionId;

        if(fAuthenticated && hashGenesis == 0)
            return SessionConsistencyResult::MissingGenesis;

        if(fAuthenticated && hashKeyID == 0)
            return SessionConsistencyResult::MissingFalconKey;

        if(fRewardBound && hashRewardAddress == 0)
            return SessionConsistencyResult::RewardBoundMissingHash;

        if(fEncryptionReady && vChaChaKey.empty())
            return SessionConsistencyResult::EncryptionReadyMissingKey;

        return SessionConsistencyResult::Ok;
    }

    uint256_t MiningContext::GetPayoutAddress() const
    {
        /* Priority 1: Use explicit reward address if bound via MINER_SET_REWARD.
         * This address MUST be a TritiumGenesis (UserType) — Register Addresses are
         * NOT valid coinbase recipients and will cause Coinbase::Verify() to fail on
         * all network peers. ValidateRewardAddress() enforces this at bind time. */
        if(fRewardBound && hashRewardAddress != 0)
        {
            return hashRewardAddress;
        }
        
        /* Priority 2: Fall back to genesis hash (legacy behavior from v5.1+)
         * The genesis hash is always a valid TritiumGenesis by construction —
         * it is validated during MINER_AUTH_INIT via GenesisConstants::ValidateGenesis()
         * which checks the UserType byte. This fallback is therefore always safe.
         */
        if(hashGenesis != 0)
        {
            return hashGenesis;
        }
        
        /* Priority 3: No valid payout address available */
        debug::error(FUNCTION, "GetPayoutAddress: No valid payout address!");
        debug::error(FUNCTION, "  Neither reward address nor genesis hash is set");
        return uint256_t(0);
    }

    std::string MiningContext::RewardBindingHex() const
    {
        if(fRewardBound && hashRewardAddress != 0)
            return FullHexOrUnset(hashRewardAddress);

        if(hashGenesis != 0)
            return FullHexOrUnset(hashGenesis);

        return "NOT SET";
    }

    std::string MiningContext::RewardBindingSource() const
    {
        if(fRewardBound && hashRewardAddress != 0)
            return "current session reward binding";

        if(hashGenesis != 0)
            return "session genesis fallback";

        return "not configured";
    }

    std::string MiningContext::GenesisHex() const
    {
        return FullHexOrUnset(hashGenesis);
    }

    bool MiningContext::HasValidPayout() const
    {
        return hashGenesis != 0 || !strUserName.empty();
    }

    bool MiningContext::IsSessionExpired(uint64_t nNow) const
    {
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Session never started means it's "expired" */
        if(nSessionStart == 0)
            return true;

        /* Check if time since last activity exceeds timeout.
         * This is an activity-based timeout (like keepalive):
         * - nTimestamp is updated on each keepalive/activity
         * - Session expires if no activity within nSessionTimeout */
        return (nNow - nTimestamp) > nSessionTimeout;
    }

    uint64_t MiningContext::GetSessionDuration(uint64_t nNow) const
    {
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* No session started */
        if(nSessionStart == 0)
            return 0;

        /* Return time since session start */
        return nNow - nSessionStart;
    }


    /* ═══════════════════════════════════════════════════════════════════════════════════ */
    /* PR #134: CORRECTED TEMPLATE STALENESS DETECTION                                     */
    /* ═══════════════════════════════════════════════════════════════════════════════════ */
    
    /**
     *  TemplateMetadata::IsStale (CORRECTED in PR #134)
     *  
     *  Implements channel-specific staleness detection to eliminate ~40% false positives.
     *  
     *  CRITICAL FIX: Now checks channel-specific height instead of unified blockchain height.
     *  This prevents templates from being incorrectly marked stale when other channels mine blocks.
     *  
     *  STALENESS LOGIC:
     *  ================
     *  
     *  1. PRIMARY CHECK: Channel height comparison
     *     - Get current blockchain state's channel height for this template's channel
     *     - Template is stale if: blockchain_channel_height != template_channel_height - 1
     *     - Rationale: Template is for NEXT block in channel (template_channel_height)
     *                  If blockchain already has that block, template is obsolete
     *  
     *  2. SECONDARY CHECK: Age timeout (600 second safety net)
     *     - Template is stale if older than 600 seconds
     *     - Handles edge cases: reorgs, network partitions, clock skew
     *  3. TERTIARY CHECK: Unified height drift guard
     *     - Template is stale if current unified height has advanced > 1 block beyond
     *       template creation height.  Catches the scenario where stake or opposite-
     *       channel blocks advance hashBestChain while the mining channel is unchanged.
     *  
     *  EXAMPLE SCENARIOS:
     *  ==================
     *  
     *  Scenario A: Template is FRESH (correct behavior)
     *  ------------------------------------------------
     *  Template: prime_channel=2165443, unified=6535198, age=10s
     *  Blockchain: prime_channel=2165443 (no new Prime blocks)
     *  Result: NOT STALE ✅ (blockchain is at 2165443, template mines 2165444)
     *  
     *  Scenario B: Template is STALE - same channel advanced (correct behavior)
     *  -------------------------------------------------------------------------
     *  Template: prime_channel=2165443, unified=6535198, age=10s
     *  Blockchain: prime_channel=2165444 (new Prime block mined!)
     *  Result: STALE ✅ (blockchain advanced, template obsolete)
     *  
     *  Scenario C: Template is FRESH - different channel advanced (FIXED in PR #134)
     *  ------------------------------------------------------------------------------
     *  Template: prime_channel=2165443, unified=6535198, age=10s
     *  Blockchain: prime_channel=2165443, but Hash channel mined → unified=6535199
     *  OLD BEHAVIOR: STALE ❌ (checked unified height, incorrect!)
     *  NEW BEHAVIOR: NOT STALE ✅ (checks prime channel height, correct!)
     *  
     *  Scenario D: Template is STALE - age timeout (safety net)
     *  ---------------------------------------------------------
     *  Template: prime_channel=2165443, unified=6535198, age=610s
     *  Blockchain: prime_channel=2165443 (no channel change)
     *  Result: STALE ✅ (age exceeded 600s limit)
     *  
     *  Scenario E: Template is STALE - unified height drift (FIXED here)
     *  ------------------------------------------------------------------
     *  Template: prime_channel=2346186, unified=6647130, age=874s
     *  Blockchain: prime_channel=2346186 (unchanged), but 4 stake/hash blocks mined
     *    → unified=6647134, hashBestChain has advanced 4 blocks
     *  OLD BEHAVIOR: NOT STALE ❌ (channel height unchanged, age<600s, hash check
     *    bypassed if hashBestChainAtCreation was zero — miner mined stale template)
     *  NEW BEHAVIOR: STALE ✅ (drift=4 > 1 — TERTIARY check fires; template discarded)
     */
    bool TemplateMetadata::IsStale(uint64_t nNow) const
    {
        /* ═══════════════════════════════════════════════════════════════════════════ */
        /* SECONDARY CHECK: Age-based timeout (600 second safety net)                  */
        /* ═══════════════════════════════════════════════════════════════════════════ */
        
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Handle uninitialized or invalid timestamps */
        if(nCreationTime == 0 || nNow < nCreationTime)
            return true;  // Treat as stale if timestamp is invalid

        /* Calculate age in seconds */
        uint64_t nAge = nNow - nCreationTime;

        /* Check age timeout - templates older than 600s are always stale */
        if(nAge > LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS)
            return true;  // STALE: Age exceeded safety timeout
        
        /* ═══════════════════════════════════════════════════════════════════════════ */
        /* PRIMARY CHECK: Hash-based chain tip comparison (StakeMinter pattern)        */
        /* ═══════════════════════════════════════════════════════════════════════════ */

        /* hashBestChainAtCreation == pBlock->hashPrevBlock at creation time.
         * If hashBestChain has advanced, the chain has moved (new block or reorg)
         * and this template is stale — even at the same integer channel height.
         * This mirrors StakeMinter line 674 and catches same-height reorgs. */
        if(hashBestChainAtCreation != uint1024_t(0) &&
           hashBestChainAtCreation != TAO::Ledger::ChainState::hashBestChain.load())
        {
            return true;  // STALE: hashBestChain advanced (new block or reorg)
        }

        /* ═══════════════════════════════════════════════════════════════════════════ */
        /* SECONDARY CHECK: Channel-specific height comparison (PR #134 FIX)           */
        /* ═══════════════════════════════════════════════════════════════════════════ */
        
        /* Get current blockchain state atomically */
        TAO::Ledger::BlockState stateCurrent = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Walk backward to find last block in this template's channel */
        if(!TAO::Ledger::GetLastState(stateCurrent, nChannel))
        {
            /* GetLastState failed (e.g., genesis block, invalid channel) */
            /* Mark as stale for safety - better to refresh than mine invalid data */
            return true;  // STALE: Channel lookup failed
        }
        
        /* Compare blockchain's channel height against template's channel height
         * 
         * Template represents the NEXT block in the channel:
         *   If template has nChannelHeight=2165443, it's mining block 2165443
         *   Blockchain should be at nChannelHeight=2165442 (previous block)
         * 
         * Therefore, template is fresh if:
         *   blockchain_channel_height == template_channel_height - 1
         * 
         * Template is stale if blockchain has advanced:
         *   blockchain_channel_height != template_channel_height - 1
         * 
         * Special case: If nChannelHeight is 0 (uninitialized or first block in channel),
         * we can't reliably determine staleness via channel height, so mark as stale for safety.
         */
        if(nChannelHeight == 0)
        {
            /* Template has no valid channel height - mark as stale for safety */
            return true;  // STALE: Invalid channel height
        }
        
        if(stateCurrent.nChannelHeight != nChannelHeight - 1)
        {
            /* Channel has advanced - template is obsolete */
            return true;  // STALE: Channel height mismatch
        }
        
        /* ═══════════════════════════════════════════════════════════════════════════ */
        /* TERTIARY CHECK: Unified height drift guard                                   */
        /* ═══════════════════════════════════════════════════════════════════════════ */

        /* A template becomes cryptographically stale if the unified chain tip has
         * advanced more than 1 block since creation, even when the same-channel
         * height is unchanged.  This catches the scenario where stake blocks or
         * opposite-channel blocks advance hashBestChain while the mining channel
         * height stays constant — the template's hashPrevBlock then points to a
         * stale chain tip and any submitted solution would be rejected by Guard 2
         * (hashPrevBlock != hashBestChain).
         *
         * Tolerance of 1 is intentional: a single unified-height advance between
         * template creation and this staleness check is normal racing behaviour
         * (another channel solved a block just after this template was built).
         * A drift of ≥ 2 means the miner is mining on an obsolete chain tip.
         *
         * Observed in the field (2026-03-25): unified drift of 4 (template.nHeight=
         * 6647130, current_best=6647134) caused 874 s of stale mining because
         * the channel height was unchanged and the previous checks did not fire. */
        const uint32_t nCurrentUnified = static_cast<uint32_t>(
            TAO::Ledger::ChainState::nBestHeight.load());
        if(nCurrentUnified > nHeight + 1)
        {
            debug::log(1, "TemplateMetadata::IsStale",
                       " UNIFIED DRIFT: template.nHeight=", nHeight,
                       " current_best=", nCurrentUnified,
                       " drift=", (nCurrentUnified - nHeight),
                       " (>1) — template hashPrevBlock is stale; marking STALE");
            return true;  // STALE: Unified height drift exceeded tolerance
        }

        /* ═══════════════════════════════════════════════════════════════════════════ */
        /* TEMPLATE IS FRESH                                                            */
        /* ═══════════════════════════════════════════════════════════════════════════ */
        
        /* Template passes all checks:
         * ✅ Age < 600 seconds
         * ✅ hashBestChain unchanged (hash-based chain tip comparison)
         * ✅ Channel height matches (blockchain is at N-1, template mines N)
         * ✅ Unified height drift within tolerance (≤ 1 block)
         */
        return false;  // FRESH: Template is valid for mining
    }

    
    /**
     *  TemplateMetadata::GetAge
     *  
     *  Calculate template age in seconds for diagnostics and timeout checks.
     */
    uint64_t TemplateMetadata::GetAge(uint64_t nNow) const
    {
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();
        
        /* Handle invalid timestamps */
        if(nCreationTime == 0 || nNow < nCreationTime)
            return 0;  // Return 0 for invalid/future timestamps
        
        /* Return age in seconds */
        return nNow - nCreationTime;
    }

    
    /**
     *  TemplateMetadata::GetChannelName
     *  
     *  Get human-readable channel name for logging and diagnostics.
     */
    std::string TemplateMetadata::GetChannelName() const
    {
        switch(nChannel)
        {
            case 0:  return "Stake";
            case 1:  return "Prime";
            case 2:  return "Hash";
            default: return "Unknown";
        }
    }


    /* ProcessResult private constructor */
    ProcessResult::ProcessResult(
        const MiningContext& ctx_,
        bool fSuccess_,
        const std::string& error_,
        const StatelessPacket& resp_
    )
    : context(ctx_)
    , fSuccess(fSuccess_)
    , strError(error_)
    , response(resp_)
    {
    }

    /* Static factory methods */
    ProcessResult ProcessResult::Success(const MiningContext& ctx, const StatelessPacket& resp)
    {
        return ProcessResult(ctx, true, "", resp);
    }

    ProcessResult ProcessResult::Error(const MiningContext& ctx, const std::string& error)
    {
        return ProcessResult(ctx, false, error, StatelessPacket());
    }


    /* Packet type definitions - must match miner.h and NexusMiner Phase 2 protocol
     * 
     * NOTE: Strict lane separation - stateless lane uses native 16-bit opcodes throughout.
     * All case statements and response packets use 16-bit opcodes from StatelessOpcodes::.
     */
    using namespace StatelessOpcodes;  // 16-bit opcodes for case statements and responses


    /* ProcessPacket overload for legacy 8-bit Packet type.
     * Converts legacy Packet to StatelessPacket for processing. */
    ProcessResult StatelessMiner::ProcessPacket(
        const MiningContext& context,
        const Packet& legacyPacket)
    {
        /* Convert legacy 8-bit Packet to 16-bit StatelessPacket.
         * Mirror the opcode so the main ProcessPacket switch handles it with 16-bit cases. */
        StatelessPacket packet;
        packet.HEADER = OpcodeUtility::Stateless::Mirror(static_cast<uint8_t>(legacyPacket.HEADER));
        packet.LENGTH = legacyPacket.LENGTH;
        packet.DATA = legacyPacket.DATA;

        /* Call main ProcessPacket with converted packet */
        ProcessResult result = ProcessPacket(context, packet);

        /* NOTE: result.response is a StatelessPacket, but the legacy Miner class
         * uses BaseConnection<Packet>, so WritePacket expects Packet type.
         * However, since both Packet and StatelessPacket have the same structure
         * (HEADER, LENGTH, DATA), and the legacy Miner needs the response as-is,
         * we leave result.response as a StatelessPacket. The Miner class will
         * need to handle this type mismatch by converting the response manually. */

        return result;
    }


    /* Main packet processing entry point */
    ProcessResult StatelessMiner::ProcessPacket(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        /* DEBUG: Log incoming packet details for Falcon handshake debugging */
        debug::log(3, FUNCTION, "ProcessPacket: opcode=0x", std::hex, packet.HEADER, std::dec, " length=", packet.LENGTH);

        /* DEBUG: Log packet bytes when verbose >= 5 */
        if(config::nVerbose >= 5)
        {
            DisposableFalcon::DebugLogPacket("ProcessPacket::incoming", packet.DATA, 5);
        }

        /* Keep native 16-bit format - strict lane separation, no conversion needed */
        uint16_t nRouteOpcode = packet.HEADER;

        /* Route based on packet type */
        /* Note: GET_BLOCK and SUBMIT_BLOCK are handled in StatelessMinerConnection */
        /* due to their need for stateful block management */
        switch(nRouteOpcode)
        {
            case AUTH_INIT:
                debug::log(2, FUNCTION, "Routing to ProcessMinerAuthInit");
                return ProcessMinerAuthInit(context, packet);

            case AUTH_RESPONSE:
                debug::log(2, FUNCTION, "Routing to ProcessFalconResponse");
                return ProcessFalconResponse(context, packet);

            case SESSION_START:
                debug::log(2, FUNCTION, "Routing to ProcessSessionStart");
                return ProcessSessionStart(context, packet);

            case SET_CHANNEL:
                debug::log(2, FUNCTION, "Routing to ProcessSetChannel");
                return ProcessSetChannel(context, packet);

            case SESSION_KEEPALIVE:
                debug::log(3, FUNCTION, "Routing to ProcessSessionKeepalive");
                return ProcessSessionKeepalive(context, packet);

            case SET_REWARD:
                debug::log(2, FUNCTION, "Routing to ProcessSetReward");
                return ProcessSetReward(context, packet);

            /* Opcodes handled by connection layer (StatelessMinerConnection/ProcessPacketStateless).
             * Return "Unknown packet type" to trigger fallback without logging a warning. */
            case GET_BLOCK:
            case SUBMIT_BLOCK:
            case BLOCK_DATA:
            case GET_HEIGHT:
            case GET_REWARD:
            case GET_ROUND:
            case BLOCK_ACCEPTED:
            case BLOCK_REJECTED:
            case CHANNEL_ACK:
            case AUTH_CHALLENGE:
            case AUTH_RESULT:
            case REWARD_RESULT:
                debug::log(3, FUNCTION, "Opcode 0x", std::hex, uint32_t(nRouteOpcode), std::dec,
                           " handled by connection layer");
                return ProcessResult::Error(context, "Unknown packet type");

            default:
                debug::log(1, FUNCTION, "Unknown miner opcode: 0x", std::hex, uint32_t(packet.HEADER), std::dec);
                return ProcessResult::Error(context, "Unknown packet type");
        }
    }


    /* Build message for Falcon signature verification */
    std::vector<uint8_t> StatelessMiner::BuildAuthMessage(const MiningContext& context)
    {
        /* For challenge-response authentication, use the stored nonce */
        if(!context.vAuthNonce.empty())
        {
            return context.vAuthNonce;
        }

        /* Fallback: Build message from address + timestamp (legacy mode) */
        std::vector<uint8_t> vMessage;

        /* Add address string */
        vMessage.insert(vMessage.end(), context.strAddress.begin(), context.strAddress.end());

        /* Add timestamp (8 bytes, little-endian) */
        uint64_t nTime = context.nTimestamp;
        for(int i = 0; i < 8; ++i)
        {
            vMessage.push_back(static_cast<uint8_t>(nTime & 0xFF));
            nTime >>= 8;
        }

        return vMessage;
    }


    /* Process MINER_AUTH_INIT - first step of authentication handshake */
    ProcessResult StatelessMiner::ProcessMinerAuthInit(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_AUTH_INIT from ", context.strAddress);

        const std::vector<uint8_t>& vData = packet.DATA;

        /* DEBUG: Log incoming packet details for GetBytes() debugging */
        debug::log(2, FUNCTION, "MINER_AUTH_INIT packet: opcode=", packet.HEADER, " length=", packet.LENGTH);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_INIT::data", vData, 3);

        /* Validate minimum packet size: genesis(32) + pubkey_len(2) + miner_id_len(2) = 36 */
        if(vData.size() < 36)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small, size=", vData.size(), " expected>=36");
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small");
        }

        size_t nPos = 0;

        /* ═══════════════════════════════════════════════════════════
         * STEP 1: Parse hashGenesis FIRST (32 bytes)
         * ═══════════════════════════════════════════════════════════ */
        uint256_t hashGenesis(0);
        std::vector<uint8_t> vGenesis(vData.begin(), vData.begin() + 32);
        
        /* Convert raw bytes to hex string, then use SetHex which preserves order */
        std::string strGenesisHex = HexStr(vGenesis);
        hashGenesis.SetHex(strGenesisHex);
        
        /* Debug logging to verify */
        debug::log(0, FUNCTION, "Genesis raw bytes: ", HexStr(vGenesis));
        debug::log(0, FUNCTION, "Genesis hex string: ", strGenesisHex);
        debug::log(0, FUNCTION, "Genesis after SetHex: ", hashGenesis.GetHex());
        {
            std::ostringstream oss;
            oss << "Genesis type byte: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<uint32_t>(hashGenesis.GetType());
            debug::log(0, FUNCTION, oss.str());
        }
        
        /* Log genesis in canonical hex format for ChaCha20 shared secret derivation */
        debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
        debug::log(0, FUNCTION, "GENESIS HEX (for ChaCha20 shared secret):");
        debug::log(0, FUNCTION, "  ", hashGenesis.GetHex());
        debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
        
        nPos += 32;

        /* Derive ChaCha20 session key from genesis */
        std::vector<uint8_t> vSessionKey;
        bool fCanDecrypt = false;
        if(hashGenesis != 0)
        {
            vSessionKey = LLC::MiningSessionKeys::DeriveChaCha20Key(hashGenesis);
            fCanDecrypt = true;
            debug::log(2, FUNCTION, "ChaCha20 key derived from genesis");
        }

        /* ═══════════════════════════════════════════════════════════
         * STEP 2: Parse pubkey_len (2 bytes, big-endian)
         * ═══════════════════════════════════════════════════════════ */
        DisposableFalcon::DebugLogDeserialize("pubkey_len", nPos, 2, vData.size());
        uint16_t nPubKeyLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                              static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: parsed pubkey_len=", nPubKeyLen);

        /* Detect wrapped vs unwrapped - Support both Falcon-512 and Falcon-1024 */
        constexpr size_t CHACHA20_OVERHEAD = 12 + 16;  // nonce(12) + tag(16)
        constexpr size_t FALCON512_WRAPPED_SIZE = LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE + CHACHA20_OVERHEAD;  // 897 + 28 = 925
        constexpr size_t FALCON1024_WRAPPED_SIZE = LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE + CHACHA20_OVERHEAD; // 1793 + 28 = 1821
        bool fWrapped = (nPubKeyLen == FALCON512_WRAPPED_SIZE || nPubKeyLen == FALCON1024_WRAPPED_SIZE);

        /* Validate pubkey_len */
        if(nPubKeyLen == 0 || nPubKeyLen > 2048)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: invalid pubkey_len=", nPubKeyLen);
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid pubkey_len");
        }

        if(nPos + nPubKeyLen + 2 > vData.size())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small for pubkey, need=",
                       nPos + nPubKeyLen + 2, " have=", vData.size());
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for pubkey");
        }

        /* Extract public key data */
        DisposableFalcon::DebugLogDeserialize("pubkey", nPos, nPubKeyLen, vData.size());
        std::vector<uint8_t> vPubKeyData(vData.begin() + nPos, vData.begin() + nPos + nPubKeyLen);
        nPos += nPubKeyLen;

        /* ═══════════════════════════════════════════════════════════
         * STEP 3: Unwrap if ChaCha20 encrypted
         * ═══════════════════════════════════════════════════════════ */
        std::vector<uint8_t> vPubKey;
        if(fWrapped)
        {
            if(!fCanDecrypt)
            {
                debug::log(0, FUNCTION, "Wrapped key but zero genesis");
                return ProcessResult::Error(context, "Wrapped key requires genesis");
            }

            /* ChaCha20-Poly1305 format: nonce(12) + ciphertext(897 or 1793) + tag(16) */
            if(vPubKeyData.size() != FALCON512_WRAPPED_SIZE && 
               vPubKeyData.size() != FALCON1024_WRAPPED_SIZE)
            {
                debug::log(0, FUNCTION, "Invalid wrapped key size: ", vPubKeyData.size(), 
                          " (expected ", FALCON512_WRAPPED_SIZE, " or ", FALCON1024_WRAPPED_SIZE, ")");
                return ProcessResult::Error(context, "Invalid wrapped key size");
            }

            std::vector<uint8_t> vNonce(vPubKeyData.begin(), vPubKeyData.begin() + 12);
            std::vector<uint8_t> vCiphertext(vPubKeyData.begin() + 12, vPubKeyData.end() - 16);
            std::vector<uint8_t> vTag(vPubKeyData.end() - 16, vPubKeyData.end());
            std::vector<uint8_t> vAAD{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};

            if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vSessionKey, vNonce, vPubKey, vAAD))
            {
                debug::log(0, FUNCTION, "ChaCha20 decryption FAILED - genesis mismatch?");
                debug::log(0, FUNCTION, "  Session genesis (hex): ", hashGenesis.GetHex());
                if(vSessionKey.size() >= 8)
                    debug::log(0, FUNCTION, "  Derived key fingerprint (first 8 bytes): ",
                               HexStr(vSessionKey.begin(), vSessionKey.begin() + 8));
                return ProcessResult::Error(context, "ChaCha20 decryption failed");
            }
            debug::log(2, FUNCTION, "ChaCha20 decryption OK for MINER_AUTH_INIT");
            debug::log(2, FUNCTION, "  Session genesis (hex): ", hashGenesis.GetHex());
            if(vSessionKey.size() >= 8)
                debug::log(2, FUNCTION, "  Derived key fingerprint (first 8 bytes): ",
                           HexStr(vSessionKey.begin(), vSessionKey.begin() + 8));
        }
        else
        {
            vPubKey = vPubKeyData;
        }

        /* Validate pubkey size - Accept BOTH Falcon-512 (897) and Falcon-1024 (1793) */
        constexpr size_t FALCON512_PUBKEY_SIZE = LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE;
        constexpr size_t FALCON1024_PUBKEY_SIZE = LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE;
        
        if(vPubKey.size() != FALCON512_PUBKEY_SIZE && 
           vPubKey.size() != FALCON1024_PUBKEY_SIZE)
        {
            debug::log(0, FUNCTION, "Invalid pubkey size: ", vPubKey.size(), 
                      " (expected ", FALCON512_PUBKEY_SIZE, " or ", FALCON1024_PUBKEY_SIZE, ")");
            return ProcessResult::Error(context, "Invalid public key size");
        }

        /* Auto-detect Falcon version from public key */
        LLC::FalconVersion detected;
        if(!LLP::FalconVerify::VerifyPublicKey(vPubKey, detected))
        {
            debug::log(0, FUNCTION, "❌ Invalid Falcon public key");
            return ProcessResult::Error(context, "Invalid Falcon public key");
        }

        debug::log(2, FUNCTION, "✅ ", 
                  (detected == LLC::FalconVersion::FALCON_512) ? "Falcon-512" : "Falcon-1024", 
                  " public key detected (", vPubKey.size(), " bytes)");

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: extracted pubkey, len=", vPubKey.size());

        /* ═══════════════════════════════════════════════════════════
         * STEP 4: Parse miner_id
         * ═══════════════════════════════════════════════════════════ */
        DisposableFalcon::DebugLogDeserialize("miner_id_len", nPos, 2, vData.size());
        uint16_t nMinerIdLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                               static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: parsed miner_id_len=", nMinerIdLen);

        /* Validate miner_id_len */
        if(nMinerIdLen > 256)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: invalid miner_id_len=", nMinerIdLen);
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid miner_id_len");
        }

        if(nPos + nMinerIdLen > vData.size())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small for miner_id, need=",
                       nPos + nMinerIdLen, " have=", vData.size());
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for miner_id");
        }

        /* Extract miner ID (for logging) */
        DisposableFalcon::DebugLogDeserialize("miner_id", nPos, nMinerIdLen, vData.size());
        std::string strMinerId = "<no-id>";
        if(nMinerIdLen > 0)
            strMinerId.assign(vData.begin() + nPos, vData.begin() + nPos + nMinerIdLen);

        /* Generate challenge nonce */
        std::vector<uint8_t> vAuthNonce = LLC::GetRand256().GetBytes();

        /* Log summary */
        debug::log(0, FUNCTION, "MINER_AUTH_INIT: genesis_hash=", FullHexOrUnset(hashGenesis),
                    " miner=", strMinerId, " pubkey=", vPubKey.size(), 
                    fWrapped ? " (unwrapped)" : "");

        /* Validate genesis binding if FalconAuth is available
         * Genesis validation is CRITICAL for reward routing.
         * If genesis is invalid or doesn't resolve to a valid account, 
         * authentication MUST fail to prevent mining with incorrect reward routing. */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        if(pAuth && hashGenesis != 0)
        {
            uint256_t hashKeyID = pAuth->DeriveKeyId(vPubKey);
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            
            /* If this key has a bound genesis, verify it matches */
            if(boundGenesis.has_value() && boundGenesis.value() != 0)
            {
                if(boundGenesis.value() != hashGenesis)
                {
                    debug::log(0, FUNCTION, "MINER_AUTH_INIT: genesis mismatch! claimed_hash=", 
                               FullHexOrUnset(hashGenesis), " bound_genesis_hash=", FullHexOrUnset(boundGenesis.value()));
                    return ProcessResult::Error(context, "Genesis mismatch with bound Falcon key");
                }
                debug::log(2, FUNCTION, "MINER_AUTH_INIT: genesis binding verified");
            }
            else
            {
                /* No existing binding - this is a new key */
                debug::log(0, FUNCTION, "MINER_AUTH_INIT: new key, genesis_hash=", FullHexOrUnset(hashGenesis));
            }
        }

        /* Update context */
        MiningContext newContext = context
            .WithPubKey(vPubKey)
            .WithNonce(vAuthNonce)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithFalconVersion(detected);

        /* Build challenge */
        StatelessPacket response(StatelessOpcodes::AUTH_CHALLENGE);
        uint16_t nNonceLen = static_cast<uint16_t>(vAuthNonce.size());
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen >> 8));
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen & 0xFF));
        response.DATA.insert(response.DATA.end(), vAuthNonce.begin(), vAuthNonce.end());
        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        /* DEBUG: Log response packet */
        debug::log(2, FUNCTION, "MINER_AUTH_CHALLENGE response: ", response.DebugString());

        debug::log(0, FUNCTION, "Sending MINER_AUTH_CHALLENGE nonce_len=", vAuthNonce.size());

        return ProcessResult::Success(newContext, response);
    }


    /* Process Falcon authentication response (MINER_AUTH_RESPONSE) */
    ProcessResult StatelessMiner::ProcessFalconResponse(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE from ", context.strAddress);

        /* DEBUG: Log incoming packet details */
        debug::log(2, FUNCTION, "MINER_AUTH_RESPONSE packet: opcode=", packet.HEADER, " length=", packet.LENGTH);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::data", packet.DATA, 3);

        /* Validate that we have nonce and pubkey from MINER_AUTH_INIT */
        if(context.vAuthNonce.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no nonce (MINER_AUTH_INIT not received)");
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        if(context.vMinerPubKey.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no pubkey (MINER_AUTH_INIT not received)");
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate minimum packet size (2 bytes for sig_len) */
        if(vData.size() < 2)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small, size=", vData.size());
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Parse sig_len (2 bytes, little-endian to match NexusMiner protocol) */
        DisposableFalcon::DebugLogDeserialize("sig_len", 0, 2, vData.size());
        uint16_t nSigLen = static_cast<uint16_t>(vData[0]) |
                           (static_cast<uint16_t>(vData[1]) << 8);

        debug::log(3, FUNCTION, "MINER_AUTH_RESPONSE: parsed sig_len=", nSigLen);

        /* Validate sig_len */
        if(nSigLen == 0 || nSigLen > FalconConstants::FALCON512_SIG_MAX_VALIDATION)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid sig_len ", nSigLen);
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        if(vData.size() < 2 + nSigLen)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small for signature, need=",
                       2 + nSigLen, " have=", vData.size());
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Extract signature */
        DisposableFalcon::DebugLogDeserialize("signature", 2, nSigLen, vData.size());
        std::vector<uint8_t> vSignature(vData.begin() + 2, vData.begin() + 2 + nSigLen);

        debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE sig_len=", nSigLen);

        /* DEBUG: Log verification data */
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::pubkey", context.vMinerPubKey, 4);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::nonce", context.vAuthNonce, 4);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::signature", vSignature, 4);

        /* AUTO-DETECT Falcon version from public key size (PR #122) */
        LLC::FalconVersion detectedVersion;
        if(!LLP::FalconVerify::VerifyPublicKey(context.vMinerPubKey, detectedVersion))
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid Falcon public key, len=",
                       context.vMinerPubKey.size());
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        debug::log(2, FUNCTION, "Detected Falcon-",
                  (detectedVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024"),
                  " from public key size ", context.vMinerPubKey.size());

        /* Validate signature size matches detected version */
        size_t expectedSize = LLC::FalconConstants::GetSignatureCTSize(detectedVersion);
        if(vSignature.size() != expectedSize)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: signature size mismatch: ", vSignature.size(),
                      " expected ", expectedSize, " for Falcon-",
                      (detectedVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024"));
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Verify Falcon signature using LLC::FLKey directly */
        LLC::FLKey flkey;
        if(!flkey.SetPubKey(context.vMinerPubKey))
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid public key, len=",
                       context.vMinerPubKey.size());
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        debug::log(3, FUNCTION, "MINER_AUTH_RESPONSE: verifying signature...");

        if(!flkey.Verify(context.vAuthNonce, vSignature))
        {
            debug::log(0, FUNCTION, "MINER_AUTH verification FAILED from ", context.strAddress);
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        debug::log(2, FUNCTION, "MINER_AUTH_RESPONSE: signature verified successfully");

        /* Authentication succeeded */
        /* Derive key ID from public key */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        uint256_t hashKeyID(0);
        if(pAuth)
            hashKeyID = pAuth->DeriveKeyId(context.vMinerPubKey);

        /* Derive session ID from key ID (lower 32 bits) */
        uint32_t nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0));

        /* Use genesis from MINER_AUTH_INIT if provided, otherwise check binding */
        uint256_t hashGenesis = context.hashGenesis;  // Already set from INIT
        if(hashGenesis == 0 && pAuth)
        {
            /* Fallback: check for bound genesis */
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            if(boundGenesis.has_value())
                hashGenesis = boundGenesis.value();
        }

        debug::log(0, FUNCTION, "MINER_AUTH success: falcon_key_id=", FullHexOrUnset(hashKeyID),
                   " session_genesis=", FullHexOrUnset(hashGenesis), " sessionId=", nSessionId);

        /* Authentication succeeded - now resolve reward routing */
        uint256_t hashGenesisFinal = hashGenesis;

        /* If no genesis in context, check for bound genesis */
        if(hashGenesisFinal == 0 && pAuth)
        {
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            if(boundGenesis.has_value())
                hashGenesisFinal = boundGenesis.value();
        }

        /* Validate genesis exists on blockchain for ChaCha20 key derivation security */
        if(hashGenesisFinal != 0)
        {
            debug::log(0, FUNCTION, "");
            debug::log(0, FUNCTION, "════════════════════════════════════════════════════════");
            debug::log(0, FUNCTION, "       GENESIS VALIDATED FOR CHACHA20 KEY DERIVATION");
            debug::log(0, FUNCTION, "════════════════════════════════════════════════════════");
            debug::log(0, FUNCTION, "Genesis hash: ", hashGenesisFinal.ToString());
            
            /* Only validate genesis exists on chain - reward address comes via MINER_SET_REWARD */
            if(!LLD::Ledger->HasFirst(hashGenesisFinal))
            {
                debug::log(0, FUNCTION, "✗ Genesis not found on blockchain");
                debug::log(0, FUNCTION, "════════════════════════════════════════════════════════");
                
                StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
                response.DATA.push_back(0x00); // Failure
                response.LENGTH = 1;
                return ProcessResult::Success(context, response);
            }
            
            debug::log(0, FUNCTION, "✓ Genesis validated on blockchain");
            debug::log(0, FUNCTION, "✓ ChaCha20 session key can be derived");
            debug::log(0, FUNCTION, "");
            debug::log(0, FUNCTION, "NOTE: Reward address will be set via MINER_SET_REWARD packet");
            debug::log(0, FUNCTION, "════════════════════════════════════════════════════════");
        }
        else
        {
            /* Genesis is required for ChaCha20 key derivation and secure communication */
            debug::log(0, FUNCTION, "✗ No genesis hash provided - ChaCha20 encryption not possible");
            debug::log(0, FUNCTION, "  Genesis hash is required for secure session key derivation");
            
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Log authentication success summary */
        debug::log(0, FUNCTION, "");
        debug::log(0, FUNCTION, "╔═══════════════════════════════════════════════════════════╗");
        debug::log(0, FUNCTION, "║         MINER AUTHENTICATION SUCCESSFUL                   ║");
        debug::log(0, FUNCTION, "╠═══════════════════════════════════════════════════════════╣");
        debug::log(0, FUNCTION, "║ Falcon Key ID:", FullHexOrUnset(hashKeyID));
        debug::log(0, FUNCTION, "║ Session ID:   ", nSessionId);
        debug::log(0, FUNCTION, "║ Falcon Ver:   ", (detectedVersion == LLC::FalconVersion::FALCON_512 ? "Falcon-512" : "Falcon-1024"));
        debug::log(0, FUNCTION, "║ Genesis Hash: ", FullHexOrUnset(hashGenesisFinal), " (ChaCha20)");
        debug::log(0, FUNCTION, "║ ChaCha20:     Ready for encryption");
        debug::log(0, FUNCTION, "║ Reward Addr:  (awaiting MINER_SET_REWARD)");
        debug::log(0, FUNCTION, "║ From:         ", context.strAddress);
        debug::log(0, FUNCTION, "╚═══════════════════════════════════════════════════════════╝");

        /* Update context with auth success.
         * Note: We clear the nonce after successful authentication for security:
         * - The nonce was single-use and should not be reused
         * - The pubkey hash is preserved in hashKeyID for audit trails
         * - The pubkey MUST remain in context for extraction by StatelessMinerConnection
         *   to enable Falcon signature verification of work submissions (PR #107) */
        MiningContext newContext = context
            .WithAuth(true)
            .WithSession(nSessionId)
            .WithKeyId(hashKeyID)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithSessionStart(runtime::unifiedtimestamp())   // Auto-start session at auth
            .WithSessionTimeout(DEFAULT_SESSION_TIMEOUT)      // Set default timeout
            .WithNonce(std::vector<uint8_t>())  // Clear single-use nonce
            .WithFalconVersion(detectedVersion);  // Store detected Falcon version (PR #122)
            // Keep vMinerPubKey in context - it will be extracted and stored in mapSessionKeys

        /* Set up ChaCha20 encryption context (CRITICAL for PR #111)
         * This MUST be set after successful authentication for SUBMIT_BLOCK to work */
        std::vector<uint8_t> vChaChaKey = LLC::MiningSessionKeys::DeriveChaCha20Key(hashGenesisFinal);
        newContext = newContext.WithChaChaKey(vChaChaKey);
        
        /* Validate encryption context was set correctly */
        if(!newContext.fEncryptionReady || newContext.vChaChaKey.empty() || newContext.vChaChaKey.size() != 32)
        {
            debug::error(FUNCTION, "❌ CRITICAL: Failed to set ChaCha20 encryption context!");
            debug::error(FUNCTION, "   fEncryptionReady: ", newContext.fEncryptionReady);
            debug::error(FUNCTION, "   vChaChaKey size: ", newContext.vChaChaKey.size(), " (expected: 32)");
            
            /* This should never happen, but fail loudly if it does */
            StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }
        
        /* Log encryption context setup */
        debug::log(0, FUNCTION, "");
        debug::log(0, FUNCTION, "🔐 ENCRYPTION CONTEXT SETUP:");
        debug::log(0, FUNCTION, "   ChaCha20:     Ready for encryption");
        debug::log(0, FUNCTION, "   Key size:     ", newContext.vChaChaKey.size(), " bytes");
        debug::log(2, FUNCTION, "   ChaCha20 session key derived from genesis");
        debug::log(2, FUNCTION, "   Encryption is now ACTIVE for session ", nSessionId);
        debug::log(2, FUNCTION, "Session auto-started at auth: nSessionStart=", newContext.nSessionStart,
                   " timeout=", newContext.nSessionTimeout, "s");
        debug::log(0, FUNCTION, "");

        /* Build success response */
        StatelessPacket response(StatelessOpcodes::AUTH_RESULT);
        response.DATA.push_back(0x01); // Success
        
        // Append session ID (4 bytes, little-endian)
        response.DATA.push_back(nSessionId & 0xFF);
        response.DATA.push_back((nSessionId >> 8) & 0xFF);
        response.DATA.push_back((nSessionId >> 16) & 0xFF);
        response.DATA.push_back((nSessionId >> 24) & 0xFF);
        
        response.LENGTH = 5;  // 1 byte status + 4 bytes session ID

        /* DEBUG: Log response packet */
        debug::log(2, FUNCTION, "MINER_AUTH_RESULT response: ", response.DebugString());

        debug::log(0, FUNCTION, "MINER_AUTH success for key ", hashKeyID.SubString(),
                   " sessionId=", nSessionId, " from ", context.strAddress);

        return ProcessResult::Success(newContext, response);
    }


    /* Process session start */
    ProcessResult StatelessMiner::ProcessSessionStart(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        /* Phase 2: Require authentication before session start */
        if(!context.fAuthenticated)
            return ProcessResult::Error(context, "Not authenticated");

        const std::vector<uint8_t>& vData = packet.DATA;
        uint64_t nNow = runtime::unifiedtimestamp();

        /* Validate: session was already auto-started at auth time.
         * ProcessFalconResponse() sets nSessionStart via .WithSessionStart() at auth.
         * The node also sends SESSION_START automatically after AUTH_RESULT in both
         * stateless_miner_connection.cpp and miner.cpp.  If a miner sends a redundant
         * SESSION_START, treat it as a re-negotiation (update timeout only) rather
         * than silently resetting keepalive counters and timing state. */
        const bool fSessionAlreadyActive = (context.nSessionStart != 0 && context.nSessionId != 0);
        if(fSessionAlreadyActive)
        {
            debug::log(0, FUNCTION, "SESSION_START re-negotiation for existing session: sessionId=",
                       context.nSessionId, " original_start=", context.nSessionStart,
                       " keepalives=", context.nKeepaliveCount, " from ", context.strAddress);
        }

        /* Parse optional session parameters from packet.DATA */
        /* Format: [timeout (4 bytes, optional)] */
        uint64_t nRequestedTimeout = DEFAULT_SESSION_TIMEOUT;
        if(vData.size() >= 4)
        {
            /* Parse timeout as 4-byte little-endian */
            nRequestedTimeout = vData[0] | (vData[1] << 8) |
                               (vData[2] << 16) | (vData[3] << 24);

            /* Clamp timeout to reasonable range (60s to 86400s / 24 hours) */
            if(nRequestedTimeout < 60)
                nRequestedTimeout = 60;
            else if(nRequestedTimeout > 86400)
                nRequestedTimeout = 86400;
        }

        /* Build updated context.
         * - If session is already active (re-negotiation), preserve nSessionStart
         *   and nKeepaliveCount to avoid wiping timing/liveness state.
         * - If session is not yet active (first explicit SESSION_START), initialize
         *   nSessionStart to now and reset keepalive counter. */
        MiningContext newContext = context.WithTimestamp(nNow).WithSessionTimeout(nRequestedTimeout);
        if(!fSessionAlreadyActive)
        {
            newContext = newContext.WithSessionStart(nNow).WithKeepaliveCount(0);
        }

        debug::log(0, FUNCTION, "SESSION_START ", (fSessionAlreadyActive ? "re-negotiated" : "established"),
                   " for sessionId=", context.nSessionId,
                   " timeout=", nRequestedTimeout, "s from ", context.strAddress);

        /* After session is established, log detailed summary */
        debug::log(0, FUNCTION, "");
        debug::log(0, FUNCTION, "╔═══════════════════════════════════════════════════════════╗");
        debug::log(0, FUNCTION, "║           MINING SESSION ", (fSessionAlreadyActive ? "RE-NEGOTIATED" : "ESTABLISHED  "), "          ║");
        debug::log(0, FUNCTION, "╠═══════════════════════════════════════════════════════════╣");
        debug::log(0, FUNCTION, "║ Session ID:    ", context.nSessionId);
        debug::log(0, FUNCTION, "║ Timeout:       ", nRequestedTimeout, " seconds");
        debug::log(0, FUNCTION, "║ Genesis Hash:  ", context.GenesisHex());
        debug::log(0, FUNCTION, "║ Miner:         ", context.strAddress);
        if(fSessionAlreadyActive)
        {
            debug::log(0, FUNCTION, "║ Start:         ", context.nSessionStart, " (preserved)");
            debug::log(0, FUNCTION, "║ Keepalives:    ", context.nKeepaliveCount, " (preserved)");
        }
        debug::log(0, FUNCTION, "╚═══════════════════════════════════════════════════════════╝");

        /* Log reward routing status */
        if(context.fRewardBound && context.hashRewardAddress != 0)
        {
            debug::log(0, FUNCTION, "REWARDS → Bound reward hash (full): ", context.hashRewardAddress.GetHex());
            debug::log(0, FUNCTION, "REWARDS → Reward source: ", context.RewardBindingSource());
        }
        else if(context.hashGenesis != 0)
        {
            debug::log(0, FUNCTION, "REWARDS → Session genesis fallback hash: ", context.GenesisHex());
            debug::log(0, FUNCTION, "REWARDS → Reward source: ", context.RewardBindingSource(),
                       " (waiting for MINER_SET_REWARD)");
        }
        else
        {
            debug::log(0, FUNCTION, "REWARDS → Not configured (send MINER_SET_REWARD after auth)");
        }

        /* Build acknowledgment response with session parameters using shared utility.
         * Response format: [success (1)][session_id (4)][timeout (4)][genesis (32)] */
        StatelessPacket response(StatelessOpcodes::SESSION_START);
        response.DATA = SessionStartPacket::BuildPayload(
            newContext.nSessionId, nRequestedTimeout, newContext.hashGenesis);
        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        return ProcessResult::Success(newContext, response);
    }


    /* Process set channel */
    ProcessResult StatelessMiner::ProcessSetChannel(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate payload size (1 byte or 4+ bytes for legacy compatibility) */
        if(vData.size() != 1 && vData.size() < 4)
            return ProcessResult::Error(context, "Invalid channel payload size");

        uint32_t nChannel = 0;

        /* Parse channel value */
        if(vData.size() == 1)
        {
            /* New format: single byte */
            nChannel = static_cast<uint32_t>(vData[0]);
        }
        else
        {
            /* Legacy format: 4-byte little-endian */
            nChannel = vData[0] | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
        }

        /* Validate channel (1=Prime, 2=Hash, 0=invalid) */
        if(nChannel != 1 && nChannel != 2)
            return ProcessResult::Error(context, "Invalid channel");

        /* Update context */
        MiningContext newContext = context
            .WithChannel(nChannel)
            .WithChannelName(nChannel)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Build acknowledgment response with channel number */
        StatelessPacket response(StatelessOpcodes::CHANNEL_ACK);
        response.DATA.push_back(static_cast<uint8_t>(nChannel));
        response.LENGTH = 1;

        debug::log(2, FUNCTION, "Channel set to ", nChannel);

        return ProcessResult::Success(newContext, response);
    }


    /* Process session keepalive — unified handler for SESSION_KEEPALIVE (0xD0D4) on both ports.
     * Request: 8 bytes — [session_id LE][hashPrevBlock_lo32 BE]
     * Response: 32 bytes BE — unified chain state telemetry */
    ProcessResult StatelessMiner::ProcessSessionKeepalive(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        /* Require authentication before keepalive */
        if(!context.fAuthenticated)
            return ProcessResult::Error(context, "SESSION_KEEPALIVE: not authenticated");

        /* Check if session has been started */
        if(context.nSessionStart == 0)
            return ProcessResult::Error(context, "Session not started");

        uint64_t nNow = runtime::unifiedtimestamp();

        /* Check if session has expired (even for keepalive) */
        if(context.IsSessionExpired(nNow))
        {
            /* If the miner is still authenticated and has a valid session ID, the keepalive
             * is evidence of liveness — extend the session instead of forcing a full reconnect. */
            if(context.fAuthenticated && context.nSessionId != 0)
            {
                uint64_t nIdleSec = nNow - context.nTimestamp;
                debug::log(0, FUNCTION, "SESSION_KEEPALIVE: session nominally expired but miner is "
                           "authenticated — extending session for sessionId=", context.nSessionId,
                           " (idle=", nIdleSec, "s, timeout=", context.nSessionTimeout, "s)");
                /* Fall through to normal keepalive processing; WithTimestamp(nNow) will clear expiry */
            }
            else
            {
                debug::log(0, FUNCTION, "SESSION_KEEPALIVE rejected - session expired for sessionId=",
                           context.nSessionId, " last_activity=", context.nTimestamp);

                /* Send SESSION_EXPIRED notification to miner instead of silent disconnect. */
                StatelessPacket expiredResponse(StatelessOpcodes::SESSION_EXPIRED);

                /* Build 5-byte payload: session_id (4 bytes LE) + reason (1 byte) */
                uint32_t nSessionId = context.nSessionId;
                expiredResponse.DATA.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
                expiredResponse.DATA.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
                expiredResponse.DATA.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
                expiredResponse.DATA.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));

                /* Reason code: 0x01 = EXPIRED_INACTIVITY (no keepalive within timeout) */
                expiredResponse.DATA.push_back(0x01);

                expiredResponse.LENGTH = 5;

                debug::log(0, FUNCTION, "Sending SESSION_EXPIRED notification to miner (sessionId=",
                           context.nSessionId, ", reason=INACTIVITY)");

                return ProcessResult::Success(context, expiredResponse);
            }
        }

        /* Parse 8-byte keepalive payload (session_id LE, hashPrevBlock_lo32 BE) */
        uint32_t nPayloadSession = 0;
        std::array<uint8_t, 4> prevblockSuffixBytes = {};
        if(!KeepaliveV2::ParsePayload(packet.DATA, nPayloadSession, prevblockSuffixBytes))
        {
            debug::log(1, FUNCTION, "SESSION_KEEPALIVE: payload too short (", packet.DATA.size(), " bytes, need 8)");
            return ProcessResult::Error(context, "SESSION_KEEPALIVE: invalid payload (need 8 bytes)");
        }

        /* Update timestamp, keepalive counters, and prevblock suffix */
        uint32_t nNewKeepaliveCount = context.nKeepaliveCount + 1;
        uint32_t nNewKeepaliveSent = context.nKeepaliveSent + 1;
        MiningContext newContext = context
            .WithTimestamp(nNow)
            .WithKeepaliveCount(nNewKeepaliveCount)
            .WithKeepaliveSent(nNewKeepaliveSent)
            .WithLastKeepaliveTime(nNow)
            .WithMinerPrevblockSuffix(prevblockSuffixBytes);

        /* Derive nMinerPrevHashLo32 from suffix bytes (BE) */
        uint32_t nMinerPrevHashLo32 =
            (uint32_t(prevblockSuffixBytes[0]) << 24) |
            (uint32_t(prevblockSuffixBytes[1]) << 16) |
            (uint32_t(prevblockSuffixBytes[2]) <<  8) |
             uint32_t(prevblockSuffixBytes[3]);

        /* Log at different verbosity levels based on keepalive frequency */
        uint32_t nLogLevel = (nNewKeepaliveCount % 10 == 0) ? 2 : 3;
        {
            char suffixHex[9];
            std::snprintf(suffixHex, sizeof(suffixHex), "%02x%02x%02x%02x",
                          prevblockSuffixBytes[0], prevblockSuffixBytes[1],
                          prevblockSuffixBytes[2], prevblockSuffixBytes[3]);
            debug::log(nLogLevel, FUNCTION, "SESSION_KEEPALIVE from sessionId=", context.nSessionId,
                       " keepalive_rx=", nNewKeepaliveCount,
                       " keepalive_tx=", nNewKeepaliveSent,
                       " prevblock_suffix=", suffixHex,
                       " session_duration=", newContext.GetSessionDuration(nNow), "s");
        }

        /* Gather current chain state for unified 32-byte reply.
         * CRITICAL: Use stateBest.GetHash() instead of loading hashBestChain separately
         * to prevent race conditions where heights come from block N but hash from block N+1. */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        uint32_t nUnifiedHeight = stateBest.nHeight;

        TAO::Ledger::BlockState stateChannel = stateBest;
        uint32_t nPrimeHeight = 0;
        if(TAO::Ledger::GetLastState(stateChannel, 1))
            nPrimeHeight = stateChannel.nChannelHeight;

        stateChannel = stateBest;
        uint32_t nHashHeight = 0;
        if(TAO::Ledger::GetLastState(stateChannel, 2))
            nHashHeight = stateChannel.nChannelHeight;

        stateChannel = stateBest;
        uint32_t nStakeHeight = 0;
        if(TAO::Ledger::GetLastState(stateChannel, 0))
            nStakeHeight = stateChannel.nChannelHeight;

        /* Extract hash from the same atomic snapshot to ensure consistency */
        uint1024_t hashBestChain = stateBest.GetHash();
        uint32_t nHashTipLo32 = static_cast<uint32_t>(hashBestChain.Get64(0) & 0xFFFFFFFF);

        /* fork_score: non-zero when miner's prevHash lo32 differs from node tip */
        uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;

        std::vector<uint8_t> vResponse = KeepaliveV2::BuildUnifiedResponse(
            newContext.nSessionId,
            nMinerPrevHashLo32,    // echo miner's fork canary
            nUnifiedHeight,
            nHashTipLo32,
            nPrimeHeight,
            nHashHeight,
            nStakeHeight,
            nForkScore);

        debug::log(nLogLevel, FUNCTION, "SESSION_KEEPALIVE unified reply (32 bytes): sessionId=",
                   newContext.nSessionId,
                   " unified=", nUnifiedHeight,
                   " prime=", nPrimeHeight,
                   " hash=", nHashHeight,
                   " stake=", nStakeHeight,
                   " hash_tip_lo32=0x", std::hex, nHashTipLo32,
                   " miner_prevhash_lo32=0x", nMinerPrevHashLo32,
                   " fork_score=", std::dec, nForkScore);

        StatelessPacket response(StatelessOpcodes::SESSION_KEEPALIVE);
        response.DATA = vResponse;
        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        /* Notify Colin agent with keepalive telemetry (observability hook) */
        {
            std::string genesis_prefix = context.hashGenesis != uint256_t(0)
                ? context.hashGenesis.SubString(8)
                : std::string{};
            ColinMiningAgent::Get().on_keepalive_ack(
                genesis_prefix,
                nUnifiedHeight,
                nPrimeHeight,
                nHashHeight,
                nStakeHeight,
                nForkScore);
        }

        /* Update context: refresh timestamp and cache latest stake height */
        nNow = runtime::unifiedtimestamp();
        newContext = newContext
            .WithTimestamp(nNow)
            .WithStakeHeight(nStakeHeight);

        return ProcessResult::Success(newContext, response);
    }


    /* Decrypts reward address payload using ChaCha20-Poly1305 */
    bool StatelessMiner::DecryptRewardPayload(
        const std::vector<uint8_t>& vEncrypted,
        const std::vector<uint8_t>& vKey,
        std::vector<uint8_t>& vPlaintext
    )
    {
        /* Use LLC helper with domain-specific AAD for AEAD authentication */
        return LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vPlaintext, AAD_REWARD_ADDRESS);
    }


    /* Encrypts reward result response using ChaCha20-Poly1305 */
    std::vector<uint8_t> StatelessMiner::EncryptRewardResult(
        const std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vKey
    )
    {
        /* Use LLC helper with domain-specific AAD for AEAD authentication */
        return LLC::EncryptPayloadChaCha20(vPlaintext, vKey, AAD_REWARD_RESULT);
    }


    /* Validates reward address format */
    bool StatelessMiner::ValidateRewardAddress(const uint256_t& hashReward)
    {
        /* Check for zero address */
        if(hashReward == 0)
        {
            debug::error(FUNCTION, "Zero reward address not allowed");
            return false;
        }

        /* Must be a TritiumGenesis (user sigchain) — Register Addresses are NOT valid coinbase
         * recipients. Coinbase::Verify() hard-enforces that the coinbase field must carry a
         * UserType genesis hash; any other type byte causes network peers to reject the block. */
        if(!GenesisConstants::IsValidGenesisType(hashReward))
        {
            debug::error(FUNCTION, "Reward address has invalid type byte 0x",
                         std::hex, static_cast<int>(hashReward.GetType()), std::dec,
                         " — must be a TritiumGenesis account (type 0x",
                         std::hex, static_cast<int>(TAO::Ledger::GENESIS::UserType()), std::dec,
                         "). Register Addresses are not supported as mining reward addresses.");
            return false;
        }

        return true;
    }


    /* Process MINER_SET_REWARD packet to bind reward address */
    ProcessResult StatelessMiner::ProcessSetReward(
        const MiningContext& context,
        const StatelessPacket& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_SET_REWARD from ", context.strAddress,
                   " length=", packet.LENGTH);

        /* Check authentication first */
        if(!context.fAuthenticated)
        {
            debug::error(FUNCTION, "MINER_SET_REWARD: not authenticated");
            return ProcessResult::Error(context, "Authentication required");
        }

        /* Check that genesis is set (needed for ChaCha20 key derivation) */
        if(context.hashGenesis == 0)
        {
            debug::error(FUNCTION, "MINER_SET_REWARD: genesis not set");
            return ProcessResult::Error(context, "Genesis not established");
        }

        /* R-02: Session consistency gate — reject before any key-material access.
         * Skip on legacy-lane contexts where hashKeyID is intentionally 0; the live
         * hashKeyID for legacy miners is stored in StatelessMinerManager, not here. */
        if(context.hashKeyID != 0)
        {
            const SessionConsistencyResult consistency = context.ValidateConsistency();
            if(consistency != SessionConsistencyResult::Ok)
            {
                debug::log(0, FUNCTION, "Session consistency violation at MINER_SET_REWARD: ",
                           SessionConsistencyResultString(consistency));
                StatelessPacket authFailed(StatelessOpcodes::AUTH_RESULT);
                authFailed.DATA.push_back(0x00);  // failure
                authFailed.LENGTH = 1;
                return ProcessResult::Success(context, authFailed);
            }
        }

        /* Derive ChaCha20 session key from genesis */
        std::vector<uint8_t> vChaChaKey = LLC::MiningSessionKeys::DeriveChaCha20Key(context.hashGenesis);
        const auto optRecoveredSession = SessionRecoveryManager::Get().RecoverSessionByIdentity(
            context.hashKeyID,
            context.strAddress
        );
        const bool fRecoveredSessionState = optRecoveredSession.has_value();
        const bool fRecoveryGenesisMatches = fRecoveredSessionState &&
            optRecoveredSession->hashGenesis == context.hashGenesis;

        /* Decrypt the payload */
        std::vector<uint8_t> vDecrypted;
        if(!DecryptRewardPayload(packet.DATA, vChaChaKey, vDecrypted))
        {
            debug::error(FUNCTION, "Failed to decrypt reward address payload");
            debug::log(0, FUNCTION, "REWARD BINDING DIAGNOSTIC");
            debug::log(0, FUNCTION, "- miner reward string: NOT AVAILABLE (packet carries encrypted 32-byte hash only)");
            debug::log(0, FUNCTION, "- decoded reward register/account hash: NOT AVAILABLE (decryption failed)");
            debug::log(0, FUNCTION, "- bound reward hash from current session: ", FullHexOrUnset(context.hashRewardAddress));
            debug::log(0, FUNCTION, "- bound reward source: ", context.RewardBindingSource());
            debug::log(0, FUNCTION, "- session genesis used for ChaCha20 KDF: ", context.GenesisHex());
            debug::log(0, FUNCTION, "- derived ChaCha20 key fingerprint: ", KeyFingerprint(vChaChaKey));
            debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(fRecoveredSessionState));
            debug::log(0, FUNCTION, "- recovered session genesis: ",
                       fRecoveredSessionState ? FullHexOrUnset(optRecoveredSession->hashGenesis) : "NOT AVAILABLE");
            debug::log(0, FUNCTION, "- recovered session genesis matches live context: ", YesNo(fRecoveryGenesisMatches));
            debug::log(0, FUNCTION, "- consistency result: FAIL");
            
            /* Build encrypted error response */
            std::vector<uint8_t> vErrorMsg = {0x00};  // Failure status
            std::vector<uint8_t> vEncryptedError = EncryptRewardResult(vErrorMsg, vChaChaKey);
            
            StatelessPacket errorResponse(REWARD_RESULT);
            errorResponse.DATA = vEncryptedError;
            errorResponse.LENGTH = static_cast<uint32_t>(vEncryptedError.size());

            /* Return success with error response - we want to send the encrypted error to miner */
            return ProcessResult::Success(context, errorResponse);
        }

        /* Extract the reward address (32 bytes) */
        if(vDecrypted.size() != 32)
        {
            debug::error(FUNCTION, "Invalid reward address payload size: ", vDecrypted.size(), " (expected 32)");
            
            std::vector<uint8_t> vErrorMsg = {0x00};
            std::vector<uint8_t> vEncryptedError = EncryptRewardResult(vErrorMsg, vChaChaKey);
            
            StatelessPacket errorResponse(REWARD_RESULT);
            errorResponse.DATA = vEncryptedError;
            errorResponse.LENGTH = static_cast<uint32_t>(vEncryptedError.size());

            /* Return success with error response */
            return ProcessResult::Success(context, errorResponse);
        }

        /* Parse the 32-byte reward address from decrypted payload.
         * NexusMiner sends the genesis hash in big-endian / display byte order
         * (hex_decode_genesis_hash decodes left-to-right: byte[0] == 0xa1 type byte).
         * uint256_t internal storage is little-endian word order: pn[0] is the least
         * significant word and pn[WIDTH-1] is the most significant.  GetType() reads
         * from pn[WIDTH-1], so we must load via SetHex() which performs the correct
         * reversal — exactly the same pattern used in ProcessMinerAuthInit for genesis.
         * Do NOT use memcpy(begin(), ...) — that places byte[0] into the least
         * significant word, causing GetType() to read the wrong byte. */
        uint256_t hashReward;
        hashReward.SetHex(HexStr(vDecrypted.begin(), vDecrypted.end()));

        debug::log(0, FUNCTION, "Received decoded reward register/account hash: ", hashReward.GetHex());

        const bool fExistingRewardPresent = context.fRewardBound && context.hashRewardAddress != 0;
        const bool fExistingRewardMatches = !fExistingRewardPresent ||
            context.hashRewardAddress == hashReward;
        const bool fRewardEqualsGenesis = (context.hashGenesis != 0 && hashReward == context.hashGenesis);

        debug::log(0, FUNCTION, "REWARD BINDING DIAGNOSTIC");
        debug::log(0, FUNCTION, "- miner reward string: NOT AVAILABLE (packet carries encrypted 32-byte hash only)");
        debug::log(0, FUNCTION, "- decoded reward register/account hash: ", hashReward.GetHex());
        debug::log(0, FUNCTION, "- bound reward hash from current session: ", FullHexOrUnset(context.hashRewardAddress));
        debug::log(0, FUNCTION, "- bound reward source: ", context.RewardBindingSource());
        debug::log(0, FUNCTION, "- session genesis used for ChaCha20 KDF: ", context.GenesisHex());
        debug::log(0, FUNCTION, "- derived ChaCha20 key fingerprint: ", KeyFingerprint(vChaChaKey));
        debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(fRecoveredSessionState));
        debug::log(0, FUNCTION, "- recovered session genesis: ",
                   fRecoveredSessionState ? FullHexOrUnset(optRecoveredSession->hashGenesis) : "NOT AVAILABLE");
        debug::log(0, FUNCTION, "- recovered session genesis matches live context: ", YesNo(fRecoveryGenesisMatches));
        debug::log(0, FUNCTION, "- reward hash == bound reward hash: ",
                   fExistingRewardPresent ? YesNo(fExistingRewardMatches) : "NOT PREVIOUSLY BOUND");
        debug::log(0, FUNCTION, "- decoded reward hash == session genesis: ", YesNo(fRewardEqualsGenesis));
        debug::log(0, FUNCTION, "- source of decoded reward hash: MINER_SET_REWARD decrypted payload");

        if(!fExistingRewardMatches)
        {
            debug::warning(FUNCTION, "REWARD BINDING MISMATCH: existing bound reward hash=",
                           FullHexOrUnset(context.hashRewardAddress),
                           " differs from decoded reward hash=", hashReward.GetHex());
        }

        if(fRecoveredSessionState && !fRecoveryGenesisMatches)
        {
            debug::warning(FUNCTION, "SESSION RECOVERY GENESIS MISMATCH: recovered session genesis=",
                           FullHexOrUnset(optRecoveredSession->hashGenesis),
                           " differs from live session genesis=", context.GenesisHex());
        }

        /* Validate the reward address */
        if(!ValidateRewardAddress(hashReward))
        {
            debug::error(FUNCTION, "Invalid reward address");
            
            std::vector<uint8_t> vErrorMsg = {0x00};
            std::vector<uint8_t> vEncryptedError = EncryptRewardResult(vErrorMsg, vChaChaKey);
            
            StatelessPacket errorResponse(REWARD_RESULT);
            errorResponse.DATA = vEncryptedError;
            errorResponse.LENGTH = static_cast<uint32_t>(vEncryptedError.size());
            
            /* Return success with error response */
            return ProcessResult::Success(context, errorResponse);
        }

        /* Bind reward address to context using dedicated field */
        MiningContext newContext = context.WithRewardAddress(hashReward);

        /* The MINER_SET_REWARD packet already proved the live session can decrypt with the
         * session-derived ChaCha20 key. Persist that same key into the authoritative
         * recovery container so a later lane switch keeps reward binding and
         * decryption state attached to the same miner session. */
        if(!vChaChaKey.empty())
            newContext = newContext.WithChaChaKey(vChaChaKey);

        /* Update the context in StatelessMinerManager to persist the change */
        StatelessMinerManager::Get().UpdateMiner(context.strAddress, newContext, 0);

        if(newContext.fAuthenticated && newContext.hashKeyID != 0)
        {
            SessionRecoveryManager::Get().SaveSession(newContext);
            SessionRecoveryManager::Get().UpdateLane(newContext.hashKeyID, 0);
            if(newContext.fEncryptionReady && !newContext.vChaChaKey.empty())
            {
                uint256_t hashKey(newContext.vChaChaKey);
                SessionRecoveryManager::Get().SaveChaCha20State(newContext.hashKeyID, hashKey, 0);
            }
        }

        /* Log successful binding */
        debug::log(0, FUNCTION, "✓ Reward binding stored: decoded reward register/account hash=", hashReward.GetHex());
        debug::log(1, FUNCTION, "Session updated:");
        debug::log(1, FUNCTION, "  Session genesis: ", context.GenesisHex());
        debug::log(1, FUNCTION, "  Bound reward hash: ", hashReward.GetHex());
        debug::log(1, FUNCTION, "  Bound reward source: MINER_SET_REWARD decrypted payload");
        debug::log(1, FUNCTION, "  Reward hash == prior bound reward hash: ",
                   fExistingRewardPresent ? YesNo(fExistingRewardMatches) : "NOT PREVIOUSLY BOUND");
        debug::log(1, FUNCTION, "  Session recovery state available: ", YesNo(fRecoveredSessionState));
        debug::log(2, FUNCTION, "  ChaCha20 ready: ", YesNo(!vChaChaKey.empty()),
                   " fingerprint=", KeyFingerprint(vChaChaKey));
        debug::log(1, FUNCTION, "  Consistency result: ", PassFail(fExistingRewardMatches && (!fRecoveredSessionState || fRecoveryGenesisMatches)));

        /* Build success response (encrypted) */
        std::vector<uint8_t> vSuccessMsg = {0x01};  // Success status
        std::vector<uint8_t> vEncryptedSuccess = EncryptRewardResult(vSuccessMsg, vChaChaKey);
        
        StatelessPacket response(StatelessOpcodes::REWARD_RESULT);
        response.DATA = vEncryptedSuccess;
        response.LENGTH = static_cast<uint32_t>(vEncryptedSuccess.size());

        return ProcessResult::Success(newContext, response);
    }

} // namespace LLP
