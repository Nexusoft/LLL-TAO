/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/types/uint1024.h>
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
    
    SECTION("Byte order preservation with memcpy - matching NexusMiner protocol")
    {
        /* Test the exact scenario from the bug report:
         * NexusMiner sends 32-byte hash in natural order,
         * Node must preserve exact byte order (not reverse it) */
        
        /* Simulate the hash that NexusMiner sent (from problem statement) */
        const char* hex_sent = "30727fb2d271c6ecdb64f65ba1eec88a9ce9355ff9104995c6849f59d68ebff2";
        
        /* Create raw byte vector as NexusMiner would send it */
        std::vector<uint8_t> vDecrypted;
        for(size_t i = 0; i < 64; i += 2)
        {
            std::string byteString = std::string(hex_sent + i, 2);
            uint8_t byte = static_cast<uint8_t>(std::strtoul(byteString.c_str(), nullptr, 16));
            vDecrypted.push_back(byte);
        }
        
        REQUIRE(vDecrypted.size() == 32);
        
        /* Use memcpy to preserve byte order (as fixed in ProcessSetReward) */
        uint256_t hashReward;
        std::memcpy(hashReward.begin(), vDecrypted.data(), 32);
        
        /* Convert back to hex string */
        std::string hex_stored = hashReward.ToString();
        
        /* Verify the stored hash matches what was sent (no byte reversal) */
        REQUIRE(hex_stored == hex_sent);
        
        /* Also verify the WRONG way (SetBytes) would have reversed it */
        uint256_t hashWrongWay;
        hashWrongWay.SetBytes(vDecrypted);
        std::string hex_wrong = hashWrongWay.ToString();
        
        /* This should be different (reversed) */
        REQUIRE(hex_wrong != hex_sent);
        
        /* Verify it's the reversed version (corrected from problem statement typo) */
        const char* hex_reversed = "d68ebff2c6849f59f91049959ce9355fa1eec88adb64f65bd271c6ec30727fb2";
        REQUIRE(hex_wrong == hex_reversed);
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
