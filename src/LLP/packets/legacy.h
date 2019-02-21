/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKET_LEGACY_H
#define NEXUS_LLP_PACKET_LEGACY_H

#include <queue>
#include <vector>
#include <algorithm>

#include <LLC/hash/SK.h>

#include <LLP/include/version.h>

#include <Util/templates/datastream.h>
#include <Util/templates/flatdata.h>

#include <Util/include/debug.h>
#include <Util/include/args.h>
#include <Util/include/memory.h>


namespace LLP
{

    /* Message Packet Leading Bytes. */
    const uint8_t MESSAGE_START_TESTNET[4] = { 0xe9, 0x59, 0x0d, 0x05 };
    const uint8_t MESSAGE_START_MAINNET[4] = { 0x05, 0x0d, 0x59, 0xe9 };


    /** LegacyPacket
     *
     *  Class to handle sending and receiving of More Complese Message LLP Packets.
     *
     *  Components of a Message LLP Packet.
     *  BYTE 0 - 4    : Start
     *  BYTE 5 - 17   : Message
     *  BYTE 18 - 22  : Size
     *  BYTE 23 - 26  : Checksum
     *  BYTE 26 - X   : Data
     *
     ***/
    class LegacyPacket
    {
    public:

        uint8_t       HEADER[4];
        char	        MESSAGE[12];
        uint32_t	    LENGTH;
        uint32_t	    CHECKSUM;

        std::vector<uint8_t> DATA;

        /** Default Constructor **/
        LegacyPacket()
        {
            SetNull();
            SetHeader();
        }

        /** Constructor **/
        LegacyPacket(const char* chMessage)
        {
            SetNull();
            SetHeader();
            SetMessage(chMessage);
        }

        /** Serialization **/
        IMPLEMENT_SERIALIZE
        (
            READWRITE(FLATDATA(HEADER));
            READWRITE(FLATDATA(MESSAGE));
            READWRITE(LENGTH);
            READWRITE(CHECKSUM);
        )


        /** SetNull
         *
         *  Set the Packet Null Flags.
         *
         **/
        void SetNull()
        {
            LENGTH    = 0;
            CHECKSUM  = 0;
            SetMessage("");

            DATA.clear();
        }


        /** GetMessage
         *
         *  Get the Command of packet in a std::string type.
         *
         *  @return Returns a string message.
         *
         **/
        std::string GetMessage() const
        {
            return std::string(MESSAGE, MESSAGE + strlen(MESSAGE));
        }


        /** IsNull
         *
         *  Packet Null Flag. Length and Checksum both 0.
         *
         **/
        bool IsNull() const
        {
            return (std::string(MESSAGE) == "" && LENGTH == 0 && CHECKSUM == 0);
        }


        /** Complete
         *
         *  Determine if a packet is fully read.
         *
         **/
        bool Complete() const
        {
            return (Header() && DATA.size() == LENGTH);
        }


        /** Header
         *
         *  Determine if header is fully read
         *
         **/
        bool Header() const
        {
            return IsNull() ? false : (CHECKSUM > 0 && std::string(MESSAGE) != "");
        }


        /** SetHeader
         *
         *  Set the first four bytes in the packet headcer to be of the
         *  byte series selected.
         *
         **/
        void SetHeader()
        {
            if (config::fTestNet)
                //memcpy(HEADER, MESSAGE_START_TESTNET, sizeof(MESSAGE_START_TESTNET));
                std::copy(MESSAGE_START_TESTNET,
                    MESSAGE_START_TESTNET + sizeof(MESSAGE_START_TESTNET),
                    HEADER);
            else
                //memcpy(HEADER, MESSAGE_START_MAINNET, sizeof(MESSAGE_START_MAINNET));
                std::copy(MESSAGE_START_MAINNET,
                    MESSAGE_START_MAINNET + sizeof(MESSAGE_START_MAINNET),
                    HEADER);
        }


        /** SetMessage
         *
         *  Set the message in the packet header.
         *
         *  @param[in] chMessage the message to set.
         *
         **/
        void SetMessage(const char* chMessage)
        {
            //std::copy((char*)chMessage, (char*)chMessage + std::min((size_t)12, sizeof(chMessage)), (char*)&MESSAGE);

            /* In gcc 8+ it throws a warning that copying the full length of the destination may truncate the terminating null character.
             * This is the behavior we want, because it is a fixed-width message. We also want padding with null characters when chMessage is shorter.
             * Thus, the warning is that we might be doing exactly what we want to do.
             *
             * This code conditionally ignores the warning in gcc 8 and above, while doing nothing for versions < gcc 8 which do not
             * understand -Wstringop-truncation
             */
            #if defined (__GNUC__) && (__GNUC__ > 7)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wstringop-truncation"
            #endif

            strncpy(MESSAGE, chMessage, 12);

            #if defined (__GNUC__) && (__GNUC__ > 7)
            #pragma GCC diagnostic pop
            #endif
        }


        /** SetLength
         *
         * Sets the size of the packet from Byte Vector.
         *
         *  @param[in] BYTES the byte buffer to set the length of.
         *
         **/
        void SetLength(const std::vector<uint8_t>& BYTES)
        {
            DataStream ssLength(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
            ssLength >> LENGTH;
        }


        /** SetChecksum
         *
         *  Set the Packet Checksum Data.
         *
         **/
        void SetChecksum()
        {
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            //memcpy(&CHECKSUM, &hash, sizeof(CHECKSUM));
            std::copy((uint8_t *)&hash, (uint8_t *)&hash + sizeof(CHECKSUM), (uint8_t *)&CHECKSUM);
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
            DATA = std::vector<uint8_t>(ssData.begin(), ssData.end());
            LENGTH = DATA.size();

            SetChecksum();
        }


        /** IsValid
         *
         *  Check the Validity of the Packet.
         *
         **/
        bool IsValid()
        {
            /* Check that the packet isn't nullptr. */
            if(IsNull())
                return false;

            /* Check the Header Bytes. */
            //if(memcmp(HEADER, (config::fTestNet ? MESSAGE_START_TESTNET : MESSAGE_START_MAINNET), sizeof(HEADER)) != 0)
            if(memory::compare((uint8_t *)HEADER, (uint8_t *)(config::fTestNet ? MESSAGE_START_TESTNET : MESSAGE_START_MAINNET), sizeof(HEADER)) != 0)
                return debug::error("Message Packet (Invalid Packet Header");

            /* Make sure Packet length is within bounds. (Max 512 MB Packet Size) */
            if (LENGTH > (1024 * 1024 * 512))
                return debug::error("Message Packet (", MESSAGE, ", ", LENGTH, " bytes) : Message too Large");

            /* Double check the Message Checksum. */
            uint512_t hash = LLC::SK512(DATA.begin(), DATA.end());
            uint32_t nChecksum = 0;
            //memcpy(&nChecksum, &hash, sizeof(nChecksum));
            std::copy((uint8_t *)&hash, (uint8_t *)&hash + sizeof(nChecksum), (uint8_t *)&nChecksum);

            if (nChecksum != CHECKSUM)
                return debug::error("Message Packet (", MESSAGE, ", ", LENGTH,
                    " bytes) : CHECKSUM MISMATCH nChecksum=", nChecksum, " hdr.nChecksum=", CHECKSUM);

            return true;
        }


        /** GetBytes
         *
         *  Serializes class into a Byte Vector. Used to write Packet to Sockets.
         *
         *  @return Returns the serialized byte vector.
         *
         **/
        std::vector<uint8_t> GetBytes() const
        {
            DataStream ssHeader(SER_NETWORK, MIN_PROTO_VERSION);
            ssHeader << *this;

            std::vector<uint8_t> BYTES(ssHeader.begin(), ssHeader.end());
            BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());

            return BYTES;
        }
    };
}

#endif
