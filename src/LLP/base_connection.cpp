/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/base_connection.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>

#include <LLP/packets/packet.h>
#include <LLP/packets/http.h>
#include <LLP/packets/legacy.h>
#include <LLP/packets/tritium.h>

#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/args.h>

#include <Util/include/runtime.h>


namespace LLP
{

    /** Build Base Connection with no parameters **/
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection()
    : Socket()
    , INCOMING()
    , DDOS(nullptr)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , fDDOS(false)
    , fOUTGOING(false)
    , fCONNECTED(false)
    , nDataThread(-1)
    , nDataIndex(-1)
    , fEVENT(false)
    , EVENT_MUTEX()
    , EVENT_CONDITION()
    {
        INCOMING.SetNull();
    }

    /** Build Base Connection with all Parameters. **/
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS, bool fOutgoing)
    : Socket(SOCKET_IN)
    , INCOMING()
    , DDOS(DDOS_IN)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , fDDOS(isDDOS)
    , fOUTGOING(fOutgoing)
    , fCONNECTED(false)
    , nDataThread(-1)
    , nDataIndex(-1)
    , fEVENT(false)
    , EVENT_MUTEX()
    , EVENT_CONDITION()
    {
    }


    /** Build Base Connection with all Parameters. **/
    template <class PacketType>
    BaseConnection<PacketType>::BaseConnection(DDOS_Filter* DDOS_IN, bool isDDOS, bool fOutgoing)
    : Socket()
    , INCOMING()
    , DDOS(DDOS_IN)
    , nLatency(std::numeric_limits<uint32_t>::max())
    , fDDOS(isDDOS)
    , fOUTGOING(fOutgoing)
    , fCONNECTED(false)
    , nDataThread(-1)
    , nDataIndex(-1)
    , fEVENT(false)
    , EVENT_MUTEX()
    , EVENT_CONDITION()
    {
    }


    /* Default destructor */
    template <class PacketType>
    BaseConnection<PacketType>::~BaseConnection()
    {
        Disconnect();
        SetNull();
    }


    /*  Sets the object to an invalid state. */
    template <class PacketType>
    void BaseConnection<PacketType>::SetNull()
    {
        fd = -1;
        nError = 0;
        INCOMING.SetNull();
        DDOS  = nullptr;
        fDDOS = false;
        fOUTGOING = false;
        fCONNECTED = false;
        nDataThread = -1;
        nDataIndex  = -1;
    }


    /*  Connection flag to determine if socket should be handled if not connected. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Connected() const
    {
        return fCONNECTED.load();
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
    void BaseConnection<PacketType>::WritePacket(const PacketType& PACKET)
    {
        /* Get the bytes of the packet. */
        std::vector<uint8_t> vBytes = PACKET.GetBytes();

        /* Debug dump of message type. */
        debug::log(3, NODE "Sent Message (", vBytes.size(), " bytes)");

        /* Debug dump of packet data. */
        if(config::GetArg("-verbose", 0) >= 5)
            PrintHex(vBytes);

        /* Write the packet to socket buffer. */
        Write(vBytes, vBytes.size());
    }


    /*  Connect Socket to a Remote Endpoint. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Connect(const BaseAddress &addrConnect)
    {
        std::string connectStr = addrConnect.ToStringIP();

        /* Check for connect to self */
        if(addr.ToStringIP() == connectStr)
            return debug::error(NODE, "cannot self-connect");

        debug::log(1, NODE, "Connecting to ", connectStr);

        // Connect
        if(Attempt(addrConnect))
        {
            debug::log(1, NODE, "Connected to ", connectStr);

            fCONNECTED = true;
            fOUTGOING = true;

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
        EVENT_CONDITION.notify_one();
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
    template class BaseConnection<LegacyPacket>;
    template class BaseConnection<TritiumPacket>;
    template class BaseConnection<HTTPPacket>;

}
