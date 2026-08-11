/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/types/miner.h>  /* For MINER_SET_REWARD and MINER_REWARD_RESULT constants */

#include <array>

/** Test Packet::HasDataPayload() for reward address binding packets
 * 
 * This test verifies that MINER_SET_REWARD (213) and MINER_REWARD_RESULT (214)
 * are correctly identified as packets requiring data payloads.
 **/

TEST_CASE("Packet::HasDataPayload() for reward binding packets", "[packet][reward]")
{
    SECTION("MINER_SET_REWARD (213) requires data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 213;  // LLP::Miner::MINER_SET_REWARD
        packet.LENGTH = 65;   // Typical encrypted payload size
        
        REQUIRE(packet.HasDataPayload() == true);
    }
    
    SECTION("MINER_REWARD_RESULT (214) requires data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 214;  // LLP::Miner::MINER_REWARD_RESULT
        packet.LENGTH = 50;   // Typical encrypted response size
        
        REQUIRE(packet.HasDataPayload() == true);
    }
    
    SECTION("CHANNEL_ACK (206) requires data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 206;  // LLP::Miner::CHANNEL_ACK
        packet.LENGTH = 1;    // Channel number (1 byte)
        
        REQUIRE(packet.HasDataPayload() == true);
    }
    
    SECTION("Traditional data packets (< 128) require data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 1;   // SUBMIT_BLOCK
        packet.LENGTH = 100;
        
        REQUIRE(packet.HasDataPayload() == true);
    }
    
    SECTION("Falcon auth packets (207-212) require data payload")
    {
        for(uint8_t header = 207; header <= 212; ++header)
        {
            LLP::Packet packet;
            packet.HEADER = header;
            packet.LENGTH = 50;
            
            REQUIRE(packet.HasDataPayload() == true);
        }
    }
    
    SECTION("Request packets (215-254) do NOT require data payload")
    {
        LLP::Packet packet;
        packet.HEADER = 253;  // PING
        packet.LENGTH = 0;
        
        REQUIRE(packet.HasDataPayload() == false);
    }
}


TEST_CASE("Packet::Header() validation for reward binding packets", "[packet][reward]")
{
    SECTION("MINER_SET_REWARD with LENGTH > 0 is valid header")
    {
        LLP::Packet packet;
        packet.HEADER = 213;  // LLP::Miner::MINER_SET_REWARD
        packet.LENGTH = 65;
        packet.fLengthRead = true;
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("MINER_REWARD_RESULT with LENGTH > 0 is valid header")
    {
        LLP::Packet packet;
        packet.HEADER = 214;  // LLP::Miner::MINER_REWARD_RESULT
        packet.LENGTH = 50;
        packet.fLengthRead = true;
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("MINER_SET_REWARD with LENGTH = 0 still has complete framing")
    {
        LLP::Packet packet;
        packet.HEADER = 213;  // LLP::Miner::MINER_SET_REWARD
        packet.LENGTH = 0;
        packet.fLengthRead = true;
         
        REQUIRE(packet.Header() == true);
    }
}


TEST_CASE("Packet::Complete() for reward binding packets", "[packet][reward]")
{
    SECTION("MINER_SET_REWARD packet is complete when DATA matches LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 213;
        packet.LENGTH = 65;
        packet.fLengthRead = true;
        packet.DATA.resize(65, 0x00);
        
        REQUIRE(packet.Complete() == true);
    }
    
    SECTION("MINER_SET_REWARD packet is incomplete when DATA.size() < LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 213;
        packet.LENGTH = 65;
        packet.fLengthRead = true;
        packet.DATA.resize(30, 0x00);  // Partial data
        
        REQUIRE(packet.Complete() == false);
    }
}


TEST_CASE("Packet::HasDataPayload() for control/status mining packets", "[packet][control]")
{
    using namespace LLP::OpcodeUtility;

    SECTION("BLOCK_REJECTED may carry GET_BLOCK control payload bytes")
    {
        LLP::Packet packet;
        packet.HEADER = Opcodes::BLOCK_REJECTED;
        packet.LENGTH = 8;

        REQUIRE(packet.HasDataPayload() == true);
        REQUIRE(HasDataPayload16(Stateless::BLOCK_REJECTED) == true);
    }

    SECTION("SESSION_EXPIRED may carry session_id and reason bytes")
    {
        LLP::Packet packet;
        packet.HEADER = Opcodes::SESSION_EXPIRED;
        packet.LENGTH = 5;

        REQUIRE(packet.HasDataPayload() == true);
        REQUIRE(HasDataPayload16(Stateless::SESSION_EXPIRED) == true);
    }
}


TEST_CASE("Legacy packet framing always includes the 4-byte length field", "[packet][framing]")
{
    using namespace LLP::OpcodeUtility;

    SECTION("Header-only GET_BLOCK serializes as HEADER + zero LENGTH")
    {
        LLP::Packet packet(Opcodes::GET_BLOCK);
        packet.LENGTH = 0;
        packet.fLengthRead = true;

        const std::vector<uint8_t> bytes = packet.GetBytes();

        REQUIRE(bytes.size() == 5);
        REQUIRE(bytes[0] == Opcodes::GET_BLOCK);
        REQUIRE(bytes[1] == 0x00);
        REQUIRE(bytes[2] == 0x00);
        REQUIRE(bytes[3] == 0x00);
        REQUIRE(bytes[4] == 0x00);
        REQUIRE(packet.Header() == true);
    }

    SECTION("Representative mining opcodes have consistent framing and classification")
    {
        struct WireCase
        {
            uint8_t opcode;
            uint32_t length;
            bool expectPayloadCapable;
            bool expectHeaderOnlyRequest;
        };

        const std::array<WireCase, 4> cases{{
            {Opcodes::BLOCK_DATA,      3u, true,  false},
            {Opcodes::GET_BLOCK,       0u, false, true },
            {Opcodes::BLOCK_REJECTED,  8u, true,  false},
            {Opcodes::SESSION_EXPIRED, 5u, true,  false},
        }};

        for(const auto& testCase : cases)
        {
            INFO("opcode=0x" << std::hex << static_cast<uint32_t>(testCase.opcode) << std::dec);

            LLP::Packet packet(testCase.opcode);
            packet.LENGTH = testCase.length;
            packet.fLengthRead = true;
            packet.DATA.resize(testCase.length, 0xAB);

            const std::vector<uint8_t> bytes = packet.GetBytes();
            REQUIRE(bytes.size() == 5u + testCase.length);
            REQUIRE(bytes[0] == testCase.opcode);

            const uint32_t serializedLength =
                (static_cast<uint32_t>(bytes[1]) << 24) |
                (static_cast<uint32_t>(bytes[2]) << 16) |
                (static_cast<uint32_t>(bytes[3]) << 8) |
                static_cast<uint32_t>(bytes[4]);

            REQUIRE(serializedLength == testCase.length);
            REQUIRE(packet.HasDataPayload() == testCase.expectPayloadCapable);
            REQUIRE(IsHeaderOnlyRequest(testCase.opcode) == testCase.expectHeaderOnlyRequest);
            REQUIRE(packet.Header() == true);
            REQUIRE(packet.Complete() == true);
        }
    }
}
