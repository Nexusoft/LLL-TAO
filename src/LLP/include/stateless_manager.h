/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_STATELESS_MANAGER_H
#define NEXUS_LLP_INCLUDE_STATELESS_MANAGER_H

#include <LLP/include/stateless_miner.h>
#include <Util/templates/concurrent_hashmap.h>

#include <string>
#include <vector>
#include <optional>
#include <atomic>

namespace LLP
{
    /** StatelessMinerManager
     *
     *  Manages active stateless miner connections.
     *  Keeps track of miner contexts and provides RPC query interface.
     *
     *  Scalability Features:
     *  - Thread-safe concurrent hash map for parallel miner access
     *  - Support for multiple Falcon sessions with secure isolation
     *  - Session indexing by both address and keyID for efficient lookups
     *  - Atomic counters for lock-free statistics
     *
     *  Concurrency Refinements:
     *  - Atomic session counter for real-time monitoring
     *  - Lock-free read operations where possible
     *  - Fine-grained updates without global locks
     *
     **/
    class StatelessMinerManager
    {
    public:
        /** Get
         *
         *  Get the global manager instance.
         *
         *  @return Reference to the singleton manager
         *
         **/
        static StatelessMinerManager& Get();

        /** UpdateMiner
         *
         *  Update or add a miner context.
         *
         *  @param[in] strAddress Miner address
         *  @param[in] context Updated context
         *
         **/
        void UpdateMiner(const std::string& strAddress, const MiningContext& context);

        /** RemoveMiner
         *
         *  Remove a miner from tracking.
         *
         *  @param[in] strAddress Miner address
         *
         *  @return true if miner was found and removed
         *
         **/
        bool RemoveMiner(const std::string& strAddress);

        /** RemoveMinerByKeyID
         *
         *  Remove a miner from tracking by Falcon key ID.
         *
         *  @param[in] hashKeyID Falcon key identifier
         *
         *  @return true if miner was found and removed
         *
         **/
        bool RemoveMinerByKeyID(const uint256_t& hashKeyID);

        /** GetMinerContext
         *
         *  Retrieve context for a specific miner.
         *
         *  @param[in] strAddress Miner address
         *
         *  @return Context if found, nullopt otherwise
         *
         **/
        std::optional<MiningContext> GetMinerContext(const std::string& strAddress) const;

        /** GetMinerContextByKeyID
         *
         *  Retrieve context by Falcon key ID.
         *
         *  @param[in] hashKeyID Falcon key identifier
         *
         *  @return Context if found, nullopt otherwise
         *
         **/
        std::optional<MiningContext> GetMinerContextByKeyID(const uint256_t& hashKeyID) const;

        /** GetMinerContextBySessionID
         *
         *  Retrieve context by session ID.
         *
         *  @param[in] nSessionId Session identifier
         *
         *  @return Context if found, nullopt otherwise
         *
         **/
        std::optional<MiningContext> GetMinerContextBySessionID(uint32_t nSessionId) const;

        /** GetMinerContextByGenesis
         *
         *  Retrieve context by genesis hash (payout address).
         *  Useful for GenesisHash reward mapping lookups.
         *
         *  @param[in] hashGenesis Tritium genesis hash
         *
         *  @return Context if found, nullopt otherwise
         *
         **/
        std::optional<MiningContext> GetMinerContextByGenesis(const uint256_t& hashGenesis) const;

        /** ListMiners
         *
         *  List all active miners.
         *
         *  @return Vector of all miner contexts
         *
         **/
        std::vector<MiningContext> ListMiners() const;

        /** ListMinersByGenesis
         *
         *  List all miners mining for a specific genesis hash.
         *  Supports GenesisHash reward mapping queries.
         *
         *  @param[in] hashGenesis Tritium genesis hash
         *
         *  @return Vector of matching miner contexts
         *
         **/
        std::vector<MiningContext> ListMinersByGenesis(const uint256_t& hashGenesis) const;

        /** GetMinerStatus
         *
         *  Get status for a specific miner in JSON format.
         *
         *  @param[in] strAddress Miner address
         *
         *  @return JSON string with miner status, or error
         *
         **/
        std::string GetMinerStatus(const std::string& strAddress) const;

        /** GetAllMinersStatus
         *
         *  Get status for all miners in JSON format.
         *
         *  @return JSON string array with all miner statuses
         *
         **/
        std::string GetAllMinersStatus() const;

        /** GetMinerCount
         *
         *  Get the number of active miners.
         *  Uses atomic counter for lock-free access.
         *
         *  @return Number of miners
         *
         **/
        size_t GetMinerCount() const;

        /** GetAuthenticatedCount
         *
         *  Get the number of authenticated miners.
         *  Uses atomic counter for lock-free access.
         *
         *  @return Number of authenticated miners
         *
         **/
        size_t GetAuthenticatedCount() const;

        /** GetActiveSessionCount
         *
         *  Get the number of active sessions (started and not expired).
         *
         *  @return Number of active sessions
         *
         **/
        size_t GetActiveSessionCount() const;

        /** GetTotalKeepalives
         *
         *  Get total number of keepalives processed across all sessions.
         *  Uses atomic counter for lock-free access.
         *
         *  @return Total keepalive count
         *
         **/
        uint64_t GetTotalKeepalives() const;

        /** GetPeakSessionCount
         *
         *  Get the peak concurrent session count (high water mark).
         *
         *  @return Peak session count
         *
         **/
        size_t GetPeakSessionCount() const;

        /** CleanupInactive
         *
         *  Remove miners that have been inactive for too long.
         *
         *  @param[in] nTimeoutSec Inactivity timeout in seconds
         *
         *  @return Number of miners removed
         *
         **/
        uint32_t CleanupInactive(uint64_t nTimeoutSec = 300);

        /** CleanupExpiredSessions
         *
         *  Remove miners with expired sessions based on their session timeout.
         *
         *  @return Number of miners removed
         *
         **/
        uint32_t CleanupExpiredSessions();

    private:
        /** Private constructor for singleton **/
        StatelessMinerManager() = default;

        /** Thread-safe concurrent hash map of miner address to context **/
        util::ConcurrentHashMap<std::string, MiningContext> mapMiners;

        /** Index by Falcon key ID for efficient lookups **/
        util::ConcurrentHashMap<uint256_t, std::string> mapKeyToAddress;

        /** Index by session ID for efficient lookups **/
        util::ConcurrentHashMap<uint32_t, std::string> mapSessionToAddress;

        /** Index by genesis hash for GenesisHash reward mapping **/
        util::ConcurrentHashMap<uint256_t, std::string> mapGenesisToAddress;

        /** Atomic counter for total miners (lock-free stats) **/
        mutable std::atomic<size_t> nTotalMiners{0};

        /** Atomic counter for authenticated miners **/
        mutable std::atomic<size_t> nAuthenticatedMiners{0};

        /** Atomic counter for total keepalives processed **/
        mutable std::atomic<uint64_t> nTotalKeepalives{0};

        /** Atomic peak session count (high water mark) **/
        mutable std::atomic<size_t> nPeakSessions{0};
    };

} // namespace LLP

#endif
