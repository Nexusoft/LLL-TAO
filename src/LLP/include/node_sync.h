/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NODE_SYNC_H
#define NEXUS_LLP_INCLUDE_NODE_SYNC_H

#include <LLC/types/uint1024.h>
#include <Util/templates/concurrent_hashmap.h>

#include <string>
#include <vector>
#include <chrono>
#include <atomic>

namespace LLP
{
    /** SessionMetadata
     *
     *  Metadata about a mining session for inter-node synchronization.
     *  This is a lightweight subset of session data for sync purposes.
     *
     **/
    struct SessionMetadata
    {
        uint32_t nSessionId;                    // Session identifier
        uint256_t hashKeyID;                    // Falcon key identifier
        uint256_t hashGenesis;                  // Tritium genesis hash (payout address)
        uint32_t nChannel;                      // Mining channel
        uint64_t nLastActivity;                 // Last activity timestamp
        std::string strNodeAddress;             // Node that owns this session
        bool fAuthenticated;                    // Authentication status

        /** Default Constructor **/
        SessionMetadata();

        /** Serialize
         *
         *  Serialize to byte vector for network transmission.
         *
         *  @return Serialized bytes
         *
         **/
        std::vector<uint8_t> Serialize() const;

        /** Deserialize
         *
         *  Deserialize from byte vector.
         *
         *  @param[in] vData Input bytes
         *
         *  @return true on success
         *
         **/
        bool Deserialize(const std::vector<uint8_t>& vData);
    };


    /** BlockMetadata
     *
     *  Metadata about a block being mined for redundancy.
     *
     **/
    struct BlockMetadata
    {
        uint512_t hashMerkleRoot;               // Block merkle root
        uint32_t nHeight;                       // Block height
        uint32_t nChannel;                      // Mining channel
        uint64_t nTimestamp;                    // Block timestamp
        std::string strNodeAddress;             // Node that created block

        /** Default Constructor **/
        BlockMetadata();

        /** Serialize **/
        std::vector<uint8_t> Serialize() const;

        /** Deserialize **/
        bool Deserialize(const std::vector<uint8_t>& vData);
    };


    /** NodeSyncManager
     *
     *  Manages inter-node communication for session and block metadata synchronization.
     *  Provides decentralized fault tolerance by sharing critical state across nodes.
     *
     *  Features:
     *  - Session metadata sync for miner failover
     *  - Block metadata broadcast for redundancy
     *  - Node health monitoring
     *
     **/
    class NodeSyncManager
    {
    public:
        /** Get
         *
         *  Get the global manager instance.
         *
         *  @return Reference to singleton manager
         *
         **/
        static NodeSyncManager& Get();

        /** RegisterSession
         *
         *  Register a session for sync to other nodes.
         *
         *  @param[in] meta Session metadata
         *
         *  @return true if registered
         *
         **/
        bool RegisterSession(const SessionMetadata& meta);

        /** UnregisterSession
         *
         *  Remove a session from sync.
         *
         *  @param[in] hashKeyID Session key identifier
         *
         *  @return true if removed
         *
         **/
        bool UnregisterSession(const uint256_t& hashKeyID);

        /** GetSession
         *
         *  Get session metadata by key ID.
         *
         *  @param[in] hashKeyID Session key identifier
         *
         *  @return Session metadata if found
         *
         **/
        std::optional<SessionMetadata> GetSession(const uint256_t& hashKeyID) const;

        /** GetSessionsByNode
         *
         *  Get all sessions from a specific node.
         *
         *  @param[in] strNodeAddress Node address
         *
         *  @return Vector of session metadata
         *
         **/
        std::vector<SessionMetadata> GetSessionsByNode(const std::string& strNodeAddress) const;

        /** RegisterBlock
         *
         *  Register a block being mined for redundancy.
         *
         *  @param[in] meta Block metadata
         *
         *  @return true if registered
         *
         **/
        bool RegisterBlock(const BlockMetadata& meta);

        /** UnregisterBlock
         *
         *  Remove a block from sync.
         *
         *  @param[in] hashMerkleRoot Block merkle root
         *
         *  @return true if removed
         *
         **/
        bool UnregisterBlock(const uint512_t& hashMerkleRoot);

        /** GetBlock
         *
         *  Get block metadata by merkle root.
         *
         *  @param[in] hashMerkleRoot Block merkle root
         *
         *  @return Block metadata if found
         *
         **/
        std::optional<BlockMetadata> GetBlock(const uint512_t& hashMerkleRoot) const;

        /** BroadcastSession
         *
         *  Broadcast session metadata to peer nodes.
         *  NOTE: This is a placeholder for future P2P integration.
         *
         *  @param[in] meta Session to broadcast
         *
         *  @return true if broadcast initiated
         *
         **/
        bool BroadcastSession(const SessionMetadata& meta);

        /** BroadcastBlock
         *
         *  Broadcast block metadata to peer nodes.
         *  NOTE: This is a placeholder for future P2P integration.
         *
         *  @param[in] meta Block to broadcast
         *
         *  @return true if broadcast initiated
         *
         **/
        bool BroadcastBlock(const BlockMetadata& meta);

        /** OnNodeFailure
         *
         *  Handle notification of a peer node failure.
         *  Can be used to take over sessions from failed node.
         *
         *  @param[in] strNodeAddress Failed node address
         *
         **/
        void OnNodeFailure(const std::string& strNodeAddress);

        /** GetSessionCount
         *
         *  Get total number of tracked sessions.
         *
         *  @return Session count
         *
         **/
        size_t GetSessionCount() const;

        /** GetBlockCount
         *
         *  Get total number of tracked blocks.
         *
         *  @return Block count
         *
         **/
        size_t GetBlockCount() const;

        /** CleanupStale
         *
         *  Remove stale sessions and blocks.
         *
         *  @param[in] nSessionTimeoutSec Session timeout
         *  @param[in] nBlockTimeoutSec Block timeout
         *
         *  @return Number of entries removed
         *
         **/
        uint32_t CleanupStale(uint64_t nSessionTimeoutSec = 3600, uint64_t nBlockTimeoutSec = 600);

        /** SetLocalNodeAddress
         *
         *  Set the local node's address for sync identification.
         *
         *  @param[in] strAddress Local node address
         *
         **/
        void SetLocalNodeAddress(const std::string& strAddress);

        /** GetLocalNodeAddress
         *
         *  Get the local node's address.
         *
         *  @return Local node address
         *
         **/
        std::string GetLocalNodeAddress() const;

    private:
        /** Private constructor for singleton **/
        NodeSyncManager();

        /** Session storage **/
        util::ConcurrentHashMap<uint256_t, SessionMetadata> mapSessions;

        /** Block storage **/
        util::ConcurrentHashMap<uint512_t, BlockMetadata> mapBlocks;

        /** Local node address **/
        std::string strLocalAddress;

        /** Mutex for local address **/
        mutable std::mutex ADDRESS_MUTEX;
    };


    /** Hash function for uint512_t **/
    struct Uint512Hash
    {
        size_t operator()(const uint512_t& key) const
        {
            return static_cast<size_t>(key.Get64(0) ^ key.Get64(1));
        }
    };

} // namespace LLP

#endif
