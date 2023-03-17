/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/random.h>
#include <LLC/types/pqssl.h>

#include <LLP/packets/message.h>
#include <LLP/templates/base_connection.h>

#include <Util/types/lock_unique_ptr.h>

namespace LLP
{

    class RelayNode : public BaseConnection<MessagePacket>
    {

        /** Create a context to track SSL related data. **/
        LLC::PQSSL_CTX* pqSSL;


        /** Crypto Object Register of Peer for Verification. **/
        TAO::Register::Crypto oCrypto;


        /** Internal map to track RTR's that are servicing each user-id. **/
        util::atomic::lock_unique_ptr<std::multimap<uint256_t, LLP::BaseAddress>> mapRoutingTable;


    public:

        /** Requests are core functions to ask for response. **/
        struct REQUEST
        {
            enum : MessagePacket::message_t
            {
                /* Object Types. */
                RESERVED1     = 0x00,

                HANDSHAKE     = 0x01, //establish a new handshake
                SESSION       = 0x02, //establish a new session
                FIND          = 0x03, //find a given node by genesis-id
                LIST          = 0x04, //list active datatypes
                PING          = 0x05,
                COMMAND       = 0x06,

                RESERVED2     = 0x08,
            };

            /** VALID
             *
             *  Inline function to check if message request is valid request.
             *
             *  @param[in] nMsg The message value to check if valid.
             *
             *  @return true if the request was in range.
             */
            static inline bool VALID(const MessagePacket::message_t nMsg)
            {
                return (nMsg > RESERVED1 && nMsg < RESERVED2);
            }
        };

        /** A response is a given message response to requests. **/
        struct RESPONSE
        {
            enum : MessagePacket::message_t
            {
                HANDSHAKE       = 0x11, //respond with response data for handshake to exchange keys
                PONG            = 0x12, //pong messages give us latency and keep connection alive
                COMMAND         = 0x13, //command response passes command information back to remote host
            };
        };

        /** Relay messages are messages meant to be passed on to another available node. **/
        struct RELAY
        {
            enum : MessagePacket::message_t
            {
                AVAILABLE = 0x20, //this message is responsible for keeping the list of active nodes synced
                FORWARD   = 0x21, //this message is responsible for forwarding between nodes
            };
        };


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Relay"; }


        /** Constructor **/
        RelayNode();


        /** Constructor **/
        RelayNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        RelayNode(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /* Virtual destructor. */
        virtual ~RelayNode();


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket();


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] nMsg The message type.
         *  @param[in] ssData A datastream object with data to write.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        MessagePacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            /* Build our packet to send now. */
            MessagePacket RESPONSE(nMsg);

            /* Encrypt packet if we have valid context. */
            if(pqSSL && pqSSL->Completed())
            {
                /* Encrypt our packet payload now. */
                std::vector<uint8_t> vCipherText;
                pqSSL->Encrypt(ssData.Bytes(), vCipherText);

                /* Set our packet payload. */
                RESPONSE.DATA = vCipherText;
            }

            /* Otherwise no encryption set data like normal. */
            else
                RESPONSE.DATA = ssData.Bytes();

            return RESPONSE;
        }


        /** PushMessage
         *
         *  Adds a relay packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg, const std::vector<uint8_t>& vData)
        {
            /* Build our packet to send now. */
            MessagePacket RESPONSE(nMsg);

            /* Encrypt packet if we have valid context. */
            if(pqSSL && pqSSL->Completed())
            {
                /* Encrypt our packet payload now. */
                std::vector<uint8_t> vCipherText;
                pqSSL->Encrypt(vData, vCipherText);

                /* Set our packet payload. */
                RESPONSE.DATA = vCipherText;
            }

            /* Otherwise no encryption set data like normal. */
            else
                RESPONSE.DATA = vData;

            /* Write the packet to our pipe. */
            WritePacket(RESPONSE);

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, vData.size(), " bytes");
        }


        /** PushMessage
         *
         *  Adds a relay packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            /* Write the packet to our pipe. */
            WritePacket(NewMessage(nMsg, ssData));

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void PushMessage(const uint16_t nMsg, Args&&... args)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ((ssData << args), ...);

            WritePacket(NewMessage(nMsg, ssData));

            /* We want to track verbose to save some copies into log buffers. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }
    };
}
