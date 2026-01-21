/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/stateless_connection.h>
#include <LLP/templates/events.h>

namespace LLP
{

    /* Default Constructor */
    StatelessConnection::StatelessConnection()
    : BaseConnection<StatelessPacket>()
    {
    }


    /* Constructor */
    StatelessConnection::StatelessConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
    : BaseConnection<StatelessPacket>(SOCKET_IN, DDOS_IN, fDDOSIn, fOutgoing)
    {
    }


    /* Constructor */
    StatelessConnection::StatelessConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
   : BaseConnection<StatelessPacket>(DDOS_IN, fDDOSIn, fOutgoing)
   {
   }


    /* Default destructor */
    StatelessConnection::~StatelessConnection()
    {
    }


    /*  Stateless Connection Read Packet Method.
     *  Reads 16-bit framed packets: HEADER (2 bytes) + LENGTH (4 bytes) + DATA (LENGTH bytes)
     */
    void StatelessConnection::ReadPacket()
    {
        /* Handle Reading Packet Header (16-bit, big-endian). */
        if(Available() >= 2 && INCOMING.IsNull())
        {
            std::vector<uint8_t> HEADER(2, 0);
            if(Read(HEADER, 2) == 2)
            {
                /* Decode 16-bit header (big-endian) */
                uint16_t nOpcode16 = (static_cast<uint16_t>(HEADER[0]) << 8) | HEADER[1];
                INCOMING.HEADER = nOpcode16;
            }
        }

        /* Handle Reading Packet Length (32-bit, big-endian). */
        if(Available() >= 4 && !INCOMING.IsNull() && INCOMING.LENGTH == 0)
        {
            std::vector<uint8_t> BYTES(4, 0);
            if(Read(BYTES, 4) == 4)
            {
                INCOMING.SetLength(BYTES);
                Event(EVENTS::HEADER);
            }
        }

        /* Handle Reading Packet Data. */
        uint32_t nAvailable = Available();
        if(INCOMING.Header() && nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
        {
            /* The maximum number of bytes to read is the number of bytes specified in the message length,
               minus any already read on previous reads */
            uint32_t nMaxRead = (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size());

            /* Vector to receive the read bytes. This should be the smaller of the number of bytes currently available or the
               maximum amount to read */
            std::vector<uint8_t> DATA(std::min(nAvailable, nMaxRead), 0);

            /* Read up to the buffer size. */
            int32_t nRead = Read(DATA, DATA.size());

            /* If something was read, insert it into the packet data. NOTE: that due to SSL packet framing we could end up
               reading less bytes than appear available. Therefore we only copy the number of bytes actually read */
            if(nRead > 0)
                INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.begin() + nRead);

            /* If the packet is now considered complete, fire the packet complete event */
            if(INCOMING.Complete())
                Event(EVENTS::PACKET, static_cast<uint32_t>(DATA.size()));
        }
    }

}
