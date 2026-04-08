/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/mining_session_keys.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <Util/include/hex.h>

#include <openssl/sha.h>

using namespace LLC;
using namespace LLC::MiningSessionKeys;


TEST_CASE("DeriveChaCha20Key Tests", "[mining_session_keys]")
{
    SECTION("Deterministic derivation - same genesis produces same key")
    {
        /* Create a test genesis hash */
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        /* Derive key twice */
        std::vector<uint8_t> vKey1 = DeriveChaCha20Key(hashGenesis);
        std::vector<uint8_t> vKey2 = DeriveChaCha20Key(hashGenesis);
        
        /* Keys should be identical */
        REQUIRE(vKey1 == vKey2);
        
        /* Keys should be 32 bytes (256 bits) */
        REQUIRE(vKey1.size() == 32);
        REQUIRE(vKey2.size() == 32);
    }
    
    SECTION("Different genesis hashes produce different keys")
    {
        /* Create two different genesis hashes */
        uint256_t hashGenesis1;
        hashGenesis1.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        uint256_t hashGenesis2;
        hashGenesis2.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        /* Derive keys */
        std::vector<uint8_t> vKey1 = DeriveChaCha20Key(hashGenesis1);
        std::vector<uint8_t> vKey2 = DeriveChaCha20Key(hashGenesis2);
        
        /* Keys should be different */
        REQUIRE(vKey1 != vKey2);
    }
    
    SECTION("Key is not all zeros for non-zero genesis")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);
        
        /* Check that key is not all zeros */
        bool fAllZeros = true;
        for(size_t i = 0; i < vKey.size(); ++i)
        {
            if(vKey[i] != 0)
            {
                fAllZeros = false;
                break;
            }
        }
        
        REQUIRE_FALSE(fAllZeros);
    }
    
    SECTION("Domain separation - uses correct domain string")
    {
        /* This test verifies that the domain string is properly included
         * by checking that the result is different from hashing genesis alone */
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        /* Get the key using the helper */
        std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);
        
        /* Hash genesis alone (without domain) using OpenSSL SHA256 */
        std::string genesis_hex = hashGenesis.GetHex();
        std::vector<uint8_t> genesis_bytes = ParseHex(genesis_hex);
        std::vector<uint8_t> vKeyNoDomain(32);
        SHA256(genesis_bytes.data(), genesis_bytes.size(), vKeyNoDomain.data());
        
        /* Keys should be different (proving domain string is included) */
        REQUIRE(vKey != vKeyNoDomain);
    }
}


/* Suppress deprecation warnings for testing DeriveFalconSessionId (scheduled for removal) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

TEST_CASE("DeriveFalconSessionId Tests", "[mining_session_keys]")
{
    SECTION("Deterministic derivation - same inputs produce same session ID")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vPubKey(897, 0xAB);  // Falcon-512 pubkey size
        uint64_t nTimestamp = 1609459200;  // 2021-01-01 00:00:00 UTC
        
        /* Derive session ID twice */
        uint256_t sessionId1 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp);
        uint256_t sessionId2 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp);
        
        /* Session IDs should be identical */
        REQUIRE(sessionId1 == sessionId2);
    }
    
    SECTION("Different genesis produces different session ID")
    {
        uint256_t hashGenesis1;
        hashGenesis1.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        uint256_t hashGenesis2;
        hashGenesis2.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        std::vector<uint8_t> vPubKey(897, 0xAB);
        uint64_t nTimestamp = 1609459200;
        
        uint256_t sessionId1 = DeriveFalconSessionId(hashGenesis1, vPubKey, nTimestamp);
        uint256_t sessionId2 = DeriveFalconSessionId(hashGenesis2, vPubKey, nTimestamp);
        
        REQUIRE(sessionId1 != sessionId2);
    }
    
    SECTION("Different public key produces different session ID")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vPubKey1(897, 0xAB);
        std::vector<uint8_t> vPubKey2(897, 0xCD);
        uint64_t nTimestamp = 1609459200;
        
        uint256_t sessionId1 = DeriveFalconSessionId(hashGenesis, vPubKey1, nTimestamp);
        uint256_t sessionId2 = DeriveFalconSessionId(hashGenesis, vPubKey2, nTimestamp);
        
        REQUIRE(sessionId1 != sessionId2);
    }
    
    SECTION("Timestamp rounding - same day produces same session ID")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vPubKey(897, 0xAB);
        
        /* Timestamps within same day (86400 seconds) */
        uint64_t nTimestamp1 = 1609459200;          // 2021-01-01 00:00:00
        uint64_t nTimestamp2 = 1609459200 + 43200;  // 2021-01-01 12:00:00
        uint64_t nTimestamp3 = 1609459200 + 86399;  // 2021-01-01 23:59:59
        
        uint256_t sessionId1 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp1);
        uint256_t sessionId2 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp2);
        uint256_t sessionId3 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp3);
        
        /* All should be the same (rounded to same day) */
        REQUIRE(sessionId1 == sessionId2);
        REQUIRE(sessionId2 == sessionId3);
    }
    
    SECTION("Different days produce different session IDs")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vPubKey(897, 0xAB);
        
        /* Timestamps in different days */
        uint64_t nTimestamp1 = 1609459200;      // 2021-01-01 00:00:00
        uint64_t nTimestamp2 = 1609545600;      // 2021-01-02 00:00:00
        
        uint256_t sessionId1 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp1);
        uint256_t sessionId2 = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp2);
        
        REQUIRE(sessionId1 != sessionId2);
    }
}

#pragma GCC diagnostic pop


TEST_CASE("DeriveKeyId Tests", "[mining_session_keys]")
{
    SECTION("Deterministic derivation - same pubkey produces same key ID")
    {
        std::vector<uint8_t> vPubKey(897, 0xAB);  // Falcon-512 pubkey size
        
        uint256_t keyId1 = DeriveKeyId(vPubKey);
        uint256_t keyId2 = DeriveKeyId(vPubKey);
        
        REQUIRE(keyId1 == keyId2);
    }
    
    SECTION("Different pubkeys produce different key IDs")
    {
        std::vector<uint8_t> vPubKey1(897, 0xAB);
        std::vector<uint8_t> vPubKey2(897, 0xCD);
        
        uint256_t keyId1 = DeriveKeyId(vPubKey1);
        uint256_t keyId2 = DeriveKeyId(vPubKey2);
        
        REQUIRE(keyId1 != keyId2);
    }
    
    SECTION("Small change in pubkey produces different key ID")
    {
        std::vector<uint8_t> vPubKey1(897, 0xAB);
        std::vector<uint8_t> vPubKey2(897, 0xAB);
        vPubKey2[0] = 0xAC;  // Change one byte
        
        uint256_t keyId1 = DeriveKeyId(vPubKey1);
        uint256_t keyId2 = DeriveKeyId(vPubKey2);
        
        REQUIRE(keyId1 != keyId2);
    }
    
    SECTION("Key ID is not zero for non-zero pubkey")
    {
        std::vector<uint8_t> vPubKey(897, 0xAB);
        
        uint256_t keyId = DeriveKeyId(vPubKey);
        
        REQUIRE(keyId != 0);
    }
    
    SECTION("Uses SK256 hash function")
    {
        std::vector<uint8_t> vPubKey(897, 0xAB);
        
        /* Derive using helper */
        uint256_t keyId = DeriveKeyId(vPubKey);
        
        /* Derive directly with SK256 */
        uint256_t keyIdDirect = LLC::SK256(vPubKey);
        
        /* Should be identical (proves SK256 is used) */
        REQUIRE(keyId == keyIdDirect);
    }
}


/* Suppress deprecation warnings for testing DeriveFalconSessionId (scheduled for removal) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

TEST_CASE("Integration Tests - All Functions Work Together", "[mining_session_keys]")
{
    SECTION("Complete mining session workflow")
    {
        /* Setup test data */
        uint256_t hashGenesis;
        hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
        
        std::vector<uint8_t> vPubKey(897, 0xAB);
        uint64_t nTimestamp = 1609459200;
        
        /* Derive all key types */
        std::vector<uint8_t> vChaChaKey = DeriveChaCha20Key(hashGenesis);
        uint256_t sessionId = DeriveFalconSessionId(hashGenesis, vPubKey, nTimestamp);
        uint256_t keyId = DeriveKeyId(vPubKey);
        
        /* Verify all are valid */
        REQUIRE(vChaChaKey.size() == 32);
        REQUIRE(sessionId != 0);
        REQUIRE(keyId != 0);
        
        /* Verify they are all different from each other */
        uint256_t chachaAsUint256;
        chachaAsUint256.SetBytes(vChaChaKey);
        
        REQUIRE(chachaAsUint256 != sessionId);
        REQUIRE(chachaAsUint256 != keyId);
        REQUIRE(sessionId != keyId);
    }
}

#pragma GCC diagnostic pop
