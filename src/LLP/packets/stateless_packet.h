/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <string>

#include <Util/templates/datastream.h>

namespace LLP
{

    /** StatelessPacket
     *
     *  Packet type for stateless mining protocol with 16-bit opcodes.
     *
     *  Components of a Stateless Mining Packet:
     *   BYTE 0 - 1   : Header (16-bit opcode, big-endian)
     *   BYTE 2 - 5   : Length (32-bit, big-endian)
     *   BYTE 6 - End : Data
     *
     *  This is the proper framing for NexusMiner stateless opcodes (0xD007, 0xD008, etc.)
     *  and is incompatible with legacy 8-bit LLP framing.
     *
     **/
    class StatelessPacket
    {
    public:
        /* Message typedef for compatibility with BaseConnection template. */
        typedef uint16_t message_t;


        /** The packet header (16-bit opcode, big-endian). **/
        uint16_t                HEADER;


        /** The length of the packet data. **/
        uint32_t                LENGTH;


        /** The packet payload. **/
        std::vector<uint8_t>    DATA;


        /** Default Constructor **/
        StatelessPacket()
        : HEADER (0xFFFF)
        , LENGTH (0)
        , DATA   ( )
        {
        }


        /** Copy Constructor **/
        StatelessPacket(const StatelessPacket& packet)
        : HEADER (packet.HEADER)
        , LENGTH (packet.LENGTH)
        , DATA   (packet.DATA)
        {
        }


        /** Move Constructor **/
        StatelessPacket(StatelessPacket&& packet) noexcept
        : HEADER (std::move(packet.HEADER))
        , LENGTH (std::move(packet.LENGTH))
        , DATA   (std::move(packet.DATA))
        {
        }


        /** Copy Assignment Operator **/
        StatelessPacket& operator=(const StatelessPacket& packet)
        {
            HEADER = packet.HEADER;
            LENGTH = packet.LENGTH;
            DATA   = packet.DATA;

            return *this;
        }


        /** Move Assignment Operator **/
        StatelessPacket& operator=(StatelessPacket&& packet) noexcept
        {
            HEADER = std::move(packet.HEADER);
            LENGTH = std::move(packet.LENGTH);
            DATA   = std::move(packet.DATA);

            return *this;
        }


        /** Destructor. **/
        ~StatelessPacket()
        {
            std::vector<uint8_t>().swap(DATA);
        }


        /** Constructor with 16-bit opcode **/
        StatelessPacket(const message_t nMessage)
        {
            SetNull();
            HEADER = nMessage;
        }


        /** SetNull
         *
         *  Set the Packet Null Flags.
         *
         **/
        void SetNull()
        {
            HEADER   = 0xFFFF;
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
            return (HEADER == 0xFFFF);
        }


        /** GetOpcode
         *
         *  Get the 16-bit opcode value.
         *
         *  @return The 16-bit opcode
         *
         **/
        uint16_t GetOpcode() const
        {
            return HEADER;
        }


        /** Complete
         *
         *  Determines if a packet is fully read.
         *  Packet is complete when we have header, length, and all data bytes.
         *
         **/
        bool Complete() const
        {
            return (Header() && static_cast<uint32_t>(DATA.size()) == LENGTH);
        }


        /** Header
         *
         *  Determines if header and length are fully read.
         *
         **/
        bool Header() const
        {
            if(IsNull())
                return false;

            /* For stateless packets, header is complete when LENGTH is set */
            return LENGTH > 0 || (HEADER != 0xFFFF && LENGTH == 0);
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
         *  Sets the size of the packet from the byte vector (big-endian).
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
         *  Format: [HEADER (2, big-endian)] [LENGTH (4, big-endian)] [DATA (variable)]
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            std::vector<uint8_t> BYTES;
            BYTES.reserve(6 + DATA.size());

            /* Encode 16-bit header (big-endian) */
            BYTES.push_back(static_cast<uint8_t>(HEADER >> 8));   // High byte
            BYTES.push_back(static_cast<uint8_t>(HEADER & 0xFF)); // Low byte

            /* Encode 32-bit length (big-endian) */
            BYTES.push_back(static_cast<uint8_t>(LENGTH >> 24));
            BYTES.push_back(static_cast<uint8_t>(LENGTH >> 16));
            BYTES.push_back(static_cast<uint8_t>(LENGTH >> 8));
            BYTES.push_back(static_cast<uint8_t>(LENGTH));

            /* Append data payload */
            BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());

            return BYTES;
        }


        /** DebugString
         *
         *  Returns a debug string representation of the packet for logging.
         *
         *  @param[in] nMaxDataBytes Maximum number of data bytes to include (0 = all)
         *
         *  @return Human-readable string representation of packet
         *
         **/
        std::string DebugString(size_t nMaxDataBytes = 64) const
        {
            std::ostringstream oss;
            oss << "StatelessPacket{header=0x" << std::hex << std::setw(4) << std::setfill('0')
                << static_cast<uint32_t>(HEADER) << std::dec
                << ", length=" << LENGTH
                << ", data_size=" << DATA.size();

            if(!DATA.empty() && nMaxDataBytes > 0)
            {
                oss << ", data_preview=";
                size_t nShow = std::min(nMaxDataBytes, DATA.size());
                for(size_t i = 0; i < nShow; ++i)
                {
                    oss << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<uint32_t>(DATA[i]);
                }
                if(DATA.size() > nMaxDataBytes)
                    oss << "...";
            }

            oss << "}";
            return oss.str();
        }
    };
}
