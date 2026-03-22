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

/* Test packet length validation for request opcodes and fixed-size packets
 * This validates the fixes for Bug #1: Packet parser rejection of impossible LENGTH values
 */

TEST_CASE("Request opcodes must have zero-length payloads", "[llp][packet][validation]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;
    
    SECTION("GET_BLOCK (129) with LENGTH=0 should be valid")
    {
        Packet packet;
        packet.HEADER = 129; // GET_BLOCK
        packet.LENGTH = 0;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
        REQUIRE(strError.empty());
    }
    
    SECTION("GET_BLOCK (129) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 129; // GET_BLOCK
        packet.LENGTH = 100;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
        REQUIRE(strError.find("no data payload") != std::string::npos);
    }
    
    SECTION("GET_HEIGHT (130) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 130; // GET_HEIGHT
        packet.LENGTH = 50;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
        REQUIRE(!strError.empty());
    }
    
    SECTION("GET_REWARD (131) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 131; // GET_REWARD
        packet.LENGTH = 1;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("GET_ROUND (133) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 133; // GET_ROUND
        packet.LENGTH = 16;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("PING (253) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 253; // PING
        packet.LENGTH = 1000;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("CLOSE (254) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 254; // CLOSE
        packet.LENGTH = 8;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("MINER_READY (216) with LENGTH>0 should be rejected")
    {
        Packet packet;
        packet.HEADER = 216; // MINER_READY
        packet.LENGTH = 4;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
}

TEST_CASE("Fixed-size opcodes have maximum length limits", "[llp][packet][validation]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;
    
    SECTION("SET_CHANNEL with LENGTH<=4 should be valid")
    {
        Packet packet;
        packet.HEADER = 3; // SET_CHANNEL
        packet.LENGTH = 4;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
    }
    
    SECTION("SET_CHANNEL with LENGTH>4 should be rejected")
    {
        Packet packet;
        packet.HEADER = 3; // SET_CHANNEL
        packet.LENGTH = 5;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
        REQUIRE(strError.find("exceeds maximum") != std::string::npos);
    }
    
    SECTION("SESSION_KEEPALIVE with LENGTH<=8 should be valid")
    {
        Packet packet;
        packet.HEADER = 212; // SESSION_KEEPALIVE
        packet.LENGTH = 4;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
    }
    
    SECTION("SESSION_KEEPALIVE with LENGTH>8 should be rejected")
    {
        Packet packet;
        packet.HEADER = 212; // SESSION_KEEPALIVE
        packet.LENGTH = 1000;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
        REQUIRE(strError.find("exceeds maximum") != std::string::npos);
    }
    
    SECTION("SESSION_START with LENGTH<=8 should be valid")
    {
        Packet packet;
        packet.HEADER = 211; // SESSION_START
        packet.LENGTH = 8;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
    }
    
    SECTION("SESSION_START with LENGTH>8 should be rejected")
    {
        Packet packet;
        packet.HEADER = 211; // SESSION_START
        packet.LENGTH = 100;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("MINER_AUTH_CHALLENGE with LENGTH<=40 should be valid")
    {
        Packet packet;
        packet.HEADER = 208; // MINER_AUTH_CHALLENGE
        packet.LENGTH = 34;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
    }
    
    SECTION("MINER_AUTH_CHALLENGE with LENGTH>40 should be rejected")
    {
        Packet packet;
        packet.HEADER = 208; // MINER_AUTH_CHALLENGE
        packet.LENGTH = 50;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("MINER_AUTH_RESULT with LENGTH<=10 should be valid")
    {
        Packet packet;
        packet.HEADER = 210; // MINER_AUTH_RESULT
        packet.LENGTH = 6;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == true);
    }
    
    SECTION("MINER_AUTH_RESULT with LENGTH>10 should be rejected")
    {
        Packet packet;
        packet.HEADER = 210; // MINER_AUTH_RESULT
        packet.LENGTH = 20;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
}

TEST_CASE("Large payload attack vectors are blocked", "[llp][packet][security]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;
    
    SECTION("Malicious GET_BLOCK with 1MB payload should be rejected")
    {
        Packet packet;
        packet.HEADER = 129; // GET_BLOCK
        packet.LENGTH = 1024 * 1024; // 1MB
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("Malicious PING with large payload should be rejected")
    {
        Packet packet;
        packet.HEADER = 253; // PING
        packet.LENGTH = 10000;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
    
    SECTION("Malicious SESSION_KEEPALIVE with excessive payload should be rejected")
    {
        Packet packet;
        packet.HEADER = 212; // SESSION_KEEPALIVE
        packet.LENGTH = 500;
        
        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);
        
        REQUIRE(bValid == false);
    }
}
