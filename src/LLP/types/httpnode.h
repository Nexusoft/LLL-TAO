/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_HTTPNODE_H
#define NEXUS_LLP_TYPES_HTTPNODE_H

#include <LLP/templates/base_connection.h>
#include <LLP/packets/http.h>

#include <string>
#include <vector>
#include <cstdint>


#define HTTPNODE ANSI_COLOR_FUNCTION "HTTPNode" ANSI_COLOR_RESET " : "

namespace LLP
{

    /** HTTPNode
     *
     *  A node that can speak over HTTP protocols.
     *
     *  This overrides process packet as null virtual function to require one more level
     *  of inheritance. This is to allow different interpretations of standard HTTP header.
     *
     *  Such examples would be:
     *  HTTP-JSON-API - Nexus Contracts API
     *  POST <command> HTTP/1.1
     *  {"params":[]}
     *
     *  HTTP-JSON-RPC - Ledger Level API
     *  POST / HTTP/1.1
     *  {"method":"", "params":[]}
     *
     *  This could also be used as the base for a HTTP-LLP server implementation.
     *
     **/
    class HTTPNode : public BaseConnection<HTTPPacket>
    {
        /* Internal Read Buffer. */
        std::vector<int8_t> vchBuffer;


    public:

        /** Default Constructor **/
        HTTPNode();


        /** Constructor **/
        HTTPNode(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        HTTPNode(DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~HTTPNode();


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) override = 0;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() override = 0;


        /** DoS
         *
         *  Send the DoS Score to DDOS Filte
         *
         *  @param[in] nDoS The score to add for DoS banning
         *  @param[in] fReturn The value to return (False disconnects this node)
         *
         **/
        bool DoS(uint32_t nDoS, bool fReturn);


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final;


        /** PushResponse
         *
         *  Returns an HTTP packet with response code and content.
         *
         *  @param[in] nMsg The status code to respond with.
         *
         *  @param[in] strContent The content to post return with.
         *
         **/
        void PushResponse(const uint16_t nMsg, const std::string& strContent);

    };

}

#endif
