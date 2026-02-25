/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_COLIN_MINING_AGENT_H
#define NEXUS_LLP_INCLUDE_COLIN_MINING_AGENT_H

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <cstdint>

namespace LLP
{

    /** ColinMiningAgent
     *
     *  Context-Oriented LLP Intelligence Node (Mining Side).
     *
     *  Node-side diagnostic agent that monitors connected miners and emits
     *  structured health reports at a configurable interval.  Designed to work
     *  alongside the NexusMiner SIM Link dual-connection architecture.
     *
     *  Configuration (nexus.conf):
     *    colin=1              Enable agent (default: 1)
     *    colin_interval=60    Report interval in seconds (default: 60)
     *
     **/
    class ColinMiningAgent
    {
    public:

        /** Get
         *
         *  Returns the global singleton instance.
         *
         **/
        static ColinMiningAgent& Get();


        /** start
         *
         *  Starts the periodic report thread.  Reads colin_interval from config.
         *  No-op if already running or if colin=0 in config.
         *
         **/
        void start();


        /** stop
         *
         *  Stops the periodic report thread and emits one final report.
         *
         **/
        void stop();


        /** on_block_submitted
         *
         *  Called by mining LLP handlers when a block submission is processed.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis
         *  @param[in] channel         Mining channel (1=Prime, 2=Hash)
         *  @param[in] accepted        True if the block was accepted
         *  @param[in] rejection_reason  Reason string if rejected (empty if accepted)
         *
         **/
        void on_block_submitted(const std::string& genesis_prefix, uint32_t channel,
                                bool accepted, const std::string& rejection_reason);


        /** on_template_pushed
         *
         *  Called when a block template push notification is sent to a miner.
         *
         *  @param[in] channel  Mining channel (1=Prime, 2=Hash)
         *  @param[in] height   Unified block height of the pushed template
         *
         **/
        void on_template_pushed(uint32_t channel, uint32_t height);


        /** on_miner_connected
         *
         *  Called when a miner connects and authenticates.
         *  Detects SIM Link (dual connection) when same genesis connects twice.
         *
         *  @param[in] genesis_prefix    First 8 hex chars of miner genesis
         *  @param[in] remote_endpoint   Remote IP:port string
         *
         **/
        void on_miner_connected(const std::string& genesis_prefix,
                                const std::string& remote_endpoint);


        /** on_miner_disconnected
         *
         *  Called when a miner disconnects.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis
         *
         **/
        void on_miner_disconnected(const std::string& genesis_prefix);


        /** on_get_block_received
         *
         *  Called when a GET_BLOCK request is received.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis
         *  @param[in] rate_limited    True if the request was rate-limited
         *
         **/
        void on_get_block_received(const std::string& genesis_prefix, bool rate_limited);


        /** check_and_record_submission
         *
         *  Thread-safe deduplication check for SUBMIT_BLOCK across connections.
         *
         *  Implements a 10-second TTL cache keyed by (nHeight, nNonce, merkleHex).
         *  When the NexusMiner SIM Link architecture submits the same block on both
         *  the legacy (port 8323) and stateless (port 9323) lanes simultaneously,
         *  only the first submission returns false (not a duplicate).  The second
         *  returns true (duplicate) and should be silently rejected.
         *
         *  The cache is cleared when clear_dedup_cache() is called (i.e., on new round).
         *
         *  @param[in] nHeight    Unified block height from template
         *  @param[in] nNonce     Block nonce submitted by miner
         *  @param[in] merkleHex  Hex string of hashMerkleRoot
         *
         *  @return true if this is a duplicate submission (already seen within 10s)
         *
         **/
        bool check_and_record_submission(uint32_t nHeight, uint64_t nNonce,
                                          const std::string& merkleHex);


        /** clear_dedup_cache
         *
         *  Clears the SUBMIT_BLOCK deduplication cache.
         *  Should be called when a new mining round starts (e.g., clear_map()).
         *
         **/
        void clear_dedup_cache();


    private:

        /** Private constructor — use Get() for singleton access **/
        ColinMiningAgent();

        /** Destructor — joins report thread **/
        ~ColinMiningAgent();

        /* Non-copyable */
        ColinMiningAgent(const ColinMiningAgent&) = delete;
        ColinMiningAgent& operator=(const ColinMiningAgent&) = delete;


        /** run_report — thread entry point **/
        void run_report();


        /** emit_report — formats and logs the diagnostic report **/
        void emit_report();


        /** Per-miner statistics **/
        struct MinerStats
        {
            std::string genesis_prefix;
            std::string remote_endpoint;
            uint32_t connection_count{0};       // >1 indicates SIM Link
            uint32_t blocks_submitted{0};
            uint32_t blocks_accepted{0};
            uint32_t blocks_rejected{0};
            std::string last_rejection_reason;
            uint32_t get_block_count{0};
            uint32_t get_block_rate_limited{0};
            std::chrono::steady_clock::time_point connected_at;
            std::chrono::steady_clock::time_point last_submit;
        };

        /** Global push statistics (across all miners) **/
        struct GlobalStats
        {
            uint32_t prime_pushes{0};
            uint32_t hash_pushes{0};
        };

        std::mutex                          m_mutex;
        std::map<std::string, MinerStats>   m_miners;     // keyed by genesis_prefix
        GlobalStats                         m_global;
        uint32_t                            m_interval_s;
        std::thread                         m_report_thread;
        std::atomic<bool>                   m_stop{false};
        std::atomic<bool>                   m_running{false};

        /** Cross-connection SUBMIT_BLOCK deduplication cache.
         *  Keyed by hash of (nHeight, nNonce, merkleHex).  TTL = 10 seconds.
         *  Protected by m_dedup_mutex (separate from m_mutex to avoid contention).
         **/
        std::unordered_map<size_t, std::chrono::steady_clock::time_point> m_dedup_cache;
        std::mutex m_dedup_mutex;

        /** TTL for dedup cache entries (milliseconds) **/
        static constexpr uint32_t DEDUP_TTL_MS = 10000;
    };

} // namespace LLP

#endif
