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
#include <cassert>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace LLP
{

    /** PingFrame
     *
     *  64-byte cache-line-aligned diagnostic PING payload sent by the node
     *  to each active miner during every Colin Agent emit_report() cycle.
     *
     *  Wire format: big-endian. Both lanes supported:
     *    LEGACY    opcode: 0xE0 (8-bit)
     *    STATELESS opcode: 0xD0E0 (16-bit)
     *
     *  LAYOUT (64 bytes):
     *    [0-1]   uint16_t opcode              0xD0E0 stateless / 0xE0 legacy
     *    [2-3]   uint16_t version             0x0001
     *    [4-7]   uint32_t sequence            Monotonic per-miner counter
     *    [8-15]  uint64_t timestamp_send_us   Node send time (µs since epoch)
     *    [16-19] uint32_t unified_height      Chain height at send time
     *    [20-23] uint32_t channel_height      Channel-specific height at send time
     *    [24-27] uint32_t prime_pushes_30s    Prime pushes in last ~30s window
     *    [28-31] uint32_t hash_pushes_30s     Hash pushes in last ~30s window
     *    [32-35] uint32_t blocks_submitted    Miner submissions this interval
     *    [36-39] uint32_t blocks_accepted     Miner accepted this interval
     *    [40-43] uint32_t blocks_rejected     Miner rejected this interval
     *    [44-47] uint32_t get_block_count     GET_BLOCK requests this interval
     *    [48]    uint8_t  health_flags        Node-side health bit field
     *    [49]    uint8_t  channel_id          0x01=PRIME 0x02=HASH
     *    [50-51] uint16_t reserved            0x0000
     *    [52-63] uint8_t  padding[12]         Zero-fill
     *
     *  health_flags bits:
     *    7  STALE_TEMPLATE_DETECTED
     *    6  RATE_LIMITED
     *    5  SIM_LINK_ACTIVE
     *    4  DEDUP_HIT
     *    3  CHANNEL_MISMATCH
     *    2  HIGH_REJECTION_RATE  (rejected > 20%)
     *    1  FIRST_CONNECT
     *    0  NODE_SYNCING
     **/
    struct alignas(64) PingFrame
    {
        uint16_t opcode{0};
        uint16_t version{0x0001};
        uint32_t sequence{0};
        uint64_t timestamp_send_us{0};
        uint32_t unified_height{0};
        uint32_t channel_height{0};
        uint32_t prime_pushes_30s{0};
        uint32_t hash_pushes_30s{0};
        uint32_t blocks_submitted{0};
        uint32_t blocks_accepted{0};
        uint32_t blocks_rejected{0};
        uint32_t get_block_count{0};
        uint8_t  health_flags{0};
        uint8_t  channel_id{0};
        uint16_t reserved{0};
        uint8_t  padding[12]{};

        /* health_flags bit constants */
        static constexpr uint8_t FLAG_STALE_TEMPLATE    = 0x80;
        static constexpr uint8_t FLAG_RATE_LIMITED       = 0x40;
        static constexpr uint8_t FLAG_SIM_LINK_ACTIVE    = 0x20;
        static constexpr uint8_t FLAG_DEDUP_HIT          = 0x10;
        static constexpr uint8_t FLAG_CHANNEL_MISMATCH   = 0x08;
        static constexpr uint8_t FLAG_HIGH_REJECT_RATE   = 0x04;
        static constexpr uint8_t FLAG_FIRST_CONNECT      = 0x02;
        static constexpr uint8_t FLAG_NODE_SYNCING       = 0x01;

        /** Serialize to 64-byte big-endian wire vector **/
        std::vector<uint8_t> Serialize() const;
    };
    static_assert(sizeof(PingFrame) == 64, "PingFrame must be exactly 64 bytes");


    /** PongRecord
     *
     *  Stores the most recently received PongFrame data for a miner session.
     *  Populated asynchronously when PONG_DIAG (0xD0E1 / 0xE1) arrives.
     *  Read by emit_report() at the next Colin cycle.
     *
     *  PongFrame wire layout (64 bytes, big-endian):
     *    [0-1]   uint16_t opcode              0xD0E1 stateless / 0xE1 legacy
     *    [2-3]   uint16_t version             Echo of PingFrame version
     *    [4-7]   uint32_t sequence            Echo of PingFrame sequence
     *    [8-15]  uint64_t timestamp_send_us   Echo of PingFrame timestamp_send_us
     *    [16-23] uint64_t timestamp_recv_us   Miner receive time (µs since epoch)
     *    [24-27] uint32_t miner_hash_rate     kH/s (or chains/s for Prime)
     *    [28-31] uint32_t miner_temp_cdeg     Temperature * 10 in Celsius (optional, 0=unknown)
     *    [32-35] uint32_t miner_threads       Active mining thread count
     *    [36-39] uint32_t miner_queue_depth   Internal work queue depth
     *    [40]    uint8_t  miner_health_flags  Miner-side health bit field
     *    [41]    uint8_t  echo_channel_id     Echo of PingFrame channel_id
     *    [42-43] uint16_t reserved            0x0000
     *    [44-63] uint8_t  padding[20]         Zero-fill
     *
     *  miner_health_flags bits:
     *    7  HASH_RATE_ZERO     worker stalled (zero hash rate)
     *    6  TEMP_CRITICAL      temperature > 85°C
     *    5  QUEUE_OVERFLOW     work queue depth exceeded threshold
     *    4  THREAD_ZERO        no active mining threads
     *    3  HIGH_STALE_RATE    stale rate > 10%
     *    2  LOW_ACCEPT_RATE    accept rate < 80%
     *    1  RECONNECT_WINDOW   reconnected within last 60s
     *    0  FIRST_PONG         first PONG of this session
     **/
    struct PongRecord
    {
        uint32_t sequence{0};
        uint64_t rtt_us{0};          // Computed: now_us - timestamp_send_us on pong receive
        uint64_t rtt_prev_us{0};     // Previous cycle RTT for trend
        uint32_t miner_hash_rate{0};
        uint32_t miner_temp_cdeg{0};
        uint32_t miner_threads{0};
        uint32_t miner_queue_depth{0};
        uint8_t  miner_health_flags{0};
        bool     pong_received{false};

        /* miner_health_flags bit constants */
        static constexpr uint8_t MFLAG_HASH_RATE_ZERO    = 0x80; // worker stalled
        static constexpr uint8_t MFLAG_TEMP_CRITICAL      = 0x40; // temp > 85°C
        static constexpr uint8_t MFLAG_QUEUE_OVERFLOW     = 0x20; // queue depth > threshold
        static constexpr uint8_t MFLAG_THREAD_ZERO        = 0x10; // no active threads
        static constexpr uint8_t MFLAG_HIGH_STALE_RATE    = 0x08; // stale > 10%
        static constexpr uint8_t MFLAG_LOW_ACCEPT_RATE    = 0x04; // accept < 80%
        static constexpr uint8_t MFLAG_RECONNECT_WINDOW   = 0x02; // reconnected in last 60s
        static constexpr uint8_t MFLAG_FIRST_PONG         = 0x01; // first PONG this session
    };


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


        /** on_keepalive_ack
         *
         *  Called by miner.cpp (SESSION_KEEPALIVE reply) and stateless_miner.cpp
         *  (KEEPALIVE_V2_ACK reply) immediately after BuildUnifiedResponse() is sent.
         *
         *  Stores the unified chain-state snapshot for the connected miner so that
         *  Colin's periodic report can display live heights and fork canary status.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis (or empty string on legacy path before genesis is known)
         *  @param[in] unified_height  Node unified height echoed in the reply
         *  @param[in] prime_height    Node Prime channel height echoed in the reply
         *  @param[in] hash_height     Node Hash channel height echoed in the reply
         *  @param[in] stake_height    Node Stake channel height echoed in the reply
         *  @param[in] fork_score      Fork divergence score (0=healthy)
         *
         **/
        void on_keepalive_ack(const std::string& genesis_prefix,
                              uint32_t unified_height,
                              uint32_t prime_height,
                              uint32_t hash_height,
                              uint32_t stake_height,
                              uint32_t fork_score);


        /** on_pong_received
         *
         *  Called by stateless_miner_connection when a PONG_DIAG frame arrives.
         *  Parses the 64-byte PongFrame payload, computes RTT, stores in MinerStats.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis
         *  @param[in] payload         Raw 64-byte PongFrame wire data
         *
         **/
        void on_pong_received(const std::string& genesis_prefix,
                              const std::vector<uint8_t>& payload);


        /** on_canonical_snap_updated
         *
         *  Called by GET_BLOCK handler and SendStatelessTemplate() after a
         *  CanonicalChainState snapshot is captured and stored in MiningContext.
         *
         *  Records the snapshot's age (time since canonical_received_at) and
         *  staleness flag in MinerStats.  The periodic Colin report emits a
         *  [WARN] for any miner whose latest snapshot is stale.
         *
         *  @param[in] genesis_prefix  First 8 hex chars of miner genesis
         *  @param[in] snap_age_ms     Age of the snapshot in milliseconds
         *  @param[in] is_stale        True if snapshot.is_canonically_stale()
         *
         **/
        void on_canonical_snap_updated(const std::string& genesis_prefix,
                                       uint64_t snap_age_ms,
                                       bool is_stale);


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
            uint32_t    ping_sequence{0};   // Monotonic ping counter for this miner
            PongRecord  last_pong;          // Latest pong data (updated async)
            std::deque<uint64_t> rtt_history; // Rolling RTT history (last RTT_HISTORY_SIZE samples, µs)

            // Keepalive ACK telemetry (populated by on_keepalive_ack)
            uint32_t keepalive_ack_count{0};
            uint32_t last_unified_height{0};
            uint32_t last_prime_height{0};
            uint32_t last_hash_height{0};
            uint32_t last_stake_height{0};
            uint32_t last_fork_score{0};
            uint32_t peak_fork_score{0};
            std::chrono::steady_clock::time_point last_keepalive_ack_at{};

            // Canonical snapshot diagnostics (populated by on_canonical_snap_updated)
            uint64_t last_canonical_snap_age_ms{0};  ///< Age of canonical_snap in ms (0=unknown)
            bool     last_canonical_snap_stale{false}; ///< True when snap.is_canonically_stale()
        };

        /** Global push statistics (across all miners) **/
        struct GlobalStats
        {
            uint32_t prime_pushes{0};
            uint32_t hash_pushes{0};
        };


        /** run_report — thread entry point **/
        void run_report();


        /** emit_report — formats and logs the diagnostic report **/
        void emit_report();


        /** build_ping_frame
         *
         *  Builds a PingFrame for a given miner using the current MinerStats snapshot.
         *
         *  @param[in] stats        Snapshot of per-miner stats
         *  @param[in] global       Snapshot of global push stats
         *  @param[in] u_height     Unified chain height
         *  @param[in] c_height     Channel-specific height
         *  @param[in] channel_id   0x01=PRIME, 0x02=HASH
         *
         *  @return Populated PingFrame ready for serialization
         *
         **/
        PingFrame build_ping_frame(const MinerStats& stats,
                                   const GlobalStats& global,
                                   uint32_t u_height,
                                   uint32_t c_height,
                                   uint8_t  channel_id) const;

        /** format_rtt — format microsecond RTT as human string (e.g. "4.2ms") **/
        static std::string format_rtt(uint64_t rtt_us);

        /** format_temp — format cdeg temperature (e.g. "68.3°C" or "n/a") **/
        static std::string format_temp(uint32_t cdeg);

        /** format_miner_health — expand miner_health_flags to a status string **/
        static std::string format_miner_health(uint8_t flags);

        /** format_rtt_trend — compute RTT trend from rolling history
         *
         *  Returns a string like "▼ -0.3ms improving", "▲ +1.2ms degrading", or "─ stable".
         *  Returns empty string if history has fewer than 2 samples.
         *
         **/
        static std::string format_rtt_trend(const std::deque<uint64_t>& history);

        /** Number of RTT samples retained per miner for trend computation **/
        static constexpr size_t RTT_HISTORY_SIZE = 5;

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
