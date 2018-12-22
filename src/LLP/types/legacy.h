/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_LEGACY_H
#define NEXUS_LLP_TYPES_LEGACY_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/legacy.h>
#include <LLP/templates/connection.h>

namespace LLP
{
    extern Address addrMyNode; //TODO: move this to a better location

    class LegacyNode : public BaseConnection<LegacyPacket>
    {
        Address addrThisNode;

    public:

        static std::string Name() { return "Legacy"; }

        /* Constructors for Message LLP Class. */
        LegacyNode() : BaseConnection<LegacyPacket>() {}

        LegacyNode(Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<LegacyPacket>(SOCKET_IN, DDOS_IN) { }


        /** Randomly genearted session ID. **/
        uint64_t nSessionID;


        /** String version of this Node's Version. **/
        std::string strNodeVersion;


        /** The current Protocol Version of this Node. **/
        int nCurrentVersion;


        /** LEGACY: The height of this ndoe given at the version message. **/
        int nStartingHeight;


        /** Flag to determine if a connection is Inbound. **/
        bool fInbound;


        /** Latency in Milliseconds to determine a node's reliability. **/
        uint32_t nNodeLatency; //milli-seconds


        /** Counter to keep track of the last time a ping was made. **/
        uint32_t nLastPing;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** Mao to keep track of sent request ID's while witing for them to return. **/
        std::map<uint32_t, uint64_t> mapSentRequests;


        /** Virtual Functions to Determine Behavior of Message LLP.
        *
        * @param[in] EVENT The byte header of the event type
        * @param[in[ LENGTH The size of bytes read on packet read events
        *
        */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise
         *
         **/
        bool ProcessPacket();


        /** Handle for version message **/
        void PushVersion();


        /** Send an Address to Node.
        *
        * @param[in] addr The address to send to nodes
        *
        */
        void PushAddress(const Address& addr);


        /** Send the DoS Score to DDOS Filte
        *
        * @param[in] nDoS The score to add for DoS banning
        * @param[in] fReturn The value to return (False disconnects this node)
        *
        */
        inline bool DoS(int nDoS, bool fReturn)
        {
            if(fDDOS)
                DDOS->rSCORE += nDoS;

            return fReturn;
        }


        /** Get the current IP address of this node. **/
        //Address GetAddress();


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket()
        {
            if(!INCOMING.Complete())
            {
                /** Handle Reading Packet Length Header. **/
                if(INCOMING.IsNull() && SOCKET.Available() >= 24)
                {
                    std::vector<uint8_t> BYTES(24, 0);
                    if(Read(BYTES, 24) == 24)
                    {
                        CDataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                        ssHeader >> INCOMING;

                        Event(EVENT_HEADER);
                    }
                }

                /** Handle Reading Packet Data. **/
                uint32_t nAvailable = SOCKET.Available();
                if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
                {
                    /* Create the packet data object. */
                    std::vector<uint8_t> DATA( std::min( std::min(512u, nAvailable), (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                    /* Read up to 512 bytes of data. */
                    if(Read(DATA, DATA.size()) == DATA.size())
                    {
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                        Event(EVENT_PACKET, DATA.size());
                    }
                }
            }
        }


        LegacyPacket NewMessage(const char* chCommand, CDataStream ssData)
        {
            LegacyPacket RESPONSE(chCommand);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }


        void PushMessage(const char* chCommand)
        {
            try
            {
                LegacyPacket RESPONSE(chCommand);
                RESPONSE.SetChecksum();

                this->WritePacket(RESPONSE);
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1>
        void PushMessage(const char* chMessage, const T1& t1)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4 << t5;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4 << t5 << t6;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9)
        {
            try
            {
                CDataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
                ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8 << t9;

                this->WritePacket(NewMessage(chMessage, ssData));
            }
            catch(...)
            {
                throw;
            }
        }

    };

}

#endif
