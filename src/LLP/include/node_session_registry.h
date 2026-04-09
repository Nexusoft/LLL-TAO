/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NODE_SESSION_REGISTRY_H
#define NEXUS_LLP_INCLUDE_NODE_SESSION_REGISTRY_H

#include <LLP/include/stateless_miner.h>
#include <LLP/include/session_recovery.h>
#include <Util/templates/concurrent_hashmap.h>
#include <LLC/types/uint1024.h>

#include <cstdint>
#include <string>
#include <optional>
#include <atomic>

namespace LLP
{
    /** NodeSessionEntryKey
     *
     *  Lightweight identity + liveness tuple extracted from NodeSessionEntry.
     *  Contains only the three identity scalars (nSessionId, hashKeyID,
     *  hashGenesis), the two liveness booleans, and the activity timestamp —
     *  no MiningContext.  Significantly cheaper to copy than the full entry.
     *
     *  The inline With*() builders return copies with a single field modified,
     *  mirroring the NodeSessionEntry builder API for ergonomic test code.
     *
     *  NOTE: uint256_t is not constexpr in C++17, so the builders are regular
     *  inline functions rather than constexpr.
     *
     **/
    struct NodeSessionEntryKey
    {
        uint32_t  nSessionId     = 0;
        uint256_t hashKeyID      = 0;
        uint256_t hashGenesis    = 0;
        bool      fStatelessLive = false;
        bool      fLegacyLive    = false;
        uint64_t  nLastActivity  = 0;

        /** Default Constructor **/
        NodeSessionEntryKey() = default;

        /** Parameterized Constructor **/
        NodeSessionEntryKey(
            uint32_t nSessionId_,
            const uint256_t& hashKeyID_,
            const uint256_t& hashGenesis_,
            bool fStatelessLive_,
            bool fLegacyLive_,
            uint64_t nLastActivity_
        )
            : nSessionId(nSessionId_)
            , hashKeyID(hashKeyID_)
            , hashGenesis(hashGenesis_)
            , fStatelessLive(fStatelessLive_)
            , fLegacyLive(fLegacyLive_)
            , nLastActivity(nLastActivity_)
        {
        }

        /** WithStatelessLive — returns copy with updated stateless liveness. **/
        NodeSessionEntryKey WithStatelessLive(bool fLive_) const
        {
            NodeSessionEntryKey key = *this;
            key.fStatelessLive = fLive_;
            return key;
        }

        /** WithLegacyLive — returns copy with updated legacy liveness. **/
        NodeSessionEntryKey WithLegacyLive(bool fLive_) const
        {
            NodeSessionEntryKey key = *this;
            key.fLegacyLive = fLive_;
            return key;
        }

        /** WithActivity — returns copy with updated activity timestamp. **/
        NodeSessionEntryKey WithActivity(uint64_t nTime_) const
        {
            NodeSessionEntryKey key = *this;
            key.nLastActivity = nTime_;
            return key;
        }

        /** AnyPortLive — true if either port is connected. **/
        bool AnyPortLive() const { return fStatelessLive || fLegacyLive; }

        /** GetSessionBinding — returns identity snapshot for comparison. **/
        SessionBinding GetSessionBinding() const
        {
            SessionBinding binding;
            binding.nSessionId = nSessionId;
            binding.hashKeyID  = hashKeyID;
            binding.hashGenesis = hashGenesis;
            return binding;
        }
    };


    /** NodeSessionEntry
     *
     *  Canonical session state for a single mining identity (one Falcon key).
     *  One Falcon identity (hashKeyID) maps to one canonical MiningContext.
     *
     *  SINGLE-LANE POLICY:
     *  ====================
     *  A miner can only be on ONE port at a time.  When RegisterOrRefresh()
     *  activates a lane, the other lane is automatically marked dead.  The
     *  fStatelessLive / fLegacyLive flags track which port is currently active,
     *  but only one may be true at any given time.
     *
     *  DESIGN RATIONALE:
     *  =================
     *  The node-side registry mirrors the miner-side NodeSession structure. When a miner
     *  reconnects on the same or different port, the node looks up the canonical session
     *  by hashKeyID and returns the same nSessionId, preventing session_id=0 bugs.
     *
     *  ARCHITECTURE:
     *  =============
     *  - nSessionId: Derived from lower 32 bits of hashKeyID (ProcessFalconResponse pattern)
     *  - hashGenesis: Tritium account identity for this miner
     *  - fStatelessLive / fLegacyLive: Track which port is currently connected (mutually exclusive)
     *  - context: Canonical MiningContext with WithXxx() immutable builders
     *
     *  SESSION LIVENESS:
     *  =================
     *  nLastActivity is refreshed on every keepalive and authentication event.
     *  SweepExpired() removes entries only when no ports are live AND nLastActivity
     *  exceeds SESSION_LIVENESS_TIMEOUT_SECONDS (24 hours).
     *
     *  THREAD SAFETY:
     *  ==============
     *  NodeSessionRegistry uses ConcurrentHashMap for lock-free reads and fine-grained writes.
     *  NodeSessionEntry is copied on lookup, modified via WithXxx() builders, and stored back.
     *
     **/
    struct NodeSessionEntry
    {
        uint32_t nSessionId;           // Unique session identifier (lower 32 bits of hashKeyID)
        uint256_t hashKeyID;           // Falcon public key hash (stable identity)
        uint256_t hashGenesis;         // Tritium genesis hash (authentication identity)
        bool fStatelessLive;           // True if stateless port (9323) is connected
        bool fLegacyLive;              // True if legacy port (8323) is connected
        uint64_t nLastActivity;        // Timestamp of last activity on any port
        MiningContext context;         // Canonical mining state (shared across ports)

        /** Default Constructor **/
        NodeSessionEntry();

        /** Parameterized Constructor **/
        NodeSessionEntry(
            uint32_t nSessionId_,
            const uint256_t& hashKeyID_,
            const uint256_t& hashGenesis_,
            bool fStatelessLive_,
            bool fLegacyLive_,
            uint64_t nLastActivity_,
            const MiningContext& context_
        );

        /** WithStatelessLive
         *
         *  Returns a new entry with updated stateless connection status.
         *
         *  @param[in] fLive_ New stateless connection status
         *
         *  @return New entry with updated status
         *
         **/
        NodeSessionEntry WithStatelessLive(bool fLive_) const;

        /** WithLegacyLive
         *
         *  Returns a new entry with updated legacy connection status.
         *
         *  @param[in] fLive_ New legacy connection status
         *
         *  @return New entry with updated status
         *
         **/
        NodeSessionEntry WithLegacyLive(bool fLive_) const;

        /** WithActivity
         *
         *  Returns a new entry with updated last activity timestamp.
         *
         *  @param[in] nTime_ New activity timestamp
         *
         *  @return New entry with updated timestamp
         *
         **/
        NodeSessionEntry WithActivity(uint64_t nTime_) const;

        /** WithContext
         *
         *  Returns a new entry with updated mining context.
         *
         *  @param[in] context_ New mining context
         *
         *  @return New entry with updated context
         *
         **/
        NodeSessionEntry WithContext(const MiningContext& context_) const;

        /** GetSessionBinding
         *
         *  Returns the canonical identity snapshot for node-side comparisons and
         *  diagnostics.
         *
         **/
        SessionBinding GetSessionBinding() const;

        /** GetCryptoContext
         *
         *  Returns the canonical crypto snapshot derived from the node-side
         *  canonical context.
         *
         **/
        CryptoContext GetCryptoContext() const;

        /** ValidateConsistency
         *
         *  Canonical node-side consistency validation, including checks that the
         *  cached context still matches the authoritative registry identity.
         *
         **/
        SessionConsistencyResult ValidateConsistency() const;

        /** AnyPortLive
         *
         *  Check if any port is currently connected.
         *
         *  @return true if stateless or legacy port is live
         *
         **/
        bool AnyPortLive() const;

        /** IsExpired
         *
         *  Check if the session has expired based on timeout.
         *
         *  @param[in] nTimeoutSec Timeout in seconds
         *  @param[in] nNow Current timestamp (defaults to current time if 0)
         *
         *  @return true if expired
         *
         **/
        bool IsExpired(uint64_t nTimeoutSec, uint64_t nNow = 0) const;

        /** ToKey
         *
         *  Returns the lightweight identity + liveness key (no MiningContext).
         *
         **/
        NodeSessionEntryKey ToKey() const
        {
            return NodeSessionEntryKey(
                nSessionId, hashKeyID, hashGenesis,
                fStatelessLive, fLegacyLive, nLastActivity
            );
        }
    };


    /** NodeSessionRegistry
     *
     *  CANONICAL SESSION IDENTITY STORE
     *  =================================
     *  Singleton registry that is the single source of truth for mining session
     *  identity and liveness.  All other session stores (StatelessMinerManager,
     *  SessionRecoveryManager) are derived caches whose data must agree with
     *  this registry.
     *
     *  ARCHITECTURE:
     *  =============
     *  NodeSessionRegistry is the authoritative owner of:
     *    - Session identity (hashKeyID → nSessionId mapping)
     *    - Lane liveness (fStatelessLive / fLegacyLive)
     *    - Activity timestamp (nLastActivity)
     *    - Canonical MiningContext snapshot
     *
     *  StatelessMinerManager:
     *    - Thin address-based index (strAddress → hashKeyID) + statistics
     *    - UpdateMiner() automatically syncs with registry via RegisterOrRefresh()
     *    - RemoveMiner() propagates to both NodeSessionRegistry and SessionRecoveryManager
     *
     *  SessionRecoveryManager:
     *    - Off-line recovery persistence (crypto keys, disposable Falcon keys)
     *    - Slated for removal in Phase 2 (full re-auth or merge into registry)
     *
     *  CANONICAL IDENTITY:
     *  ====================
     *  hashKeyID = SK256(falcon_pubkey) is the canonical, immutable identity.
     *  nSessionId = lower 32 bits of hashKeyID (wire protocol).
     *  All liveness refreshes (keepalive, authentication) must flow through
     *  RegisterOrRefresh() to keep nLastActivity current.
     *
     *  SINGLE-LANE POLICY:
     *  ====================
     *  Each miner identity (hashKeyID) maps to exactly one session that is
     *  active on ONE port at a time.  When RegisterOrRefresh() activates a
     *  lane, the other lane is automatically marked dead.
     *
     *  DESIGN:
     *  =======
     *  - Two-map structure for O(1) lookup by both hashKeyID and nSessionId
     *  - m_mapByKey: Primary storage indexed by Falcon public key hash
     *  - m_mapSessionToKey: Reverse lookup indexed by session ID
     *  - All mutations use Transform() for atomic read-modify-write (no TOCTOU)
     *
     *  USAGE PATTERN:
     *  ==============
     *  1. Authentication (either port):
     *     auto [sessionId, isNew] = registry.RegisterOrRefresh(hashKeyID, hashGenesis, context, lane);
     *     // Patch wire response DATA[1..4] with sessionId
     *
     *  2. Keepalive (critical for liveness):
     *     registry.RegisterOrRefresh(hashKeyID, hashGenesis, context, lane);
     *     // Refreshes nLastActivity to prevent SweepExpired() reaping
     *
     *  3. Disconnection:
     *     registry.MarkDisconnected(hashKeyID, lane);
     *
     *  4. Periodic cleanup:
     *     registry.SweepExpired(timeoutSec);
     *
     *  CAPACITY LIMIT:
     *  ===============
     *  EnforceCacheLimit() prevents unbounded growth from auth floods.
     *  Called automatically from RegisterOrRefresh() when 20% over limit.
     *
     *  THREAD SAFETY:
     *  ==============
     *  Uses ConcurrentHashMap for all storage, providing lock-free reads and
     *  fine-grained write locks. Safe for concurrent access from multiple threads.
     *
     **/
    class NodeSessionRegistry
    {
    public:
        /** Default maximum registry entries before eviction kicks in (BUG-5 fix). */
        static constexpr size_t DEFAULT_MAX_REGISTRY_SIZE = 1000;

        /** Get
         *
         *  Get the global registry instance (singleton).
         *
         *  @return Reference to singleton registry
         *
         **/
        static NodeSessionRegistry& Get();

        /** RegisterOrRefresh
         *
         *  Register a new session or refresh an existing one.
         *  Returns the canonical session ID (same for both ports).
         *
         *  BEHAVIOR:
         *  - If hashKeyID exists: Refresh activity, update context, activate this lane,
         *    deactivate the other lane (single-lane policy)
         *  - If hashKeyID new: Derive nSessionId from hashKeyID, create entry
         *
         *  LIVENESS:
         *  This method MUST be called on every keepalive and authentication event
         *  to keep nLastActivity current.  SweepExpired() is the single authority
         *  for session expiration and uses nLastActivity as the sole clock.
         *
         *  @param[in] hashKeyID Falcon public key hash
         *  @param[in] hashGenesis Tritium genesis hash
         *  @param[in] context Initial mining context
         *  @param[in] lane Protocol lane (LEGACY or STATELESS)
         *
         *  @return Pair of (nSessionId, isNewSession)
         *
         **/
        std::pair<uint32_t, bool> RegisterOrRefresh(
            const uint256_t& hashKeyID,
            const uint256_t& hashGenesis,
            const MiningContext& context,
            ProtocolLane lane
        );

        /** Lookup
         *
         *  Look up session entry by session ID (hot path, O(1)).
         *
         *  @param[in] nSessionId Session identifier
         *
         *  @return Optional entry if found
         *
         **/
        std::optional<NodeSessionEntry> Lookup(uint32_t nSessionId) const;

        /** LookupByKey
         *
         *  Look up session entry by Falcon key hash (auth path, O(1)).
         *
         *  @param[in] hashKeyID Falcon public key hash
         *
         *  @return Optional entry if found
         *
         **/
        std::optional<NodeSessionEntry> LookupByKey(const uint256_t& hashKeyID) const;

        /** UpdateContext
         *
         *  Update the canonical mining context for a session.
         *
         *  @param[in] hashKeyID Falcon public key hash
         *  @param[in] context New mining context
         *
         *  @return true if updated
         *
         **/
        bool UpdateContext(const uint256_t& hashKeyID, const MiningContext& context);

        /** MarkDisconnected
         *
         *  Mark a protocol lane as disconnected.
         *  If both lanes are dead, the entry remains but is marked inactive.
         *
         *  @param[in] hashKeyID Falcon public key hash
         *  @param[in] lane Protocol lane (LEGACY or STATELESS)
         *
         *  @return true if marked
         *
         **/
        bool MarkDisconnected(const uint256_t& hashKeyID, ProtocolLane lane);

        /** SweepExpired
         *
         *  Remove expired sessions with no live connections.
         *  Should be called periodically by a timer.
         *
         *  @param[in] nTimeoutSec Session timeout in seconds
         *
         *  @return Number of sessions removed
         *
         **/
        uint32_t SweepExpired(uint64_t nTimeoutSec);

        /** Count
         *
         *  Get the total number of registered sessions.
         *
         *  @return Session count
         *
         **/
        size_t Count() const;

        /** CountLive
         *
         *  Get the number of sessions with at least one live connection.
         *
         *  @return Live session count
         *
         **/
        size_t CountLive() const;

        /** EnforceCacheLimit
         *
         *  Enforce maximum capacity on the registry to prevent unbounded growth
         *  from auth floods (BUG-5 fix).  Evicts the oldest expired sessions
         *  first, then the oldest disconnected sessions if still over limit.
         *
         *  @param[in] nMaxSize Maximum allowed entries (default: 1000)
         *
         *  @return Number of entries evicted
         *
         **/
        uint32_t EnforceCacheLimit(size_t nMaxSize = DEFAULT_MAX_REGISTRY_SIZE);

        /** Clear
         *
         *  Remove all sessions (for testing).
         *
         **/
        void Clear();

    private:
        /** Private constructor for singleton **/
        NodeSessionRegistry();

        /** Primary storage: hashKeyID → NodeSessionEntry **/
        util::ConcurrentHashMap<uint256_t, NodeSessionEntry, Uint256Hash> m_mapByKey;

        /** Reverse lookup: nSessionId → hashKeyID **/
        util::ConcurrentHashMap<uint32_t, uint256_t> m_mapSessionToKey;
    };

} // namespace LLP

#endif
