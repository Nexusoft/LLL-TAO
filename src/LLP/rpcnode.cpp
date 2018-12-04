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
#include <Util/include/config.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>

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
        /** Check HTTP authorization */
        if (INCOMING.mapHeaders.count("authorization") == 0)
        {
            PushResponse(401, "");
            return true;
        }
        if (!HTTPAuthorized(INCOMING.mapHeaders))
        {
            debug::log(0, "RPC incorrect password attempt from %s\n",this->SOCKET.addr.ToString()); //PS TODO this address of the peer is incorrect
            /* Deter brute-forcing short passwords.
                If this results in a DOS the user really
                shouldn't have their RPC port exposed.*/
            if (config::mapArgs["-rpcpassword"].size() < 20)
                Sleep(250);

            PushResponse(401, "");
            return true;
        }

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

    bool RPCNode::HTTPAuthorized(std::map<std::string, std::string>& mapHeaders)
    {
        std::string strAuth = mapHeaders["authorization"];
        if (strAuth.substr(0,6) != "Basic ")
            return false;
        std::string strUserPass64 = strAuth.substr(6); 
        trim(strUserPass64);
        std::string strUserPass = encoding::DecodeBase64(strUserPass64);
        std::string strRPCUserColonPass = config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"];
        return strUserPass == strRPCUserColonPass;
    }


}
