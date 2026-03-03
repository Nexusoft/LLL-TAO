/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/failover_connection_tracker.h>
#include <Util/include/debug.h>

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
    void FailoverConnectionTracker::RecordConnection(const std::string& strAddr)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_setPendingFailoverIPs.insert(strAddr);
        }

        /* Increment global counter atomically; capture new total for logging */
        uint32_t nNewTotal = m_nTotalFailoverCount.fetch_add(1, std::memory_order_relaxed) + 1;

        debug::log(0, FUNCTION, "Potential failover connection recorded: addr=", strAddr,
                   " total=", nNewTotal);
    }


    /* IsFailover */
    bool FailoverConnectionTracker::IsFailover(const std::string& strAddr) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (m_setPendingFailoverIPs.count(strAddr) > 0);
    }


    /* ClearConnection */
    void FailoverConnectionTracker::ClearConnection(const std::string& strAddr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_setPendingFailoverIPs.erase(strAddr);
    }


    /* GetTotalFailoverCount */
    uint32_t FailoverConnectionTracker::GetTotalFailoverCount() const
    {
        return m_nTotalFailoverCount.load(std::memory_order_relaxed);
    }

} // namespace LLP
