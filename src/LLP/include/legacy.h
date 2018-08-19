/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_LEGACY_H
#define NEXUS_LLP_INCLUDE_LEGACY_H

#include <queue>
#include <vector>

#include "network.h"
#include "version.h"

#include "../templates/types.h"

#include "../../LLC/hash/SK.h"


namespace LLP
{
    extern CAddress addrMyNode;


    /* Message Packet Leading Bytes. */
    const uint8_t MESSAGE_START_TESTNET[4] = { 0xe9, 0x59, 0x0d, 0x05 };
    const uint8_t MESSAGE_START_MAINNET[4] = { 0x05, 0x0d, 0x59, 0xe9 };


    /* Used to Lock-Out Nodes that are running a protocol version that are too old. */
    const int MIN_PROTO_VERSION = 10000;


    /** Class to handle sending and receiving of More Complese Message LLP Packets. **/
    class LegacyPacket
    {
    public:

        /*
        * Components of a Message LLP Packet.
        * BYTE 0 - 4    : Start
        * BYTE 5 - 17   : Message
        * BYTE 18 - 22  : Size
        * BYTE 23 - 26  : Checksum
        * BYTE 26 - X   : Data
        *
        */
        uint8_t	HEADER[4];
        char			MESSAGE[12];
        uint32_t	LENGTH;
        uint32_t	CHECKSUM;

        std::vector<uint8_t> DATA;

        LegacyPacket()
        {
            SetNull();
            SetHeader();
        }

        LegacyPacket(const char* chMessage)
        {
            SetNull();
            SetHeader();
            SetMessage(chMessage);
        }

        IMPLEMENT_SERIALIZE
        (
            READWRITE(FLATDATA(HEADER));
            READWRITE(FLATDATA(MESSAGE));
            READWRITE(LENGTH);
            READWRITE(CHECKSUM);
        )


        /* Set the Packet Null Flags. */
        void SetNull()
        {
            LENGTH    = 0;
            CHECKSUM  = 0;
            SetMessage("");

            DATA.clear();
        }


        /* Get the Command of packet in a std::string type. */
        std::string GetMessage()
        {
            return std::string(MESSAGE, MESSAGE + strlen(MESSAGE));
        }


        /* Packet Null Flag. Length and Checksum both 0. */
        bool IsNull() { return (std::string(MESSAGE) == "" && LENGTH == 0 && CHECKSUM == 0); }


        /* Determine if a packet is fully read. */
        bool Complete() { return (Header() && DATA.size() == LENGTH); }


        /* Determine if header is fully read */
        bool Header()   { return IsNull() ? false : (CHECKSUM > 0 && std::string(MESSAGE) != ""); }


        /* Set the first four bytes in the packet headcer to be of the byte series selected. */
        void SetHeader()
        {
            if (fTestNet)
                memcpy(HEADER, MESSAGE_START_TESTNET, sizeof(MESSAGE_START_TESTNET));
            else
                memcpy(HEADER, MESSAGE_START_MAINNET, sizeof(MESSAGE_START_MAINNET));
        }


        /* Set the message in the packet header. */
        void SetMessage(const char* chMessage)
        {
            strncpy(MESSAGE, chMessage, 12);
        }


        /* Sets the size of the packet from Byte Vector. */
        void SetLength(std::vector<uint8_t> BYTES)
        {
            CDataStream ssLength(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
            ssLength >> LENGTH;
        }


        /* Set the Packet Checksum Data. */
        void SetChecksum()
        {
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            //std::copy(hash, hash + sizeof(CHECKSUM), CHECKSUM);
            memcpy(&CHECKSUM, &hash, sizeof(CHECKSUM));
        }


        /* Set the Packet Data. */
        void SetData(CDataStream ssData)
        {
            std::vector<uint8_t> vData(ssData.begin(), ssData.end());

            LENGTH = vData.size();
            DATA   = vData;

            SetChecksum();
        }


        /* Check the Validity of the Packet. */
        bool IsValid()
        {
            /* Check that the packet isn't NULL. */
            if(IsNull())
                return false;

            /* Check the Header Bytes. */
            if(memcmp(HEADER, (fTestNet ? MESSAGE_START_TESTNET : MESSAGE_START_MAINNET), sizeof(HEADER)) != 0)
                return error("Message Packet (Invalid Packet Header");

            /* Make sure Packet length is within bounds. (Max 512 MB Packet Size) */
            if (LENGTH > (1024 * 1024 * 512))
                return error("Message Packet (%s, %u bytes) : Message too Large", MESSAGE, LENGTH);

            /* Double check the Message Checksum. */
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            uint32_t nChecksum = 0;
            //std::copy(hash.begin(), hash.begin() + sizeof(nChecksum), &nChecksum);
            memcpy(&nChecksum, &hash, sizeof(nChecksum));

            if (nChecksum != CHECKSUM)
                return error("Message Packet (%s, %u bytes) : CHECKSUM MISMATCH nChecksum=%u hdr.nChecksum=%u",
                MESSAGE, LENGTH, nChecksum, CHECKSUM);

            return true;
        }


        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes()
        {
            CDataStream ssHeader(SER_NETWORK, MIN_PROTO_VERSION);
            ssHeader << *this;

            std::vector<uint8_t> BYTES(ssHeader.begin(), ssHeader.end());
            BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());
            return BYTES;
        }
    };



    class LegacyNode : public BaseConnection<LegacyPacket>
    {
        CAddress addrThisNode;

    public:

        /* Constructors for Message LLP Class. */
        LegacyNode() : BaseConnection<LegacyPacket>() {}
        LegacyNode( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false ) : BaseConnection<LegacyPacket>( SOCKET_IN, DDOS_IN ) { }


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


        /** Timer object to keep track of ping latency. **/
        std::map<uint64_t, Timer> mapLatencyTracker;


        /** Mao to keep track of sent request ID's while witing for them to return. **/
        std::map<uint32_t, uint64_t> mapSentRequests;


        /** Virtual Functions to Determine Behavior of Message LLP.
        *
        * @param[in] EVENT The byte header of the event type
        * @param[in[ LENGTH The size of bytes read on packet read events
        *
        */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** Main message handler once a packet is recieved. **/
        bool ProcessPacket();


        /** Handle for version message **/
        void PushVersion();


        /** Send an Address to Node.
        *
        * @param[in] addr The address to send to nodes
        *
        */
        void PushAddress(const CAddress& addr);


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
        CAddress GetAddress();


        /** Non-Blocking Packet reader to build a packet from TCP Connection.
        * This keeps thread from spending too much time for each Connection.
        */
        virtual void ReadPacket()
        {
            if(!INCOMING.Complete())
            {
                /** Handle Reading Packet Length Header. **/
                if(SOCKET->available() >= 24 && INCOMING.IsNull())
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
                uint32_t nAvailable = SOCKET->available();
                if(nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
                {
                    std::vector<uint8_t> DATA( std::min(std::min(nAvailable, 512u), (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);
                    uint32_t nRead = Read(DATA, DATA.size());

                    if(nRead == DATA.size())
                    {
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                        Event(EVENT_PACKET, nRead);
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
