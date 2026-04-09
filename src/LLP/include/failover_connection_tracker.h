/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_FAILOVER_CONNECTION_TRACKER_H
#define NEXUS_LLP_INCLUDE_FAILOVER_CONNECTION_TRACKER_H

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_set>
#include <cstdint>

namespace LLP
{
    /** FailoverConnectionTracker
     *
     *  Thread-safe singleton that tracks failover connection attempts per IP address.
     *
     *  DESIGN OVERVIEW:
     *  ================
     *  When a miner switches from its primary node to a failover node, it establishes
     *  a brand-new connection with NO pre-existing Session ID.
     *
     *  This singleton observes that signal (fresh connection from known IP with no
     *  recoverable session) and records it as a potential failover event.
     *
     *  USAGE:
     *  ======
     *  1. In StatelessMinerConnection::Event(EVENTS::CONNECT):
     *     - For non-localhost IPs: call RecordConnection(strAddr)
     *
     *  2. After successful MINER_AUTH_RESPONSE:
     *     - If IsFailover(strAddr): call ChannelStateManager::NotifyFailoverConnection()
     *
     *  THREAD SAFETY:
     *  ==============
     *  - Per-IP flags stored in unordered_map, protected by mutex
     *  - Global total counter uses std::atomic for lock-free reads
     *
     **/
    class FailoverConnectionTracker
    {
    public:
        /** Get
         *
         *  Get the global singleton instance.
         *  Thread-safe initialization guaranteed by C++11.
         *
         *  @return Reference to singleton
         *
         **/
        static FailoverConnectionTracker& Get();

        /** RecordConnection
         *
         *  Record a potential failover connection from an IP address.
         *  Called when a fresh connection arrives but no session can be recovered
         *  for that IP — the canonical signal that a miner has failed over to this node.
         *
         *  @param[in] strAddr  IP address of the connecting miner
         *
         **/
        void RecordConnection(const std::string& strAddr);

        /** IsFailover
         *
         *  Check whether a connection from the given IP is classified as a failover.
         *  Returns true if RecordConnection() was previously called for this IP
         *  and the connection has not yet been cleared.
         *
         *  @param[in] strAddr  IP address to check
         *
         *  @return true if this IP has a recorded failover connection
         *
         **/
        bool IsFailover(const std::string& strAddr) const;

        /** ClearConnection
         *
         *  Clear the failover flag for an IP after successful authentication.
         *  Ensures repeat fresh authentications from the same IP are also flagged.
         *
         *  @param[in] strAddr  IP address to clear
         *
         **/
        void ClearConnection(const std::string& strAddr);

        /** GetTotalFailoverCount
         *
         *  Get the total number of failover connections recorded since process start.
         *
         *  @return Total failover connection count (monotonically increasing)
         *
         **/
        uint32_t GetTotalFailoverCount() const;

    private:
        /** Private constructor — use Get() for singleton access **/
        FailoverConnectionTracker();

        /** Non-copyable **/
        FailoverConnectionTracker(const FailoverConnectionTracker&) = delete;
        FailoverConnectionTracker& operator=(const FailoverConnectionTracker&) = delete;

        /** Mutex for thread-safe per-IP set access **/
        mutable std::mutex m_mutex;

        /** Per-IP failover flag — IPs with pending (unauthenticated) failover connections **/
        std::unordered_set<std::string> m_setPendingFailoverIPs;

        /** Total failover connections recorded since process start **/
        std::atomic<uint32_t> m_nTotalFailoverCount{0};
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_FAILOVER_CONNECTION_TRACKER_H
