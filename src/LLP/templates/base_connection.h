/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_BASE_CONNECTION_H
#define NEXUS_LLP_TEMPLATES_BASE_CONNECTION_H

#include <LLP/templates/socket.h>
#include <LLP/templates/trigger.h>
#include <LLP/include/version.h>

#include <Util/include/mutex.h>
#include <Util/templates/datastream.h>

#include <vector>
#include <condition_variable>

namespace LLP
{

    /* Forward declarations. */
    class DDOS_Filter;


    /** BaseConnection
     *
     *  Base Template class to handle outgoing / incoming LLP data for both Client and Server.
     *
     **/
    template <class PacketType>
    class BaseConnection : public Socket
    {
    public:

        /** The packet message type. **/
        typedef typename PacketType::message_t message_t;


        /** The template packet type. */
        typedef PacketType packet_t;

    protected:

        /** Event
         *
         *  Pure Virtual Event Function to be Overridden allowing Custom Read Events.
         *  Each event fired on Header Complete, and each time data is read to fill packet.
         *  Useful to check Header length to maximum size of packet type for DDOS protection,
         *  sending a keep-alive ping while downloading large files, etc.
         *
         *  LENGTH == 0: General Events
         *  LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
         *
         **/
        virtual void Event(uint8_t EVENT, uint32_t LENGTH = 0) = 0;


        /** ProcessPacket
         *
         *  Pure Virtual Process Function. To be overridden with your own custom
         *  packet processing.
         *
         **/
        virtual bool ProcessPacket() = 0;


    public:

        /** Incoming Packet Being Built. **/
        PacketType     INCOMING;


        /** DDOS Score for Connection. **/
        DDOS_Filter*   DDOS;


        /** Latency in Milliseconds to determine a node's reliability. **/
        std::atomic<uint32_t> nLatency; //milli-seconds


        /** Flag to Determine if DDOS is Enabled. **/
        std::atomic<bool> fDDOS;


        /** Flag to Determine if the connection was made by this Node. **/
        std::atomic<bool> fOUTGOING;


        /** Flag to determine if the connection is active. **/
        std::atomic<bool> fCONNECTED;


        /** Index for the current data thread processing. **/
        int32_t nDataThread;


        /** Index for the current slot in data thread. **/
        int32_t nDataIndex;


        /** Condition variable pointer from data thread. **/
        std::condition_variable* FLUSH_CONDITION;


        /** Total incoming packets. **/
        static std::atomic<uint64_t> REQUESTS;


        /** Total outgoing packets. **/
        static std::atomic<uint64_t> PACKETS;


        /** Total incoming connections. **/
        static std::atomic<uint64_t> CONNECTIONS;
        

        /** Total disconnections. **/
        static std::atomic<uint64_t> DISCONNECTS;


    private:

        /** Flag to determine if event occurred. **/
        std::atomic<bool> fEVENT;


        /** Mutex used for event condition variable **/
        std::mutex EVENT_MUTEX;


        /** event condition variable to wake up sleeping connection. **/
        std::condition_variable EVENT_CONDITION;


        /** Mutex to protect events triggers. **/
        std::mutex TRIGGER_MUTEX;


        /** Special foreign triggers for connection. **/
        std::map<message_t, Trigger*> TRIGGERS;


    public:


        /** Build Base Connection with no parameters **/
        BaseConnection();


        /** Build Base Connection with all Parameters. **/
        BaseConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /** Build Base Connection with all Parameters. **/
        BaseConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /* Default destructor */
        virtual ~BaseConnection();


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Base"; }


        /** Notifications
         *
         *  Filter out relay requests with notifications node is subscribed to.
         *
         **/
        template<typename MessageType>
        const DataStream RelayFilter(const MessageType& message, const DataStream& ssData) const
        {
            return ssData; //copy over relay like normal for all items to be relayed
        }


        /** AddTrigger
         *
         *  Adds a new event listener to this connection to fire off condition variables on specific message types.
         *
         *  @param[in] nMsg The message type.
         *  @param[in] EVENT_CONDITION The condition variable to set to triggers.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        void AddTrigger(const message_t nMsg, Trigger* TRIGGER);


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void TriggerEvent(const message_t nMsg, Args&&... args)
        {
            LOCK(TRIGGER_MUTEX);

            /* Notify trigger if found. */
            if(TRIGGERS.count(nMsg))
            {
                /* Grab the trigger and check for nullptr. */
                Trigger* pTrigger = TRIGGERS[nMsg];
                if(pTrigger)
                {
                    /* Pass back the data to the trigger. */
                    pTrigger->SetArgs(std::forward<Args>(args)...);
                    pTrigger->notify_all();
                }
            }
        }


        /** Release
         *
         *  Release an event listener from triggers.
         *
         *  @param[in] nMsg The message type.
         *
         *
         **/
        void Release(const message_t nMsg);


        /** NotifyTriggers
         *
         *  Notify the active triggers that we are exiting sequences.
         *
         **/
        void NotifyTriggers();


        /** SetNull
         *
         *  Sets the object to an invalid state.

         *
         **/
        void SetNull();


        /** Connected
         *
         *  Connection flag to determine if socket should be handled if not connected.
         *
         **/
        bool Connected() const;


        /** Incoming
         *
         *  Flag to detect if connection is an inbound connection.
         *
         **/
        bool Incoming() const;


        /** PacketComplete
         *
         *  Handles two types of packets, requests which are of header >= 128,
         *  and data which are of header < 128.
         *
         **/
        bool PacketComplete() const;


        /** ResetPacket
         *
         *  Used to reset the packet to Null after it has been processed.
         *  This then flags the Connection to read another packet.
         *
         **/
        void ResetPacket();


        /** WritePacket
         *
         *  Write a single packet to the TCP stream.
         *
         *  @param[in] PACKET The packet of type PacketType to write.
         *
         **/
        void WritePacket(const PacketType& PACKET);


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        virtual void ReadPacket() = 0;


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] nMsg The message type.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        static PacketType NewMessage(const message_t msg)
        {
            PacketType RESPONSE(msg);
            return RESPONSE;
        }


        /** Connect
         *
         *  Connect Socket to a Remote Endpoint.
         *
         *  @param[in] addrConnect The address to connect to.
         *
         *  @return Returns true if successful connection, false otherwise.
         *
         */
        bool Connect(const BaseAddress &addrConnect);


        /** Disconnect
         *
         *  Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management.
         *
         **/
        void Disconnect();


        /** NotifyEvent
         *
         *  Notify connection an event occurred to wake up a sleeping connection.
         *
         **/
        void NotifyEvent();


        /** WaitEvent
         *
         *  Have connection wait for a notify signal to wake up.
         *
         **/
        void WaitEvent();

    };
}

#endif
