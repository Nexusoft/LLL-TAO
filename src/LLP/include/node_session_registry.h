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
#include <Util/templates/concurrent_hashmap.h>
#include <LLC/types/uint1024.h>

#include <cstdint>
#include <string>
#include <optional>
#include <atomic>

namespace LLP
{
    /** Hash function for uint256_t (from session_recovery.h pattern) **/
    struct Uint256Hash
    {
        size_t operator()(const uint256_t& key) const
        {
            /* Combine multiple 64-bit segments for better distribution */
            size_t hash = static_cast<size_t>(key.Get64(0));
            hash ^= static_cast<size_t>(key.Get64(1)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<size_t>(key.Get64(2)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<size_t>(key.Get64(3)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    /** NodeSessionEntry
     *
     *  Unified session state shared across both mining server ports (8323 and 9323).
     *  One Falcon identity (hashKeyID) maps to one canonical MiningContext that any
     *  connection on any port can look up and inherit.
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
     *  - fStatelessLive / fLegacyLive: Track which ports are currently connected
     *  - context: Canonical MiningContext with WithXxx() immutable builders
     *
     *  LIFECYCLE:
     *  ==========
     *  1. First auth on either port: Create entry, derive nSessionId from hashKeyID
     *  2. Second auth on other port: Look up entry, inherit nSessionId
     *  3. Disconnection: Mark fStatelessLive or fLegacyLive as false
     *  4. Expiration: SweepExpired() removes entries with no live connections after timeout
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
    };


    /** NodeSessionRegistry
     *
     *  Singleton registry managing cross-port mining sessions on the node side.
     *  Provides unified session lookup and management across legacy (8323) and
     *  stateless (9323) mining server ports.
     *
     *  DESIGN:
     *  =======
     *  - Two-map structure for O(1) lookup by both hashKeyID and nSessionId
     *  - m_mapByKey: Primary storage indexed by Falcon public key hash
     *  - m_mapSessionToKey: Reverse lookup indexed by session ID
     *
     *  USAGE PATTERN:
     *  ==============
     *  1. Authentication (either port):
     *     auto [sessionId, isNew] = registry.RegisterOrRefresh(hashKeyID, hashGenesis, context, lane);
     *     // Patch wire response DATA[1..4] with sessionId
     *
     *  2. Packet processing (hot path):
     *     auto entry = registry.Lookup(context.nSessionId);
     *     if (!entry) { reject; }
     *     // Use entry->context for processing
     *
     *  3. Disconnection:
     *     registry.MarkDisconnected(hashKeyID, lane);
     *
     *  4. Periodic cleanup:
     *     registry.SweepExpired(timeoutSec);
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
         *  - If hashKeyID exists: Refresh activity, update context, mark port live
         *  - If hashKeyID new: Derive nSessionId from hashKeyID, create entry
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
