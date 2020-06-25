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

        /* Makes a new outgoing connection request to a peer.  
           This method can optionally block (for up to 30s) waiting for an answer. */
        json::json P2P::Request(const json::json& params, bool fHelp)
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

            /* Flag indicating whether to wait for the peer to respond and establish a connection */
            bool fWait = false;

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
            
            /* Get the wait parameter */
            if(params.find("wait") != params.end())
                fWait = params["wait"].get<std::string>() == "1" || params["wait"].get<std::string>() == "true";

           
            /* Check to see if P2P is enabled */
            if(!LLP::P2P_SERVER)
                throw APIException(-280, "P2P server not enabled on this node");

            /* The new connection request */
            LLP::P2P::ConnectionRequest request;

            /* Connection pointer  */
            memory::atomic_ptr<LLP::P2PNode> connection;

            /* Check to see if a connection already exists */
            if(get_connection(strAppID, hashGenesis, hashPeer, connection))
                throw APIException(-283, "A connection to this peer already exists");

            /* If an outgoing request already exists from this peer then remove it */
            if(session.HasP2PRequest(strAppID, hashPeer, false))
                session.DeleteP2PRequest(strAppID, hashPeer, false);

            /* Set up the request  */
            request.strAppID = strAppID;
            request.hashPeer = hashPeer;
            request.nTimestamp = runtime::unifiedtimestamp();

            /* Get this nodes IP address from the tritium server as that is the most reliable way to obtain it*/
            request.address.SetIP(LLP::TritiumNode::thisAddress);
            
            /* Set the port to be contacted on, which is the port for the P2P server */
            /* If SSL enabled then set the SSL port */
            if(LLP::P2P_SERVER->SSLEnabled())
                request.nSSLPort = LLP::P2P_SERVER->GetPort(true);
            
            /* If the server allows non-ssl connections then set the standard port */
            if(!LLP::P2P_SERVER->SSLRequired())
                request.nPort = LLP::P2P_SERVER->GetPort(false);

            /* Generate a new random session ID */
            request.nSession = LLC::GetRand();

            /* Add the request to our outgoing list */
            session.AddP2PRequest(request, false);
            
            /* Now build and push the request message out to the network */

            /* Public key bytes*/
            std::vector<uint8_t> vchPubKey;
        
            /* Signature bytes */
            std::vector<uint8_t> vchSig;

            /* Build the byte stream from the request data in order to generate the signature */
            DataStream ssMsgData(SER_NETWORK, LLP::P2P::PROTOCOL_VERSION);
            ssMsgData << hashGenesis << request;

            /* Generate signature */
            session.GetAccount()->Sign("network", ssMsgData.Bytes(), session.GetNetworkKey(), vchPubKey, vchSig);

            debug::log(3, "Sending P2P connection request."); 
            
            /* Relay the message to all peers */
            LLP::TRITIUM_SERVER->Relay(
                uint8_t(LLP::Tritium::ACTION::REQUEST),
                uint8_t(LLP::Tritium::TYPES::P2PCONNECTION),
                hashGenesis,                
                request,
                vchPubKey,
                vchSig);

            /* Optionally wait for a response */
            if(fWait)
            {
                /* Indicates the peer has accepted the request and a connection has been established */
                bool fConnected = false;

                /* Calculate the timestamp when the wait should expire.  The maximum time to wait is the 5s less than the API server 
                   timeout, so that there is time to respond before the API request times out. */
                uint32_t nExpires = runtime::unifiedtimestamp() + (static_cast<uint32_t>(config::GetArg(std::string("-apitimeout"), 30)) - 5);

                /* Loop until expiry or connection established */
                while(runtime::unifiedtimestamp() < nExpires && !fConnected)
                {
                    /* Check to see if there is a connection */
                    if(get_connection(strAppID, hashGenesis, hashPeer, connection))
                        fConnected = true; //break out
                    
                    /* Sleep for 1s */
                    runtime::sleep(1000);
                }

                if(!fConnected)
                    throw APIException(-286, "Timeout waiting for peer to accept connection request");

            }
            
            /* Flag successful request */
            response["success"] = true;

            return response;
        }

    }
}
