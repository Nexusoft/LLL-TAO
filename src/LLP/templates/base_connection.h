/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

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

#include <Util/include/args.h>
#include <Util/include/mutex.h>
#include <Util/templates/datastream.h>

#include <vector>
#include <queue>
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

        /** IsTimeoutExempt
         *
         *  Virtual method to determine if this connection should be exempted from
         *  socket read-idle timeout disconnection in the DataThread polling loop.
         *
         *  Mining connections override this to return true when the miner has
         *  completed Falcon authentication. This prevents the socket-level
         *  DISCONNECT::TIMEOUT from killing authenticated miners that are
         *  legitimately idle (e.g., during long Prime block searches).
         *
         *  The session-level keepalive timeout (24 hours) remains the authority
         *  for session expiration; the socket timeout is bypassed.
         *
         *  @return true if connection should bypass socket read timeout, false otherwise.
         *
         **/
        virtual bool IsTimeoutExempt() const { return false; }


        /** GetWriteTimeout
         *
         *  Virtual method to return the write-stall timeout in milliseconds.
         *  The DataThread uses this to decide when to disconnect a connection
         *  whose send buffer has pending data but no successful Flush() has
         *  completed within the timeout period (DISCONNECT::TIMEOUT_WRITE).
         *
         *  The default is 5 000 ms (5 seconds), suitable for P2P connections.
         *
         *  Mining connections override this to return a longer timeout
         *  (default 30 000 ms) because miners may temporarily stop reading
         *  during CPU-intensive proof-of-work computation, causing the TCP
         *  receive window to close.  A 5-second write stall is normal during
         *  heavy hashing or fork resolution bursts.
         *
         *  @return write-stall timeout in milliseconds.
         *
         **/
        virtual uint32_t GetWriteTimeout() const
        {
            return config::GetArg("-writetimeout", 5000);
        }


        /** GetMaxSendBuffer
         *
         *  Virtual method to return the maximum send buffer size for this
         *  connection.  The DataThread uses this to decide when to disconnect
         *  a connection whose send buffer has overflowed, and WritePacket()
         *  uses it to decide when to drop outgoing packets.
         *
         *  Mining connections override this to return a much larger limit
         *  (15 MB default) when the miner is authenticated, because push
         *  notifications are the primary — not advisory — mechanism for
         *  delivering fresh work to miners.  A slow reader must not trigger
         *  DISCONNECT::BUFFER merely because it is busy hashing.
         *
         *  Unauthenticated connections inherit the default (3 MB) to prevent
         *  resource abuse before Falcon authentication completes.
         *
         *  @return maximum send buffer size in bytes for this connection.
         *
         **/
        virtual uint64_t GetMaxSendBuffer() const
        {
            return config::GetArg("-maxsendbuffer", MAX_SEND_BUFFER);
        }


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


        /** QueuePacket
         *
         *  Enqueues a pre-built packet for deferred sending by FLUSH_THREAD.
         *  This decouples the caller (e.g., SendChannelNotification on the
         *  block-acceptance notification thread) from SOCKET_MUTEX contention.
         *  The caller builds the packet and enqueues it lock-free (relative to
         *  the socket); FLUSH_THREAD drains the queue on its next iteration,
         *  performing the actual WritePacket() + Flush() under SOCKET_MUTEX
         *  without blocking the notification thread or the DataThread's
         *  ReadPacket() path.
         *
         *  @param[in] PACKET The packet to enqueue for deferred sending.
         *
         **/
        void QueuePacket(const PacketType& PACKET)
        {
            {
                LOCK(OUTGOING_MUTEX);
                OUTGOING_QUEUE.push(PACKET);
            }

            /* Set atomic flag so HasQueuedPackets() can check lock-free. */
            fHasOutgoing.store(true, std::memory_order_release);

            /* Wake FLUSH_THREAD to drain the queue. */
            if(FLUSH_CONDITION)
                FLUSH_CONDITION->notify_all();
        }


        /** DrainOutgoingQueue
         *
         *  Called by FLUSH_THREAD to drain all queued outgoing packets.
         *  Each packet is written via WritePacket() which serializes it
         *  and hands it to Socket::Write().
         *
         *  @return the number of packets drained.
         *
         **/
        uint32_t DrainOutgoingQueue()
        {
            uint32_t nDrained = 0;

            /* Grab all packets under lock, then release lock before writing.
             * This minimizes contention between QueuePacket() callers and
             * the FLUSH_THREAD drain path.  Use move-assignment to transfer
             * ownership of queue contents efficiently. */
            std::queue<PacketType> qLocal;
            {
                LOCK(OUTGOING_MUTEX);
                qLocal = std::move(OUTGOING_QUEUE);

                /* Reset the underlying queue to a clean empty state after move. */
                OUTGOING_QUEUE = std::queue<PacketType>();
            }

            /* Clear atomic flag now that the queue is empty under lock.
             * A concurrent QueuePacket() may re-set it immediately, which
             * is correct — the next FLUSH_THREAD iteration will drain it. */
            fHasOutgoing.store(false, std::memory_order_release);

            /* Write each packet — WritePacket() handles buffering and
             * SOCKET_MUTEX acquisition internally. */
            while(!qLocal.empty())
            {
                WritePacket(qLocal.front());
                qLocal.pop();
                ++nDrained;
            }

            return nDrained;
        }


        /** HasQueuedPackets
         *
         *  Lock-free check for queued outgoing packets.
         *  Uses an atomic flag updated by QueuePacket() and DrainOutgoingQueue()
         *  to avoid mutex contention in the FLUSH_THREAD wait predicate
         *  (which evaluates for every connection on every wakeup).
         *
         *  @return true if the outgoing queue is likely non-empty.
         *
         **/
        bool HasQueuedPackets() const
        {
            return fHasOutgoing.load(std::memory_order_acquire);
        }


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


        /** Mutex to protect the outgoing packet queue. **/
        std::mutex OUTGOING_MUTEX;


        /** Queue of packets enqueued by QueuePacket() for deferred
         *  sending by FLUSH_THREAD via DrainOutgoingQueue(). **/
        std::queue<PacketType> OUTGOING_QUEUE;


        /** Lock-free flag for HasQueuedPackets() — avoids mutex contention
         *  in the FLUSH_THREAD wait predicate (evaluated for every connection
         *  on every wakeup; with 500+ connections this saves ~500 mutex
         *  acquisitions per predicate evaluation). **/
        std::atomic<bool> fHasOutgoing{false};


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
         *  Release an event listener from tirggers.
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
         *  Notify connection an event occured to wake up a sleeping connection.
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
