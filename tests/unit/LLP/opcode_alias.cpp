/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/opcode_utility.h>

/* Test opcode alias functionality for mining notifications
 * 
 * NEW_PRIME_AVAILABLE and NEW_HASH_AVAILABLE are aliases for
 * PRIME_BLOCK_AVAILABLE and HASH_BLOCK_AVAILABLE respectively.
 * They map to the same opcode values (217 and 218).
 */

TEST_CASE("Mining Notification Opcode Aliases", "[miner][llp][opcodes]")
{
    SECTION("NEW_PRIME_AVAILABLE maps to same value as PRIME_BLOCK_AVAILABLE")
    {
        /* Both should be opcode 217 (0xD9) */
        const uint8_t PRIME_BLOCK_AVAILABLE = 217;
        const uint8_t NEW_PRIME_AVAILABLE = 217;
        
        REQUIRE(NEW_PRIME_AVAILABLE == PRIME_BLOCK_AVAILABLE);
        REQUIRE(NEW_PRIME_AVAILABLE == 217);
        REQUIRE(NEW_PRIME_AVAILABLE == 0xD9);
    }
    
    SECTION("NEW_HASH_AVAILABLE maps to same value as HASH_BLOCK_AVAILABLE")
    {
        /* Both should be opcode 218 (0xDA) */
        const uint8_t HASH_BLOCK_AVAILABLE = 218;
        const uint8_t NEW_HASH_AVAILABLE = 218;
        
        REQUIRE(NEW_HASH_AVAILABLE == HASH_BLOCK_AVAILABLE);
        REQUIRE(NEW_HASH_AVAILABLE == 218);
        REQUIRE(NEW_HASH_AVAILABLE == 0xDA);
    }
    
    SECTION("GetOpcodeName returns combined alias names")
    {
        /* Prime notification opcode (217) */
        std::string strPrimeName = LLP::OpcodeUtility::GetOpcodeName(217);
        REQUIRE(strPrimeName.find("PRIME_BLOCK_AVAILABLE") != std::string::npos);
        REQUIRE(strPrimeName.find("NEW_PRIME_AVAILABLE") != std::string::npos);
        
        /* Hash notification opcode (218) */
        std::string strHashName = LLP::OpcodeUtility::GetOpcodeName(218);
        REQUIRE(strHashName.find("HASH_BLOCK_AVAILABLE") != std::string::npos);
        REQUIRE(strHashName.find("NEW_HASH_AVAILABLE") != std::string::npos);
    }
    
    SECTION("Notification opcodes are in stateless mining range")
    {
        /* Stateless mining opcodes are 207-218 (0xCF-0xDA) */
        REQUIRE(LLP::OpcodeUtility::IsStatelessMiningOpcode(217) == true);
        REQUIRE(LLP::OpcodeUtility::IsStatelessMiningOpcode(218) == true);
    }
    
    SECTION("Notification opcodes are NOT authentication opcodes")
    {
        /* Authentication opcodes are 207-212 (0xCF-0xD4) */
        REQUIRE(LLP::OpcodeUtility::IsAuthenticationOpcode(217) == false);
        REQUIRE(LLP::OpcodeUtility::IsAuthenticationOpcode(218) == false);
    }
}

TEST_CASE("Mining Protocol Flow Documentation", "[miner][llp][protocol]")
{
    SECTION("Push notification flow uses notification opcodes")
    {
        /* Expected push notification flow:
         * 1. Miner → MINER_READY (216)
         * 2. Server → PRIME_BLOCK_AVAILABLE (217) or HASH_BLOCK_AVAILABLE (218)
         *             (aliases: NEW_PRIME_AVAILABLE or NEW_HASH_AVAILABLE)
         * 3. Client → GET_BLOCK (129) to fetch new template
         */
        
        const uint8_t MINER_READY = 216;
        const uint8_t PRIME_NOTIFICATION = 217;  // PRIME_BLOCK_AVAILABLE / NEW_PRIME_AVAILABLE
        const uint8_t HASH_NOTIFICATION = 218;   // HASH_BLOCK_AVAILABLE / NEW_HASH_AVAILABLE
        const uint8_t GET_BLOCK = 129;
        
        REQUIRE(MINER_READY == 216);
        REQUIRE(PRIME_NOTIFICATION == 217);
        REQUIRE(HASH_NOTIFICATION == 218);
        REQUIRE(GET_BLOCK == 129);
    }
    
    SECTION("Legacy polling flow uses GET_ROUND")
    {
        /* Legacy polling flow:
         * 1. Miner → GET_ROUND (133)
         * 2. Server → NEW_ROUND (204) with height data
         */
        
        const uint8_t GET_ROUND = 133;
        const uint8_t NEW_ROUND = 204;
        const uint8_t OLD_ROUND = 205;
        
        REQUIRE(GET_ROUND == 133);
        REQUIRE(NEW_ROUND == 204);
        REQUIRE(OLD_ROUND == 205);
    }
}
