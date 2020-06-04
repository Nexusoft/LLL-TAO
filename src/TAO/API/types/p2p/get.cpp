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

        /* Returns information about a single connection for the logged in user including the number of outstanding messages. */
        json::json P2P::Get(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json response;

            /* Get the logged in session */
            Session& session = users->GetSession(params);

            /* Get the Genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* The App ID to search for */
            std::string strAppID = "";

            /* The peer genesis hash to search for */
            uint256_t hashPeer;

            /* Get App ID  */
            if(params.find("appid") == params.end())
                throw APIException(-281, "Missing App ID");

            strAppID = params["appid"].get<std::string>();

            /* Get the peer hash  */
            if(params.find("genesis") != params.end())
                hashPeer.SetHex(params["genesis"].get<std::string>());
            /* Check for username. */
            else if(params.find("username") != params.end())
                hashPeer = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else 
                throw APIException(-111, "Missing genesis / username");
            
           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");

            /* Connection pointer  */
            memory::atomic_ptr<LLP::P2PNode> connection;

            /* Get the connection matching the criteria */
            if(get_connection(strAppID, hashGenesis, hashPeer, connection))
            {
                /* Populate the response JSON */
                response["appid"]    = connection->strAppID;
                response["genesis"]  = connection->hashPeer.ToString();
                response["session"]  = connection->nSession;
                response["messages"] = connection->MessageCount();
                response["address"]  = connection->addr.ToString();    
                response["latency"]  = connection->nLatency.load() == std::numeric_limits<uint32_t>::max() ? 0 : connection->nLatency.load();
                response["lastseen"] = connection->nLastPing.load();
            }
            else
                throw APIException(-282, "Connection not found");

            return response;
        }

    }
}
