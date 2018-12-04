/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_RPCNODE_H
#define NEXUS_LLP_TYPES_RPCNODE

#include <LLP/types/http.h>
#include <TAO/API/types/base.h>

namespace LLP
{

    /** RPC API Server Node
     *
     *  A node that can speak over HTTP protocols.
     *
     *  RPC API Functionality:
     *  HTTP-JSON-RPC - Nexus Ledger Level API
     *  POST / HTTP/1.1
     *  {"method":"", "params":[]}
     **/
    class RPCNode : public LLP::HTTPNode
    {
    public:

        /* Constructors for Message LLP Class. */
        RPCNode() : HTTPNode() {}
        RPCNode( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false ) : HTTPNode(SOCKET_IN, DDOS_IN, isDDOS) {}


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
