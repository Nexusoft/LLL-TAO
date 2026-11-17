/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>

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

TEST_CASE("Falcon auth packets enforce legacy and stateless length ceilings", "[llp][packet][validation][auth]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;

    SECTION("Legacy MINER_AUTH_INIT rejects oversized payloads")
    {
        Packet packet(Opcodes::MINER_AUTH_INIT);
        packet.LENGTH = static_cast<uint32_t>(FalconConstants::MINER_AUTH_INIT_MAX + 1);

        std::string strError;
        REQUIRE_FALSE(ValidatePacketLength(packet, &strError));
        REQUIRE(strError.find("MINER_AUTH_INIT") != std::string::npos);
    }

    SECTION("Legacy MINER_AUTH_RESPONSE rejects oversized payloads")
    {
        Packet packet(Opcodes::MINER_AUTH_RESPONSE);
        packet.LENGTH = static_cast<uint32_t>(FalconConstants::AUTH_RESPONSE_ENCRYPTED_MAX + 1);

        std::string strError;
        REQUIRE_FALSE(ValidatePacketLength(packet, &strError));
        REQUIRE(strError.find("MINER_AUTH_RESPONSE") != std::string::npos);
    }

    SECTION("Stateless mirrored auth opcodes inherit the same ceilings")
    {
        StatelessPacket init(Stateless::AUTH_INIT);
        init.LENGTH = static_cast<uint32_t>(FalconConstants::MINER_AUTH_INIT_MAX + 1);

        std::string strError;
        REQUIRE_FALSE(ValidatePacketLength(init, &strError));
        REQUIRE(strError.find("MINER_AUTH_INIT") != std::string::npos);

        StatelessPacket response(Stateless::AUTH_RESPONSE);
        response.LENGTH = static_cast<uint32_t>(FalconConstants::AUTH_RESPONSE_ENCRYPTED_MAX + 1);
        strError.clear();

        REQUIRE_FALSE(ValidatePacketLength(response, &strError));
        REQUIRE(strError.find("MINER_AUTH_RESPONSE") != std::string::npos);
    }
}

TEST_CASE("Mirrored stateless opcodes inherit legacy payload rules", "[llp][packet][validation][stateless]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;

    SECTION("STATELESS_GET_ROUND with LENGTH>0 is rejected")
    {
        StatelessPacket packet(Stateless::GET_ROUND);
        packet.LENGTH = 16;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("GET_ROUND") != std::string::npos);
    }

    SECTION("STATELESS_GET_HEIGHT with LENGTH>0 is rejected")
    {
        StatelessPacket packet(Stateless::GET_HEIGHT);
        packet.LENGTH = 4;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("GET_HEIGHT") != std::string::npos);
    }

    SECTION("STATELESS_MINER_READY with LENGTH>0 is rejected")
    {
        StatelessPacket packet(Stateless::MINER_READY);
        packet.LENGTH = 1;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("MINER_READY") != std::string::npos);
    }

    SECTION("STATELESS_SESSION_KEEPALIVE keeps the legacy max payload limit")
    {
        StatelessPacket packet(Stateless::SESSION_KEEPALIVE);
        packet.LENGTH = 9;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("SESSION_KEEPALIVE") != std::string::npos);
    }

    SECTION("STATELESS_SESSION_STATUS_ACK keeps its fixed-size contract")
    {
        StatelessPacket packet(Stateless::SESSION_STATUS_ACK);
        packet.LENGTH = 15;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("requires exactly 16 bytes") != std::string::npos);
    }

    SECTION("Unmirrored NODE_SHUTDOWN keeps stateless-only fixed-size contract")
    {
        StatelessPacket packet(Stateless::NODE_SHUTDOWN);
        packet.LENGTH = 3;

        std::string strError;
        bool bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == false);
        REQUIRE(strError.find("requires exactly 4 bytes") != std::string::npos);

        packet.LENGTH = 4;
        strError.clear();
        bValid = ValidatePacketLength(packet, &strError);

        REQUIRE(bValid == true);
        REQUIRE(strError.empty());
    }
}

TEST_CASE("Packet completion waits for the physical length field read", "[llp][packet][validation][framing]")
{
    using namespace LLP;
    using namespace LLP::OpcodeUtility;

    SECTION("Legacy packet header stays incomplete until SetLength runs")
    {
        Packet packet(Opcodes::GET_BLOCK);

        REQUIRE(packet.Header() == false);
        REQUIRE(packet.Complete() == false);

        packet.SetLength({0x00, 0x00, 0x00, 0x00});

        REQUIRE(packet.Header() == true);
        REQUIRE(packet.Complete() == true);
    }

    SECTION("Stateless packet header stays incomplete until SetLength runs")
    {
        StatelessPacket packet(Stateless::GET_BLOCK);

        REQUIRE(packet.Header() == false);
        REQUIRE(packet.Complete() == false);

        packet.SetLength({0x00, 0x00, 0x00, 0x00});

        REQUIRE(packet.Header() == true);
        REQUIRE(packet.Complete() == true);
    }

    SECTION("Payload packets remain incomplete until the declared bytes arrive")
    {
        Packet packet(Opcodes::BLOCK_DATA);
        packet.SetLength({0x00, 0x00, 0x00, 0x03});

        REQUIRE(packet.Header() == true);
        REQUIRE(packet.Complete() == false);

        packet.DATA = {0xAA, 0xBB, 0xCC};

        REQUIRE(packet.Complete() == true);
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

TEST_CASE("LooksLikeStatelessFrameOnLegacy correctly identifies misplaced stateless frames", "[llp][packet][security][lane]")
{
    using namespace LLP::OpcodeUtility;

    SECTION("0xD0D8 stateless MINER_READY on legacy lane is diagnosed")
    {
        /* Bytes [0xD0][0xD8][0][0][0][0] are stateless MINER_READY length 0,
         * but a legacy reader sees HEADER=0xD0 and LENGTH=0xD8000000. */
        REQUIRE(LooksLikeStatelessFrameOnLegacy(
            0xD0,
            Opcodes::MINER_READY,
            0xD8000000u));
    }

    SECTION("valid legacy MINER_AUTH_CHALLENGE is not diagnosed")
    {
        REQUIRE_FALSE(LooksLikeStatelessFrameOnLegacy(
            Opcodes::MINER_AUTH_CHALLENGE,
            0x00,
            32u));
    }

    SECTION("oversized non-0xD0 legacy packet is not called stateless")
    {
        REQUIRE_FALSE(LooksLikeStatelessFrameOnLegacy(
            Opcodes::GET_BLOCK,
            0xD8,
            MAX_ANY_PACKET_LENGTH + 1u));
    }
}
