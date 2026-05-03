/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/connection.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/templates/events.h>
#include <Util/include/debug.h>

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


    /*  Regular Connection Read Packet Method. 
     *  Handles legacy 8-bit LLP framing: HEADER (1) + LENGTH (4) + DATA (LENGTH)
     */
    void Connection::ReadPacket()
    {
        /* Snapshot available bytes once — avoids up to three ioctl(FIONREAD)
         * syscalls per ReadPacket() call (one per conditional below).  We
         * subtract bytes as they are consumed; if the kernel accumulates more
         * data between reads, it will be picked up on the next ReadPacket()
         * invocation (next epoll/poll event).  This is safe for the
         * non-blocking, event-driven model used by all DataThreads. */
        int32_t nAvailable = Available();

        /* Handle Reading Packet Type Header (8-bit). */
        if(nAvailable >= 1 && INCOMING.IsNull())
        {
            std::vector<uint8_t> HEADER(1, 255);
            if(Read(HEADER, 1) == 1)
            {
                INCOMING.HEADER = HEADER[0];
                nAvailable -= 1;
            }
        }

        /* Read the packet length. */
        if(nAvailable >= 4 && !INCOMING.IsNull() && !INCOMING.fLengthRead)
        {
            /* Handle Reading Packet Length Header. */
            std::vector<uint8_t> BYTES(4, 0);
            if(Read(BYTES, 4) == 4)
            {
                INCOMING.SetLength(BYTES);
                Event(EVENTS::HEADER);
                nAvailable -= 4;

                /* Validate immediately after the 4-byte length field is read.
                 * This matches the stateless reader's allocation hardening and
                 * prevents malformed header-only packets (for example GET_BLOCK
                 * with LENGTH > 0) from stalling forever as partial packets. */
                std::string strLengthReason;
                if(!OpcodeUtility::ValidatePacketLength(INCOMING, &strLengthReason))
                {
                    debug::error(FUNCTION, "Legacy packet length validation failed from ",
                        GetAddress().ToStringIP(), ": ", strLengthReason);
                    Disconnect();
                    return;
                }

                /* Pre-allocate the DATA vector to the declared packet length.
                 * Without reserve(), each subsequent insert() into a growing
                 * vector triggers a geometric reallocation (copy of all existing
                 * bytes).  With reserve(), the vector has capacity for the full
                 * payload up front, making every subsequent end-insert O(1).
                 * Never reserve an oversized wire declaration. */
                if(INCOMING.LENGTH > 0 && INCOMING.LENGTH <= OpcodeUtility::MAX_ANY_PACKET_LENGTH)
                    INCOMING.DATA.reserve(INCOMING.LENGTH);
            }
        }

        /* Handle Reading Packet Data. */
        if(INCOMING.Header() && nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
        {
            /* The maximum number of bytes to read is the number of bytes specified in the message length,
               minus any already read on previous reads*/
            uint32_t nMaxRead = (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size());

            /* Vector to receive the read bytes. This should be the smaller of the number of bytes currently available or the
               maximum amount to read */
            std::vector<uint8_t> DATA(std::min((uint32_t)nAvailable, nMaxRead), 0);

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
