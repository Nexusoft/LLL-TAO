/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/lisp.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>

#include <LLP/types/corenode.h>
#include <LLP/include/base_address.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** The LISP API instance. **/
        Lisp lisp;

        /* Standard initialization function. */
        void Lisp::Initialize()
        {
            mapFunctions["myeids"]              = Function(std::bind(&Lisp::MyEIDs,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["myrlocs"]             = Function(std::bind(&Lisp::MyRLOCs,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["databasemapping"]     = Function(std::bind(&Lisp::DatabaseMapping,    this, std::placeholders::_1, std::placeholders::_2));
        }

        /* Makes a request to the lispers.net API for the desired endpoint */
        std::string Lisp::LispersAPIRequest(std::string strEndPoint)
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
                throw APIException(-1, "Couldn't Connect to lispers.net API");
            }

            /* Write the buffer to the socket. */
            coreNode.Write(vBuffer, vBuffer.size());

            /* Read the response packet. */
            while(!coreNode.INCOMING.Complete() && !config::fShutdown.load())
            {

                /* Catch if the connection was closed. */
                if(!coreNode.Connected())
                {
                    throw APIException(-1, "lispers.net API Connection Terminated");
                }

                /* Catch if the socket experienced errors. */
                if(coreNode.Errors())
                {
                    throw APIException(-1, "lispers.net API Socket Error");
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
                throw APIException(-1, "lispers.net Invalid response");
                return "";
            }
        }

        /* Queries the lisp api and returns the EID's for this node. */
        json::json Lisp::MyEIDs(const json::json& params, bool fHelp)
        {
            json::json jsonRet;
            std::string strResponse = LispersAPIRequest( "database-mapping");

            if( strResponse.length() > 0 )
            {
                json::json jsonLispResponse = json::json::parse(strResponse);
                json::json jsonLispDatabaseMapping = jsonLispResponse[0]["lisp database-mapping"];

                json::json jsonEIDs = json::json::array();

                for(auto& el : jsonLispDatabaseMapping.items())
                {
                    for(json::json::iterator it = el.value().begin(); it != el.value().end(); ++it )
                    {
                        if(it.key() == "prefix")
                            jsonEIDs.push_back(it.value()["eid-prefix"]);
                    }
                }
                jsonRet["eids"] = jsonEIDs;
            }
            else
            {
                throw APIException(-1, "Communications error with lispers.net API");
            }

            return jsonRet;

        }

        /* Queries the lisp api and returns the RLOC's for this node. */
        json::json Lisp::MyRLOCs(const json::json& params, bool fHelp)
        {
            json::json jsonRet;
            std::string strResponse = LispersAPIRequest( "database-mapping");

            if( strResponse.length() > 0 )
            {
                json::json jsonLispResponse = json::json::parse(strResponse);
                json::json jsonLispDatabaseMapping = jsonLispResponse[0]["lisp database-mapping"];

                json::json jsonRLOCs = json::json::array();

                for(auto& el : jsonLispDatabaseMapping.items())
                {
                    for(json::json::iterator it = el.value().begin(); it != el.value().end(); ++it )
                    {
                        if(it.key() == "rloc" && it.value().find("address") != it.value().end())
                            jsonRLOCs.push_back(it.value()["address"]);
                    }
                }
                jsonRet["rlocs"] = jsonRLOCs;
            }
            else
            {
                throw APIException(-1, "Communications error with lispers.net API");
            }

            return jsonRet;

        }

        /* Queries the lisp api and returns the database mapping for this node. */
        json::json Lisp::DatabaseMapping(const json::json& params, bool fHelp)
        {
            json::json jsonRet;
            std::string strResponse = LispersAPIRequest( "database-mapping");

            if( strResponse.length() > 0 )
            {
                json::json jsonLispResponse = json::json::parse(strResponse);
                json::json jsonLispDatabaseMapping = jsonLispResponse[0]["lisp database-mapping"];

                jsonRet["database-mapping"] = jsonLispDatabaseMapping;
            }
            else
            {
                throw APIException(-1, "Communications error with lispers.net API");
            }

            return jsonRet;

        }

    }

}
