/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/types/rpcapi.h>
#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

namespace TAO::API::RPC
{

    void RPCAPI::Initialize()
    {
        // register the commands for this API with the global function pointer map
        mapFunctions["testfunc"] = RPCMethod( std::bind(&RPCAPI::TestFunc, this, std::placeholders::_1, std::placeholders::_2), true);
    }

    /** Handler for this API, conforms to the JSON-RPC spec **/
    nlohmann::json RPCAPI::HandleJSONAPIMethod(std::string strMethod, nlohmann::json jsonParameters)
    {
        if(!fInitialized)
            Initialize();

        nlohmann::json ret;

        if(mapFunctions.count(strMethod))
        {
            ret = mapFunctions[strMethod].function(false, jsonParameters); //TODO: add help support as param[0]
        }
        else
        {
            ret = { {"result", ""}, {"errors","method not found"} };
        }

        return ret;
    }

    /** API Method implementations **/
    nlohmann::json RPCAPI::TestFunc(bool fHelp, nlohmann::json jsonParameters)
    {
        printf("Test Function!\n");

        nlohmann::json ret;
        ret.push_back("Test!");

        return ret;
    }
}
