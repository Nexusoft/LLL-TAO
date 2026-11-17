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
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

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
     *  prior session) and records it as a potential failover event.
     *
     *  USAGE:
     *  ======
     *  1. In StatelessMinerConnection::Event(EVENTS::CONNECT):
     *     - Check StatelessMinerManager::GetMinerContextByIP()
     *     - On miss for non-localhost IPs: call RecordConnection(strAddr)
     *
     *  2. After successful MINER_AUTH_RESPONSE:
     *     - If this connection still holds a slot and IsFailover(strAddr):
     *       call ChannelStateManager::NotifyFailoverConnection()
     *     - Release this connection's slot (sibling connections keep theirs)
     *
     *  3. On DISCONNECT or destruction: release this connection's slot.
     *     Unreleased slots expire after PENDING_TTL and the map is capped.
     *
     *  THREAD SAFETY:
     *  ==============
     *  - Per-IP slots stored in unordered_map, protected by mutex
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

        using Clock = std::chrono::steady_clock;

        /* Missed disconnects expire. Live handshakes are also capped so a
         * pre-auth flood cannot grow this set for the process lifetime. */
        static constexpr auto PENDING_TTL = std::chrono::minutes(10);
        static constexpr std::size_t MAX_PENDING_IPS = 4096;

        /** RecordConnection
         *
         *  Record one potential failover connection from an IP address.
         *  Called when a fresh connection arrives but no session can be recovered
         *  for that IP — the canonical signal that a miner has failed over to this node.
         *
         *  Concurrent connections from the same IP each take a slot. The returned
         *  generation must be passed to ReleaseConnection so a later reconnect
         *  cannot be cleared by a stale disconnect.
         *
         *  @param[in]  strAddr       IP address of the connecting miner
         *  @param[out] nGeneration   Slot generation, valid only if this returns true
         *  @param[in]  now           Observation time (injectable for tests)
         *
         *  @return true if a pending slot was taken
         *
         **/
        bool RecordConnection(const std::string& strAddr, uint64_t& nGeneration,
                              Clock::time_point now = Clock::now());

        /** IsFailover
         *
         *  Check whether a connection from the given IP is classified as a failover.
         *  Returns true if a non-expired RecordConnection() slot is still held.
         *
         *  @param[in] strAddr  IP address to check
         *  @param[in] now      Observation time (injectable for tests)
         *
         *  @return true if this IP has a recorded failover connection
         *
         **/
        bool IsFailover(const std::string& strAddr, Clock::time_point now = Clock::now()) const;

        /** ReleaseConnection
         *
         *  Release one pending slot after authentication or disconnect.
         *  A mismatched generation is ignored so concurrent connections and
         *  expired-then-reused IPs are not cleared by a stale caller.
         *
         *  @param[in] strAddr      IP address to release
         *  @param[in] nGeneration  Generation returned by RecordConnection
         *
         **/
        void ReleaseConnection(const std::string& strAddr, uint64_t nGeneration);

        /** ClearConnection
         *
         *  Drop every pending slot for an IP. Prefer ReleaseConnection when the
         *  caller owns a single connection slot.
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

        struct Pending
        {
            uint32_t nConnections{0};
            uint64_t nGeneration{0};
            Clock::time_point tExpires{};
        };

        void PruneExpired(Clock::time_point now);

        /** Mutex for thread-safe per-IP map access **/
        mutable std::mutex m_mutex;

        /** Per-IP pending unauthenticated failover connections. **/
        std::unordered_map<std::string, Pending> m_mapPendingFailoverIPs;

        /** Next generation assigned to a newly inserted IP. **/
        uint64_t m_nNextGeneration{1};

        /** Total failover connections recorded since process start **/
        std::atomic<uint32_t> m_nTotalFailoverCount{0};
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_FAILOVER_CONNECTION_TRACKER_H
