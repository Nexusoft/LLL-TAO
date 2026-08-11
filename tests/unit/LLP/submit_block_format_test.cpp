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
        
        /* Additional fields to reach nonce at the protocol-defined offset */
        size_t currentOffset = data.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill to the Tritium nonce offset */
        for(size_t i = currentOffset; i < FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET; ++i)
            data.push_back(0xDD);
        
        /* nNonce at the Tritium nonce offset */
        REQUIRE(data.size() == FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET);
        uint64_t nonce = 0xFEDCBA9876543210;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(nonce);
        data.insert(data.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to the Tritium full-block size */
        while(data.size() < FalconConstants::FULL_BLOCK_TRITIUM_SIZE)
            data.push_back(0xEE);
        
        REQUIRE(data.size() == FalconConstants::FULL_BLOCK_TRITIUM_SIZE);
        
        /* This should be detected as full block format */
        bool fFullBlockFormat = (data.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD);
        REQUIRE(fFullBlockFormat == true);
        
        /* Verify merkle root extraction at offset 132 */
        uint512_t hashMerkle;
        hashMerkle.SetBytes(std::vector<uint8_t>(
            data.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
            data.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE));
        
        /* Verify nonce extraction at the Tritium nonce offset */
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
        
        /* Additional fields to reach nonce at the protocol-defined offset */
        size_t currentOffset = data.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill to the Legacy nonce offset */
        for(size_t i = currentOffset; i < FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET; ++i)
            data.push_back(0xDD);
        
        /* nNonce at the Legacy nonce offset */
        REQUIRE(data.size() == FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET);
        uint64_t nonce = 0x1122334455667788;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(nonce);
        data.insert(data.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to the Legacy full-block size */
        while(data.size() < FalconConstants::FULL_BLOCK_LEGACY_SIZE)
            data.push_back(0xEE);
        
        REQUIRE(data.size() == FalconConstants::FULL_BLOCK_LEGACY_SIZE);
        
        /* This should be detected as full block format */
        bool fFullBlockFormat = (data.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD);
        REQUIRE(fFullBlockFormat == true);
        
        /* Verify nonce extraction at the Legacy nonce offset */
        uint64_t extractedNonce = convert::bytes2uint64(std::vector<uint8_t>(
            data.begin() + FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET,
            data.begin() + FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET + FalconConstants::NONCE_SIZE));
        
        REQUIRE(extractedNonce == nonce);
    }
    
    SECTION("Constants are correctly defined")
    {
        // Full block header sizes (block headers only; miners do NOT submit transactions)
        REQUIRE(FalconConstants::FULL_BLOCK_TRITIUM_SIZE == 216);
        REQUIRE(FalconConstants::FULL_BLOCK_LEGACY_SIZE == 220);
        REQUIRE(FalconConstants::FULL_BLOCK_MERKLE_OFFSET == 132);
        /* Both wrapped full-block layouts now place the nonce at byte 208.
         * The total packet sizes still differ (216 Tritium vs 220 Legacy)
         * because Legacy carries 4 extra bytes after the shared nonce slot. */
        REQUIRE(FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET == 208);
        REQUIRE(FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET == 208);
        
        // Main wrapper constants:
        // SUBMIT_BLOCK_WRAPPER_MAX = legacy_header(220) + prime_offsets(22)
        //   + timestamp(8) + siglen(2) + Falcon-1024 sig(1577) = 1829 bytes
        // Per NexusMiner PR #675, prime_offsets is capped at PRIME_VOFFSETS_MAX_SIZE = 22.
        REQUIRE(FalconConstants::SUBMIT_BLOCK_WRAPPER_MAX == 1829);
        // SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX = 1829 + ChaCha20 overhead(28) = 1857 bytes
        REQUIRE(FalconConstants::SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX == 1857);
        
        // Detailed constants for Tritium/Legacy specific values (Falcon-512, no prime offsets)
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX == 1035);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_ENCRYPTED_MAX == 1063);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX == 1039);
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_ENCRYPTED_MAX == 1067);
        
        // Format detection
        REQUIRE(FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD == 200);
        REQUIRE(FalconConstants::FULL_BLOCK_TYPE_DETECTION_MARGIN == 100);
    }
}
