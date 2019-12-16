/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_PACKETS_PACKET_H
#define NEXUS_LLP_PACKETS_PACKET_H

#include <vector>
#include <cstdint>

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
        /* Message typedef. */
        typedef uint8_t message_t;


        /** The packet message. **/
        uint8_t                 HEADER;


        /** The length of the packet data. **/
        uint32_t                LENGTH;


        /** The packet payload. **/
        std::vector<uint8_t>    DATA;


        /** Default Constructor **/
        Packet()
        : HEADER (255)
        , LENGTH (0)
        , DATA   ( )
        {
        }


        /** Copy Constructor **/
        Packet(const Packet& packet)
        : HEADER (packet.HEADER)
        , LENGTH (packet.LENGTH)
        , DATA   (packet.DATA)
        {
        }


        /** Move Constructor **/
        Packet(Packet&& packet) noexcept
        : HEADER (std::move(packet.HEADER))
        , LENGTH (std::move(packet.LENGTH))
        , DATA   (std::move(packet.DATA))
        {
        }


        /** Copy Assignment Operator **/
        Packet& operator=(const Packet& packet)
        {
            HEADER = packet.HEADER;
            LENGTH = packet.LENGTH;
            DATA   = packet.DATA;

            return *this;
        }


        /** Move Assignment Operator **/
        Packet& operator=(Packet&& packet) noexcept
        {
            HEADER = std::move(packet.HEADER);
            LENGTH = std::move(packet.LENGTH);
            DATA   = std::move(packet.DATA);

            return *this;
        }


        /** Destructor. **/
        ~Packet()
        {
            std::vector<uint8_t>().swap(DATA);
        }


        /** Constructor **/
        Packet(const message_t nMessage)
        {
            SetNull();

            HEADER = nMessage;
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
            return (Header() && static_cast<uint32_t>(DATA.size()) == LENGTH);
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


        /** SetData
         *
         *  Set the Packet Data.
         *
         *  @param[in] ssData The datastream with the data to set.
         *
         **/
        void SetData(const DataStream& ssData)
        {
            LENGTH = static_cast<uint32_t>(ssData.size());
            DATA   = ssData.Bytes();
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
            LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3]);
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
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 24));
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 16));
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 8));
                BYTES.push_back(static_cast<uint8_t>(LENGTH));

                BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
            }

            return BYTES;
        }
    };
}

#endif
