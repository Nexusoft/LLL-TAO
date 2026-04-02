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


    /* ── Registration ───────────────────────────────────────────────────────── */

    void ActiveSessionBoard::Register(uint32_t nSessionId, ProtocolLane lane,
                                       uint32_t nChannel, bool fSubscribed)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_mapSessions.find(key);
        if(it != m_mapSessions.end())
        {
            /* Re-registration: update fields and reset health counters */
            it->second.nChannel = nChannel;
            it->second.fSubscribedToNotifications = fSubscribed;
            it->second.nFailedPackets.store(0, std::memory_order_relaxed);
            it->second.fMarkedDisconnected.store(false, std::memory_order_relaxed);
            it->second.nLastSuccessfulSend.store(
                runtime::unifiedtimestamp(), std::memory_order_relaxed);

            debug::log(2, FUNCTION, "Re-registered session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " channel=", nChannel);
        }
        else
        {
            ActiveSessionEntry entry(nSessionId, lane, nChannel, fSubscribed);
            entry.nLastSuccessfulSend.store(
                runtime::unifiedtimestamp(), std::memory_order_relaxed);

            m_mapSessions.emplace(key, std::move(entry));

            debug::log(2, FUNCTION, "Registered session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " channel=", nChannel,
                " total=", m_mapSessions.size());
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

        if(nFailed >= nThreshold && !it->second.fMarkedDisconnected.load(std::memory_order_relaxed))
        {
            it->second.fMarkedDisconnected.store(true, std::memory_order_relaxed);

            debug::log(0, FUNCTION,
                "Session marked disconnected: session=", nSessionId,
                " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                " failures=", nFailed, "/", nThreshold,
                " — stopped receiving PUSH until re-authentication");
        }
    }


    void ActiveSessionBoard::MarkDisconnected(uint32_t nSessionId, ProtocolLane lane)
    {
        const SessionKey key{nSessionId, lane};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(key);
        if(it != m_mapSessions.end())
            it->second.fMarkedDisconnected.store(true, std::memory_order_relaxed);
    }


    /* ── Queries ────────────────────────────────────────────────────────────── */

    std::vector<uint32_t> ActiveSessionBoard::GetActiveForChannel(
        uint32_t nChannel, ProtocolLane lane) const
    {
        std::vector<uint32_t> vResult;

        std::lock_guard<std::mutex> lock(m_mutex);
        for(const auto& [key, entry] : m_mapSessions)
        {
            if(key.nLane != lane)
                continue;
            if(entry.nChannel != nChannel)
                continue;
            if(!entry.fSubscribedToNotifications)
                continue;
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

} // namespace LLP
