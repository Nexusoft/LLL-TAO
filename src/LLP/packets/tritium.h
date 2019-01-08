/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_PACKETS_TRITIUM_H
#define NEXUS_LLP_PACKETS_TRITIUM_H

#include <vector>
#include <limits.h>
#include <cstdint>

#include <LLC/hash/SK.h>

#include <Util/include/debug.h>

namespace LLP
{

    /** Message format enumeration. **/
    enum
    {
        DAT_VERSION     = 0,
        GET_OFFSET      = 1,
        DAT_OFFSET      = 2,

        DAT_INVENTORY   = 10,
        GET_INVENTORY   = 11,

        GET_ADDRESSES   = 20,
        DAT_ADDRESSES   = 21,

        DAT_TRANSACTION = 30,

        DAT_PING        = 65533,
        DAT_PONG        = 65534,
        DAT_NULL        = 65535
    };


    /** Class to handle sending and receiving of More Complese Message LLP Packets. **/
    class TritiumPacket
    {
    public:

        /** Components of a Message LLP Packet.
         *
         *  BYTE 0 - 1  : Message Code
         *  BYTE 2 - 6  : Packet Length
         *  BYTE 7 - 10 : Packet Checksum
         *
         *  Byte 11 +   : Packet Data
         *
         **/
        uint16_t        MESSAGE;
        uint32_t	    LENGTH;
        uint32_t	    CHECKSUM;

        std::vector<uint8_t> DATA;

        TritiumPacket()
        {
            SetNull();
        }

        TritiumPacket(uint16_t nMessage)
        {
            SetNull();

            MESSAGE = nMessage;
        }

        IMPLEMENT_SERIALIZE
        (
            READWRITE(MESSAGE);
            READWRITE(LENGTH);
            READWRITE(CHECKSUM);
        )


        /* Set the Packet Null Flags. */
        void SetNull()
        {
            MESSAGE   = DAT_NULL;
            LENGTH    = 0;
            CHECKSUM  = 0;

            DATA.clear();
        }


        /* Packet Null Flag. Length and Checksum both 0. */
        bool IsNull() { return (MESSAGE == DAT_NULL && LENGTH == 0 && CHECKSUM == 0 && DATA.size() == 0); }


        /* Determine if a packet is fully read. */
        bool Complete() { return (Header() && DATA.size() == LENGTH); }


        /* Determine if header is fully read */
        bool Header()   { return IsNull() ? false : (CHECKSUM > 0 && MESSAGE != DAT_NULL); }


        /* Sets the size of the packet from Byte Vector. */
        void SetLength(std::vector<uint8_t> vBytes)
        {
            DataStream ssLength(vBytes, SER_NETWORK, MIN_PROTO_VERSION);
            ssLength >> LENGTH;
        }


        /* Set the Packet Checksum Data. */
        void SetChecksum()
        {
            CHECKSUM = LLC::SK32(DATA.begin(), DATA.end());
        }


        /* Set the Packet Data. */
        void SetData(DataStream ssData)
        {
            std::vector<uint8_t> vData(ssData.begin(), ssData.end());

            LENGTH = vData.size();
            DATA   = vData;

            SetChecksum();
        }


        /* Check the Validity of the Packet. */
        bool IsValid()
        {
            /* Check that the packet isn't nullptr. */
            if(IsNull())
                return false;

            /* Make sure Packet length is within bounds. (Max 2 MB Packet Size) */
            if (LENGTH > (1024 * 1024 * 2))
                return debug::error("Tritium Packet (%s, %u bytes) : Message too Large", MESSAGE, LENGTH);

            /* Double check the Message Checksum. */
            if (LLC::SK32(DATA.begin(), DATA.end()) != CHECKSUM)
                return debug::error("Tritium Packet (%s, %u bytes) : CHECKSUM MISMATCH nChecksum=%u hdr.nChecksum=%u",
                                    MESSAGE, LENGTH, LLC::SK32(DATA.begin(), DATA.end()), CHECKSUM);

            return true;
        }


        /* Serializes class into a Byte Vector. Used to write Packet to Sockets. */
        std::vector<uint8_t> GetBytes()
        {
            DataStream ssHeader(SER_NETWORK, MIN_PROTO_VERSION);
            ssHeader << *this;

            std::vector<uint8_t> vBytes(ssHeader.begin(), ssHeader.end());
            vBytes.insert(vBytes.end(), DATA.begin(), DATA.end());

            return vBytes;
        }
    };
}

#endif
