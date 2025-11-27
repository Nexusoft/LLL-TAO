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
#include <Util/include/runtime.h>
#include <Util/include/debug.h>

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
        /* Update main miner map */
        mapMiners.InsertOrUpdate(strAddress, context);

        /* Update keyID index if authenticated */
        if(context.fAuthenticated && context.hashKeyID != 0)
        {
            mapKeyToAddress.InsertOrUpdate(context.hashKeyID, strAddress);
        }

        /* Update session index if session ID is set */
        if(context.nSessionId != 0)
        {
            mapSessionToAddress.InsertOrUpdate(context.nSessionId, strAddress);
        }
    }

    /* Remove a miner by address */
    bool StatelessMinerManager::RemoveMiner(const std::string& strAddress)
    {
        auto optContext = mapMiners.GetAndRemove(strAddress);
        if(!optContext.has_value())
            return false;

        const MiningContext& ctx = optContext.value();

        /* Remove from indices */
        if(ctx.hashKeyID != 0)
            mapKeyToAddress.Erase(ctx.hashKeyID);

        if(ctx.nSessionId != 0)
            mapSessionToAddress.Erase(ctx.nSessionId);

        return true;
    }

    /* Remove a miner by key ID */
    bool StatelessMinerManager::RemoveMinerByKeyID(const uint256_t& hashKeyID)
    {
        auto optAddress = mapKeyToAddress.GetAndRemove(hashKeyID);
        if(!optAddress.has_value())
            return false;

        return RemoveMiner(optAddress.value());
    }

    /* Get miner context by address */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContext(
        const std::string& strAddress
    ) const
    {
        return mapMiners.Get(strAddress);
    }

    /* Get miner context by key ID */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextByKeyID(
        const uint256_t& hashKeyID
    ) const
    {
        auto optAddress = mapKeyToAddress.Get(hashKeyID);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }

    /* Get miner context by session ID */
    std::optional<MiningContext> StatelessMinerManager::GetMinerContextBySessionID(
        uint32_t nSessionId
    ) const
    {
        auto optAddress = mapSessionToAddress.Get(nSessionId);
        if(!optAddress.has_value())
            return std::nullopt;

        return mapMiners.Get(optAddress.value());
    }

    /* List all miners */
    std::vector<MiningContext> StatelessMinerManager::ListMiners() const
    {
        return mapMiners.GetAll();
    }

    /* Get miner count */
    size_t StatelessMinerManager::GetMinerCount() const
    {
        return mapMiners.Size();
    }

    /* Get authenticated miner count */
    size_t StatelessMinerManager::GetAuthenticatedCount() const
    {
        size_t nCount = 0;
        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
            if(ctx.fAuthenticated)
                ++nCount;
        }
        return nCount;
    }

    /* Cleanup inactive miners */
    uint32_t StatelessMinerManager::CleanupInactive(uint64_t nTimeoutSec)
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();

        auto pairs = mapMiners.GetAllPairs();
        for(const auto& pair : pairs)
        {
            if((nNow - pair.second.nTimestamp) > nTimeoutSec)
            {
                if(RemoveMiner(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " inactive miners");
        }

        return nRemoved;
    }

    /* Get miner status as JSON */
    std::string StatelessMinerManager::GetMinerStatus(const std::string& strAddress) const
    {
        auto optContext = mapMiners.Get(strAddress);
        if(!optContext.has_value())
            return "{\"error\": \"Miner not found\"}";

        const MiningContext& ctx = optContext.value();

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
        encoding::json miners = encoding::json::array();

        auto vMiners = mapMiners.GetAll();
        for(const auto& ctx : vMiners)
        {
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
