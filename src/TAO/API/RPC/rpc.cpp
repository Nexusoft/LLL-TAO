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
        mapFunctions["help"] = Function(std::bind(&RPC::Help, this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["getinfo"] = Function(std::bind(&RPC::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
    }


    /** Echo
    *
    *  Test method to echo back the parameters passed by the caller
    *
    *  @param[in] jsonParams Parameters array passed by the caller
    *
    *  @return JSON containing the user supplied parameters array
    *
    **/
    json::json RPC::Echo(bool fHelp, const json::json& jsonParams)
    {
        
        if (fHelp || jsonParams.size() == 0)
            return std::string(
                "echo [param]...[param]"
                " - Test function that echo's back the parameters supplied in the call.");

        if(jsonParams.size() == 0)
            throw APIException(-11, "Not enough parameters");

        json::json ret;
        ret["echo"] = jsonParams;

        return ret;
    }

    /** Help
    *
    *  Returns help list.  Iterates through all functions in mapFunctions and calls each one with fHelp=true 
    *
    *  @param[in] jsonParams Parameters array passed by the caller
    *
    *  @return JSON containing the help list
    *
    **/
    json::json RPC::Help(bool fHelp, const json::json& jsonParams)
    {
        json::json ret;

        if (fHelp || jsonParams.size() > 1)
            return std::string(
                "help [command]"
                " - List commands, or get help for a command.");

        std::string strCommand = "";

        if(!jsonParams.is_null() && jsonParams.size() > 0 )
            strCommand = jsonParams.at(0);

        if( strCommand.length() > 0)
        {
            if( mapFunctions.find(strCommand) == mapFunctions.end())
                throw APIException(-32601, debug::strprintf("Method not found: %s", strCommand.c_str()));
            else
            {
                ret = mapFunctions[strCommand].Execute(true, jsonParams).get<std::string>();
            }
        }
        else
        {
            // iterate through all registered commands and build help list to return 
            std::string strHelp = "";
            for(auto& pairFunctionEntry : mapFunctions)
            {
                strHelp += pairFunctionEntry.second.Execute( true, jsonParams).get<std::string>() + "\n";
            }

            ret = strHelp;
        }

        return ret;
    }

}
