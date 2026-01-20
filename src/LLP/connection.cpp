/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/connection.h>
#include <LLP/templates/events.h>

namespace LLP
{

    /* Default Constructor */
    Connection::Connection()
    : BaseConnection<Packet>()
    {
    }


    /* Constructor */
    Connection::Connection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
    : BaseConnection<Packet>(SOCKET_IN, DDOS_IN, fDDOSIn, fOutgoing)
    {
    }


    /* Constructor */
    Connection::Connection(DDOS_Filter* DDOS_IN, bool fDDOSIn, bool fOutgoing)
   : BaseConnection<Packet>(DDOS_IN, fDDOSIn, fOutgoing)
   {
   }


    /* Default destructor */
    Connection::~Connection()
    {
    }


    /*  Regular Connection Read Packet Method. */
    void Connection::ReadPacket()
    {

        /* Handle Reading Packet Type Header. */
        if(Available() >= 1 && INCOMING.IsNull())
        {
            std::vector<uint8_t> HEADER(1, 255);
            if(Read(HEADER, 1) == 1)
            {
                /* Check if this is a 16-bit opcode (prefix byte 0xD0).
                 * Note: 0xD0 (208) is MINER_AUTH_CHALLENGE which is only sent FROM node TO miner,
                 * never FROM miner TO node. So if we're receiving 0xD0, it's a 16-bit opcode prefix. */
                if(HEADER[0] == 0xD0)
                {
                    /* Read second byte to complete 16-bit opcode */
                    if(Available() >= 1)
                    {
                        std::vector<uint8_t> SECOND_BYTE(1, 0);
                        if(Read(SECOND_BYTE, 1) == 1)
                        {
                            /* Form 16-bit opcode (big-endian) */
                            uint16_t nOpcode16 = (static_cast<uint16_t>(HEADER[0]) << 8) | SECOND_BYTE[0];
                            
                            /* Set as 16-bit opcode packet */
                            INCOMING.Set16BitOpcode(nOpcode16);
                            
                            /* For 16-bit opcodes, we read all available data directly (no LENGTH field) */
                            /* The handler will know how much data to expect based on the opcode */
                        }
                    }
                }
                else
                {
                    /* Standard 8-bit opcode */
                    INCOMING.HEADER = HEADER[0];
                }
            }
        }

        /* At this point we need to check again whether the packet is considered complete as some
           packet types only require a header and no length or data*/
        if(!INCOMING.IsNull() && !INCOMING.Complete())
        {
            /* 16-bit opcodes don't have a LENGTH field - data is read directly if available */
            if(INCOMING.Is16Bit())
            {
                /* Read all available data for 16-bit opcode packets */
                uint32_t nAvailable = Available();
                if(nAvailable > 0)
                {
                    std::vector<uint8_t> DATA(nAvailable, 0);
                    int32_t nRead = Read(DATA, DATA.size());
                    
                    if(nRead > 0)
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.begin() + nRead);
                }
                
                /* Fire packet complete event (16-bit opcodes are complete after reading opcode) */
                Event(EVENTS::PACKET, static_cast<uint32_t>(INCOMING.DATA.size()));
            }
            else
            {
                /* Standard 8-bit format with LENGTH field */
                
                /* Read the packet length. */
                if(Available() >= 4 && INCOMING.LENGTH == 0)
                {
                    /* Handle Reading Packet Length Header. */
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
                    /* The maximum number of bytes to read is th number of bytes specified in the message length,
                       minus any already read on previous reads*/
                    uint32_t nMaxRead = (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size());

                    /* Vector to receve the read bytes. This should be the smaller of the number of bytes currently available or the
                       maximum amount to read */
                    std::vector<uint8_t> DATA(std::min(nAvailable, nMaxRead), 0);

                    /* Read up to the buffer size. */
                    int32_t nRead = Read(DATA, DATA.size());

                    /* If something was read, insert it into the packet data.  NOTE: that due to SSL packet framing we could end up
                       reading less bytes than appear available.  Therefore we only copy the number of bytes actually read */
                    if(nRead > 0)
                        INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.begin() + nRead);

                    /* If the packet is now considered complete, fire the packet complete event */
                    if(INCOMING.Complete())
                        Event(EVENTS::PACKET, static_cast<uint32_t>(DATA.size()));
                }
            }
        }
    }

}
