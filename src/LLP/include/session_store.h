/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_STORE_H
#define NEXUS_LLP_INCLUDE_SESSION_STORE_H

#include <LLP/include/canonical_session.h>
#include <LLP/include/node_session_registry.h>   /* Uint256Hash */
#include <Util/templates/concurrent_hashmap.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>


namespace LLP
{

    /** SessionStore
     *
     *  Unified session store — single ConcurrentHashMap<uint256_t, CanonicalSession>
     *  with thin secondary indexes for O(1) lookup by sessionId, address, genesis, IP.
     *
     *  Replaces:
     *    - StatelessMinerManager  (session data portion: mapMiners + 5 reverse indexes)
     *    - NodeSessionRegistry    (m_mapByKey + m_mapSessionToKey)
     *
     *  THREAD SAFETY:
     *  ==============
     *  All maps use ConcurrentHashMap (shared_mutex, lock-free reads).
     *  All mutations go through Register/Remove/Transform which update indexes atomically.
     *
     *  ARCHITECTURE:
     *  =============
     *  Primary map:   hashKeyID → CanonicalSession    (single source of truth)
     *  Index maps:    sessionId → hashKeyID           (thin reverse pointers)
     *                 address   → hashKeyID
     *                 genesis   → hashKeyID
     *                 IP        → hashKeyID
     *
     **/
    class SessionStore
    {
    public:

        /** Get singleton instance. **/
        static SessionStore& Get();


        /* ══════════════════════════════════════════════════════════════════════
         *  Registration
         * ══════════════════════════════════════════════════════════════════════ */

        /** Register
         *
         *  Insert or upsert a canonical session.  Updates primary map and all
         *  secondary indexes atomically.
         *
         *  @param[in] session  The canonical session to register.
         *
         *  @return true if a new entry was created, false if an existing entry was updated.
         *
         **/
        bool Register(const CanonicalSession& session);

        /** Remove
         *
         *  Remove a session by primary key.  Cleans primary map + all indexes.
         *
         *  @param[in] hashKeyID  Falcon key hash identifying the session.
         *
         *  @return true if an entry was found and removed.
         *
         **/
        bool Remove(const uint256_t& hashKeyID);


        /* ══════════════════════════════════════════════════════════════════════
         *  Lookups
         * ══════════════════════════════════════════════════════════════════════ */

        /** LookupByKey — direct primary lookup, O(1). **/
        std::optional<CanonicalSession> LookupByKey(const uint256_t& hashKeyID) const;

        /** LookupBySessionId — index hop: sessionId → keyID → session. **/
        std::optional<CanonicalSession> LookupBySessionId(uint32_t nSessionId) const;

        /** LookupByAddress — index hop: address → keyID → session. **/
        std::optional<CanonicalSession> LookupByAddress(const std::string& strAddress) const;

        /** LookupByGenesis — index hop: genesis → keyID → session. **/
        std::optional<CanonicalSession> LookupByGenesis(const uint256_t& hashGenesis) const;

        /** LookupByIP — index hop: IP → keyID → session. **/
        std::optional<CanonicalSession> LookupByIP(const std::string& strIP) const;

        /** Contains — check if a session exists by primary key. **/
        bool Contains(const uint256_t& hashKeyID) const;


        /* ══════════════════════════════════════════════════════════════════════
         *  Atomic Mutations
         * ══════════════════════════════════════════════════════════════════════ */

        /** Transform
         *
         *  Atomic read-modify-write on a session identified by hashKeyID.
         *  The transformer receives a const reference and returns a new copy.
         *  If the address or genesis changed, indexes are updated.
         *
         *  @param[in] hashKeyID     Primary key.
         *  @param[in] transformer   fn(const CanonicalSession&) → CanonicalSession
         *
         *  @return true if the entry was found and transformed.
         *
         **/
        bool Transform(const uint256_t& hashKeyID,
                       std::function<CanonicalSession(const CanonicalSession&)> transformer);

        /** TransformBySessionId
         *
         *  Same as Transform but lookup by session ID first.
         *
         **/
        bool TransformBySessionId(uint32_t nSessionId,
                                  std::function<CanonicalSession(const CanonicalSession&)> transformer);

        /** TransformByAddress
         *
         *  Same as Transform but lookup by address first.
         *
         **/
        bool TransformByAddress(const std::string& strAddress,
                                std::function<CanonicalSession(const CanonicalSession&)> transformer);


        /* ══════════════════════════════════════════════════════════════════════
         *  Iteration & Aggregation
         * ══════════════════════════════════════════════════════════════════════ */

        /** ForEach — iterate all sessions (read-only snapshot of each). **/
        void ForEach(std::function<void(const uint256_t&, const CanonicalSession&)> fn) const;

        /** GetAll — return a snapshot vector of all sessions. **/
        std::vector<CanonicalSession> GetAll() const;

        /** GetAllForChannel — return sessions subscribed to a specific channel. **/
        std::vector<CanonicalSession> GetAllForChannel(uint32_t nChannel) const;

        /** Count — total number of sessions. **/
        size_t Count() const;

        /** CountLive — sessions with at least one port live. **/
        size_t CountLive() const;

        /** CountAuthenticated — sessions with fAuthenticated == true. **/
        size_t CountAuthenticated() const;


        /* ══════════════════════════════════════════════════════════════════════
         *  Cleanup & Maintenance
         * ══════════════════════════════════════════════════════════════════════ */

        /** SweepExpired
         *
         *  Remove sessions that are expired: no ports live, not recoverable,
         *  and nLastActivity + nTimeoutSec < nNow.
         *
         *  @param[in] nTimeoutSec  Liveness timeout (default: SESSION_LIVENESS_TIMEOUT_SEC).
         *  @param[in] nNow         Current time (default: runtime::unifiedtimestamp()).
         *
         *  @return Number of sessions swept.
         *
         **/
        uint32_t SweepExpired(uint64_t nTimeoutSec, uint64_t nNow = 0);

        /** Clear — remove all sessions (for testing). **/
        void Clear();


        /* ══════════════════════════════════════════════════════════════════════
         *  Health Tracking (replaces ActiveSessionBoard)
         * ══════════════════════════════════════════════════════════════════════ */

        /** Default cooldown duration in seconds after exceeding failure threshold.
         *  After this period, the session auto-recovers and can receive pushes again.
         *  Configurable via -miningcooldownseconds (default 60).
         *
         *  This replaces the previous permanent shadow-ban behavior where a session
         *  exceeding the failure threshold was permanently excluded from pushes. */
        static constexpr uint64_t DEFAULT_COOLDOWN_DURATION_SEC = 60;

        /** RecordSendSuccess — reset failure counter, clear cooldown. **/
        bool RecordSendSuccess(const uint256_t& hashKeyID, uint64_t nNow = 0);

        /** RecordSendFailure — increment failure counter; enter timed cooldown
         *  (NOT permanent ban) at threshold. Auto-recovers after cooldown expires. **/
        bool RecordSendFailure(const uint256_t& hashKeyID, uint32_t nThreshold = 5);

        /** MarkDisconnected — version-checked disconnect.
         *  @param nExpectedVersion  The registration version at the time the
         *         disconnect was initiated.  If the session has been re-registered
         *         since (version mismatch), the mark is silently rejected.
         *         Pass 0 to bypass version checking (unconditional mark). **/
        bool MarkDisconnected(const uint256_t& hashKeyID, uint64_t nExpectedVersion = 0);

        /** GetVersion — get the current registration version for a session.
         *  Returns 0 if session is not registered. **/
        uint64_t GetVersion(const uint256_t& hashKeyID) const;

        /** IsActive — check if session is registered, not disconnected, and not in cooldown. **/
        bool IsActive(const uint256_t& hashKeyID) const;

        /** IsActiveBySessionId — same as IsActive but by session ID. **/
        bool IsActiveBySessionId(uint32_t nSessionId) const;

        /** GetActiveForChannel
         *
         *  Get session IDs for a specific channel on a specific lane that are
         *  active (not disconnected, not in cooldown) and subscribed.
         *  Sessions with expired cooldowns are treated as active.
         *
         **/
        std::vector<uint32_t> GetActiveSessionIdsForChannel(uint32_t nChannel, ProtocolLane lane) const;

        /** SweepCooldowns
         *
         *  Clear expired cooldowns and reset fMarkedDisconnected on sessions
         *  whose cooldown has elapsed.  Called from Meter() periodic cleanup.
         *
         *  @return Number of sessions recovered from cooldown.
         *
         **/
        uint32_t SweepCooldowns();


    private:

        /** Private constructor for singleton. **/
        SessionStore() = default;

        /** UpdateIndexes — add/update secondary indexes for a session. **/
        void UpdateIndexes(const CanonicalSession& session);

        /** RemoveIndexes — remove secondary indexes for a session. **/
        void RemoveIndexes(const CanonicalSession& session);


        /* ── Primary store ──────────────────────────────────────────────────── */

        /** Single source of truth: hashKeyID → CanonicalSession. **/
        util::ConcurrentHashMap<uint256_t, CanonicalSession, Uint256Hash> mapSessions;

        /* ── Secondary indexes (thin reverse pointers) ──────────────────────── */

        /** sessionId → hashKeyID **/
        util::ConcurrentHashMap<uint32_t, uint256_t> mapSessionIdToKey;

        /** address (IP:port) → hashKeyID **/
        util::ConcurrentHashMap<std::string, uint256_t> mapAddressToKey;

        /** genesis hash → hashKeyID **/
        util::ConcurrentHashMap<uint256_t, uint256_t, Uint256Hash> mapGenesisToKey;

        /** IP (port-agnostic) → hashKeyID **/
        util::ConcurrentHashMap<std::string, uint256_t> mapIPToKey;
    };

} // namespace LLP

#endif
