/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
    : BaseConnection()
    {
    }


    /* Constructor */
    Connection::Connection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS, bool fOutgoing)
    : BaseConnection(SOCKET_IN, DDOS_IN, isDDOS, fOutgoing)
    {
    }


    /* Constructor */
    Connection::Connection(DDOS_Filter* DDOS_IN, bool isDDOS, bool fOutgoing)
   : BaseConnection(DDOS_IN, isDDOS, fOutgoing)
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
                INCOMING.HEADER = HEADER[0];
        }

        /* At this point we need to check agin whether the packet is considered complete as some
           packet types only require a header and no length or data*/
        if(!INCOMING.IsNull() && !INCOMING.Complete())
        {
            /* Read the packet length. */
            if(Available() >= 4 && INCOMING.LENGTH == 0)
            {
                /* Handle Reading Packet Length Header. */
                std::vector<uint8_t> BYTES(4, 0);
                if(Read(BYTES, 4) == 4)
                {
                    INCOMING.SetLength(BYTES);
                    Event(EVENT_HEADER);
                }
            }

            /* Handle Reading Packet Data. */
            uint32_t nAvailable = Available();
            if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                /* Read the data in the packet */
                std::vector<uint8_t> DATA(std::min(nAvailable, (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                /* On successful read, fire event and add data to packet. */
                if(Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    Event(EVENT_PACKET, static_cast<uint32_t>(DATA.size()));
                }
            }
        }
    }

}
