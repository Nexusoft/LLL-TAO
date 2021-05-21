/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/apinode.h>
#include <LLP/templates/events.h>

#include <TAO/API/types/exception.h>
#include <TAO/API/include/global.h>

#include <Util/include/string.h>
#include <Util/include/urlencode.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

namespace LLP
{


    /** Default Constructor **/
    APINode::APINode()
    : HTTPNode()
    {
    }

    /** Constructor **/
    APINode::APINode(const LLP::Socket &SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(SOCKET_IN, DDOS_IN, fDDOSIn)
    {
    }


    /** Constructor **/
    APINode::APINode(LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(DDOS_IN, fDDOSIn)
    {
    }


    /** Default Destructor **/
    APINode::~APINode()
    {
    }


    /* Custom Events for Core API */
    void APINode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Reset this thread's errors on new connection. */
        if(EVENT == EVENTS::CONNECT)
        {
            /* Reset the error log for this thread */
            debug::GetLastError();

            /* Log API request event */
            debug::log(5, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " API Request connected at timestamp ",   runtime::unifiedtimestamp());

            return;
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool APINode::ProcessPacket()
    {
        /* Check our http-basic authentication for the API. */
        if(!Authorized(INCOMING.mapHeaders))
        {
            /* Log a warning to the console. */
            debug::warning(FUNCTION, "API incorrect password attempt from ", this->addr.ToString());

            /* Deter brute-forcing short passwords.
             * If this results in a DOS the user really
             * shouldn't have their RPC port exposed. */
            if(config::GetArg("-apipassword", "").size() < 20)
                runtime::sleep(250); //XXX: lets add better DoS handling here, we want to throttle failed attempts for all passwords

            /* Use code 401 for unauthorized as response. */
            PushResponse(401, "");

            return false;
        }

        /* Parse the packet request. */
        std::string::size_type npos = INCOMING.strRequest.find('/', 1);

        /* Extract the API requested. */
        std::string strAPI    = INCOMING.strRequest.substr(1, npos - 1);
        std::string strMethod = INCOMING.strRequest.substr(npos + 1);

        /* The JSON response */
        json::json ret;

        /* The HTTP response status code, default to 200 unless an error is encountered */
        uint16_t nStatus = 200;

        /* Log the starting time for this command. */
        runtime::timer tLatency;
        tLatency.Start();

        /* Handle basic HTTP logic here. */
        try
        {
            json::json params;
            if(INCOMING.strType == "POST")
            {
                /* Only parse content if some has been provided */
                if(INCOMING.strContent.size() > 0)
                {
                    /* Handle different content types. */
                    if(INCOMING.mapHeaders.count("content-type"))
                    {
                        /* Form encoding. */
                        if(INCOMING.mapHeaders["content-type"] == "application/x-www-form-urlencoded")
                        {
                            /* Decode if url-form-encoded. */
                            INCOMING.strContent = encoding::urldecode(INCOMING.strContent);

                            /* parse the querystring */
                            params = QuerystringToJSON(INCOMING.strContent, strMethod);
                        }

                        /* JSON encoding. */
                        else if(INCOMING.mapHeaders["content-type"] == "application/json")
                            params = json::json::parse(INCOMING.strContent);
                        else
                            throw TAO::API::APIException(-5, debug::safe_printstr("content-type ", INCOMING.mapHeaders["content-type"], " not supported"));
                    }
                    else
                        throw TAO::API::APIException(-6, "content-type not provided when content included");
                }
            }
            else if(INCOMING.strType == "GET")
            {
                /* Detect if it is url form encoding. */
                std::string::size_type pos = strMethod.find("?");
                if(pos != strMethod.npos)
                {
                    /* Parse the querystring */
                    std::string strQuerystring = strMethod.substr(pos + 1);

                    /* Parse the method. */
                    strMethod = strMethod.substr(0, pos);

                    /* parse the querystring */
                    params = QuerystringToJSON(encoding::urldecode(strQuerystring), strMethod);
                }
            }
            else if(INCOMING.strType == "OPTIONS")
            {
                /* Build packet. */
                HTTPPacket RESPONSE(204);
                if(INCOMING.mapHeaders.count("origin"))
                    RESPONSE.mapHeaders["Access-Control-Allow-Origin"] = INCOMING.mapHeaders["origin"];;

                /* Check for access methods. */
                if(INCOMING.mapHeaders.count("access-control-request-method"))
                    RESPONSE.mapHeaders["Access-Control-Allow-Methods"] = "POST, GET, OPTIONS";

                /* Check for access headers. */
                if(INCOMING.mapHeaders.count("access-control-request-headers"))
                    RESPONSE.mapHeaders["Access-Control-Allow-Headers"] = INCOMING.mapHeaders["access-control-request-headers"];

                /* Set conneciton headers. */
                RESPONSE.mapHeaders["Connection"]             = "keep-alive";
                RESPONSE.mapHeaders["Access-Control-Max-Age"] = "86400";
                //RESPONSE.mapHeaders["Content-Length"]         = "0";
                RESPONSE.mapHeaders["Accept"]                 = "*/*";

                /* Add content. */
                this->WritePacket(RESPONSE);

                return true;
            }

            /* Execute the api and methods. */
            ret = { {"result", TAO::API::Commands::Invoke(strAPI, strMethod, params) } };
        }

        /* Handle for custom API exceptions. */
        catch(TAO::API::APIException& e)
        {
            /* Get error from exception. */
            json::json jsonError = e.ToJSON();

            /* Check to see if the caller has specified an error code to use for general API errors */
            if(INCOMING.mapHeaders.count("api-error-code"))
                nStatus = std::stoi(INCOMING.mapHeaders["api-error-code"]);
            else
                /* Default error status code is 400. */
                nStatus = 400;

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

            /* Populate the return JSON to the error */
            ret = { { "error", jsonError } };
        }


        /* Build packet. */
        HTTPPacket RESPONSE(nStatus);

        /* Add the origin header if supplied in the request */
        if(INCOMING.mapHeaders.count("origin"))
            RESPONSE.mapHeaders["Access-Control-Allow-Origin"] = INCOMING.mapHeaders["origin"];

        /* Add the connection header */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "keep-alive")
            RESPONSE.mapHeaders["Connection"] = "keep-alive";
        else
            RESPONSE.mapHeaders["Connection"] = "close";

        /* Track the stopping time of this command. */
        const double nLatency = tLatency.ElapsedNanoseconds() / 1000000.0;

        /* Add some micro-benchamrks to response data. */
        ret["info"] =
        {
            {"method",    strAPI + "/" + strMethod},
            {"status",    "active"}, //we want to check our functions map for method status
            {"timestamp", debug::rfc1123Time() },
            {"address",   this->addr.ToString()},
            {"latency",   debug::safe_printstr(std::fixed, nLatency, " ms") }
        };

        /* Log our response if argument is specified. */
        if(config::GetBoolArg("-httpresponse", false))
            debug::log(0, ret.dump(4));

        /* Add content. */
        RESPONSE.strContent = ret.dump();

        /* Write the response */
        this->WritePacket(RESPONSE);

        return true; //XXX: assess if we can return false here, if my memory serves we had issues here a couple years ago
        //because if we disconnect immediately, we break the pipe. We need to wait for buffer to clear before disconnect
    }


    bool APINode::Authorized(std::map<std::string, std::string>& mapHeaders)
    {
        /* Check for apiauth settings. */
        if(!config::GetBoolArg("-apiauth", true))
            return true;

        /* Check the headers. */
        if(!mapHeaders.count("authorization"))
            return debug::error(FUNCTION, "no authorization in header");

        /* Get the authorization encoding from the header. */
        std::string strAuth = mapHeaders["authorization"];
        if(strAuth.substr(0, 5) != "Basic")
            return debug::error(FUNCTION, "incorrect authorization type");

        /* Get the encoded content */
        std::string strUserPass64 = strAuth.substr(6);
        trim(strUserPass64);

        /* Decode from base64 */
        std::string strUserPass = (config::GetArg("-apiuser", "") + ":" + config::GetArg("-apipassword", ""));
        return encoding::DecodeBase64(strUserPass64) == strUserPass;
    }


    /* Parses the querystring and coverts it to a json object of key=value pairs */
    json::json APINode::QuerystringToJSON(const std::string& strQuerystring, const std::string& strMethod)
    {
        /* The json params to return */
        json::json params;

        /* Flag indicating that this is a list API call, in which case we need to parse the params differently */
        bool fIsList = strMethod.find("list/") != strMethod.npos;

        /* list of keywords that are acceptale parameters for a /list/xxx method.  Parameters not in this list will be converted
           into a `where` array */
        std::vector<std::string> vKeywords = {"genesis", "username", "verbose", "page", "limit", "offset", "sort", "order", "where"};

        /* Parse out the form entries by char '&' */
        std::vector<std::string> vParams;
        ParseString(strQuerystring, '&', vParams);

        /* Check each form entry. */
        for(std::string strParam : vParams)
        {
            std::string::size_type pos2 = std::string::npos;

            if(fIsList)
                pos2 = strParam.find_first_of("><=", 0);
            else
                pos2 = strParam.find('=', 0);

            if(pos2 == strParam.npos)
                break;

            /* Get the parameter key */
            std::string key = strParam.substr(0, pos2);

            /* If this is a list command, check to see if this is a where clause (not a keyword parameter supported by list)*/
            if(fIsList && std::find(vKeywords.begin(), vKeywords.end(), key) == vKeywords.end())
            {
                /* add the where clause */
                json::json jsonClause;
                jsonClause["field"] = key;

                /* Check to see if the parameter delimter is a two-character operand */
                if(strParam.find_first_of("><=", pos2+1) != strParam.npos)
                {
                    /* Extract the operand */
                    jsonClause["op"] = strParam.substr(pos2, 2);
                    pos2++;
                }
                else
                {
                    /* Extract the operand */
                    jsonClause["op"] = strParam.substr(pos2, 1);
                }

                /* Extract the value */
                jsonClause["value"] = strParam.substr(pos2 + 1);

                /* Add it to the where params*/
                params["where"].push_back(jsonClause);
            }

            /* Add the parameter as a JSON parameter regardless of whether it is has been added as where clause as it might be a
               keyword required by a list method such as name or address */
            params[key] =  strParam.substr(pos2 + 1);
        }

        return params;
    }

}
