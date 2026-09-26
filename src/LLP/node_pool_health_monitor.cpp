/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_pool_health_monitor.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /* Singleton accessor */
    NodePoolHealthMonitor& NodePoolHealthMonitor::Get()
    {
        static NodePoolHealthMonitor instance;
        return instance;
    }

    NodePoolHealthMonitor::NodePoolHealthMonitor()
    {
    }

    /* ── Connection tracking ────────────────────────────────────────────────── */

    void NodePoolHealthMonitor::RecordConnect(ProtocolLane lane)
    {
        if(lane == ProtocolLane::STATELESS)
            nStatelessConnected.fetch_add(1, std::memory_order_relaxed);
        else
            nLegacyConnected.fetch_add(1, std::memory_order_relaxed);
    }

    void NodePoolHealthMonitor::RecordDisconnect(ProtocolLane lane)
    {
        if(lane == ProtocolLane::STATELESS)
        {
            /* Guard against underflow from unmatched disconnect calls */
            uint32_t nCurrent = nStatelessConnected.load(std::memory_order_relaxed);
            while(nCurrent > 0)
            {
                if(nStatelessConnected.compare_exchange_weak(nCurrent, nCurrent - 1,
                        std::memory_order_relaxed))
                    break;
            }
        }
        else
        {
            uint32_t nCurrent = nLegacyConnected.load(std::memory_order_relaxed);
            while(nCurrent > 0)
            {
                if(nLegacyConnected.compare_exchange_weak(nCurrent, nCurrent - 1,
                        std::memory_order_relaxed))
                    break;
            }
        }
    }

    uint32_t NodePoolHealthMonitor::GetConnected(ProtocolLane lane) const
    {
        return (lane == ProtocolLane::STATELESS)
            ? nStatelessConnected.load(std::memory_order_relaxed)
            : nLegacyConnected.load(std::memory_order_relaxed);
    }

    uint32_t NodePoolHealthMonitor::GetTotalConnected() const
    {
        return nStatelessConnected.load(std::memory_order_relaxed)
             + nLegacyConnected.load(std::memory_order_relaxed);
    }

    /* ── Template throughput ────────────────────────────────────────────────── */

    void NodePoolHealthMonitor::RecordTemplateServed(ProtocolLane lane)
    {
        if(lane == ProtocolLane::STATELESS)
            nStatelessTemplatesServed.fetch_add(1, std::memory_order_relaxed);
        else
            nLegacyTemplatesServed.fetch_add(1, std::memory_order_relaxed);
    }

    void NodePoolHealthMonitor::RecordTemplateFailure(ProtocolLane lane)
    {
        if(lane == ProtocolLane::STATELESS)
            nStatelessTemplateFailures.fetch_add(1, std::memory_order_relaxed);
        else
            nLegacyTemplateFailures.fetch_add(1, std::memory_order_relaxed);
    }

    void NodePoolHealthMonitor::RecordRateLimitHit(ProtocolLane lane)
    {
        if(lane == ProtocolLane::STATELESS)
            nStatelessRateLimitHits.fetch_add(1, std::memory_order_relaxed);
        else
            nLegacyRateLimitHits.fetch_add(1, std::memory_order_relaxed);
    }

    /* ── Lane status ────────────────────────────────────────────────────────── */

    LaneStatus NodePoolHealthMonitor::GetLaneStatus(ProtocolLane lane) const
    {
        const uint32_t nConnected = GetConnected(lane);
        const uint32_t nMax = GetMaxMiners();

        /* Overloaded: >= 90% of max capacity */
        if(nConnected >= (nMax * 9 / 10))
            return LaneStatus::OVERLOADED;

        /* Degraded: >= 70% of max capacity */
        if(nConnected >= (nMax * 7 / 10))
            return LaneStatus::DEGRADED;

        return LaneStatus::ACTIVE;
    }

    /* ── Capacity enforcement ───────────────────────────────────────────────── */

    bool NodePoolHealthMonitor::IsAtCapacity() const
    {
        return GetTotalConnected() >= GetMaxMiners();
    }

    uint32_t NodePoolHealthMonitor::GetMaxMiners() const
    {
        return static_cast<uint32_t>(config::GetArg("-maxminers", 500));
    }

    /* ── Periodic logging ───────────────────────────────────────────────────── */

    void NodePoolHealthMonitor::LogHealthSummary()
    {
        const uint64_t nNow = runtime::unifiedtimestamp();

        /* Enforce 60s minimum interval between health logs */
        {
            std::lock_guard<std::mutex> lock(m_logMutex);
            if(nNow - nLastLogTimestamp < 60)
                return;
            nLastLogTimestamp = nNow;
        }

        const uint32_t nStateless = nStatelessConnected.load(std::memory_order_relaxed);
        const uint32_t nLegacy    = nLegacyConnected.load(std::memory_order_relaxed);
        const uint32_t nTotal     = nStateless + nLegacy;
        const uint32_t nMax       = GetMaxMiners();

        const LaneStatus eStatelessStatus = GetLaneStatus(ProtocolLane::STATELESS);
        const LaneStatus eLegacyStatus    = GetLaneStatus(ProtocolLane::LEGACY);

        debug::log(0, FUNCTION, "[POOL HEALTH] ",
            "total=", nTotal, "/", nMax,
            " | Stateless=", nStateless, " (", LaneStatusString(eStatelessStatus), ")",
            " | Legacy=", nLegacy, " (", LaneStatusString(eLegacyStatus), ")",
            " | templates: stateless=", nStatelessTemplatesServed.load(std::memory_order_relaxed),
            " legacy=", nLegacyTemplatesServed.load(std::memory_order_relaxed),
            " | failures: stateless=", nStatelessTemplateFailures.load(std::memory_order_relaxed),
            " legacy=", nLegacyTemplateFailures.load(std::memory_order_relaxed),
            " | rate_limits: stateless=", nStatelessRateLimitHits.load(std::memory_order_relaxed),
            " legacy=", nLegacyRateLimitHits.load(std::memory_order_relaxed));
    }

} // namespace LLP
