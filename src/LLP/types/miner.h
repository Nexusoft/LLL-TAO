/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_MINER_H
#define NEXUS_LLP_TYPES_MINER_H

#include <LLP/templates/connection.h>
#include <LLP/include/graceful_shutdown.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/auto_cooldown.h>
#include <LLP/include/mining_constants.h>
#include <TAO/Ledger/types/block.h>
#include <Legacy/types/coinbase.h>
#include <atomic>
#include <chrono>

//forward declarations
namespace Legacy { class ReserveKey; }

namespace LLP
{

    /** Miner
     *
     *  Connection class that handles requests and responses from miners.
     *
     *  PROTOCOL DESIGN OVERVIEW:
     *  
     *  This LLP implements the Miner Protocol for NexusMiner communication with the Nexus Core node.
     *  
     *  Key Design Principles:
     *  1. Backward Compatibility: Supports both legacy (4-byte) and new (1-byte) SET_CHANNEL payloads
     *     to ensure smooth transition during miner upgrades without protocol breakage.
     *  
     *  2. Stateless Operation:
     *     - Uses Falcon signatures for authentication (no TAO API session required)
     *     - Supports both localhost and remote mining connections
     *     - Immutable MiningContext for state management in dedicated StatelessMinerConnection
     *  
     *  3. Forward Compatibility: SESSION_START and SESSION_KEEPALIVE packet types are defined
     *     and acknowledged but not fully implemented. This allows future miners to use these
     *     packets without breaking existing nodes.
     *  
     *  4. Mining Channels:
     *     - Channel 1: Prime (Fermat prime number discovery)
     *     - Channel 2: Hash (traditional SHA3 hashing)
     *     - Channel 0: Reserved for Proof of Stake (not valid for mining)
     *  
     *  5. Authentication Flow (Stateless Mode):
     *     MINER_AUTH_INIT -> MINER_AUTH_CHALLENGE -> MINER_AUTH_RESPONSE -> MINER_AUTH_RESULT
     *     Uses Falcon post-quantum signatures for security.
     *  
     *  This design coordinates with the unified mining stack implementation across:
     *  - LLL-TAO (this node implementation)
     *  - NexusMiner (miner client)
     *  - NexusInterface (GUI management)
     *
     **/
    class Miner : public Connection
    {
        /* Protocol messages based on Default Packet.
         * All opcode values reference OpcodeUtility::Opcodes (opcode_utility.h).
         * Legacy enum aliases maintained for backward compatibility.
         */
        enum : Packet::message_t
        {
            /** DATA PACKETS **/
            BLOCK_DATA     = OpcodeUtility::Opcodes::BLOCK_DATA,
            SUBMIT_BLOCK   = OpcodeUtility::Opcodes::SUBMIT_BLOCK,
            BLOCK_HEIGHT   = OpcodeUtility::Opcodes::BLOCK_HEIGHT,
            SET_CHANNEL    = OpcodeUtility::Opcodes::SET_CHANNEL,
            BLOCK_REWARD   = OpcodeUtility::Opcodes::BLOCK_REWARD,
            SET_COINBASE   = OpcodeUtility::Opcodes::SET_COINBASE,
            GOOD_BLOCK     = OpcodeUtility::Opcodes::GOOD_BLOCK,
            ORPHAN_BLOCK   = OpcodeUtility::Opcodes::ORPHAN_BLOCK,


            /** DATA REQUESTS **/
            CHECK_BLOCK    = OpcodeUtility::Opcodes::CHECK_BLOCK,
            SUBSCRIBE      = OpcodeUtility::Opcodes::SUBSCRIBE,


            /** REQUEST PACKETS **/
            GET_BLOCK      = OpcodeUtility::Opcodes::GET_BLOCK,
            GET_HEIGHT     = OpcodeUtility::Opcodes::GET_HEIGHT,
            GET_REWARD     = OpcodeUtility::Opcodes::GET_REWARD,


            /** SERVER COMMANDS **/
            CLEAR_MAP      = OpcodeUtility::Opcodes::CLEAR_MAP,
            
            /** GET_ROUND (133) - Multi-Channel Height Information
             *
             *  Returns unified blockchain height + per-channel heights for staleness detection.
             *
             *  NEXUS MULTI-CHANNEL CONSENSUS ARCHITECTURE:
             *  -------------------------------------------
             *  Nexus uses three independent mining channels that compete on the same blockchain:
             *  - Prime channel (1): CPU mining via prime number cluster discovery
             *  - Hash channel (2):  GPU/FPGA mining via SHA3 hashing  
             *  - Stake channel (0): Proof-of-Stake (trust-based)
             *
             *  UNIFIED vs CHANNEL HEIGHTS:
             *  ---------------------------
             *  • Unified Height (nHeight): Increments for EVERY block regardless of channel
             *    Example: Height 6535193 (Hash) → 6535194 (Prime) → 6535195 (Hash) → 6535196 (Stake)
             *
             *  • Channel Height (nChannelHeight): Only increments when THAT SPECIFIC CHANNEL mines a block
             *    Example at unified height 6535196:
             *      - Prime channel height:  2165442 (last Prime block)
             *      - Hash channel height:   4165000 (last Hash block)
             *      - Stake channel height:  235000  (last Stake block)
             *
             *  TEMPLATE STALENESS DETECTION:
             *  -----------------------------
             *  Mining templates should only be discarded when THEIR SPECIFIC CHANNEL advances,
             *  not when other channels mine blocks. This prevents ~40% wasted mining work.
             *
             *  RESPONSE FORMAT (PR #134 - Enhanced GET_ROUND):
             *  ------------------------------------------------
             *  Total: 16 bytes
             *    [0-3]   uint32_t nUnifiedHeight      - Current blockchain height (all channels)
             *    [4-7]   uint32_t nPrimeChannelHeight - Last Prime channel block height
             *    [8-11]  uint32_t nHashChannelHeight  - Last Hash channel block height
             *    [12-15] uint32_t nStakeChannelHeight - Last Stake channel block height
             *
             *  BACKWARD COMPATIBILITY:
             *  -----------------------
             *  Old miners (pre-PR #134): Read 4 bytes (unified height), ignore remaining 12 bytes ✅
             *  New miners (PR #134+):    Read all 16 bytes for enhanced staleness detection ✅
             *  TCP stream protocol allows extra bytes to be ignored by older clients ✅
             *
             *  USAGE:
             *  ------
             *  Miners poll GET_ROUND every 5-10 seconds to check for new blocks:
             *  - If unified height changes: Always fetch new template (any channel advanced)
             *  - If only other channels changed: Keep mining current template (efficiency!)
             *  - If own channel changed: Discard template, fetch new one (correctness!)
             *
             *  EFFICIENCY IMPACT:
             *  ------------------
             *  Before: Templates marked stale when ANY channel mines → ~40% wasted work
             *  After:  Templates marked stale only when SAME channel mines → <5% wasted work
             **/
            GET_ROUND      = OpcodeUtility::Opcodes::GET_ROUND,

            /** MINER_READY (216 / 0xd8) - Subscribe to Push Notifications
             *
             *  Miner → Node: Subscribe to channel-specific push notifications.
             *  
             *  REPLACES: Polling-based GET_ROUND (still supported for backward compatibility).
             *  
             *  PROTOCOL FLOW:
             *  1. Miner authenticates via MINER_AUTH_INIT/RESPONSE
             *  2. Miner sets channel via SET_CHANNEL (1=Prime, 2=Hash)
             *  3. Miner sends MINER_READY (header-only, no payload)
             *  4. Node validates authentication + channel
             *  5. Node sends immediate PRIME_BLOCK_AVAILABLE or HASH_BLOCK_AVAILABLE
             *  6. Node pushes notifications when miner's channel advances
             *  
             *  PAYLOAD: None (header-only packet)
             *  
             *  RESPONSE:
             *  - Success: Immediate PRIME_BLOCK_AVAILABLE or HASH_BLOCK_AVAILABLE
             *  - Error: Connection closed with error message
             *  
             *  REQUIREMENTS:
             *  - Authentication required (fAuthenticated must be true)
             *  - Channel must be 1 (Prime) or 2 (Hash)
             *  - Stake channel (0) is REJECTED (not minable)
             *  
             *  BENEFITS vs GET_ROUND POLLING:
             *  - Instant notification (<10ms vs 0-5s polling delay)
             *  - 50% less network traffic (server-side filtering)
             *  - No rate limiting conflicts
             *  - Reduced node CPU usage (event-driven vs continuous polling)
             *  
             *  CHANNEL ISOLATION:
             *  - Prime miners receive ONLY Prime notifications
             *  - Hash miners receive ONLY Hash notifications
             *  - Server filters before sending (no client-side filtering needed)
             *  
             *  BACKWARD COMPATIBILITY:
             *  - Legacy miners continue using GET_ROUND polling
             *  - Both protocols coexist on same node
             *  - No breaking changes to existing miners
             **/
            MINER_READY    = OpcodeUtility::Opcodes::MINER_READY,


            /** RESPONSE PACKETS **/
            BLOCK_ACCEPTED = OpcodeUtility::Opcodes::BLOCK_ACCEPTED,
            BLOCK_REJECTED = OpcodeUtility::Opcodes::BLOCK_REJECTED,


            /** VALIDATION RESPONSES **/
            COINBASE_SET   = OpcodeUtility::Opcodes::COINBASE_SET,
            COINBASE_FAIL  = OpcodeUtility::Opcodes::COINBASE_FAIL,
            CHANNEL_ACK    = OpcodeUtility::Opcodes::CHANNEL_ACK,

            /** ROUND VALIDATIONS. **/
            NEW_ROUND      = OpcodeUtility::Opcodes::NEW_ROUND,
            OLD_ROUND      = OpcodeUtility::Opcodes::OLD_ROUND,

            /** AUTHENTICATION PACKETS **/
            MINER_AUTH_INIT      = OpcodeUtility::Opcodes::MINER_AUTH_INIT,  // 0xcf - miner -> node: Genesis + Falcon pubkey + miner ID
            MINER_AUTH_CHALLENGE = OpcodeUtility::Opcodes::MINER_AUTH_CHALLENGE,  // 0xd0 - node -> miner: Random nonce challenge
            MINER_AUTH_RESPONSE  = OpcodeUtility::Opcodes::MINER_AUTH_RESPONSE,  // 0xd1 - miner -> node: Falcon signature over nonce
            MINER_AUTH_RESULT    = OpcodeUtility::Opcodes::MINER_AUTH_RESULT,  // 0xd2 - node -> miner: Auth success/failure + session ID

            /** SESSION MANAGEMENT (handled via Node Cache) **/
            // Note: Keep-alive is handled automatically via the node's connection cache
            // SESSION_START (211) and SESSION_KEEPALIVE (212) reserved but not actively used
            SESSION_START        = OpcodeUtility::Opcodes::SESSION_START,  // session start request (not fully implemented yet)
            SESSION_KEEPALIVE    = OpcodeUtility::Opcodes::SESSION_KEEPALIVE,  // session keepalive ping (not fully implemented yet)

            /** REWARD ADDRESS BINDING (encrypted with ChaCha20 after Falcon auth) **/
            MINER_SET_REWARD     = OpcodeUtility::Opcodes::MINER_SET_REWARD,  // 0xd5 - miner -> node: Encrypted reward address (32 bytes)
            MINER_REWARD_RESULT  = OpcodeUtility::Opcodes::MINER_REWARD_RESULT,  // 0xd6 - node -> miner: Encrypted validation result

            /** UNIFIED PUSH NOTIFICATION SYSTEM (PR #230)
             *
             *  Both protocol lanes use identical 12-byte payload:
             *    [0-3]   nUnifiedHeight  (uint32, big-endian)
             *    [4-7]   nChannelHeight  (uint32, big-endian)
             *    [8-11]  nBits           (uint32, big-endian)
             *
             *  Legacy Tritium Protocol lane:    PRIME_BLOCK_AVAILABLE (0xD9), HASH_BLOCK_AVAILABLE (0xDA)
             *  Stateless Tritium Protocol lane: STATELESS_PRIME_BLOCK_AVAILABLE (0xD0D9), STATELESS_HASH_BLOCK_AVAILABLE (0xD0DA)
             *
             *  Unified builder: PushNotificationBuilder::BuildChannelNotification<T>(channel, heights)
             *    - BuildChannelNotification<Packet>          = Legacy Tritium Protocol (8-bit opcodes)
             *    - BuildChannelNotification<StatelessPacket>  = Stateless Tritium Protocol (16-bit opcodes)
             **/

            /** PRIME_BLOCK_AVAILABLE (217 / 0xd9) - Prime Block Notification
             *
             *  Node → Miner: New Prime block has been validated (channel 1 only).
             *  
             *  SERVER-INITIATED: Sent automatically when a Prime block is added to blockchain.
             *  Delivered on both Legacy Tritium Protocol (0xD9) and Stateless Tritium Protocol (0xD0D9) lanes.
             *  
             *  CHANNEL FILTERING:
             *  - Only sent to miners subscribed to Prime channel (1)
             *  - Hash miners (channel 2) never receive this
             *  - Server-side filtering ensures no wasted bandwidth
             *  
             *  PAYLOAD FORMAT (12 bytes, big-endian):
             *    [0-3]   uint32_t nUnifiedHeight   - Current blockchain height (all channels)
             *    [4-7]   uint32_t nPrimeHeight     - Prime channel height
             *    [8-11]  uint32_t nDifficulty      - Current Prime difficulty
             *  
             *  TRIGGER:
             *  - Called from BlockState::SetBest() after Prime block indexing
             *  - Only when GetChannel() returns 1 (Prime)
             *  
             *  MINER ACTION:
             *  - Detect template staleness (if mining)
             *  - Request new template via GET_BLOCK
             *  - Compare heights to avoid unnecessary refreshes
             *  
             *  PERFORMANCE:
             *  - <10ms notification latency
             *  - No polling overhead
             *  - 50% less traffic vs broadcast to all miners
             **/
            PRIME_BLOCK_AVAILABLE = OpcodeUtility::Opcodes::PRIME_BLOCK_AVAILABLE,  // 0xd9 - node -> miner: New Prime block available (channel 1 only)

            /** HASH_BLOCK_AVAILABLE (218 / 0xda) - Hash Block Notification
             *
             *  Node → Miner: New Hash block has been validated (channel 2 only).
             *  
             *  SERVER-INITIATED: Sent automatically when a Hash block is added to blockchain.
             *  Delivered on both Legacy Tritium Protocol (0xDA) and Stateless Tritium Protocol (0xD0DA) lanes.
             *  
             *  CHANNEL FILTERING:
             *  - Only sent to miners subscribed to Hash channel (2)
             *  - Prime miners (channel 1) never receive this
             *  - Server-side filtering ensures no wasted bandwidth
             *  
             *  PAYLOAD FORMAT (12 bytes, big-endian):
             *    [0-3]   uint32_t nUnifiedHeight   - Current blockchain height (all channels)
             *    [4-7]   uint32_t nHashHeight      - Hash channel height
             *    [8-11]  uint32_t nDifficulty      - Current Hash difficulty
             *  
             *  TRIGGER:
             *  - Called from BlockState::SetBest() after Hash block indexing
             *  - Only when GetChannel() returns 2 (Hash)
             *  
             *  MINER ACTION:
             *  - Detect template staleness (if mining)
             *  - Request new template via GET_BLOCK
             *  - Compare heights to avoid unnecessary refreshes
             *  
             *  PERFORMANCE:
             *  - <10ms notification latency
             *  - No polling overhead
             *  - 50% less traffic vs broadcast to all miners
             *  
             *  NOTE ON STAKE BLOCKS:
             *  - Stake blocks (channel 0) do NOT trigger any notifications
             *  - Stake uses Proof-of-Stake, not stateless mining
             *  - No STAKE_BLOCK_AVAILABLE opcode exists (not needed)
             **/
            HASH_BLOCK_AVAILABLE  = OpcodeUtility::Opcodes::HASH_BLOCK_AVAILABLE,  // 0xda - node -> miner: New Hash block available (channel 2 only)

            /** ALIAS OPCODES - Backward Compatibility & Client Clarity
             *
             *  These aliases provide alternative names for existing notification opcodes
             *  to improve code clarity and maintain compatibility with different client
             *  implementations. They map to the same opcode values and handlers.
             *
             *  Purpose:
             *  - Provide semantic clarity about what "new work" means
             *  - Allow clients to use either naming convention
             *  - Document expected client behavior more explicitly
             *
             *  Client Expected Behavior:
             *  - Upon receiving NEW_PRIME_AVAILABLE: Issue GET_BLOCK request
             *  - Upon receiving NEW_HASH_AVAILABLE: Issue GET_BLOCK request
             *  - Fallback to polling GET_ROUND if notifications timeout
             **/
            
            /** NEW_PRIME_AVAILABLE - Alias for PRIME_BLOCK_AVAILABLE
             *
             *  Alternative name emphasizing "new work available" semantics.
             *  Maps to same opcode (217) and handler as PRIME_BLOCK_AVAILABLE.
             *
             *  Client Action: Upon receipt, send GET_BLOCK to fetch new Prime template.
             **/
            NEW_PRIME_AVAILABLE = OpcodeUtility::Opcodes::NEW_PRIME_AVAILABLE,  // Alias for PRIME_BLOCK_AVAILABLE (0xd9)
            
            /** NEW_HASH_AVAILABLE - Alias for HASH_BLOCK_AVAILABLE
             *
             *  Alternative name emphasizing "new work available" semantics.
             *  Maps to same opcode (218) and handler as HASH_BLOCK_AVAILABLE.
             *
             *  Client Action: Upon receipt, send GET_BLOCK to fetch new Hash template.
             **/
            NEW_HASH_AVAILABLE = OpcodeUtility::Opcodes::NEW_HASH_AVAILABLE,   // Alias for HASH_BLOCK_AVAILABLE (0xda)

            /** GENERIC **/
            PING           = OpcodeUtility::Opcodes::PING,
            CLOSE          = OpcodeUtility::Opcodes::CLOSE
        };

        /** Stateless Tritium Protocol 16-bit Opcodes
         *
         *  STATELESS TRITIUM PROTOCOL (Compatible with NexusMiner):
         *  ========================================================
         *
         *  These 16-bit opcodes enable a simplified stateless mining protocol where:
         *  1. Miner connects and sends STATELESS_MINER_READY (0xD0D8 = Mirror(216))
         *  2. Node immediately pushes template via STATELESS_GET_BLOCK (0xD081 = Mirror(129))
         *  3. Miner begins hashing (no authentication or round polling needed)
         *
         *  PACKET FORMAT (16-bit):
         *  - Header: 2 bytes (big-endian, e.g., [0xD0, 0xD8])
         *  - Data: Direct payload (no 4-byte LENGTH field)
         *
         *  MIRROR-MAPPED SCHEME:
         *  - All stateless opcodes follow: statelessOpcode = 0xD000 | legacyOpcode
         *  - This provides 1:1 mapping with Legacy Tritium Protocol
         *  - Simplifies protocol bridging and maintains compatibility
         *
         *  BACKWARD COMPATIBILITY:
         *  - Existing 8-bit opcodes (216-218) continue to work on Legacy Tritium Protocol (port 9325)
         *  - Old miners use MINER_READY (216) with GET_BLOCK polling
         *  - New miners use STATELESS_MINER_READY (0xD0D8) on Stateless Tritium Protocol (port 9323)
         *
         **/
        
        /* 16-bit opcode constants reference OpcodeUtility::Stateless (opcode_utility.h) */
        static const uint16_t STATELESS_MINER_READY = OpcodeUtility::Stateless::MINER_READY;   // Mirror(216): Miner -> Node: Subscribe to template push
        static const uint16_t STATELESS_GET_BLOCK   = OpcodeUtility::Stateless::GET_BLOCK;   // Mirror(129): Node -> Miner: Template push (228 bytes)
        static const uint16_t STATELESS_BLOCK_DATA      = OpcodeUtility::Stateless::BLOCK_DATA;      // Node -> Miner: Block template payload
        static const uint16_t STATELESS_SUBMIT_BLOCK    = OpcodeUtility::Stateless::SUBMIT_BLOCK;    // Miner -> Node: Submit solved block
        static const uint16_t STATELESS_BLOCK_ACCEPTED  = OpcodeUtility::Stateless::BLOCK_ACCEPTED;  // Node -> Miner: Accepted result
        static const uint16_t STATELESS_BLOCK_REJECTED  = OpcodeUtility::Stateless::BLOCK_REJECTED;  // Node -> Miner: Rejected result
        static const uint16_t STATELESS_GET_ROUND       = OpcodeUtility::Stateless::GET_ROUND;       // Miner -> Node: Round status check
        static const uint16_t STATELESS_PRIME_AVAILABLE = OpcodeUtility::Stateless::PRIME_AVAILABLE; // Node -> Miner: Prime template notification
        static const uint16_t STATELESS_HASH_AVAILABLE  = OpcodeUtility::Stateless::HASH_AVAILABLE;  // Node -> Miner: Hash template notification
        static const uint16_t STATELESS_NEW_ROUND       = OpcodeUtility::Stateless::NEW_ROUND;       // Node -> Miner: New round state
        static const uint16_t STATELESS_OLD_ROUND       = OpcodeUtility::Stateless::OLD_ROUND;       // Node -> Miner: Existing round state

    private:

        /* Externally set coinbase to be set on mined blocks */
        Legacy::Coinbase tCoinbaseTx;


        /** Mutex for thread safe data access **/
        std::mutex MUTEX;


        /** The map to hold the list of blocks that are being mined. */
        std::map<uint512_t, TAO::Ledger::Block *> mapBlocks;


        /** Parallel map: block merkle root → hashBestChain snapshot at template creation.
         *
         *  Populated alongside mapBlocks in handle_get_block_stateless().  Used in
         *  check_best_height() to detect same-height reorgs (hashBestChain changes at
         *  the same integer height) that nBestHeight cannot catch.
         *
         *  Lifecycle mirrors mapBlocks: cleared in clear_map(), entries added/removed
         *  together with their mapBlocks counterparts.
         */
        std::map<uint512_t, uint1024_t> mapBlockHashes;


        /** The current best block. **/
        std::atomic<uint32_t> nBestHeight;


        /* Subscribe to display how many blocks connection subscribed to */
        std::atomic<uint32_t> nSubscribed;


        /* The current channel mining for. */
        std::atomic<uint32_t> nChannel;


        /* The last txid on user's signature chain. Used to orphan blocks if another transaction is made while mining. */
        uint512_t nHashLast;


        /* The last height that the notifications processor was run at.  This is used to ensure that events are only processed once
           across all threads when the height changes */
        static std::atomic<uint32_t> nLastNotificationsHeight;


        /** Used as an ID iterator for generating unique hashes from same block transactions. **/
        static std::atomic<uint32_t> nBlockIterator;


        /* Authentication state for stateless miners using Falcon keys */
        std::vector<uint8_t> vMinerPubKey;       // Falcon public key received from miner
        std::string          strMinerId;         // Miner label or ID from payload
        std::vector<uint8_t> vAuthNonce;         // Random challenge nonce generated by node
        std::atomic<bool>    fMinerAuthenticated; // Whether auth succeeded (atomic: read by DataThread via IsTimeoutExempt)
        uint256_t            hashGenesis;        // Miner's genesis (for Falcon auth)
        uint32_t             nSessionId = 0;     // Session ID derived from Falcon key hash
        uint256_t            hashKeyID = 0;      // Falcon key hash for cross-lane disconnect tracking (0 = unauthenticated)

        /* Protocol lane detection flag.
         * Set to true after successful Falcon authentication handshake.
         * When true, respond_auto() uses 16-bit stateless framing; otherwise 8-bit legacy. */
        bool                 fStatelessProtocol{false};

        /* ChaCha20 encryption state (established after Falcon auth) */
        std::vector<uint8_t> vChaChaKey;         // ChaCha20 session key
        bool                 fEncryptionReady;   // ChaCha20 encryption established

        /* Reward address binding (can be different from genesis!) */
        uint256_t            hashRewardAddress;  // Where to send mining rewards
        bool                 fRewardBound;       // True after successful MINER_SET_REWARD

        /* Push notification subscription state */
        bool                 fSubscribedToNotifications;  // Whether miner subscribed to push notifications
        uint32_t             nSubscribedChannel;         // Channel miner subscribed to (1=Prime, 2=Hash)

        /* Per-connection template tracking (channel-specific, not unified).
         * Tracks the channel height at which the last BLOCK_DATA was sent.
         * Used by GET_ROUND auto-send to only send templates when the miner's
         * OWN channel advances, preventing ~40% wasted work from cross-channel triggers. */
        uint32_t             nLastTemplateChannelHeight;

        /* KEEPALIVE v2 telemetry fields.
         * fKeepaliveV2: true once the miner sent a v2 keepalive (len==8) on this connection.
         * nMinerPrevblockSuffix: raw bytes [4..7] of v2 payload (prevblock suffix as-sent). */
        bool                           fKeepaliveV2;
        std::array<uint8_t, 4>         nMinerPrevblockSuffix;

        /** Timestamp of the last template push (SendChannelNotification).
         *
         *  Used by the push throttle guard to prevent flooding miners with
         *  notifications during a fork-resolution burst (multiple SetBest()
         *  events firing in < 100 ms).  Protected by MUTEX.
         **/
        std::chrono::steady_clock::time_point m_last_template_push_time;

        /** When true, the next call to SendChannelNotification() bypasses the push
         *  throttle entirely.  Set by the MINER_READY handler so that a re-subscribing
         *  miner always gets an immediate fresh push regardless of when the previous
         *  push was sent.  Protected by MUTEX.
         **/
        bool m_force_next_push{false};

        /** 1-second rate-limit floor for GET_BLOCK fallback polling.
         *
         *  With the event-driven push model the miner should almost never poll.
         *  This 1-second floor prevents rapid-fire polling abuse.
         *  The cooldown is NOT Reset() after serving a GET_BLOCK — it naturally
         *  expires, allowing miners to retry every 1 second during recovery.
         *  MINER_READY reassigns it to the "never triggered" state so the first
         *  recovery GET_BLOCK is served immediately.
         *  Protected by MUTEX.
         **/
        AutoCoolDown m_get_block_cooldown{std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS)};

        /** Consecutive GET_BLOCK rate-limit counter (legacy lane).
         *
         *  Incremented each time GET_BLOCK is rejected with RATE_LIMIT_EXCEEDED.
         *  Reset to 0 whenever a GET_BLOCK request succeeds.  After
         *  MiningConstants::RATE_LIMIT_STRIKE_THRESHOLD consecutive rejections without
         *  a successful request the connection is closed to prevent a tight-loop
         *  self-DDoS. */
        uint32_t m_nConsecutiveRateLimitStrikes = 0;

    public:

        /** Default Constructor **/
        Miner();


        /** Constructor **/
        Miner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        Miner(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Default Destructor **/
        ~Miner();


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
         static std::string Name()
         {
             return "Miner";
         }


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;


        /** IsTimeoutExempt
         *
         *  Authenticated mining connections are exempt from socket read-idle timeout.
         *  The session-level 24-hour keepalive timeout governs session expiration;
         *  the socket timeout must not kill long-running authenticated miners.
         *
         *  @return true if miner is authenticated and should bypass socket timeout.
         *
         **/
        bool IsTimeoutExempt() const final
        {
            return fMinerAuthenticated;
        }


        /** ProcessPacketStateless
         *
         *  Handles packets from stateless miners.
         *
         *  @param[in] PACKET The packet to process.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool ProcessPacketStateless(const Packet& PACKET);


        /** GetContext
         *
         *  Returns a MiningContext populated from the connection's subscription state.
         *  Required by Server::NotifyChannelMiners() to check subscription and channel.
         *
         *  @return Copy of the current mining context.
         *
         **/
        MiningContext GetContext();


        /** SendChannelNotification
         *
         *  Send a channel-specific push notification to this miner.
         *  Called when miner subscribes via MINER_READY (216/0xD8) and
         *  from Server::NotifyChannelMiners() on block acceptance broadcast.
         *
         *  Uses 8-bit opcodes (217/218) for Legacy Tritium Protocol lane.
         *  See also: PushNotificationBuilder for unified builder supporting both lanes.
         *
         **/
        void SendChannelNotification();



    private:

        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] header_response The header message to send.
         *  @param[in] data The packet data to send.
         *
         **/
        void respond(uint8_t nHeader, const std::vector<uint8_t>& vData = std::vector<uint8_t>());


        /** respond_auto
         *
         *  Unified response dispatch with lane auto-detection.
         *  Checks fStatelessProtocol to determine whether to send 8-bit legacy
         *  framing (respond) or 16-bit stateless framing (respond_stateless).
         *
         *  @param[in] nLegacyOpcode The 8-bit legacy opcode to send.
         *  @param[in] vData The payload data to send.
         *
         **/
        void respond_auto(uint8_t nLegacyOpcode, const std::vector<uint8_t>& vData = std::vector<uint8_t>());


        /** check_best_height
         *
         *  Checks the current height index and updates best height. Clears the block map if the height is outdated.
         *
         *  @return Returns true if best height was outdated, false otherwise.
         *
         **/
        bool check_best_height();


        /** check_round
         *
         *  For Tritium, this checks the mempool to make sure that there are no new transactions that would be orphaned by the
         *  the current round block.
         *
         *  @return Returns false if the current round is no longer valid.
         *
         **/
        bool check_round();


        /** clear_map
         *
         *  Clear the blocks map.
         *
         **/
        void clear_map();


        /** find_block
         *
         *  Determines if the block exists.
         *
         **/
        bool find_block(const uint512_t& hashMerkleRoot);


        /** new_block
         *
         *  Adds a new block to the map.
         *
         **/
        TAO::Ledger::Block *new_block();


        /** validate_block
         *
         *  validates the block for the derived miner class.
         *
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool validate_block(const uint512_t& hashMerkleRoot);


        /** sign_block
         *
         *  signs the block to seal the proof of work.
         *
         *  @param[in] nNonce The nonce secret for the block proof.
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *  @param[in] vOffsets Miner-submitted Prime Cunningham chain offsets.
         *                     Hash submissions should pass an empty vector.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot, const std::vector<uint8_t>& vOffsets = {});


        /** is_locked
         *
         *  Returns true if the mining wallet locked, false otherwise.
         *
         **/
        bool is_locked();


        /** is_prime_mod
         *
         *  Helper function used for prime channel modification rule in loop.
         *  Returns true if the condition is satisfied, false otherwise.
         *
         *  @param[in] nBitMask The bitMask for the highest order bits of a block hash to check for to satisfy rule.
         *  @param[in] pBlock The block to check.
         *
         **/
        bool is_prime_mod(uint32_t nBitMask, TAO::Ledger::Block *pBlock);


        /** handle_get_block_stateless
         *
         *  Stateless handler for GET_BLOCK - creates a block template and sends BLOCK_DATA.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool handle_get_block_stateless();


        /** handle_miner_ready_stateless
         *
         *  Stateless handler for MINER_READY - subscribes to push notifications and sends template.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool handle_miner_ready_stateless();


        /** handle_submit_block_stateless
         *
         *  Stateless handler for SUBMIT_BLOCK - validates and processes a block submission.
         *
         *  @param[in] PACKET The packet containing the block submission data.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool handle_submit_block_stateless(const Packet& PACKET);


        /** handle_get_round_stateless
         *
         *  Stateless handler for GET_ROUND - sends NEW_ROUND with height information.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool handle_get_round_stateless();


        /** respond_stateless
         *
         *  Sends a stateless (16-bit) protocol response packet.
         *
         *  @param[in] nOpcode The 16-bit stateless opcode (e.g., STATELESS_BLOCK_DATA).
         *  @param[in] vData The payload data to send.
         *
         **/
        void respond_stateless(uint16_t nOpcode, const std::vector<uint8_t>& vData = std::vector<uint8_t>());


        /** Track whether NODE_SHUTDOWN was already sent on this connection. **/
        GracefulShutdown::NotificationState m_nodeShutdownNotification;

        /** Set when disconnect/shutdown begins so in-flight push notifications can abort early. **/
        std::atomic<bool> m_shutdownRequested{false};


    public:

        /** Mark this miner as shutting down and close the socket to interrupt any in-flight notification work. **/
        void RequestShutdown()
        {
            m_shutdownRequested.store(true, std::memory_order_release);
            Disconnect();
        }

        /** SendNodeShutdown
         *
         *  Send a NODE_SHUTDOWN (0xD0FF) packet to notify the miner of graceful shutdown.
         *  Uses the same wire format as the stateless lane so both legacy and
         *  stateless miners receive an identical shutdown notification.
         *
         *  @param[in] nReasonCode  Shutdown reason: 1=GRACEFUL, 2=MAINTENANCE
         *
         **/
        void SendNodeShutdown(uint32_t nReasonCode = GracefulShutdown::REASON_GRACEFUL);


        /** Check whether NODE_SHUTDOWN was already attempted on this connection. **/
        bool NodeShutdownSent() const
        {
            return m_nodeShutdownNotification.Sent();
        }

        /** Check whether disconnect/shutdown has been requested on this connection. **/
        bool ShutdownRequested() const
        {
            return m_shutdownRequested.load(std::memory_order_acquire);
        }

    };
}

#endif
