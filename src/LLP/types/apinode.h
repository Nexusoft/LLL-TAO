/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_APINODE_H
#define NEXUS_LLP_TYPES_APINODE_H

#include <LLP/types/httpnode.h>
#include <Util/include/json.h>

namespace LLP
{
    /** APINode
     *
     * Core API
     *
     *  A node that can speak over HTTP protocols.
     *
     *  Core API Functionality:
     *  HTTP-JSON-API - Nexus Contracts API
     *  POST /<api>/<command> HTTP/1.1
     *  {"params":[]}
     *
     *  This could also be used as the base for a HTTP-LLP server implementation.
     *
     **/
    class APINode : public HTTPNode
    {
    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "API"; }


        /** Default Constructor **/
        APINode();


        /** Constructor **/
        APINode(const LLP::Socket &SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        APINode(LLP::DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~APINode();


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;


        /** ErrorReply
         *
         *  Handles a reply error code and response.
         *
         *  @param[in] jsonError The error object.
         *
         **/
        void ErrorReply(const json::json& jsonError);


        /** Authorized
         *
         *  Check if an authorization base64 encoded string is correct.
         *
         *  @param[in] mapHeaders The list of headers to check.
         *
         *  @return True if this connection is authorized.
         *
         **/
        bool Authorized(std::map<std::string, std::string>& mapHeaders);

    };
}

#endif
