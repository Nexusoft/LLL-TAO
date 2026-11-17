/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/system.h>
#include <TAO/API/include/json.h>

#include <LLP/include/global.h>
#include <LLP/include/mining_config.h>
#include <LLP/types/miner.h>

#include <Util/include/config.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/json.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* SetMining
     *
     * Enable or disable mining via RPC.
     * 
     * When enabling, validates that required configuration is present.
     * When disabling, stops the mining server.
     * 
     * Signature: setmining <true|false>
     */
    encoding::json System::SetMining(const encoding::json& params, const bool fHelp)
    {
        /* Check for help flag. */
        if(fHelp || params.size() != 1)
            return std::string(
                "setmining <true|false> - Enable or disable mining.\n"
                "\nArguments:\n"
                "1. enable    (boolean, required) true to enable mining, false to disable\n");

        /* Parse the enable flag from parameters */
        bool fEnable = false;
        
        /* Handle different input formats */
        if(params.is_boolean())
        {
            fEnable = params.get<bool>();
        }
        else if(params.is_array() && params.size() > 0)
        {
            if(params[0].is_boolean())
                fEnable = params[0].get<bool>();
            else if(params[0].is_string())
            {
                std::string strValue = params[0].get<std::string>();
                fEnable = (strValue == "true" || strValue == "1");
            }
        }
        else if(params.is_string())
        {
            std::string strValue = params.get<std::string>();
            fEnable = (strValue == "true" || strValue == "1");
        }

        encoding::json jsonRet;

        if(fEnable)
        {
            /* When enabling mining, validate configuration using auto-config helper */
            
            /* Check if mining is already enabled */
            if(config::GetBoolArg("-mining", false))
            {
                jsonRet["enabled"] = true;
                jsonRet["message"] = "Mining is already enabled";
                return jsonRet;
            }

            /* Use LoadMiningConfig to validate and auto-configure mining */
            if(!LLP::LoadMiningConfig())
            {
                throw Exception(-1, "Mining configuration validation failed. "
                                    "See debug log for details. Ensure miningpubkey is set in nexus.conf");
            }

            /* Set the mining flag */
            config::mapArgs["-mining"] = "1";
            
            /* If MINING_SERVER exists and is not running, start it */
            if(LLP::MINING_SERVER)
            {
                /* Server already exists, just ensure it's listening */
                debug::log(0, FUNCTION, "Enabling mining server");
                LLP::OpenListening(LLP::MINING_SERVER);
            }
            else
            {
                /* Note: Starting a new MINING_SERVER instance would require more infrastructure
                 * that's typically handled at daemon startup. For now, we just set the flag
                 * and log that the server should be started. */
                debug::log(0, FUNCTION, "Mining enabled - MINING_SERVER should be initialized at next startup");
            }

            jsonRet["enabled"] = true;
            jsonRet["message"] = "Mining enabled";
        }
        else
        {
            /* Disabling mining */
            
            /* Clear the mining flag */
            config::mapArgs["-mining"] = "0";
            
            /* If MINING_SERVER exists, close its listening socket */
            if(LLP::MINING_SERVER)
            {
                debug::log(0, FUNCTION, "Disabling mining server");
                LLP::CloseSockets(LLP::MINING_SERVER);
            }

            jsonRet["enabled"] = false;
            jsonRet["message"] = "Mining disabled";
        }

        return jsonRet;
    }
}
