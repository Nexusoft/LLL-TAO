/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKETS_PACKET_H
#define NEXUS_LLP_PACKETS_PACKET_H

#include <vector>
#include <cinttypes>

namespace LLP
{

    /** Packet
     *
     *  Class to handle sending and receiving of LLP Packets.
     *
     *  Components of an LLP Packet.
     *   BYTE 0       : Header
     *   BYTE 1 - 5   : Length
     *   BYTE 6 - End : Data
     *
     **/
    class Packet
    {
    public:
        uint8_t                 HEADER;
        uint32_t                LENGTH;
        std::vector<uint8_t>    DATA;


        /** Default Constructor **/
        Packet()
        : HEADER(255)
        , LENGTH(0)
        , DATA()
        {
        }


        /** SetNull
         *
         *  Set the Packet Null Flags.
         *
         **/
        inline void SetNull()
        {
            HEADER   = 255;
            LENGTH   = 0;

            DATA.clear();
        }


        /** IsNull
         *
         *  Returns packet null flag.
         *
         **/
        bool IsNull() const
        {
            return (HEADER == 255);
        }


        /** Complete
         *
         *  Determines if a packet is fully read.
         *
         **/
        bool Complete() const
        {
            return (Header() && DATA.size() == LENGTH);
        }


        /** Header
         *
         *  Determines if header is fully read.
         *
         **/
        bool Header() const
        {
            return IsNull() ? false : (HEADER < 128 && LENGTH > 0) ||
                                      (HEADER >= 128 && HEADER < 255 && LENGTH == 0);
        }


        /** SetLength
         *
         *  Sets the size of the packet from the byte vector.
         *
         *  @param[in] BYTES The vector of bytes to set length from.
         *
         **/
        void SetLength(const std::vector<uint8_t> &BYTES)
        {
            LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] );
        }


        /** GetBytes
         *
         *  Serializes class into a byte vector. Used to write packet to sockets.
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            std::vector<uint8_t> BYTES(1, HEADER);

            if(HEADER < 128) /* Handle for Data Packets. */
            {
                BYTES.push_back((LENGTH >> 24));
                BYTES.push_back((LENGTH >> 16));
                BYTES.push_back((LENGTH >> 8));
                BYTES.push_back(LENGTH);

                BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
            }

            return BYTES;
        }
    };
}

#endif
