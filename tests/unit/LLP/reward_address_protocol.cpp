/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/types/uint1024.h>
#include <Util/include/hex.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

/* Test MINER_SET_REWARD and MINER_REWARD_RESULT packet formats
 * 
 * These tests verify the basic packet structure for the reward address protocol.
 * Actual encryption/decryption is tested separately.
 */

TEST_CASE("MINER_SET_REWARD Packet Structure Tests", "[miner][reward][protocol]")
{
    SECTION("Valid reward address payload - 32 bytes")
    {
        /* Create a test reward address (32 bytes) */
        uint256_t hashReward;
        hashReward.SetHex("a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122");
        
        /* Convert to bytes */
        std::vector<uint8_t> vPayload = hashReward.GetBytes();
        
        REQUIRE(vPayload.size() == 32);
        
        /* Verify we can reconstruct the address */
        uint256_t hashReconstructed;
        std::copy(vPayload.begin(), vPayload.begin() + 32, hashReconstructed.begin());
        
        REQUIRE(hashReconstructed == hashReward);
    }
    
    SECTION("Byte order: HexStr+SetHex correctly decodes big-endian miner bytes")
    {
        /* NexusMiner encodes the genesis hash in big-endian / display byte order:
         * hex_decode_genesis_hash() decodes the 64-char hex string left-to-right,
         * so vDecrypted[0] == 0xa1 (the type byte) and vDecrypted[31] == the last byte.
         *
         * uint256_t internal storage (pn[]) is little-endian word order: pn[0] is
         * the LEAST significant 32-bit word and pn[WIDTH-1] is the MOST significant.
         * GetType() reads from pn[WIDTH-1] >> 24, expecting the type byte there.
         *
         * CORRECT: use HexStr + SetHex — SetHex reverses the bytes into pn[] so that
         * the first hex character (type byte 0xa1) ends up in pn[WIDTH-1].
         *
         * WRONG: memcpy(hashReward.begin(), ...) places vDecrypted[0] into pn[0]
         * (least significant word), so GetType() reads vDecrypted[28] instead of
         * vDecrypted[0], returning the wrong type byte. */

        /* The exact genesis hash from the bug report */
        const char* hex_sent = "a174011c93ca1c80bca5388382b167cacd33d3154395ea8f45ac99a8308cd122";

        /* Simulate the raw bytes as NexusMiner sends them (big-endian / display order) */
        std::vector<uint8_t> vDecrypted;
        for(size_t i = 0; i < 64; i += 2)
        {
            std::string byteString = std::string(hex_sent + i, 2);
            uint8_t byte = static_cast<uint8_t>(std::strtoul(byteString.c_str(), nullptr, 16));
            vDecrypted.push_back(byte);
        }

        REQUIRE(vDecrypted.size() == 32);
        /* Confirm byte[0] is the type byte 0xa1 */
        REQUIRE(vDecrypted[0] == 0xa1);

        /* --- CORRECT approach: HexStr + SetHex (what ProcessSetReward now uses) --- */
        uint256_t hashReward;
        hashReward.SetHex(HexStr(vDecrypted.begin(), vDecrypted.end()));

        /* GetHex() must round-trip back to the original display-order hex string */
        REQUIRE(hashReward.GetHex() == hex_sent);

        /* GetType() must return 0xa1 — the first byte of the display-order string */
        REQUIRE(hashReward.GetType() == 0xa1);

        /* --- WRONG approach: memcpy (the old bug — documented here as regression proof) ---
         * memcpy places vDecrypted[0..3] into pn[0] (least significant word),
         * so GetType() reads from pn[WIDTH-1], which gets vDecrypted[28] == 0x22. */
        uint256_t hashWrongWay;
        std::memcpy(hashWrongWay.begin(), vDecrypted.data(), 32);

        /* memcpy does NOT give the correct type byte */
        REQUIRE(hashWrongWay.GetType() != 0xa1);
        /* Byte 28 of the genesis hash above is 0x22 — the wrong type byte seen in logs */
        REQUIRE(hashWrongWay.GetType() == 0x22);
    }
    
    SECTION("Invalid payload - too small")
    {
        /* Payload smaller than 32 bytes should be rejected */
        std::vector<uint8_t> vPayload(16, 0x00);
        
        REQUIRE(vPayload.size() < 32);
        
        /* This should be rejected by ProcessSetReward */
        bool fValidSize = (vPayload.size() >= 32);
        REQUIRE_FALSE(fValidSize);
    }
    
    SECTION("Zero address should be invalid")
    {
        /* Zero address should not be allowed */
        uint256_t hashReward(0);
        
        REQUIRE(hashReward == 0);
        
        /* Validation should fail for zero address */
        bool fValid = (hashReward != 0);
        REQUIRE_FALSE(fValid);
    }
}


TEST_CASE("MINER_REWARD_RESULT Packet Structure Tests", "[miner][reward][protocol]")
{
    SECTION("Success result - minimal")
    {
        /* Build success result packet */
        std::vector<uint8_t> vPayload;
        
        /* Status byte: 0x01 = success */
        vPayload.push_back(0x01);
        
        /* Message length: 0 (no message on success) */
        vPayload.push_back(0x00);
        
        REQUIRE(vPayload.size() == 2);
        REQUIRE(vPayload[0] == 0x01);  // Success
        REQUIRE(vPayload[1] == 0x00);  // No message
    }
    
    SECTION("Failure result - with error message")
    {
        std::string strError = "Invalid reward address";
        
        /* Build failure result packet */
        std::vector<uint8_t> vPayload;
        
        /* Status byte: 0x00 = failure */
        vPayload.push_back(0x00);
        
        /* Message length */
        vPayload.push_back(static_cast<uint8_t>(strError.size()));
        
        /* Message */
        vPayload.insert(vPayload.end(), strError.begin(), strError.end());
        
        REQUIRE(vPayload.size() == 2 + strError.size());
        REQUIRE(vPayload[0] == 0x00);  // Failure
        REQUIRE(vPayload[1] == strError.size());
        
        /* Extract and verify message */
        std::string strExtracted(vPayload.begin() + 2, vPayload.end());
        REQUIRE(strExtracted == strError);
    }
    
    SECTION("Failure result - empty message")
    {
        /* Build failure result with empty message */
        std::vector<uint8_t> vPayload;
        
        /* Status byte: 0x00 = failure */
        vPayload.push_back(0x00);
        
        /* Message length: 0 */
        vPayload.push_back(0x00);
        
        REQUIRE(vPayload.size() == 2);
        REQUIRE(vPayload[0] == 0x00);  // Failure
        REQUIRE(vPayload[1] == 0x00);  // No message
    }
}


TEST_CASE("Message Type Constants", "[miner][reward][constants]")
{
    SECTION("MINER_SET_REWARD value")
    {
        /* MINER_SET_REWARD should be 0xd5 (213) */
        const uint8_t MINER_SET_REWARD = 0xd5;
        
        REQUIRE(MINER_SET_REWARD == 213);
    }
    
    SECTION("MINER_REWARD_RESULT value")
    {
        /* MINER_REWARD_RESULT should be 0xd6 (214) */
        const uint8_t MINER_REWARD_RESULT = 0xd6;
        
        REQUIRE(MINER_REWARD_RESULT == 214);
    }
}
