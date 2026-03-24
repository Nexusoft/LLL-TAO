/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_constants.h>
#include <LLP/include/disposable_falcon.h>
#include <Util/include/convert.h>
#include <LLC/include/flkey.h>

using namespace LLP;
using LLP::DisposableFalcon::SignedWorkSubmission;

TEST_CASE("Falcon Full Block Format Reconstruction", "[falcon_full_block]")
{
    SECTION("Reconstruct signed data format from full block packet")
    {
        /* Simulate what the miner sends:
         * [full_block(216)][timestamp(8)][sig_len(2)][signature(~809)]
         */
        
        std::vector<uint8_t> fullBlockPacket;
        
        /* nVersion (4 bytes) */
        for(size_t i = 0; i < 4; ++i)
            fullBlockPacket.push_back(0x00);
        
        /* hashPrevBlock (128 bytes) */
        for(size_t i = 0; i < 128; ++i)
            fullBlockPacket.push_back(0xBB);
        
        /* hashMerkleRoot at offset FalconConstants::FULL_BLOCK_MERKLE_OFFSET (64 bytes) */
        REQUIRE(fullBlockPacket.size() == FalconConstants::FULL_BLOCK_MERKLE_OFFSET);
        uint512_t expectedMerkle;
        for(size_t i = 0; i < FalconConstants::MERKLE_ROOT_SIZE; ++i)
        {
            fullBlockPacket.push_back(0xCC + (i % 16));
        }
        expectedMerkle.SetBytes(std::vector<uint8_t>(
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE));
        
        /* Additional fields to reach nonce at offset FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET */
        size_t currentOffset = fullBlockPacket.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill nChannel(4), nHeight(4), nBits(4) to reach nNonce at offset 208 */
        for(size_t i = currentOffset; i < FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET; ++i)
            fullBlockPacket.push_back(0xDD);
        
        /* nNonce at offset FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET (8 bytes) */
        REQUIRE(fullBlockPacket.size() == FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET);
        uint64_t expectedNonce = 0xFEDCBA9876543210;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(expectedNonce);
        fullBlockPacket.insert(fullBlockPacket.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to FalconConstants::FULL_BLOCK_TRITIUM_SIZE bytes */
        while(fullBlockPacket.size() < FalconConstants::FULL_BLOCK_TRITIUM_SIZE)
            fullBlockPacket.push_back(0xEE);
        
        REQUIRE(fullBlockPacket.size() == FalconConstants::FULL_BLOCK_TRITIUM_SIZE);
        
        /* Add timestamp (8 bytes) */
        uint64_t timestamp = 1735392000;
        std::vector<uint8_t> timestampBytes = convert::uint2bytes64(timestamp);
        fullBlockPacket.insert(fullBlockPacket.end(), timestampBytes.begin(), timestampBytes.end());
        
        REQUIRE(fullBlockPacket.size() == FalconConstants::FULL_BLOCK_TRITIUM_SIZE + FalconConstants::TIMESTAMP_SIZE);
        
        /* Add dummy signature length and signature */
        uint16_t sigLen = FalconConstants::FALCON512_SIG_CT_SIZE;
        fullBlockPacket.push_back(sigLen & 0xFF);
        fullBlockPacket.push_back((sigLen >> 8) & 0xFF);
        
        /* Add dummy signature data */
        for(size_t i = 0; i < sigLen; ++i)
            fullBlockPacket.push_back(0xAA);
        
        REQUIRE(fullBlockPacket.size() == FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_MAX);
        
        /* Now reconstruct the format that UnwrapWorkSubmission expects */
        /* Extract merkle root (64 bytes at offset FalconConstants::FULL_BLOCK_MERKLE_OFFSET) */
        uint512_t extractedMerkle;
        extractedMerkle.SetBytes(std::vector<uint8_t>(
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE
        ));
        
        /* Extract nonce (8 bytes at offset FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET) */
        uint64_t extractedNonce = convert::bytes2uint64(std::vector<uint8_t>(
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET,
            fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE
        ));
        
        /* Verify extraction */
        REQUIRE(extractedMerkle == expectedMerkle);
        REQUIRE(extractedNonce == expectedNonce);
        
        /* Reconstruct signed data format: [merkle(64)][nonce(8)][timestamp(8)][sig_len(2)][signature] */
        std::vector<uint8_t> signedData;
        
        /* Add merkle root (64 bytes) */
        std::vector<uint8_t> merkleBytes = extractedMerkle.GetBytes();
        signedData.insert(signedData.end(), merkleBytes.begin(), merkleBytes.end());
        
        /* Add nonce (8 bytes, little-endian)
         * NOTE: Manual byte extraction is required because Falcon protocol uses little-endian,
         * while convert::uint2bytes64() uses big-endian. This matches SignedWorkSubmission::Deserialize() */
        for(size_t i = 0; i < FalconConstants::NONCE_SIZE; ++i)
        {
            signedData.push_back((extractedNonce >> (i * 8)) & 0xFF);
        }
        
        /* Add timestamp + sig_len + signature (everything after the full block) */
        signedData.insert(signedData.end(),
                        fullBlockPacket.begin() + FalconConstants::FULL_BLOCK_TRITIUM_SIZE,
                        fullBlockPacket.end());
        
        /* Verify reconstructed format */
        const size_t expectedReconstructedSize = FalconConstants::MERKLE_ROOT_SIZE + 
                                                  FalconConstants::NONCE_SIZE + 
                                                  FalconConstants::TIMESTAMP_SIZE + 
                                                  FalconConstants::LENGTH_FIELD_SIZE + 
                                                  FalconConstants::FALCON512_SIG_CT_SIZE;
        REQUIRE(signedData.size() == expectedReconstructedSize);
        
        /* Verify the format can be deserialized */
        SignedWorkSubmission submission;
        bool fDeserialized = submission.Deserialize(signedData);
        REQUIRE(fDeserialized == true);
        
        /* Verify deserialized values */
        REQUIRE(submission.hashMerkleRoot == expectedMerkle);
        REQUIRE(submission.nNonce == expectedNonce);
        REQUIRE(submission.nTimestamp == timestamp);
        REQUIRE(submission.vSignature.size() == sigLen);
    }
    
    SECTION("Verify format constants")
    {
        /* Full Tritium block header size (without transactions) */
        size_t fullBlockSize = FalconConstants::FULL_BLOCK_TRITIUM_SIZE;
        size_t timestampSize = FalconConstants::TIMESTAMP_SIZE;
        size_t sigLenSize = FalconConstants::LENGTH_FIELD_SIZE;
        size_t sigSize = FalconConstants::FALCON512_SIG_CT_SIZE;
        size_t totalPacketSize = fullBlockSize + timestampSize + sigLenSize + sigSize;
        
        /* Tritium Hash-channel packet with Falcon-512 signature = 1035 bytes */
        REQUIRE(totalPacketSize == FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_MAX);
        /* All valid submission packets must not exceed the protocol maximum */
        REQUIRE(totalPacketSize <= FalconConstants::SUBMIT_BLOCK_WRAPPER_MAX);
        
        /* Reconstructed signed data format size */
        size_t merkleSize = FalconConstants::MERKLE_ROOT_SIZE;
        size_t nonceSize = FalconConstants::NONCE_SIZE;
        size_t reconstructedSize = merkleSize + nonceSize + timestampSize + sigLenSize + sigSize;
        
        REQUIRE(reconstructedSize == 891);  // This is the format UnwrapWorkSubmission expects
    }
}
