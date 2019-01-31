/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_LEGACY_H
#define NEXUS_LLP_TYPES_LEGACY_H

#include <Legacy/types/locator.h>

#include <LLP/include/legacyaddress.h>
#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/legacy.h>
#include <LLP/templates/connection.h>

namespace LLP
{
    extern LegacyAddress addrMyNode; //TODO: move this to a better location

    /** LegacyNode
     *
     *  A Node that processes packets and messages for the Legacy Server
     *
     **/
    class LegacyNode : public BaseConnection<LegacyPacket>
    {
    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Legacy"; }


        /** Process
         *
         *  Verify a block and accept it into the block chain
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        static bool Process(const Legacy::LegacyBlock& block, LegacyNode* pnode);


        /** Default Constructor **/
        LegacyNode()
        : BaseConnection<LegacyPacket>()
        , nSessionID(0)
        , strNodeVersion()
        , nCurrentVersion(LLP::PROTOCOL_VERSION)
        , nStartingHeight(0)
        , fInbound(false)
        , nNodeLatency(std::numeric_limits<uint32_t>::max())
        , nLastPing(runtime::timestamp())
        , nConsecutiveTimeouts(0)
        , hashContinue(0)
        , mapLatencyTracker()
        , mapSentRequests()
        {

        }


        /** Constructor **/
        LegacyNode(Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<LegacyPacket>(SOCKET_IN, DDOS_IN)
        , nSessionID(0)
        , strNodeVersion()
        , nCurrentVersion(LLP::PROTOCOL_VERSION)
        , nStartingHeight(0)
        , fInbound(false)
        , nNodeLatency(std::numeric_limits<uint32_t>::max())
        , nLastPing(runtime::timestamp())
        , nConsecutiveTimeouts(0)
        , hashContinue(0)
        , mapLatencyTracker()
        , mapSentRequests()
        {

        }


        /** Randomly genearted session ID. **/
        uint64_t nSessionID;


        /** String version of this Node's Version. **/
        std::string strNodeVersion;


        /** The current Protocol Version of this Node. **/
        uint32_t nCurrentVersion;


        /** LEGACY: The height of this ndoe given at the version message. **/
        uint32_t nStartingHeight;


        /** Flag to determine if a connection is Inbound. **/
        bool fInbound;


        /** Latency in Milliseconds to determine a node's reliability. **/
        uint32_t nNodeLatency; //milli-seconds


        /** Counter to keep track of the last time a ping was made. **/
        uint32_t nLastPing;


        /** The last getblocks call this node has received. **/
        static uint1024_t hashLastGetblocks;


        /** The time since last getblocks call. **/
        static uint64_t nLastGetBlocks;


        /** The number of times getblocks has timed out (to deal with unreliable NON-TRITIUM nodes). **/
        uint32_t nConsecutiveTimeouts;


        /** The trigger hash to send a continue inv message to remote node. **/
        uint1024_t hashContinue;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** Map to keep track of sent request ID's while witing for them to return. **/
        std::map<uint32_t, uint64_t> mapSentRequests;


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


        /** PushVersion
         *
         *  Handle for version message
         *
         **/
        void PushVersion();


        /** PushAddress
         *
         *  Send an LegacyAddress to Node.
         *
         *  @param[in] addr The address to send to nodes
         *
         **/
        void PushAddress(const LegacyAddress& addr);


        /** DoS
         *
         *  Send the DoS Score to DDOS Filte
         *
         *  @param[in] nDoS The score to add for DoS banning
         *  @param[in] fReturn The value to return (False disconnects this node)
         *
         **/
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
                if(INCOMING.IsNull() && Available() >= 24)
                {
                    std::vector<uint8_t> BYTES(24, 0);
                    if(Read(BYTES, 24) == 24)
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


        /** Push Get Blocks
         *
         *  Send a request to get recent inventory from remote node.
         *
         *  @param[in] hashBlockFrom The block to start from
         *  @param[in] hashBlockTo The block to search to
         *
         **/
        void PushGetBlocks(const uint1024_t& hashBlockFrom, const uint1024_t& hashBlockTo)
        {
            /* Filter out duplicate requests. */
            if(hashLastGetblocks == hashBlockFrom && nLastGetBlocks + 10 > runtime::timestamp())
                return;

            /* Update the last timestamp this was called. */
            nLastGetBlocks = runtime::timestamp();

            /* Update the hash that was used for last request. */
            hashLastGetblocks = hashBlockFrom;

            /* Push the request to the node. */
            PushMessage("getblocks", Legacy::Locator(hashBlockFrom), hashBlockTo);

            /* Debug output for monitoring. */
            debug::log(0, NODE, "requesting getblocks from ", hashBlockFrom.ToString().substr(0, 20), " to ", hashBlockTo.ToString().substr(0, 20));
        }


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] chCommand Commands to add to the legacy packet.
         *  @param[in] ssData A datastream object with data to write.
         *
         *  @return Returns a filled out legacy packet.
         *
         **/
        LegacyPacket NewMessage(const char* chCommand, const DataStream& ssData)
        {
            LegacyPacket RESPONSE(chCommand);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }

        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         *  @param[in] chCommand Commands to add to the legacy packet.
         *
         **/
        void PushMessage(const char* chCommand)
        {
            LegacyPacket RESPONSE(chCommand);
            RESPONSE.SetChecksum();

            this->WritePacket(RESPONSE);
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1>
        void PushMessage(const char* chMessage, const T1& t1)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8;

            this->WritePacket(NewMessage(chMessage, ssData));
        }


        /** PushMessage
         *
         *  Adds a legacy packet to the queue to write to the socket.
         *
         **/
        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
        void PushMessage(const char* chMessage, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, const T9& t9)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ssData << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8 << t9;

            this->WritePacket(NewMessage(chMessage, ssData));
        }

    };

}

#endif
