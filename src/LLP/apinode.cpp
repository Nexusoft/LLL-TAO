/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <LLP/types/apinode.h>
#include <LLP/templates/events.h>

#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/exception.h>

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

        /* Handle for a HEADER event. */
        if(EVENT == EVENTS::HEADER)
        {
            /* Check for forward DDOS filter. */
            if(INCOMING.mapHeaders.count("x-forwarded-for"))
            {
                /* Grab our forwarded address from headers. */
                const std::string strAddress = INCOMING.mapHeaders["x-forwarded-for"];

                /* Update the address inside this connection. */
                this->addr = LLP::BaseAddress(strAddress);

                /* Swap our new address back into DDOS map */
                if(!API_SERVER->DDOS_MAP->count(this->addr))
                    API_SERVER->DDOS_MAP->insert(std::make_pair(this->addr, new DDOS_Filter(API_SERVER->CONFIG.DDOS_TIMESPAN)));

                /* Add a connection to score. */
                this->DDOS = API_SERVER->DDOS_MAP->at(this->addr);
                this->DDOS->cSCORE += 1;
            }
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
        const std::string::size_type nPos = INCOMING.strRequest.find('/', 1);

        /* Extract the API requested. */
        std::string strAPI    = INCOMING.strRequest.substr(1, nPos - 1);
        std::string strMethod = INCOMING.strRequest.substr(nPos + 1);

        /* The JSON response */
        encoding::json jRet;

        /* The HTTP response status code, default to 200 unless an error is encountered */
        uint16_t nStatus = 200;

        /* Log the starting time for this command. */
        runtime::timer tLatency;
        tLatency.Start();

        /* Handle basic HTTP logic here. */
        encoding::json jParams;
        try
        {
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

                            /* Split by delimiter. */
                            std::vector<std::string> vParams;
                            ParseString(INCOMING.strContent, '&', vParams);

                            /* Grab our parameters. */
                            jParams = TAO::API::ParamsToJSON(vParams);
                        }

                        /* JSON encoding. */
                        else if(INCOMING.mapHeaders["content-type"] == "application/json")
                        {
                            /* Parse JSON like normal. */
                            jParams = encoding::json::parse(INCOMING.strContent);
                        }
                        else
                            throw TAO::API::Exception(-5, "content-type [", INCOMING.mapHeaders["content-type"], "] not supported");
                    }
                    else
                        throw TAO::API::Exception(-5, "content-type [null or misisng] not supported");
                }
            }
            else if(INCOMING.strType == "GET")
            {
                /* Detect if it is url form encoding. */
                const auto nPos = strMethod.find("?");
                if(nPos != strMethod.npos)
                {
                    /* Parse out the form entries by char '&' */
                    std::vector<std::string> vParams;
                    ParseString(encoding::urldecode(strMethod.substr(nPos + 1)), '&', vParams);

                    /* Parse the form from the end of method. */
                    strMethod = strMethod.substr(0, nPos);

                    /* Grab our parameters. */
                    jParams = TAO::API::ParamsToJSON(vParams);
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

            /* Check if we need to parse a where query. */
            if(jParams.find("where") != jParams.end())
                jParams["where"] = TAO::API::QueryToJSON(jParams["where"].get<std::string>());

            /* Add our request information before invoking command. */
            jParams["request"] =
            {
                {"commands", strAPI },
                {"method", strMethod},
            };

            /* Execute the api and methods. */
            jRet = { {"result", TAO::API::Commands::Invoke(strAPI, strMethod, jParams) } };
        }

        /* Handle for custom API exceptions. */
        catch(TAO::API::Exception& e)
        {
            /* Get error from exception. */
            encoding::json jError = e.ToJSON();

            /* Check to see if the caller has specified an error code to use for general API errors */
            if(INCOMING.mapHeaders.count("api-error-code"))
                nStatus = std::stoi(INCOMING.mapHeaders["api-error-code"]);
            else
                /* Default error status code is 400. */
                nStatus = 400;

            /* Cache our error code here. */
            const int32_t nError = jError["code"].get<int32_t>();

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
            jRet = { { "error", jError } };
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
        jRet["info"] =
        {
            {"method",    strAPI + "/" + strMethod                          },
            {"status",    TAO::API::Commands::Status(strAPI, strMethod)     },
            {"address",   this->addr.ToString()                             },
            {"latency",   debug::safe_printstr(std::fixed, nLatency, " ms") }
        };

        /* Log our response if argument is specified. */
        if(config::GetBoolArg("-httpresponse", false))
            debug::log(0, jRet.dump(4));

        /* Add content. */
        RESPONSE.strContent = jRet.dump();

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
        const std::string strUserPass64 = trim(strAuth.substr(6));

        /* Decode from base64 */
        const std::string strUserPass = (config::GetArg("-apiuser", "") + ":" + config::GetArg("-apipassword", ""));
        return encoding::DecodeBase64(strUserPass64) == strUserPass;
    }
}
