/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NODE_POOL_HEALTH_MONITOR_H
#define NEXUS_LLP_INCLUDE_NODE_POOL_HEALTH_MONITOR_H

/* Node Pool Health Monitor
 *
 * Monitors node health when hundreds of miners are connected on both
 * lanes (Stateless port 9323, Legacy port 8323) simultaneously.
 *
 * Tracks per-lane connection counts, template throughput, error rates,
 * and enforces configurable MAX_MINERS capacity limits.
 *
 * This is a node-only diagnostic and capacity tool — no miner-side
 * behavior changes. Miners receive the same lane-active status as before.
 */

#include <LLP/include/push_notification.h>  /* ProtocolLane */

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace LLP
{
    /** LaneStatus — health status of a mining lane. **/
    enum class LaneStatus : uint8_t
    {
        ACTIVE    = 0,   /* Lane operating normally */
        DEGRADED  = 1,   /* Lane has elevated error rate or latency */
        OVERLOADED = 2   /* Lane at or near capacity */
    };

    /** Return human-readable string for LaneStatus. **/
    inline const char* LaneStatusString(LaneStatus s)
    {
        switch(s)
        {
            case LaneStatus::ACTIVE:     return "ACTIVE";
            case LaneStatus::DEGRADED:   return "DEGRADED";
            case LaneStatus::OVERLOADED: return "OVERLOADED";
            default:                     return "UNKNOWN";
        }
    }


    /** NodePoolHealthMonitor
     *
     *  Singleton that tracks per-lane health metrics for the mining node.
     *
     *  Thread-safe: all counters are atomic; periodic log is guarded by mutex.
     *
     **/
    class NodePoolHealthMonitor
    {
    public:
        /** Get singleton instance. **/
        static NodePoolHealthMonitor& Get();

        /* ── Connection tracking ────────────────────────────────────────────── */

        /** Record a new miner connection on a lane. **/
        void RecordConnect(ProtocolLane lane);

        /** Record a miner disconnect from a lane. **/
        void RecordDisconnect(ProtocolLane lane);

        /** Get current connection count for a lane. **/
        uint32_t GetConnected(ProtocolLane lane) const;

        /** Get total connections across both lanes. **/
        uint32_t GetTotalConnected() const;

        /* ── Template throughput ────────────────────────────────────────────── */

        /** Record a template served on a lane. **/
        void RecordTemplateServed(ProtocolLane lane);

        /** Record a template creation failure on a lane. **/
        void RecordTemplateFailure(ProtocolLane lane);

        /** Record a rate-limit hit on a lane. **/
        void RecordRateLimitHit(ProtocolLane lane);

        /* ── Lane status ────────────────────────────────────────────────────── */

        /** Get the health status of a lane.
         *  Uses connection count and error rate to determine status. **/
        LaneStatus GetLaneStatus(ProtocolLane lane) const;

        /* ── Capacity enforcement ───────────────────────────────────────────── */

        /** Check if total miners have reached the configured maximum.
         *  Default MAX_MINERS is 500, configurable via -maxminers. **/
        bool IsAtCapacity() const;

        /** Get the configured maximum miners. **/
        uint32_t GetMaxMiners() const;

        /* ── Periodic logging ───────────────────────────────────────────────── */

        /** Log pool health summary.  Called periodically (every 60s) by the
         *  stateless manager cleanup timer or push dispatcher. **/
        void LogHealthSummary();


    private:
        NodePoolHealthMonitor();

        /* Per-lane atomic counters */
        std::atomic<uint32_t> nStatelessConnected{0};
        std::atomic<uint32_t> nLegacyConnected{0};

        std::atomic<uint64_t> nStatelessTemplatesServed{0};
        std::atomic<uint64_t> nLegacyTemplatesServed{0};

        std::atomic<uint64_t> nStatelessTemplateFailures{0};
        std::atomic<uint64_t> nLegacyTemplateFailures{0};

        std::atomic<uint64_t> nStatelessRateLimitHits{0};
        std::atomic<uint64_t> nLegacyRateLimitHits{0};

        /* Timestamp of last health log to enforce 60s interval */
        std::mutex m_logMutex;
        uint64_t nLastLogTimestamp{0};
    };

} // namespace LLP

#endif
