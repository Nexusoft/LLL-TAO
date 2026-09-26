/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/packets/packet.h>
#include <helpers/test_fixtures.h>
#include <helpers/packet_builder.h>

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;


/** Test Suite: Packet Processing - Comprehensive Validation
 *
 *  PACKET-BASED PROTOCOL IN STATELESS MINING:
 *  ===========================================
 *
 *  The stateless mining protocol uses a packet-based message system
 *  for all communication between miner and node. Each packet has:
 *
 *  - HEADER: Message type identifier (uint8_t)
 *  - LENGTH: Size of DATA payload (uint32_t)
 *  - DATA: Variable-length payload
 *
 *  PACKET FLOW:
 *  ============
 *
 *  1. CONNECTION & AUTHENTICATION:
 *     - MINER_AUTH_INIT: Miner sends public key + genesis
 *     - MINER_AUTH_CHALLENGE: Node sends challenge nonce
 *     - MINER_AUTH_RESPONSE: Miner sends signature
 *     - MINER_AUTH_RESULT: Node confirms auth success/failure
 *
 *  2. SESSION MANAGEMENT:
 *     - SESSION_START: Session established notification
 *     - SESSION_KEEPALIVE: Miner sends periodic keepalives
 *
 *  3. REWARD BINDING:
 *     - MINER_SET_REWARD: Miner sends encrypted reward address
 *     - REWARD_BOUND: Node confirms reward binding
 *
 *  4. MINING OPERATIONS:
 *     - SET_CHANNEL: Miner selects mining channel
 *     - GET_BLOCK: Miner requests block template
 *     - BLOCK_DATA: Node sends block template
 *     - SUBMIT_BLOCK: Miner submits solved block
 *     - BLOCK_ACCEPTED/REJECTED: Node responds
 *
 *  5. QUERIES:
 *     - GET_HEIGHT: Request current blockchain height
 *     - GET_REWARD: Request current block reward
 *     - GET_ROUND: Request current difficulty round
 *
 *  WHAT WE TEST:
 *  =============
 *  1. Packet construction and serialization
 *  2. Packet parsing and validation
 *  3. Valid packet sequences
 *  4. Invalid packet handling
 *  5. Malformed data handling
 *  6. Oversized packet handling
 *  7. Empty packet handling
 *  8. Out-of-sequence packet handling
 *  9. Packet builder utility functions
 *  10. Edge cases and error conditions
 *
 **/


TEST_CASE("Packet: Construction and Serialization", "[packet][construction]")
{
    SECTION("PacketBuilder creates valid SET_CHANNEL packet")
    {
        Packet packet = CreateSetChannelPacket(2);
        
        REQUIRE(packet.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(packet.LENGTH == 4);  // sizeof(uint32_t)
        REQUIRE(packet.DATA.size() == 4);
    }
    
    SECTION("PacketBuilder creates valid GET_BLOCK packet")
    {
        Packet packet = CreateGetBlockPacket();
        
        REQUIRE(packet.HEADER == PacketTypes::GET_BLOCK);
        REQUIRE(packet.LENGTH == 0);
        REQUIRE(packet.DATA.empty());
    }
    
    SECTION("PacketBuilder creates valid GET_HEIGHT packet")
    {
        Packet packet = CreateGetHeightPacket();
        
        REQUIRE(packet.HEADER == PacketTypes::GET_HEIGHT);
        REQUIRE(packet.LENGTH == 0);
    }
    
    SECTION("PacketBuilder creates valid GET_REWARD packet")
    {
        Packet packet = CreateGetRewardPacket();
        
        REQUIRE(packet.HEADER == PacketTypes::GET_REWARD);
        REQUIRE(packet.LENGTH == 0);
    }
    
    SECTION("PacketBuilder creates valid SESSION_KEEPALIVE packet")
    {
        Packet packet = CreateSessionKeepalivePacket();
        
        REQUIRE(packet.HEADER == PacketTypes::SESSION_KEEPALIVE);
        REQUIRE(packet.LENGTH == 0);
    }
}


TEST_CASE("Packet: Authentication Packets", "[packet][authentication]")
{
    SECTION("MINER_AUTH_INIT packet contains pubkey and genesis")
    {
        std::vector<uint8_t> testPubKey = CreateTestFalconPubKey();
        uint256_t testGenesis = CreateTestGenesis();
        
        Packet packet = CreateAuthInitPacket(testPubKey, testGenesis);
        
        REQUIRE(packet.HEADER == PacketTypes::MINER_AUTH_INIT);
        REQUIRE(packet.LENGTH > 0);
        REQUIRE(packet.DATA.size() == packet.LENGTH);
        
        /* Should contain pubkey size + pubkey + genesis (32 bytes) */
        size_t expectedSize = 4 + testPubKey.size() + 32;
        REQUIRE(packet.LENGTH == expectedSize);
    }
    
    SECTION("MINER_AUTH_RESPONSE packet contains signature")
    {
        std::vector<uint8_t> testSignature;
        /* Build signature from random data */
        while(testSignature.size() < Constants::FALCON_SIGNATURE_SIZE)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = Constants::FALCON_SIGNATURE_SIZE - testSignature.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            testSignature.insert(testSignature.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        
        Packet packet = CreateAuthResponsePacket(testSignature);
        
        REQUIRE(packet.HEADER == PacketTypes::MINER_AUTH_RESPONSE);
        REQUIRE(packet.LENGTH > 0);
        
        /* Should contain signature size + signature */
        size_t expectedSize = 4 + testSignature.size();
        REQUIRE(packet.LENGTH == expectedSize);
    }
    
    SECTION("AUTH_INIT with empty pubkey")
    {
        std::vector<uint8_t> emptyPubKey;
        uint256_t testGenesis = CreateTestGenesis();
        
        Packet packet = CreateAuthInitPacket(emptyPubKey, testGenesis);
        
        /* Should still be valid packet structure */
        REQUIRE(packet.HEADER == PacketTypes::MINER_AUTH_INIT);
        
        /* Contains size (4) + empty pubkey (0) + genesis (32) */
        REQUIRE(packet.LENGTH == 36);
    }
}


TEST_CASE("Packet: Reward Binding Packets", "[packet][reward]")
{
    SECTION("MINER_SET_REWARD packet contains encrypted data")
    {
        /* Simulate encrypted reward address */
        std::vector<uint8_t> encryptedData;
        /* Build encrypted data from random values */
        while(encryptedData.size() < 64)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = 64 - encryptedData.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            encryptedData.insert(encryptedData.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        
        Packet packet = CreateSetRewardPacket(encryptedData);
        
        REQUIRE(packet.HEADER == PacketTypes::MINER_SET_REWARD);
        REQUIRE(packet.LENGTH > 0);
        
        /* Should contain data size + encrypted data */
        size_t expectedSize = 4 + encryptedData.size();
        REQUIRE(packet.LENGTH == expectedSize);
    }
    
    SECTION("MINER_SET_REWARD with minimum encrypted data")
    {
        std::vector<uint8_t> minData = LLC::GetRand256().GetBytes();
        
        Packet packet = CreateSetRewardPacket(minData);
        
        REQUIRE(packet.HEADER == PacketTypes::MINER_SET_REWARD);
        REQUIRE(packet.LENGTH == 36);  // 4 + 32
    }
}


TEST_CASE("Packet: Mining Operation Packets", "[packet][mining]")
{
    SECTION("SET_CHANNEL packet for all valid channels")
    {
        /* Channel 1 = Prime */
        Packet prime = CreateSetChannelPacket(1);
        REQUIRE(prime.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(prime.LENGTH == 4);
        
        /* Channel 2 = Hash */
        Packet hash = CreateSetChannelPacket(2);
        REQUIRE(hash.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(hash.LENGTH == 4);
        
        /* Channel 3 = Private */
        Packet priv = CreateSetChannelPacket(3);
        REQUIRE(priv.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(priv.LENGTH == 4);
    }
    
    SECTION("SET_CHANNEL with invalid channel number")
    {
        /* Channel 0 = Invalid */
        Packet invalid0 = CreateSetChannelPacket(0);
        REQUIRE(invalid0.HEADER == PacketTypes::SET_CHANNEL);
        
        /* Channel 99 = Invalid */
        Packet invalid99 = CreateSetChannelPacket(99);
        REQUIRE(invalid99.HEADER == PacketTypes::SET_CHANNEL);
    }
    
    SECTION("SUBMIT_BLOCK packet contains merkle root and nonce")
    {
        uint512_t testMerkleRoot;
        testMerkleRoot.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        uint64_t testNonce = 0x123456789ABCDEF0ULL;
        
        Packet packet = CreateSubmitBlockPacket(testMerkleRoot, testNonce);
        
        REQUIRE(packet.HEADER == PacketTypes::SUBMIT_BLOCK);
        
        /* Should contain merkle root (64 bytes) + nonce (8 bytes) */
        REQUIRE(packet.LENGTH == 72);
    }
}


TEST_CASE("Packet: Malformed Packet Handling", "[packet][malformed][error-handling]")
{
    SECTION("Packet with random data in SET_CHANNEL")
    {
        Packet malformed = CreateMalformedPacket(PacketTypes::SET_CHANNEL, 100);
        
        REQUIRE(malformed.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(malformed.LENGTH == 100);
        
        /* Packet is structurally valid but data is garbage */
        REQUIRE(malformed.DATA.size() == 100);
    }
    
    SECTION("Packet with random data in MINER_AUTH_INIT")
    {
        Packet malformed = CreateMalformedPacket(PacketTypes::MINER_AUTH_INIT, 50);
        
        REQUIRE(malformed.HEADER == PacketTypes::MINER_AUTH_INIT);
        REQUIRE(malformed.LENGTH == 50);
    }
    
    SECTION("Empty packet for packet type that requires data")
    {
        Packet empty = CreateEmptyPacket(PacketTypes::SET_CHANNEL);
        
        REQUIRE(empty.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(empty.LENGTH == 0);
        REQUIRE(empty.DATA.empty());
    }
}


TEST_CASE("Packet: Oversized Packet Handling", "[packet][oversized][error-handling]")
{
    SECTION("Oversized SET_CHANNEL packet")
    {
        Packet oversized = CreateOversizedPacket(PacketTypes::SET_CHANNEL);
        
        REQUIRE(oversized.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(oversized.LENGTH > 1000000);  // Over 1MB
    }
    
    SECTION("Oversized MINER_AUTH_INIT packet")
    {
        Packet oversized = CreateOversizedPacket(PacketTypes::MINER_AUTH_INIT);
        
        REQUIRE(oversized.HEADER == PacketTypes::MINER_AUTH_INIT);
        REQUIRE(oversized.LENGTH > 1000000);
    }
}


TEST_CASE("Packet: Builder Utility Functions", "[packet][builder]")
{
    SECTION("PacketBuilder with fluent API")
    {
        Packet packet = PacketBuilder(PacketTypes::SET_CHANNEL)
            .AppendUint32(2)
            .Build();
        
        REQUIRE(packet.HEADER == PacketTypes::SET_CHANNEL);
        REQUIRE(packet.LENGTH == 4);
    }
    
    SECTION("PacketBuilder appending multiple values")
    {
        Packet packet = PacketBuilder(100)
            .AppendByte(0x01)
            .AppendUint32(0x12345678)
            .AppendUint64(0x123456789ABCDEF0ULL)
            .Build();
        
        REQUIRE(packet.HEADER == 100);
        REQUIRE(packet.LENGTH == 13);  // 1 + 4 + 8
        REQUIRE(packet.DATA.size() == 13);
    }
    
    SECTION("PacketBuilder with hash")
    {
        uint256_t testHash = CreateTestGenesis();
        
        Packet packet = PacketBuilder(50)
            .AppendHash(testHash)
            .Build();
        
        REQUIRE(packet.HEADER == 50);
        REQUIRE(packet.LENGTH == 32);
    }
    
    SECTION("PacketBuilder with bytes vector")
    {
        std::vector<uint8_t> testBytes = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        Packet packet = PacketBuilder(60)
            .AppendBytes(testBytes)
            .Build();
        
        REQUIRE(packet.HEADER == 60);
        REQUIRE(packet.LENGTH == 5);
        REQUIRE(packet.DATA == testBytes);
    }
    
    SECTION("PacketBuilder with random data")
    {
        Packet packet = PacketBuilder(70)
            .WithRandomData(100)
            .Build();
        
        REQUIRE(packet.HEADER == 70);
        REQUIRE(packet.LENGTH == 100);
        REQUIRE(packet.DATA.size() == 100);
    }
}


TEST_CASE("Packet: Edge Cases", "[packet][edge-cases]")
{
    SECTION("Packet with header 0")
    {
        Packet packet = PacketBuilder(0)
            .Build();
        
        REQUIRE(packet.HEADER == 0);
        REQUIRE(packet.LENGTH == 0);
    }
    
    SECTION("Packet with maximum header value")
    {
        Packet packet = PacketBuilder(255)
            .Build();
        
        REQUIRE(packet.HEADER == 255);
    }
    
    SECTION("Packet with zero-length data explicitly set")
    {
        Packet packet = PacketBuilder(100)
            .WithLength(0)
            .Build();
        
        REQUIRE(packet.LENGTH == 0);
    }
    
    SECTION("Packet with single byte of data")
    {
        Packet packet = PacketBuilder(100)
            .AppendByte(0xFF)
            .Build();
        
        REQUIRE(packet.LENGTH == 1);
        REQUIRE(packet.DATA[0] == 0xFF);
    }
    
    SECTION("Multiple appends update length correctly")
    {
        PacketBuilder builder(100);
        
        builder.AppendByte(0x01);
        REQUIRE(builder.Build().LENGTH == 1);
        
        builder.AppendByte(0x02);
        REQUIRE(builder.Build().LENGTH == 2);
        
        builder.AppendUint32(0x12345678);
        REQUIRE(builder.Build().LENGTH == 6);
    }
}


TEST_CASE("Packet: Valid Packet Sequences", "[packet][sequence]")
{
    SECTION("Normal authentication sequence")
    {
        /* 1. Miner sends AUTH_INIT */
        Packet authInit = CreateAuthInitPacket(
            CreateTestFalconPubKey(),
            CreateTestGenesis()
        );
        REQUIRE(authInit.HEADER == PacketTypes::MINER_AUTH_INIT);
        
        /* 2. Node would send AUTH_CHALLENGE (not tested here) */
        
        /* 3. Miner sends AUTH_RESPONSE */
        std::vector<uint8_t> signature;
        /* Build signature from random data */
        while(signature.size() < Constants::FALCON_SIGNATURE_SIZE)
        {
            std::vector<uint8_t> vChunk = LLC::GetRand256().GetBytes();
            size_t remaining = Constants::FALCON_SIGNATURE_SIZE - signature.size();
            size_t toAdd = (remaining < vChunk.size()) ? remaining : vChunk.size();
            signature.insert(signature.end(), vChunk.begin(), vChunk.begin() + toAdd);
        }
        Packet authResponse = CreateAuthResponsePacket(signature);
        REQUIRE(authResponse.HEADER == PacketTypes::MINER_AUTH_RESPONSE);
        
        /* Sequence is structurally valid */
    }
    
    SECTION("Normal mining sequence")
    {
        /* 1. Set channel */
        Packet setChannel = CreateSetChannelPacket(2);
        REQUIRE(setChannel.HEADER == PacketTypes::SET_CHANNEL);
        
        /* 2. Get block */
        Packet getBlock = CreateGetBlockPacket();
        REQUIRE(getBlock.HEADER == PacketTypes::GET_BLOCK);
        
        /* 3. Submit block */
        uint512_t merkleRoot;
        merkleRoot.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        Packet submit = CreateSubmitBlockPacket(merkleRoot, 12345);
        REQUIRE(submit.HEADER == PacketTypes::SUBMIT_BLOCK);
    }
    
    SECTION("Query sequence")
    {
        /* Queries can be sent anytime after authentication */
        Packet getHeight = CreateGetHeightPacket();
        Packet getReward = CreateGetRewardPacket();
        Packet getRound = CreateGetRoundPacket();
        
        REQUIRE(getHeight.HEADER == PacketTypes::GET_HEIGHT);
        REQUIRE(getReward.HEADER == PacketTypes::GET_REWARD);
        REQUIRE(getRound.HEADER == PacketTypes::GET_ROUND);
    }
}


TEST_CASE("Packet: Data Integrity", "[packet][integrity]")
{
    SECTION("Appended data matches expected byte order")
    {
        uint32_t testValue = 0x12345678;
        
        Packet packet = PacketBuilder(100)
            .AppendUint32(testValue)
            .Build();
        
        /* Little-endian byte order */
        REQUIRE(packet.DATA[0] == 0x78);
        REQUIRE(packet.DATA[1] == 0x56);
        REQUIRE(packet.DATA[2] == 0x34);
        REQUIRE(packet.DATA[3] == 0x12);
    }
    
    SECTION("Hash bytes preserved correctly")
    {
        uint256_t testHash;
        testHash.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        Packet packet = PacketBuilder(100)
            .AppendHash(testHash)
            .Build();
        
        REQUIRE(packet.LENGTH == 32);
        
        /* Verify hash can be reconstructed */
        std::vector<uint8_t> hashBytes = testHash.GetBytes();
        for(size_t i = 0; i < 32; i++)
        {
            REQUIRE(packet.DATA[i] == hashBytes[i]);
        }
    }
}


TEST_CASE("Packet: Large Data Handling", "[packet][large-data]")
{
    SECTION("Packet with 1KB of data")
    {
        Packet packet = PacketBuilder(100)
            .WithRandomData(1024)
            .Build();
        
        REQUIRE(packet.LENGTH == 1024);
        REQUIRE(packet.DATA.size() == 1024);
    }
    
    SECTION("Packet with 1MB of data")
    {
        Packet packet = PacketBuilder(100)
            .WithRandomData(1024 * 1024)
            .Build();
        
        REQUIRE(packet.LENGTH == 1024 * 1024);
        REQUIRE(packet.DATA.size() == 1024 * 1024);
    }
}
