/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_TRITIUM_H
#define NEXUS_LLP_TYPES_TRITIUM_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/tritium.h>
#include <LLP/templates/base_connection.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <TAO/Ledger/types/tritium.h>

namespace LLP
{

    /** Actions invoke behavior in remote node. **/
    namespace ACTION
    {
        enum
        {
            RESERVED     = 0,

            /* Verbs. */
            LIST         = 0x10,
            GET          = 0x11,
            NOTIFY       = 0x12,
            AUTH         = 0x13,

            /* Protocol. */
            PING         = 0x1a

        };
    }


    /** Types are objects that can be sent in packets. **/
    namespace TYPES
    {
        enum
        {
            /* Key Types. */
            UINT256_T   = 0x20,
            UINT512_T   = 0x21,
            UINT1024_T  = 0x22,
            STRING      = 0x23,
            BYTES       = 0x24,

            /* Object Types. */
            BLOCK       = 0x30,
            TRANSACTION = 0x31,
            TIMESEED    = 0x32,

            /* Specifier. */
            LEGACY      = 0x3a
        };
    }


    /** Status returns available states. **/
    namespace RESPONSE
    {
        enum
        {
            ACCEPTED    = 0x40,
            REJECTED    = 0x41,
            STALE       = 0x42,
            PONG        = 0x43,
        };
    }


    /** TritiumNode
     *
     *  A Node that processes packets and messages for the Tritium Server
     *
     **/
    class TritiumNode : public BaseConnection<TritiumPacket>
    {
    public:

      /** Name
       *
       *  Returns a string for the name of this type of Node.
       *
       **/
        static std::string Name() { return "Tritium"; }


        /** Default Constructor **/
        TritiumNode();


        /** Constructor **/
        TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Constructor **/
        TritiumNode(DDOS_Filter* DDOS_IN, bool isDDOS = false);


        /** Default Destructor **/
        virtual ~TritiumNode();


        /** Counter to keep track of the last time a ping was made. **/
        std::atomic<uint64_t> nLastPing;


        /** Counter to keep track of last time sample request. */
        std::atomic<uint64_t> nLastSamples;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


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


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final;


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
        TritiumPacket NewMessage(const uint16_t nMsg, const DataStream &ssData)
        {
            TritiumPacket RESPONSE(nMsg);
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
            TritiumPacket RESPONSE(nMsg);
            this->WritePacket(RESPONSE);
        }

        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1>
        void PushMessage(const uint16_t nMsg, const T1& t1)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8;

            this->WritePacket(NewMessage(nMsg, ssData));
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8 << t9;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

    };

}

#endif
