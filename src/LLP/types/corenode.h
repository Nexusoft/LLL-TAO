/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_CORENODE_H
#define NEXUS_LLP_TYPES_CORENODE_H

#include <LLP/types/http.h>

#include <functional>

#include <Util/include/json.h>

namespace LLP
{
    /** Core API
     *
     *  A node that can speak over HTTP protocols.
     *
     *  Core API Functionality:
     *  HTTP-JSON-API - Nexus Contracts API
     *  POST /<api>/<command> HTTP/1.1
     *  {"params":[]}
     *
     *  This could also be used as the base for a HTTP-LLP server implementation.
     **/
    class CoreNode : public LLP::HTTPNode
    {
    public:

        /* Constructors for Message LLP Class. */
        CoreNode() : LLP::HTTPNode() {}
        CoreNode( LLP::Socket_t SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false ) : LLP::HTTPNode( SOCKET_IN, DDOS_IN ) { }


        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** Main message handler once a packet is recieved. **/
        bool ProcessPacket();


        /** Error Reply
         *
         *  Handles a reply error code and response.
         *
         *  @param[in] jsonError The error object.
         *
         **/
        void ErrorReply(const json::json& jsonError);

    };
}

#endif
