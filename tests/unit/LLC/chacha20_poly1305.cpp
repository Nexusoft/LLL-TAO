/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/encrypt.h>
#include <LLC/hash/SK.h>
#include <LLC/types/uint1024.h>

#include <unit/catch2/catch.hpp>

#include <vector>
#include <string>
#include <cstring>


TEST_CASE("ChaCha20-Poly1305 AEAD Encryption/Decryption Tests", "[LLC]")
{
    SECTION("Basic encryption and decryption")
    {
        /* Test data */
        std::string strPlaintext = "The quick brown fox jumps over the lazy dog";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        /* 256-bit key (32 bytes) */
        std::vector<uint8_t> vKey = {
            0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
            0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f
        };

        /* 96-bit nonce (12 bytes) */
        std::vector<uint8_t> vNonce = {
            0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
        };

        /* Encrypt */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag) == true);

        /* Verify ciphertext size */
        REQUIRE(vCiphertext.size() == vPlaintext.size());

        /* Verify tag size (128 bits = 16 bytes) */
        REQUIRE(vTag.size() == 16);

        /* Decrypt */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vDecrypted) == true);

        /* Verify decrypted matches original */
        REQUIRE(vDecrypted.size() == vPlaintext.size());
        REQUIRE(memcmp(vDecrypted.data(), vPlaintext.data(), vPlaintext.size()) == 0);
    }

    SECTION("Encryption with AAD (Additional Authenticated Data)")
    {
        std::string strPlaintext = "Hello, World!";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        std::vector<uint8_t> vKey(32, 0x42);  // All 0x42
        std::vector<uint8_t> vNonce(12, 0x01);  // All 0x01

        /* AAD: metadata that is authenticated but not encrypted */
        std::string strAAD = "FALCON_PUBKEY";
        std::vector<uint8_t> vAAD(strAAD.begin(), strAAD.end());

        /* Encrypt with AAD */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag, vAAD) == true);

        /* Decrypt with matching AAD */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vDecrypted, vAAD) == true);
        REQUIRE(memcmp(vDecrypted.data(), vPlaintext.data(), vPlaintext.size()) == 0);

        /* Decrypt with WRONG AAD should fail */
        std::vector<uint8_t> vWrongAAD(5, 0xFF);
        std::vector<uint8_t> vDecrypted2;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vDecrypted2, vWrongAAD) == false);
    }

    SECTION("Authentication failure with wrong key")
    {
        std::string strPlaintext = "Secret message";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        std::vector<uint8_t> vKey(32, 0x11);
        std::vector<uint8_t> vNonce(12, 0x22);

        /* Encrypt */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag) == true);

        /* Try to decrypt with wrong key */
        std::vector<uint8_t> vWrongKey(32, 0x99);
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vWrongKey, vNonce, vDecrypted) == false);
    }

    SECTION("Authentication failure with tampered ciphertext")
    {
        std::string strPlaintext = "Important data";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());

        std::vector<uint8_t> vKey(32, 0xAA);
        std::vector<uint8_t> vNonce(12, 0xBB);

        /* Encrypt */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag) == true);

        /* Tamper with ciphertext */
        if(!vCiphertext.empty())
            vCiphertext[0] ^= 0x01;

        /* Decryption should fail due to authentication */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vDecrypted) == false);
    }

    SECTION("Genesis hash key derivation + ChaCha20")
    {
        /* Simulate deriving a key from genesis hash (like in stateless_miner.cpp) */
        uint256_t hashGenesis;
        hashGenesis.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

        /* Derive key using domain separation */
        std::string DOMAIN = "nexus-mining-chacha20-v1";
        std::vector<uint8_t> preimage;
        preimage.insert(preimage.end(), DOMAIN.begin(), DOMAIN.end());
        std::vector<uint8_t> genesis_bytes = hashGenesis.GetBytes();
        preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());
        
        uint256_t hashKey = LLC::SK256(preimage);
        std::vector<uint8_t> vSessionKey = hashKey.GetBytes();

        /* Verify key is 32 bytes */
        REQUIRE(vSessionKey.size() == 32);

        /* Use derived key for encryption */
        std::string strPlaintext = "Falcon public key data";
        std::vector<uint8_t> vPlaintext(strPlaintext.begin(), strPlaintext.end());
        std::vector<uint8_t> vNonce(12, 0x00);
        std::vector<uint8_t> vAAD = {'F','A','L','C','O','N','_','P','U','B','K','E','Y'};

        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vSessionKey, vNonce, vCiphertext, vTag, vAAD) == true);

        /* Decrypt with same derived key */
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vSessionKey, vNonce, vDecrypted, vAAD) == true);
        REQUIRE(memcmp(vDecrypted.data(), vPlaintext.data(), vPlaintext.size()) == 0);
    }

    SECTION("Invalid key size")
    {
        std::vector<uint8_t> vPlaintext = {0x01, 0x02, 0x03};
        std::vector<uint8_t> vShortKey(16, 0xFF);  // Only 16 bytes (wrong!)
        std::vector<uint8_t> vNonce(12, 0x00);
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;

        /* Should fail with wrong key size */
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vShortKey, vNonce, vCiphertext, vTag) == false);
    }

    SECTION("Invalid nonce size")
    {
        std::vector<uint8_t> vPlaintext = {0x01, 0x02, 0x03};
        std::vector<uint8_t> vKey(32, 0xFF);
        std::vector<uint8_t> vShortNonce(8, 0x00);  // Only 8 bytes (wrong!)
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;

        /* Should fail with wrong nonce size */
        REQUIRE(LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vShortNonce, vCiphertext, vTag) == false);
    }
}
