/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>
#include <TAO/API/types/exception.h>

// using alias to simplify using APIException liberally without having to reference the TAO:API namespace
using APIException = TAO::API::APIException ;

namespace LLP
{


    //
    // JSON-RPC protocol.  Nexus speaks version 1.0 for maximum compatibility,
    // but uses JSON-RPC 1.1/2.0 standards for parts of the 1.0 standard that were
    // unspecified (HTTP errors and contents of 'error').
    //
    // 1.0 spec: http://json-rpc.org/wiki/specification
    // 1.2 spec: http://groups.google.com/group/json-rpc/web/json-rpc-over-http
    // http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
    //

    /* Custom Events for Core API */
    void RPCNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        //no events for now
        //TODO: see if at all possible to call from down in inheritance heirarchy
    }


    /** Main message handler once a packet is received. **/
    bool RPCNode::ProcessPacket()
    {
        /** Check HTTP authorization */
        if (INCOMING.mapHeaders.count("Authorization") == 0)
        {
            PushResponse(401, "");
            return true;
        }
        if (!HTTPAuthorized(INCOMING.mapHeaders))
        {
            debug::log(0, "RPC incorrect password attempt from %s\n",this->SOCKET.addr.ToString().c_str()); //PS TODO this address of the peer is incorrect
            /* Deter brute-forcing short passwords.
                If this results in a DOS the user really
                shouldn't have their RPC port exposed.*/
            if (config::mapArgs["-rpcpassword"].size() < 20)
                Sleep(250);

            PushResponse(401, "");
            return true;
        }

        json::json jsonID = nullptr;
        try
        {
            /* Get the parameters from the HTTP Packet. */
            json::json jsonIncoming = json::json::parse(INCOMING.strContent);

            if(jsonIncoming["method"].is_null())
                throw APIException(-32600, "Missing method");
            if (!jsonIncoming["method"].is_string())
                throw APIException(-32600, "Method must be a string");
            std::string strMethod = jsonIncoming["method"].get<std::string>();

            json::json jsonParams = !jsonIncoming["params"].is_null() ? jsonIncoming["params"] : "{}";
            jsonID = !jsonIncoming["id"].is_null() ? jsonIncoming["id"] : json::json(nullptr);

            if(!jsonParams.is_array())
                throw APIException(-32600, "Params must be an array");

            json::json jsonResult = TAO::API::RPCCommands->Execute(strMethod, jsonParams, false);

            json::json jsonReply = JSONRPCReply(jsonResult, nullptr, jsonID);

            PushResponse(200, jsonReply.dump(4));

        }
        catch( APIException& e)
        {
            debug::log(0, "RPC Exception: %s\n", e.what());
            ErrorReply(e.ToJSON(), jsonID);
        }
        catch (json::detail::exception& e)
        {
            debug::log(0, "RPC Exception: %s\n", e.what());
            ErrorReply(APIException(e.id, e.what()).ToJSON(), jsonID);
        }
        catch (std::exception& e)
        {
            debug::log(0, "RPC Exception: %s\n", e.what());
            ErrorReply(APIException(-32700, e.what()).ToJSON(), jsonID);
        }

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }

    json::json RPCNode::JSONRPCReply(const json::json& jsonResponse, const json::json& jsonError, const json::json& jsonID)
    {
        json::json jsonReply;
        if (!jsonError.is_null() )
        {
            jsonReply["result"] = nullptr;
            jsonReply["error"] = jsonError;
        }
        else
        {
            jsonReply["result"] = jsonResponse;
            jsonReply["error"] = nullptr;
        }

        jsonReply["id"] = jsonID;
        return jsonReply;
    }

    void RPCNode::ErrorReply(const json::json& jsonError, const json::json& jsonID)
    {
        try
        {
            // Send error reply from json-rpc error object
            int nStatus = 500;
            int code = jsonError["code"].get<int>();
            if (code == -32600) nStatus = 400;
            else if (code == -32601) nStatus = 404;
            json::json jsonReply = JSONRPCReply(json::json(nullptr), jsonError, jsonID);
            PushResponse(nStatus, jsonReply.dump(4));
        }
        catch( APIException& e)
        {
            debug::log(0, "RPC Exception: %s\n", e.what());
        }
        catch (std::exception& e)
        {
            debug::log(0, "RPC Exception: %s\n", e.what());
        }
    }

    bool RPCNode::HTTPAuthorized(std::map<std::string, std::string>& mapHeaders)
    {
        std::string strAuth = mapHeaders["Authorization"];
        if (strAuth.substr(0,6) != "Basic ")
            return false;

        std::string strUserPass64 = strAuth.substr(6);
        trim(strUserPass64);
        std::string strUserPass = encoding::DecodeBase64(strUserPass64);
        std::string strRPCUserColonPass = config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"];

        printf("%s - %s\n", strRPCUserColonPass.c_str(), strUserPass.c_str());
        return strUserPass == strRPCUserColonPass;
    }


}
