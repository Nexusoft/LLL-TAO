/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/random.h>

#include <LLP/packets/packet.h>
#include <LLP/templates/connection.h>

#include <Util/types/lock_unique_ptr.h>

namespace LLP
{

    class RelayNode : public BaseConnection<MessagePacket>
    {
    public:

        /** Requests are core functions to ask for response. **/
        struct REQUEST
        {
            enum : Packet::message_t
            {
                /* Object Types. */
                RESERVED1     = 0x00,

                SESSION       = 0x01, //establish a new session
                FIND          = 0x03, //find a given node by genesis-id
                LIST          = 0x04, //list active datatypes
                PING          = 0x05,

                RESERVED2     = 0x06,
            };

            /** VALID
             *
             *  Inline function to check if message request is valid request.
             *
             *  @param[in] nMsg The message value to check if valid.
             *
             *  @return true if the request was in range.
             */
            static inline bool VALID(const Packet::message_t nMsg)
            {
                return (nMsg > RESERVED1 && nMsg < RESERVED2);
            }
        };

        /** A response is a given message response to requests. **/
        struct RESPONSE
        {
            enum : Packet::message_t
            {
                HANDSHAKE       = 0x11, //respond with response data for handshake to exchange keys
            };
        };

        /** Types are available data types that this protocol can process. **/
        struct TYPES
        {
            enum : Packet::message_t
            {
                RELAY       = 0x20, //relay data is intended to pass through to recipient
            };
        }


        /** Set our static locked ptr. **/
        static util::atomic::lock_unique_ptr<std::set<uint64_t>> setRequests;


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
        static MessagePacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            MessagePacket RESPONSE(nMsg);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg)
        {
            MessagePacket RESPONSE(nMsg);
            WritePacket(RESPONSE);
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ((ssData << args), ...);

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
