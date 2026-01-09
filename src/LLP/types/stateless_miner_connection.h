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

#include <LLP/templates/connection.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/channel_state_manager.h>
#include <TAO/Ledger/types/block.h>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>

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
     *  - Falcon authentication is mandatory before any mining operations
     *  - All state is managed through immutable MiningContext objects
     *  - Packet processing is stateless and returns ProcessResult with updated context
     *  - Compatible with NexusMiner Phase 2 protocol
     *
     **/
    class StatelessMinerConnection : public Connection
    {
    private:
        /** The current mining context (immutable snapshot) **/
        MiningContext context;

        /** Mutex for thread-safe context updates **/
        std::mutex MUTEX;

        /** The map to hold the list of blocks that are being mined with metadata. 
         *  Updated in PR #131 to track template metadata for staleness detection. **/
        std::map<uint512_t, TemplateMetadata> mapBlocks;

        /** Used as an ID iterator for generating unique hashes from same block transactions. **/
        static std::atomic<uint32_t> nBlockIterator;

        /** Disposable Falcon wrapper for signature verification **/
        std::unique_ptr<LLP::DisposableFalcon::IDisposableFalconWrapper> m_pFalconWrapper;

        /** Map of session ID -> Falcon public key for signature verification **/
        std::map<uint32_t, std::vector<uint8_t>> mapSessionKeys;

        /** Mutex for thread-safe session key access **/
        mutable std::mutex SESSION_MUTEX;

        /** Channel state managers for fork-aware mining (PR #136) **/
        std::unique_ptr<PrimeStateManager> m_pPrimeState;
        std::unique_ptr<HashStateManager> m_pHashState;

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
        // ═══════════════════════════════════════════════════════════════════════
        
        struct RateLimitConfig {
            // Request limits per minute
            static constexpr uint32_t MAX_GET_ROUND_PER_MINUTE = 12;
            static constexpr uint32_t MAX_GET_BLOCK_PER_MINUTE = 10;
            static constexpr uint32_t MAX_SUBMIT_BLOCK_PER_MINUTE = 60;  // Lenient for solutions!
            static constexpr uint32_t MAX_SET_CHANNEL_PER_MINUTE = 5;
            
            // Minimum intervals (milliseconds)
            static constexpr uint32_t MIN_GET_ROUND_INTERVAL_MS = 5000;   // 5 seconds
            static constexpr uint32_t MIN_GET_BLOCK_INTERVAL_MS = 6000;   // 6 seconds
            static constexpr uint32_t MIN_SUBMIT_BLOCK_INTERVAL_MS = 1000; // 1 second (lenient)
            
            // Violation thresholds
            static constexpr uint32_t VIOLATIONS_BEFORE_STRIKE = 3;
            static constexpr uint32_t VIOLATIONS_BEFORE_THROTTLE = 6;
            static constexpr uint32_t VIOLATIONS_BEFORE_DISCONNECT = 10;
            
            // Cooldown duration (NOT a ban - auto-expires)
            static constexpr uint32_t COOLDOWN_DURATION_SECONDS = 300;  // 5 minutes
            
            // Throttle delay when in throttle mode
            static constexpr uint32_t THROTTLE_DELAY_MS = 10000;  // 10 seconds forced delay
        };
        
        struct RateLimitState {
            // Per-minute request counters
            uint32_t nGetRoundCount = 0;
            uint32_t nGetBlockCount = 0;
            uint32_t nSubmitBlockCount = 0;
            uint32_t nSetChannelCount = 0;
            
            // Last request timestamps
            std::chrono::steady_clock::time_point tLastGetRound;
            std::chrono::steady_clock::time_point tLastGetBlock;
            std::chrono::steady_clock::time_point tLastSubmitBlock;
            std::chrono::steady_clock::time_point tLastCounterReset;
            
            // Violation tracking
            uint32_t nViolationCount = 0;
            uint32_t nStrikeCount = 0;
            bool fThrottleMode = false;
            
            // Initialize timestamps
            RateLimitState() : tLastCounterReset(std::chrono::steady_clock::now()) {}
        };
        
        RateLimitState m_rateLimit;

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

        /** GetContext
         *
         *  Get the current mining context (for server-level operations like notifications).
         *
         *  @return Reference to the current mining context
         *
         **/
        MiningContext& GetContext();

        /** SendChannelNotification
         *
         *  Send a channel-specific push notification to this miner.
         *  Called from server broadcast when blockchain advances.
         *
         *  Updates context with notification statistics after sending.
         *
         **/
        void SendChannelNotification();

    private:
        /** respond
         *
         *  Sends a packet response.
         *
         *  @param[in] packet The packet to send.
         *
         **/
        void respond(const Packet& packet);

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
        bool sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot);

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
         *  @param[in] nRequestType The request type being checked
         *  @return true if allowed, false if rate limited
         *
         **/
        bool CheckRateLimit(uint8_t nRequestType);
        
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
        
        /** ResetMinuteCounters
         *
         *  @brief Reset per-minute counters (called every 60 seconds)
         *
         **/
        void ResetMinuteCounters();
        
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
