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
#include <LLD/cache/binary_key.h>
#include <TAO/Ledger/types/locator.h>
#include <Util/include/memory.h>
#include <LLP/templates/ddos.h>
#include <TAO/Ledger/types/tritium.h>

namespace LLP
{

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
        TritiumNode()
        : BaseConnection<TritiumPacket>()
        , nLastPing(0)
        , nLastSamples(0)
        , mapLatencyTracker()
        , mapSentRequests()
        , hashContinue(0)
        , nConsecutiveFails(0)
        , nConsecutiveOrphans(0)
        , fInbound(false)
        {
        }

        /** Constructor **/
        TritiumNode( Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<TritiumPacket>( SOCKET_IN, DDOS_IN, isDDOS )
        , nLastPing(0)
        , nLastSamples(0)
        , mapLatencyTracker()
        , mapSentRequests()
        , hashContinue(0)
        , nConsecutiveFails(0)
        , nConsecutiveOrphans(0)
        , fInbound(false)
        {
        }


        /** Constructor **/
        TritiumNode( DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : BaseConnection<TritiumPacket>(DDOS_IN, isDDOS )
        , nLastPing(0)
        , nLastSamples(0)
        , mapLatencyTracker()
        , mapSentRequests()
        , hashContinue(0)
        , nConsecutiveFails(0)
        , nConsecutiveOrphans(0)
        , fInbound(false)
        {
        }

        virtual ~TritiumNode()
        {
        }


        /** Randomly genearted session ID. **/
        static uint64_t nSessionID;

        /* global map connections to session ID's to be used to prevent duplicate connections to the same 
           sever, but via a different RLOC / EID */
        static std::map<uint64_t, TritiumNode*> mapSessions;

        /** Counter to keep track of the last time a ping was made. **/
        std::atomic<uint64_t> nLastPing;


        /** Counter to keep track of last time sample request. */
        std::atomic<uint64_t> nLastSamples;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** Map to keep track of sent request ID's while witing for them to return. **/
        std::map<uint64_t, uint64_t> mapSentRequests;

        /** The trigger hash to send a continue inv message to remote node. **/
        uint1024_t hashContinue;

        /* Duplicates connection reset. */
        uint32_t nConsecutiveFails;

        /* Orphans connection reset. */
        uint32_t nConsecutiveOrphans;

        /** The last getblocks call this node has received. **/
        static memory::atomic<uint1024_t> hashLastGetblocks;

        /** The time since last getblocks call. **/
        static std::atomic<uint64_t> nLastGetBlocks;

        /** Handle an average calculation of fast sync blocks. */
        static std::atomic<uint32_t> nFastSyncAverage;

        /** The current node that is being used for fast sync.l **/
        static memory::atomic<BaseAddress> addrFastSync;

        /** The last time a block was accepted. **/
        static std::atomic<uint64_t> nLastTimeReceived;

        static LLD::KeyLRU cacheInventory;


        /** Flag to determine if a connection is Inbound. **/
        bool fInbound;


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
                        Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
                    }
                }
            }
        }

        /** PushGetInventory
         *
         *  Send a request to get recent inventory from remote node.
         *
         *  @param[in] hashBlockFrom The block to start from
         *  @param[in] hashBlockTo The block to search to
         *
         **/
        void PushGetInventory(const uint1024_t& hashBlockFrom, const uint1024_t& hashBlockTo)
        {
            /* Filter out duplicate requests. */
            if(hashLastGetblocks.load() == hashBlockFrom && nLastGetBlocks.load() + 1 > runtime::timestamp())
                return;

            /* Set the fast sync address. */
            if(addrFastSync != GetAddress())
            {
                /* Set the new sync address. */
                addrFastSync = GetAddress();

                /* Reset the last time received. */
                nLastTimeReceived = runtime::timestamp();

                debug::log(0, NODE, "New sync address set");
            }

            /* Calculate the fast sync average. */
            nFastSyncAverage = std::min((uint64_t)25, (nFastSyncAverage.load() + (runtime::timestamp() - nLastGetBlocks.load())) / 2);

            /* Update the last timestamp this was called. */
            nLastGetBlocks = runtime::timestamp();

            /* Update the hash that was used for last request. */
            hashLastGetblocks = hashBlockFrom;

            /* Push the request to the node. */
            PushMessage(GET_INVENTORY, TAO::Ledger::Locator(hashBlockFrom), hashBlockTo);

            /* Debug output for monitoring. */
            debug::log(0, NODE, "(", nFastSyncAverage.load(), ") requesting getinventory from ", hashBlockFrom.ToString().substr(0, 20), " to ", hashBlockTo.ToString().substr(0, 20));
        }

        /** Process
         *
         *  Verify a block and accept it into the block chain
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        static bool Process(const TAO::Ledger::Block& block, TritiumNode* pnode);


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
            RESPONSE.SetChecksum();

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
