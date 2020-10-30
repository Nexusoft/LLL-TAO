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

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Vector of where clauses to apply to filter the results */
            std::map<std::string, std::vector<Clause>> vWhere;

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset, vWhere);

            /* Flag indicating there are top level filters  */
            bool fHasFilter = vWhere.count("") > 0;

            /* Fields to ignore in the where clause.  This is necessary so that the appid param is not treated as 
               standard where clauses to filter the json */
            std::vector<std::string> vIgnore = {"appid"};
           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");

            /* Get the connections from the P2P server */
            std::vector<memory::atomic_ptr<LLP::P2PNode>*> vConnections = LLP::P2P_SERVER->GetConnections();

            /* Iterate the connections*/
            uint32_t nTotal = 0;
            for(const auto& connection : vConnections)
            {
                /* Skip over inactive connections. */
                if(!connection->load())
                    continue;

                /* Push the active connection. */
                if(connection->load()->Connected())
                {
                    /* Check the appID filter, if specified */
                    if(!strAppID.empty() && connection->load()->strAppID != strAppID)
                        continue;

                    json::json jsonConnection;

                    /* Populate the response JSON */
                    jsonConnection["appid"]    = connection->load()->strAppID;
                    jsonConnection["genesis"]  = connection->load()->hashPeer.ToString();
                    jsonConnection["session"]  = connection->load()->nSession;
                    jsonConnection["messages"] = connection->load()->MessageCount();
                    jsonConnection["address"]  = connection->load()->addr.ToStringIP();
                    jsonConnection["port"]     = connection->load()->addr.ToStringPort();
                    jsonConnection["latency"]  = connection->load()->nLatency.load() == std::numeric_limits<uint32_t>::max() ? 0 : connection->load()->nLatency.load();
                    jsonConnection["lastseen"] = connection->load()->nLastPing.load();

                    /* Check to see that it matches the where clauses */
                    if(fHasFilter)
                    {
                        /* Skip this top level record if not all of the filters were matched */
                        if(!MatchesWhere(jsonConnection, vWhere[""], vIgnore))
                            continue;
                    }

                    ++nTotal;

                    /* Check the offset. */
                    if(nTotal <= nOffset)
                        continue;
                    
                    /* Check the limit */
                    if(nTotal - nOffset > nLimit)
                        break;

                    response.push_back(jsonConnection);
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

            /* Number of results to return. */
            uint32_t nLimit = 100;

            /* Offset into the result set to return results from */
            uint32_t nOffset = 0;

            /* Sort order to apply */
            std::string strOrder = "desc";

            /* Vector of where clauses to apply to filter the results */
            std::map<std::string, std::vector<Clause>> vWhere;

            /* Get the params to apply to the response. */
            GetListParams(params, strOrder, nLimit, nOffset, vWhere);

            /* Flag indicating there are top level filters  */
            bool fHasFilter = vWhere.count("") > 0;

            /* Fields to ignore in the where clause.  This is necessary so that the appid param is not treated as 
               standard where clauses to filter the json */
            std::vector<std::string> vIgnore = {"appid", "incoming"};
           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");


            /* Get the connection requests */
            const std::vector<LLP::P2P::ConnectionRequest> requests = session.GetP2PRequests(fIncoming);

            /* Loop through the connection requests */
            uint32_t nTotal = 0;
            for(const auto& connection : requests)
            {
                json::json jsonConnection;

                /* Populate the response JSON */
                jsonConnection["appid"]     = connection.strAppID;
                jsonConnection["genesis"]   = connection.hashPeer.ToString();
                jsonConnection["session"]   = connection.nSession;
                jsonConnection["address"]   = connection.address.ToStringIP();
                jsonConnection["port"]      = connection.nPort;
                jsonConnection["sslport"]   = connection.nSSLPort;    
                jsonConnection["timestamp"] = connection.nTimestamp;

                /* Check to see that it matches the where clauses */
                if(fHasFilter)
                {
                    /* Skip this top level record if not all of the filters were matched */
                    if(!MatchesWhere(jsonConnection, vWhere[""], vIgnore))
                        continue;
                }

                ++nTotal;

                /* Check the offset. */
                if(nTotal <= nOffset)
                    continue;
                
                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                response.push_back(jsonConnection);
            }
            
            return response;
        }
    }
}
