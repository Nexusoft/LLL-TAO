/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/active_session_board.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /* Singleton accessor */
    ActiveSessionBoard& ActiveSessionBoard::Get()
    {
        static ActiveSessionBoard instance;
        return instance;
    }


    uint32_t ActiveSessionBoard::GetFailedPacketThreshold() const
    {
        return static_cast<uint32_t>(
            config::GetArg("-maxfailedpackets", DEFAULT_FAILED_PACKET_THRESHOLD));
    }


    uint64_t ActiveSessionBoard::GetCooldownDuration() const
    {
        return static_cast<uint64_t>(
            config::GetArg("-miningcooldownseconds", DEFAULT_COOLDOWN_DURATION_SEC));
    }


    /* ── Registration ───────────────────────────────────────────────────────── */

    uint64_t ActiveSessionBoard::Register(uint32_t nSessionId, ProtocolLane lane,
                                       uint32_t nChannel, bool fSubscribed)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_mapSessions.find(key);
        if(it != m_mapSessions.end())
        {
            /* Re-registration: update fields and reset ALL health state.
             * This is the recovery path for a miner that was in cooldown or
             * marked disconnected — re-subscribing via MINER_READY clears
             * all negative state unconditionally. */
            it->second.nChannel = nChannel;
            it->second.fSubscribedToNotifications = fSubscribed;
            it->second.nFailedPackets.store(0, std::memory_order_relaxed);
            it->second.fMarkedDisconnected.store(false, std::memory_order_relaxed);
            it->second.nCooldownExpiry.store(0, std::memory_order_relaxed);
            it->second.nLastSuccessfulSend.store(
                runtime::unifiedtimestamp(), std::memory_order_relaxed);

            /* Bump version to invalidate any in-flight MarkDisconnected calls
             * from old connections (prevents AUTH/DISCONNECT race). */
            const uint64_t nNewVersion = it->second.nVersion.fetch_add(1, std::memory_order_relaxed) + 1;

            debug::log(2, FUNCTION, "Re-registered session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " channel=", nChannel, " version=", nNewVersion);

            return nNewVersion;
        }
        else
        {
            ActiveSessionEntry entry(nSessionId, lane, nChannel, fSubscribed);
            entry.nLastSuccessfulSend.store(
                runtime::unifiedtimestamp(), std::memory_order_relaxed);
            entry.nVersion.store(1, std::memory_order_relaxed);

            m_mapSessions.emplace(key, std::move(entry));

            debug::log(2, FUNCTION, "Registered session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " channel=", nChannel,
                " total=", m_mapSessions.size());

            return 1;
        }
    }


    void ActiveSessionBoard::Unregister(uint32_t nSessionId, ProtocolLane lane)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto nErased = m_mapSessions.erase(key);

        if(nErased > 0)
        {
            debug::log(2, FUNCTION, "Unregistered session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " remaining=", m_mapSessions.size());
        }
    }


    /* ── Health tracking ────────────────────────────────────────────────────── */

    void ActiveSessionBoard::RecordSendSuccess(uint32_t nSessionId, ProtocolLane lane)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it == m_mapSessions.end())
            return;

        it->second.nFailedPackets.store(0, std::memory_order_relaxed);
        it->second.nCooldownExpiry.store(0, std::memory_order_relaxed);
        it->second.nLastSuccessfulSend.store(
            runtime::unifiedtimestamp(), std::memory_order_relaxed);
    }


    void ActiveSessionBoard::RecordSendFailure(uint32_t nSessionId, ProtocolLane lane)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it == m_mapSessions.end())
            return;

        const uint32_t nFailed = it->second.nFailedPackets.fetch_add(1, std::memory_order_relaxed) + 1;
        const uint32_t nThreshold = GetFailedPacketThreshold();

        if(nFailed >= nThreshold && it->second.nCooldownExpiry.load(std::memory_order_relaxed) == 0)
        {
            /* Enter timed cooldown instead of permanent ban.
             * The session will auto-recover when current time > nCooldownExpiry.
             * This prevents the permanent shadow-ban that occurred with the old
             * fMarkedDisconnected-only approach. */
            const uint64_t nDuration = GetCooldownDuration();
            const uint64_t nExpiry = runtime::unifiedtimestamp() + nDuration;
            it->second.nCooldownExpiry.store(nExpiry, std::memory_order_relaxed);
            it->second.fMarkedDisconnected.store(true, std::memory_order_relaxed);

            debug::log(0, FUNCTION,
                "Session entered cooldown: session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " failures=", nFailed, "/", nThreshold,
                " cooldown=", nDuration, "s",
                " — PUSH paused (will auto-recover)");
        }
    }


    void ActiveSessionBoard::MarkDisconnected(uint32_t nSessionId, ProtocolLane lane,
                                               uint64_t nExpectedVersion)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it == m_mapSessions.end())
            return;

        /* Version check: if the session has been re-registered since the caller
         * captured the version, reject this stale disconnect.  This prevents
         * the AUTH/DISCONNECT race where an old connection's DISCONNECT handler
         * kills a freshly re-registered session. */
        if(nExpectedVersion != 0)
        {
            const uint64_t nCurrentVersion = it->second.nVersion.load(std::memory_order_relaxed);
            if(nCurrentVersion != nExpectedVersion)
            {
                debug::log(0, FUNCTION,
                    "Rejected stale MarkDisconnected: session=", nSessionId,
                    " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                    " expected_version=", nExpectedVersion,
                    " current_version=", nCurrentVersion,
                    " — session was re-registered; disconnect is stale");
                return;
            }
        }

        it->second.fMarkedDisconnected.store(true, std::memory_order_relaxed);
    }


    /* ── Queries ────────────────────────────────────────────────────────────── */

    std::vector<uint32_t> ActiveSessionBoard::GetActiveForChannel(
        uint32_t nChannel, ProtocolLane lane)
    {
        std::vector<uint32_t> vResult;
        const uint64_t nNow = runtime::unifiedtimestamp();

        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto& [key, entry] : m_mapSessions)
        {
            if(key.nLane != lane)
                continue;
            if(entry.nChannel != nChannel)
                continue;
            if(!entry.fSubscribedToNotifications)
                continue;

            /* Check cooldown: auto-recover if cooldown has expired */
            const uint64_t nExpiry = entry.nCooldownExpiry.load(std::memory_order_relaxed);
            if(nExpiry != 0 && nNow >= nExpiry)
            {
                /* Cooldown expired — auto-recover the session */
                entry.nCooldownExpiry.store(0, std::memory_order_relaxed);
                entry.fMarkedDisconnected.store(false, std::memory_order_relaxed);
                entry.nFailedPackets.store(0, std::memory_order_relaxed);

                debug::log(0, FUNCTION,
                    "Session auto-recovered from cooldown: session=", key.nSessionId,
                    " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                    " channel=", nChannel);
            }

            /* Skip sessions that are still in cooldown or permanently disconnected */
            if(entry.fMarkedDisconnected.load(std::memory_order_relaxed))
                continue;

            vResult.push_back(key.nSessionId);
        }

        return vResult;
    }


    uint32_t ActiveSessionBoard::GetTotalActive() const
    {
        uint32_t nActive = 0;

        std::lock_guard<std::mutex> lock(m_mutex);
        for(const auto& [key, entry] : m_mapSessions)
        {
            if(!entry.fMarkedDisconnected.load(std::memory_order_relaxed))
                ++nActive;
        }

        return nActive;
    }


    uint32_t ActiveSessionBoard::GetTotalRegistered() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_mapSessions.size());
    }


    bool ActiveSessionBoard::IsActive(uint32_t nSessionId, ProtocolLane lane) const
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it == m_mapSessions.end())
            return false;

        return !it->second.fMarkedDisconnected.load(std::memory_order_relaxed);
    }


    uint64_t ActiveSessionBoard::GetVersion(uint32_t nSessionId, ProtocolLane lane) const
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it == m_mapSessions.end())
            return 0;

        return it->second.nVersion.load(std::memory_order_relaxed);
    }


    /* ── Maintenance ────────────────────────────────────────────────────────── */

    uint32_t ActiveSessionBoard::SweepStaleEntries(uint64_t nStaleTimeout)
    {
        const uint64_t nNow = runtime::unifiedtimestamp();
        uint32_t nRemoved = 0;

        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto it = m_mapSessions.begin(); it != m_mapSessions.end(); )
        {
            const auto& entry = it->second;

            /* Only sweep entries that are disconnected (not just in cooldown) */
            if(entry.fMarkedDisconnected.load(std::memory_order_relaxed))
            {
                const uint64_t nLastSend = entry.nLastSuccessfulSend.load(std::memory_order_relaxed);

                /* Remove if disconnected for longer than stale timeout.
                 * Guard against clock skew: nNow must exceed nLastSend. */
                if(nLastSend > 0 && nNow > nLastSend && (nNow - nLastSend) > nStaleTimeout)
                {
                    debug::log(2, FUNCTION, "Swept stale session=", entry.nSessionId,
                        " lane=", (entry.nLane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                        " idle=", (nNow - nLastSend), "s");

                    it = m_mapSessions.erase(it);
                    ++nRemoved;
                    continue;
                }
            }

            ++it;
        }

        if(nRemoved > 0)
        {
            debug::log(0, FUNCTION, "Swept ", nRemoved, " stale entries",
                " remaining=", m_mapSessions.size());
        }

        return nRemoved;
    }

} // namespace LLP
