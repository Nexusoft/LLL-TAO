/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_TRITIUM_H
#define NEXUS_LLP_TYPES_TRITIUM_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/tritium.h>
#include <LLP/templates/connection.h>

namespace LLP
{

    class TritiumNode : public BaseConnection<TritiumPacket>
    {
    public:

        static std::string Name() { return "Tritium"; }

        /* Constructors for Message LLP Class. */
        TritiumNode()
        : BaseConnection<TritiumPacket>()
        , nSessionID(0), fInbound(false)
        , nNodeLatency(0)
        , nLastPing(0)
        , nLastSamples(0) {}

        TritiumNode( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<TritiumPacket>( SOCKET_IN, DDOS_IN )
        , nSessionID(0)
        , fInbound(false)
        , nNodeLatency(0)
        , nLastPing(0)
        , nLastSamples(0) { }


        /** Randomly genearted session ID. **/
        uint64_t nSessionID;


        /** Flag to determine if a connection is Inbound. **/
        bool fInbound;


        /** Latency in Milliseconds to determine a node's reliability. **/
        uint32_t nNodeLatency; //milli-seconds


        /** Counter to keep track of the last time a ping was made. **/
        uint32_t nLastPing;


        /** Counter to keep track of last time sample request. */
        uint32_t nLastSamples;


        /** timer object to keep track of ping latency. **/
        std::map<uint32_t, uint64_t> mapLatencyTracker;


        /** Map to keep track of sent request ID's while witing for them to return. **/
        std::map<uint32_t, uint64_t> mapSentRequests;


        /** Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type
         *  @param[in[ LENGTH The size of bytes read on packet read events
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise
         *
         **/
        bool ProcessPacket() final;


        /** DoS
         *
         *  Send the DoS Score to DDOS Filte
         *
         *  @param[in] nDoS The score to add for DoS banning
         *  @param[in] fReturn The value to return (False disconnects this node)
         *
         *  @return fReturn
         *
         */
        inline bool DoS(int nDoS, bool fReturn)
        {
            if(fDDOS)
                DDOS->rSCORE += nDoS;

            return fReturn;
        }


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final
        {
            if(!INCOMING.Complete())
            {
                /** Handle Reading Packet Length Header. **/
                if(INCOMING.IsNull() && Available() >= 10)
                {
                    std::vector<uint8_t> BYTES(10, 0);
                    if(Read(BYTES, 10) == 10)
                    {
                        DataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                        ssHeader >> INCOMING;

                        Event(EVENT_HEADER);
                    }
                }

                /** Handle Reading Packet Data. **/
                uint32_t nAvailable = Available();
                if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
                {
                    /* Create the packet data object. */
                    std::vector<uint8_t> DATA( std::min( nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                    /* Read up to 512 bytes of data. */
                    if(Read(DATA, DATA.size()) == DATA.size())
                    {
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                        Event(EVENT_PACKET, DATA.size());
                    }
                }
            }
        }


        TritiumPacket NewMessage(const uint16_t nMsg, DataStream ssData)
        {
            TritiumPacket RESPONSE(nMsg);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }


        void PushMessage(const uint16_t nMsg)
        {
            TritiumPacket RESPONSE(nMsg);
            RESPONSE.SetChecksum();

            this->WritePacket(RESPONSE);
        }

        template<typename T1>
        void PushMessage(const uint16_t nMsg, const T1& t1)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3, typename T4>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
        void PushMessage(const uint16_t nMsg, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8;

            this->WritePacket(NewMessage(nMsg, ssData));
        }

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
