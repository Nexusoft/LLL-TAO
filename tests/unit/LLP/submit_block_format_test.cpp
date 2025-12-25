/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_constants.h>
#include <LLP/packets/packet.h>
#include <Util/include/convert.h>

using namespace LLP;

TEST_CASE("SUBMIT_BLOCK Full Block Format Detection", "[submit_block]")
{
    SECTION("Legacy 64-byte merkle format is correctly detected (72 bytes)")
    {
        /* Create a 72-byte packet: merkle(64) + nonce(8) */
        std::vector<uint8_t> data;
        
        /* Merkle root (64 bytes) */
        for(size_t i = 0; i < FalconConstants::MERKLE_ROOT_SIZE; ++i)
            data.push_back(0xAA);
        
        /* Nonce (8 bytes) */
        uint64_t nonce = 0x123456789ABCDEF0;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(nonce);
        data.insert(data.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Verify size */
        REQUIRE(data.size() == 72);
        
        /* This should be detected as legacy format */
        bool fFullBlockFormat = (data.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD);
        REQUIRE(fFullBlockFormat == false);
    }
    
    SECTION("Full Tritium block format is correctly detected (216 bytes)")
    {
        /* Create a 216-byte packet simulating a full Tritium block */
        std::vector<uint8_t> data;
        
        /* nVersion (4 bytes) */
        for(size_t i = 0; i < 4; ++i)
            data.push_back(0x00);
        
        /* hashPrevBlock (128 bytes) */
        for(size_t i = 0; i < 128; ++i)
            data.push_back(0xBB);
        
        /* hashMerkleRoot at offset 132 (64 bytes) */
        REQUIRE(data.size() == 132);
        for(size_t i = 0; i < FalconConstants::MERKLE_ROOT_SIZE; ++i)
            data.push_back(0xCC);
        
        /* Additional fields to reach nonce at offset 200 */
        size_t currentOffset = data.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill to offset 200 (4 bytes) */
        for(size_t i = currentOffset; i < 200; ++i)
            data.push_back(0xDD);
        
        /* nNonce at offset 200 (8 bytes) */
        REQUIRE(data.size() == 200);
        uint64_t nonce = 0xFEDCBA9876543210;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(nonce);
        data.insert(data.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to 216 bytes */
        while(data.size() < 216)
            data.push_back(0xEE);
        
        REQUIRE(data.size() == 216);
        
        /* This should be detected as full block format */
        bool fFullBlockFormat = (data.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD);
        REQUIRE(fFullBlockFormat == true);
        
        /* Verify merkle root extraction at offset 132 */
        uint512_t hashMerkle;
        hashMerkle.SetBytes(std::vector<uint8_t>(
            data.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
            data.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE));
        
        /* Verify nonce extraction at offset 200 */
        uint64_t extractedNonce = convert::bytes2uint64(std::vector<uint8_t>(
            data.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET,
            data.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE));
        
        REQUIRE(extractedNonce == nonce);
    }
    
    SECTION("Full Legacy block format is correctly detected (220 bytes)")
    {
        /* Create a 220-byte packet simulating a full Legacy block */
        std::vector<uint8_t> data;
        
        /* nVersion (4 bytes) */
        for(size_t i = 0; i < 4; ++i)
            data.push_back(0x00);
        
        /* hashPrevBlock (128 bytes) */
        for(size_t i = 0; i < 128; ++i)
            data.push_back(0xBB);
        
        /* hashMerkleRoot at offset 132 (64 bytes) */
        REQUIRE(data.size() == 132);
        for(size_t i = 0; i < FalconConstants::MERKLE_ROOT_SIZE; ++i)
            data.push_back(0xCC);
        
        /* Additional fields to reach nonce at offset 204 */
        size_t currentOffset = data.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill to offset 204 (8 bytes) */
        for(size_t i = currentOffset; i < 204; ++i)
            data.push_back(0xDD);
        
        /* nNonce at offset 204 (8 bytes) */
        REQUIRE(data.size() == 204);
        uint64_t nonce = 0x1122334455667788;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(nonce);
        data.insert(data.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to 220 bytes */
        while(data.size() < 220)
            data.push_back(0xEE);
        
        REQUIRE(data.size() == 220);
        
        /* This should be detected as full block format */
        bool fFullBlockFormat = (data.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD);
        REQUIRE(fFullBlockFormat == true);
        
        /* Verify nonce extraction at offset 204 */
        uint64_t extractedNonce = convert::bytes2uint64(std::vector<uint8_t>(
            data.begin() + FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET,
            data.begin() + FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET + FalconConstants::NONCE_SIZE));
        
        REQUIRE(extractedNonce == nonce);
    }
    
    SECTION("Constants are correctly defined")
    {
        /* Full block sizes */
        REQUIRE(FalconConstants::FULL_BLOCK_TRITIUM_SIZE == 216);
        REQUIRE(FalconConstants::FULL_BLOCK_LEGACY_SIZE == 220);
        REQUIRE(FalconConstants::FULL_BLOCK_MERKLE_OFFSET == 132);
        REQUIRE(FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET == 200);
        REQUIRE(FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET == 204);
        
        /* Format detection thresholds */
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD == 200);
        REQUIRE(FalconConstants::FULL_BLOCK_TYPE_DETECTION_MARGIN == 100);
        
        /* BLOCK_DATA response sizes (NEW) */
        REQUIRE(FalconConstants::BLOCK_DATA_RESPONSE_MIN == 216);
        REQUIRE(FalconConstants::BLOCK_DATA_RESPONSE_MAX == 220);
        
        /* SUBMIT_BLOCK minimum sizes (NEW) */
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_MIN == 224);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_LEGACY_MIN == 228);
        
        /* SUBMIT_BLOCK with wrapper signature (NEW) */
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX == 1035);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX == 1039);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_ENCRYPTED_MAX == 1063);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_ENCRYPTED_MAX == 1067);
        
        /* Dual signature overhead helpers (NEW) */
        REQUIRE(FalconConstants::DUAL_SIG_OVERHEAD == 1622);
        REQUIRE(FalconConstants::DUAL_SIG_TOTAL_OVERHEAD == 1630);
        
        /* SUBMIT_BLOCK with dual signatures (NEW) */
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_MAX == 1846);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_MAX == 1850);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_ENCRYPTED_MAX == 1874);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_ENCRYPTED_MAX == 1878);
        
        /* Deprecated constants should still work for backward compatibility */
        REQUIRE(FalconConstants::SUBMIT_BLOCK_DUAL_SIG_TRITIUM_ENCRYPTED_MAX == 1874);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_DUAL_SIG_LEGACY_ENCRYPTED_MAX == 1878);
    }
}
