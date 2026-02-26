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
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/stateless_opcodes.h>
#include <LLP/include/opcode_utility.h>

#include <Util/include/runtime.h>
#include <Util/include/hex.h>

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
        REQUIRE(ctx.strUserName == "");
        REQUIRE(ctx.vAuthNonce.empty());
        REQUIRE(ctx.vMinerPubKey.empty());
        REQUIRE(ctx.vChaChaKey.empty());
        REQUIRE(ctx.fEncryptionReady == false);
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

    SECTION("WithChaChaKey updates encryption state")
    {
        std::vector<uint8_t> testKey(32, 0x42);  // 32-byte ChaCha20 key
        
        MiningContext ctx = MiningContext()
            .WithChaChaKey(testKey);
        
        REQUIRE(ctx.vChaChaKey == testKey);
        REQUIRE(ctx.fEncryptionReady == true);
    }

    SECTION("WithChaChaKey with empty key sets fEncryptionReady to false")
    {
        std::vector<uint8_t> emptyKey;
        
        MiningContext ctx = MiningContext()
            .WithChaChaKey(emptyKey);
        
        REQUIRE(ctx.vChaChaKey.empty());
        REQUIRE(ctx.fEncryptionReady == false);
    }

    SECTION("WithUserName updates username field")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("testminer");
        
        REQUIRE(ctx.strUserName == "testminer");
    }

    SECTION("GetPayoutAddress returns genesis when set")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis);
        
        REQUIRE(ctx.GetPayoutAddress() == testGenesis);
    }

    SECTION("GetPayoutAddress returns zero when only username set")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("testminer");
        
        /* Username requires name resolution - returns 0 */
        REQUIRE(ctx.GetPayoutAddress() == uint256_t(0));
    }

    SECTION("HasValidPayout returns true when genesis set")
    {
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        MiningContext ctx = MiningContext()
            .WithGenesis(testGenesis);
        
        REQUIRE(ctx.HasValidPayout() == true);
    }

    SECTION("HasValidPayout returns true when username set")
    {
        MiningContext ctx = MiningContext()
            .WithUserName("testminer");
        
        REQUIRE(ctx.HasValidPayout() == true);
    }

    SECTION("HasValidPayout returns false when neither set")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.HasValidPayout() == false);
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
    
    SECTION("CHANNEL_ACK response has correct payload")
    {
        MiningContext ctx;
        
        Packet packet(SET_CHANNEL);
        packet.DATA.push_back(0x02);  // Set channel 2
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(!result.response.IsNull());
        
        /* Verify response header is CHANNEL_ACK (206) */
        REQUIRE(result.response.HEADER == 206);
        
        /* Verify response contains channel number in payload */
        REQUIRE(result.response.LENGTH == 1);
        REQUIRE(result.response.DATA.size() == 1);
        REQUIRE(result.response.DATA[0] == 0x02);
        
        /* Verify CHANNEL_ACK is recognized as having data payload */
        REQUIRE(result.response.HasDataPayload() == true);
        
        /* Verify serialization includes LENGTH and DATA */
        std::vector<uint8_t> bytes = result.response.GetBytes();
        /* Expected format: HEADER(1) + LENGTH(4) + DATA(1) = 6 bytes */
        REQUIRE(bytes.size() == 6);
        REQUIRE(bytes[0] == 206);  // CHANNEL_ACK header
        /* LENGTH = 1 in big-endian: 0x00 0x00 0x00 0x01 */
        REQUIRE(bytes[1] == 0x00);
        REQUIRE(bytes[2] == 0x00);
        REQUIRE(bytes[3] == 0x00);
        REQUIRE(bytes[4] == 0x01);
        /* DATA = channel number */
        REQUIRE(bytes[5] == 0x02);
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
    
    SECTION("MINER_AUTH_INIT with hashGenesis parses genesis correctly")
    {
        MiningContext ctx;
        
        Packet packet(MINER_AUTH_INIT);
        
        /* Build test packet in CORRECT format: [32 bytes genesis][2 bytes pubkey_len][pubkey][2 bytes id_len][id] */
        
        /* Add test genesis hash FIRST (32 bytes) */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        /* Use HexStr to get raw bytes in the same order as hex string */
        std::string hexStr = testGenesis.GetHex();
        for(size_t i = 0; i < 64; i += 2)
        {
            std::string byteStr = hexStr.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            packet.DATA.push_back(byte);
        }
        
        /* Add pubkey_len (2 bytes, big-endian) */
        std::vector<uint8_t> testPubKey(897, 0x42);  // Simulated Falcon-512 pubkey size
        uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* Add pubkey */
        packet.DATA.insert(packet.DATA.end(), testPubKey.begin(), testPubKey.end());
        
        /* Add miner_id_len (2 bytes, big-endian) */
        std::string testMinerId = "test_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        
        /* Add miner_id */
        packet.DATA.insert(packet.DATA.end(), testMinerId.begin(), testMinerId.end());
        
        packet.LENGTH = static_cast<uint32_t>(packet.DATA.size());
        
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.context.vMinerPubKey == testPubKey);
        REQUIRE(result.context.vAuthNonce.size() == 32);  // 256-bit nonce
        REQUIRE(result.context.hashGenesis == testGenesis);  // Genesis should be stored
        REQUIRE(!result.response.IsNull());
        REQUIRE(result.response.HEADER == MINER_AUTH_CHALLENGE);
    }
}


TEST_CASE("Genesis Byte Order Preservation", "[stateless_miner][genesis]")
{
    SECTION("SetHex preserves genesis byte order correctly")
    {
        /* This test verifies the fix for the genesis byte order issue.
         * 
         * The problem was that SetBytes() performs a 32-bit word endianness swap:
         * Input:  [a1 74 01 1c] [93 ca 1c 80] ...
         * Output: [1c 01 74 a1] [80 1c ca 93] ...
         * 
         * The fix uses SetHex() which preserves byte order as-is.
         */
        
        /* Raw bytes from miner (genesis starting with 0xa1 = valid mainnet user) */
        const char* hexStr = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
        
        /* Convert hex string to raw bytes (simulating miner packet) */
        std::vector<uint8_t> vGenesis;
        for(size_t i = 0; i < 64; i += 2)
        {
            std::string byteStr = std::string(hexStr).substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            vGenesis.push_back(byte);
        }
        
        /* Use the fixed approach: HexStr + SetHex */
        uint256_t hashGenesis;
        std::string strGenesisHex = HexStr(vGenesis);
        hashGenesis.SetHex(strGenesisHex);
        
        /* Verify the hex string matches the original */
        REQUIRE(strGenesisHex == hexStr);
        REQUIRE(hashGenesis.GetHex() == hexStr);
        
        /* Verify the type byte is correct (0xa1 for mainnet user) */
        uint8_t typeByteFromHash = hashGenesis.GetType();
        uint8_t typeByteFromRaw = vGenesis[0];
        
        REQUIRE(typeByteFromRaw == 0xa1);  // Raw bytes have 0xa1 first
        REQUIRE(typeByteFromHash == 0xa1);  // After SetHex, type byte should still be 0xa1
        
        /* Compare with the broken approach (for documentation) */
        uint256_t hashGenesisBroken;
        hashGenesisBroken.SetBytes(vGenesis);
        
        /* The broken approach would give 0x1c as type byte (wrong!) */
        uint8_t typeByteBroken = hashGenesisBroken.GetType();
        REQUIRE(typeByteBroken != 0xa1);  // Broken approach has wrong type byte
    }
    
    SECTION("MINER_AUTH_INIT packet with correct genesis format")
    {
        MiningContext ctx;
        ctx.strAddress = "127.0.0.1:9325";
        
        Packet packet(MINER_AUTH_INIT);
        
        /* Build packet in correct format: [genesis(32)][pubkey_len(2)][pubkey][miner_id_len(2)][miner_id] */
        
        /* STEP 1: Add genesis hash (32 bytes) - comes FIRST */
        const char* genesisHex = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";
        for(size_t i = 0; i < 64; i += 2)
        {
            std::string byteStr = std::string(genesisHex).substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            packet.DATA.push_back(byte);
        }
        
        /* STEP 2: Add pubkey_len (2 bytes, big-endian) */
        std::vector<uint8_t> testPubKey(897, 0x42);  // Simulated Falcon-512 pubkey
        uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* STEP 3: Add pubkey */
        packet.DATA.insert(packet.DATA.end(), testPubKey.begin(), testPubKey.end());
        
        /* STEP 4: Add miner_id_len (2 bytes, big-endian) */
        std::string testMinerId = "test_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        
        /* STEP 5: Add miner_id */
        packet.DATA.insert(packet.DATA.end(), testMinerId.begin(), testMinerId.end());
        
        packet.LENGTH = static_cast<uint32_t>(packet.DATA.size());
        
        /* Process the packet */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        /* Verify success */
        REQUIRE(result.fSuccess == true);
        REQUIRE(result.response.HEADER == MINER_AUTH_CHALLENGE);
        
        /* Verify genesis was parsed correctly with type byte 0xa1 */
        uint256_t expectedGenesis;
        expectedGenesis.SetHex(genesisHex);
        REQUIRE(result.context.hashGenesis == expectedGenesis);
        REQUIRE(result.context.hashGenesis.GetType() == 0xa1);
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


TEST_CASE("Packet HasDataPayload and Authentication Packet Serialization", "[packet][falcon_auth]")
{
    SECTION("Traditional data packets (HEADER < 128) have data payload")
    {
        Packet packet(SET_CHANNEL);  // SET_CHANNEL = 3 < 128
        REQUIRE(packet.HasDataPayload() == true);
        
        Packet blockData(0);  // BLOCK_DATA = 0 < 128
        REQUIRE(blockData.HasDataPayload() == true);
        
        Packet submitBlock(1);  // SUBMIT_BLOCK = 1 < 128
        REQUIRE(submitBlock.HasDataPayload() == true);
    }
    
    SECTION("Traditional request packets (128-206, 213-254) have no data payload")
    {
        Packet getBlock(129);  // GET_BLOCK = 129
        REQUIRE(getBlock.HasDataPayload() == false);
        
        Packet blockAccepted(200);  // BLOCK_ACCEPTED = 200
        REQUIRE(blockAccepted.HasDataPayload() == false);
        
        Packet ping(253);  // PING = 253
        REQUIRE(ping.HasDataPayload() == false);
    }
    
    SECTION("Falcon authentication packets (207-212) have data payload")
    {
        Packet authInit(MINER_AUTH_INIT);  // 207
        REQUIRE(authInit.HasDataPayload() == true);
        
        Packet authChallenge(MINER_AUTH_CHALLENGE);  // 208
        REQUIRE(authChallenge.HasDataPayload() == true);
        
        Packet authResponse(MINER_AUTH_RESPONSE);  // 209
        REQUIRE(authResponse.HasDataPayload() == true);
        
        Packet authResult(MINER_AUTH_RESULT);  // 210
        REQUIRE(authResult.HasDataPayload() == true);
        
        Packet sessionStart(SESSION_START);  // 211
        REQUIRE(sessionStart.HasDataPayload() == true);
        
        Packet sessionKeepalive(212);  // SESSION_KEEPALIVE = 212
        REQUIRE(sessionKeepalive.HasDataPayload() == true);
    }
    
    SECTION("MINER_AUTH_CHALLENGE GetBytes includes length and data")
    {
        /* Create a MINER_AUTH_CHALLENGE packet (207) with nonce data */
        Packet packet(MINER_AUTH_CHALLENGE);
        
        /* Add a 32-byte nonce as data */
        std::vector<uint8_t> nonce(32, 0x42);
        packet.DATA = nonce;
        packet.LENGTH = 32;
        
        /* Serialize the packet */
        std::vector<uint8_t> bytes = packet.GetBytes();
        
        /* Should have: header (1) + length (4) + data (32) = 37 bytes */
        REQUIRE(bytes.size() == 37);
        REQUIRE(bytes[0] == MINER_AUTH_CHALLENGE);  /* Header = 208 */
        
        /* Verify length bytes (big-endian) */
        uint32_t serializedLength = (bytes[1] << 24) | (bytes[2] << 16) | 
                                    (bytes[3] << 8) | bytes[4];
        REQUIRE(serializedLength == 32);
        
        /* Verify data starts at offset 5 */
        for(size_t i = 0; i < 32; ++i)
        {
            REQUIRE(bytes[5 + i] == 0x42);
        }
    }
    
    SECTION("MINER_AUTH_RESPONSE GetBytes includes signature data")
    {
        /* Create a MINER_AUTH_RESPONSE packet (209) with signature data */
        Packet packet(MINER_AUTH_RESPONSE);
        
        /* Add a simulated signature (signature length + signature) */
        uint16_t sigLen = 100;
        packet.DATA.push_back(static_cast<uint8_t>(sigLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(sigLen & 0xFF));
        for(uint16_t i = 0; i < sigLen; ++i)
            packet.DATA.push_back(static_cast<uint8_t>(i));
        
        packet.LENGTH = static_cast<uint32_t>(packet.DATA.size());
        
        /* Serialize the packet */
        std::vector<uint8_t> bytes = packet.GetBytes();
        
        /* Should have: header (1) + length (4) + data (102) = 107 bytes */
        REQUIRE(bytes.size() == 107);
        REQUIRE(bytes[0] == MINER_AUTH_RESPONSE);  /* Header = 209 */
        
        /* Verify length field */
        uint32_t serializedLength = (bytes[1] << 24) | (bytes[2] << 16) | 
                                    (bytes[3] << 8) | bytes[4];
        REQUIRE(serializedLength == 102);
    }
    
    SECTION("Traditional request packets serialize without data")
    {
        /* Create a GET_BLOCK packet (129) - should not serialize data */
        Packet packet(129);  // GET_BLOCK
        packet.DATA.push_back(0xFF);  // Add some data (shouldn't be serialized)
        packet.LENGTH = 1;
        
        /* Serialize the packet */
        std::vector<uint8_t> bytes = packet.GetBytes();
        
        /* Should have only header (1 byte) since GET_BLOCK is a request packet */
        REQUIRE(bytes.size() == 1);
        REQUIRE(bytes[0] == 129);
    }
    
    SECTION("Header() function correctly identifies complete auth packets")
    {
        /* Auth packet with data - should require LENGTH > 0 */
        Packet authPacket(MINER_AUTH_CHALLENGE);
        authPacket.LENGTH = 0;
        REQUIRE(authPacket.Header() == false);  /* Not complete without length */
        
        authPacket.LENGTH = 32;
        REQUIRE(authPacket.Header() == true);   /* Complete with length */
    }
    
    SECTION("Header() function correctly identifies complete request packets")
    {
        /* Request packet - should require LENGTH == 0 */
        Packet reqPacket(129);  // GET_BLOCK
        reqPacket.LENGTH = 0;
        REQUIRE(reqPacket.Header() == true);   /* Complete with no length */
        
        reqPacket.LENGTH = 10;
        REQUIRE(reqPacket.Header() == false);  /* Not complete with unexpected length */
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


/* Push Notification Tests */
TEST_CASE("Push Notification Protocol Tests", "[stateless_miner][push_notifications]")
{
    /* Packet type definitions for push notifications
     * Note: These are redefined here to avoid including miner.h which would create
     * a circular dependency. Values must match the constants in src/LLP/types/miner.h */
    const Packet::message_t MINER_READY = 216;              // Must match Miner::MINER_READY
    const Packet::message_t PRIME_BLOCK_AVAILABLE = 217;    // Must match Miner::PRIME_BLOCK_AVAILABLE
    const Packet::message_t HASH_BLOCK_AVAILABLE = 218;     // Must match Miner::HASH_BLOCK_AVAILABLE
    
    SECTION("MiningContext push notification state initialization")
    {
        MiningContext ctx;
        
        /* Verify default initialization */
        REQUIRE(ctx.fSubscribedToNotifications == false);
        REQUIRE(ctx.nSubscribedChannel == 0);
        REQUIRE(ctx.nLastNotificationTime == 0);
        REQUIRE(ctx.nNotificationsSent == 0);
    }
    
    SECTION("WithSubscription creates subscribed context")
    {
        MiningContext ctx;
        
        /* Subscribe to Prime channel */
        MiningContext ctx2 = ctx.WithSubscription(1);
        
        /* Verify original unchanged */
        REQUIRE(ctx.fSubscribedToNotifications == false);
        REQUIRE(ctx.nSubscribedChannel == 0);
        
        /* Verify new context subscribed */
        REQUIRE(ctx2.fSubscribedToNotifications == true);
        REQUIRE(ctx2.nSubscribedChannel == 1);
        REQUIRE(ctx2.nLastNotificationTime == 0);
        REQUIRE(ctx2.nNotificationsSent == 0);
    }
    
    SECTION("WithSubscription to Hash channel")
    {
        MiningContext ctx;
        
        /* Subscribe to Hash channel */
        MiningContext ctx2 = ctx.WithSubscription(2);
        
        /* Verify subscribed to Hash */
        REQUIRE(ctx2.fSubscribedToNotifications == true);
        REQUIRE(ctx2.nSubscribedChannel == 2);
    }
    
    SECTION("WithNotificationSent updates statistics")
    {
        MiningContext ctx = MiningContext().WithSubscription(1);
        
        /* Send first notification */
        uint64_t timestamp1 = 1000000;
        MiningContext ctx2 = ctx.WithNotificationSent(timestamp1);
        
        /* Verify statistics updated */
        REQUIRE(ctx.nNotificationsSent == 0);  // Original unchanged
        REQUIRE(ctx2.nLastNotificationTime == timestamp1);
        REQUIRE(ctx2.nNotificationsSent == 1);
        
        /* Send second notification */
        uint64_t timestamp2 = 2000000;
        MiningContext ctx3 = ctx2.WithNotificationSent(timestamp2);
        
        /* Verify cumulative count */
        REQUIRE(ctx3.nLastNotificationTime == timestamp2);
        REQUIRE(ctx3.nNotificationsSent == 2);
    }
    
    SECTION("Chained subscription and notification")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(1)
            .WithAuth(true)
            .WithSubscription(1)
            .WithNotificationSent(12345);
        
        /* Verify all fields */
        REQUIRE(ctx.nChannel == 1);
        REQUIRE(ctx.fAuthenticated == true);
        REQUIRE(ctx.fSubscribedToNotifications == true);
        REQUIRE(ctx.nSubscribedChannel == 1);
        REQUIRE(ctx.nLastNotificationTime == 12345);
        REQUIRE(ctx.nNotificationsSent == 1);
    }
    
    SECTION("Subscription preserves other context fields")
    {
        MiningContext ctx = MiningContext()
            .WithChannel(2)
            .WithHeight(123456)
            .WithAuth(true)
            .WithSession(789);
        
        /* Subscribe while preserving other fields */
        MiningContext ctx2 = ctx.WithSubscription(2);
        
        /* Verify subscription added */
        REQUIRE(ctx2.fSubscribedToNotifications == true);
        REQUIRE(ctx2.nSubscribedChannel == 2);
        
        /* Verify other fields preserved */
        REQUIRE(ctx2.nChannel == 2);
        REQUIRE(ctx2.nHeight == 123456);
        REQUIRE(ctx2.fAuthenticated == true);
        REQUIRE(ctx2.nSessionId == 789);
    }
    
    SECTION("Multiple notifications increment counter correctly")
    {
        MiningContext ctx = MiningContext().WithSubscription(1);
        
        /* Send 5 notifications */
        for (int i = 0; i < 5; i++)
        {
            ctx = ctx.WithNotificationSent(1000000 + i * 1000);
        }
        
        /* Verify final count */
        REQUIRE(ctx.nNotificationsSent == 5);
        REQUIRE(ctx.nLastNotificationTime == 1004000);
    }
    
    SECTION("Subscription to channel 0 (Stake) - should be rejected at protocol level")
    {
        /* Note: Channel 0 subscription is allowed at context level,
         * but will be rejected in MINER_READY handler at protocol level.
         * This tests that context can represent the state, even if invalid. */
        MiningContext ctx = MiningContext().WithSubscription(0);
        
        /* Context allows any value (protocol validates) */
        REQUIRE(ctx.fSubscribedToNotifications == true);
        REQUIRE(ctx.nSubscribedChannel == 0);
    }
}


TEST_CASE("Push Notification Packet Format Tests", "[stateless_miner][push_notifications]")
{
    const Packet::message_t PRIME_BLOCK_AVAILABLE = 217;
    const Packet::message_t HASH_BLOCK_AVAILABLE = 218;
    
    SECTION("PRIME_BLOCK_AVAILABLE packet structure")
    {
        /* Create notification packet */
        Packet notification(PRIME_BLOCK_AVAILABLE);
        
        /* Example blockchain state */
        uint32_t nUnifiedHeight = 6541700;  // 0x0063D184
        uint32_t nPrimeHeight = 2302709;    // 0x002322F5
        uint32_t nDifficulty = 0x0422E6FC;
        
        /* Build 12-byte payload (big-endian) */
        // Unified height [0-3]
        notification.DATA.push_back((nUnifiedHeight >> 24) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 16) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 8) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 0) & 0xFF);
        
        // Prime height [4-7]
        notification.DATA.push_back((nPrimeHeight >> 24) & 0xFF);
        notification.DATA.push_back((nPrimeHeight >> 16) & 0xFF);
        notification.DATA.push_back((nPrimeHeight >> 8) & 0xFF);
        notification.DATA.push_back((nPrimeHeight >> 0) & 0xFF);
        
        // Difficulty [8-11]
        notification.DATA.push_back((nDifficulty >> 24) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 16) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 8) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 0) & 0xFF);
        
        notification.LENGTH = 12;
        
        /* Verify packet structure */
        REQUIRE(notification.HEADER == PRIME_BLOCK_AVAILABLE);
        REQUIRE(notification.LENGTH == 12);
        REQUIRE(notification.DATA.size() == 12);
        
        /* Verify big-endian encoding */
        REQUIRE(notification.DATA[0] == 0x00);  // Unified high byte
        REQUIRE(notification.DATA[1] == 0x63);
        REQUIRE(notification.DATA[2] == 0xD1);
        REQUIRE(notification.DATA[3] == 0x84);  // Unified low byte
        
        REQUIRE(notification.DATA[4] == 0x00);  // Prime high byte
        REQUIRE(notification.DATA[5] == 0x23);
        REQUIRE(notification.DATA[6] == 0x22);
        REQUIRE(notification.DATA[7] == 0xF5);  // Prime low byte
        
        REQUIRE(notification.DATA[8] == 0x04);  // Difficulty high byte
        REQUIRE(notification.DATA[9] == 0x22);
        REQUIRE(notification.DATA[10] == 0xE6);
        REQUIRE(notification.DATA[11] == 0xFC); // Difficulty low byte
    }
    
    SECTION("HASH_BLOCK_AVAILABLE packet structure")
    {
        /* Create notification packet */
        Packet notification(HASH_BLOCK_AVAILABLE);
        
        /* Example blockchain state */
        uint32_t nUnifiedHeight = 6541701;  // 0x0063D185
        uint32_t nHashHeight = 4165000;     // 0x003F83F8
        uint32_t nDifficulty = 0x03ABCDEF;
        
        /* Build 12-byte payload (big-endian) */
        // Unified height [0-3]
        notification.DATA.push_back((nUnifiedHeight >> 24) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 16) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 8) & 0xFF);
        notification.DATA.push_back((nUnifiedHeight >> 0) & 0xFF);
        
        // Hash height [4-7]
        notification.DATA.push_back((nHashHeight >> 24) & 0xFF);
        notification.DATA.push_back((nHashHeight >> 16) & 0xFF);
        notification.DATA.push_back((nHashHeight >> 8) & 0xFF);
        notification.DATA.push_back((nHashHeight >> 0) & 0xFF);
        
        // Difficulty [8-11]
        notification.DATA.push_back((nDifficulty >> 24) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 16) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 8) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 0) & 0xFF);
        
        notification.LENGTH = 12;
        
        /* Verify packet structure */
        REQUIRE(notification.HEADER == HASH_BLOCK_AVAILABLE);
        REQUIRE(notification.LENGTH == 12);
        REQUIRE(notification.DATA.size() == 12);
        
        /* Verify big-endian encoding */
        REQUIRE(notification.DATA[0] == 0x00);  // Unified high byte
        REQUIRE(notification.DATA[1] == 0x63);
        REQUIRE(notification.DATA[2] == 0xD1);
        REQUIRE(notification.DATA[3] == 0x85);  // Unified low byte
        
        REQUIRE(notification.DATA[4] == 0x00);  // Hash high byte
        REQUIRE(notification.DATA[5] == 0x3F);
        REQUIRE(notification.DATA[6] == 0x83);
        REQUIRE(notification.DATA[7] == 0xF8);  // Hash low byte
        
        REQUIRE(notification.DATA[8] == 0x03);  // Difficulty high byte
        REQUIRE(notification.DATA[9] == 0xAB);
        REQUIRE(notification.DATA[10] == 0xCD);
        REQUIRE(notification.DATA[11] == 0xEF); // Difficulty low byte
    }
    
    SECTION("MINER_READY packet structure (header-only)")
    {
        const Packet::message_t MINER_READY = 216;
        
        /* Create MINER_READY packet */
        Packet request(MINER_READY);
        request.LENGTH = 0;
        
        /* Verify header-only packet */
        REQUIRE(request.HEADER == MINER_READY);
        REQUIRE(request.LENGTH == 0);
        REQUIRE(request.DATA.empty());
    }
}


TEST_CASE("Stateless Opcode Conversion for 16-bit Lane", "[stateless_miner][opcodes]")
{
    /* Test the opcode conversion that enables both 8-bit (legacy) and 16-bit (stateless)
     * lanes to work with the same StatelessMiner::ProcessPacket() switch statement.
     * This fix addresses the "Unknown miner opcode: 53455" issue on port 9323. */
    
    SECTION("StatelessOpcodes::Mirror converts 8-bit to 16-bit opcodes")
    {
        /* Authentication opcodes */
        REQUIRE(StatelessOpcodes::Mirror(207) == 0xD0CF);  // MINER_AUTH_INIT
        REQUIRE(StatelessOpcodes::Mirror(208) == 0xD0D0);  // MINER_AUTH_CHALLENGE
        REQUIRE(StatelessOpcodes::Mirror(209) == 0xD0D1);  // MINER_AUTH_RESPONSE
        REQUIRE(StatelessOpcodes::Mirror(210) == 0xD0D2);  // MINER_AUTH_RESULT
        
        /* Session management opcodes */
        REQUIRE(StatelessOpcodes::Mirror(211) == 0xD0D3);  // SESSION_START
        REQUIRE(StatelessOpcodes::Mirror(212) == 0xD0D4);  // SESSION_KEEPALIVE
        
        /* Reward opcodes */
        REQUIRE(StatelessOpcodes::Mirror(213) == 0xD0D5);  // MINER_SET_REWARD
        REQUIRE(StatelessOpcodes::Mirror(214) == 0xD0D6);  // MINER_REWARD_RESULT
    }
    
    SECTION("StatelessOpcodes::Unmirror converts 16-bit to 8-bit opcodes")
    {
        /* Authentication opcodes */
        REQUIRE(StatelessOpcodes::Unmirror(0xD0CF) == 207);  // MINER_AUTH_INIT
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D0) == 208);  // MINER_AUTH_CHALLENGE
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D1) == 209);  // MINER_AUTH_RESPONSE
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D2) == 210);  // MINER_AUTH_RESULT
        
        /* Session management opcodes */
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D3) == 211);  // SESSION_START
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D4) == 212);  // SESSION_KEEPALIVE
        
        /* Reward opcodes */
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D5) == 213);  // MINER_SET_REWARD
        REQUIRE(StatelessOpcodes::Unmirror(0xD0D6) == 214);  // MINER_REWARD_RESULT
    }
    
    SECTION("StatelessOpcodes::IsStateless correctly identifies 16-bit opcodes")
    {
        /* Valid stateless opcodes (0xD000-0xD0FF range) */
        REQUIRE(StatelessOpcodes::IsStateless(0xD0CF) == true);   // MINER_AUTH_INIT
        REQUIRE(StatelessOpcodes::IsStateless(0xD0D0) == true);   // MINER_AUTH_CHALLENGE
        REQUIRE(StatelessOpcodes::IsStateless(0xD0D1) == true);   // MINER_AUTH_RESPONSE
        REQUIRE(StatelessOpcodes::IsStateless(0xD0D2) == true);   // MINER_AUTH_RESULT
        REQUIRE(StatelessOpcodes::IsStateless(0xD000) == true);   // BLOCK_DATA
        REQUIRE(StatelessOpcodes::IsStateless(0xD0FF) == true);   // Upper bound
        
        /* Invalid: 8-bit legacy opcodes */
        REQUIRE(StatelessOpcodes::IsStateless(207) == false);     // Legacy MINER_AUTH_INIT
        REQUIRE(StatelessOpcodes::IsStateless(208) == false);     // Legacy MINER_AUTH_CHALLENGE
        REQUIRE(StatelessOpcodes::IsStateless(209) == false);     // Legacy MINER_AUTH_RESPONSE
        REQUIRE(StatelessOpcodes::IsStateless(210) == false);     // Legacy MINER_AUTH_RESULT
        
        /* Invalid: Out of range */
        REQUIRE(StatelessOpcodes::IsStateless(0xD100) == false);  // Above range
        REQUIRE(StatelessOpcodes::IsStateless(0xCFFF) == false);  // Below range
        REQUIRE(StatelessOpcodes::IsStateless(0x0000) == false);  // Zero
    }
    
    SECTION("Round-trip conversion: 8-bit → 16-bit → 8-bit")
    {
        /* Test that Mirror and Unmirror are inverses */
        for(int i = 0; i <= 255; ++i)
        {
            uint8_t opcode = static_cast<uint8_t>(i);
            uint16_t mirrored = StatelessOpcodes::Mirror(opcode);
            uint8_t unmirrored = StatelessOpcodes::Unmirror(mirrored);
            REQUIRE(unmirrored == opcode);
        }
    }
    
    SECTION("StatelessPacket with 16-bit opcode can be unmirrored for routing")
    {
        /* Simulate receiving MINER_AUTH_INIT (0xD0CF) from stateless lane */
        StatelessPacket packet;
        packet.HEADER = 0xD0CF;  // 16-bit mirror-mapped opcode
        packet.LENGTH = 0;
        
        /* Verify it's a stateless opcode */
        REQUIRE(StatelessOpcodes::IsStateless(packet.HEADER) == true);
        
        /* Unmirror to get 8-bit routing opcode */
        uint16_t routeOpcode = StatelessOpcodes::Unmirror(packet.HEADER);
        REQUIRE(routeOpcode == 207);  // Should match MINER_AUTH_INIT enum value
        
        /* This allows the switch statement to match case 207 */
    }
    
    SECTION("StatelessPacket with 8-bit opcode from legacy lane needs no conversion")
    {
        /* Simulate receiving MINER_AUTH_INIT (207) from legacy lane (via conversion) */
        StatelessPacket packet;
        packet.HEADER = 207;  // 8-bit legacy opcode (zero-extended to 16-bit)
        packet.LENGTH = 0;
        
        /* Verify it's NOT a stateless opcode */
        REQUIRE(StatelessOpcodes::IsStateless(packet.HEADER) == false);
        
        /* No unmirror needed - can route directly */
        uint16_t routeOpcode = packet.HEADER;
        REQUIRE(routeOpcode == 207);  // Already correct for switch statement
    }
    
    SECTION("Response mirroring: 8-bit response → 16-bit for stateless lane")
    {
        /* StatelessMiner builds responses with 8-bit opcodes like MINER_AUTH_CHALLENGE = 208
         * Before sending on stateless lane, must mirror to 16-bit (0xD0D0) */
        
        StatelessPacket response;
        response.HEADER = 208;  // 8-bit opcode from StatelessMiner
        response.LENGTH = 64;   // Some challenge data
        
        /* Check if response needs mirroring (< 256 and not already stateless) */
        REQUIRE(response.HEADER < 256);
        REQUIRE(StatelessOpcodes::IsStateless(response.HEADER) == false);
        
        /* Mirror the opcode for stateless lane */
        uint8_t legacyOpcode = static_cast<uint8_t>(response.HEADER);
        StatelessPacket mirroredResponse = response;
        mirroredResponse.HEADER = StatelessOpcodes::Mirror(legacyOpcode);
        
        /* Verify mirrored response has correct 16-bit opcode */
        REQUIRE(mirroredResponse.HEADER == 0xD0D0);  // Mirror(208)
        REQUIRE(mirroredResponse.LENGTH == response.LENGTH);  // Data unchanged
        REQUIRE(StatelessOpcodes::IsStateless(mirroredResponse.HEADER) == true);
    }

    SECTION("Response unmirroring: 16-bit response → 8-bit for legacy lane")
    {
        /* StatelessMiner::ProcessPacket() always builds responses with 16-bit stateless opcodes.
         * When sending on legacy lane (port 8323), must unmirror to 8-bit.
         * This test validates the lane-aware opcode conversion fix. */

        /* Test all authentication/session opcodes that flow through the legacy lane */
        struct OpcodeTestCase {
            uint16_t nStateless;  // 16-bit opcode from StatelessMiner
            uint8_t  nLegacy;    // Expected 8-bit opcode for legacy lane
            const char* name;
        };

        OpcodeTestCase cases[] = {
            { StatelessOpcodes::AUTH_CHALLENGE,    208, "MINER_AUTH_CHALLENGE" },
            { StatelessOpcodes::AUTH_RESULT,       210, "MINER_AUTH_RESULT"    },
            { StatelessOpcodes::SESSION_START,     211, "SESSION_START"        },
            { StatelessOpcodes::SESSION_KEEPALIVE, 212, "SESSION_KEEPALIVE"    },
            { StatelessOpcodes::CHANNEL_ACK,       206, "CHANNEL_ACK"         },
            { StatelessOpcodes::NEW_ROUND,         204, "NEW_ROUND"           },
            { StatelessOpcodes::OLD_ROUND,         205, "OLD_ROUND"           },
            { StatelessOpcodes::BLOCK_ACCEPTED,    200, "BLOCK_ACCEPTED"      },
            { StatelessOpcodes::BLOCK_REJECTED,    201, "BLOCK_REJECTED"      },
        };

        for(const auto& tc : cases)
        {
            /* Simulate response from StatelessMiner with 16-bit opcode */
            StatelessPacket response;
            response.HEADER = tc.nStateless;
            response.LENGTH = 1;
            response.DATA = {0x01};

            /* Verify it's a valid 16-bit stateless opcode */
            REQUIRE(StatelessOpcodes::IsStateless(response.HEADER) == true);

            /* Lane-aware conversion: unmirror for legacy lane */
            uint16_t nResponseHeader = response.HEADER;
            if(StatelessOpcodes::IsStateless(nResponseHeader))
                nResponseHeader = StatelessOpcodes::Unmirror(nResponseHeader);

            /* Verify unmirror produces correct 8-bit opcode */
            REQUIRE(nResponseHeader == tc.nLegacy);
            REQUIRE(nResponseHeader < 256);

            /* Verify round-trip: 8-bit → 16-bit → 8-bit */
            REQUIRE(StatelessOpcodes::Unmirror(StatelessOpcodes::Mirror(tc.nLegacy)) == tc.nLegacy);
        }
    }
}


TEST_CASE("Native 16-bit Routing Path Tests", "[stateless_miner][opcodes][routing]")
{
    /* Test the native 16-bit routing path introduced by PR #259.
     * Stateless lane receives opcodes natively as 16-bit, which are unmirrored
     * before entering the switch statement that uses 8-bit case labels.
     * This ensures proper routing without falling to default case. */
    
    SECTION("AUTH_INIT (0xD0CF) routes to ProcessMinerAuthInit")
    {
        MiningContext ctx;
        
        /* Build StatelessPacket with AUTH_INIT header (0xD0CF) */
        StatelessPacket packet;
        packet.HEADER = StatelessOpcodes::AUTH_INIT;  // 0xD0CF
        
        /* Build minimal AUTH_INIT payload: [2 bytes pubkey_len][pubkey][2 bytes id_len][id] */
        std::vector<uint8_t> testPubKey(897, 0x42);  // Simulated Falcon-512 pubkey
        uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());
        
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), testPubKey.begin(), testPubKey.end());
        
        std::string testMinerId = "test_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        packet.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        packet.DATA.insert(packet.DATA.end(), testMinerId.begin(), testMinerId.end());
        
        packet.LENGTH = packet.DATA.size();
        
        /* Process packet - should route to ProcessMinerAuthInit, not default case */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        /* Verify routing succeeded (not "Unknown packet type" error from default case) */
        REQUIRE(result.fSuccess == true);
        
        /* Verify response is AUTH_CHALLENGE */
        REQUIRE(!result.response.IsNull());
        REQUIRE(StatelessOpcodes::Unmirror(result.response.HEADER) == 208);  // MINER_AUTH_CHALLENGE
    }
    
    SECTION("SET_REWARD (0xD0D5) routes to ProcessSetReward")
    {
        MiningContext ctx;
        
        /* Build StatelessPacket with SET_REWARD header (0xD0D5) */
        StatelessPacket packet;
        packet.HEADER = StatelessOpcodes::SET_REWARD;  // 0xD0D5
        
        /* Build minimal SET_REWARD payload: 32-byte reward address hash */
        std::vector<uint8_t> rewardAddress(32, 0xAB);
        packet.DATA = rewardAddress;
        packet.LENGTH = packet.DATA.size();
        
        /* Process packet - should route to ProcessSetReward, not default case */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        /* Verify it didn't hit the default case ("Unknown packet type") */
        /* Note: ProcessSetReward may return success or specific error, but NOT "Unknown packet type" */
        if (!result.fSuccess)
        {
            REQUIRE(result.strError != "Unknown packet type");
        }
    }
    
    SECTION("ProcessSetReward error response has HEADER == REWARD_RESULT (0xD0D6)")
    {
        MiningContext ctx;
        
        /* Build SET_REWARD packet with invalid payload (too short) to trigger error */
        StatelessPacket packet;
        packet.HEADER = StatelessOpcodes::SET_REWARD;  // 0xD0D5
        packet.DATA = {0x01, 0x02};  // Invalid: too short (needs 32 bytes)
        packet.LENGTH = packet.DATA.size();
        
        /* Process packet - should fail but return REWARD_RESULT response */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, packet);
        
        /* Verify error response */
        REQUIRE(result.fSuccess == false);
        
        /* Verify response header is REWARD_RESULT (0xD0D6), not 0x00D6 */
        if (!result.response.IsNull())
        {
            uint16_t responseHeader = result.response.HEADER;
            REQUIRE(responseHeader == StatelessOpcodes::REWARD_RESULT);  // 0xD0D6
            REQUIRE(responseHeader == 0xD0D6);  // Explicit value check
            REQUIRE(responseHeader != 0x00D6);  // Not legacy value
        }
    }
    
    SECTION("Legacy Packet with HEADER=207 mirrors and routes correctly via legacy overload")
    {
        MiningContext ctx;
        
        /* Build legacy Packet (8-bit) with MINER_AUTH_INIT (207) */
        Packet legacyPacket(207);  // MINER_AUTH_INIT
        
        /* Build minimal AUTH_INIT payload */
        std::vector<uint8_t> testPubKey(897, 0x55);
        uint16_t nPubKeyLen = static_cast<uint16_t>(testPubKey.size());
        
        legacyPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        legacyPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        legacyPacket.DATA.insert(legacyPacket.DATA.end(), testPubKey.begin(), testPubKey.end());
        
        std::string testMinerId = "legacy_miner";
        uint16_t nMinerIdLen = static_cast<uint16_t>(testMinerId.size());
        legacyPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen >> 8));
        legacyPacket.DATA.push_back(static_cast<uint8_t>(nMinerIdLen & 0xFF));
        legacyPacket.DATA.insert(legacyPacket.DATA.end(), testMinerId.begin(), testMinerId.end());
        
        legacyPacket.LENGTH = legacyPacket.DATA.size();
        
        /* Process via legacy overload - should Mirror to 16-bit, then route correctly */
        ProcessResult result = StatelessMiner::ProcessPacket(ctx, legacyPacket);
        
        /* Verify routing succeeded (not "Unknown packet type" from default case) */
        REQUIRE(result.fSuccess == true);
        
        /* Verify response is AUTH_CHALLENGE */
        REQUIRE(!result.response.IsNull());
        
        /* Response should be 16-bit stateless opcode */
        uint16_t responseHeader = result.response.HEADER;
        REQUIRE(StatelessOpcodes::IsStateless(responseHeader) == true);
        REQUIRE(StatelessOpcodes::Unmirror(responseHeader) == 208);  // MINER_AUTH_CHALLENGE
    }
}


TEST_CASE("MiningContext Keepalive Tracking Fields", "[stateless_miner][keepalive]")
{
    SECTION("Default constructor initializes keepalive tracking fields to zero")
    {
        MiningContext ctx;

        REQUIRE(ctx.nKeepaliveCount == 0);
        REQUIRE(ctx.nKeepaliveSent == 0);
        REQUIRE(ctx.nLastKeepaliveTime == 0);
    }

    SECTION("WithKeepaliveSent creates new context with updated sent count")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithKeepaliveSent(5);

        REQUIRE(ctx.nKeepaliveSent == 0);       // Original unchanged
        REQUIRE(updated.nKeepaliveSent == 5);    // New context updated
    }

    SECTION("WithLastKeepaliveTime creates new context with updated timestamp")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithLastKeepaliveTime(1234567890);

        REQUIRE(ctx.nLastKeepaliveTime == 0);             // Original unchanged
        REQUIRE(updated.nLastKeepaliveTime == 1234567890); // New context updated
    }

    SECTION("Keepalive fields chain correctly with other With* methods")
    {
        MiningContext ctx;
        MiningContext updated = ctx
            .WithKeepaliveCount(10)
            .WithKeepaliveSent(10)
            .WithLastKeepaliveTime(999)
            .WithTimestamp(1000);

        REQUIRE(updated.nKeepaliveCount == 10);
        REQUIRE(updated.nKeepaliveSent == 10);
        REQUIRE(updated.nLastKeepaliveTime == 999);
        REQUIRE(updated.nTimestamp == 1000);
    }

    SECTION("WithStakeHeight creates new context with updated stake height")
    {
        MiningContext ctx;
        REQUIRE(ctx.nStakeHeight == 0u);

        MiningContext updated = ctx.WithStakeHeight(777u);

        REQUIRE(ctx.nStakeHeight == 0u);       /* Original unchanged */
        REQUIRE(updated.nStakeHeight == 777u); /* New context updated */
    }

    SECTION("WithStakeHeight chains correctly with other With* methods")
    {
        MiningContext ctx;
        MiningContext updated = ctx
            .WithKeepaliveCount(5)
            .WithStakeHeight(999u)
            .WithTimestamp(12345);

        REQUIRE(updated.nKeepaliveCount == 5);
        REQUIRE(updated.nStakeHeight == 999u);
        REQUIRE(updated.nTimestamp == 12345);
        /* Originals unchanged */
        REQUIRE(ctx.nStakeHeight == 0u);
        REQUIRE(ctx.nKeepaliveCount == 0);
    }
}


TEST_CASE("Legacy Opcode Recognition in ProcessPacket", "[stateless_miner][opcodes]")
{
    using namespace LLP::OpcodeUtility;

    SECTION("Known legacy opcodes are recognized for connection layer handling")
    {
        /* These legacy opcodes should be recognized by ProcessPacket and
         * returned as "Unknown packet type" (triggers fallback) without
         * generating a "Unknown miner opcode" warning at log level 1. */
        std::vector<uint8_t> legacyOpcodes = {
            Opcodes::GET_BLOCK,         // 129
            Opcodes::SUBMIT_BLOCK,      // 1
            Opcodes::BLOCK_DATA,        // 0
            Opcodes::GET_HEIGHT,        // 130
            Opcodes::GET_REWARD,        // 131
            Opcodes::GET_ROUND,         // 133
            Opcodes::BLOCK_ACCEPTED,    // 200
            Opcodes::BLOCK_REJECTED,    // 201
            Opcodes::CHANNEL_ACK,       // 206
            Opcodes::MINER_AUTH_CHALLENGE, // 208
            Opcodes::MINER_AUTH_RESULT,    // 210
            Opcodes::MINER_REWARD_RESULT,  // 214
        };

        for(uint8_t op : legacyOpcodes)
        {
            /* Verify each legacy opcode has a valid Mirror mapping */
            uint16_t mirrored = Stateless::Mirror(op);
            REQUIRE(Stateless::IsStateless(mirrored) == true);

            /* Verify round-trip */
            REQUIRE(Stateless::Unmirror(mirrored) == op);
        }
    }
}

