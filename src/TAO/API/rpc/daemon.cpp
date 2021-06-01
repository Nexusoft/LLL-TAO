/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/rpc/types/rpc.h>

#include <LLP/include/global.h>
#include <LLP/include/hosts.h>
#include <LLP/include/inv.h>
#include <LLP/include/port.h>

#include <Util/include/json.h>
#include <Util/include/signals.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* stop"
        *  Stop Nexus server */
        encoding::json RPC::Stop(const encoding::json& params, const bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string("stop - Stop Nexus server.");
            // Shutdown will take long enough that the response should get back
            Shutdown();
            return "Nexus server stopping";
        }

        /* getconnectioncount
           Returns the number of connections to other nodes */
        encoding::json RPC::GetConnectionCount(const encoding::json& params, const bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string(
                    "getconnectioncount"
                    " - Returns the number of connections to other nodes.");

            return GetTotalConnectionCount();
        }

        /* Restart all node connections */
        encoding::json RPC::Reset(const encoding::json& params, const bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string(
                    "reset - Restart all node connections");

            // read in any config file changes
            config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);

            if(LLP::TRITIUM_SERVER)
            {
                LLP::TRITIUM_SERVER->DisconnectAll();

                /* Add connections and resolve potential DNS lookups. */
                for(const auto& address : config::mapMultiArgs["-connect"])
                {
                    /* Flag indicating connection was successful */
                    bool fConnected = false;
                    
                    /* First attempt SSL if configured */
                    if(LLP::TRITIUM_SERVER->SSLEnabled())
                    fConnected = LLP::TRITIUM_SERVER->AddConnection(address, LLP::TRITIUM_SERVER->GetPort(true), true, true);

                    /* If SSL connection failed or was not attempted and SSL is not required, attempt on the non-SSL port */
                    if(!fConnected && !LLP::TRITIUM_SERVER->SSLRequired())
                        fConnected = LLP::TRITIUM_SERVER->AddConnection(address, LLP::TRITIUM_SERVER->GetPort(false), false, true);
                }

                for(const auto& node : config::mapMultiArgs["-addnode"])
                    LLP::TRITIUM_SERVER->AddNode(node);

            }

            return "success";
        }
    }
}
