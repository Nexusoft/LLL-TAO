/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKETS_PACKET_H
#define NEXUS_LLP_PACKETS_PACKET_H

#include <vector>

namespace LLP
{

    /* Class to handle sending and receiving of LLP Packets. */
    class Packet
    {
    public:
        Packet() { SetNull(); }

        /* Components of an LLP Packet.
            BYTE 0       : Header
            BYTE 1 - 5   : Length
            BYTE 6 - End : Data      */
        uint8_t                 HEADER;
        uint32_t                LENGTH;
        std::vector<uint8_t>    DATA;


        /* Set the Packet Null Flags. */
        inline void SetNull()
        {
            HEADER   = 255;
            LENGTH   = 0;

            DATA.clear();
        }


        /* Packet Null Flag. Header = 255. */
        bool IsNull() { return (HEADER == 255); }


        /* Determine if a packet is fully read. */
        bool Complete() { return (Header() && DATA.size() == LENGTH); }


        /* Determine if header is fully read */
        bool Header() { return IsNull() ? false : (HEADER < 128 && LENGTH > 0) || (HEADER >= 128 && HEADER < 255 && LENGTH == 0); }


        /* Sets the size of the packet from Byte Vector. */
        void SetLength(std::vector<uint8_t> BYTES) { LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] ); }


        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes()
        {
            std::vector<uint8_t> BYTES(1, HEADER);

            /* Handle for Data Packets. */
            if(HEADER < 128)
            {
                BYTES.push_back((LENGTH >> 24)); BYTES.push_back((LENGTH >> 16));
                BYTES.push_back((LENGTH >> 8));  BYTES.push_back(LENGTH);

                BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
            }

            return BYTES;
        }
    };
}

#endif
