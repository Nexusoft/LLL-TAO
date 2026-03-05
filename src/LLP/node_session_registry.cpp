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

        /* Derive session ID from lower 32 bits of hashKeyID (matches ProcessFalconResponse) */
        const uint32_t nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0));

        /* Check if session exists */
        auto existing = m_mapByKey.Get(hashKeyID);
        if(existing)
        {
            /* Session exists - refresh it */
            NodeSessionEntry entry = *existing;

            /* Update liveness flags based on lane */
            if(lane == ProtocolLane::STATELESS)
                entry = entry.WithStatelessLive(true);
            else
                entry = entry.WithLegacyLive(true);

            /* Update activity timestamp and context */
            entry = entry.WithActivity(nNow);
            entry = entry.WithContext(context);

            /* Store updated entry */
            m_mapByKey.InsertOrUpdate(hashKeyID, entry);

            debug::log(3, FUNCTION, "Refreshed session ", nSessionId,
                       " for key ", hashKeyID.SubString(),
                       " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"));

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

        /* Store in both maps */
        m_mapByKey.InsertOrUpdate(hashKeyID, entry);
        m_mapSessionToKey.InsertOrUpdate(nSessionId, hashKeyID);

        debug::log(2, FUNCTION, "Registered new session ", nSessionId,
                   " for key ", hashKeyID.SubString(),
                   " lane=", (lane == ProtocolLane::STATELESS ? "STATELESS" : "LEGACY"),
                   " genesis=", hashGenesis.SubString());

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
                   " key=", hashKeyID.SubString());

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

        /* Get all sessions for iteration */
        auto allSessions = m_mapByKey.GetAllPairs();

        for(const auto& pair : allSessions)
        {
            const uint256_t& hashKeyID = pair.first;
            const NodeSessionEntry& entry = pair.second;

            /* Check if session is expired */
            if(entry.IsExpired(nTimeoutSec, nNow))
            {
                /* Remove from both maps */
                m_mapByKey.Erase(hashKeyID);
                m_mapSessionToKey.Erase(entry.nSessionId);

                debug::log(2, FUNCTION, "Swept expired session ", entry.nSessionId,
                           " key=", hashKeyID.SubString(),
                           " inactiveFor=", (nNow - entry.nLastActivity), "s");

                ++nRemoved;
            }
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
