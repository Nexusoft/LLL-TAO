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

#include <Util/include/config.h>
#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns a list of currently connected miners (if running as a pool). */
    encoding::json Mining::GetConnectedMiners(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || jParams.size() != 0)
            return std::string(
                "get/connectedminers"
                "\n\nReturns a list of currently connected miners (if running as a pool).\n"
                "\n\nReturns:\n"
                "[\n"
                "  {\n"
                "    \"genesis\": \"8abc123def...\",\n"
                "    \"rewardaddress\": \"8def456abc...\",\n"
                "    \"hashrate\": 1000000,\n"
                "    \"authenticated\": true,\n"
                "    \"rewardbound\": true,\n"
                "    \"connectedtime\": 3600,\n"
                "    \"lastactivity\": 1734529175\n"
                "  }\n"
                "]\n"
            );

        /* Check if pool is enabled */
        bool fPoolEnabled = config::GetBoolArg("-mining.pool.enabled", false);
        if(!fPoolEnabled)
        {
            throw Exception(-1, "Pool service is not currently running");
        }

        /* Get miner list from StatelessMinerManager */
        LLP::StatelessMinerManager& manager = LLP::StatelessMinerManager::Get();
        std::vector<LLP::MinerInfo> vMiners = manager.GetMinerList();

        /* Build JSON array */
        encoding::json jsonRet = encoding::json::array();

        for(const auto& miner : vMiners)
        {
            encoding::json jsonMiner;
            jsonMiner["genesis"] = miner.hashGenesis.ToString();
            jsonMiner["rewardaddress"] = miner.hashRewardAddress.ToString();
            jsonMiner["authenticated"] = miner.fAuthenticated;
            jsonMiner["rewardbound"] = miner.fRewardBound;
            jsonMiner["sessionid"] = static_cast<int>(miner.nSessionId);
            
            /* TODO: Add additional miner metrics when tracking is implemented:
             * - hashrate: miner's current hashrate
             * - sharessubmitted: number of shares submitted
             * - sharesaccepted: number of shares accepted
             * - blocksfound: number of blocks found by this miner
             * - connectedtime: how long miner has been connected
             * - lastactivity: timestamp of last miner activity
             * These require additional tracking in the miner connection handler
             */
            jsonMiner["hashrate"] = 0;
            jsonMiner["sharessubmitted"] = 0;
            jsonMiner["sharesaccepted"] = 0;
            jsonMiner["blocksfound"] = 0;
            jsonMiner["connectedtime"] = 0;
            jsonMiner["lastactivity"] = 0;

            jsonRet.push_back(jsonMiner);
        }

        return jsonRet;
    }
}
