/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <LLP/include/inv.h>
#include <LLP/include/hosts.h>
#include <LLP/include/global.h>
#include <Util/include/signals.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* stop"
        *  Stop Nexus server */
        json::json RPC::Stop(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "stop"
                    " - Stop Nexus server.");
            // Shutdown will take long enough that the response should get back
            Shutdown();
            return "Nexus server stopping";
        }

        /* getconnectioncount
           Returns the number of connections to other nodes */
        json::json RPC::GetConnectionCount(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getconnectioncount"
                    " - Returns the number of connections to other nodes.");

            return GetTotalConnectionCount();
        }

        /* Restart all node connections */
        json::json RPC::Reset(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string(
                    "reset"
                    " - Restart all node connections");

            // read in any config file changes
            config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);

            //disconnect all nodes currently active
            if(LLP::LEGACY_SERVER )
            {
                LLP::LEGACY_SERVER->DisconnectAll();

                uint16_t port = static_cast<uint16_t>(config::GetArg(
                    "-port", config::fTestNet ? 8323 : 9323));

                for(const auto& node : config::mapMultiArgs["-connect"])
                    LLP::LEGACY_SERVER->AddConnection(node, port);

                for(const auto& node : config::mapMultiArgs["-addnode"])
                    LLP::LEGACY_SERVER->AddNode(node, port);
            }

            if(LLP::TRITIUM_SERVER)
            {
                LLP::TRITIUM_SERVER->DisconnectAll();

                uint16_t port = static_cast<uint16_t>(config::GetArg(
                    "-port", config::fTestNet ? 8888 : 9888));

                for(const auto& node : config::mapMultiArgs["-connect"])
                    LLP::TRITIUM_SERVER->AddConnection(node, port);

                for(const auto& node : config::mapMultiArgs["-addnode"])
                    LLP::TRITIUM_SERVER->AddNode(node, port);

            }

            return "success";
        }
    }
}
