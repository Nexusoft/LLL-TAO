/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

namespace TAO::API
{

    /* Create the list of commands. */
    TAO::API::RPC* RPCCommands;

    void RPC::Initialize()
    {
        mapFunctions["echo"] = Function(std::bind(&RPC::Echo, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getinfo"] = Function(std::bind(&RPC::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
    }

    json::json RPC::Echo(bool fHelp, json::json jsonParams)
    {
        printf("Echo Function!\n");

        json::json ret;
        ret["echo"] = jsonParams;

        return ret;

    }

}