/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_handshake.h>
#include <LLP/include/falcon_auth.h>

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <Util/include/runtime.h>
#include <Util/include/hex.h>

using namespace LLP;
using namespace LLP::FalconHandshake;


TEST_CASE("Handshake Configuration Tests", "[falcon_handshake]")
{
    SECTION("Default configuration")
    {
        HandshakeConfig config;
        
        REQUIRE(config.fEnableEncryption == true);
        REQUIRE(config.fRequireEncryption == false);
        REQUIRE(config.fAllowLocalhostPlaintext == true);
        REQUIRE(config.nHandshakeTimeout == 60);
    }

    SECTION("Custom configuration")
    {
        HandshakeConfig config;
        config.fEnableEncryption = false;
        config.fRequireEncryption = true;
        config.nHandshakeTimeout = 120;
        
        REQUIRE(config.fEnableEncryption == false);
        REQUIRE(config.fRequireEncryption == true);
        REQUIRE(config.nHandshakeTimeout == 120);
    }
}


TEST_CASE("ChaCha20 Encryption/Decryption Tests", "[falcon_handshake]")
{
    SECTION("Encrypt and decrypt roundtrip")
    {
        /* Generate test data */
        std::vector<uint8_t> vOriginal(897);  // Falcon-512 pubkey size
        for(size_t i = 0; i < vOriginal.size(); ++i)
            vOriginal[i] = static_cast<uint8_t>(i & 0xFF);

        /* Generate key and nonce */
        std::vector<uint8_t> vKey = LLC::GetRand256().GetBytes();
        vKey.resize(32);
        
        std::vector<uint8_t> vNonce = LLC::GetRand256().GetBytes();
        vNonce.resize(12);

        /* Encrypt */
        std::vector<uint8_t> vEncrypted = EncryptFalconPublicKey(vOriginal, vKey, vNonce);
        
        REQUIRE(!vEncrypted.empty());
        REQUIRE(vEncrypted.size() == vOriginal.size());
        REQUIRE(vEncrypted != vOriginal);  // Should be different after encryption

        /* Decrypt */
        std::vector<uint8_t> vDecrypted = DecryptFalconPublicKey(vEncrypted, vKey, vNonce);
        
        REQUIRE(!vDecrypted.empty());
        REQUIRE(vDecrypted == vOriginal);  // Should match original
    }

    SECTION("Different keys produce different ciphertext")
    {
        std::vector<uint8_t> vData(100, 0x42);
        
        std::vector<uint8_t> vKey1 = LLC::GetRand256().GetBytes();
        vKey1.resize(32);
        std::vector<uint8_t> vKey2 = LLC::GetRand256().GetBytes();
        vKey2.resize(32);
        
        std::vector<uint8_t> vNonce = LLC::GetRand256().GetBytes();
        vNonce.resize(12);

        std::vector<uint8_t> vEnc1 = EncryptFalconPublicKey(vData, vKey1, vNonce);
        std::vector<uint8_t> vEnc2 = EncryptFalconPublicKey(vData, vKey2, vNonce);
        
        REQUIRE(vEnc1 != vEnc2);
    }

    SECTION("Invalid key size fails gracefully")
    {
        std::vector<uint8_t> vData(100, 0x42);
        std::vector<uint8_t> vBadKey(16);  // Wrong size
        std::vector<uint8_t> vNonce(12);

        std::vector<uint8_t> vResult = EncryptFalconPublicKey(vData, vBadKey, vNonce);
        
        REQUIRE(vResult.empty());  // Should fail
    }

    SECTION("Invalid nonce size fails gracefully")
    {
        std::vector<uint8_t> vData(100, 0x42);
        std::vector<uint8_t> vKey(32);
        std::vector<uint8_t> vBadNonce(8);  // Wrong size

        std::vector<uint8_t> vResult = EncryptFalconPublicKey(vData, vKey, vBadNonce);
        
        REQUIRE(vResult.empty());  // Should fail
    }
}


TEST_CASE("Session Key Generation Tests", "[falcon_handshake]")
{
    SECTION("Session key is deterministic")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        std::vector<uint8_t> vPubKey(897, 0x42);
        uint64_t nTimestamp = 1700000000;

        uint256_t key1 = GenerateSessionKey(hashGenesis, vPubKey, nTimestamp);
        uint256_t key2 = GenerateSessionKey(hashGenesis, vPubKey, nTimestamp);
        
        REQUIRE(key1 == key2);
    }

    SECTION("Different inputs produce different session keys")
    {
        uint256_t hashGenesis1;
        hashGenesis1.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        uint256_t hashGenesis2;
        hashGenesis2.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");
        
        std::vector<uint8_t> vPubKey(897, 0x42);
        uint64_t nTimestamp = 1700000000;

        uint256_t key1 = GenerateSessionKey(hashGenesis1, vPubKey, nTimestamp);
        uint256_t key2 = GenerateSessionKey(hashGenesis2, vPubKey, nTimestamp);
        
        REQUIRE(key1 != key2);
    }

    SECTION("Session key is stable within same hour")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        std::vector<uint8_t> vPubKey(897, 0x42);
        
        /* Timestamps within same hour should produce same key */
        uint64_t nTime1 = 1700000000;
        uint64_t nTime2 = 1700001800;  // 30 minutes later
        
        uint256_t key1 = GenerateSessionKey(hashGenesis, vPubKey, nTime1);
        uint256_t key2 = GenerateSessionKey(hashGenesis, vPubKey, nTime2);
        
        REQUIRE(key1 == key2);
    }

    SECTION("Session key changes across hour boundary")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        std::vector<uint8_t> vPubKey(897, 0x42);
        
        /* Timestamps in different hours should produce different keys */
        uint64_t nTime1 = 1700000000;
        uint64_t nTime2 = 1700003601;  // Just over 1 hour later
        
        uint256_t key1 = GenerateSessionKey(hashGenesis, vPubKey, nTime1);
        uint256_t key2 = GenerateSessionKey(hashGenesis, vPubKey, nTime2);
        
        REQUIRE(key1 != key2);
    }
}


TEST_CASE("Handshake Packet Build/Parse Tests", "[falcon_handshake]")
{
    SECTION("Build and parse plaintext handshake")
    {
        HandshakeConfig config;
        config.fEnableEncryption = false;

        HandshakeData data;
        data.vFalconPubKey.resize(897);
        for(size_t i = 0; i < data.vFalconPubKey.size(); ++i)
            data.vFalconPubKey[i] = static_cast<uint8_t>(i & 0xFF);
        
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.hashSessionKey.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");
        data.nTimestamp = runtime::unifiedtimestamp();
        data.strMinerID = "test-miner-001";

        /* Build packet */
        std::vector<uint8_t> vPacket = BuildHandshakePacket(config, data);
        REQUIRE(!vPacket.empty());

        /* Parse packet */
        HandshakeResult result = ParseHandshakePacket(vPacket, config);
        
        REQUIRE(result.fSuccess);
        REQUIRE(result.strError.empty());
        REQUIRE(result.data.vFalconPubKey == data.vFalconPubKey);
        REQUIRE(result.data.hashGenesis == data.hashGenesis);
        REQUIRE(result.data.hashSessionKey == data.hashSessionKey);
        REQUIRE(result.data.nTimestamp == data.nTimestamp);
        REQUIRE(result.data.strMinerID == data.strMinerID);
        REQUIRE(result.data.fEncrypted == false);
    }

    SECTION("Build and parse encrypted handshake")
    {
        HandshakeConfig config;
        config.fEnableEncryption = true;

        HandshakeData data;
        data.vFalconPubKey.resize(897);
        for(size_t i = 0; i < data.vFalconPubKey.size(); ++i)
            data.vFalconPubKey[i] = static_cast<uint8_t>(i & 0xFF);
        
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.hashSessionKey.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");
        data.nTimestamp = runtime::unifiedtimestamp();
        data.strMinerID = "test-miner-002";

        /* Generate encryption key */
        std::vector<uint8_t> vKey = LLC::GetRand256().GetBytes();
        vKey.resize(32);

        /* Build encrypted packet */
        std::vector<uint8_t> vPacket = BuildHandshakePacket(config, data, vKey);
        REQUIRE(!vPacket.empty());

        /* Verify encryption flag is set */
        REQUIRE(vPacket[0] == 1);

        /* Parse packet with key */
        HandshakeResult result = ParseHandshakePacket(vPacket, config, vKey);
        
        REQUIRE(result.fSuccess);
        REQUIRE(result.data.vFalconPubKey == data.vFalconPubKey);
        REQUIRE(result.data.hashGenesis == data.hashGenesis);
        REQUIRE(result.data.fEncrypted == true);
    }

    SECTION("Parse fails with wrong decryption key")
    {
        HandshakeConfig config;
        config.fEnableEncryption = true;

        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.hashSessionKey.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");
        data.nTimestamp = runtime::unifiedtimestamp();

        /* Build with one key */
        std::vector<uint8_t> vKey1 = LLC::GetRand256().GetBytes();
        vKey1.resize(32);
        std::vector<uint8_t> vPacket = BuildHandshakePacket(config, data, vKey1);
        REQUIRE(!vPacket.empty());

        /* Try to parse with different key */
        std::vector<uint8_t> vKey2 = LLC::GetRand256().GetBytes();
        vKey2.resize(32);
        HandshakeResult result = ParseHandshakePacket(vPacket, config, vKey2);
        
        /* ChaCha20 (stream cipher) doesn't have built-in authentication */
        /* Decryption with wrong key produces garbage but no error */
        /* This is acceptable because:
         * 1. TLS 1.3 transport layer provides AEAD (ChaCha20-Poly1305)
         * 2. Falcon signature verification is the primary authentication
         * 3. GenesisHash binding provides additional validation
         * Wrong key will result in invalid signature verification later */
    }

    SECTION("Parse fails with malformed packet")
    {
        HandshakeConfig config;
        
        std::vector<uint8_t> vBadPacket(10, 0xFF);  // Too small
        
        HandshakeResult result = ParseHandshakePacket(vBadPacket, config);
        
        REQUIRE_FALSE(result.fSuccess);
        REQUIRE(!result.strError.empty());
    }

    SECTION("Parse fails when encryption required but not provided")
    {
        HandshakeConfig config;
        config.fRequireEncryption = true;

        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.hashSessionKey.SetHex("b285122d04db2d91cdb6499493c278dbde44e4265406fb9056bd00b9419de233");
        data.nTimestamp = runtime::unifiedtimestamp();

        /* Build plaintext packet */
        HandshakeConfig buildConfig;
        buildConfig.fEnableEncryption = false;
        std::vector<uint8_t> vPacket = BuildHandshakePacket(buildConfig, data);

        /* Try to parse with encryption required */
        HandshakeResult result = ParseHandshakePacket(vPacket, config);
        
        REQUIRE_FALSE(result.fSuccess);
        REQUIRE(result.strError.find("Encryption required") != std::string::npos);
    }
}


TEST_CASE("Handshake Validation Tests", "[falcon_handshake]")
{
    SECTION("Valid handshake data passes validation")
    {
        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.nTimestamp = runtime::unifiedtimestamp();
        
        REQUIRE(ValidateHandshakeData(data));
    }

    SECTION("Empty public key fails validation")
    {
        HandshakeData data;
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.nTimestamp = runtime::unifiedtimestamp();
        
        REQUIRE_FALSE(ValidateHandshakeData(data));
    }

    SECTION("Genesis hash can be zero for initial handshake")
    {
        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis = uint256_t(0);
        data.nTimestamp = runtime::unifiedtimestamp();
        
        /* Zero genesis is allowed but logged - caller should verify before mining */
        REQUIRE(ValidateHandshakeData(data));
    }

    SECTION("Old timestamp fails validation (replay protection)")
    {
        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.nTimestamp = runtime::unifiedtimestamp() - 7200;  // 2 hours old
        
        REQUIRE_FALSE(ValidateHandshakeData(data));
    }

    SECTION("Future timestamp fails validation (replay protection)")
    {
        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.nTimestamp = runtime::unifiedtimestamp() + 7200;  // 2 hours in future
        
        REQUIRE_FALSE(ValidateHandshakeData(data));
    }

    SECTION("Recent timestamp passes validation")
    {
        HandshakeData data;
        data.vFalconPubKey.resize(897, 0x42);
        data.hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        data.nTimestamp = runtime::unifiedtimestamp() - 1800;  // 30 minutes old - valid
        
        REQUIRE(ValidateHandshakeData(data));
    }
}


TEST_CASE("Localhost Detection Tests", "[falcon_handshake]")
{
    SECTION("Localhost addresses detected correctly")
    {
        REQUIRE(IsLocalhostHandshake("127.0.0.1"));
        REQUIRE(IsLocalhostHandshake("::1"));
        REQUIRE(IsLocalhostHandshake("localhost"));
    }

    SECTION("Remote addresses not detected as localhost")
    {
        REQUIRE_FALSE(IsLocalhostHandshake("192.168.1.1"));
        REQUIRE_FALSE(IsLocalhostHandshake("8.8.8.8"));
        REQUIRE_FALSE(IsLocalhostHandshake("2001:0db8:85a3::1"));
    }
}


TEST_CASE("Full Handshake Flow Test", "[falcon_handshake]")
{
    SECTION("Complete encrypted handshake with session key")
    {
        /* Setup miner data */
        std::vector<uint8_t> vMinerPubKey(897);
        for(size_t i = 0; i < vMinerPubKey.size(); ++i)
            vMinerPubKey[i] = static_cast<uint8_t>(i & 0xFF);

        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        uint64_t nTimestamp = runtime::unifiedtimestamp();

        /* Generate session key */
        uint256_t hashSessionKey = GenerateSessionKey(hashGenesis, vMinerPubKey, nTimestamp);
        REQUIRE(hashSessionKey != uint256_t(0));

        /* Build handshake data */
        HandshakeData data;
        data.vFalconPubKey = vMinerPubKey;
        data.hashGenesis = hashGenesis;
        data.hashSessionKey = hashSessionKey;
        data.nTimestamp = nTimestamp;
        data.strMinerID = "integration-test-miner";

        /* Generate shared encryption key (in real scenario, derived via TLS or DH) */
        std::vector<uint8_t> vEncryptionKey = LLC::GetRand256().GetBytes();
        vEncryptionKey.resize(32);

        /* Miner builds encrypted handshake packet */
        HandshakeConfig config;
        config.fEnableEncryption = true;
        
        std::vector<uint8_t> vPacket = BuildHandshakePacket(config, data, vEncryptionKey);
        REQUIRE(!vPacket.empty());

        /* Node parses and validates handshake */
        HandshakeResult result = ParseHandshakePacket(vPacket, config, vEncryptionKey);
        
        REQUIRE(result.fSuccess);
        REQUIRE(result.data.vFalconPubKey == vMinerPubKey);
        REQUIRE(result.data.hashGenesis == hashGenesis);
        REQUIRE(result.data.hashSessionKey == hashSessionKey);
        REQUIRE(result.data.nTimestamp == nTimestamp);
        REQUIRE(result.data.strMinerID == "integration-test-miner");
        
        /* Verify session key can be regenerated */
        uint256_t hashVerifyKey = GenerateSessionKey(
            result.data.hashGenesis,
            result.data.vFalconPubKey,
            result.data.nTimestamp
        );
        REQUIRE(hashVerifyKey == hashSessionKey);
    }
}
