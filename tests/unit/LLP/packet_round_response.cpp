/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/packets/packet.h>

/** Test Packet::HasDataPayload() and GetBytes() for mining round response packets
 * 
 * This test verifies that NEW_ROUND (204) and OLD_ROUND (205) are correctly
 * identified as packets requiring data payloads, and that GetBytes() properly
 * serializes them with header + LENGTH + DATA fields.
 *
 * Bug Fix: Previously, HasDataPayload() returned false for headers 204-205,
 * causing GetBytes() to serialize only the header byte, dropping the LENGTH
 * and DATA fields entirely (1 byte instead of 17 bytes for 12-byte payload).
 **/

TEST_CASE("Packet::HasDataPayload() for mining round response packets", "[packet][round_response]")
{
    SECTION("NEW_ROUND (204) requires data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 204;  // NEW_ROUND
        packet.LENGTH = 12;   // unified height + channel height + difficulty
        
        REQUIRE(packet.HasDataPayload() == true);
    }
    
    SECTION("OLD_ROUND (205) requires data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 205;  // OLD_ROUND
        packet.LENGTH = 12;   // Variable length, using 12 as example
        
        REQUIRE(packet.HasDataPayload() == true);
    }
}


TEST_CASE("Packet::GetBytes() serialization for NEW_ROUND", "[packet][round_response][serialization]")
{
    SECTION("NEW_ROUND with 12-byte payload serializes to 17 bytes (header + LENGTH + DATA)")
    {
        LLP::Packet packet;
        packet.HEADER = 204;  // NEW_ROUND
        packet.LENGTH = 12;
        
        /* Create 12-byte payload: unified height (4) + channel height (4) + difficulty (4) */
        std::vector<uint8_t> vData = {
            0xDC, 0xAE, 0x9E, 0x00,  // nUnifiedHeight (little-endian)
            0x08, 0x28, 0x23, 0x00,  // nChannelHeight (little-endian)
            0xFF, 0xFF, 0x00, 0x1D   // nDifficulty (little-endian)
        };
        packet.DATA = vData;
        
        /* Get serialized bytes */
        std::vector<uint8_t> bytes = packet.GetBytes();
        
        /* Verify packet is 17 bytes: 1 (header) + 4 (length) + 12 (data) */
        REQUIRE(bytes.size() == 17);
        
        /* Verify header byte */
        REQUIRE(bytes[0] == 204);
        
        /* Verify LENGTH field (4 bytes, big-endian) */
        uint32_t serialized_length = (bytes[1] << 24) | (bytes[2] << 16) | (bytes[3] << 8) | bytes[4];
        REQUIRE(serialized_length == 12);
        
        /* Verify DATA field matches original */
        for(size_t i = 0; i < vData.size(); ++i)
        {
            REQUIRE(bytes[5 + i] == vData[i]);
        }
    }
}


TEST_CASE("Packet::GetBytes() serialization for OLD_ROUND", "[packet][round_response][serialization]")
{
    SECTION("OLD_ROUND with 8-byte payload serializes to 13 bytes (header + LENGTH + DATA)")
    {
        LLP::Packet packet;
        packet.HEADER = 205;  // OLD_ROUND
        packet.LENGTH = 8;
        
        /* Create 8-byte payload (example rejection data) */
        std::vector<uint8_t> vData = {
            0x01, 0x00, 0x00, 0x00,  // Rejection reason code
            0xAB, 0xCD, 0xEF, 0x12   // Additional data
        };
        packet.DATA = vData;
        
        /* Get serialized bytes */
        std::vector<uint8_t> bytes = packet.GetBytes();
        
        /* Verify packet is 13 bytes: 1 (header) + 4 (length) + 8 (data) */
        REQUIRE(bytes.size() == 13);
        
        /* Verify header byte */
        REQUIRE(bytes[0] == 205);
        
        /* Verify LENGTH field (4 bytes, big-endian) */
        uint32_t serialized_length = (bytes[1] << 24) | (bytes[2] << 16) | (bytes[3] << 8) | bytes[4];
        REQUIRE(serialized_length == 8);
        
        /* Verify DATA field matches original */
        for(size_t i = 0; i < vData.size(); ++i)
        {
            REQUIRE(bytes[5 + i] == vData[i]);
        }
    }
}


TEST_CASE("Packet::Header() validation for round response packets", "[packet][round_response]")
{
    SECTION("NEW_ROUND with LENGTH > 0 is valid header")
    {
        LLP::Packet packet;
        packet.HEADER = 204;  // NEW_ROUND
        packet.LENGTH = 12;
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("OLD_ROUND with LENGTH > 0 is valid header")
    {
        LLP::Packet packet;
        packet.HEADER = 205;  // OLD_ROUND
        packet.LENGTH = 8;
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("NEW_ROUND with LENGTH = 0 is INVALID header")
    {
        LLP::Packet packet;
        packet.HEADER = 204;  // NEW_ROUND
        packet.LENGTH = 0;    // Invalid - should have data
        
        REQUIRE(packet.Header() == false);
    }
}


TEST_CASE("Packet::Complete() for round response packets", "[packet][round_response]")
{
    SECTION("NEW_ROUND packet is complete when DATA matches LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 204;
        packet.LENGTH = 12;
        packet.DATA.resize(12, 0x00);
        
        REQUIRE(packet.Complete() == true);
    }
    
    SECTION("NEW_ROUND packet is incomplete when DATA.size() < LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 204;
        packet.LENGTH = 12;
        packet.DATA.resize(6, 0x00);  // Partial data
        
        REQUIRE(packet.Complete() == false);
    }
}
