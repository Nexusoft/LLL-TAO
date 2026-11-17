/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/failover_connection_tracker.h>
#include <Util/include/debug.h>

#include <limits>

namespace LLP
{
    /* Constructor */
    FailoverConnectionTracker::FailoverConnectionTracker()
        : m_nTotalFailoverCount(0)
    {
    }


    /* Get singleton instance */
    FailoverConnectionTracker& FailoverConnectionTracker::Get()
    {
        static FailoverConnectionTracker instance;
        return instance;
    }


    /* RecordConnection */
    bool FailoverConnectionTracker::RecordConnection(const std::string& strAddr,
                                                     uint64_t& nGeneration,
                                                     Clock::time_point now)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            PruneExpired(now);

            auto it = m_mapPendingFailoverIPs.find(strAddr);
            if(it == m_mapPendingFailoverIPs.end())
            {
                if(m_mapPendingFailoverIPs.size() >= MAX_PENDING_IPS)
                    return false;

                Pending pending;
                pending.nConnections = 1;
                pending.nGeneration = m_nNextGeneration++;
                pending.tExpires = now + PENDING_TTL;
                nGeneration = pending.nGeneration;
                m_mapPendingFailoverIPs.emplace(strAddr, pending);
            }
            else
            {
                if(it->second.nConnections == std::numeric_limits<uint32_t>::max())
                    return false;

                ++it->second.nConnections;
                it->second.tExpires = now + PENDING_TTL;
                nGeneration = it->second.nGeneration;
            }
        }

        /* Increment global counter atomically; capture new total for logging */
        uint32_t nNewTotal = m_nTotalFailoverCount.fetch_add(1, std::memory_order_relaxed) + 1;

        debug::log(0, FUNCTION, "Potential failover connection recorded: addr=", strAddr,
                   " total=", nNewTotal);
        return true;
    }


    /* IsFailover */
    bool FailoverConnectionTracker::IsFailover(const std::string& strAddr,
                                               Clock::time_point now) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_mapPendingFailoverIPs.find(strAddr);
        if(it == m_mapPendingFailoverIPs.end())
            return false;
        if(it->second.nConnections == 0 || now >= it->second.tExpires)
            return false;
        return true;
    }


    /* ReleaseConnection */
    void FailoverConnectionTracker::ReleaseConnection(const std::string& strAddr,
                                                      uint64_t nGeneration)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_mapPendingFailoverIPs.find(strAddr);
        if(it == m_mapPendingFailoverIPs.end() || it->second.nGeneration != nGeneration)
            return;

        if(it->second.nConnections <= 1)
            m_mapPendingFailoverIPs.erase(it);
        else
            --it->second.nConnections;
    }


    /* ClearConnection */
    void FailoverConnectionTracker::ClearConnection(const std::string& strAddr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mapPendingFailoverIPs.erase(strAddr);
    }


    void FailoverConnectionTracker::PruneExpired(Clock::time_point now)
    {
        for(auto it = m_mapPendingFailoverIPs.begin(); it != m_mapPendingFailoverIPs.end();)
        {
            if(now >= it->second.tExpires)
                it = m_mapPendingFailoverIPs.erase(it);
            else
                ++it;
        }
    }


    /* GetTotalFailoverCount */
    uint32_t FailoverConnectionTracker::GetTotalFailoverCount() const
    {
        return m_nTotalFailoverCount.load(std::memory_order_relaxed);
    }

} // namespace LLP
