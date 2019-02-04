/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/corenode.h>

#include <TAO/API/types/exception.h>
#include <TAO/API/include/accounts.h>
#include <TAO/API/include/supply.h>
#include <TAO/API/include/ledger.h>
#include <TAO/API/include/lisp.h>

#include <Util/include/urlencode.h>

namespace LLP
{

    /* Custom Events for Core API */
    void CoreNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        //no events for now
        //TODO: see if at all possible to call from down in inheritance heirarchy
    }


    /** Main message handler once a packet is recieved. **/
    bool CoreNode::ProcessPacket()
    {
        /* Parse the packet request. */
        std::string::size_type npos = INCOMING.strRequest.find('/', 1);

        /* Extract the API requested. */
        std::string strAPI = INCOMING.strRequest.substr(1, npos - 1);

        /* Extract the method to invoke. */
        std::string METHOD = INCOMING.strRequest.substr(npos + 1);

        /* Extract the parameters. */
        json::json ret;
        try
        {
            json::json params;
            if(INCOMING.strContent.size() > 0)
            {
                if(INCOMING.mapHeaders.count("content-type"))
                {
                    if(INCOMING.mapHeaders["content-type"] == "application/x-www-form-urlencoded")
                    {
                        /* Decode if url-form-encoded. */
                        INCOMING.strContent = encoding::urldecode(INCOMING.strContent);

                        /* Split by delimiter. */
                        std::vector<std::string> vParams;
                        ParseString(INCOMING.strContent, '&', vParams);

                        /* Get the parameters. */
                        for(std::string strParam : vParams)
                        {
                            std::string::size_type pos2 = strParam.find("=");
                            if(pos2 == strParam.npos)
                                break;

                            std::string key   = strParam.substr(0, pos2);
                            std::string value = strParam.substr(pos2 + 1);

                            params[key] = value;
                        }
                    }
                    else if(INCOMING.mapHeaders["content-type"] == "application/json")
                        params = json::json::parse(INCOMING.strContent);
                    else
                        throw TAO::API::APIException(-5, debug::strprintf("content-type %s not supported\n", INCOMING.mapHeaders["content-type"].c_str()));
                }
                else
                    throw TAO::API::APIException(-6, "content-type not provided when content included");

            }

            /* Detect if it is url form encoding. */
            std::string::size_type pos = METHOD.find("?");
            if(pos != METHOD.npos)
            {
                /* Parse out the form entries by char '&' */
                std::vector<std::string> vParams;
                ParseString(encoding::urldecode(METHOD.substr(pos + 1)), '&', vParams);

                /* Parse the form from the end of method. */
                METHOD = METHOD.substr(0, pos);

                /* Check each form entry. */
                for(std::string strParam : vParams)
                {
                    std::string::size_type pos2 = strParam.find("=");
                    if(pos2 == strParam.npos)
                        break;

                    std::string key   = strParam.substr(0, pos2);
                    std::string value = strParam.substr(pos2 + 1);

                    params[key] = value;
                }
            }

            /* Execute the api and methods. */
            if(strAPI == "supply")
                ret = { {"result", TAO::API::supply.Execute(METHOD, params) } };
            else if(strAPI == "accounts")
                ret = { {"result", TAO::API::accounts.Execute(METHOD, params) } };
            else if(strAPI == "ledger")
                ret = { {"result", TAO::API::ledger.Execute(METHOD, params) } };
            else if(strAPI == "lisp")
                ret = { {"result", TAO::API::lisp.Execute(METHOD, params) } };
            else
                throw TAO::API::APIException(-4, debug::strprintf("API not found: %s", strAPI.c_str()));
        }

        /* Handle for custom API exceptions. */
        catch(TAO::API::APIException& e)
        {
            ErrorReply(e.ToJSON());

            return false;
        }

        /* Push a response. */
        PushResponse(200, ret.dump());

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }


    /* Handles a reply error code and response. */
    void CoreNode::ErrorReply(const json::json& jsonError)
    {
        /* Default error status code is 500. */
        int32_t nStatus = 500;

        /* Get the error code from json. */
        int32_t nError = jsonError["code"].get<int32_t>();

        /* Set status by error code. */
        switch(nError)
        {
            //API not found error code
            case -4:
                nStatus = 404;
                break;

            //unsupported content type
            case -5:
                nStatus = 500;
                break;

            //content type not provided
            case -6:
                nStatus = 500;
                break;
        }

        /* Send the response packet. */
        json::json ret = { { "error", jsonError } };
        PushResponse(nStatus, ret.dump());
    }

}
