/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_INCLUDE_RPC_H
#define NEXUS_TAO_API_INCLUDE_RPC_H


#include <TAO/API/types/jsonapibase.h>

namespace TAO::API::RPC
{
    class RPCMethod : public JSONAPIMethod
    {
        public:
            RPCMethod(std::function<nlohmann::json(bool, nlohmann::json)> function, bool okSafeMode) : JSONAPIMethod(function), okSafeMode(okSafeMode){}
            bool okSafeMode;
    };

    
    /** RPC API
     *
     *  A node that can speak over HTTP protocols.
     *
     *  RPC API Functionality:
     *  HTTP-JSON-RPC - Nexus Ledger Level API
     *  POST / HTTP/1.1
     *  {"method":"", "params":[]}
     *
     *  This could also be used as the base for a HTTP-LLP server implementation.
     **/
    class RPCServer : public TAO::API::JSONAPINode
    {
    public:

        /* Constructors for Message LLP Class. */
        RPCServer() : JSONAPINode() {}
        RPCServer( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false );


        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** Main message handler once a packet is recieved. **/
        bool ProcessPacket();
    
    
    };
}

#endif
