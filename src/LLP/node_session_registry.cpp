/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_session_registry.h>
#include <LLP/include/active_session_board.h>
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

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

        /* Check if session exists */
        auto existing = m_mapByKey.Get(hashKeyID);
        if(existing)
        {
            /* Session exists - refresh it */
            NodeSessionEntry entry = *existing;

            /* Enforce single-lane operation: a miner can only be on ONE port at a time.
             * When a lane registers or refreshes, the other lane is marked dead.
             * This prevents stale dual-lane state from persisting across reconnections.
             *
             * Also notify ActiveSessionBoard so it stops sending push notifications
             * on the dead lane.  Without this, the board continues tracking the old
             * (sessionId, deadLane) as "active", sending ghost notifications that
             * increment nFailedPackets and eventually soft-disconnect the session. */
            if(lane == ProtocolLane::STATELESS)
            {
                entry = entry.WithStatelessLive(true);
                if(entry.fLegacyLive)
                    ActiveSessionBoard::Get().MarkDisconnected(nSessionId, ProtocolLane::LEGACY);
                entry = entry.WithLegacyLive(false);
            }
            else
            {
                entry = entry.WithLegacyLive(true);
                if(entry.fStatelessLive)
                    ActiveSessionBoard::Get().MarkDisconnected(nSessionId, ProtocolLane::STATELESS);
                entry = entry.WithStatelessLive(false);
            }

            /* Update activity timestamp and context */
            entry = entry.WithActivity(nNow);
            entry = entry.WithContext(context);

            /* Store updated entry */
            m_mapByKey.InsertOrUpdate(hashKeyID, entry);

            debug::log(3, FUNCTION, "Refreshed session ", nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                       " consistency=", SessionConsistencyResultString(entry.ValidateConsistency()));

            return {nSessionId, false};  // Not new
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
        auto existing = m_mapByKey.Get(hashKeyID);
        if(!existing)
            return false;

        NodeSessionEntry entry = existing->WithContext(context);
        m_mapByKey.InsertOrUpdate(hashKeyID, entry);

        debug::log(3, FUNCTION, "Updated context for session ", entry.nSessionId,
                   " key=", hashKeyID.SubString(),
                   " consistency=", SessionConsistencyResultString(entry.ValidateConsistency()));

        return true;
    }

    /** MarkDisconnected **/
    bool NodeSessionRegistry::MarkDisconnected(const uint256_t& hashKeyID, ProtocolLane lane)
    {
        auto existing = m_mapByKey.Get(hashKeyID);
        if(!existing)
            return false;

        NodeSessionEntry entry = *existing;

        /* Mark the appropriate lane as disconnected */
        if(lane == ProtocolLane::STATELESS)
            entry = entry.WithStatelessLive(false);
        else
            entry = entry.WithLegacyLive(false);

        /* Update activity timestamp */
        entry = entry.WithActivity(runtime::unifiedtimestamp());

        /* Store updated entry */
        m_mapByKey.InsertOrUpdate(hashKeyID, entry);

        debug::log(3, FUNCTION, "Marked disconnected session ", entry.nSessionId,
                   " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                   " anyLive=", entry.AnyPortLive());

        return true;
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
        size_t nLive = 0;
        auto allSessions = m_mapByKey.GetAll();

        for(const auto& entry : allSessions)
        {
            if(entry.AnyPortLive())
                ++nLive;
        }

        return nLive;
    }

    /** Clear **/
    void NodeSessionRegistry::Clear()
    {
        m_mapByKey.Clear();
        m_mapSessionToKey.Clear();
        debug::log(1, FUNCTION, "Cleared all sessions");
    }

} // namespace LLP
