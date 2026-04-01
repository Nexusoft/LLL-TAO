/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_STATELESS_MINER_CONNECTION_H
#define NEXUS_LLP_TYPES_STATELESS_MINER_CONNECTION_H

#include <LLP/templates/stateless_connection.h>
#include <LLP/include/graceful_shutdown.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/channel_state_manager.h>
#include <LLP/include/auto_cooldown.h>
#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_constants.h>
#include <TAO/Ledger/types/block.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <map>
#include <memory>
#include <vector>

namespace LLP
{
    /** StatelessMinerConnection
     *
     *  Phase 2 stateless miner connection handler.
     *  Wraps the pure functional StatelessMiner processor in a Connection class.
     *
     *  This is the dedicated stateless miner LLP connection type for miningport.
     *  It uses MiningContext for immutable state management and routes all packets
     *  through StatelessMiner::ProcessPacket for pure functional processing.
     *
     *  PROTOCOL DESIGN:
     *  - Uses 16-bit stateless packet framing (HEADER + LENGTH + DATA)
     *  - Falcon authentication is mandatory before any mining operations
     *  - All state is managed through immutable MiningContext objects
     *  - Packet processing is stateless and returns ProcessResult with updated context
     *  - Compatible with NexusMiner Phase 2 protocol
     *
     **/
    class StatelessMinerConnection : public StatelessConnection
    {
    private:
        /** The current mining context (immutable snapshot) **/
        MiningContext context;

        /** Mutex for thread-safe context updates **/
        std::mutex MUTEX;

        /** Atomic mirror of context.fAuthenticated for lock-free reads by DataThread.
         *
         *  The DataThread calls IsTimeoutExempt() from its polling loop on every
         *  iteration — taking the MUTEX there would serialize all timeout checks
         *  against packet processing.  This atomic bool is updated under MUTEX
         *  whenever context.fAuthenticated transitions false → true, and is read
         *  lock-free by IsTimeoutExempt().  Because authentication is write-once
         *  (set to true, never cleared), a relaxed store / relaxed load is safe. **/
        std::atomic<bool> fAuthenticatedAtomic{false};

        /** The map to hold the list of blocks that are being mined with metadata. 
         *  Updated in PR #131 to track template metadata for staleness detection. **/
        std::map<uint512_t, TemplateMetadata> mapBlocks;

        /** Used as an ID iterator for generating unique hashes from same block transactions. **/
        static std::atomic<uint32_t> nBlockIterator;

        /** Map of session ID -> Falcon public key for signature verification **/
        std::map<uint32_t, std::vector<uint8_t>> mapSessionKeys;

        /** Mutex for thread-safe session key access **/
        mutable std::mutex SESSION_MUTEX;

        /** Channel state managers for fork-aware mining (PR #136) **/
        std::unique_ptr<PrimeStateManager> m_pPrimeState;
        std::unique_ptr<HashStateManager> m_pHashState;

        /** Per-session in-flight template guard for CreateBlockForStatelessMining().
         *  Concurrent requests wait and reuse the just-created template pointer. */
        std::mutex TEMPLATE_CREATE_MUTEX;
        std::condition_variable TEMPLATE_CREATE_CV;
        bool m_template_create_in_flight{false};
        TAO::Ledger::Block* m_last_created_template{nullptr};

        /** Timestamp of the last template push (SendStatelessTemplate / SendChannelNotification).
         *
         *  Used by the push throttle guard to prevent flooding miners with full
         *  228-byte templates during a fork-resolution burst (multiple SetBest()
         *  events firing in < 100 ms).  Protected by MUTEX.
         **/
        std::chrono::steady_clock::time_point m_last_template_push_time;

        /** When true, the next call to SendChannelNotification() or SendStatelessTemplate()
         *  bypasses the push throttle entirely.  Set by the STATELESS_MINER_READY and
         *  MINER_READY handlers so that a re-subscribing miner always gets an immediate
         *  fresh push regardless of when the previous push was sent.  Protected by MUTEX.
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

        // ═══════════════════════════════════════════════════════════════════════
        // AUTOMATED RATE LIMITING (Layer 2 Protection)
        // ═══════════════════════════════════════════════════════════════════════
        // 
        // SECURITY PRINCIPLES:
        // - Fully automated: NO manual ban capability
        // - Transparent: Clear rules, logged violations
        // - Fair: Only verified bad behavior triggers penalties
        // - Reversible: Temp cooldowns only, auto-expire
        // - Non-invasive: Cannot steal work or target good miners
        // 
        // CONFIGURATION: See src/LLP/include/mining_constants.h for DEBUG vs PRODUCTION
        // ═══════════════════════════════════════════════════════════════════════
        
        struct RateLimitConfig {
            // Session-scoped rolling limit (authoritative, from get_block_policy.h)
            static constexpr uint32_t MAX_GET_BLOCK_PER_MINUTE = static_cast<uint32_t>(GET_BLOCK_ROLLING_LIMIT_PER_MINUTE);
            static_assert(MAX_GET_BLOCK_PER_MINUTE == 25,
                "MAX_GET_BLOCK_PER_MINUTE must equal GET_BLOCK_ROLLING_LIMIT_PER_MINUTE (25). "
                "Update get_block_policy.h if the rolling limit changes.");
            
            // Minimum intervals between consecutive requests of the same type
            static constexpr uint32_t MIN_GET_ROUND_INTERVAL_MS = 5000;   // 5 seconds
            static constexpr uint32_t MIN_SUBMIT_BLOCK_INTERVAL_MS = 1000; // 1 second (lenient)
            
            // Violation thresholds
            static constexpr uint32_t VIOLATIONS_BEFORE_STRIKE = 3;
            static constexpr uint32_t VIOLATIONS_BEFORE_THROTTLE = 6;
            static constexpr uint32_t VIOLATIONS_BEFORE_DISCONNECT = 10;

            // Throttle hysteresis: throttle activates at VIOLATIONS_BEFORE_THROTTLE (high water)
            // and deactivates only when violations drop to THROTTLE_LOW_WATER (low water).
            // This prevents yo-yo behavior for borderline miners.
            static constexpr uint32_t THROTTLE_LOW_WATER = 3;

            /** After this many consecutive GET_BLOCK rate-limit violations with no
             *  successful request between them, the connection is closed with a
             *  diagnostic message to prevent the tight-loop self-DDoS. */
            static constexpr uint32_t MAX_CONSECUTIVE_RATE_LIMIT_STRIKES =
                MiningConstants::RATE_LIMIT_STRIKE_THRESHOLD;
            
            // Cooldown duration (NOT a ban - auto-expires)
            static constexpr uint32_t COOLDOWN_DURATION_SECONDS = 300;  // 5 minutes
            
            // Throttle delay when in throttle mode
            static constexpr uint32_t THROTTLE_DELAY_MS = 10000;  // 10 seconds forced delay
        };
        
        struct RateLimitState {
            // Last request timestamps (used for minimum-interval enforcement)
            std::chrono::steady_clock::time_point tLastGetRound;
            std::chrono::steady_clock::time_point tLastGetBlock;
            std::chrono::steady_clock::time_point tLastSubmitBlock;
            
            // Violation tracking
            uint32_t nViolationCount = 0;
            uint32_t nStrikeCount = 0;
            bool fThrottleMode = false;

            /** Consecutive GET_BLOCK rate-limit counter.
             *
             *  Incremented each time GET_BLOCK is rejected with RATE_LIMIT_EXCEEDED.
             *  Reset to 0 whenever a GET_BLOCK request succeeds.  When it reaches
             *  RateLimitConfig::MAX_CONSECUTIVE_RATE_LIMIT_STRIKES the connection is
             *  closed to stop the tight-loop self-DDoS. */
            uint32_t nConsecutiveRateLimitStrikes = 0;

            /** ResetViolationState
             *
             *  @brief Clear throttle/violation counters on clean re-authentication.
             *  The session-scoped rolling window limiter (GetBlockRollingLimiter)
             *  is keyed by session and self-manages its own window expiry.
             *
             **/
            void ResetViolationState()
            {
                fThrottleMode  = false;
                nViolationCount = 0;
                nStrikeCount   = 0;
                nConsecutiveRateLimitStrikes = 0;
            }
        };
        
        RateLimitState m_rateLimit;

        /** Track whether NODE_SHUTDOWN was already sent on this connection. **/
        GracefulShutdown::NotificationState m_nodeShutdownNotification;

    public:
        /** Default Constructor **/
        StatelessMinerConnection();

        /** Constructor **/
        StatelessMinerConnection(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);

        /** Constructor **/
        StatelessMinerConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);

        /** Default Destructor **/
        ~StatelessMinerConnection();

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name()
        {
            return "StatelessMiner";
        }

        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in] LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;

        /** ProcessPacket
         *
         *  Main message handler once a packet is received.
         *
         *  @return True if no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;

        /** IsTimeoutExempt
         *
         *  Authenticated stateless mining connections are exempt from socket
         *  read-idle timeout. The session-level 24-hour keepalive timeout
         *  governs session expiration; the socket timeout must not kill
         *  long-running authenticated miners during extended mining operations.
         *
         *  @return true if miner is authenticated and should bypass socket timeout.
         *
         **/
        bool IsTimeoutExempt() const final;

        /** GetContext
         *
         *  Get the current mining context (for server-level operations like notifications).
         *  Returns a copy under mutex protection to prevent data races.
         *
         *  @return Copy of the current mining context
         *
         **/
        MiningContext GetContext();

        /** SendChannelNotification
         *
         *  Send a channel-specific push notification to this miner.
         *  Called from server broadcast when blockchain advances.
         *
         *  Updates context with notification statistics after sending.
         *
         **/
        void SendChannelNotification();


        /** SendStatelessTemplate
         *
         *  Send a complete mining template using 16-bit opcode 0xD008 (STATELESS_GET_BLOCK).
         *  This is used by the stateless mining protocol for NexusMiner.
         *
         *  Packet format (228 bytes total):
         *  - Opcode: 0xD008 (2 bytes, big-endian)
         *  - Metadata (12 bytes, big-endian):
         *    - Unified height (4 bytes)
         *    - Channel height (4 bytes)
         *    - Difficulty (4 bytes)
         *  - Block template (216 bytes): Serialized Tritium block
         *
         *  Called after miner sends STATELESS_MINER_READY (0xD007).
         *
         **/
        void SendStatelessTemplate();

        /** SendNodeShutdown
         *
         *  Send a NODE_SHUTDOWN (0xD0FF) packet to notify the miner of graceful shutdown.
         *  The miner should stop its workers cleanly and wait before reconnecting.
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
        
        // ═══════════════════════════════════════════════════════════════════════
        // DIFFICULTY CACHING (Performance Optimization)
        // ═══════════════════════════════════════════════════════════════════════
        
        /** Static difficulty cache (shared across all connections) 
         *  Note: Each atomic is padded to prevent false sharing on cache lines
         **/
        static std::atomic<uint64_t> nDiffCacheTime;
        
        /** Padded cache values to prevent false sharing (64-byte cache line alignment) **/
        struct alignas(64) PaddedDifficultyCache {
            std::atomic<uint32_t> nDifficulty{0};
        };
        static PaddedDifficultyCache nDiffCacheValue[3];  // Per channel [0=PoS, 1=Prime, 2=Hash]
        
        /** GetCachedDifficulty
         *
         *  @brief Get difficulty with 1-second TTL cache
         *  
         *  Reduces expensive GetNextTargetRequired() calls during high mining activity.
         *  Cache is shared across all miner connections for consistency.
         * 
         *  @param[in] nChannel Mining channel (0=PoS, 1=Prime, 2=Hash)
         *  @return Target difficulty bits for the channel
         *
         **/
        static uint32_t GetCachedDifficulty(uint32_t nChannel);

    private:
        /** respond
         *
         *  Sends a stateless packet response.
         *
         *  @param[in] packet The stateless packet to send.
         *
         **/
        void respond(const StatelessPacket& packet);

        /** new_block
         *
         *  Adds a new block to the map.
         *
         *  @return Pointer to newly created block, or nullptr on failure.
         *
         **/
        TAO::Ledger::Block* new_block();

        /** find_block
         *
         *  Determines if the block exists.
         *
         *  @param[in] hashMerkleRoot The merkle root to search for.
         *
         *  @return True if block exists, false otherwise.
         *
         **/
        bool find_block(const uint512_t& hashMerkleRoot);

        /** sign_block
         *
         *  Signs the block to seal the proof of work.
         *
         *  @param[in] nNonce The nonce secret for the block proof.
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return True if block is valid, false otherwise.
         *
         **/
        bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot, const std::vector<uint8_t>& vOffsets = {});

        /** validate_block
         *
         *  Validates the block for the derived miner class.
         *
         *  @param[in] hashMerkleRoot The root hash of the merkle tree.
         *
         *  @return Returns true if block is valid, false otherwise.
         *
         **/
        bool validate_block(const uint512_t& hashMerkleRoot);

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

        /** clear_map
         *
         *  Clear the blocks map.
         *
         **/
        void clear_map();

        /** CleanupStaleTemplates
         *
         *  Remove templates that are no longer valid due to height changes or age.
         *  This is called automatically when blockchain height changes or periodically
         *  to prevent miners from working on stale templates.
         *
         *  @param[in] nCurrentHeight Current blockchain height (templates for other heights are removed)
         *
         **/
        void CleanupStaleTemplates(uint32_t nCurrentHeight);

        /** GetChannelManager (PR #136: Fork-Aware Channel State Management)
         *
         *  Get the appropriate channel state manager for a given channel.
         *  
         *  @param[in] nChannel Mining channel (1=Prime, 2=Hash)
         *  @return Pointer to channel manager, or nullptr if invalid channel
         *
         **/
        ChannelStateManager* GetChannelManager(uint32_t nChannel);

        // ═══════════════════════════════════════════════════════════════════════
        // RATE LIMIT METHODS
        // ═══════════════════════════════════════════════════════════════════════
        
        /** CheckRateLimit
         *
         *  @brief Check if request is allowed under rate limits
         * 
         *  AUTOMATED: No manual override possible
         *  TRANSPARENT: Logs all violations with clear reasons
         *  FAIR: Same rules for all connections
         * 
         *  @param[in] nRequestType The request type being checked (16-bit opcode)
         *  @return true if allowed, false if rate limited
         *
         **/
        bool CheckRateLimit(uint16_t nRequestType, uint32_t* pnRetryAfterMs = nullptr);
        
        /** RecordViolation
         *
         *  @brief Record a rate limit violation
         * 
         *  Graduated response:
         *  - Violations 1-3: Warning only
         *  - Violations 4-6: Add strike
         *  - Violations 7-10: Enable throttle mode
         *  - Violations 11+: Disconnect + cooldown
         * 
         *  @param[in] strReason Human-readable reason for the violation
         *
         **/
        void RecordViolation(const std::string& strReason);

        /** Build per-session/lane limiter key for GET_BLOCK rolling policy. **/
        std::string GetBlockRateKey() const;

        /** Send explicit machine-readable control response for non-success GET_BLOCK outcomes. **/
        void SendGetBlockControlResponse(GetBlockPolicyReason eReason, uint32_t nRetryAfterMs, bool fAuthenticatedPath);

        /** Send BLOCK_DATA and enforce authenticated-path non-empty payload invariant. **/
        void SendGetBlockDataResponse(const std::vector<uint8_t>& vPayload, bool fAuthenticatedPath);
        
        /** IsThrottled
         *
         *  @brief Check if connection should be in throttle mode
         *  @return true if requests should be delayed
         *
         **/
        bool IsThrottled() const { return m_rateLimit.fThrottleMode; }
    };
}

#endif
