/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

namespace LLP
{

    /* Custom Events for Core API */
    void RPCNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        //no events for now
        //TODO: see if at all possible to call from down in inheritance heirarchy
    }


    /** Main message handler once a packet is received. **/
    bool RPCNode::ProcessPacket()
    {
        /* Get the parameters from the HTTP Packet. */
        json::json jsonIncoming = json::json::parse(INCOMING.strContent);
    
        std::string strMethod = jsonIncoming["method"].get<std::string>();
        auto jsonParams = !jsonIncoming["params"].is_null() ? jsonIncoming["params"] : "";


        json::json ret = TAO::API::RPCCommands->Execute(strMethod, jsonParams, false);


        PushResponse(200, ret.dump(4));

        /* Handle a connection close header. */
        if(INCOMING.mapHeaders.count("connection") && INCOMING.mapHeaders["connection"] == "close")
            return false;

        return true;
    }


}
