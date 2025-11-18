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

#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <optional>

namespace LLP
{
    /** StatelessMinerManager
     *
     *  Manages active stateless miner connections.
     *  Keeps track of miner contexts and provides RPC query interface.
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

        /** ListMiners
         *
         *  List all active miners.
         *
         *  @return Vector of all miner contexts
         *
         **/
        std::vector<MiningContext> ListMiners() const;

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

    private:
        /** Private constructor for singleton **/
        StatelessMinerManager() = default;

        /** Map of miner address to context **/
        std::map<std::string, MiningContext> mapMiners;

        /** Mutex for thread-safe access **/
        mutable std::mutex MUTEX;
    };

} // namespace LLP

#endif
