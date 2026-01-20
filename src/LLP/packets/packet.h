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
     *  Components of an LLP Packet (8-bit format - traditional):
     *   BYTE 0       : Header (8-bit opcode)
     *   BYTE 1 - 4   : Length (32-bit, big-endian)
     *   BYTE 5 - End : Data
     *
     *  Components of an LLP Packet (16-bit format - stateless mining):
     *   BYTE 0 - 1   : Header (16-bit opcode, big-endian)
     *   BYTE 2 - End : Data (implicit length, no LENGTH field)
     *
     *  Note: 16-bit opcodes are detected by first byte being 0xD0 (208).
     *  This is safe because 208 is MINER_AUTH_CHALLENGE which is only sent
     *  FROM node TO miner, never FROM miner TO node.
     *
     **/
    class Packet
    {
    public:
        /* Message typedef. */
        typedef uint8_t message_t;


        /** The packet message (8-bit format). **/
        uint8_t                 HEADER;


        /** The length of the packet data (8-bit format only). **/
        uint32_t                LENGTH;


        /** The packet payload. **/
        std::vector<uint8_t>    DATA;


        /** Flag indicating if this is a 16-bit opcode packet. **/
        bool                    fIs16BitOpcode;


        /** The full 16-bit opcode (only valid if fIs16BitOpcode is true). **/
        uint16_t                nOpcode16;


        /** Default Constructor **/
        Packet()
        : HEADER (255)
        , LENGTH (0)
        , DATA   ( )
        , fIs16BitOpcode (false)
        , nOpcode16 (0)
        {
        }


        /** Copy Constructor **/
        Packet(const Packet& packet)
        : HEADER (packet.HEADER)
        , LENGTH (packet.LENGTH)
        , DATA   (packet.DATA)
        , fIs16BitOpcode (packet.fIs16BitOpcode)
        , nOpcode16 (packet.nOpcode16)
        {
        }


        /** Move Constructor **/
        Packet(Packet&& packet) noexcept
        : HEADER (std::move(packet.HEADER))
        , LENGTH (std::move(packet.LENGTH))
        , DATA   (std::move(packet.DATA))
        , fIs16BitOpcode (std::move(packet.fIs16BitOpcode))
        , nOpcode16 (std::move(packet.nOpcode16))
        {
        }


        /** Copy Assignment Operator **/
        Packet& operator=(const Packet& packet)
        {
            HEADER = packet.HEADER;
            LENGTH = packet.LENGTH;
            DATA   = packet.DATA;
            fIs16BitOpcode = packet.fIs16BitOpcode;
            nOpcode16 = packet.nOpcode16;

            return *this;
        }


        /** Move Assignment Operator **/
        Packet& operator=(Packet&& packet) noexcept
        {
            HEADER = std::move(packet.HEADER);
            LENGTH = std::move(packet.LENGTH);
            DATA   = std::move(packet.DATA);
            fIs16BitOpcode = std::move(packet.fIs16BitOpcode);
            nOpcode16 = std::move(packet.nOpcode16);

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


        /** Constructor for 16-bit opcode **/
        Packet(const uint16_t nOpcode)
        {
            SetNull();

            fIs16BitOpcode = true;
            nOpcode16 = nOpcode;
            HEADER = static_cast<uint8_t>(nOpcode >> 8); // High byte
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
            
            fIs16BitOpcode = false;
            nOpcode16 = 0;
        }


        /** IsNull
         *
         *  Returns packet null flag.
         *
         **/
        bool IsNull() const
        {
            return (HEADER == 255 && !fIs16BitOpcode);
        }


        /** GetOpcode
         *
         *  Get the opcode value (handles both 8-bit and 16-bit).
         *
         *  @return The opcode as a 16-bit value (8-bit opcodes are zero-extended)
         *
         **/
        uint16_t GetOpcode() const
        {
            return fIs16BitOpcode ? nOpcode16 : static_cast<uint16_t>(HEADER);
        }


        /** Is16Bit
         *
         *  Check if this is a 16-bit opcode packet.
         *
         *  @return true if 16-bit opcode, false if 8-bit
         *
         **/
        bool Is16Bit() const
        {
            return fIs16BitOpcode;
        }


        /** Set16BitOpcode
         *
         *  Set this packet to use a 16-bit opcode.
         *
         *  @param[in] nOpcode The 16-bit opcode value
         *
         **/
        void Set16BitOpcode(uint16_t nOpcode)
        {
            fIs16BitOpcode = true;
            nOpcode16 = nOpcode;
            HEADER = static_cast<uint8_t>(nOpcode >> 8); // High byte for compatibility
        }


        /** Complete
         *
         *  Determines if a packet is fully read.
         *  For 8-bit opcodes: checks if data size matches LENGTH
         *  For 16-bit opcodes: always returns true after opcode is read (no LENGTH field)
         *
         **/
        bool Complete() const
        {
            /* 16-bit opcodes have no LENGTH field, so they're complete once opcode is read */
            if(fIs16BitOpcode)
                return true;
                
            /* 8-bit opcodes need header and length to match data */
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
            /* Boundary constants for mining round response packets (PR #151/PR #153)
             * These packets carry height and difficulty data for stateless mining.
             * NEW_ROUND (204): 12 bytes - unified height (4) + channel height (4) + difficulty (4)
             * OLD_ROUND (205): variable - rejection reason or stale height info
             */
            static const uint8_t ROUND_RESPONSE_FIRST = 204;  // NEW_ROUND
            static const uint8_t ROUND_RESPONSE_LAST = 205;   // OLD_ROUND
            
            /* Boundary constants for Falcon authentication packets */
            static const uint8_t FALCON_AUTH_FIRST = 207;  // MINER_AUTH_INIT
            static const uint8_t FALCON_AUTH_LAST = 212;   // SESSION_KEEPALIVE
            
            /* Boundary constants for reward address binding packets */
            static const uint8_t REWARD_BINDING_FIRST = 213;  // MINER_SET_REWARD
            static const uint8_t REWARD_BINDING_LAST = 214;   // MINER_REWARD_RESULT
            
            /* Channel acknowledgment packet */
            static const uint8_t CHANNEL_ACK = 206;

            /* Traditional data packets */
            if(HEADER < 128)
                return true;

            /* Mining round response packets carry height data (PR #151/PR #153) */
            if(HEADER >= ROUND_RESPONSE_FIRST && HEADER <= ROUND_RESPONSE_LAST)
                return true;

            /* Channel acknowledgment requires data (channel number) */
            if(HEADER == CHANNEL_ACK)
                return true;

            /* Falcon authentication packets require data */
            if(HEADER >= FALCON_AUTH_FIRST && HEADER <= FALCON_AUTH_LAST)
                return true;
            
            /* Reward address binding packets require data (encrypted) */
            if(HEADER >= REWARD_BINDING_FIRST && HEADER <= REWARD_BINDING_LAST)
                return true;

            return false;
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
         *  Handles both traditional 8-bit packets and new 16-bit opcode packets.
         *
         *  8-bit format: [HEADER (1)] [LENGTH (4)] [DATA (variable)]
         *  16-bit format: [OPCODE (2, big-endian)] [DATA (implicit length)]
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            /* Handle 16-bit opcode format */
            if(fIs16BitOpcode)
            {
                std::vector<uint8_t> BYTES(2);
                BYTES[0] = static_cast<uint8_t>(nOpcode16 >> 8);   // High byte
                BYTES[1] = static_cast<uint8_t>(nOpcode16 & 0xFF); // Low byte
                
                /* Append data directly (no LENGTH field) */
                BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());
                
                return BYTES;
            }

            /* Handle traditional 8-bit format */
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
