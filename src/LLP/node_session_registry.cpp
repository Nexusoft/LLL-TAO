/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_session_registry.h>
#include <LLP/include/session_store.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <algorithm>

namespace LLP
{
    /* ═══════════════════════════════════════════════════════════════════════════════════════
     * NodeSessionEntry Implementation
     * ═══════════════════════════════════════════════════════════════════════════════════════ */

    /** Default Constructor **/
    NodeSessionEntry::NodeSessionEntry()
        : nSessionId(0)
        , hashKeyID(0)
        , hashGenesis(0)
        , fStatelessLive(false)
        , fLegacyLive(false)
        , nLastActivity(0)
        , context()
    {
    }

    /** Parameterized Constructor **/
    NodeSessionEntry::NodeSessionEntry(
        uint32_t nSessionId_,
        const uint256_t& hashKeyID_,
        const uint256_t& hashGenesis_,
        bool fStatelessLive_,
        bool fLegacyLive_,
        uint64_t nLastActivity_,
        const MiningContext& context_
    )
        : nSessionId(nSessionId_)
        , hashKeyID(hashKeyID_)
        , hashGenesis(hashGenesis_)
        , fStatelessLive(fStatelessLive_)
        , fLegacyLive(fLegacyLive_)
        , nLastActivity(nLastActivity_)
        , context(context_)
    {
    }

    /** WithStatelessLive **/
    NodeSessionEntry NodeSessionEntry::WithStatelessLive(bool fLive_) const
    {
        NodeSessionEntry entry = *this;
        entry.fStatelessLive = fLive_;
        return entry;
    }

    /** WithLegacyLive **/
    NodeSessionEntry NodeSessionEntry::WithLegacyLive(bool fLive_) const
    {
        NodeSessionEntry entry = *this;
        entry.fLegacyLive = fLive_;
        return entry;
    }

    /** WithActivity **/
    NodeSessionEntry NodeSessionEntry::WithActivity(uint64_t nTime_) const
    {
        NodeSessionEntry entry = *this;
        entry.nLastActivity = nTime_;
        return entry;
    }

    /** WithContext **/
    NodeSessionEntry NodeSessionEntry::WithContext(const MiningContext& context_) const
    {
        NodeSessionEntry entry = *this;
        entry.context = context_;
        return entry;
    }

    SessionBinding NodeSessionEntry::GetSessionBinding() const
    {
        SessionBinding binding = context.GetSessionBinding();
        binding.nSessionId = nSessionId;   // Entry-level fields are authoritative; override context values for node-side comparisons.
        binding.hashGenesis = hashGenesis; // Entry-level fields are authoritative; override context values for node-side comparisons.
        binding.hashKeyID = hashKeyID;     // Entry-level fields are authoritative; override context values for node-side comparisons.
        return binding;
    }

    CryptoContext NodeSessionEntry::GetCryptoContext() const
    {
        CryptoContext crypto = context.GetCryptoContext();
        crypto.nSessionId = nSessionId;
        crypto.hashGenesis = hashGenesis;
        crypto.hashKeyID = hashKeyID;
        return crypto;
    }

    SessionConsistencyResult NodeSessionEntry::ValidateConsistency() const
    {
        /* Entry-level identity validation via SessionBinding::IsValid(). */
        const SessionBinding entryBinding = GetSessionBinding();
        if(entryBinding.nSessionId == 0)
            return SessionConsistencyResult::MissingSessionId;

        if(entryBinding.hashGenesis == 0)
            return SessionConsistencyResult::MissingGenesis;

        if(entryBinding.hashKeyID == 0)
            return SessionConsistencyResult::MissingFalconKey;

        /* Cross-check FIRST: context identity fields must match entry-level
         * fields.  Identity divergence is a more severe diagnostic than any
         * field-completeness error reported by MiningContext::ValidateConsistency
         * (a peer attempting to bind to a different session ID / genesis /
         * Falcon key is a security event; a peer that simply hasn't completed
         * the encryption handshake is operational state).  Reporting the
         * identity error first preserves the operator triage signal and
         * matches the diagnostic contract asserted by node_session_registry
         * unit tests :502 and :540.
         *
         * Uses SessionBinding::FirstMismatch() to centralize the partial-match
         * comparison.  Fields that are zero in the context binding are
         * skipped — they are not yet authoritative. */
        const SessionBinding ctxBinding = context.GetSessionBinding();
        const SessionConsistencyResult crossCheck = ctxBinding.FirstMismatch(entryBinding);
        if(crossCheck != SessionConsistencyResult::Ok)
            return crossCheck;

        /* Then delegate to the embedded MiningContext's own consistency
         * checks (reward-bound coherence, encryption-ready coherence, etc.). */
        const SessionConsistencyResult contextConsistency = context.ValidateConsistency();
        if(contextConsistency != SessionConsistencyResult::Ok)
            return contextConsistency;

        return SessionConsistencyResult::Ok;
    }

    /** AnyPortLive **/
    bool NodeSessionEntry::AnyPortLive() const
    {
        return fStatelessLive || fLegacyLive;
    }

    /** IsExpired **/
    bool NodeSessionEntry::IsExpired(uint64_t nTimeoutSec, uint64_t nNow) const
    {
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Session is expired if no ports are live AND last activity exceeded timeout */
        if(AnyPortLive())
            return false;

        /* Guard against non-monotonic timestamps (BUG-4 fix).
         * runtime::unifiedtimestamp() uses system_clock + UNIFIED_AVERAGE_OFFSET,
         * which can move backwards after time adjustments.  If nNow < nLastActivity,
         * the unsigned subtraction wraps to a huge value, falsely triggering expiry.
         * Be conservative: if the clock went backwards, the session is NOT expired.
         *
         * NOTE: Use strict `<` (not `<=`) so that the same-second case
         * (nNow == nLastActivity) falls through to the comparison below.  This
         * keeps the documented `SweepExpired(0)` semantic — "expire immediately
         * after disconnect" — working when the disconnect and the sweep land
         * in the same wall-clock second. */
        if(nNow < nLastActivity)
            return false;

        /* Use `>=` rather than `>` so that nTimeoutSec == 0 means
         * "expire as soon as nNow has caught up with nLastActivity" rather
         * than "expire only after at least one second of inactivity". */
        return (nNow - nLastActivity) >= nTimeoutSec;
    }


    /* ═══════════════════════════════════════════════════════════════════════════════════════
     * NodeSessionRegistry Implementation
     * ═══════════════════════════════════════════════════════════════════════════════════════ */

    /** Private Constructor (Singleton) **/
    NodeSessionRegistry::NodeSessionRegistry()
        : m_mapLiveByKey()
        , m_mapInactiveByKey()
        , m_mapSessionToKey()
    {
    }

    /** Get Singleton Instance **/
    NodeSessionRegistry& NodeSessionRegistry::Get()
    {
        static NodeSessionRegistry instance;
        return instance;
    }

    /** RegisterOrRefresh **/
    std::pair<uint32_t, bool> NodeSessionRegistry::RegisterOrRefresh(
        const uint256_t& hashKeyID,
        const uint256_t& hashGenesis,
        const MiningContext& context,
        ProtocolLane lane
    )
    {
        const uint64_t nNow = runtime::unifiedtimestamp();

        /* Derive session ID using canonical derivation (matches ProcessFalconResponse) */
        const uint32_t nSessionId = MiningContext::DeriveSessionId(hashKeyID);

        const auto fnRefreshEntry =
            [&](const NodeSessionEntry& existing)
            {
                NodeSessionEntry entry = existing;

                /* Enforce single-lane operation: a miner can only be on ONE port at a time.
                 * When a lane registers or refreshes, the other lane is marked dead.
                 * This prevents stale dual-lane state from persisting across reconnections. */
                if(lane == ProtocolLane::STATELESS)
                {
                    entry = entry.WithStatelessLive(true);
                    entry = entry.WithLegacyLive(false);
                }
                else
                {
                    entry = entry.WithLegacyLive(true);
                    entry = entry.WithStatelessLive(false);
                }

                /* Update activity timestamp and context */
                entry = entry.WithActivity(nNow);
                entry = entry.WithContext(context);

                return entry;
            };

        const auto fnSyncStore =
            [&](const NodeSessionEntry& entry)
            {
                /* Dual-write: sync liveness flags to SessionStore.
                 * Return value intentionally unchecked: during dual-write migration the
                 * session may not yet exist in SessionStore (populated by UpdateMiner). */
                SessionStore::Get().Transform(hashKeyID, [&](const CanonicalSession& cs) {
                    CanonicalSession updated = cs;
                    updated.fStatelessLive = entry.fStatelessLive;
                    updated.fLegacyLive    = entry.fLegacyLive;
                    updated.nLastActivity  = entry.nLastActivity;
                    return updated;
                });
            };

        /* Try to atomically refresh an existing session.
         * Transform() operates on the CURRENT value under the write lock,
         * eliminating the TOCTOU race of the old snapshot-copy-modify-write
         * pattern (BUG-2 fix). */
        bool fTransformed = m_mapLiveByKey.Transform(hashKeyID, fnRefreshEntry);

        if(fTransformed)
        {
            auto optEntry = m_mapLiveByKey.Get(hashKeyID);
            if(optEntry.has_value())
                fnSyncStore(optEntry.value());

            debug::log(3, FUNCTION, "Refreshed session ", nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            return {nSessionId, false};  // Not new
        }

        /* Split-map transitions are serialized only on the slow path after the
         * lock-free live refresh misses.  This keeps the common live-session
         * refresh path lock-free while ensuring inactive↔live promotion cannot
         * race with disconnect demotion. */
        std::lock_guard<std::mutex> lock(m_transitionMutex);

        /* Retry the live map under transition lock in case a concurrent thread
         * promoted the session after the first lock-free refresh attempt. */
        fTransformed = m_mapLiveByKey.Transform(hashKeyID, fnRefreshEntry);
        if(fTransformed)
        {
            auto optEntry = m_mapLiveByKey.Get(hashKeyID);
            if(optEntry.has_value())
                fnSyncStore(optEntry.value());

            debug::log(3, FUNCTION, "Refreshed session ", nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            return {nSessionId, false};
        }

        /* Reactivate a disconnected session from the inactive cache. */
        auto optInactive = m_mapInactiveByKey.GetAndRemove(hashKeyID);
        if(optInactive.has_value())
        {
            NodeSessionEntry entry = fnRefreshEntry(optInactive.value());
            m_mapLiveByKey.InsertOrUpdate(hashKeyID, entry);
            m_mapSessionToKey.InsertOrUpdate(entry.nSessionId, hashKeyID);
            fnSyncStore(entry);

            debug::log(2, FUNCTION, "Reactivated session ", entry.nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            return {entry.nSessionId, false};
        }

        /* New session — live runtime state is non-evictable.
         * Inactive-cache budgets are enforced from periodic maintenance, not on
         * the hot authentication/keepalive path. */

        /* New session - create entry */
        NodeSessionEntry entry(
            nSessionId,
            hashKeyID,
            hashGenesis,
            (lane == ProtocolLane::STATELESS),  // fStatelessLive
            (lane == ProtocolLane::LEGACY),     // fLegacyLive
            nNow,
            context
        );

        /* Store in both maps.
         * OPT-4: Collision detection — if mapSessionToKey already holds a
         * *different* hashKeyID for this nSessionId, two distinct Falcon keys
         * have collided in their lower 32 bits.  The displaced old entry in either
         * m_mapLiveByKey or m_mapInactiveByKey would become orphaned (unreachable via
         * session ID lookup and never cleaned up by SweepExpired, which iterates the registry maps but
         * relies on m_mapSessionToKey for the reverse mapping).
         *
         * Fix: explicitly remove the displaced entry from both registry maps and
         * SessionStore so it doesn't leak memory or cause misattributed blocks.  The newer
         * key wins since it's the most recently authenticated miner. */
        {
            auto existingKey = m_mapSessionToKey.Get(nSessionId);
            if(existingKey && *existingKey != hashKeyID)
            {
                debug::warning(FUNCTION,
                    "mapSessionToKey collision: session ", nSessionId,
                    " was mapped to key ", existingKey->SubString(),
                    " but new key ", hashKeyID.SubString(),
                    " derives the same session ID — evicting displaced entry");

                /* Remove displaced entry from whichever registry map still owns it
                 * before replacing the reverse session-id binding. */
                const bool fRemovedLive = m_mapLiveByKey.Erase(*existingKey);
                const bool fRemovedInactive = m_mapInactiveByKey.Erase(*existingKey);
                m_mapSessionToKey.Erase(nSessionId);
                SessionStore::Get().Remove(*existingKey);

                debug::log(2, FUNCTION, "Collision cleanup for session ", nSessionId,
                           ": live_removed=", fRemovedLive,
                           " inactive_removed=", fRemovedInactive);
            }
        }
        m_mapLiveByKey.InsertOrUpdate(hashKeyID, entry);
        m_mapSessionToKey.InsertOrUpdate(nSessionId, hashKeyID);
        fnSyncStore(entry);

        debug::log(2, FUNCTION, "Registered new session ", nSessionId,
                   " for key ", hashKeyID.SubString(),
                   " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                   " genesis=", hashGenesis.SubString(),
                   " consistency=", SessionConsistencyResultString(entry.ValidateConsistency()));

        return {nSessionId, true};  // New session
    }

    /** Lookup **/
    std::optional<NodeSessionEntry> NodeSessionRegistry::Lookup(uint32_t nSessionId) const
    {
        /* Reverse lookup: nSessionId → hashKeyID → NodeSessionEntry */
        auto keyOpt = m_mapSessionToKey.Get(nSessionId);
        if(!keyOpt)
            return std::nullopt;

        auto liveOpt = m_mapLiveByKey.Get(*keyOpt);
        if(liveOpt.has_value())
            return liveOpt;

        return m_mapInactiveByKey.Get(*keyOpt);
    }

    /** LookupByKey **/
    std::optional<NodeSessionEntry> NodeSessionRegistry::LookupByKey(const uint256_t& hashKeyID) const
    {
        auto liveOpt = m_mapLiveByKey.Get(hashKeyID);
        if(liveOpt.has_value())
            return liveOpt;

        return m_mapInactiveByKey.Get(hashKeyID);
    }

    /** UpdateContext **/
    bool NodeSessionRegistry::UpdateContext(const uint256_t& hashKeyID, const MiningContext& context)
    {
        /* Use Transform() for atomic read-modify-write, eliminating the TOCTOU
         * race of the old snapshot-copy-modify-write pattern (BUG-2 fix). */
        bool fTransformed = m_mapLiveByKey.Transform(hashKeyID,
            [&context](const NodeSessionEntry& existing) {
                return existing.WithContext(context);
            });

        if(!fTransformed)
        {
            fTransformed = m_mapInactiveByKey.Transform(hashKeyID,
                [&context](const NodeSessionEntry& existing) {
                    return existing.WithContext(context);
                });
        }

        if(fTransformed)
        {
            debug::log(3, FUNCTION, "Updated context for key=", hashKeyID.SubString());
        }

        return fTransformed;
    }

    /** MarkDisconnected **/
    bool NodeSessionRegistry::MarkDisconnected(const uint256_t& hashKeyID, ProtocolLane lane)
    {
        const uint64_t nNow = runtime::unifiedtimestamp();

        /* Use Transform() for atomic read-modify-write, eliminating the TOCTOU
         * race where a concurrent RegisterOrRefresh() could be clobbered by our
         * stale snapshot write-back (BUG-2 fix). */
        bool fTransformed = m_mapLiveByKey.Transform(hashKeyID,
            [lane, nNow](const NodeSessionEntry& existing) {
                NodeSessionEntry entry = existing;

                /* Mark the appropriate lane as disconnected */
                if(lane == ProtocolLane::STATELESS)
                    entry = entry.WithStatelessLive(false);
                else
                    entry = entry.WithLegacyLive(false);

                /* Update activity timestamp */
                entry = entry.WithActivity(nNow);

                return entry;
            });

        if(!fTransformed)
        {
            fTransformed = m_mapInactiveByKey.Transform(hashKeyID,
                [lane, nNow](const NodeSessionEntry& existing) {
                    NodeSessionEntry entry = existing;

                    if(lane == ProtocolLane::STATELESS)
                        entry = entry.WithStatelessLive(false);
                    else
                        entry = entry.WithLegacyLive(false);

                    return entry.WithActivity(nNow);
                });
        }

        if(fTransformed)
        {
            std::lock_guard<std::mutex> lock(m_transitionMutex);

            /* Remove-and-classify under the transition lock.  A concurrent
             * refresh that misses the now-absent live entry must wait on the
             * same transition mutex before promoting from inactive or creating
             * a new entry, so the entry cannot end up live and inactive at once. */
            auto optEntry = m_mapLiveByKey.Get(hashKeyID);
            if(optEntry.has_value() && !optEntry->AnyPortLive())
            {
                auto movedEntry = m_mapLiveByKey.GetAndRemove(hashKeyID);
                if(movedEntry.has_value())
                {
                    if(movedEntry->AnyPortLive())
                        m_mapLiveByKey.InsertOrUpdate(hashKeyID, movedEntry.value());
                    else
                        m_mapInactiveByKey.InsertOrUpdate(hashKeyID, movedEntry.value());
                }
            }
            else if(optEntry.has_value())
            {
                /* Still live on the opposite lane after this disconnect; no move required. */
            }
            else
            {
                /* Another thread may have already transitioned it while we were
                 * holding only the per-map transform lock. */
                debug::log(2, FUNCTION, "MarkDisconnected transition skipped; live entry vanished for key=",
                           hashKeyID.SubString());
            }

            debug::log(3, FUNCTION, "Marked disconnected key=", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            /* Dual-write: sync liveness flags to SessionStore.
             * Return value intentionally unchecked: during dual-write migration the
             * session may not yet exist in SessionStore (populated by UpdateMiner). */
            SessionStore::Get().Transform(hashKeyID, [&](const CanonicalSession& cs) {
                CanonicalSession updated = cs;
                if(lane == ProtocolLane::STATELESS)
                    updated.fStatelessLive = false;
                else
                    updated.fLegacyLive = false;
                updated.nLastActivity = nNow;
                return updated;
            });
        }
        else
        {
            debug::log(2, FUNCTION, "MarkDisconnected found no session for key=",
                       hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));
        }

        return fTransformed;
    }

    /** SweepExpired **/
    uint32_t NodeSessionRegistry::SweepExpired(uint64_t nTimeoutSec)
    {
        const uint64_t nNow = runtime::unifiedtimestamp();
        uint32_t nRemoved = 0;

        /* Take a snapshot for candidate selection.  Because the snapshot may be
         * stale by the time we act on it (a keepalive or re-auth could refresh
         * the entry), we re-read the live entry before erasing. */
        auto allSessions = m_mapInactiveByKey.GetAllPairs();

        for(const auto& pair : allSessions)
        {
            const uint256_t& hashKeyID = pair.first;
            const NodeSessionEntry& snapshotEntry = pair.second;

            /* Candidate from snapshot — may be stale */
            if(!snapshotEntry.IsExpired(nTimeoutSec, nNow))
                continue;

            /* Re-read live entry before deleting to avoid removing a session
             * that was refreshed after the snapshot was taken. */
            auto liveEntry = m_mapInactiveByKey.Get(hashKeyID);
            if(!liveEntry.has_value() || !liveEntry->IsExpired(nTimeoutSec, nNow))
                continue;

            /* Remove from both maps */
            m_mapInactiveByKey.Erase(hashKeyID);
            m_mapSessionToKey.Erase(liveEntry->nSessionId);

            /* Dual-write: also remove from SessionStore */
            SessionStore::Get().Remove(hashKeyID);

            debug::log(2, FUNCTION, "Swept expired session ", liveEntry->nSessionId,
                       " key=", hashKeyID.SubString(),
                       " inactiveFor=", (nNow - liveEntry->nLastActivity), "s");

            ++nRemoved;
        }

        if(nRemoved > 0)
        {
            debug::log(1, FUNCTION, "Swept ", nRemoved, " expired sessions");
        }

        return nRemoved;
    }

    /** Count **/
    size_t NodeSessionRegistry::Count() const
    {
        return m_mapLiveByKey.Size() + m_mapInactiveByKey.Size();
    }

    /** CountLive **/
    size_t NodeSessionRegistry::CountLive() const
    {
        /* Live sessions are stored in their own map, so the count is O(1). */
        return m_mapLiveByKey.Size();
    }

    /** EnforceCacheLimit **/
    uint32_t NodeSessionRegistry::EnforceCacheLimit(size_t nMaxSize)
    {
        if(m_mapInactiveByKey.Size() <= nMaxSize)
            return 0;

        uint32_t nRemoved = 0;

        /* Collect inactive cache entries sorted by last activity (oldest first). */
        auto allSessions = m_mapInactiveByKey.GetAllPairs();
        std::sort(allSessions.begin(), allSessions.end(),
            [](const auto& a, const auto& b) {
                return a.second.nLastActivity < b.second.nLastActivity;
            });

        /* Evict inactive cache entries only; live sessions are stored
         * separately and are never touched here. */
        for(const auto& pair : allSessions)
        {
            if(m_mapInactiveByKey.Size() <= nMaxSize)
                break;

            auto liveEntry = m_mapInactiveByKey.Get(pair.first);
            if(!liveEntry.has_value())
                continue;

            if(liveEntry->AnyPortLive())
            {
                debug::warning(FUNCTION, "Inactive cache contained live session ",
                               liveEntry->nSessionId, " for key ", pair.first.SubString(),
                               " — restoring it to the live map");
                m_mapInactiveByKey.Erase(pair.first);
                m_mapLiveByKey.InsertOrUpdate(pair.first, liveEntry.value());
                continue;
            }

            m_mapInactiveByKey.Erase(pair.first);
            m_mapSessionToKey.Erase(liveEntry->nSessionId);
            SessionStore::Get().Remove(pair.first);
            ++nRemoved;
        }

        if(nRemoved > 0)
        {
            debug::log(1, FUNCTION, "Evicted ", nRemoved,
                       " inactive sessions to enforce cache budget of ", nMaxSize);
        }

        return nRemoved;
    }

    /** Clear **/
    void NodeSessionRegistry::Clear()
    {
        m_mapLiveByKey.Clear();
        m_mapInactiveByKey.Clear();
        m_mapSessionToKey.Clear();
        debug::log(1, FUNCTION, "Cleared all sessions");
    }

} // namespace LLP
