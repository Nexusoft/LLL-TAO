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
        /* Check HTTP authorization */
        if (!INCOMING.mapHeaders.count("Authorization"))
        {
            PushResponse(401, "");
            return true;
        }

        if (!HTTPAuthorized(INCOMING.mapHeaders))
        {
            debug::log(0, "RPC incorrect password attempt from %s\n", this->SOCKET.addr.ToString().c_str()); //PS TODO this address of the peer is incorrect

            /* Deter brute-forcing short passwords.
             * If this results in a DOS the user really
             * shouldn't have their RPC port exposed. */
            if (config::mapArgs["-rpcpassword"].size() < 20)
                runtime::Sleep(250);

            PushResponse(401, "");
            return true;
        }

        json::json jsonID = nullptr;
        try
        {
            /* Get the parameters from the HTTP Packet. */
            json::json jsonIncoming = json::json::parse(INCOMING.strContent);

            /* Ensure the method is in the calling json. */
            if(jsonIncoming["method"].is_null())
                throw APIException(-32600, "Missing method");

            /* Ensure the method is correct type. */
            if (!jsonIncoming["method"].is_string())
                throw APIException(-32600, "Method must be a string");

            /* Get the method string. */
            std::string strMethod = jsonIncoming["method"].get<std::string>();

            /* Check for parameters, if none set default value to empty array. */
            json::json jsonParams = jsonIncoming["params"].is_null() ? "[]" : jsonIncoming["params"];

            /* Extract the ID from the json */
            if(!jsonIncoming["id"].is_null())
                jsonID = jsonIncoming["id"];

            /* Check the parameters type for array. */
            if(!jsonParams.is_array())
                throw APIException(-32600, "Params must be an array");

            /* Execute the RPC method. */
            json::json jsonResult = TAO::API::RPCCommands->Execute(strMethod, jsonParams, false);

            json::json jsonReply = JSONRPCReply(jsonResult, nullptr, jsonID);

            printf("RPC API Sending Reply: %s\n", jsonReply.dump().c_str());

            /* Push the response data with json payload. */
            PushResponse(200, jsonReply.dump());

        }

        /* Handle for custom API exceptions. */
        catch(APIException& e)
        {
            ErrorReply(e.ToJSON(), jsonID);

            return debug::error("RPC Exception: %s", e.what());
        }

        /* Handle for JSON exceptions. */
        catch (json::detail::exception& e)
        {
            ErrorReply(APIException(e.id, e.what()).ToJSON(), jsonID);

            return debug::error("RPC Exception: %s", e.what());
        }

        /* Handle for STD exceptions. */
        catch (std::exception& e)
        {
            ErrorReply(APIException(-32700, e.what()).ToJSON(), jsonID);

            return debug::error("RPC Exception: %s", e.what());
        }

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }

    /* JSON Spec 1.0 Reply including error messages. */
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
            //special case to handle help response so that we put the multiline help response striaght into
            if( jsonResponse.is_string())
                jsonReply["result"] = jsonResponse.get<std::string>();
            else
                jsonReply["result"] = jsonResponse;
            jsonReply["error"] = nullptr;


        }

        return jsonReply;
    }

    void RPCNode::ErrorReply(const json::json& jsonError, const json::json& jsonID)
    {
        try
        {
            /* Default error status code is 500. */
            int32_t nStatus = 500;

            /* Get the error code from json. */
            int32_t nError = jsonError["code"].get<int>();

            /* Set status by error code. */
            switch(nError)
            {
                case -32600:
                    nStatus = 400;
                    break;

                case -32601:
                    nStatus = 404;
                    break;
            }

            /* Send the response packet. */
            PushResponse(nStatus, JSONRPCReply(json::json(nullptr), jsonError, jsonID).dump());
        }
        catch( APIException& e)
        {
            debug::error("RPC Exception: %s", e.what());
        }
        catch (std::exception& e)
        {
            debug::error("RPC Exception: %s", e.what());
        }
    }

    bool RPCNode::HTTPAuthorized(std::map<std::string, std::string>& mapHeaders)
    {
        /* Check the headers. */
        std::string strAuth = mapHeaders["Authorization"];
        if (strAuth.substr(0,6) != "Basic ")
            return false;

        /* Get the encoded content */
        std::string strUserPass64 = strAuth.substr(6);
        trim(strUserPass64);

        /* Decode from base64 */
        std::string strUserPass = encoding::DecodeBase64(strUserPass64);
        std::string strRPCUserColonPass = config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"];

        return strUserPass == strRPCUserColonPass;
    }


}
