/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_manager.h>

#include <Util/include/json.h>
#include <Util/include/string.h>
#include <Util/include/mutex.h>

#include <optional>
#include <sstream>

namespace LLP
{
    /* Get singleton instance */
    StatelessMinerManager& StatelessMinerManager::Get()
    {
        static StatelessMinerManager instance;
        return instance;
    }

    /* Update or add a miner */
    void StatelessMinerManager::UpdateMiner(
        const std::string& strAddress,
        const MiningContext& context
    )
    {
        LOCK(MUTEX);
        mapMiners[strAddress] = context;
    }

    /* Remove a miner */
    bool StatelessMinerManager::RemoveMiner(const std::string& strAddress)
    {
        LOCK(MUTEX);
        auto it = mapMiners.find(strAddress);
        if(it == mapMiners.end())
            return false;

        mapMiners.erase(it);
        return true;
    }

    /* Get miner context */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContext(
        const std::string& strAddress
    ) const
    {
        LOCK(MUTEX);
        auto it = mapMiners.find(strAddress);
        if(it == mapMiners.end())
            return std::nullopt;

        return it->second;
    }

    /* List all miners */
    std::vector<MiningContext> StatelessMinerManager::ListMiners() const
    {
        LOCK(MUTEX);
        std::vector<MiningContext> vMiners;
        vMiners.reserve(mapMiners.size());

        for(const auto& pair : mapMiners)
            vMiners.push_back(pair.second);

        return vMiners;
    }

    /* Get miner status as JSON */
    std::string StatelessMinerManager::GetMinerStatus(const std::string& strAddress) const
    {
        LOCK(MUTEX);
        auto it = mapMiners.find(strAddress);
        if(it == mapMiners.end())
            return "{\"error\": \"Miner not found\"}";

        const MiningContext& ctx = it->second;

        /* Build JSON response with Phase 2 identity fields */
        encoding::json result;
        result["address"] = ctx.strAddress;
        result["channel"] = ctx.nChannel;
        result["height"] = ctx.nHeight;
        result["authenticated"] = ctx.fAuthenticated;
        result["session_id"] = ctx.nSessionId;
        result["protocol_version"] = ctx.nProtocolVersion;
        result["last_seen"] = ctx.nTimestamp;
        result["key_id"] = ctx.hashKeyID.ToString();
        result["genesis"] = ctx.hashGenesis.ToString();

        return result.dump(4);
    }

    /* Get all miners status as JSON */
    std::string StatelessMinerManager::GetAllMinersStatus() const
    {
        LOCK(MUTEX);
        
        encoding::json miners = encoding::json::array();

        for(const auto& pair : mapMiners)
        {
            const MiningContext& ctx = pair.second;
            
            encoding::json miner;
            miner["address"] = ctx.strAddress;
            miner["channel"] = ctx.nChannel;
            miner["height"] = ctx.nHeight;
            miner["authenticated"] = ctx.fAuthenticated;
            miner["session_id"] = ctx.nSessionId;
            miner["protocol_version"] = ctx.nProtocolVersion;
            miner["last_seen"] = ctx.nTimestamp;
            miner["key_id"] = ctx.hashKeyID.ToString();
            miner["genesis"] = ctx.hashGenesis.ToString();
            
            miners.push_back(miner);
        }

        return miners.dump(4);
    }

} // namespace LLP
