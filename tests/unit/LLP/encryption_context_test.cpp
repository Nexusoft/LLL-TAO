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

#include <LLC/include/mining_session_keys.h>

#include <Util/include/runtime.h>

using namespace LLP;

/* Packet type definitions for testing */
const Packet::message_t MINER_AUTH_INIT_TEST = 207;
const Packet::message_t MINER_AUTH_CHALLENGE_TEST = 208;
const Packet::message_t MINER_AUTH_RESPONSE_TEST = 209;
const Packet::message_t MINER_AUTH_RESULT_TEST = 210;


TEST_CASE("Encryption Context Setup After Authentication", "[encryption][authentication]")
{
    /* Initialize Falcon auth for testing */
    FalconAuth::Initialize();

    SECTION("Encryption context is set after successful Falcon authentication")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a real Falcon key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "test_encryption_context"
        );

        /* Create a test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Bind the genesis to the key */
        bool bound = pAuth->BindGenesis(meta.keyId, testGenesis);
        REQUIRE(bound == true);

        /* Step 1: MINER_AUTH_INIT with genesis */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT_TEST);

        /* Add pubkey length (2 bytes, big-endian) */
        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* Add pubkey */
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        /* Add miner ID length (2 bytes, big-endian) - set to 0 for this test */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* Add genesis (32 bytes) */
        std::vector<uint8_t> vGenesis = testGenesis.GetBytes();
        initPacket.DATA.insert(initPacket.DATA.end(), vGenesis.begin(), vGenesis.end());

        /* Process INIT packet */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE_TEST);
        REQUIRE(!initResult.context.vAuthNonce.empty());
        REQUIRE(initResult.context.hashGenesis == testGenesis);

        /* Verify encryption is NOT ready yet (not authenticated yet) */
        REQUIRE(initResult.context.fEncryptionReady == false);
        REQUIRE(initResult.context.vChaChaKey.empty());

        /* Step 2: Sign the challenge with real Falcon key */
        std::vector<uint8_t> vSignature = pAuth->Sign(meta.keyId, initResult.context.vAuthNonce);
        REQUIRE(!vSignature.empty());

        /* Step 3: MINER_AUTH_RESPONSE */
        Packet responsePacket(MINER_AUTH_RESPONSE_TEST);
        
        /* Add signature length (2 bytes, little-endian) */
        uint16_t nSigLen = static_cast<uint16_t>(vSignature.size());
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen & 0xFF));
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen >> 8));
        
        /* Add signature */
        responsePacket.DATA.insert(responsePacket.DATA.end(), vSignature.begin(), vSignature.end());

        /* Process AUTH_RESPONSE packet */
        ProcessResult authResult = StatelessMiner::ProcessPacket(initResult.context, responsePacket);

        /* Verify authentication succeeded */
        REQUIRE(authResult.fSuccess == true);
        REQUIRE(authResult.response.HEADER == MINER_AUTH_RESULT_TEST);
        REQUIRE(authResult.response.DATA.size() >= 1);
        REQUIRE(authResult.response.DATA[0] == 0x01);  // Success code
        REQUIRE(authResult.context.fAuthenticated == true);

        /* CRITICAL: Verify encryption context is now set */
        REQUIRE(authResult.context.fEncryptionReady == true);
        REQUIRE(!authResult.context.vChaChaKey.empty());
        REQUIRE(authResult.context.vChaChaKey.size() == 32);

        /* Verify the ChaCha20 key is correctly derived from genesis */
        std::vector<uint8_t> expectedKey = LLC::MiningSessionKeys::DeriveChaCha20Key(testGenesis);
        REQUIRE(authResult.context.vChaChaKey == expectedKey);

        /* Verify session ID is set */
        REQUIRE(authResult.context.nSessionId != 0);

        /* Verify genesis is preserved */
        REQUIRE(authResult.context.hashGenesis == testGenesis);
    }

    SECTION("Encryption context is not set if genesis is missing")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a real Falcon key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "test_no_genesis"
        );

        /* Step 1: MINER_AUTH_INIT WITHOUT genesis */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT_TEST);

        /* Add pubkey */
        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        /* Add empty miner ID */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* NO genesis added - packet ends here */

        /* Process INIT packet */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE_TEST);

        /* Step 2: Sign and respond */
        std::vector<uint8_t> vSignature = pAuth->Sign(meta.keyId, initResult.context.vAuthNonce);
        REQUIRE(!vSignature.empty());

        Packet responsePacket(MINER_AUTH_RESPONSE_TEST);
        uint16_t nSigLen = static_cast<uint16_t>(vSignature.size());
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen & 0xFF));
        responsePacket.DATA.push_back(static_cast<uint8_t>(nSigLen >> 8));
        responsePacket.DATA.insert(responsePacket.DATA.end(), vSignature.begin(), vSignature.end());

        ProcessResult authResult = StatelessMiner::ProcessPacket(initResult.context, responsePacket);

        /* Should fail because genesis is required for ChaCha20 encryption */
        REQUIRE(authResult.fSuccess == true);  // Returns response
        REQUIRE(authResult.response.DATA.size() >= 1);
        REQUIRE(authResult.response.DATA[0] == 0x00);  // Failure code
        
        /* Encryption context should NOT be set */
        REQUIRE(authResult.context.fEncryptionReady == false);
        REQUIRE(authResult.context.vChaChaKey.empty());
    }

    SECTION("ChaCha20 key derivation is deterministic")
    {
        /* Create test genesis */
        uint256_t genesis;
        genesis.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");

        /* Derive key multiple times */
        std::vector<uint8_t> key1 = LLC::MiningSessionKeys::DeriveChaCha20Key(genesis);
        std::vector<uint8_t> key2 = LLC::MiningSessionKeys::DeriveChaCha20Key(genesis);
        std::vector<uint8_t> key3 = LLC::MiningSessionKeys::DeriveChaCha20Key(genesis);

        /* All keys should be identical (deterministic) */
        REQUIRE(key1 == key2);
        REQUIRE(key2 == key3);
        
        /* All keys should be 32 bytes */
        REQUIRE(key1.size() == 32);
        REQUIRE(key2.size() == 32);
        REQUIRE(key3.size() == 32);
    }

    SECTION("Different genesis produces different keys")
    {
        uint256_t genesis1;
        genesis1.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        uint256_t genesis2;
        genesis2.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");

        std::vector<uint8_t> key1 = LLC::MiningSessionKeys::DeriveChaCha20Key(genesis1);
        std::vector<uint8_t> key2 = LLC::MiningSessionKeys::DeriveChaCha20Key(genesis2);

        /* Different genesis should produce different keys */
        REQUIRE(key1 != key2);
        
        /* Both should be 32 bytes */
        REQUIRE(key1.size() == 32);
        REQUIRE(key2.size() == 32);
    }
}
