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
    {
    }


    /* Default destructor */
    template <class PacketType>
    BaseConnection<PacketType>::~BaseConnection()
    {
        Disconnect();
        SetNull();
    }



    /*  Resets the internal timers. */
    template <class PacketType>
    void BaseConnection<PacketType>::Reset()
    {
        nLastRecv = runtime::timestamp();
        nLastSend = runtime::timestamp();
    }



    /*  Sets the object to an invalid state. */
    template <class PacketType>
    void BaseConnection<PacketType>::SetNull()
    {

        fd = -1;
        nError = 0;

        INCOMING.SetNull();

        if(DDOS)
        {
            delete DDOS;
            DDOS  = nullptr;
        }

        fDDOS = false;
        fOUTGOING = false;
        fCONNECTED = false;
    }


    /*  Checks if is in null state. */
    template <class PacketType>
    bool BaseConnection<PacketType>::IsNull() const
    {
        return fd == -1;
    }


    /*  Checks for any flags in the Error Handle. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Errors() const
    {
        return ErrorCode() != 0;
    }


    /*  Give the message (c-string) of the error in the socket. */
    template <class PacketType>
    char *BaseConnection<PacketType>::Error() const
    {
        return strerror(ErrorCode());
    }


    /*  Connection flag to determine if socket should be handled if not connected. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Connected() const
    {
        return fCONNECTED.load();
    }


    /*  Determines if nTime seconds have elapsed since last Read / Write. */
    template <class PacketType>
    bool BaseConnection<PacketType>::Timeout(uint32_t nTime) const
    {
        return (runtime::timestamp() > nLastSend + nTime &&
                runtime::timestamp() > nLastRecv + nTime);
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
    bool BaseConnection<PacketType>::Connect(std::string strAddress, uint16_t nPort)
    {
        BaseAddress addrConnect(strAddress, nPort);

        /* Check for connect to self */
        if(addr.ToStringIP() == addrConnect.ToStringIP())
            return debug::error(NODE, "cannot self-connect");

        debug::log(1, NODE, "Connecting to ", addrConnect.ToStringIP());

        // Connect
        if (Attempt(addrConnect))
        {
            debug::log(1, NODE, "Connected to ", addrConnect.ToStringIP());

            fCONNECTED.store(true);
            fOUTGOING.store(true);

            return true;
        }

        return false;
    }


    template <class PacketType>
    BaseAddress BaseConnection<PacketType>::GetAddress() const
    {
        return addr;
    }


    template <class PacketType>
    void BaseConnection<PacketType>::Disconnect()
    {
        Close();

        fCONNECTED.store(false);
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class BaseConnection<Packet>;
    template class BaseConnection<LegacyPacket>;
    template class BaseConnection<TritiumPacket>;
    template class BaseConnection<HTTPPacket>;

}
