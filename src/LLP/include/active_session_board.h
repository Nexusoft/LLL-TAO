/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_ACTIVE_SESSION_BOARD_H
#define NEXUS_LLP_INCLUDE_ACTIVE_SESSION_BOARD_H

/* Active Session Board
 *
 * Tracks connection health for mining sessions to provide reliable
 * connection filtering for NotifyChannelMiners() and enforce capacity limits.
 *
 * Design:
 *   - Each active mining session is registered on MINER_READY
 *   - Weak references avoid preventing connection cleanup
 *   - Failed packet delivery increments a failure counter
 *   - After configurable threshold failures, session enters cooldown
 *   - Cooldown auto-expires after configurable duration (default 300s)
 *   - On expiry, failure counter resets and push resumes automatically
 *   - Re-authentication also clears cooldown immediately
 *   - Board doubles as MAX_MINERS enforcement point
 */

#include <LLP/include/push_notification.h>  /* ProtocolLane */

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace LLP
{
    /** ActiveSessionEntry
     *
     *  Per-session health tracking entry.
     *  Stored in the ActiveSessionBoard keyed by (nSessionId, nLane).
     *
     **/
    struct ActiveSessionEntry
    {
        uint32_t nSessionId = 0;
        ProtocolLane nLane = ProtocolLane::STATELESS;
        uint32_t nChannel = 0;                             /* 1=Prime, 2=Hash */
        bool fSubscribedToNotifications = false;

        std::atomic<uint32_t> nFailedPackets{0};           /* consecutive failed sends */
        std::atomic<uint64_t> nLastSuccessfulSend{0};      /* timestamp of last success */
        std::atomic<bool> fMarkedDisconnected{false};      /* soft-disconnect flag (cooldown) */
        std::atomic<uint64_t> nCooldownStartTime{0};       /* when cooldown began (0 = not set) */

        ActiveSessionEntry() = default;
        ActiveSessionEntry(uint32_t sid, ProtocolLane lane, uint32_t ch, bool sub)
        : nSessionId(sid)
        , nLane(lane)
        , nChannel(ch)
        , fSubscribedToNotifications(sub)
        , nFailedPackets(0)
        , nLastSuccessfulSend(0)
        , fMarkedDisconnected(false)
        , nCooldownStartTime(0)
        {
        }

        /* Move constructor for atomics */
        ActiveSessionEntry(ActiveSessionEntry&& other) noexcept
        : nSessionId(other.nSessionId)
        , nLane(other.nLane)
        , nChannel(other.nChannel)
        , fSubscribedToNotifications(other.fSubscribedToNotifications)
        , nFailedPackets(other.nFailedPackets.load(std::memory_order_relaxed))
        , nLastSuccessfulSend(other.nLastSuccessfulSend.load(std::memory_order_relaxed))
        , fMarkedDisconnected(other.fMarkedDisconnected.load(std::memory_order_relaxed))
        , nCooldownStartTime(other.nCooldownStartTime.load(std::memory_order_relaxed))
        {
        }

        /* No copy (has atomics) */
        ActiveSessionEntry(const ActiveSessionEntry&) = delete;
        ActiveSessionEntry& operator=(const ActiveSessionEntry&) = delete;
    };


    /** ActiveSessionBoard
     *
     *  Singleton session registry for connection health tracking.
     *  Thread-safe: all mutations are protected by m_mutex.
     *
     **/
    class ActiveSessionBoard
    {
    public:
        /** Get singleton instance. **/
        static ActiveSessionBoard& Get();

        /** Consecutive failure threshold before marking session disconnected.
         *  Configurable via -maxfailedpackets (default 5). **/
        static constexpr uint32_t DEFAULT_FAILED_PACKET_THRESHOLD = 5;

        /** Default cooldown duration before auto-recovery (seconds).
         *  After this many seconds the session automatically re-enables push.
         *  Configurable via -pushcooldownsec (default 300 = 5 minutes). **/
        static constexpr uint32_t DEFAULT_PUSH_COOLDOWN_SEC = 300;

        /* ── Registration ───────────────────────────────────────────────────── */

        /** Register an active session.  Called on MINER_READY.
         *  If session+lane already exists, updates fields and resets health counters. **/
        void Register(uint32_t nSessionId, ProtocolLane lane, uint32_t nChannel,
                      bool fSubscribed);

        /** Unregister a session.  Called on DISCONNECT event. **/
        void Unregister(uint32_t nSessionId, ProtocolLane lane);

        /* ── Health tracking ────────────────────────────────────────────────── */

        /** Record a successful send to a session. **/
        void RecordSendSuccess(uint32_t nSessionId, ProtocolLane lane);

        /** Record a failed send to a session.
         *  If failures >= threshold, session is marked disconnected. **/
        void RecordSendFailure(uint32_t nSessionId, ProtocolLane lane);

        /** Manually mark a session as disconnected. **/
        void MarkDisconnected(uint32_t nSessionId, ProtocolLane lane);

        /* ── Queries ────────────────────────────────────────────────────────── */

        /** Get session IDs for a specific channel on a specific lane that are
         *  active (not marked disconnected) and subscribed to notifications. **/
        std::vector<uint32_t> GetActiveForChannel(uint32_t nChannel, ProtocolLane lane) const;

        /** Get total number of active (not marked disconnected) sessions. **/
        uint32_t GetTotalActive() const;

        /** Get total number of registered sessions (including marked disconnected). **/
        uint32_t GetTotalRegistered() const;

        /** Check if a session+lane is registered and not marked disconnected. **/
        bool IsActive(uint32_t nSessionId, ProtocolLane lane) const;


    private:
        ActiveSessionBoard() = default;

        /** Compound key: (sessionId, lane) **/
        struct SessionKey
        {
            uint32_t nSessionId;
            ProtocolLane nLane;

            bool operator==(const SessionKey& o) const
            {
                return nSessionId == o.nSessionId && nLane == o.nLane;
            }
        };

        struct SessionKeyHash
        {
            std::size_t operator()(const SessionKey& k) const
            {
                /* Combine session ID with lane for distribution */
                return std::hash<uint32_t>()(k.nSessionId) ^
                       (std::hash<uint8_t>()(static_cast<uint8_t>(k.nLane)) << 16);
            }
        };

        mutable std::mutex m_mutex;
        mutable std::unordered_map<SessionKey, ActiveSessionEntry, SessionKeyHash> m_mapSessions;

        /** Get failure threshold from config. **/
        uint32_t GetFailedPacketThreshold() const;

        /** Get push cooldown duration from config (seconds). **/
        uint32_t GetPushCooldownSec() const;

        /** Check if a cooldown-marked entry has expired; if so, clear it.
         *  Caller must hold m_mutex.  Returns true if entry is still in cooldown. **/
        bool CheckAndExpireCooldown(ActiveSessionEntry& entry) const;
    };

} // namespace LLP

#endif
