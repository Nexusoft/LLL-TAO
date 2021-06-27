/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/rpcnode.h>
#include <LLP/templates/events.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/exception.h>

#include <Util/include/config.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>

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


    /** Default Constructor **/
    RPCNode::RPCNode()
    : HTTPNode()
    {
    }


    /** Constructor **/
    RPCNode::RPCNode(LLP::Socket SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(SOCKET_IN, DDOS_IN, fDDOSIn)
    {
    }


    /** Constructor **/
    RPCNode::RPCNode(LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(DDOS_IN, fDDOSIn)
    {
    }


    /** Default Destructor **/
    RPCNode::~RPCNode()
    {
    }


    /* Custom Events for Core API */
    void RPCNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Log connect event */
        if(EVENT == EVENTS::CONNECT)
        {
            debug::log(3, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " RPC Connected at timestamp ",   runtime::unifiedtimestamp());

            return;
        }


        /* Log disconnect event */
        if(EVENT == EVENTS::DISCONNECT)
        {
            std::string strReason = "";
            switch(LENGTH)
            {
                case DISCONNECT::TIMEOUT:
                    strReason = "Timeout";
                    break;

                case DISCONNECT::PEER:
                    strReason = "Peer disconnected";
                    break;

                case DISCONNECT::ERRORS:
                    strReason = "Errors";
                    break;

                case DISCONNECT::POLL_ERROR:
                    strReason = "Poll Error";
                    break;

                case DISCONNECT::POLL_EMPTY:
                    strReason = "Unavailable";
                    break;

                case DISCONNECT::DDOS:
                    strReason = "DDOS";
                    break;

                case DISCONNECT::FORCE:
                    strReason = "Forced";
                    break;

                default:
                    strReason = "Other";
                    break;
            }

            /* Debug output for node disconnect. */
            debug::log(3, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                " RPC Disconnected (", strReason, ") at timestamp ", runtime::unifiedtimestamp());

            return;
        }
    }


    /** Main message handler once a packet is received. **/
    bool RPCNode::ProcessPacket()
    {
        /* Check HTTP authorization */
        if(!Authorized(INCOMING.mapHeaders))
        {
            debug::error(FUNCTION, "RPC incorrect password attempt from ", this->addr.ToString());

            /* Deter brute-forcing short passwords.
             * If this results in a DOS the user really
             * shouldn't have their RPC port exposed. */
            if(config::GetArg("-rpcpassword", "").size() < 20)
                runtime::sleep(250);

            PushResponse(401, "");

            return false;
        }

        encoding::json jID = nullptr;
        try
        {
            /* Get the parameters from the HTTP Packet. */
            encoding::json jIncoming = encoding::json::parse(INCOMING.strContent);

            /* Ensure the method is in the calling json. */
            if(jIncoming["method"].is_null())
                throw TAO::API::APIException(-32600, "Missing method");

            /* Ensure the method is correct type. */
            if(!jIncoming["method"].is_string())
                throw TAO::API::APIException(-32600, "Method must be a string");

            /* Get the method string. */
            std::string strMethod = jIncoming["method"].get<std::string>();

            /* Check for parameters, if none set default value to empty array. */
            encoding::json jParams = jIncoming["params"].is_null() ? "[]" : jIncoming["params"];

            /* Extract the ID from the json */
            if(!jIncoming["id"].is_null())
                jID = jIncoming["id"];

            /* Check the parameters type for array. */
            if(!jParams.is_array())
                throw TAO::API::APIException(-32600, "Params must be an array");

            /* Check that the node is initialized. */
            if(!config::fInitialized)
                throw TAO::API::APIException(-1, "Daemon is still initializing");

            /* Execute the RPC method. */
            #ifndef NO_WALLET
            encoding::json jsonResult = TAO::API::legacy->Execute(strMethod, jParams, false);

            /* Push the response data with json payload. */
            PushResponse(200, JSONReply(jsonResult, nullptr, jID).dump());
            #endif
        }

        /* Handle for custom API exceptions. */
        catch(TAO::API::APIException& e)
        {
            ErrorReply(e.ToJSON(), jID);

            return debug::error("RPC Exception: ", e.what());
        }

        /* Handle for JSON exceptions. */
        catch(const encoding::detail::exception& e)
        {
            ErrorReply(TAO::API::APIException(e.id, e.what()).ToJSON(), jID);

            return debug::error("RPC Exception: ", e.what());
        }

        /* Handle for STD exceptions. */
        catch(const std::exception& e)
        {
            ErrorReply(TAO::API::APIException(-32700, e.what()).ToJSON(), jID);

            return debug::error("RPC Exception: ", e.what());
        }

        /* Handle a connection close header. */
        return true;
    }


    /* JSON Spec 1.0 Reply including error messages. */
    encoding::json RPCNode::JSONReply(const encoding::json& jResponse, const encoding::json& jError, const encoding::json& jID)
    {
        encoding::json jReply;
        if(!jError.is_null())
        {
            jReply["result"] = nullptr;
            jReply["error"] = jError;
        }
        else
        {
            //special case to handle help response so that we put the multiline help response striaght into
            if(jResponse.is_string())
                jReply["result"] = jResponse.get<std::string>();
            else
                jReply["result"] = jResponse;

            jReply["error"] = nullptr;
        }

        return jReply;
    }

    void RPCNode::ErrorReply(const encoding::json& jError, const encoding::json& jID)
    {
        /* Default error status code is 500. */
        uint16_t nStatus = 500;

        /* Get the error code from json. */
        int32_t nError = jError["code"].get<int32_t>();

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
        PushResponse(nStatus, JSONReply(encoding::json(nullptr), jError, jID).dump());
    }

    bool RPCNode::Authorized(std::map<std::string, std::string>& mapHeaders)
    {
        /* Check the headers. */
        if(!mapHeaders.count("authorization"))
            return debug::error(FUNCTION, "no authorization in header");

        std::string strAuth = mapHeaders["authorization"];
        if(strAuth.substr(0,6) != "Basic ")
            return debug::error(FUNCTION, "incorrect authorization type");

        /* Get the encoded content */
        const std::string strUserPass64 = trim(strAuth.substr(6));

        /* Decode from base64 */
        const std::string strUserPass = encoding::DecodeBase64(strUserPass64);
        const std::string strRPCUserColonPass = config::GetArg("-rpcuser", "") + ":" + config::GetArg("-rpcpassword", "");

        return strUserPass == strRPCUserColonPass;
    }


}
