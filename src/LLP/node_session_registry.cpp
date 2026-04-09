/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_session_registry.h>
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

        /* Delegate to the embedded MiningContext's own consistency checks. */
        const SessionConsistencyResult contextConsistency = context.ValidateConsistency();
        if(contextConsistency != SessionConsistencyResult::Ok)
            return contextConsistency;

        /* Cross-check: context identity fields must match entry-level fields.
         * Uses SessionBinding::FirstMismatch() to centralize the partial-match
         * comparison.  Fields that are zero in the context binding are
         * skipped — they are not yet authoritative. */
        const SessionBinding ctxBinding = context.GetSessionBinding();
        const SessionConsistencyResult crossCheck = ctxBinding.FirstMismatch(entryBinding);
        if(crossCheck != SessionConsistencyResult::Ok)
            return crossCheck;

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
         * Be conservative: if the clock went backwards, the session is NOT expired. */
        if(nNow <= nLastActivity)
            return false;

        return (nNow - nLastActivity) > nTimeoutSec;
    }


    /* ═══════════════════════════════════════════════════════════════════════════════════════
     * NodeSessionRegistry Implementation
     * ═══════════════════════════════════════════════════════════════════════════════════════ */

    /** Private Constructor (Singleton) **/
    NodeSessionRegistry::NodeSessionRegistry()
        : m_mapByKey()
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

        /* Try to atomically refresh an existing session.
         * Transform() operates on the CURRENT value under the write lock,
         * eliminating the TOCTOU race of the old snapshot-copy-modify-write
         * pattern (BUG-2 fix). */
        bool fTransformed = m_mapByKey.Transform(hashKeyID,
            [&](const NodeSessionEntry& existing) {
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
            });

        if(fTransformed)
        {
            debug::log(3, FUNCTION, "Refreshed session ", nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

            return {nSessionId, false};  // Not new
        }

        /* New session — enforce capacity limit before inserting (BUG-5 fix).
         * Use a 20% overshoot threshold to amortize the cost of enforcement,
         * matching the pattern used in StatelessMinerManager. */
        {
            size_t nCurrent = m_mapByKey.Size();
            size_t nLimit = 1000;
            if(nCurrent > nLimit + nLimit / 5)
                EnforceCacheLimit(nLimit);
        }

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
         * have collided in their lower 32 bits.  The displaced old entry in
         * m_mapByKey would become orphaned (unreachable via session ID lookup
         * and never cleaned up by SweepExpired, which iterates m_mapByKey but
         * relies on m_mapSessionToKey for the reverse mapping).
         *
         * Fix: explicitly remove the displaced entry from m_mapByKey so it
         * doesn't leak memory or cause misattributed blocks.  The newer
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

                /* Remove displaced entry from primary map to prevent orphaning */
                m_mapByKey.Erase(*existingKey);
            }
        }
        m_mapByKey.InsertOrUpdate(hashKeyID, entry);
        m_mapSessionToKey.InsertOrUpdate(nSessionId, hashKeyID);

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

        return m_mapByKey.Get(*keyOpt);
    }

    /** LookupByKey **/
    std::optional<NodeSessionEntry> NodeSessionRegistry::LookupByKey(const uint256_t& hashKeyID) const
    {
        return m_mapByKey.Get(hashKeyID);
    }

    /** UpdateContext **/
    bool NodeSessionRegistry::UpdateContext(const uint256_t& hashKeyID, const MiningContext& context)
    {
        /* Use Transform() for atomic read-modify-write, eliminating the TOCTOU
         * race of the old snapshot-copy-modify-write pattern (BUG-2 fix). */
        bool fTransformed = m_mapByKey.Transform(hashKeyID,
            [&context](const NodeSessionEntry& existing) {
                return existing.WithContext(context);
            });

        if(fTransformed)
        {
            debug::log(3, FUNCTION, "Updated context for key=", hashKeyID.SubString());
        }

        return fTransformed;
    }

    /** MarkDisconnected **/
    bool NodeSessionRegistry::MarkDisconnected(const uint256_t& hashKeyID, ProtocolLane lane)
    {
        /* Use Transform() for atomic read-modify-write, eliminating the TOCTOU
         * race where a concurrent RegisterOrRefresh() could be clobbered by our
         * stale snapshot write-back (BUG-2 fix). */
        bool fTransformed = m_mapByKey.Transform(hashKeyID,
            [lane](const NodeSessionEntry& existing) {
                NodeSessionEntry entry = existing;

                /* Mark the appropriate lane as disconnected */
                if(lane == ProtocolLane::STATELESS)
                    entry = entry.WithStatelessLive(false);
                else
                    entry = entry.WithLegacyLive(false);

                /* Update activity timestamp */
                entry = entry.WithActivity(runtime::unifiedtimestamp());

                return entry;
            });

        if(fTransformed)
        {
            debug::log(3, FUNCTION, "Marked disconnected key=", hashKeyID.SubString(),
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
        auto allSessions = m_mapByKey.GetAllPairs();

        for(const auto& pair : allSessions)
        {
            const uint256_t& hashKeyID = pair.first;
            const NodeSessionEntry& snapshotEntry = pair.second;

            /* Candidate from snapshot — may be stale */
            if(!snapshotEntry.IsExpired(nTimeoutSec, nNow))
                continue;

            /* Re-read live entry before deleting to avoid removing a session
             * that was refreshed after the snapshot was taken. */
            auto liveEntry = m_mapByKey.Get(hashKeyID);
            if(!liveEntry.has_value() || !liveEntry->IsExpired(nTimeoutSec, nNow))
                continue;

            /* Remove from both maps */
            m_mapByKey.Erase(hashKeyID);
            m_mapSessionToKey.Erase(liveEntry->nSessionId);

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
        return m_mapByKey.Size();
    }

    /** CountLive **/
    size_t NodeSessionRegistry::CountLive() const
    {
        /* Use ForEach() to iterate under a single shared lock instead of
         * GetAll() which copies the entire map (BUG-6 fix). */
        size_t nLive = 0;

        m_mapByKey.ForEach([&nLive](const uint256_t& /*key*/, const NodeSessionEntry& entry) {
            if(entry.AnyPortLive())
                ++nLive;
        });

        return nLive;
    }

    /** EnforceCacheLimit **/
    uint32_t NodeSessionRegistry::EnforceCacheLimit(size_t nMaxSize)
    {
        if(m_mapByKey.Size() <= nMaxSize)
            return 0;

        const uint64_t nNow = runtime::unifiedtimestamp();
        uint32_t nRemoved = 0;

        /* Collect all entries sorted by last activity (oldest first) */
        auto allSessions = m_mapByKey.GetAllPairs();
        std::sort(allSessions.begin(), allSessions.end(),
            [](const auto& a, const auto& b) {
                return a.second.nLastActivity < b.second.nLastActivity;
            });

        /* Pass 1: Evict entries with no live connections (oldest first) */
        for(const auto& pair : allSessions)
        {
            if(m_mapByKey.Size() <= nMaxSize)
                break;

            if(!pair.second.AnyPortLive())
            {
                m_mapByKey.Erase(pair.first);
                m_mapSessionToKey.Erase(pair.second.nSessionId);
                ++nRemoved;
            }
        }

        /* Pass 2: If still over limit, evict oldest entries regardless of liveness */
        if(m_mapByKey.Size() > nMaxSize)
        {
            for(const auto& pair : allSessions)
            {
                if(m_mapByKey.Size() <= nMaxSize)
                    break;

                /* May have been erased in pass 1, check existence */
                auto liveEntry = m_mapByKey.Get(pair.first);
                if(!liveEntry.has_value())
                    continue;

                m_mapByKey.Erase(pair.first);
                m_mapSessionToKey.Erase(liveEntry->nSessionId);
                ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(1, FUNCTION, "Evicted ", nRemoved,
                       " sessions to enforce cache limit of ", nMaxSize);
        }

        return nRemoved;
    }

    /** Clear **/
    void NodeSessionRegistry::Clear()
    {
        m_mapByKey.Clear();
        m_mapSessionToKey.Clear();
        debug::log(1, FUNCTION, "Cleared all sessions");
    }

} // namespace LLP
