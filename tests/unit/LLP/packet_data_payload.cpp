/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/packets/packet.h>
#include <LLP/types/miner.h>  /* For MINER_SET_REWARD and MINER_REWARD_RESULT constants */

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
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("MINER_REWARD_RESULT with LENGTH > 0 is valid header")
    {
        LLP::Packet packet;
        packet.HEADER = 214;  // LLP::Miner::MINER_REWARD_RESULT
        packet.LENGTH = 50;
        
        REQUIRE(packet.Header() == true);
    }
    
    SECTION("MINER_SET_REWARD with LENGTH = 0 is INVALID header")
    {
        LLP::Packet packet;
        packet.HEADER = 213;  // LLP::Miner::MINER_SET_REWARD
        packet.LENGTH = 0;    // Invalid - should have data
        
        REQUIRE(packet.Header() == false);
    }
}


TEST_CASE("Packet::Complete() for reward binding packets", "[packet][reward]")
{
    SECTION("MINER_SET_REWARD packet is complete when DATA matches LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 213;
        packet.LENGTH = 65;
        packet.DATA.resize(65, 0x00);
        
        REQUIRE(packet.Complete() == true);
    }
    
    SECTION("MINER_SET_REWARD packet is incomplete when DATA.size() < LENGTH")
    {
        LLP::Packet packet;
        packet.HEADER = 213;
        packet.LENGTH = 65;
        packet.DATA.resize(30, 0x00);  // Partial data
        
        REQUIRE(packet.Complete() == false);
    }
}
