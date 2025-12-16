/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/chacha20_helpers.h>
#include <LLC/hash/SK.h>
#include <LLC/types/uint1024.h>

#include <unit/catch2/catch.hpp>

#include <vector>
#include <string>
#include <cstring>


TEST_CASE("ChaCha20 Helper Functions Tests", "[LLC]")
{
    SECTION("Basic encryption and decryption round-trip")
    {
        /* Test data */
        std::string strPlaintext = "The quick brown fox jumps over the lazy dog";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        /* 256-bit key (32 bytes) */
        std::vector<uint8_t> vKey(32);
        for(size_t i = 0; i < 32; ++i)
            vKey[i] = static_cast<uint8_t>(i + 0x80);

        /* Encrypt using helper */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        REQUIRE(!vEncrypted.empty());

        /* Verify packaged format: nonce(12) + ciphertext + tag(16) */
        REQUIRE(vEncrypted.size() >= 28);  // At least nonce + tag
        REQUIRE(vEncrypted.size() == 12 + vPlaintext.size() + 16);

        /* Decrypt using helper */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted) == true);

        /* Verify decrypted matches original */
        REQUIRE(vDecrypted.size() == vPlaintext.size());
        REQUIRE(memcmp(vDecrypted.data(), vPlaintext.data(), vPlaintext.size()) == 0);
    }

    SECTION("Encryption with AAD (Additional Authenticated Data)")
    {
        std::string strPlaintext = "Reward address binding";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        std::vector<uint8_t> vKey(32, 0x42);  // All 0x42

        /* AAD for domain separation (like REWARD_ADDRESS in stateless_miner.cpp) */
        std::vector<uint8_t> vAAD = {'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};

        /* Encrypt with AAD */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey, vAAD);
        REQUIRE(!vEncrypted.empty());

        /* Decrypt with matching AAD */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted, vAAD) == true);
        REQUIRE(memcmp(vDecrypted.data(), vPlaintext.data(), vPlaintext.size()) == 0);

        /* Decrypt with WRONG AAD should fail */
        std::vector<uint8_t> vWrongAAD = {'W','R','O','N','G','_','A','A','D'};
        std::vector<uint8_t> vDecrypted2;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted2, vWrongAAD) == false);
    }

    SECTION("Nonce uniqueness - different nonces for same plaintext")
    {
        std::string strPlaintext = "Same message";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vKey(32, 0xAA);

        /* Encrypt same plaintext twice */
        std::vector<uint8_t> vEncrypted1 = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        std::vector<uint8_t> vEncrypted2 = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);

        REQUIRE(!vEncrypted1.empty());
        REQUIRE(!vEncrypted2.empty());

        /* Encrypted payloads should be different (different random nonces) */
        REQUIRE(vEncrypted1 != vEncrypted2);

        /* But both should decrypt to the same plaintext */
        std::vector<uint8_t> vDecrypted1, vDecrypted2;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted1, vKey, vDecrypted1) == true);
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted2, vKey, vDecrypted2) == true);
        REQUIRE(vDecrypted1 == vDecrypted2);
        REQUIRE(vDecrypted1 == vPlaintext);
    }

    SECTION("Authentication failure with wrong key")
    {
        std::string strPlaintext = "Secret message";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vKey(32, 0x11);

        /* Encrypt */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        REQUIRE(!vEncrypted.empty());

        /* Try to decrypt with wrong key */
        std::vector<uint8_t> vWrongKey(32, 0x99);
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vWrongKey, vDecrypted) == false);
    }

    SECTION("Authentication failure with tampered ciphertext")
    {
        std::string strPlaintext = "Important data";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vKey(32, 0xBB);

        /* Encrypt */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        REQUIRE(!vEncrypted.empty());

        /* Tamper with ciphertext portion (skip nonce, modify ciphertext) */
        if(vEncrypted.size() > 15)
            vEncrypted[15] ^= 0x01;

        /* Decryption should fail due to authentication */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted) == false);
    }

    SECTION("Authentication failure with tampered tag")
    {
        std::string strPlaintext = "Important data";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vKey(32, 0xCC);

        /* Encrypt */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        REQUIRE(!vEncrypted.empty());

        /* Tamper with tag (last byte) */
        vEncrypted[vEncrypted.size() - 1] ^= 0x01;

        /* Decryption should fail due to authentication */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted) == false);
    }

    SECTION("Invalid inputs - empty plaintext")
    {
        std::vector<uint8_t> vEmpty;
        std::vector<uint8_t> vKey(32, 0xFF);

        /* Should return empty vector on error */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vEmpty, vKey);
        REQUIRE(vEncrypted.empty());
    }

    SECTION("Invalid inputs - wrong key size")
    {
        std::vector<uint8_t> vPlaintext = {0x01, 0x02, 0x03};
        std::vector<uint8_t> vShortKey(16, 0xFF);  // Only 16 bytes (wrong!)

        /* Should return empty vector on error */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vShortKey);
        REQUIRE(vEncrypted.empty());
    }

    SECTION("Invalid inputs - encrypted payload too small")
    {
        std::vector<uint8_t> vKey(32, 0xFF);
        std::vector<uint8_t> vTooSmall = {0x01, 0x02, 0x03};  // Less than 28 bytes
        std::vector<uint8_t> vDecrypted;

        /* Should fail with payload too small */
        REQUIRE(LLC::DecryptPayloadChaCha20(vTooSmall, vKey, vDecrypted) == false);
    }

    SECTION("Mining protocol simulation - reward address encryption")
    {
        /* Simulate reward address encryption flow from stateless_miner.cpp */
        
        /* 1. Derive ChaCha20 key from genesis hash */
        uint256_t hashGenesis;
        hashGenesis.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

        std::string DOMAIN = "nexus-mining-chacha20-v1";
        std::vector<uint8_t> preimage;
        preimage.insert(preimage.end(), DOMAIN.begin(), DOMAIN.end());
        std::vector<uint8_t> genesis_bytes = hashGenesis.GetBytes();
        preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());
        
        uint256_t hashKey = LLC::SK256(preimage);
        std::vector<uint8_t> vSessionKey = hashKey.GetBytes();

        /* 2. Create reward address payload (32 bytes) */
        uint256_t hashRewardAddress;
        hashRewardAddress.SetHex("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210");
        std::vector<uint8_t> vRewardPayload = hashRewardAddress.GetBytes();

        /* 3. Encrypt with AAD for domain separation */
        std::vector<uint8_t> vAAD = {'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vRewardPayload, vSessionKey, vAAD);
        REQUIRE(!vEncrypted.empty());
        REQUIRE(vEncrypted.size() == 12 + 32 + 16);  // nonce + address + tag

        /* 4. Decrypt and verify */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vSessionKey, vDecrypted, vAAD) == true);
        REQUIRE(vDecrypted.size() == 32);

        /* 5. Reconstruct reward address */
        uint256_t hashDecryptedAddress;
        std::copy(vDecrypted.begin(), vDecrypted.end(), hashDecryptedAddress.begin());
        REQUIRE(hashDecryptedAddress == hashRewardAddress);
    }

    SECTION("Large payload encryption")
    {
        /* Test with a larger payload (1KB) */
        std::vector<uint8_t> vLargePlaintext(1024);
        for(size_t i = 0; i < 1024; ++i)
            vLargePlaintext[i] = static_cast<uint8_t>(i & 0xFF);

        std::vector<uint8_t> vKey(32, 0xDD);

        /* Encrypt */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vLargePlaintext, vKey);
        REQUIRE(!vEncrypted.empty());
        REQUIRE(vEncrypted.size() == 12 + 1024 + 16);

        /* Decrypt */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted) == true);
        REQUIRE(vDecrypted == vLargePlaintext);
    }

    SECTION("Cross-compatibility with low-level functions")
    {
        /* Verify helpers are compatible with direct LLC::EncryptChaCha20Poly1305 calls */
        std::string strPlaintext = "Cross-compatibility test";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vKey(32, 0xEE);

        /* Encrypt using helper */
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPlaintext, vKey);
        REQUIRE(!vEncrypted.empty());

        /* Manually extract components */
        std::vector<uint8_t> vNonce(vEncrypted.begin(), vEncrypted.begin() + 12);
        std::vector<uint8_t> vTag(vEncrypted.end() - 16, vEncrypted.end());
        std::vector<uint8_t> vCiphertext(vEncrypted.begin() + 12, vEncrypted.end() - 16);

        /* Decrypt using low-level function */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vDecrypted) == true);
        REQUIRE(vDecrypted == vPlaintext);
    }
}
