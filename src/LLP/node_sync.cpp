/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/node_sync.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /***************************************************************************
     * SessionMetadata Implementation
     **************************************************************************/

    /** Default Constructor **/
    SessionMetadata::SessionMetadata()
    : nSessionId(0)
    , hashKeyID(0)
    , hashGenesis(0)
    , nChannel(0)
    , nLastActivity(0)
    , strNodeAddress()
    , fAuthenticated(false)
    {
    }


    /** Serialize **/
    std::vector<uint8_t> SessionMetadata::Serialize() const
    {
        std::vector<uint8_t> vData;

        /* Session ID (4 bytes) */
        for(int i = 0; i < 4; ++i)
            vData.push_back(static_cast<uint8_t>((nSessionId >> (i * 8)) & 0xFF));

        /* Key ID (32 bytes) */
        std::vector<uint8_t> vKeyID = hashKeyID.GetBytes();
        vData.insert(vData.end(), vKeyID.begin(), vKeyID.end());

        /* Genesis (32 bytes) */
        std::vector<uint8_t> vGenesis = hashGenesis.GetBytes();
        vData.insert(vData.end(), vGenesis.begin(), vGenesis.end());

        /* Channel (4 bytes) */
        for(int i = 0; i < 4; ++i)
            vData.push_back(static_cast<uint8_t>((nChannel >> (i * 8)) & 0xFF));

        /* Last activity (8 bytes) */
        for(int i = 0; i < 8; ++i)
            vData.push_back(static_cast<uint8_t>((nLastActivity >> (i * 8)) & 0xFF));

        /* Node address length + address */
        uint16_t nAddrLen = static_cast<uint16_t>(strNodeAddress.size());
        vData.push_back(static_cast<uint8_t>(nAddrLen >> 8));
        vData.push_back(static_cast<uint8_t>(nAddrLen & 0xFF));
        vData.insert(vData.end(), strNodeAddress.begin(), strNodeAddress.end());

        /* Authenticated flag */
        vData.push_back(fAuthenticated ? 0x01 : 0x00);

        return vData;
    }


    /** Deserialize **/
    bool SessionMetadata::Deserialize(const std::vector<uint8_t>& vData)
    {
        /* Minimum size: 4 + 32 + 32 + 4 + 8 + 2 + 1 = 83 bytes */
        if(vData.size() < 83)
            return false;

        size_t nPos = 0;

        /* Session ID */
        nSessionId = 0;
        for(int i = 0; i < 4; ++i)
            nSessionId |= static_cast<uint32_t>(vData[nPos++]) << (i * 8);

        /* Key ID */
        hashKeyID.SetBytes(std::vector<uint8_t>(vData.begin() + nPos, vData.begin() + nPos + 32));
        nPos += 32;

        /* Genesis */
        hashGenesis.SetBytes(std::vector<uint8_t>(vData.begin() + nPos, vData.begin() + nPos + 32));
        nPos += 32;

        /* Channel */
        nChannel = 0;
        for(int i = 0; i < 4; ++i)
            nChannel |= static_cast<uint32_t>(vData[nPos++]) << (i * 8);

        /* Last activity */
        nLastActivity = 0;
        for(int i = 0; i < 8; ++i)
            nLastActivity |= static_cast<uint64_t>(vData[nPos++]) << (i * 8);

        /* Node address */
        uint16_t nAddrLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                            static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        if(nPos + nAddrLen + 1 > vData.size())
            return false;

        strNodeAddress.assign(vData.begin() + nPos, vData.begin() + nPos + nAddrLen);
        nPos += nAddrLen;

        /* Authenticated */
        fAuthenticated = (vData[nPos] == 0x01);

        return true;
    }


    /***************************************************************************
     * BlockMetadata Implementation
     **************************************************************************/

    /** Default Constructor **/
    BlockMetadata::BlockMetadata()
    : hashMerkleRoot(0)
    , nHeight(0)
    , nChannel(0)
    , nTimestamp(0)
    , strNodeAddress()
    {
    }


    /** Serialize **/
    std::vector<uint8_t> BlockMetadata::Serialize() const
    {
        std::vector<uint8_t> vData;

        /* Merkle root (64 bytes) */
        std::vector<uint8_t> vMerkle = hashMerkleRoot.GetBytes();
        vData.insert(vData.end(), vMerkle.begin(), vMerkle.end());

        /* Height (4 bytes) */
        for(int i = 0; i < 4; ++i)
            vData.push_back(static_cast<uint8_t>((nHeight >> (i * 8)) & 0xFF));

        /* Channel (4 bytes) */
        for(int i = 0; i < 4; ++i)
            vData.push_back(static_cast<uint8_t>((nChannel >> (i * 8)) & 0xFF));

        /* Timestamp (8 bytes) */
        for(int i = 0; i < 8; ++i)
            vData.push_back(static_cast<uint8_t>((nTimestamp >> (i * 8)) & 0xFF));

        /* Node address */
        uint16_t nAddrLen = static_cast<uint16_t>(strNodeAddress.size());
        vData.push_back(static_cast<uint8_t>(nAddrLen >> 8));
        vData.push_back(static_cast<uint8_t>(nAddrLen & 0xFF));
        vData.insert(vData.end(), strNodeAddress.begin(), strNodeAddress.end());

        return vData;
    }


    /** Deserialize **/
    bool BlockMetadata::Deserialize(const std::vector<uint8_t>& vData)
    {
        /* Minimum size: 64 + 4 + 4 + 8 + 2 = 82 bytes */
        if(vData.size() < 82)
            return false;

        size_t nPos = 0;

        /* Merkle root */
        hashMerkleRoot.SetBytes(std::vector<uint8_t>(vData.begin() + nPos, vData.begin() + nPos + 64));
        nPos += 64;

        /* Height */
        nHeight = 0;
        for(int i = 0; i < 4; ++i)
            nHeight |= static_cast<uint32_t>(vData[nPos++]) << (i * 8);

        /* Channel */
        nChannel = 0;
        for(int i = 0; i < 4; ++i)
            nChannel |= static_cast<uint32_t>(vData[nPos++]) << (i * 8);

        /* Timestamp */
        nTimestamp = 0;
        for(int i = 0; i < 8; ++i)
            nTimestamp |= static_cast<uint64_t>(vData[nPos++]) << (i * 8);

        /* Node address */
        uint16_t nAddrLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                            static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        if(nPos + nAddrLen > vData.size())
            return false;

        strNodeAddress.assign(vData.begin() + nPos, vData.begin() + nPos + nAddrLen);

        return true;
    }


    /***************************************************************************
     * NodeSyncManager Implementation
     **************************************************************************/

    /** Private constructor **/
    NodeSyncManager::NodeSyncManager()
    : mapSessions()
    , mapBlocks()
    , strLocalAddress()
    , ADDRESS_MUTEX()
    {
    }


    /** Get singleton **/
    NodeSyncManager& NodeSyncManager::Get()
    {
        static NodeSyncManager instance;
        return instance;
    }


    /** RegisterSession **/
    bool NodeSyncManager::RegisterSession(const SessionMetadata& meta)
    {
        if(meta.hashKeyID == 0)
            return false;

        mapSessions.InsertOrUpdate(meta.hashKeyID, meta);

        debug::log(3, FUNCTION, "Registered session keyID=", meta.hashKeyID.SubString(),
                   " from node=", meta.strNodeAddress);

        return true;
    }


    /** UnregisterSession **/
    bool NodeSyncManager::UnregisterSession(const uint256_t& hashKeyID)
    {
        return mapSessions.Erase(hashKeyID);
    }


    /** GetSession **/
    std::optional<SessionMetadata> NodeSyncManager::GetSession(const uint256_t& hashKeyID) const
    {
        return mapSessions.Get(hashKeyID);
    }


    /** GetSessionsByNode **/
    std::vector<SessionMetadata> NodeSyncManager::GetSessionsByNode(const std::string& strNodeAddress) const
    {
        std::vector<SessionMetadata> vSessions;
        auto allSessions = mapSessions.GetAll();

        for(const auto& session : allSessions)
        {
            if(session.strNodeAddress == strNodeAddress)
                vSessions.push_back(session);
        }

        return vSessions;
    }


    /** RegisterBlock **/
    bool NodeSyncManager::RegisterBlock(const BlockMetadata& meta)
    {
        if(meta.hashMerkleRoot == 0)
            return false;

        mapBlocks.InsertOrUpdate(meta.hashMerkleRoot, meta);

        debug::log(3, FUNCTION, "Registered block merkle=", meta.hashMerkleRoot.SubString(),
                   " height=", meta.nHeight, " from node=", meta.strNodeAddress);

        return true;
    }


    /** UnregisterBlock **/
    bool NodeSyncManager::UnregisterBlock(const uint512_t& hashMerkleRoot)
    {
        return mapBlocks.Erase(hashMerkleRoot);
    }


    /** GetBlock **/
    std::optional<BlockMetadata> NodeSyncManager::GetBlock(const uint512_t& hashMerkleRoot) const
    {
        return mapBlocks.Get(hashMerkleRoot);
    }


    /** BroadcastSession **/
    bool NodeSyncManager::BroadcastSession(const SessionMetadata& meta)
    {
        /* PLACEHOLDER: P2P broadcast is not yet implemented.
         * This function currently only stores metadata locally.
         * Full P2P integration will use the existing Tritium network
         * to broadcast session metadata to peer nodes for redundancy.
         *
         * For now, session data is stored locally and can be queried
         * by the session recovery manager for local failover support.
         */
        debug::log(2, FUNCTION, "Session broadcast (local only) for keyID=", meta.hashKeyID.SubString(),
                   " - P2P sync not yet implemented");
        return RegisterSession(meta);
    }


    /** BroadcastBlock **/
    bool NodeSyncManager::BroadcastBlock(const BlockMetadata& meta)
    {
        /* PLACEHOLDER: P2P broadcast is not yet implemented.
         * Block metadata is stored locally for coordination.
         * Full P2P integration will broadcast to peer nodes.
         */
        debug::log(2, FUNCTION, "Block broadcast (local only) for merkle=", meta.hashMerkleRoot.SubString(),
                   " - P2P sync not yet implemented");
        return RegisterBlock(meta);
    }


    /** OnNodeFailure **/
    void NodeSyncManager::OnNodeFailure(const std::string& strNodeAddress)
    {
        debug::log(0, FUNCTION, "Node failure detected: ", strNodeAddress);

        /* Get all sessions from failed node */
        auto vSessions = GetSessionsByNode(strNodeAddress);

        for(const auto& session : vSessions)
        {
            /* Sessions from failed nodes can be taken over by local node if needed.
             * For now, we just log them. In production, this would trigger
             * a session migration procedure. */
            debug::log(1, FUNCTION, "Orphaned session: keyID=", session.hashKeyID.SubString(),
                       " genesis=", session.hashGenesis.SubString());
        }
    }


    /** GetSessionCount **/
    size_t NodeSyncManager::GetSessionCount() const
    {
        return mapSessions.Size();
    }


    /** GetBlockCount **/
    size_t NodeSyncManager::GetBlockCount() const
    {
        return mapBlocks.Size();
    }


    /** CleanupStale **/
    uint32_t NodeSyncManager::CleanupStale(uint64_t nSessionTimeoutSec, uint64_t nBlockTimeoutSec)
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        /* Cleanup sessions */
        auto sessions = mapSessions.GetAllPairs();
        for(const auto& pair : sessions)
        {
            if((nNow - pair.second.nLastActivity) > nSessionTimeoutSec)
            {
                mapSessions.Erase(pair.first);
                ++nRemoved;
            }
        }

        /* Cleanup blocks */
        auto blocks = mapBlocks.GetAllPairs();
        for(const auto& pair : blocks)
        {
            if((nNow - pair.second.nTimestamp) > nBlockTimeoutSec)
            {
                mapBlocks.Erase(pair.first);
                ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " stale sync entries");
        }

        return nRemoved;
    }


    /** SetLocalNodeAddress **/
    void NodeSyncManager::SetLocalNodeAddress(const std::string& strAddress)
    {
        std::lock_guard<std::mutex> lock(ADDRESS_MUTEX);
        strLocalAddress = strAddress;
    }


    /** GetLocalNodeAddress **/
    std::string NodeSyncManager::GetLocalNodeAddress() const
    {
        std::lock_guard<std::mutex> lock(ADDRESS_MUTEX);
        return strLocalAddress;
    }

} // namespace LLP
