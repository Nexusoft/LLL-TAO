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
        
        /* hashMerkleRoot at offset 132 (64 bytes) */
        REQUIRE(fullBlockPacket.size() == 132);
        uint512_t expectedMerkle;
        for(size_t i = 0; i < 64; ++i)
        {
            fullBlockPacket.push_back(0xCC + (i % 16));
        }
        expectedMerkle.SetBytes(std::vector<uint8_t>(
            fullBlockPacket.begin() + 132,
            fullBlockPacket.begin() + 132 + 64));
        
        /* Additional fields to reach nonce at offset 200 */
        size_t currentOffset = fullBlockPacket.size();  // Should be 196
        REQUIRE(currentOffset == 196);
        
        /* Fill to offset 200 (4 bytes) */
        for(size_t i = currentOffset; i < 200; ++i)
            fullBlockPacket.push_back(0xDD);
        
        /* nNonce at offset 200 (8 bytes) */
        REQUIRE(fullBlockPacket.size() == 200);
        uint64_t expectedNonce = 0xFEDCBA9876543210;
        std::vector<uint8_t> nonceBytes = convert::uint2bytes64(expectedNonce);
        fullBlockPacket.insert(fullBlockPacket.end(), nonceBytes.begin(), nonceBytes.end());
        
        /* Fill remaining to 216 bytes */
        while(fullBlockPacket.size() < 216)
            fullBlockPacket.push_back(0xEE);
        
        REQUIRE(fullBlockPacket.size() == 216);
        
        /* Add timestamp (8 bytes) */
        uint64_t timestamp = 1735392000;
        std::vector<uint8_t> timestampBytes = convert::uint2bytes64(timestamp);
        fullBlockPacket.insert(fullBlockPacket.end(), timestampBytes.begin(), timestampBytes.end());
        
        REQUIRE(fullBlockPacket.size() == 224);
        
        /* Add dummy signature length and signature */
        uint16_t sigLen = 809;
        fullBlockPacket.push_back(sigLen & 0xFF);
        fullBlockPacket.push_back((sigLen >> 8) & 0xFF);
        
        /* Add dummy signature data */
        for(size_t i = 0; i < sigLen; ++i)
            fullBlockPacket.push_back(0xAA);
        
        REQUIRE(fullBlockPacket.size() == 1035);  // 216 + 8 + 2 + 809
        
        /* Now reconstruct the format that UnwrapWorkSubmission expects */
        /* Extract merkle root (64 bytes at offset 132) */
        uint512_t extractedMerkle;
        extractedMerkle.SetBytes(std::vector<uint8_t>(
            fullBlockPacket.begin() + 132,
            fullBlockPacket.begin() + 132 + 64
        ));
        
        /* Extract nonce (8 bytes at offset 200) */
        uint64_t extractedNonce = convert::bytes2uint64(std::vector<uint8_t>(
            fullBlockPacket.begin() + 200,
            fullBlockPacket.begin() + 200 + 8
        ));
        
        /* Verify extraction */
        REQUIRE(extractedMerkle == expectedMerkle);
        REQUIRE(extractedNonce == expectedNonce);
        
        /* Reconstruct signed data format: [merkle(64)][nonce(8)][timestamp(8)][sig_len(2)][signature] */
        std::vector<uint8_t> signedData;
        
        /* Add merkle root (64 bytes) */
        std::vector<uint8_t> merkleBytes = extractedMerkle.GetBytes();
        signedData.insert(signedData.end(), merkleBytes.begin(), merkleBytes.end());
        
        /* Add nonce (8 bytes, little-endian) */
        for(int i = 0; i < 8; ++i)
        {
            signedData.push_back((extractedNonce >> (i * 8)) & 0xFF);
        }
        
        /* Add timestamp + sig_len + signature (everything after the full block) */
        signedData.insert(signedData.end(),
                        fullBlockPacket.begin() + 216,
                        fullBlockPacket.end());
        
        /* Verify reconstructed format */
        REQUIRE(signedData.size() == 891);  // 64 + 8 + 8 + 2 + 809
        
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
        /* Full block format packet size */
        size_t fullBlockSize = 216;
        size_t timestampSize = 8;
        size_t sigLenSize = 2;
        size_t sigSize = 809;
        size_t totalPacketSize = fullBlockSize + timestampSize + sigLenSize + sigSize;
        
        REQUIRE(totalPacketSize == 1035);
        REQUIRE(totalPacketSize == FalconConstants::SUBMIT_BLOCK_WRAPPER_MAX);
        
        /* Reconstructed signed data format size */
        size_t merkleSize = 64;
        size_t nonceSize = 8;
        size_t reconstructedSize = merkleSize + nonceSize + timestampSize + sigLenSize + sigSize;
        
        REQUIRE(reconstructedSize == 891);  // This is the format UnwrapWorkSubmission expects
    }
}
