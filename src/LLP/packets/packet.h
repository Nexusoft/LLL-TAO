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
        void SetNull()
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
         *  DEBUG: This method logs detailed packet encoding information when
         *  config::nVerbose >= 5 to help diagnose packet encoding issues
         *  affecting the Falcon handshake protocol.
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


        /** GetBytesWithDebug
         *
         *  Serializes class into a byte vector with detailed debugging logs.
         *  Used for diagnosing packet encoding issues in Falcon handshake.
         *
         *  @param[in] strContext Context string for log messages
         *
         *  @return Serialized packet bytes
         *
         **/
        std::vector<uint8_t> GetBytesWithDebug(const std::string& strContext = "") const
        {
            std::vector<uint8_t> BYTES(1, HEADER);

            /* Log packet header info */
            std::string strLog = strContext.empty() ? "Packet::GetBytes" : strContext;

            if(HEADER < 128) /* Handle for Data Packets. */
            {
                /* Log data packet encoding */
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 24));
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 16));
                BYTES.push_back(static_cast<uint8_t>(LENGTH >> 8));
                BYTES.push_back(static_cast<uint8_t>(LENGTH));

                BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());
            }

            return BYTES;
        }


        /** DebugString
         *
         *  Returns a debug string representation of the packet for logging.
         *  Useful for diagnosing Falcon handshake encoding/decoding issues.
         *
         *  @param[in] nMaxDataBytes Maximum number of data bytes to include (0 = all)
         *
         *  @return Human-readable string representation of packet
         *
         **/
        std::string DebugString(uint32_t nMaxDataBytes = 64) const
        {
            std::ostringstream oss;
            oss << "Packet{header=0x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<uint32_t>(HEADER) << std::dec
                << ", length=" << LENGTH
                << ", data_size=" << DATA.size();

            if(!DATA.empty() && nMaxDataBytes > 0)
            {
                oss << ", data_preview=";
                uint32_t nShow = std::min(nMaxDataBytes, static_cast<uint32_t>(DATA.size()));
                for(uint32_t i = 0; i < nShow; ++i)
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
