/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/mining.h>

#include <Util/include/config.h>
#include <Util/include/json.h>
#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Disable pool service and stop broadcasting announcements. */
    encoding::json Mining::StopPool(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || jParams.size() != 0)
            return std::string(
                "stop/pool"
                "\n\nDisable pool service and stop broadcasting announcements.\n"
                "\n\nReturns:\n"
                "{\n"
                "  \"success\": true,\n"
                "  \"message\": \"Pool service stopped\"\n"
                "}\n"
            );

        /* Check if pool is currently enabled */
        bool fPoolEnabled = config::GetBoolArg("-mining.pool.enabled", false);
        if(!fPoolEnabled)
        {
            throw Exception(-1, "Pool service is not currently running");
        }

        /* Update configuration to disable pool */
        config::mapArgs["-mining.pool.enabled"] = "0";

        /* TODO: Gracefully disconnect all connected miners
         * This would involve:
         * - Sending disconnect notifications to all miners
         * - Waiting for in-progress work to complete
         * - Cleaning up miner connection state
         * For now, miners will timeout naturally
         */

        /* Build response */
        encoding::json jsonRet;
        jsonRet["success"] = true;
        jsonRet["message"] = "Pool service stopped";

        debug::log(0, FUNCTION, "Pool service stopped");

        return jsonRet;
    }
}
