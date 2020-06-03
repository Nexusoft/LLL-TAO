/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <LLP/include/global.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Returns a list of all active P2P connections for the logged in user, including the number of outstanding messages. */
        json::json P2P::List(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json response = json::json::array();

            /* Get the logged in session */
            Session& session = users->GetSession(params);

            /* Get the Genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Optional App ID filter */
            std::string strAppID = "";
            if(params.find("appid") != params.end())
                strAppID = params["appid"].get<std::string>();
           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");
            
            for(uint16_t nThread = 0; nThread < LLP::P2P_SERVER->MAX_THREADS; ++nThread)
            {
                /* Get the data threads. */
                LLP::DataThread<LLP::P2PNode>* dt = LLP::P2P_SERVER->DATA_THREADS[nThread];

                /* Lock the data thread. */
                uint16_t nSize = static_cast<uint16_t>(dt->CONNECTIONS->size());

                /* Loop through connections in data thread. */
                for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    try
                    {
                        /* Get the connection */
                        auto& connection = dt->CONNECTIONS->at(nIndex);

                        /* Skip over inactive connections. */
                        if(connection != nullptr && connection->Connected())
                        {
                            /* Check that the connection is from this genesis hash  */
                            if(connection->hashGenesis != hashGenesis)
                                continue;

                            /* Check the appID filter, if specified */
                            if(!strAppID.empty() && connection->strAppID != strAppID)
                                continue;

                            json::json jsonConnection;

                            /* Populate the response JSON */
                            jsonConnection["appid"]    = connection->strAppID;
                            jsonConnection["genesis"]  = connection->hashPeer.ToString();
                            jsonConnection["session"]  = connection->nSession;
                            jsonConnection["messages"] = connection->MessageCount();
                            jsonConnection["address"]  = connection->addr.ToString();    
                            jsonConnection["latency"]  = connection->nLatency.load();
                            jsonConnection["lastseen"] = connection->nLastPing.load();

                            response.push_back(jsonConnection);
                        }
                    }
                    catch(const std::exception& e)
                    {
                        //debug::error(FUNCTION, e.what());
                    }
                }
            }

            return response;
        }


        /* Returns a list of all pending connection requests for the logged in user, either outgoing (you have made). */
        json::json P2P::ListRequests(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json response = json::json::array();

            /* Get the logged in session */
            Session& session = users->GetSession(params);

            /* Get the Genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Optional App ID filter */
            std::string strAppID = "";
            if(params.find("appid") != params.end())
                strAppID = params["appid"].get<std::string>();

            /* Optional incoming flag */
            bool fIncoming = true;
            if(params.find("incoming") != params.end())
                fIncoming = params["incoming"].get<std::string>() == "1" || params["incoming"].get<std::string>() == "true";
           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");


            /* Get the connection requests */
            const std::vector<LLP::P2P::ConnectionRequest> requests = session.GetP2PRequests(fIncoming);

            /* Loop through the connection requests */
            for(const auto& connection : requests)
            {
                json::json jsonConnection;

                /* Populate the response JSON */
                jsonConnection["appid"]     = connection.strAppID;
                jsonConnection["genesis"]   = connection.hashPeer.ToString();
                jsonConnection["session"]   = connection.nSession;
                jsonConnection["address"]   = connection.address.ToString();    
                jsonConnection["timestamp"] = connection.nTimestamp;

                response.push_back(jsonConnection);
            }
            
            return response;
        }
    }
}
