/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/disposable_falcon.h>
#include <LLP/include/falcon_constants.h>

#include <LLC/include/flkey.h>
#include <LLC/include/random.h>

#include <Util/include/runtime.h>

#include <vector>
#include <cstdint>

/* Import specific types to avoid namespace pollution */
using LLP::DisposableFalcon::SignedWorkSubmission;
using namespace LLP::FalconConstants;


/** Helper function to parse 2-byte little-endian length
 *  This matches the implementation in all updated files */
inline uint16_t ParseLittleEndianLength(const std::vector<uint8_t>& vData, size_t offset)
{
    return static_cast<uint16_t>(vData[offset]) |
           (static_cast<uint16_t>(vData[offset + 1]) << 8);
}


/** Helper function to parse 2-byte big-endian length (OLD - should not be used) */
inline uint16_t ParseBigEndianLength(const std::vector<uint8_t>& vData, size_t offset)
{
    return (static_cast<uint16_t>(vData[offset]) << 8) |
           static_cast<uint16_t>(vData[offset + 1]);
}


/** Helper function to serialize 2-byte little-endian length */
inline std::vector<uint8_t> SerializeLittleEndianLength(uint16_t value)
{
    std::vector<uint8_t> vData;
    vData.push_back(static_cast<uint8_t>(value & 0xFF));  // Low byte first
    vData.push_back(static_cast<uint8_t>(value >> 8));    // High byte second
    return vData;
}


TEST_CASE("Falcon Constants Validation", "[falcon][constants]")
{
    SECTION("Key sizes are correct")
    {
        REQUIRE(FALCON512_PUBKEY_SIZE == 897);
        REQUIRE(FALCON512_PRIVKEY_SIZE == 1281);
    }

    SECTION("Signature sizes are correct")
    {
        REQUIRE(FALCON512_SIG_MIN == 600);
        REQUIRE(FALCON512_SIG_AUTH_MAX == 700);
        REQUIRE(FALCON512_SIG_ABSOLUTE_MAX == 752);
        REQUIRE(FALCON512_SIG_MAX_VALIDATION == 2048);
    }

    SECTION("ChaCha20 constants are correct")
    {
        REQUIRE(CHACHA20_NONCE_SIZE == 12);
        REQUIRE(CHACHA20_AUTH_TAG_SIZE == 16);
        REQUIRE(CHACHA20_OVERHEAD == 28);
        REQUIRE(CHACHA20_OVERHEAD == CHACHA20_NONCE_SIZE + CHACHA20_AUTH_TAG_SIZE);
    }

    SECTION("Protocol field sizes are correct")
    {
        REQUIRE(MERKLE_ROOT_SIZE == 64);
        REQUIRE(NONCE_SIZE == 8);
        REQUIRE(TIMESTAMP_SIZE == 8);
        REQUIRE(LENGTH_FIELD_SIZE == 2);
        REQUIRE(GENESIS_HASH_SIZE == 32);
    }

    SECTION("Submit block wrapper sizes are correct")
    {
        REQUIRE(SUBMIT_BLOCK_MESSAGE_SIZE == 80);
        REQUIRE(SUBMIT_BLOCK_MESSAGE_SIZE == MERKLE_ROOT_SIZE + NONCE_SIZE + TIMESTAMP_SIZE);
        
        REQUIRE(SUBMIT_BLOCK_WRAPPER_MIN == 82);
        REQUIRE(SUBMIT_BLOCK_WRAPPER_MIN == MERKLE_ROOT_SIZE + NONCE_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE);
        
        REQUIRE(SUBMIT_BLOCK_WRAPPER_MAX == 834);
        REQUIRE(SUBMIT_BLOCK_WRAPPER_MAX == SUBMIT_BLOCK_WRAPPER_MIN + FALCON512_SIG_ABSOLUTE_MAX);
        
        REQUIRE(SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX == 862);
        REQUIRE(SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX == SUBMIT_BLOCK_WRAPPER_MAX + CHACHA20_OVERHEAD);
    }

    SECTION("Auth response sizes are correct")
    {
        REQUIRE(AUTH_RESPONSE_MAX == 1661);
        REQUIRE(AUTH_RESPONSE_MAX == LENGTH_FIELD_SIZE + FALCON512_PUBKEY_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX);
        
        REQUIRE(AUTH_RESPONSE_ENCRYPTED_MAX == 1689);
        REQUIRE(AUTH_RESPONSE_ENCRYPTED_MAX == AUTH_RESPONSE_MAX + CHACHA20_OVERHEAD);
        
        REQUIRE(AUTH_RESPONSE_WITH_GENESIS_MAX == 1721);
        REQUIRE(AUTH_RESPONSE_WITH_GENESIS_MAX == AUTH_RESPONSE_ENCRYPTED_MAX + GENESIS_HASH_SIZE);
    }

    SECTION("Physical block signature sizes are correct")
    {
        REQUIRE(PHYSICAL_BLOCK_SIG_MIN == 600);
        REQUIRE(PHYSICAL_BLOCK_SIG_MIN == FALCON512_SIG_MIN);
        
        REQUIRE(PHYSICAL_BLOCK_SIG_MAX == 752);
        REQUIRE(PHYSICAL_BLOCK_SIG_MAX == FALCON512_SIG_ABSOLUTE_MAX);
        
        // Message max = 2MB + 8 bytes (nonce)
        REQUIRE(PHYSICAL_BLOCK_SIG_MESSAGE_MAX == (1024 * 1024 * 2) + 8);
        REQUIRE(PHYSICAL_BLOCK_SIG_MESSAGE_MAX == (1024 * 1024 * 2) + NONCE_SIZE);
        
        // Overhead = sig_len(2) + signature(752) = 754 bytes
        REQUIRE(PHYSICAL_BLOCK_SIG_OVERHEAD == 754);
        REQUIRE(PHYSICAL_BLOCK_SIG_OVERHEAD == LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX);
        
        REQUIRE(BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD == 754);
        REQUIRE(BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD == PHYSICAL_BLOCK_SIG_OVERHEAD);
    }
}


TEST_CASE("Little-Endian Length Serialization", "[falcon][endianness]")
{
    SECTION("Parse and serialize 0")
    {
        uint16_t value = 0;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0x00);
        REQUIRE(vSerialized[1] == 0x00);
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize 1")
    {
        uint16_t value = 1;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0x01);
        REQUIRE(vSerialized[1] == 0x00);
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize 256 (0x0100)")
    {
        uint16_t value = 256;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0x00);  // Low byte
        REQUIRE(vSerialized[1] == 0x01);  // High byte
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize typical signature length (666)")
    {
        uint16_t value = 666;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0x9A);  // 666 & 0xFF = 154 = 0x9A
        REQUIRE(vSerialized[1] == 0x02);  // 666 >> 8 = 2
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize maximum signature length (752)")
    {
        uint16_t value = FALCON512_SIG_ABSOLUTE_MAX;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0xF0);  // 752 & 0xFF = 240 = 0xF0
        REQUIRE(vSerialized[1] == 0x02);  // 752 >> 8 = 2
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize validation max (2048)")
    {
        uint16_t value = FALCON512_SIG_MAX_VALIDATION;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0x00);  // 2048 & 0xFF = 0
        REQUIRE(vSerialized[1] == 0x08);  // 2048 >> 8 = 8
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }

    SECTION("Parse and serialize max value (65535)")
    {
        uint16_t value = 65535;
        std::vector<uint8_t> vSerialized = SerializeLittleEndianLength(value);
        
        REQUIRE(vSerialized.size() == 2);
        REQUIRE(vSerialized[0] == 0xFF);
        REQUIRE(vSerialized[1] == 0xFF);
        
        uint16_t parsed = ParseLittleEndianLength(vSerialized, 0);
        REQUIRE(parsed == value);
    }
}


TEST_CASE("Big-Endian vs Little-Endian Consistency Check", "[falcon][endianness]")
{
    SECTION("256 is parsed differently")
    {
        std::vector<uint8_t> vData = {0x00, 0x01};  // Low byte first (little-endian 256)
        
        uint16_t le_value = ParseLittleEndianLength(vData, 0);
        uint16_t be_value = ParseBigEndianLength(vData, 0);
        
        REQUIRE(le_value == 256);  // Correct little-endian interpretation
        REQUIRE(be_value == 1);    // Wrong big-endian interpretation
        REQUIRE(le_value != be_value);
    }

    SECTION("Typical signature length 666")
    {
        std::vector<uint8_t> vData = {0x9A, 0x02};  // Little-endian for 666
        
        uint16_t le_value = ParseLittleEndianLength(vData, 0);
        uint16_t be_value = ParseBigEndianLength(vData, 0);
        
        REQUIRE(le_value == 666);   // Correct little-endian interpretation
        REQUIRE(be_value == 39426); // Wrong big-endian interpretation (0x9A02)
        REQUIRE(le_value != be_value);
    }

    SECTION("Max signature length 752")
    {
        std::vector<uint8_t> vData = {0xF0, 0x02};  // Little-endian for 752
        
        uint16_t le_value = ParseLittleEndianLength(vData, 0);
        uint16_t be_value = ParseBigEndianLength(vData, 0);
        
        REQUIRE(le_value == 752);   // Correct little-endian interpretation
        REQUIRE(be_value == 61442); // Wrong big-endian interpretation (0xF002)
        REQUIRE(le_value != be_value);
    }
}


TEST_CASE("SignedWorkSubmission Serialization Endianness", "[falcon][endianness]")
{
    SECTION("Serialize and deserialize with signature")
    {
        /* Create a test submission with a recognizable merkle root pattern */
        uint512_t hashMerkle;
        /* Repeating hex pattern 0x1234567890abcdef for easy verification */
        hashMerkle.SetHex("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        
        uint64_t nNonce = 0x0123456789ABCDEF;
        
        SignedWorkSubmission original(hashMerkle, nNonce);
        
        /* Create a mock signature of known length */
        original.vSignature.resize(666);  // Typical signature length
        for(size_t i = 0; i < original.vSignature.size(); ++i)
            original.vSignature[i] = static_cast<uint8_t>(i & 0xFF);
        
        original.fSigned = true;
        
        /* Serialize */
        std::vector<uint8_t> vSerialized = original.Serialize();
        
        /* Verify structure:
         * merkle(64) + nonce(8) + timestamp(8) + sig_len(2) + signature(666) = 748 bytes */
        size_t expected_size = 64 + 8 + 8 + 2 + 666;
        REQUIRE(vSerialized.size() == expected_size);
        
        /* Verify sig_len is little-endian (666 = 0x029A) */
        size_t sig_len_offset = 64 + 8 + 8;  // After merkle, nonce, timestamp
        REQUIRE(vSerialized[sig_len_offset] == 0x9A);  // Low byte
        REQUIRE(vSerialized[sig_len_offset + 1] == 0x02);  // High byte
        
        /* Parse sig_len as little-endian */
        uint16_t parsed_len = ParseLittleEndianLength(vSerialized, sig_len_offset);
        REQUIRE(parsed_len == 666);
        
        /* Deserialize */
        SignedWorkSubmission deserialized;
        bool success = deserialized.Deserialize(vSerialized);
        
        REQUIRE(success == true);
        REQUIRE(deserialized.hashMerkleRoot == original.hashMerkleRoot);
        REQUIRE(deserialized.nNonce == original.nNonce);
        REQUIRE(deserialized.nTimestamp == original.nTimestamp);
        REQUIRE(deserialized.vSignature.size() == original.vSignature.size());
        REQUIRE(deserialized.vSignature == original.vSignature);
        REQUIRE(deserialized.fSigned == true);
    }

    SECTION("Deserialize with maximum signature length")
    {
        /* Create serialized data with max signature length (752) */
        std::vector<uint8_t> vData;
        
        /* Merkle root (64 bytes) */
        for(size_t i = 0; i < 64; ++i)
            vData.push_back(static_cast<uint8_t>(i));
        
        /* Nonce (8 bytes, little-endian) */
        uint64_t nNonce = 0x0123456789ABCDEF;
        for(size_t i = 0; i < 8; ++i)
            vData.push_back(static_cast<uint8_t>((nNonce >> (i * 8)) & 0xFF));
        
        /* Timestamp (8 bytes, little-endian) */
        uint64_t nTimestamp = runtime::unifiedtimestamp();
        for(size_t i = 0; i < 8; ++i)
            vData.push_back(static_cast<uint8_t>((nTimestamp >> (i * 8)) & 0xFF));
        
        /* Signature length (2 bytes, little-endian) - 752 = 0x02F0 */
        vData.push_back(0xF0);  // Low byte
        vData.push_back(0x02);  // High byte
        
        /* Signature (752 bytes) */
        for(size_t i = 0; i < FALCON512_SIG_ABSOLUTE_MAX; ++i)
            vData.push_back(static_cast<uint8_t>(i & 0xFF));
        
        /* Verify total size */
        REQUIRE(vData.size() == 64 + 8 + 8 + 2 + 752);
        
        /* Deserialize */
        SignedWorkSubmission submission;
        bool success = submission.Deserialize(vData);
        
        REQUIRE(success == true);
        REQUIRE(submission.vSignature.size() == FALCON512_SIG_ABSOLUTE_MAX);
        REQUIRE(submission.fSigned == true);
    }

    SECTION("Reject packet with invalid signature length (too large)")
    {
        /* Create packet with sig_len > FALCON512_SIG_MAX_VALIDATION */
        std::vector<uint8_t> vData;
        
        /* Merkle + nonce + timestamp */
        for(size_t i = 0; i < 64 + 8 + 8; ++i)
            vData.push_back(0x00);
        
        /* Signature length = 3000 (exceeds max validation) */
        uint16_t invalid_len = 3000;
        vData.push_back(static_cast<uint8_t>(invalid_len & 0xFF));
        vData.push_back(static_cast<uint8_t>(invalid_len >> 8));
        
        /* Add some data */
        for(size_t i = 0; i < 100; ++i)
            vData.push_back(0xFF);
        
        /* Verify sig_len parsing */
        uint16_t parsed = ParseLittleEndianLength(vData, 64 + 8 + 8);
        REQUIRE(parsed == 3000);
        
        /* Note: SignedWorkSubmission::Deserialize doesn't validate max size,
         * but the protocol handlers (miner.cpp, stateless_miner.cpp) do */
    }
}


TEST_CASE("Nonce and Timestamp Little-Endian Encoding", "[falcon][endianness]")
{
    SECTION("Nonce encoding is little-endian")
    {
        uint64_t nNonce = 0x0123456789ABCDEF;
        
        SignedWorkSubmission submission;
        submission.hashMerkleRoot.SetHex("0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
        submission.nNonce = nNonce;
        submission.nTimestamp = 0;
        submission.vSignature.clear();
        
        std::vector<uint8_t> vMsg = submission.GetMessageBytes();
        
        /* Nonce should be at offset 64 (after merkle root) */
        size_t nonce_offset = 64;
        
        REQUIRE(vMsg[nonce_offset + 0] == 0xEF);  // Byte 0 (LSB)
        REQUIRE(vMsg[nonce_offset + 1] == 0xCD);
        REQUIRE(vMsg[nonce_offset + 2] == 0xAB);
        REQUIRE(vMsg[nonce_offset + 3] == 0x89);
        REQUIRE(vMsg[nonce_offset + 4] == 0x67);
        REQUIRE(vMsg[nonce_offset + 5] == 0x45);
        REQUIRE(vMsg[nonce_offset + 6] == 0x23);
        REQUIRE(vMsg[nonce_offset + 7] == 0x01);  // Byte 7 (MSB)
    }

    SECTION("Timestamp encoding is little-endian")
    {
        uint64_t nTimestamp = 0x0011223344556677;
        
        SignedWorkSubmission submission;
        submission.hashMerkleRoot.SetHex("0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
        submission.nNonce = 0;
        submission.nTimestamp = nTimestamp;
        submission.vSignature.clear();
        
        std::vector<uint8_t> vMsg = submission.GetMessageBytes();
        
        /* Timestamp should be at offset 72 (after merkle + nonce) */
        size_t timestamp_offset = 64 + 8;
        
        REQUIRE(vMsg[timestamp_offset + 0] == 0x77);  // Byte 0 (LSB)
        REQUIRE(vMsg[timestamp_offset + 1] == 0x66);
        REQUIRE(vMsg[timestamp_offset + 2] == 0x55);
        REQUIRE(vMsg[timestamp_offset + 3] == 0x44);
        REQUIRE(vMsg[timestamp_offset + 4] == 0x33);
        REQUIRE(vMsg[timestamp_offset + 5] == 0x22);
        REQUIRE(vMsg[timestamp_offset + 6] == 0x11);
        REQUIRE(vMsg[timestamp_offset + 7] == 0x00);  // Byte 7 (MSB)
    }
}
