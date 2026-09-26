/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/mining_session_keys.h>
#include <LLC/types/uint1024.h>
#include <Util/include/hex.h>

#include <openssl/sha.h>

using namespace LLC;
using namespace LLC::MiningSessionKeys;


TEST_CASE("Genesis Hash Endianness Verification", "[genesis_endianness]")
{
    SECTION("Verify GetHex() and GetBytes() consistency")
    {
        /* Use the exact genesis from the problem statement */
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Get hex representation */
        std::string hex = hashGenesis.GetHex();
        REQUIRE(hex == "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Get bytes using GetBytes() - this is what DeriveChaCha20Key ORIGINALLY used */
        std::vector<uint8_t> vBytesViaGetBytes = hashGenesis.GetBytes();

        /* Get bytes using GetHex() + ParseHex() - this is what the FIX uses */
        std::vector<uint8_t> vBytesViaGetHex = ParseHex(hex);

        /* Print first 8 bytes of each for debugging */
        INFO("GetBytes() first 8 bytes: "
             << HexStr(vBytesViaGetBytes.begin(), vBytesViaGetBytes.begin() + 8));
        INFO("GetHex()->ParseHex() first 8 bytes: "
             << HexStr(vBytesViaGetHex.begin(), vBytesViaGetHex.begin() + 8));

        /* These should be DIFFERENT due to endianness mismatch in GetBytes() */
        /* GetBytes() gives little-endian word order, GetHex()+ParseHex() gives big-endian */
        REQUIRE(vBytesViaGetBytes.size() == 32);
        REQUIRE(vBytesViaGetHex.size() == 32);
    }

    SECTION("Verify key derivation matches expected values from problem statement")
    {
        /* Genesis from problem statement */
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Derive key using the FIXED implementation (GetHex + ParseHex) */
        std::vector<uint8_t> vDerivedKey = DeriveChaCha20Key(hashGenesis);

        /* Expected key from miner side (Image 5): 4f2ad19bf5c32976593805418ac2333e8dcb1ae1ee2dee38906d1d6a19e2d28a */
        std::string expectedKeyHex = "4f2ad19bf5c32976593805418ac2333e8dcb1ae1ee2dee38906d1d6a19e2d28a";
        std::vector<uint8_t> vExpectedKey = ParseHex(expectedKeyHex);

        /* Print derived key for debugging */
        INFO("Derived key: " << HexStr(vDerivedKey));
        INFO("Expected key: " << expectedKeyHex);

        /* Verify the key matches */
        REQUIRE(vDerivedKey == vExpectedKey);

        /* Verify first 8 bytes match miner side */
        std::vector<uint8_t> vFirst8(vDerivedKey.begin(), vDerivedKey.begin() + 8);
        std::string first8Hex = HexStr(vFirst8);
        INFO("First 8 bytes of derived key: " << first8Hex);

        /* Miner side shows: 4f2ad19b f5c32976 */
        REQUIRE(first8Hex == "4f2ad19bf5c32976");
    }

    SECTION("Verify GetBytes() produces REVERSED byte order compared to GetHex()")
    {
        /* Simple test case */
        uint256_t hash;
        hash.SetHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

        std::string hex = hash.GetHex();
        std::vector<uint8_t> vBytesGetBytes = hash.GetBytes();
        std::vector<uint8_t> vBytesParseHex = ParseHex(hex);

        /* Verify they are different (due to endianness) */
        INFO("GetBytes(): " << HexStr(vBytesGetBytes));
        INFO("ParseHex(GetHex()): " << HexStr(vBytesParseHex));

        /* GetBytes() should produce reversed 32-bit word order */
        /* For 256-bit = 8 words of 32-bit each */
        /* GetHex() outputs big-endian (word 7, 6, 5, 4, 3, 2, 1, 0) */
        /* GetBytes() outputs little-endian (word 0, 1, 2, 3, 4, 5, 6, 7) */

        /* Verify that reversing word order makes them equal */
        std::vector<uint8_t> vReversedWords;
        for(int i = 7; i >= 0; --i) {
            vReversedWords.insert(vReversedWords.end(),
                                  vBytesGetBytes.begin() + i*4,
                                  vBytesGetBytes.begin() + (i+1)*4);
        }

        REQUIRE(vReversedWords == vBytesParseHex);
    }
}


TEST_CASE("DeriveChaCha20Key Endianness Fix Validation", "[genesis_endianness]")
{
    SECTION("Key derivation uses big-endian byte representation via GetHex()")
    {
        uint256_t hashGenesis;
        hashGenesis.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");

        /* Manually derive key using the correct method */
        std::vector<uint8_t> preimage;
        preimage.reserve(DOMAIN_CHACHA20.size() + 32);
        preimage.insert(preimage.end(), DOMAIN_CHACHA20.begin(), DOMAIN_CHACHA20.end());

        /* Use GetHex() + ParseHex() for big-endian representation */
        std::string genesis_hex = hashGenesis.GetHex();
        std::vector<uint8_t> genesis_bytes = ParseHex(genesis_hex);
        preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());

        /* Hash using OpenSSL SHA256 */
        std::vector<uint8_t> vKeyManual(32);
        SHA256(preimage.data(), preimage.size(), vKeyManual.data());

        /* Compare with helper function result */
        std::vector<uint8_t> vKeyHelper = DeriveChaCha20Key(hashGenesis);

        REQUIRE(vKeyManual == vKeyHelper);
    }
}
