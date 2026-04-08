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
 *   - After configurable threshold failures, session enters AUTO-RECOVERING
 *     cooldown (default 60s) — NOT a permanent ban
 *   - Cooldown auto-expires: GetActiveForChannel() checks cooldown timestamp
 *     and auto-recovers sessions whose cooldown has elapsed
 *   - Registration version counter prevents AUTH/DISCONNECT race conditions:
 *     MarkDisconnected() is a no-op if the session has been re-registered
 *     since the disconnect was initiated
 *   - SweepStaleEntries() removes orphaned entries during periodic cleanup
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
        std::atomic<bool> fMarkedDisconnected{false};      /* soft-disconnect flag */

        /** Cooldown expiry timestamp (epoch seconds).
         *  When non-zero, this session is in cooldown and excluded from push
         *  notifications.  Auto-recovers: GetActiveForChannel() clears the flag
         *  when current time > nCooldownExpiry.  Replaces permanent banning. */
        std::atomic<uint64_t> nCooldownExpiry{0};

        /** Monotonically increasing version counter.
         *  Incremented on every Register() call.  MarkDisconnected() compares
         *  the caller's version against the current version — if they differ,
         *  the session has been re-registered since the disconnect was initiated
         *  and the stale MarkDisconnected is silently rejected.
         *  This prevents the AUTH/DISCONNECT race (BUG 2). */
        std::atomic<uint64_t> nVersion{0};

        ActiveSessionEntry() = default;
        ActiveSessionEntry(uint32_t sid, ProtocolLane lane, uint32_t ch, bool sub)
        : nSessionId(sid)
        , nLane(lane)
        , nChannel(ch)
        , fSubscribedToNotifications(sub)
        , nFailedPackets(0)
        , nLastSuccessfulSend(0)
        , fMarkedDisconnected(false)
        , nCooldownExpiry(0)
        , nVersion(0)
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
        , nCooldownExpiry(other.nCooldownExpiry.load(std::memory_order_relaxed))
        , nVersion(other.nVersion.load(std::memory_order_relaxed))
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

        /** Consecutive failure threshold before entering cooldown.
         *  Configurable via -maxfailedpackets (default 5). **/
        static constexpr uint32_t DEFAULT_FAILED_PACKET_THRESHOLD = 5;

        /** Default cooldown duration in seconds after exceeding failure threshold.
         *  After this period, the session auto-recovers and can receive pushes again.
         *  Configurable via -miningcooldownseconds (default 60).
         *
         *  This replaces the previous permanent shadow-ban behavior where a session
         *  marked disconnected by RecordSendFailure() would NEVER recover until
         *  node restart. */
        static constexpr uint64_t DEFAULT_COOLDOWN_DURATION_SEC = 60;

        /* ── Registration ───────────────────────────────────────────────────── */

        /** Register an active session.  Called on MINER_READY.
         *  If session+lane already exists, updates fields and resets health counters.
         *  Returns the new registration version (used by MarkDisconnected callers
         *  to prevent stale disconnects from killing re-registered sessions). **/
        uint64_t Register(uint32_t nSessionId, ProtocolLane lane, uint32_t nChannel,
                      bool fSubscribed);

        /** Unregister a session.  Called on DISCONNECT event. **/
        void Unregister(uint32_t nSessionId, ProtocolLane lane);

        /* ── Health tracking ────────────────────────────────────────────────── */

        /** Record a successful send to a session.
         *  Resets failure counter AND clears any active cooldown. **/
        void RecordSendSuccess(uint32_t nSessionId, ProtocolLane lane);

        /** Record a failed send to a session.
         *  If failures >= threshold, session enters timed cooldown (auto-recovers).
         *  Does NOT permanently ban — cooldown expires after
         *  DEFAULT_COOLDOWN_DURATION_SEC seconds. **/
        void RecordSendFailure(uint32_t nSessionId, ProtocolLane lane);

        /** Mark a session as disconnected (version-checked).
         *  @param nExpectedVersion  The registration version at the time the
         *         disconnect was initiated.  If the session has been re-registered
         *         since (version mismatch), the mark is silently rejected.
         *         Pass 0 to bypass version checking (unconditional mark). **/
        void MarkDisconnected(uint32_t nSessionId, ProtocolLane lane,
                              uint64_t nExpectedVersion = 0);

        /* ── Queries ────────────────────────────────────────────────────────── */

        /** Get session IDs for a specific channel on a specific lane that are
         *  active (not disconnected, not in cooldown) and subscribed.
         *  Auto-recovers sessions whose cooldown has expired. **/
        std::vector<uint32_t> GetActiveForChannel(uint32_t nChannel, ProtocolLane lane);

        /** Get total number of active (not disconnected, not in cooldown) sessions. **/
        uint32_t GetTotalActive() const;

        /** Get total number of registered sessions (including disconnected/cooled). **/
        uint32_t GetTotalRegistered() const;

        /** Check if a session+lane is registered and not disconnected/cooled. **/
        bool IsActive(uint32_t nSessionId, ProtocolLane lane) const;

        /** Get the current registration version for a session.
         *  Returns 0 if session is not registered. **/
        uint64_t GetVersion(uint32_t nSessionId, ProtocolLane lane) const;

        /* ── Maintenance ────────────────────────────────────────────────────── */

        /** Sweep stale entries that have been disconnected for longer than
         *  nStaleTimeout seconds.  Called from Meter() periodic cleanup.
         *  Returns number of entries removed. **/
        uint32_t SweepStaleEntries(uint64_t nStaleTimeout);


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
        std::unordered_map<SessionKey, ActiveSessionEntry, SessionKeyHash> m_mapSessions;

        /** Get failure threshold from config. **/
        uint32_t GetFailedPacketThreshold() const;

        /** Get cooldown duration from config. **/
        uint64_t GetCooldownDuration() const;
    };

} // namespace LLP

#endif
