/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <LLP/types/filenode.h>
#include <LLP/templates/events.h>

#include <TAO/API/types/exception.h>

#include <Util/include/string.h>
#include <Util/include/filesystem.h>
#include <Util/include/urlencode.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <filesystem>
#include <fstream>

namespace LLP
{
    /** Default Constructor **/
    FileNode::FileNode()
    : HTTPNode()
    {
    }

    /** Constructor **/
    FileNode::FileNode(const LLP::Socket &SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(SOCKET_IN, DDOS_IN, fDDOSIn)
    {
    }


    /** Constructor **/
    FileNode::FileNode(LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : HTTPNode(DDOS_IN, fDDOSIn)
    {
    }


    /** Default Destructor **/
    FileNode::~FileNode()
    {
    }


    /* Custom Events for Core API */
    void FileNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Reset this thread's errors on new connection. */
        if(EVENT == EVENTS::CONNECT)
        {
            /* Reset the error log for this thread */
            debug::GetLastError();

            /* Log API request event */
            debug::log(5, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                       " File Request connected at timestamp ",   runtime::unifiedtimestamp());

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
    bool FileNode::ProcessPacket()
    {
        /* Check our http-basic authentication for the API. */
        if(!Authorized(INCOMING.mapHeaders))
        {
            /* Log a warning to the console. */
            debug::warning(FUNCTION, "File incorrect password attempt from ", this->addr.ToString());

            /* Check for DDOS handle. */
            if(this->DDOS)
                this->DDOS->rSCORE += 10;

            /* Use code 401 for unauthorized as response. */
            PushResponse(401, "");

            return false;
        }

        /* Handle a GET form for HTTP. */
        std::string strRequest = INCOMING.strRequest;
        if(INCOMING.strType == "GET")
        {
            /* Detect if it is url form encoding. */
            const auto nPos =
                strRequest.find("?");

            /* Get our method here if we have a GET form. */
            strRequest = strRequest.substr(0, nPos);
        }

        /* Handle for basic OPTIONS requests. */
        if(INCOMING.strType == "OPTIONS")
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

        /* The HTTP response status code, default to 200 unless an error is encountered */
        uint16_t nStatus = 500;

        /* Get our file's absolute path. */
        const std::string strFile =
            debug::safe_printstr(config::strFileServerRoot, strRequest);

        /* Check that the file exists. */
        std::string strContent = "";
        if(filesystem::exists(strFile))
        {
            /* Check if this is a directory. */
            if(!filesystem::is_directory(strFile))
            {
                /* Read our file from disk now. */
                std::fstream ssFile =
                    std::fstream(strFile, std::ios::in | std::ios::out | std::ios::binary);

                /* Read the binary data and pass back as a string. */
                const int64_t nSize =
                    filesystem::size(strFile);

                /* Make sure we have the correct size here. */
                if(nSize != -1)
                {
                    /* Get a vector for our file contents. */
                    strContent.resize(nSize);

                    /* Read our entire file's contents. */
                    ssFile.seekg(0, std::ios::beg);
                    ssFile.read(&strContent[0], nSize);

                    /* Close our file now. */
                    ssFile.close();

                    /* If we have gotten this far, we have a successful lookup. */
                    nStatus = 200;
                }
            }
            else
                nStatus = 403;
        }
        else
            nStatus = 404;

        /* Build packet. */
        HTTPPacket RESPONSE(nStatus);

        /* Add the origin header if supplied in the request */
        if(INCOMING.mapHeaders.count("origin"))
            RESPONSE.mapHeaders["Access-Control-Allow-Origin"] = INCOMING.mapHeaders["origin"];

        /* Add the connection header */
        RESPONSE.mapHeaders["Connection"] = "close";

        /* Set our packet's content now. */
        RESPONSE.strContent = strContent;
        RESPONSE.mapHeaders["Content-Type"] = "text/html";

        /* Write the response */
        this->WritePacket(RESPONSE);

        return true;
    }


    bool FileNode::Authorized(std::map<std::string, std::string>& mapHeaders)
    {
        /* Make a local cache of our authorization header. */
        const static std::string strUserPass =
            (config::GetArg("-apiuser", "") + ":" + config::GetArg("-apipassword", ""));

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
        return encoding::DecodeBase64(strUserPass64) == strUserPass;
    }
}
