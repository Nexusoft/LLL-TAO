/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLP/types/httpnode.h>

namespace LLP
{
    /** FileNode
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
    class FileNode : public HTTPNode
    {
    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "API"; }


        /** Default Constructor **/
        FileNode();


        /** Constructor **/
        FileNode(const LLP::Socket &SOCKET_IN, LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        FileNode(LLP::DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Default Destructor **/
        virtual ~FileNode();


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
