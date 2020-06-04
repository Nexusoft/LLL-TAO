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

        /* Returns the oldest message in the message queue from a peer without removing it from the queue . */
        json::json P2P::Peek(const json::json& params, bool fHelp)
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

            /* The data to be sent */
            std::vector<uint8_t> vchData;

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
            if(!get_connection(strAppID, hashGenesis, hashPeer, connection))
                throw APIException(-282, "Connection not found");
             
            /* Get the message */
            LLP::P2P::Message message = connection->PeekMessage();

            /* Populate the response */
            response["timestamp"] = message.nTimestamp;
            response["data"] = encoding::EncodeBase64(&message.vchData[0], message.vchData.size()); 

            return response;
        }

    }
}
