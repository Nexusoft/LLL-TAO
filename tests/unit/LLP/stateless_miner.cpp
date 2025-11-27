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
#include <LLP/include/disposable_falcon.h>
#include <LLP/packets/packet.h>

#include <Util/include/runtime.h>

using namespace LLP;

/* Packet type definitions for testing - Phase 2 Unified Hybrid Protocol */
const Packet::message_t SET_CHANNEL = 3;
const Packet::message_t MINER_AUTH_INIT = 207;
const Packet::message_t MINER_AUTH_CHALLENGE = 208;
const Packet::message_t MINER_AUTH_RESPONSE = 209;
const Packet::message_t MINER_AUTH_RESULT = 210;
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
        REQUIRE(ctx.vAuthNonce.empty());
        REQUIRE(ctx.vMinerPubKey.empty());
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

    SECTION("WithNonce and WithPubKey update authentication fields")
    {
        std::vector<uint8_t> testNonce = {0x01, 0x02, 0x03, 0x04};
        std::vector<uint8_t> testPubKey = {0xAA, 0xBB, 0xCC, 0xDD};
        
        MiningContext ctx = MiningContext()
            .WithNonce(testNonce)
            .WithPubKey(testPubKey);
        
        REQUIRE(ctx.vAuthNonce == testNonce);
        REQUIRE(ctx.vMinerPubKey == testPubKey);
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


TEST_CASE("StatelessMiner MINER_AUTH_INIT Processing", "[stateless_miner]")
{
    SECTION("MINER_AUTH_INIT with valid pubkey generates challenge")
    {
        MiningContext ctx;
        
        Packet packet(MINER_AUTH_INIT);
        
        /* Build test packet: [2 bytes pubkey_len (big-endian)][pubkey][2 bytes id_len][id] */
        std::vector<uint8_t> testPubKey(897, 0x42);  // Simulated Falcon-512 pubkey size
        uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());
        
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), testPubKey.begin(), testPubKey.end());
        
        std::string testMinerId = "test_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), testMinerId.begin(), testMinerId.end());
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.vMinerPubKey == testPubKey);
        REQUIRE(result.context.vAuthNonce.size() == 32);  // 256-bit nonce
        REQUIRE(!result.response.IsNull());
        REQUIRE(result.response.HEADER == MINER_AUTH_CHALLENGE);
    }
    
    SECTION("MINER_AUTH_INIT with empty packet fails")
    {
        MiningContext ctx;
        
        Packet packet(MINER_AUTH_INIT);
        // Empty packet
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("packet too small") != std::string::npos);
    }
    
    SECTION("MINER_AUTH_INIT with invalid pubkey length fails")
    {
        MiningContext ctx;
        
        Packet packet(MINER_AUTH_INIT);
        /* Invalid pubkey_len = 0 */
        packet.DATA.push_back(0x00);
        packet.DATA.push_back(0x00);
        packet.DATA.push_back(0x00);  // id_len
        packet.DATA.push_back(0x00);
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("invalid pubkey_len") != std::string::npos);
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


TEST_CASE("MiningContext Authentication State Persistence", "[stateless_miner]")
{
    /* This test verifies that authentication state is preserved correctly
     * across mining context updates such as channel changes, height changes,
     * and timestamp updates. This is critical for SOLO mining where miners
     * must remain authenticated across block height changes and new rounds.
     */
    
    SECTION("Authentication state persists after channel change")
    {
        /* Start with an authenticated context */
        uint256_t testKeyId;
        testKeyId.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(12345)
            .WithKeyId(testKeyId);
        
        /* Verify authentication is set */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nSessionId == 12345);
        REQUIRE(ctx.hashKeyID == testKeyId);
        
        /* Change channel - simulating SET_CHANNEL packet processing */
        MiningContext newCtx = ctx.WithChannel(1);
        
        /* Authentication state must persist */
        REQUIRE(newCtx.fAuthenticated == true);
        REQUIRE(newCtx.nSessionId == 12345);
        REQUIRE(newCtx.hashKeyID == testKeyId);
        REQUIRE(newCtx.nChannel == 1);
    }
    
    SECTION("Authentication state persists after height change")
    {
        /* Start with an authenticated context */
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(99999)
            .WithHeight(1000);
        
        /* Verify authentication is set */
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.nSessionId == 99999);
        
        /* Change height - simulating new block received */
        MiningContext newCtx = ctx.WithHeight(1001);
        
        /* Authentication state must persist */
        REQUIRE(newCtx.fAuthenticated == true);
        REQUIRE(newCtx.nSessionId == 99999);
        REQUIRE(newCtx.nHeight == 1001);
    }
    
    SECTION("Authentication state persists after timestamp update")
    {
        /* Start with an authenticated context */
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(55555)
            .WithTimestamp(100000);
        
        /* Verify authentication is set */
        REQUIRE(ctx.fAuthenticated == true);
        
        /* Update timestamp - simulating keepalive */
        MiningContext newCtx = ctx.WithTimestamp(200000);
        
        /* Authentication state must persist */
        REQUIRE(newCtx.fAuthenticated == true);
        REQUIRE(newCtx.nSessionId == 55555);
        REQUIRE(newCtx.nTimestamp == 200000);
    }
    
    SECTION("Chained updates preserve all authentication state")
    {
        /* Start with a fully authenticated context */
        uint256_t testKeyId;
        testKeyId.SetHex("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
        
        uint256_t testGenesis;
        testGenesis.SetHex("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210");
        
        MiningContext ctx = MiningContext()
            .WithAuth(true)
            .WithSession(77777)
            .WithKeyId(testKeyId)
            .WithGenesis(testGenesis);
        
        /* Perform multiple chained updates simulating mining round activity */
        MiningContext finalCtx = ctx
            .WithChannel(2)
            .WithHeight(5000)
            .WithTimestamp(999999);
        
        /* All authentication state must persist through chained updates */
        REQUIRE(finalCtx.fAuthenticated == true);
        REQUIRE(finalCtx.nSessionId == 77777);
        REQUIRE(finalCtx.hashKeyID == testKeyId);
        REQUIRE(finalCtx.hashGenesis == testGenesis);
        REQUIRE(finalCtx.nChannel == 2);
        REQUIRE(finalCtx.nHeight == 5000);
        REQUIRE(finalCtx.nTimestamp == 999999);
    }
}


TEST_CASE("Packet Debug Functions", "[stateless_miner]")
{
    SECTION("DebugString returns formatted packet info")
    {
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x01);
        packet.LENGTH = 1;
        
        std::string debug = packet.DebugString();
        
        REQUIRE(debug.find("header=0x") != std::string::npos);
        REQUIRE(debug.find("length=1") != std::string::npos);
        REQUIRE(debug.find("data_size=1") != std::string::npos);
    }
    
    SECTION("DebugString includes data preview")
    {
        Packet packet(MINER_AUTH_INIT);
        packet.DATA = {0xAA, 0xBB, 0xCC, 0xDD};
        packet.LENGTH = 4;
        
        std::string debug = packet.DebugString(10);
        
        /* Should contain hex representation of data */
        REQUIRE(debug.find("data_preview=") != std::string::npos);
    }
    
    SECTION("DebugString truncates long data")
    {
        Packet packet(MINER_AUTH_INIT);
        packet.DATA.resize(100, 0x42);
        packet.LENGTH = 100;
        
        /* Request only 16 bytes in preview */
        std::string debug = packet.DebugString(16);
        
        /* Should contain ellipsis indicating truncation */
        REQUIRE(debug.find("...") != std::string::npos);
    }
    
    SECTION("GetBytesWithDebug serializes correctly")
    {
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x02);
        packet.LENGTH = 1;
        
        std::vector<uint8_t> bytes = packet.GetBytesWithDebug("test");
        
        /* Should have header (1) + length (4) + data (1) = 6 bytes */
        REQUIRE(bytes.size() == 6);
        REQUIRE(bytes[0] == SET_CHANNEL);  /* Header */
        REQUIRE(bytes[5] == 0x02);          /* Data */
    }
}


TEST_CASE("DisposableFalcon SignedWorkSubmission Tests", "[disposable_falcon]")
{
    SECTION("Default constructor creates empty submission")
    {
        DisposableFalcon::SignedWorkSubmission sub;
        
        REQUIRE(sub.hashMerkleRoot == uint512_t(0));
        REQUIRE(sub.nNonce == 0);
        REQUIRE(sub.vSignature.empty());
        REQUIRE(sub.fSigned == false);
    }
    
    SECTION("GetMessageBytes returns correct format")
    {
        uint512_t merkle;
        merkle.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        DisposableFalcon::SignedWorkSubmission sub(merkle, 0x123456789ABCDEF0ULL);
        
        std::vector<uint8_t> msg = sub.GetMessageBytes();
        
        /* Should have merkle (64) + nonce (8) + timestamp (8) = 80 bytes */
        REQUIRE(msg.size() == 80);
    }
    
    SECTION("Serialize and Deserialize roundtrip works")
    {
        uint512_t merkle;
        merkle.SetHex("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
        
        DisposableFalcon::SignedWorkSubmission original(merkle, 0xDEADBEEFCAFEBABEULL);
        original.vSignature = {0x01, 0x02, 0x03, 0x04, 0x05};
        original.fSigned = true;
        
        /* Serialize */
        std::vector<uint8_t> serialized = original.Serialize();
        
        /* Deserialize into new submission */
        DisposableFalcon::SignedWorkSubmission restored;
        bool result = restored.Deserialize(serialized);
        
        REQUIRE(result == true);
        REQUIRE(restored.hashMerkleRoot == original.hashMerkleRoot);
        REQUIRE(restored.nNonce == original.nNonce);
        REQUIRE(restored.vSignature == original.vSignature);
        REQUIRE(restored.fSigned == true);
    }
    
    SECTION("Deserialize fails on too-small data")
    {
        std::vector<uint8_t> tooSmall = {0x01, 0x02, 0x03};
        
        DisposableFalcon::SignedWorkSubmission sub;
        bool result = sub.Deserialize(tooSmall);
        
        REQUIRE(result == false);
    }
    
    SECTION("DebugString returns formatted info")
    {
        uint512_t merkle;
        merkle.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        DisposableFalcon::SignedWorkSubmission sub(merkle, 12345);
        
        std::string debug = sub.DebugString();
        
        REQUIRE(debug.find("SignedWorkSubmission") != std::string::npos);
        REQUIRE(debug.find("nonce=12345") != std::string::npos);
    }
}


TEST_CASE("DisposableFalcon WrapperResult Tests", "[disposable_falcon]")
{
    SECTION("Success creates valid result")
    {
        DisposableFalcon::SignedWorkSubmission sub;
        
        DisposableFalcon::WrapperResult result = 
            DisposableFalcon::WrapperResult::Success(sub);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.strError.empty());
    }
    
    SECTION("Failure creates failed result with message")
    {
        DisposableFalcon::WrapperResult result = 
            DisposableFalcon::WrapperResult::Failure("Test error message");
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError == "Test error message");
    }
}


TEST_CASE("DisposableFalcon Wrapper Basic Operations", "[disposable_falcon]")
{
    SECTION("Create returns valid wrapper instance")
    {
        auto pWrapper = DisposableFalcon::Create();
        
        REQUIRE(pWrapper != nullptr);
        REQUIRE(pWrapper->HasActiveSession() == false);
    }
    
    SECTION("GenerateSessionKey creates active session")
    {
        auto pWrapper = DisposableFalcon::Create();
        uint256_t sessionId;
        sessionId.SetHex("deadbeefcafebabe0123456789abcdef0123456789abcdef0123456789abcdef");
        
        bool result = pWrapper->GenerateSessionKey(sessionId);
        
        REQUIRE(result == true);
        REQUIRE(pWrapper->HasActiveSession() == true);
        REQUIRE(pWrapper->GetSessionKeyId() != uint256_t(0));
        REQUIRE(!pWrapper->GetSessionPubKey().empty());
    }
    
    SECTION("ClearSession removes active session")
    {
        auto pWrapper = DisposableFalcon::Create();
        uint256_t sessionId;
        sessionId.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        pWrapper->GenerateSessionKey(sessionId);
        REQUIRE(pWrapper->HasActiveSession() == true);
        
        pWrapper->ClearSession();
        
        REQUIRE(pWrapper->HasActiveSession() == false);
        REQUIRE(pWrapper->GetSessionKeyId() == uint256_t(0));
        REQUIRE(pWrapper->GetSessionPubKey().empty());
    }
    
    SECTION("WrapWorkSubmission fails without active session")
    {
        auto pWrapper = DisposableFalcon::Create();
        uint512_t merkle;
        merkle.SetHex("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
        
        DisposableFalcon::WrapperResult result = 
            pWrapper->WrapWorkSubmission(merkle, 12345);
        
        REQUIRE(result.fSuccess == false);
        REQUIRE(result.strError.find("No active session") != std::string::npos);
    }
    
    SECTION("WrapWorkSubmission succeeds with active session")
    {
        auto pWrapper = DisposableFalcon::Create();
        uint256_t sessionId;
        sessionId.SetHex("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210");
        
        pWrapper->GenerateSessionKey(sessionId);
        
        uint512_t merkle;
        merkle.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        DisposableFalcon::WrapperResult result = 
            pWrapper->WrapWorkSubmission(merkle, 0xCAFEBABEDEADBEEFULL);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.submission.fSigned == true);
        REQUIRE(!result.submission.vSignature.empty());
        REQUIRE(result.submission.hashMerkleRoot == merkle);
        REQUIRE(result.submission.nNonce == 0xCAFEBABEDEADBEEFULL);
    }
    
    SECTION("Wrap and Unwrap roundtrip works")
    {
        auto pWrapper = DisposableFalcon::Create();
        uint256_t sessionId;
        sessionId.SetHex("1111222233334444555566667777888899990000aaaabbbbccccddddeeeeffff");
        
        pWrapper->GenerateSessionKey(sessionId);
        std::vector<uint8_t> vPubKey = pWrapper->GetSessionPubKey();
        
        uint512_t merkle;
        merkle.SetHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0000");
        
        /* Wrap */
        DisposableFalcon::WrapperResult wrapResult = 
            pWrapper->WrapWorkSubmission(merkle, 9999);
        REQUIRE(wrapResult.fSuccess == true);
        
        /* Serialize */
        std::vector<uint8_t> serialized = wrapResult.submission.Serialize();
        
        /* Unwrap (using the public key from the session) */
        DisposableFalcon::WrapperResult unwrapResult = 
            pWrapper->UnwrapWorkSubmission(serialized, vPubKey);
        
        REQUIRE(unwrapResult.fSuccess == true);
        REQUIRE(unwrapResult.submission.hashMerkleRoot == merkle);
        REQUIRE(unwrapResult.submission.nNonce == 9999);
    }
}
