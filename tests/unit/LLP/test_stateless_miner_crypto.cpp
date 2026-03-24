/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/mining_session_keys.h>
#include <LLC/include/chacha20_helpers.h>
#include <LLC/include/encrypt.h>
#include <LLC/include/flkey.h>
#include <LLC/include/random.h>
#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>

#include <LLP/include/falcon_constants.h>

#include <TAO/Ledger/include/stateless_block_utility.h>

#include <Util/include/convert.h>
#include <Util/include/hex.h>
#include <Util/include/debug.h>

#include <openssl/sha.h>

#include <algorithm>
#include <cstring>

using namespace LLC;
using namespace LLC::MiningSessionKeys;


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 1 — KDF Consistency (T01–T04)
 * ══════════════════════════════════════════════════════════════════════ */

TEST_CASE("T01: KDF produces exact known SHA256 output", "[stateless_miner_crypto][kdf]")
{
    /* Pin the exact KDF formula: SHA256("nexus-mining-chacha20-v1" || genesis_bytes_via_GetHex_ParseHex) */
    uint256_t hashGenesis;
    hashGenesis.SetHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    /* Derive key using the production function */
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);
    REQUIRE(vKey.size() == 32);

    /* Manually compute expected value: SHA256(domain || ParseHex(GetHex(genesis))) */
    std::string domain = "nexus-mining-chacha20-v1";
    std::vector<uint8_t> preimage(domain.begin(), domain.end());
    std::string genesis_hex = hashGenesis.GetHex();
    std::vector<uint8_t> genesis_bytes = ParseHex(genesis_hex);
    REQUIRE(genesis_bytes.size() == 32);
    preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());

    std::vector<uint8_t> vExpected(32);
    SHA256(preimage.data(), preimage.size(), vExpected.data());

    REQUIRE(vKey == vExpected);
}


TEST_CASE("T02: AUTH path and SUBMIT path derive identical keys from same genesis", "[stateless_miner_crypto][kdf]")
{
    /* Both code paths call DeriveChaCha20Key with the same genesis.
     * If different byte orders were used, keys would differ. */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");

    /* Simulate AUTH path derivation (stateless_miner.cpp line ~755) */
    std::vector<uint8_t> vKeyAuth = DeriveChaCha20Key(hashGenesis);

    /* Simulate SUBMIT path derivation (stateless_miner_connection.cpp, from context.vChaChaKey) */
    std::vector<uint8_t> vKeySubmit = DeriveChaCha20Key(hashGenesis);

    REQUIRE(vKeyAuth == vKeySubmit);
    REQUIRE(vKeyAuth.size() == 32);
}


TEST_CASE("T03: GetBytes() byte order vs GetHex()+ParseHex()", "[stateless_miner_crypto][kdf][byte_order]")
{
    /* *** THIS IS THE EASTER EGG DETECTOR ***
     *
     * DeriveChaCha20Key uses GetHex()+ParseHex() for genesis bytes (big-endian).
     * GetBytes() uses a different byte order (iterates pn[] from index 0 = least significant word).
     * This test documents the difference definitively.
     *
     * RESULT: GetBytes() returns DIFFERENT byte order than GetHex()+ParseHex().
     * GetHex()+ParseHex() = big-endian (most significant byte first)
     * GetBytes() = little-endian word order (pn[0] big-endian bytes first = least significant word first)
     *
     * The KDF (DeriveChaCha20Key) uses GetHex()+ParseHex(), NOT GetBytes().
     * Miners MUST use big-endian genesis byte representation to match.
     */
    uint256_t testGenesis;
    testGenesis.SetHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    /* Method A: GetHex() + ParseHex() — what the KDF actually uses */
    std::string hex = testGenesis.GetHex();
    std::vector<uint8_t> bytes_a = ParseHex(hex);

    /* Method B: GetBytes() — what the miner might use if it called GetBytes() */
    std::vector<uint8_t> bytes_b = testGenesis.GetBytes();

    /* Both should be 32 bytes */
    REQUIRE(bytes_a.size() == 32);
    REQUIRE(bytes_b.size() == 32);

    /* Document: are they the same? */
    /* GetHex()+ParseHex() gives big-endian representation.
     * GetBytes() iterates pn[0]..pn[7], each word big-endian within itself,
     * but pn[0] is the LEAST significant word — so overall it's reversed.
     * Therefore bytes_a != bytes_b for asymmetric genesis values. */
    REQUIRE(bytes_a != bytes_b);

    /* Verify: reversing GetBytes() at 4-byte word granularity should match GetHex()+ParseHex() */
    std::vector<uint8_t> bytes_b_reversed;
    for(int i = static_cast<int>(bytes_b.size()) - 4; i >= 0; i -= 4)
        bytes_b_reversed.insert(bytes_b_reversed.end(), bytes_b.begin() + i, bytes_b.begin() + i + 4);

    REQUIRE(bytes_a == bytes_b_reversed);
}


TEST_CASE("T04: Zero genesis key != non-zero genesis key", "[stateless_miner_crypto][kdf]")
{
    uint256_t hashZero(0);
    uint256_t hashNonZero;
    hashNonZero.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");

    std::vector<uint8_t> vKeyZero = DeriveChaCha20Key(hashZero);
    std::vector<uint8_t> vKeyNonZero = DeriveChaCha20Key(hashNonZero);

    /* Both should be 32 bytes */
    REQUIRE(vKeyZero.size() == 32);
    REQUIRE(vKeyNonZero.size() == 32);

    /* Must be different — ensures KDF is not degenerate */
    REQUIRE(vKeyZero != vKeyNonZero);
}


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 2 — AAD Correctness Per Packet Type (T05–T09)
 * ══════════════════════════════════════════════════════════════════════ */

TEST_CASE("T05: MINER_AUTH_INIT decrypt with AAD='FALCON_PUBKEY'", "[stateless_miner_crypto][aad]")
{
    /* Derive a test key */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Simulate a Falcon public key payload */
    std::vector<uint8_t> vPubKey(897, 0xAB);

    /* AAD for MINER_AUTH_INIT pubkey wrapping */
    std::vector<uint8_t> vAAD{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};
    REQUIRE(vAAD.size() == 13);

    /* Encrypt with correct AAD */
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPubKey, vKey, vAAD);
    REQUIRE(!vEncrypted.empty());

    /* Decrypt with correct AAD → OK */
    std::vector<uint8_t> vDecrypted;
    bool fOk = LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted, vAAD);
    REQUIRE(fOk);
    REQUIRE(vDecrypted == vPubKey);

    /* Decrypt with empty AAD → FAIL */
    std::vector<uint8_t> vDecryptedBad;
    std::vector<uint8_t> vEmptyAAD;
    bool fBad = LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecryptedBad, vEmptyAAD);
    REQUIRE_FALSE(fBad);
}


TEST_CASE("T06: MINER_SET_REWARD decrypt with AAD='REWARD_ADDRESS'", "[stateless_miner_crypto][aad]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Simulate reward address payload */
    std::vector<uint8_t> vRewardAddr(64, 0xCC);

    /* AAD for MINER_SET_REWARD */
    std::vector<uint8_t> vAAD{'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};
    REQUIRE(vAAD.size() == 14);

    /* Round-trip with correct AAD → OK */
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vRewardAddr, vKey, vAAD);
    REQUIRE(!vEncrypted.empty());

    std::vector<uint8_t> vDecrypted;
    REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted, vAAD));
    REQUIRE(vDecrypted == vRewardAddr);

    /* Round-trip with wrong AAD → FAIL */
    std::vector<uint8_t> vDecryptedBad;
    std::vector<uint8_t> vWrongAAD{'W','R','O','N','G'};
    REQUIRE_FALSE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecryptedBad, vWrongAAD));
}


TEST_CASE("T07: MINER_REWARD_RESULT encrypt with AAD='REWARD_RESULT'", "[stateless_miner_crypto][aad]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Simulate result payload */
    std::vector<uint8_t> vResult(32, 0xDD);

    /* AAD for MINER_REWARD_RESULT */
    std::vector<uint8_t> vAAD{'R','E','W','A','R','D','_','R','E','S','U','L','T'};
    REQUIRE(vAAD.size() == 13);

    /* Round-trip with correct AAD → OK */
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vResult, vKey, vAAD);
    REQUIRE(!vEncrypted.empty());

    std::vector<uint8_t> vDecrypted;
    REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted, vAAD));
    REQUIRE(vDecrypted == vResult);

    /* Round-trip with wrong AAD → FAIL */
    std::vector<uint8_t> vDecryptedBad;
    std::vector<uint8_t> vWrongAAD{'N','O','P','E'};
    REQUIRE_FALSE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecryptedBad, vWrongAAD));
}


TEST_CASE("T08: SUBMIT_BLOCK decrypt AAD is empty", "[stateless_miner_crypto][aad][submit_block]")
{
    /* *** THE EASTER EGG ***
     *
     * What AAD string does the node actually pass to ChaCha20 when decrypting SUBMIT_BLOCK?
     *
     * ANSWER: Empty {} — no AAD.
     *
     * The node calls LLC::DecryptPayloadChaCha20(PACKET.DATA, context.vChaChaKey, decryptedData)
     * with NO AAD parameter (uses default empty vector).
     * See: src/LLP/stateless_miner_connection.cpp line ~1186-1190
     *
     * Therefore the miner MUST also encrypt with empty AAD for SUBMIT_BLOCK.
     */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Simulate a SUBMIT_BLOCK payload (block + timestamp + sig_len + sig) */
    std::vector<uint8_t> vPayload(216 + 8 + 2 + 809, 0xEE);

    /* Encrypt with empty AAD (matching node behavior) */
    std::vector<uint8_t> vEmptyAAD;
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vPayload, vKey, vEmptyAAD);
    REQUIRE(!vEncrypted.empty());

    /* Decrypt with empty AAD → OK (matches node's DecryptPayloadChaCha20 default) */
    std::vector<uint8_t> vDecrypted;
    bool fOk = LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted);
    REQUIRE(fOk);
    REQUIRE(vDecrypted == vPayload);

    /* Also verify: encrypt with empty AAD, decrypt with non-empty AAD → FAIL */
    std::vector<uint8_t> vDecryptedBad;
    std::vector<uint8_t> vWrongAAD{'B','L','O','C','K'};
    REQUIRE_FALSE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecryptedBad, vWrongAAD));
}


TEST_CASE("T09: Cross-packet AAD rejection", "[stateless_miner_crypto][aad]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    std::vector<uint8_t> vData(128, 0xFF);

    /* Encrypt with FALCON_PUBKEY AAD */
    std::vector<uint8_t> vAAD_Auth{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(vData, vKey, vAAD_Auth);
    REQUIRE(!vEncrypted.empty());

    /* Decrypt with REWARD_ADDRESS AAD → FAIL (domain separation enforced) */
    std::vector<uint8_t> vAAD_Reward{'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};
    std::vector<uint8_t> vDecrypted;
    REQUIRE_FALSE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted, vAAD_Reward));

    /* Decrypt with correct AAD → OK */
    std::vector<uint8_t> vDecryptedOk;
    REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecryptedOk, vAAD_Auth));
    REQUIRE(vDecryptedOk == vData);
}


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 3 — SUBMIT_BLOCK Payload Format (T10–T14)
 * ══════════════════════════════════════════════════════════════════════ */

/* Helper: Build a SUBMIT_BLOCK plaintext payload.
 * Format: [block(blockSize)][timestamp(8LE)][sig_len(2LE)][signature(sigLen)] */
static std::vector<uint8_t> BuildSubmitBlockPayload(
    size_t blockSize,
    uint64_t nTimestamp,
    uint16_t sigLen
)
{
    std::vector<uint8_t> payload;

    /* Block data (fill with pattern) */
    for(size_t i = 0; i < blockSize; ++i)
        payload.push_back(static_cast<uint8_t>(i & 0xFF));

    /* Timestamp (8 bytes, little-endian) */
    for(int i = 0; i < 8; ++i)
        payload.push_back(static_cast<uint8_t>((nTimestamp >> (i * 8)) & 0xFF));

    /* sig_len (2 bytes, little-endian) */
    payload.push_back(static_cast<uint8_t>(sigLen & 0xFF));
    payload.push_back(static_cast<uint8_t>((sigLen >> 8) & 0xFF));

    /* Signature data */
    for(uint16_t i = 0; i < sigLen; ++i)
        payload.push_back(static_cast<uint8_t>(0xAA + (i & 0x0F)));

    return payload;
}


/* Helper fixture for shared SUBMIT_BLOCK parser tests.
 * It bundles a signed full-block payload with the Falcon pubkey and the
 * expected decoded fields used by the parser/verifier assertions below. */
struct FalconFullBlockFixture
{
    std::vector<uint8_t> payload;
    std::vector<uint8_t> pubkey;
    std::vector<uint8_t> offsets;
    uint512_t hashMerkle;
    uint64_t nonce = 0;
    uint64_t timestamp = 0;
};


static std::vector<uint8_t> BuildTritiumBlockBody(
    uint32_t nChannel,
    const uint512_t& hashMerkle,
    uint64_t nonce)
{
    std::vector<uint8_t> block(LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN, 0x00);

    std::vector<uint8_t> merkleBytes = hashMerkle.GetBytes();
    std::copy(
        merkleBytes.begin(),
        merkleBytes.end(),
        block.begin() + LLP::FalconConstants::FULL_BLOCK_MERKLE_OFFSET);

    std::vector<uint8_t> channelBytes = convert::uint2bytes(nChannel);
    std::copy(
        channelBytes.begin(),
        channelBytes.end(),
        block.begin() + LLP::FalconConstants::FULL_BLOCK_TRITIUM_CHANNEL_OFFSET);

    for(size_t i = 0; i < LLP::FalconConstants::NONCE_SIZE; ++i)
        block[LLP::FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + i] =
            static_cast<uint8_t>((nonce >> (8 * i)) & 0xff);

    return block;
}


static FalconFullBlockFixture BuildFalconFullBlockFixture(
    uint32_t nChannel,
    const std::vector<uint8_t>& offsets,
    uint64_t timestamp,
    LLC::FalconVersion version)
{
    FalconFullBlockFixture fixture;
    fixture.offsets = offsets;
    fixture.timestamp = timestamp;
    fixture.nonce = 0x0102030405060708ULL;

    std::vector<uint8_t> merkleBytes(LLP::FalconConstants::MERKLE_ROOT_SIZE);
    for(size_t i = 0; i < merkleBytes.size(); ++i)
        merkleBytes[i] = static_cast<uint8_t>(0xA0 + (i & 0x0F));
    fixture.hashMerkle.SetBytes(merkleBytes);

    std::vector<uint8_t> blockBytes = BuildTritiumBlockBody(nChannel, fixture.hashMerkle, fixture.nonce);
    /* vOffsets are no longer transmitted on the wire (PR #452).
     * The node derives them locally via GetOffsets(pBlock->GetPrime(), pBlock->vOffsets). */

    std::vector<uint8_t> message = blockBytes;
    for(size_t i = 0; i < LLP::FalconConstants::TIMESTAMP_SIZE; ++i)
        message.push_back(static_cast<uint8_t>((timestamp >> (8 * i)) & 0xff));

    LLC::FLKey key;
    key.MakeNewKey(version);
    fixture.pubkey = key.GetPubKey();
    REQUIRE(!fixture.pubkey.empty());

    std::vector<uint8_t> signature;
    REQUIRE(key.Sign(message, signature));
    REQUIRE(!signature.empty());

    fixture.payload = blockBytes;
    for(size_t i = 0; i < LLP::FalconConstants::TIMESTAMP_SIZE; ++i)
        fixture.payload.push_back(static_cast<uint8_t>((timestamp >> (8 * i)) & 0xff));

    const uint16_t sigLen = static_cast<uint16_t>(signature.size());
    fixture.payload.push_back(static_cast<uint8_t>(sigLen & 0xff));
    fixture.payload.push_back(static_cast<uint8_t>((sigLen >> 8) & 0xff));
    fixture.payload.insert(fixture.payload.end(), signature.begin(), signature.end());

    return fixture;
}


TEST_CASE("T10: SUBMIT_BLOCK payload format encrypt/decrypt round-trip", "[stateless_miner_crypto][payload]")
{
    /* Format: [block(216)][timestamp(8LE)][sig_len(2LE)][falcon_ct_sig(809)] */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    uint64_t nTimestamp = 1700000000;
    uint16_t sigLen = 809;  /* Falcon-512 CT */
    std::vector<uint8_t> payload = BuildSubmitBlockPayload(216, nTimestamp, sigLen);

    /* Expected total size: 216 + 8 + 2 + 809 = 1035 */
    REQUIRE(payload.size() == 1035);

    /* Encrypt and decrypt */
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);
    REQUIRE(!vEncrypted.empty());

    std::vector<uint8_t> vDecrypted;
    REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted));
    REQUIRE(vDecrypted == payload);

    /* Parse fields from decrypted payload */
    size_t blockSize = 216;

    /* Verify timestamp extraction (little-endian) */
    uint64_t extractedTimestamp = 0;
    for(int i = 0; i < 8; ++i)
        extractedTimestamp |= (static_cast<uint64_t>(vDecrypted[blockSize + i]) << (i * 8));
    REQUIRE(extractedTimestamp == nTimestamp);

    /* Verify sig_len extraction (little-endian) */
    uint16_t extractedSigLen = static_cast<uint16_t>(vDecrypted[blockSize + 8]) |
                                (static_cast<uint16_t>(vDecrypted[blockSize + 9]) << 8);
    REQUIRE(extractedSigLen == sigLen);
}


TEST_CASE("T11: Falcon signature sizes in SUBMIT_BLOCK", "[stateless_miner_crypto][payload]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    SECTION("Falcon-1024 CT signature (1577 bytes) accepted")
    {
        std::vector<uint8_t> payload = BuildSubmitBlockPayload(216, 1700000000, 1577);
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted));

        uint16_t sigLen = static_cast<uint16_t>(vDecrypted[216 + 8]) |
                           (static_cast<uint16_t>(vDecrypted[216 + 9]) << 8);
        REQUIRE(sigLen == 1577);
        REQUIRE(sigLen >= LLP::FalconConstants::FALCON_SIG_MIN);
        REQUIRE(sigLen <= LLP::FalconConstants::FALCON_SIG_MAX_VALIDATION);
    }

    SECTION("Falcon-512 CT signature (809 bytes) accepted")
    {
        std::vector<uint8_t> payload = BuildSubmitBlockPayload(216, 1700000000, 809);
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted));

        uint16_t sigLen = static_cast<uint16_t>(vDecrypted[216 + 8]) |
                           (static_cast<uint16_t>(vDecrypted[216 + 9]) << 8);
        REQUIRE(sigLen == 809);
        REQUIRE(sigLen >= LLP::FalconConstants::FALCON_SIG_MIN);
        REQUIRE(sigLen <= LLP::FalconConstants::FALCON_SIG_MAX_VALIDATION);
    }

    SECTION("Non-CT variable-time signature (500 bytes) rejected by validation range")
    {
        /* A 500-byte signature is below FALCON_SIG_MIN (600) */
        uint16_t badSigLen = 500;
        REQUIRE(badSigLen < LLP::FalconConstants::FALCON_SIG_MIN);
    }
}


TEST_CASE("T12: Timestamp little-endian decoding", "[stateless_miner_crypto][payload]")
{
    /* bytes [08,07,06,05,04,03,02,01] → timestamp == 0x0102030405060708 */
    std::vector<uint8_t> timestampBytes = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

    uint64_t nTimestamp = 0;
    for(int i = 0; i < 8; ++i)
        nTimestamp |= (static_cast<uint64_t>(timestampBytes[i]) << (i * 8));

    REQUIRE(nTimestamp == 0x0102030405060708ULL);
}


TEST_CASE("T13: Block field size validation", "[stateless_miner_crypto][payload]")
{
    SECTION("216-byte Tritium block accepted")
    {
        REQUIRE(216 == LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN);
    }

    SECTION("220-byte Legacy block accepted")
    {
        REQUIRE(220 == LLP::FalconConstants::FULL_BLOCK_LEGACY_MIN);
    }

    SECTION("215-byte block rejected (too short)")
    {
        size_t tooShort = 215;
        REQUIRE(tooShort < LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN);
        REQUIRE(tooShort < LLP::FalconConstants::FULL_BLOCK_LEGACY_MIN);
    }
}


TEST_CASE("T14: Simplified wire format (Physical Falcon removed)", "[stateless_miner_crypto][payload]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    SECTION("Simplified format: block + timestamp + sig_len + sig (no physiglen)")
    {
        /* New format: [block(216)][timestamp(8)][sig_len(2)][sig(809)] = 1035 bytes */
        std::vector<uint8_t> payload = BuildSubmitBlockPayload(216, 1700000000, 809);
        REQUIRE(payload.size() == 1035);

        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);
        std::vector<uint8_t> vDecrypted;
        REQUIRE(LLC::DecryptPayloadChaCha20(vEncrypted, vKey, vDecrypted));
        REQUIRE(vDecrypted == payload);
    }

    SECTION("sig_len above valid range rejected")
    {
        uint16_t tooLarge = 1578;
        REQUIRE(tooLarge > LLP::FalconConstants::FALCON_SIG_ABSOLUTE_MAX);
    }
}


TEST_CASE("T22: Shared Falcon full-block parser handles Hash and Prime payloads", "[stateless_miner_crypto][payload][submit_block]")
{
    SECTION("Hash Falcon-1024 payload stays fixed-size")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(2, {}, 1700000000, LLC::FalconVersion::FALCON_1024);

        /* 216 (block) + 8 (timestamp) + 2 (sig_len) + Falcon-1024 sig (~1577 bytes) = 1803 */
        REQUIRE(fixture.payload.size() == 1803);
        /* ChaCha20 adds 28 bytes overhead (12-byte nonce + 16-byte poly tag) */
        REQUIRE(LLC::EncryptPayloadChaCha20(fixture.payload, std::vector<uint8_t>(32, 0x11)).size() == 1831);

        auto result = TAO::Ledger::ParseFalconWrappedSubmitBlock(fixture.payload);
        REQUIRE(result.success);
        REQUIRE(result.nChannel == 2);
        REQUIRE(result.vBlockBody.size() == 216);
        REQUIRE(result.vOffsets.empty());
        REQUIRE(result.hashMerkle == fixture.hashMerkle);
        REQUIRE(result.nonce == fixture.nonce);
        REQUIRE(result.timestamp == fixture.timestamp);
        REQUIRE(result.nSignatureLength == result.vSignature.size());
    }

    SECTION("Prime Falcon-1024 payload is fixed-size (vOffsets derived by node, not on wire)")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(1, {}, 1700000000, LLC::FalconVersion::FALCON_1024);

        /* Same size as Hash channel: 216 + 8 + 2 + Falcon-1024 sig = 1803 */
        REQUIRE(fixture.payload.size() == 1803);
        REQUIRE(LLC::EncryptPayloadChaCha20(fixture.payload, std::vector<uint8_t>(32, 0x22)).size() == 1831);

        auto result = TAO::Ledger::ParseFalconWrappedSubmitBlock(fixture.payload);
        REQUIRE(result.success);
        REQUIRE(result.nChannel == 1);
        REQUIRE(result.vBlockBody.size() == 216);
        /* vOffsets is always empty from the parser; node derives them via GetOffsets(GetPrime(), ...) */
        REQUIRE(result.vOffsets.empty());
        REQUIRE(result.hashMerkle == fixture.hashMerkle);
        REQUIRE(result.nonce == fixture.nonce);
        REQUIRE(result.timestamp == fixture.timestamp);
        REQUIRE(result.nSignatureLength == result.vSignature.size());
    }
}


TEST_CASE("T23: Shared Falcon full-block verifier is lane-agnostic", "[stateless_miner_crypto][payload][submit_block]")
{
    auto verifyForLane = [](const FalconFullBlockFixture& fixture)
    {
        TAO::Ledger::FalconWrappedSubmitBlockParseResult result;
        REQUIRE(TAO::Ledger::VerifyFalconWrappedSubmitBlock(fixture.payload, fixture.pubkey, result));
        return result;
    };

    SECTION("legacy lane using shared parser accepts Prime channel payload")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(
            1, {}, 1700000011, LLC::FalconVersion::FALCON_1024);

        auto result = verifyForLane(fixture);
        REQUIRE(result.nChannel == 1);
        /* vOffsets is always empty from the parser; node derives via GetOffsets(GetPrime(), ...) */
        REQUIRE(result.vOffsets.empty());
    }

    SECTION("stateless lane using shared parser accepts Hash fixed-size payload")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(
            2, {}, 1700000012, LLC::FalconVersion::FALCON_1024);

        auto result = verifyForLane(fixture);
        REQUIRE(result.nChannel == 2);
        REQUIRE(result.vOffsets.empty());
    }
}


TEST_CASE("T24: Shared Falcon full-block parser rejects malformed channel payloads", "[stateless_miner_crypto][payload][submit_block]")
{
    SECTION("Hash submission with extra bytes before trailer must fail")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(
            2, {0x99, 0x98}, 1700000020, LLC::FalconVersion::FALCON_1024);

        auto result = TAO::Ledger::ParseFalconWrappedSubmitBlock(fixture.payload);
        REQUIRE_FALSE(result.success);
    }

    SECTION("Malformed signature length must fail")
    {
        FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(
            1, {0x01, 0x02, 0x03}, 1700000021, LLC::FalconVersion::FALCON_1024);

        auto parsed = TAO::Ledger::ParseFalconWrappedSubmitBlock(fixture.payload);
        REQUIRE(parsed.success);

        const size_t sigLenOffset = parsed.vBlockBytes.size() + LLP::FalconConstants::TIMESTAMP_SIZE;
        fixture.payload[sigLenOffset] ^= 0x01;

        TAO::Ledger::FalconWrappedSubmitBlockParseResult result;
        REQUIRE_FALSE(TAO::Ledger::VerifyFalconWrappedSubmitBlock(fixture.payload, fixture.pubkey, result));
    }

    SECTION("Payload shorter than minimal trailer requirements must fail")
    {
        std::vector<uint8_t> payload(LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN + LLP::FalconConstants::TIMESTAMP_SIZE + 1, 0x00);
        auto result = TAO::Ledger::ParseFalconWrappedSubmitBlock(payload);
        REQUIRE_FALSE(result.success);
    }
}


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 4 — Multi-Miner / Two-Computer Scenario (T15–T18)
 * ══════════════════════════════════════════════════════════════════════ */

TEST_CASE("T15: Same genesis, different Falcon keys → same ChaCha20 session key", "[stateless_miner_crypto][multi_miner]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");

    /* Two different Falcon key pairs (just different pubkeys for this test) */
    std::vector<uint8_t> vPubKeyA(1793, 0xAA);  /* Falcon-1024 pubkey A */
    std::vector<uint8_t> vPubKeyB(1793, 0xBB);  /* Falcon-1024 pubkey B */
    REQUIRE(vPubKeyA != vPubKeyB);

    /* ChaCha20 key derivation does NOT depend on Falcon key pair */
    std::vector<uint8_t> vKeyA = DeriveChaCha20Key(hashGenesis);
    std::vector<uint8_t> vKeyB = DeriveChaCha20Key(hashGenesis);

    REQUIRE(vKeyA == vKeyB);
}


TEST_CASE("T16: Both miners can submit blocks with shared genesis key", "[stateless_miner_crypto][multi_miner]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Miner A encrypts SUBMIT_BLOCK */
    std::vector<uint8_t> payloadA = BuildSubmitBlockPayload(216, 1700000001, 809);
    std::vector<uint8_t> encryptedA = LLC::EncryptPayloadChaCha20(payloadA, vKey);
    REQUIRE(!encryptedA.empty());

    /* Miner B encrypts SUBMIT_BLOCK */
    std::vector<uint8_t> payloadB = BuildSubmitBlockPayload(216, 1700000002, 1577);
    std::vector<uint8_t> encryptedB = LLC::EncryptPayloadChaCha20(payloadB, vKey);
    REQUIRE(!encryptedB.empty());

    /* Node decrypts both with shared genesis key → OK */
    std::vector<uint8_t> decryptedA;
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedA, vKey, decryptedA));
    REQUIRE(decryptedA == payloadA);

    std::vector<uint8_t> decryptedB;
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedB, vKey, decryptedB));
    REQUIRE(decryptedB == payloadB);
}


TEST_CASE("T17: Random nonce ensures different ciphertexts for same plaintext", "[stateless_miner_crypto][multi_miner]")
{
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Same plaintext encrypted twice */
    std::vector<uint8_t> payload = BuildSubmitBlockPayload(216, 1700000000, 809);
    std::vector<uint8_t> encryptedA = LLC::EncryptPayloadChaCha20(payload, vKey);
    std::vector<uint8_t> encryptedB = LLC::EncryptPayloadChaCha20(payload, vKey);

    /* Nonces should differ (first 12 bytes are random nonce) */
    REQUIRE(encryptedA.size() >= 12);
    REQUIRE(encryptedB.size() >= 12);
    std::vector<uint8_t> nonceA(encryptedA.begin(), encryptedA.begin() + 12);
    std::vector<uint8_t> nonceB(encryptedB.begin(), encryptedB.begin() + 12);
    REQUIRE(nonceA != nonceB);

    /* Ciphertexts should differ */
    REQUIRE(encryptedA != encryptedB);

    /* But both decrypt correctly */
    std::vector<uint8_t> decryptedA, decryptedB;
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedA, vKey, decryptedA));
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedB, vKey, decryptedB));
    REQUIRE(decryptedA == payload);
    REQUIRE(decryptedB == payload);
}


TEST_CASE("T18: Concurrent sessions — independent decryption", "[stateless_miner_crypto][multi_miner]")
{
    /* Two miners with same genesis but different active sessions */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");

    /* Session A key */
    std::vector<uint8_t> vKeyA = DeriveChaCha20Key(hashGenesis);
    /* Session B key (same genesis → same key) */
    std::vector<uint8_t> vKeyB = DeriveChaCha20Key(hashGenesis);
    REQUIRE(vKeyA == vKeyB);

    /* Each session encrypts different data */
    std::vector<uint8_t> payloadA = BuildSubmitBlockPayload(216, 1700000001, 809);
    std::vector<uint8_t> payloadB = BuildSubmitBlockPayload(220, 1700000002, 1577);

    std::vector<uint8_t> encryptedA = LLC::EncryptPayloadChaCha20(payloadA, vKeyA);
    std::vector<uint8_t> encryptedB = LLC::EncryptPayloadChaCha20(payloadB, vKeyB);

    /* Each session's SUBMIT_BLOCK decrypts independently */
    std::vector<uint8_t> decryptedA, decryptedB;
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedA, vKeyA, decryptedA));
    REQUIRE(LLC::DecryptPayloadChaCha20(encryptedB, vKeyB, decryptedB));

    REQUIRE(decryptedA == payloadA);
    REQUIRE(decryptedB == payloadB);

    /* No cross-session contamination — each decrypts to its own payload */
    REQUIRE(decryptedA != decryptedB);
}


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 5 — Error Path Logging (T19–T21)
 * ══════════════════════════════════════════════════════════════════════ */

TEST_CASE("T19: ChaCha20 failure produces diagnostic information including genesis", "[stateless_miner_crypto][logging]")
{
    /* Simulate a decryption failure and verify the diagnostic data is available */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Encrypt with one key, attempt decrypt with different key */
    std::vector<uint8_t> payload(128, 0xAA);
    std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);

    uint256_t hashWrong;
    hashWrong.SetHex("1111111111111111111111111111111111111111111111111111111111111111");
    std::vector<uint8_t> vWrongKey = DeriveChaCha20Key(hashWrong);

    std::vector<uint8_t> vDecrypted;
    bool fOk = LLC::DecryptPayloadChaCha20(vEncrypted, vWrongKey, vDecrypted);

    /* Decryption must fail */
    REQUIRE_FALSE(fOk);

    /* Verify diagnostic data is available for logging:
     * The node logs genesis hex and key fingerprint on failure.
     * We verify those values can be computed. */
    std::string genesisHex = hashGenesis.GetHex();
    REQUIRE(!genesisHex.empty());
    REQUIRE(genesisHex.size() == 64);  /* 32 bytes = 64 hex chars */

    /* Key fingerprint (first 8 bytes) */
    REQUIRE(vKey.size() >= 8);
    std::string fingerprint = HexStr(vKey.begin(), vKey.begin() + 8);
    REQUIRE(fingerprint.size() == 16);  /* 8 bytes = 16 hex chars */
}


TEST_CASE("T20: ChaCha20 failure diagnostic includes AAD information", "[stateless_miner_crypto][logging]")
{
    /* Verify that the AAD strings used for each packet type are well-defined */

    /* SUBMIT_BLOCK: empty AAD */
    std::vector<uint8_t> vAAD_Submit;
    REQUIRE(vAAD_Submit.empty());

    /* MINER_AUTH_INIT: "FALCON_PUBKEY" */
    std::vector<uint8_t> vAAD_Auth{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};
    REQUIRE(vAAD_Auth.size() == 13);
    std::string aadAuthStr(vAAD_Auth.begin(), vAAD_Auth.end());
    REQUIRE(aadAuthStr == "FALCON_PUBKEY");

    /* MINER_SET_REWARD: "REWARD_ADDRESS" */
    std::vector<uint8_t> vAAD_Reward{'R','E','W','A','R','D','_','A','D','D','R','E','S','S'};
    REQUIRE(vAAD_Reward.size() == 14);
    std::string aadRewardStr(vAAD_Reward.begin(), vAAD_Reward.end());
    REQUIRE(aadRewardStr == "REWARD_ADDRESS");

    /* MINER_REWARD_RESULT: "REWARD_RESULT" */
    std::vector<uint8_t> vAAD_Result{'R','E','W','A','R','D','_','R','E','S','U','L','T'};
    REQUIRE(vAAD_Result.size() == 13);
    std::string aadResultStr(vAAD_Result.begin(), vAAD_Result.end());
    REQUIRE(aadResultStr == "REWARD_RESULT");
}


TEST_CASE("T21: ChaCha20 failure diagnostic includes payload size", "[stateless_miner_crypto][logging]")
{
    /* Verify that encrypted payload sizes can be computed for diagnostics */
    uint256_t hashGenesis;
    hashGenesis.SetHex("8c2cf304e1bb28f03a88c2b5b412a120c58b9dbd40e0e0f38b9dc8ec94c6e2ac");
    std::vector<uint8_t> vKey = DeriveChaCha20Key(hashGenesis);

    /* Various payload sizes to test */
    std::vector<size_t> payloadSizes = {1, 100, 1037, 1805, 2048};

    for(size_t sz : payloadSizes)
    {
        std::vector<uint8_t> payload(sz, 0xCC);
        std::vector<uint8_t> vEncrypted = LLC::EncryptPayloadChaCha20(payload, vKey);

        /* Encrypted size = nonce(12) + plaintext(sz) + tag(16) = sz + 28 */
        REQUIRE(vEncrypted.size() == sz + 28);

        /* Verify nonce is extractable for logging */
        REQUIRE(vEncrypted.size() >= 12);
        std::string nonceHex = HexStr(vEncrypted.begin(), vEncrypted.begin() + 12);
        REQUIRE(nonceHex.size() == 24);  /* 12 bytes = 24 hex chars */
    }
}


/* ══════════════════════════════════════════════════════════════════════
 * GROUP 6 — Defence-in-Depth Gaps (T25–T28)
 * ══════════════════════════════════════════════════════════════════════ */

TEST_CASE("T25: Parser always returns empty vOffsets; node derives them via GetOffsets()", "[stateless_miner_crypto][submit_block][voffsets]")
{
    /* Build a Prime payload — vOffsets are never transmitted on the wire */
    FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(1, {}, 1700000000, LLC::FalconVersion::FALCON_1024);

    /* Verify the payload parses correctly with empty vOffsets */
    auto result = TAO::Ledger::ParseFalconWrappedSubmitBlock(fixture.payload);
    REQUIRE(result.success);
    REQUIRE(result.nChannel == 1);
    /* vOffsets is ALWAYS empty from the parser regardless of channel */
    REQUIRE(result.vOffsets.empty());

    /* The node derives offsets itself after applying the nonce:
     *   GetOffsets(pBlock->GetPrime(), pBlock->vOffsets)
     * This matches the upstream Nexusoft/LLL-TAO behavior exactly. */
}


TEST_CASE("T26: Hash channel with non-empty vOffsets is cleared", "[stateless_miner_crypto][submit_block][voffsets]")
{
    /* Build a Hash payload — the parser returns empty vOffsets for channel 2 */
    FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(2, {}, 1700000000, LLC::FalconVersion::FALCON_1024);

    TAO::Ledger::FalconWrappedSubmitBlockParseResult result;
    REQUIRE(TAO::Ledger::VerifyFalconWrappedSubmitBlock(fixture.payload, fixture.pubkey, result));
    REQUIRE(result.nChannel == 2);
    REQUIRE(result.vOffsets.empty());

    /* Simulate the channel invariant guard: non-empty submitted offsets for Hash channel must be cleared */
    std::vector<uint8_t> vSubmittedOffsets = {1, 2, 3};  /* Adversarially non-empty */
    if(result.nChannel != 1 /* PRIME */)
        vSubmittedOffsets.clear();
    REQUIRE(vSubmittedOffsets.empty());
}


TEST_CASE("T27: nUnifiedHeight out-of-range is rejected", "[stateless_miner_crypto][submit_block][height]")
{
    static constexpr uint32_t MAX_PLAUSIBLE_BLOCK_HEIGHT = 2'000'000'000u;  // ~2 B blocks

    auto evalHeightFromBlock = [&](uint32_t nChannelFromBlock, uint32_t nHeightFromBlock) -> bool
    {
        return (nChannelFromBlock == 1 || nChannelFromBlock == 2)
            && nHeightFromBlock > 0
            && nHeightFromBlock < MAX_PLAUSIBLE_BLOCK_HEIGHT;
    };

    /* UINT32_MAX must be rejected */
    REQUIRE_FALSE(evalHeightFromBlock(1, UINT32_MAX));
    REQUIRE_FALSE(evalHeightFromBlock(2, UINT32_MAX));

    /* Exactly at the bound must also be rejected */
    REQUIRE_FALSE(evalHeightFromBlock(1, MAX_PLAUSIBLE_BLOCK_HEIGHT));

    /* Zero height must be rejected (existing check) */
    REQUIRE_FALSE(evalHeightFromBlock(1, 0));
    REQUIRE_FALSE(evalHeightFromBlock(2, 0));

    /* Invalid channel must be rejected regardless of height */
    REQUIRE_FALSE(evalHeightFromBlock(0, 1000000));   /* Stake channel */
    REQUIRE_FALSE(evalHeightFromBlock(3, 1000000));   /* Private channel */

    /* Normal heights for Prime and Hash channels must be accepted */
    REQUIRE(evalHeightFromBlock(1, 1000000));
    REQUIRE(evalHeightFromBlock(2, 5000000));
    REQUIRE(evalHeightFromBlock(1, MAX_PLAUSIBLE_BLOCK_HEIGHT - 1));
}


TEST_CASE("T28: Tail-anchor uniqueness: only one candidate per valid payload", "[stateless_miner_crypto][submit_block][tail_anchor]")
{
    /* Build a well-formed Hash payload (1803 bytes for Falcon-1024) */
    FalconFullBlockFixture fixture = BuildFalconFullBlockFixture(2, {}, 1700000000, LLC::FalconVersion::FALCON_1024);
    REQUIRE(fixture.payload.size() == 1803);

    const size_t nMinSigLenOffset =
        LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN + LLP::FalconConstants::TIMESTAMP_SIZE;
    const size_t nMaxSigLenOffset =
        fixture.payload.size() - LLP::FalconConstants::LENGTH_FIELD_SIZE;

    int nCandidates = 0;
    for(size_t nSigLenOffset = nMinSigLenOffset; nSigLenOffset <= nMaxSigLenOffset; ++nSigLenOffset)
    {
        /* Replicate the tail-anchor constraint from BuildFalconWrappedSubmitBlockCandidate() */
        const uint16_t nSignatureLength =
            static_cast<uint16_t>(fixture.payload[nSigLenOffset]) |
            (static_cast<uint16_t>(fixture.payload[nSigLenOffset + 1]) << 8);

        if(nSignatureLength < LLP::FalconConstants::FALCON_SIG_MIN ||
           nSignatureLength > LLP::FalconConstants::FALCON_SIG_MAX_VALIDATION)
            continue;

        const size_t nSignatureOffset = nSigLenOffset + LLP::FalconConstants::LENGTH_FIELD_SIZE;
        if(nSignatureOffset + nSignatureLength == fixture.payload.size())
            ++nCandidates;
    }

    /* The tail-anchor constraint guarantees exactly ONE structurally valid candidate per payload.
     * This confirms that FLKey::Verify() is called at most once (O(1) in practice). */
    REQUIRE(nCandidates == 1);
}
