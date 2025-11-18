/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/packets/packet.h>

#include <Util/include/runtime.h>

using namespace LLP;

/* Packet type definitions for testing */
const Packet::message_t SET_CHANNEL = 3;
const Packet::message_t MINER_AUTH_RESPONSE = 209;
const Packet::message_t SESSION_START = 211;


TEST_CASE("MiningContext Immutability Tests", "[stateless_miner]")
{
    SECTION("Default constructor creates zero-initialized context")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.nChannel == 0);
        REQUIRE(ctx.nHeight == 0);
        REQUIRE(ctx.nTimestamp == 0);
        REQUIRE(ctx.strAddress == "");
        REQUIRE(ctx.nProtocolVersion == 0);
        REQUIRE(ctx.fAuthenticated == false);
        REQUIRE(ctx.nSessionId == 0);
        REQUIRE(ctx.hashKeyID == uint256_t(0));
        REQUIRE(ctx.hashGenesis == uint256_t(0));
    }
    
    SECTION("WithChannel creates new context with updated channel")
    {
        MiningContext ctx1;
        MiningContext ctx2 = ctx1.WithChannel(2);
        
        REQUIRE(ctx1.nChannel == 0);  // Original unchanged
        REQUIRE(ctx2.nChannel == 2);  // New context updated
        REQUIRE(ctx2.nHeight == ctx1.nHeight);  // Other fields preserved
    }
    
    SECTION("WithAuth creates new context with updated auth status")
    {
        MiningContext ctx1;
        MiningContext ctx2 = ctx1.WithAuth(true);
        
        REQUIRE(ctx1.fAuthenticated == false);  // Original unchanged
        REQUIRE(ctx2.fAuthenticated == true);   // New context updated
    }
    
    SECTION("Chained updates work correctly")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithHeight(12345)
            .WithAuth(true)
            .WithSession(42);
        
        REQUIRE(ctx.nChannel == 1);
        REQUIRE(ctx.nHeight == 12345);
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nSessionId == 42);
    }
    
    SECTION("WithKeyId and WithGenesis update identity fields")
    {
        uint256_t testKeyId;
        testKeyId.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx = MiningContext()
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis);
        
        REQUIRE(ctx.hashKeyID == testKeyId);
        REQUIRE(ctx.hashGenesis == testGenesis);
    }
}


TEST_CASE("ProcessResult Factory Tests", "[stateless_miner]")
{
    SECTION("Success creates valid result")
    {
        MiningContext ctx;
        Packet resp;
        
        ProcessResult result = ProcessResult::Success(ctx, resp);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.strError == "");
    }
    
    SECTION("Error creates failed result with message")
    {
        MiningContext ctx;
        
        ProcessResult result = ProcessResult::Error(ctx, "Test error");
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError == "Test error");
    }
}


TEST_CASE("StatelessMiner SET_CHANNEL Processing", "[stateless_miner]")
{
    SECTION("Single-byte channel payload - Channel 1")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x01);
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.nChannel == 1);
    }
    
    SECTION("Single-byte channel payload - Channel 2")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x02);
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.nChannel == 2);
    }
    
    SECTION("4-byte legacy channel payload - Channel 1")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA = {0x01, 0x00, 0x00, 0x00};
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.nChannel == 1);
    }
    
    SECTION("Invalid channel value rejected")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x00);  // Channel 0 is invalid
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("Invalid channel") != std::string::npos);
    }
    
    SECTION("Invalid payload size rejected")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA = {0x01, 0x00};  // 2 bytes - invalid
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("Invalid") != std::string::npos);
    }
}


TEST_CASE("StatelessMiner SESSION_START Processing", "[stateless_miner]")
{
    SECTION("SESSION_START requires authentication")
    {
        MiningContext ctx;  // Not authenticated
        
        Packet packet(SESSION_START);
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("Not authenticated") != std::string::npos);
    }
    
    SECTION("SESSION_START succeeds when authenticated")
    {
        MiningContext ctx = MiningContext().WithAuth(true);
        
        Packet packet(SESSION_START);
        packet.DATA.push_back(0x01);
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.fAuthenticated == true);
    }
}


TEST_CASE("FalconAuth Basic Operations", "[falcon_auth]")
{
    /* Initialize Falcon auth */
    FalconAuth::Initialize();
    
    SECTION("Generate key creates valid metadata")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);
        
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "test_key"
        );
        
        REQUIRE(meta.keyId != uint256_t(0));
        REQUIRE(!meta.pubkey.empty());
        REQUIRE(meta.profile == FalconAuth::Profile::FALCON_512);
        REQUIRE(meta.label == "test_key");
        REQUIRE(meta.created > 0);
    }
    
    SECTION("ListKeys returns generated keys")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);
        
        size_t nBefore = pAuth->ListKeys().size();
        
        pAuth->GenerateKey();
        
        size_t nAfter = pAuth->ListKeys().size();
        REQUIRE(nAfter == nBefore + 1);
    }
    
    SECTION("Sign and Verify work correctly")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);
        
        /* Generate key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey();
        
        /* Create test message */
        std::vector<uint8_t> vMessage = {0x01, 0x02, 0x03, 0x04};
        
        /* Sign */
        std::vector<uint8_t> vSignature = pAuth->Sign(meta.keyId, vMessage);
        REQUIRE(!vSignature.empty());
        
        /* Verify */
        FalconAuth::VerifyResult result = pAuth->Verify(meta.pubkey, vMessage, vSignature);
        REQUIRE(result.fValid == true);
        REQUIRE(result.keyId == meta.keyId);
    }
    
    SECTION("BindGenesis and GetBoundGenesis work correctly")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);
        
        /* Generate key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey();
        
        /* Create test genesis */
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        /* Bind genesis */
        bool fBound = pAuth->BindGenesis(meta.keyId, hashGenesis);
        REQUIRE(fBound == true);
        
        /* Retrieve bound genesis */
        std::optional<uint256_t> retrieved = pAuth->GetBoundGenesis(meta.keyId);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == hashGenesis);
    }
    
    SECTION("GetBoundGenesis returns nullopt for unbound key")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);
        
        /* Generate key without binding */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey();
        
        /* Should return nullopt */
        std::optional<uint256_t> retrieved = pAuth->GetBoundGenesis(meta.keyId);
        REQUIRE(!retrieved.has_value());
    }
    
    /* Cleanup */
    FalconAuth::Shutdown();
}
