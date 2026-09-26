/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/base_connection.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>

#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/packets/http.h>
#include <LLP/packets/message.h>

#include <LLP/templates/trigger.h>

#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/args.h>

#include <Util/include/runtime.h>

#include <type_traits>


namespace LLP
{

    /* Total incoming packets. */
    template <class PacketType>
    std::atomic<uint64_t> BaseConnection<PacketType>::REQUESTS;


    /* Total outgoing packets. */
    template <class PacketType>
    std::atomic<uint64_t> BaseConnection<PacketType>::PACKETS;


    /* Total connection requests. */
    template <class PacketType>
    std::atomic<uint64_t> BaseConnection<PacketType>::CONNECTIONS;


    /* Total connection requests. */
    template <class PacketType>
    std::atomic<uint64_t> BaseConnection<PacketType>::DISCONNECTS;


    /* Build Base Connection with no parameters */
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection()
    : Socket          ( )
    , INCOMING        ( )
    , DDOS            (nullptr)
    , nLatency        (std::numeric_limits<uint32_t>::max())
    , fDDOS           (false)
    , fOUTGOING       (false)
    , fCONNECTED      (false)
    , nConsecutivePollEmptyStrikes(0)
    , nDataThread     (-1)
    , nDataIndex      (-1)
    , FLUSH_CONDITION (nullptr)
    , fEVENT          (false)
    , EVENT_MUTEX     ( )
    , EVENT_CONDITION ( )
    , TRIGGER_MUTEX   ( )
    , TRIGGERS        ( )
    {
        INCOMING.SetNull();
    }

    /* Build Base Connection with all Parameters. */
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
    : Socket          (SOCKET_IN)
    , INCOMING        ( )
    , DDOS            (DDOS_IN)
    , nLatency        (std::numeric_limits<uint32_t>::max())
    , fDDOS           (fDDOSIn)
    , fOUTGOING       (fOutgoing)
    , fCONNECTED      (false)
    , nConsecutivePollEmptyStrikes(0)
    , nDataThread     (-1)
    , nDataIndex      (-1)
    , FLUSH_CONDITION (nullptr)
    , fEVENT          (false)
    , EVENT_MUTEX     ( )
    , EVENT_CONDITION ( )
    , TRIGGER_MUTEX   ( )
    , TRIGGERS        ( )
    {
    }


    /* Build Base Connection with all Parameters. */
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
    : Socket          ( )
    , INCOMING        ( )
    , DDOS            (DDOS_IN)
    , nLatency        (std::numeric_limits<uint32_t>::max())
    , fDDOS           (fDDOSIn)
    , fOUTGOING       (fOutgoing)
    , fCONNECTED      (false)
    , nConsecutivePollEmptyStrikes(0)
    , nDataThread     (-1)
    , nDataIndex      (-1)
    , FLUSH_CONDITION (nullptr)
    , fEVENT          (false)
    , EVENT_MUTEX     ( )
    , EVENT_CONDITION ( )
    , TRIGGER_MUTEX   ( )
    , TRIGGERS        ( )
    {
    }


    /* Default destructor */
    template <class PacketType>
    BaseConnection<PacketType>::~BaseConnection()
    {
        {
            LOCK(TRIGGER_MUTEX);

            /* Release all of our triggers before disconnect. */
            for(auto& rTrigger : TRIGGERS)
            {
                /* Check that the trigger was active. */
                if(rTrigger.second)
                    rTrigger.second->notify_all();
            }
        }

        /* Standard shutdown sequence. */
        Disconnect();
        SetNull();
    }


    /* Adds a new event listener to this connection to fire off condition variables on specific message types.*/
    template <class PacketType>
    void BaseConnection<PacketType>::AddTrigger(const message_t nMsg, Trigger* TRIGGER)
    {
        LOCK(TRIGGER_MUTEX);

        TRIGGERS[nMsg] = TRIGGER;
    }


    /* Release an event listener from tirggers. */
    template <class PacketType>
    void BaseConnection<PacketType>::Release(const message_t nMsg)
    {
        LOCK(TRIGGER_MUTEX);

        TRIGGERS.erase(nMsg);
    }


    /* Release an event listener from tirggers. */
    template <class PacketType>
    void BaseConnection<PacketType>::NotifyTriggers()
    {
        LOCK(TRIGGER_MUTEX);

        /* Release all of our triggers before disconnect. */
        for(auto& rTrigger : TRIGGERS)
        {
            /* Check that the trigger was active. */
            if(rTrigger.second)
                rTrigger.second->notify_all();
        }
    }


    /*  Sets the object to an invalid state. */
    template <class PacketType>
    void BaseConnection<PacketType>::SetNull()
    {
        fd              = -1;
        nError          = 0;
        DDOS            = nullptr;
        FLUSH_CONDITION = nullptr;
        fDDOS           = false;
        fOUTGOING       = false;
        fCONNECTED      = false;
        nConsecutivePollEmptyStrikes = 0;
        nDataThread     = -1;
        nDataIndex      = -1;

        INCOMING.SetNull();
    }


    /*  Connection flag to determine if socket should be handled if not connected. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Connected() const
    {
        return fCONNECTED.load();
    }

    /* Flag to detect if connection is an inbound connection. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Incoming() const
    {
        return !fOUTGOING.load();
    }


    /*  Handles two types of packets, requests which are of header >= 128,
     *  and data which are of header < 128. */
    template <class PacketType>
    bool BaseConnection<PacketType>::PacketComplete() const
    {
        return INCOMING.Complete();
    }


    /*  Used to reset the packet to Null after it has been processed.
     *  This then flags the Connection to read another packet. */
    template <class PacketType>
    void BaseConnection<PacketType>::ResetPacket()
    {
        INCOMING.SetNull();
    }


    /*  Write a single packet to the TCP stream. */
    template <class PacketType>
    bool BaseConnection<PacketType>::WritePacket(const PacketType& PACKET)
    {
        return WritePacket(PACKET, false);
    }


    template <class PacketType>
    bool BaseConnection<PacketType>::WritePacket(const PacketType& PACKET, bool fPriority)
    {
        RECURSIVE(SOCKET_MUTEX);

        if(Buffered() == 0)
            nBundleBufferLimit.store(0);

        bool fQueued = false;

        /* Per-connection buffer limit — mining connections return a larger value
         * (5 MB by default) so push notifications are not dropped under normal
         * mining pressure.  Ordinary writes are always constrained to this
         * configured maximum: the oversized-bundle allowance must not be
         * refilled by unrelated traffic as the bundle drains. */
        const uint64_t nMaxSendBuffer = GetMaxSendBuffer();

        /* Get the bytes of the packet. */
        const std::vector<uint8_t> vBytes = PACKET.GetBytes();

        /* Reserve space for critical control messages (keepalive ACK, session
         * status, round state).  Proportional to buffer size: 1% of max,
         * minimum 1 KB.  For 3 MB P2P: ~30 KB.  For 5 MB mining: ~50 KB.
         * The old hardcoded 1 KB was meaningless for large mining buffers. */
        const uint64_t nReserve = std::max(uint64_t(1024), nMaxSendBuffer / 100);

        /* While an oversized bundle occupies the queue, only small control
         * messages (e.g. LASTINDEX) may use the narrow reserve above it. */
        const uint64_t nBundleLimit = nBundleBufferLimit.load();

        /* Stop sending packets if send buffer is full. */
        if(Buffered() + vBytes.size() + nReserve < nMaxSendBuffer
        || (fBufferFull.load() && Buffered() + vBytes.size() < nMaxSendBuffer) //catch for critical messages (< reserve)
        || (fBufferFull.load() && nBundleLimit != 0 && vBytes.size() <= nReserve
            && Buffered() + vBytes.size() < nBundleLimit)) //control reserve above an active oversized bundle
        {
            /* Debug dump of message type. */
            debug::log(4, NODE, "sent packet (", vBytes.size(), " bytes)");

            /* Debug dump of packet data. */
            if(config::nVerbose >= 5)
                PrintHex(vBytes);

            /* Write the packet to socket buffer. */
            Write(vBytes, vBytes.size(), fPriority);

            /* Update packet count. */
            ++PACKETS;
            fQueued = true;
        }
        else
        {
            /* For authenticated mining connections, packet drops are critical — push
             * notifications are the primary mechanism for delivering fresh work.
             * Log at level 0 so operators can see buffer pressure issues.
             * For P2P connections, keep the existing level 4 to avoid log spam. */
            if(IsTimeoutExempt())
            {
                debug::log(0, NODE, "WARNING: Socket buffer full — packet DROPPED for authenticated miner."
                    " Packet size: ", vBytes.size(), " bytes.  Buffered: ", Buffered(),
                    " bytes.  MaxSendBuffer: ", nMaxSendBuffer, " bytes");
            }
            else
            {
                debug::log(4, NODE, "Socket buffer full. Packet size: ", vBytes.size(), " bytes.  Buffered: ", Buffered(), " bytes");
            }

            /* set buffer to full */
            fBufferFull.store(true);
        }

        /* Notify condition if available. */
        if(FLUSH_CONDITION && Buffered())
            FLUSH_CONDITION->notify_all();

        return fQueued;
    }

    template <class PacketType>
    bool BaseConnection<PacketType>::WritePackets(const std::vector<PacketType>& vPackets)
    {
        RECURSIVE(SOCKET_MUTEX);

        const uint64_t nBuffered = Buffered();
        if(nBuffered == 0)
            nBundleBufferLimit.store(0);
        else if(nBundleBufferLimit.load() != 0)
        {
            fBufferFull.store(true);
            return false;
        }

        std::vector<uint8_t> vBytes;
        if constexpr(std::is_same_v<PacketType, MessagePacket>)
        {
            /* Reject transaction bundles before making any wire-buffer copies. */
            uint64_t nBundleBytes = 0;
            for(const auto& packet : vPackets)
            {
                const uint64_t nHeader = packet.GetSerializeSize(SER_NETWORK, MIN_PROTO_VERSION);
                if(nHeader > MAX_BUNDLE_BYTES - nBundleBytes ||
                   packet.DATA.size() > MAX_BUNDLE_BYTES - nBundleBytes - nHeader)
                {
                    fBufferFull.store(true);
                    return false;
                }
                nBundleBytes += nHeader + packet.DATA.size();
            }
            vBytes.reserve(nBundleBytes);
        }

        uint64_t nReserve = 0;
        bool fOversized = false;
        for(const auto& packet : vPackets)
        {
            const auto bytes = packet.GetBytes();
            if(bytes.size() > MAX_BUNDLE_BYTES - vBytes.size())
            {
                fBufferFull.store(true);
                return false;
            }

            const uint64_t nMaxSendBuffer = GetMaxSendBuffer();
            nReserve = std::max(nReserve, std::max(uint64_t(1024), nMaxSendBuffer / 100));
            const uint64_t nRequired = nBuffered + vBytes.size() + bytes.size() + nReserve;

            /* Bulk bundles must not consume the reserve needed by LASTINDEX. */
            if(nRequired >= nMaxSendBuffer)
            {
                if(nBuffered != 0 || nMaxSendBuffer <= nReserve)
                {
                    fBufferFull.store(true);
                    return false;
                }

                /* A block's transaction bodies can exceed the normal queue
                 * limit. Admit one bounded bundle only on an empty queue. */
                fOversized = true;
            }

            vBytes.insert(vBytes.end(), bytes.begin(), bytes.end());
        }

        /* No packet reaches the socket until every member has been admitted.
         * The same lock excludes both ordinary writers and Flush(). */
        if(fOversized)
            nBundleBufferLimit.store(vBytes.size() + nReserve + 1);

        if(!vBytes.empty() && Write(vBytes, vBytes.size()) < 0)
        {
            nBundleBufferLimit.store(0);
            return false;
        }

        PACKETS.fetch_add(vPackets.size());
        if(fOversized && Buffered())
            fBufferFull.store(true);

        if(FLUSH_CONDITION && Buffered())
            FLUSH_CONDITION->notify_all();

        return true;
    }


    /*  Connect Socket to a Remote Endpoint. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Connect(const BaseAddress &addrConnect)
    {
        std::string strConnect = addrConnect.ToStringIP();

        /* Check for connect to self */
        if(addr.ToStringIP() == strConnect)
            return debug::error(NODE, "cannot self-connect");

        /* Debug information. */
        debug::log(3, NODE, "Connecting to ", strConnect);

        // Connect
        if(Attempt(addrConnect))
        {
            debug::log(3, NODE, "Connected to ", strConnect);

            fCONNECTED  = true;
            fOUTGOING   = true;

            return true;
        }

        return false;
    }


    /* Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management. */
    template <class PacketType>
    void BaseConnection<PacketType>::Disconnect()
    {
        /* Wake any potential sleeping connections up on disconnect. */
        NotifyEvent();

        if(fCONNECTED.load())
        {
            Close();
            fCONNECTED = false;
        }
    }


    /* Notify connection an event occured to wake up a sleeping connection. */
    template <class PacketType>
    void BaseConnection<PacketType>::NotifyEvent()
    {
        /* Set the events flag and notify. */
        fEVENT = true;
        EVENT_CONDITION.notify_all();
    }


    /* Have connection wait for a notify signal to wake up. */
    template <class PacketType>
    void BaseConnection<PacketType>::WaitEvent()
    {
        /* Reset the events flag. */
        fEVENT = false;

        /* Wait for a notify signal. */
        std::unique_lock<std::mutex> lk(EVENT_MUTEX);
        EVENT_CONDITION.wait(lk, [this]{return fEVENT.load() || config::fShutdown.load(); });
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class BaseConnection<Packet>;
    template class BaseConnection<MessagePacket>;
    template class BaseConnection<HTTPPacket>;
    template class BaseConnection<StatelessPacket>;

}
