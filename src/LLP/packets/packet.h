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
#include <LLP/include/packet_framing.h>

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


        /** Tracks whether the 4-byte length field has been physically read. **/
        bool                    fLengthRead;


        /** The packet payload. **/
        std::vector<uint8_t>    DATA;


        /** Default Constructor **/
        Packet()
        : HEADER (255)
        , LENGTH (0)
        , fLengthRead(false)
        , DATA   ( )
        {
        }


        /** Copy Constructor **/
        Packet(const Packet& packet)
        : HEADER (packet.HEADER)
        , LENGTH (packet.LENGTH)
        , fLengthRead(packet.fLengthRead)
        , DATA   (packet.DATA)
        {
        }


        /** Move Constructor **/
        Packet(Packet&& packet) noexcept
        : HEADER (std::move(packet.HEADER))
        , LENGTH (std::move(packet.LENGTH))
        , fLengthRead(std::move(packet.fLengthRead))
        , DATA   (std::move(packet.DATA))
        {
        }


        /** Copy Assignment Operator **/
        Packet& operator=(const Packet& packet)
        {
            HEADER = packet.HEADER;
            LENGTH = packet.LENGTH;
            fLengthRead = packet.fLengthRead;
            DATA   = packet.DATA;

            return *this;
        }


        /** Move Assignment Operator **/
        Packet& operator=(Packet&& packet) noexcept
        {
            HEADER = std::move(packet.HEADER);
            LENGTH = std::move(packet.LENGTH);
            fLengthRead = std::move(packet.fLengthRead);
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
            fLengthRead = false;
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
         *  Determines if this packet type may carry a data payload.
         *  Legacy LLP framing is always [HEADER][LENGTH][DATA], even when LENGTH is 0,
         *  so this helper classifies payload-capable opcodes rather than deciding
         *  whether the 4-byte length field is present.
         *
         *  Payload-capable packets:
         *  - 0-127: Traditional data packets (BLOCK_DATA, SUBMIT_BLOCK, etc.)
         *  - 204-205: Mining round response packets (PR #151/PR #153)
         *    - NEW_ROUND (204): 16 bytes - Full Height Picture: unified + prime + hash + stake heights
         *    - OLD_ROUND (205): variable - rejection reason or stale height info
         *  - 201: BLOCK_REJECTED may carry rejection/control payload bytes
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
         *  @return true if this packet type may carry data payload
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
         *  Determines if header and the legacy 4-byte length field are fully read.
         *  Payload validity is checked separately by opcode-specific validation.
         *
         **/
        bool Header() const
        {
            if(IsNull())
                return false;

            if(!fLengthRead)
                return false;

            /* Header-only request/command packets must declare LENGTH == 0. */
            if(OpcodeUtility::IsHeaderOnlyRequest(HEADER))
                return LENGTH == 0;

            /* All other legacy opcodes are complete once header+length are read. */
            return HEADER < 255;
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
            LENGTH = PacketFraming::DecodeLength(BYTES);
            fLengthRead = true;
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
            return PacketFraming::BuildLegacyBytes(HEADER, LENGTH, DATA);
        }


        /** GetBytesWithDebug
         *
         *  Serializes class into a byte vector with detailed debugging logs.
         *  Used for diagnosing packet encoding issues in Falcon handshake.
         *
         *  Emits legacy framing exactly as it appears on the wire:
         *  [HEADER][LENGTH][DATA]. This includes header-only requests where
         *  LENGTH is zero.
         *
         *  @param[in] strContext Context string for log messages
         *
         *  @return Serialized packet bytes
         *
         **/
        std::vector<uint8_t> GetBytesWithDebug(const std::string& strContext = "") const
        {
            (void)strContext;
            return GetBytes();
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
