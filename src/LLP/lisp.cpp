/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLP/include/lisp.h>
#include <TAO/API/types/exception.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <LLP/types/corenode.h>
#include <LLP/include/base_address.h>


namespace LLP
{

    /* The EID's and RLOC's are obtained at startup and cached */
    std::map<std::string, EID> EIDS;


    /* Makes a request to the lispers.net API for the desired endpoint */
    std::string LispersAPIRequest(std::string strEndPoint)
    {
            // HTTP basic authentication
        std::string strUserPass64 = encoding::EncodeBase64("root:" );

        /* Build the HTTP Header. */
        std::string strReply = debug::safe_printstr(
                "GET /lisp/api/", strEndPoint, " HTTP/1.1\r\n",
                "Authorization: Basic ", strUserPass64, "\r\n",
                "\r\n");

        /* Convert the content into a byte buffer. */
        std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

        /* Make the connection to the API server. */
        LLP::CoreNode coreNode;

        LLP::BaseAddress addr("127.0.0.1", 9090);

        if(!coreNode.Connect(addr))
        {
            throw TAO::API::APIException(-1, "Couldn't Connect to lispers.net API");
        }

        /* Write the buffer to the socket. */
        coreNode.Write(vBuffer, vBuffer.size());

        /* Read the response packet. */
        while(!coreNode.INCOMING.Complete() && !config::fShutdown.load())
        {

            /* Catch if the connection was closed. */
            if(!coreNode.Connected())
            {
                throw TAO::API::APIException(-1, "lispers.net API Connection Terminated");
            }

            /* Catch if the socket experienced errors. */
            if(coreNode.Errors())
            {
                throw TAO::API::APIException(-1, "lispers.net API Socket Error");
            }

            /* Read the response packet. */
            coreNode.ReadPacket();
            runtime::sleep(10);
        }

        /* Dump the response to the console. */
        std::string strPrint = "";
        if( coreNode.INCOMING.strContent.length() > 0)
        {
            return coreNode.INCOMING.strContent;
        }
        else
        {
            throw TAO::API::APIException(-1, "lispers.net Invalid response");
            return "";
        }
    }


    /* Asynchronously invokes the lispers.net API to obtain the EIDs and RLOCs used by this node
    *  and caches them for future use */
    void CacheEIDs()
    {
        /* use a lambda here to cache the EIDs asynchronously*/
        std::thread([=]()
        {
            EIDS.clear();

            try
            {
                std::string strResponse = LispersAPIRequest( "data/database-mapping");

                if( strResponse.length() > 0 )
                {
                    json::json jsonLispResponse = json::json::parse(strResponse);

                    json::json jsonEIDs = json::json::array();

                    for(auto& el : jsonLispResponse.items())
                    {
                        LLP::EID EID;
                        std::string strEIDPrefix = el.value()["eid-prefix"];
                        std::string::size_type nFindStart = strEIDPrefix.find("[") +1;
                        std::string::size_type nFindEnd = strEIDPrefix.find("]") ;

                        EID.strInstanceID = strEIDPrefix.substr(nFindStart, nFindEnd - nFindStart);
                        nFindStart = nFindEnd +1;
                        nFindEnd = strEIDPrefix.find("/");
                        EID.strAddress = strEIDPrefix.substr(nFindStart, nFindEnd - nFindStart);

                        json::json jsonRLOCs = el.value()["rlocs"];

                        for(const auto& rloc : jsonRLOCs.items())
                        {
                            if( rloc.value().find("interface") != rloc.value().end())
                            {
                                LLP::RLOC RLOC;
                                RLOC.strInterface = rloc.value()["interface"];
                                RLOC.strRLOCName = rloc.value()["rloc-name"];

                                /* if nodes are behind NAT then their public IP will be supplied
                                    in a translated-rloc field, otherwise the public IP will be
                                    supplied in the rloc field */
                                if( rloc.value().count("translated-rloc") > 0 )
                                    RLOC.strTranslatedRLOC = rloc.value()["translated-rloc"];
                                else
                                    RLOC.strTranslatedRLOC = rloc.value()["rloc"];

                                /* Add to the cached data. */
                                EID.vRLOCs.push_back(RLOC);
                            }
                        }

                        EIDS[EID.strAddress] = EID;
                    }
                }
            }
            catch(const std::exception& e)
            {
                /* we want to absorb an API exception here as the lispers.net API might not be available or
                   LISP might not be running.  In which case there are no EIDs to cache, so just log the error and move on. */
                debug::log(3, FUNCTION, e.what());
            }
        }).detach();

    }


}
