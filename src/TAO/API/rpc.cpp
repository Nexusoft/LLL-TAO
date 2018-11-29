/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/rpc.h>
#include <TAO/API/types/rpcapi.h>

namespace TAO::API::RPC
{

    /** Main Constructor used to initialize the node and register the API's supported this protocol **/
    RPCServer::RPCServer( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS )
        : JSONAPINode( SOCKET_IN, DDOS_IN, isDDOS )
    {
        // for RPC there is only one API.
        //For Core we would have many different API's and they can be optionally added depending on configuration
        // e.g.
        // RegisterAPI(new MusicAPI());
        // RegisterAPI(new SupplyChainAPI());
        // if( APIEnabled(SomeCustomerPrivateAPI) )
        //      RegisterAPI(new SomeCustomerPrivateAPI());
        RegisterAPI(new RPCAPI());
    }

    /* Custom Events for Core API */
    void RPCServer::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        //no events for now
        //TODO: see if at all possible to call from down in inheritance heirarchy
    }


    /** Main message handler once a packet is received. **/
    bool RPCServer::ProcessPacket()
    {
        /* Get the parameters from the HTTP Packet. */
        nlohmann::json jsonParams = nlohmann::json::parse(INCOMING.strContent);

        // for RPC there is only one API called "RPC"
        // for Core this would be...
        // std::string API = INCOMING.strRequest.substr(1, npos - 1);
        // std::string METHOD = INCOMING.strRequest.substr(npos + 1);
        // ret = mapJSONAPIHandlers[API].HandleJSONAPIMethod(METHOD, parameters)
        nlohmann::json jsonRet = mapJSONAPIHandlers["RPC"]->HandleJSONAPIMethod(jsonParams["method"], jsonParams["params"]);


        PushResponse(200, jsonRet.dump(4));

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }

}
