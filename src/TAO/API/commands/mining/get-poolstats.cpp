/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/mining.h>

#include <LLP/include/global.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/pool_discovery.h>

#include <Util/include/config.h>
#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns current pool statistics for this node (if running as a pool). */
    encoding::json Mining::GetPoolStats(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || jParams.size() != 0)
            return std::string(
                "get/poolstats"
                "\n\nReturns current pool statistics for this node (if running as a pool).\n"
                "\n\nReturns:\n"
                "{\n"
                "  \"enabled\": true,\n"
                "  \"genesis\": \"8abc123def...\",\n"
                "  \"endpoint\": \"123.45.67.89:9549\",\n"
                "  \"fee\": 2,\n"
                "  \"maxminers\": 500,\n"
                "  \"connectedminers\": 150,\n"
                "  \"totalhashrate\": 1000000000,\n"
                "  \"blocksfound\": 1234,\n"
                "  \"uptime\": 0.995\n"
                "}\n"
            );

        /* Build response object */
        encoding::json jsonRet;

        /* Check if pool is enabled */
        bool fPoolEnabled = config::GetBoolArg("-mining.pool.enabled", false);
        jsonRet["enabled"] = fPoolEnabled;

        if(!fPoolEnabled)
        {
            /* Return minimal response if pool is not enabled */
            return jsonRet;
        }

        /* Get pool configuration */
        std::string strGenesis = config::GetArg("-mining.pool.genesis", "");
        jsonRet["genesis"] = strGenesis;

        /* Detect or get configured endpoint */
        std::string strEndpoint = LLP::PoolDiscovery::DetectPublicEndpoint();
        jsonRet["endpoint"] = strEndpoint;

        /* Get pool settings */
        uint8_t nFee = static_cast<uint8_t>(config::GetArg("-mining.pool.fee", 2));
        jsonRet["fee"] = static_cast<int>(nFee);

        uint16_t nMaxMiners = static_cast<uint16_t>(config::GetArg("-mining.pool.maxminers", 500));
        jsonRet["maxminers"] = static_cast<int>(nMaxMiners);

        std::string strPoolName = config::GetArg("-mining.pool.name", "Nexus Mining Pool");
        jsonRet["name"] = strPoolName;

        /* Get live statistics from StatelessMinerManager */
        LLP::StatelessMinerManager& manager = LLP::StatelessMinerManager::Get();
        
        uint32_t nConnectedMiners = manager.GetMinerCount();
        jsonRet["connectedminers"] = static_cast<int>(nConnectedMiners);

        uint32_t nAuthenticatedMiners = manager.GetAuthenticatedCount();
        jsonRet["authenticatedminers"] = static_cast<int>(nAuthenticatedMiners);

        uint32_t nRewardBoundMiners = manager.GetRewardBoundCount();
        jsonRet["rewardboundminers"] = static_cast<int>(nRewardBoundMiners);

        /* TODO: Add additional statistics when available:
         * - totalhashrate: aggregate hashrate from all connected miners
         * - blocksfound: number of blocks found by this pool
         * - uptime: pool uptime percentage
         * These require additional tracking in the miner connection handler
         */
        jsonRet["totalhashrate"] = 0;
        jsonRet["blocksfound"] = 0;
        jsonRet["uptime"] = 1.0;

        return jsonRet;
    }
}
