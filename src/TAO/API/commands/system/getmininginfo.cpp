/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/system.h>

#include <LLP/include/global.h>
#include <LLP/types/miner.h>
#include <LLP/include/stateless_manager.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>

#include <Util/include/config.h>
#include <Util/include/json.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* GetMiningInfo
     *
     * Returns mining-related information including:
     * - enabled: whether mining is currently enabled
     * - channel: current mining channel (if applicable)
     * - height: current chain height
     * - connections: network connection count
     * 
     * This is a minimal implementation that can be expanded with additional metrics.
     */
    encoding::json System::GetMiningInfo(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 0)
            return std::string(
                "getmininginfo - Returns an object containing mining-related information.");

        /* Build response object. */
        encoding::json jsonRet;

        /* Check if mining is enabled via -mining flag or MINING_SERVER active */
        bool fMiningEnabled = config::GetBoolArg("-mining", false);
        jsonRet["enabled"] = fMiningEnabled;

        /* Get current best height */
        jsonRet["height"] = static_cast<int32_t>(TAO::Ledger::ChainState::nBestHeight.load());

        /* Get number of network connections */
        uint16_t nConnections = 0;
        if(LLP::TRITIUM_SERVER)
            nConnections = LLP::TRITIUM_SERVER->GetConnectionCount();
        jsonRet["connections"] = static_cast<int32_t>(nConnections);

        /* Channel is typically set per-connection, so we report 0 here
         * unless there's a global default configured */
        jsonRet["channel"] = 0;

        /* Get mining statistics from StatelessMinerManager */
        LLP::StatelessMinerManager& manager = LLP::StatelessMinerManager::Get();
        jsonRet["active_miners"] = static_cast<int32_t>(manager.GetMinerCount());
        jsonRet["authenticated_miners"] = static_cast<int32_t>(manager.GetAuthenticatedCount());
        jsonRet["miners_with_reward_bound"] = static_cast<int32_t>(manager.GetRewardBoundCount());

        /* If mining server exists and is running, get additional info */
        if(LLP::MINING_SERVER && fMiningEnabled)
        {
            /* Future enhancement: could add metrics like:
             * - Number of connected miners
             * - Current difficulty
             * - Recent block submissions
             * For now, keep it minimal as per requirements */
        }

        return jsonRet;
    }
}
