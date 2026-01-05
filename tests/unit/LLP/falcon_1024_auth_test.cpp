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
#include <LLC/include/flkey.h>

#include <Util/include/runtime.h>

using namespace LLP;

/* Packet type definitions for testing */
const Packet::message_t MINER_AUTH_INIT = 207;
const Packet::message_t MINER_AUTH_CHALLENGE = 208;


TEST_CASE("Falcon-1024 Authentication Handler Tests", "[falcon][falcon1024][authentication]")
{
    /* Initialize Falcon auth for testing */
    FalconAuth::Initialize();

    SECTION("MINER_AUTH_INIT accepts Falcon-512 public key (no regression)")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a Falcon-512 key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_512,
            "test_falcon512_auth"
        );

        REQUIRE(meta.pubkey.size() == LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE);  // Falcon-512 pubkey size

        /* Create test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Create MINER_AUTH_INIT packet */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT);

        /* Add pubkey length (2 bytes, big-endian) */
        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* Add pubkey */
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        /* Add miner ID length (2 bytes, big-endian) - set to 0 */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* Add genesis (32 bytes) */
        std::vector<uint8_t> vGenesis = testGenesis.GetBytes();
        initPacket.DATA.insert(initPacket.DATA.end(), vGenesis.begin(), vGenesis.end());

        /* Process INIT packet */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        /* Should succeed */
        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE);
        REQUIRE(!initResult.context.vAuthNonce.empty());
        REQUIRE(initResult.context.hashGenesis == testGenesis);
        
        /* Verify Falcon version was detected and stored */
        REQUIRE(initResult.context.fFalconVersionDetected == true);
        REQUIRE(initResult.context.nFalconVersion == LLC::FalconVersion::FALCON_512);
    }

    SECTION("MINER_AUTH_INIT accepts Falcon-1024 public key (CRITICAL FIX)")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a Falcon-1024 key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_1024,
            "test_falcon1024_auth"
        );

        REQUIRE(meta.pubkey.size() == LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE);  // Falcon-1024 pubkey size

        /* Create test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");

        /* Create MINER_AUTH_INIT packet */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT);

        /* Add pubkey length (2 bytes, big-endian) */
        uint16_t nPubKeyLen = static_cast<uint16_t>(meta.pubkey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* Add pubkey */
        initPacket.DATA.insert(initPacket.DATA.end(), meta.pubkey.begin(), meta.pubkey.end());

        /* Add miner ID length (2 bytes, big-endian) - set to 0 */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* Add genesis (32 bytes) */
        std::vector<uint8_t> vGenesis = testGenesis.GetBytes();
        initPacket.DATA.insert(initPacket.DATA.end(), vGenesis.begin(), vGenesis.end());

        /* Process INIT packet - THIS SHOULD NOW SUCCEED */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        /* Should succeed (previously failed with "Invalid pubkey size: 1793 (expected 897)") */
        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE);
        REQUIRE(!initResult.context.vAuthNonce.empty());
        REQUIRE(initResult.context.hashGenesis == testGenesis);
        
        /* Verify Falcon version was detected and stored */
        REQUIRE(initResult.context.fFalconVersionDetected == true);
        REQUIRE(initResult.context.nFalconVersion == LLC::FalconVersion::FALCON_1024);
    }

    SECTION("MINER_AUTH_INIT rejects invalid public key size")
    {
        /* Create test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("c396233e15ec3e02ded7510604d389eeed55f5376517fc0167cf11ca529ef344");

        /* Create MINER_AUTH_INIT packet with invalid pubkey size */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT);

        /* Add invalid pubkey length (500 bytes - not a valid Falcon size) */
        uint16_t nPubKeyLen = 500;
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nPubKeyLen & 0xFF));
        
        /* Add dummy pubkey data */
        std::vector<uint8_t> vBadPubKey(500, 0xAA);
        initPacket.DATA.insert(initPacket.DATA.end(), vBadPubKey.begin(), vBadPubKey.end());

        /* Add miner ID length */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* Add genesis */
        std::vector<uint8_t> vGenesis = testGenesis.GetBytes();
        initPacket.DATA.insert(initPacket.DATA.end(), vGenesis.begin(), vGenesis.end());

        /* Process INIT packet - should FAIL */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        /* Should fail with invalid size error */
        REQUIRE(initResult.fSuccess == false);
    }
}


TEST_CASE("Falcon-1024 ChaCha20 Wrapped Authentication", "[falcon][falcon1024][chacha20]")
{
    /* Initialize Falcon auth for testing */
    FalconAuth::Initialize();

    SECTION("MINER_AUTH_INIT accepts ChaCha20-wrapped Falcon-1024 public key")
    {
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        REQUIRE(pAuth != nullptr);

        /* Generate a Falcon-1024 key */
        FalconAuth::KeyMetadata meta = pAuth->GenerateKey(
            FalconAuth::Profile::FALCON_1024,
            "test_wrapped_falcon1024"
        );

        constexpr size_t CHACHA20_OVERHEAD = 12 + 16;  // nonce(12) + tag(16)
        constexpr size_t FALCON1024_WRAPPED_SIZE = LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE + CHACHA20_OVERHEAD; // 1793 + 28 = 1821
        REQUIRE(meta.pubkey.size() == LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE);  // Falcon-1024 pubkey size

        /* Create test genesis hash */
        uint256_t testGenesis;
        testGenesis.SetHex("d4a7344f26fd4f13fee8621715e4a0ffff66a6487628ad1278da22da630fa455");

        /* Derive ChaCha20 key from genesis */
        std::vector<uint8_t> vSessionKey = LLC::MiningSessionKeys::DeriveChaCha20Key(testGenesis);

        /* Wrap the public key with ChaCha20-Poly1305 */
        std::vector<uint8_t> vWrappedPubKey = LLC::EncryptPayloadChaCha20(
            meta.pubkey,
            vSessionKey,
            {'F','A','L','C','O','N','_','P','U','B','K','E','Y'}
        );

        /* Wrapped size should be 1793 + 12 (nonce) + 16 (tag) = 1821 bytes */
        REQUIRE(vWrappedPubKey.size() == FALCON1024_WRAPPED_SIZE);

        /* Create MINER_AUTH_INIT packet */
        MiningContext ctx;
        Packet initPacket(MINER_AUTH_INIT);

        /* Add wrapped pubkey length (2 bytes, big-endian) */
        uint16_t nWrappedLen = static_cast<uint16_t>(vWrappedPubKey.size());
        initPacket.DATA.push_back(static_cast<uint8_t>(nWrappedLen >> 8));
        initPacket.DATA.push_back(static_cast<uint8_t>(nWrappedLen & 0xFF));
        
        /* Add wrapped pubkey */
        initPacket.DATA.insert(initPacket.DATA.end(), vWrappedPubKey.begin(), vWrappedPubKey.end());

        /* Add miner ID length (2 bytes, big-endian) - set to 0 */
        initPacket.DATA.push_back(0x00);
        initPacket.DATA.push_back(0x00);

        /* Add genesis (32 bytes) */
        std::vector<uint8_t> vGenesis = testGenesis.GetBytes();
        initPacket.DATA.insert(initPacket.DATA.end(), vGenesis.begin(), vGenesis.end());

        /* Process INIT packet - should unwrap and validate */
        ProcessResult initResult = StatelessMiner::ProcessPacket(ctx, initPacket);

        /* Should succeed */
        REQUIRE(initResult.fSuccess == true);
        REQUIRE(initResult.response.HEADER == MINER_AUTH_CHALLENGE);
        REQUIRE(!initResult.context.vAuthNonce.empty());
        
        /* Verify Falcon-1024 version was detected */
        REQUIRE(initResult.context.fFalconVersionDetected == true);
        REQUIRE(initResult.context.nFalconVersion == LLC::FalconVersion::FALCON_1024);
    }
}
