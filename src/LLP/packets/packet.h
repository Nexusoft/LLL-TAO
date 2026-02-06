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
#include <LLP/include/opcode_utility.h>

namespace LLP
{

    /** Packet
     *
     *  Class to handle sending and receiving of legacy LLP Packets.
     *
     *  Components of an LLP Packet (8-bit format):
     *   BYTE 0       : Header (8-bit opcode)
     *   BYTE 1 - 4   : Length (32-bit, big-endian)
     *   BYTE 5 - End : Data
     *
     *  For 16-bit stateless mining opcodes, see StatelessPacket.
     *
     **/
    class Packet
    {
    public:
        /* Message typedef. */
        typedef uint8_t message_t;


        /** The packet message (8-bit opcode). **/
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


        /** GetOpcode
         *
         *  Get the 8-bit opcode value.
         *
         *  @return The opcode as an 8-bit value
         *
         **/
        uint8_t GetOpcode() const
        {
            return HEADER;
        }


        /** Complete
         *
         *  Determines if a packet is fully read.
         *  Packet is complete when data size matches LENGTH.
         *
         **/
        bool Complete() const
        {
            return (Header() && static_cast<uint32_t>(DATA.size()) == LENGTH);
        }


        /** HasDataPayload
         *
         *  Determines if this packet type requires a data payload.
         *  Traditional packets use HEADER < 128 for data packets and HEADER >= 128
         *  for request/command packets. However, the Falcon authentication protocol
         *  (headers 207-212), reward address binding packets (213-214), mining round
         *  response packets (204-205), and the channel acknowledgment packet (206)
         *  require data payloads.
         *
         *  Packet ranges requiring data:
         *  - 0-127: Traditional data packets (BLOCK_DATA, SUBMIT_BLOCK, etc.)
         *  - 204-205: Mining round response packets (PR #151/PR #153)
         *    - NEW_ROUND (204): 12 bytes - unified height + channel height + difficulty
         *    - OLD_ROUND (205): variable - rejection reason or stale height info
         *  - 206: Channel acknowledgment
         *    - CHANNEL_ACK (206): channel number (1 byte)
         *  - 207-212: Falcon authentication and session packets
         *    - MINER_AUTH_INIT (207): pubkey + miner_id
         *    - MINER_AUTH_CHALLENGE (208): random nonce
         *    - MINER_AUTH_RESPONSE (209): signature
         *    - MINER_AUTH_RESULT (210): success/failure code
         *    - SESSION_START (211): session parameters
         *    - SESSION_KEEPALIVE (212): keepalive data
         *  - 213-214: Reward address binding packets (encrypted with ChaCha20)
         *    - MINER_SET_REWARD (213): encrypted reward address
         *    - MINER_REWARD_RESULT (214): encrypted validation result
         *
         *  @return true if this packet type carries data payload
         *
         **/
        bool HasDataPayload() const
        {
            /* Delegate to centralized opcode classification in OpcodeUtility.
             * This eliminates duplicate hardcoded range constants and ensures
             * consistency across the codebase.
             */
            return OpcodeUtility::HasDataPayload(HEADER);
        }


        /** Header
         *
         *  Determines if header is fully read.
         *  For data packets (HEADER < 128, round response 204-205, channel ack 206,
         *  Falcon auth 207-212, or reward binding 213-214), requires LENGTH > 0.
         *  For request packets (128-203, 215-254), LENGTH must be 0.
         *
         **/
        bool Header() const
        {
            if(IsNull())
                return false;

            /* Data packets require LENGTH > 0 (includes Falcon auth packets) */
            if(HasDataPayload())
                return LENGTH > 0;

            /* Request/command packets have no data (LENGTH == 0) */
            return HEADER < 255 && LENGTH == 0;
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
         *  Format: [HEADER (1)] [LENGTH (4)] [DATA (variable)]
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            std::vector<uint8_t> BYTES(1, HEADER);

            /* Handle packets that carry data payloads (traditional data packets,
             * Falcon authentication packets, and reward binding packets) */
            if(HasDataPayload())
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
         *  Handles both traditional data packets (HEADER < 128), mining round
         *  response packets (204-205), Falcon authentication packets (207-212),
         *  and reward binding packets (213-214) which all require data payloads.
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

            /* Handle packets that carry data payloads */
            if(HasDataPayload())
            {
                /* Encode length and data */
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
        std::string DebugString(size_t nMaxDataBytes = 64) const
        {
            std::ostringstream oss;
            oss << "Packet{header=0x" << std::hex << std::setw(2) << std::setfill('0')
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
