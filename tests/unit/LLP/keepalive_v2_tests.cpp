/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/keepalive_v2.h>

#include <cstdint>
#include <vector>

/*
 * Unit tests for KeepaliveV2 utility.
 *
 * Covers:
 *   - ParsePayload: v1 (len==4) and v2 (len==8), edge cases
 *   - BuildBestCurrentResponse: correct size (28 bytes), field positions, endianness
 */

TEST_CASE("KeepaliveV2::ParsePayload", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("v1 payload (len==4) returns false and zero suffix")
    {
        std::vector<uint8_t> data = { 0x01, 0x02, 0x03, 0x04 };

        uint32_t nSessionId = 0xFFFFFFFF;
        uint32_t nSuffix    = 0xFFFFFFFF;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == false);
        /* session_id = 0x04030201 (little-endian) */
        REQUIRE(nSessionId == 0x04030201u);
        REQUIRE(nSuffix == 0u);
    }

    SECTION("v2 payload (len==8) returns true and correct suffix")
    {
        /* session_id  = 0xDEADBEEF (LE: EF BE AD DE)
         * suffix      = 0x12345678 (LE: 78 56 34 12) */
        std::vector<uint8_t> data = {
            0xEF, 0xBE, 0xAD, 0xDE,   /* session_id LE */
            0x78, 0x56, 0x34, 0x12    /* prevblock_suffix LE */
        };

        uint32_t nSessionId = 0;
        uint32_t nSuffix    = 0;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 0xDEADBEEFu);
        REQUIRE(nSuffix    == 0x12345678u);
    }

    SECTION("v2 payload with zero suffix (no template)")
    {
        std::vector<uint8_t> data = {
            0x01, 0x00, 0x00, 0x00,   /* session_id = 1 */
            0x00, 0x00, 0x00, 0x00    /* suffix = 0 */
        };

        uint32_t nSessionId = 0;
        uint32_t nSuffix    = 0xFFFFFFFF;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 1u);
        REQUIRE(nSuffix    == 0u);
    }

    SECTION("Payload shorter than 4 bytes returns false")
    {
        std::vector<uint8_t> data = { 0x01, 0x02, 0x03 };

        uint32_t nSessionId = 0xAA;
        uint32_t nSuffix    = 0xBB;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == false);
        /* nSuffix must be zeroed even on short input */
        REQUIRE(nSuffix == 0u);
    }

    SECTION("Empty payload returns false")
    {
        std::vector<uint8_t> data;

        uint32_t nSessionId = 0xAA;
        uint32_t nSuffix    = 0xBB;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == false);
        REQUIRE(nSuffix == 0u);
    }

    SECTION("Payload exactly 5..7 bytes treated as v1 (no suffix)")
    {
        std::vector<uint8_t> data = { 0x04, 0x03, 0x02, 0x01, 0xFF, 0xEE, 0xDD };

        uint32_t nSessionId = 0;
        uint32_t nSuffix    = 0xFFFFFFFF;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == false);
        REQUIRE(nSessionId == 0x01020304u);
        /* suffix must be zero because we did not have 8 bytes */
        REQUIRE(nSuffix == 0u);
    }
}


TEST_CASE("KeepaliveV2::BuildBestCurrentResponse", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("Response is exactly 28 bytes")
    {
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(1, 2, 3, 4, 5, 6, hash);
        REQUIRE(v.size() == 28u);
    }

    SECTION("session_id encoded little-endian at bytes [0..3]")
    {
        /* session_id = 0xDEADBEEF
         * Expected LE bytes: EF BE AD DE */
        uint32_t nSessionId = 0xDEADBEEFu;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(nSessionId, 0, 0, 0, 0, 0, hash);

        REQUIRE(v[0] == 0xEFu);
        REQUIRE(v[1] == 0xBEu);
        REQUIRE(v[2] == 0xADu);
        REQUIRE(v[3] == 0xDEu);
    }

    SECTION("unified_height encoded big-endian at bytes [4..7]")
    {
        /* unified_height = 0x01020304 → BE bytes: 01 02 03 04 */
        uint32_t nUnifiedHeight = 0x01020304u;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(0, nUnifiedHeight, 0, 0, 0, 0, hash);

        REQUIRE(v[4] == 0x01u);
        REQUIRE(v[5] == 0x02u);
        REQUIRE(v[6] == 0x03u);
        REQUIRE(v[7] == 0x04u);
    }

    SECTION("prime_height encoded big-endian at bytes [8..11]")
    {
        uint32_t nPrimeHeight = 0xAABBCCDDu;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(0, 0, nPrimeHeight, 0, 0, 0, hash);

        REQUIRE(v[8]  == 0xAAu);
        REQUIRE(v[9]  == 0xBBu);
        REQUIRE(v[10] == 0xCCu);
        REQUIRE(v[11] == 0xDDu);
    }

    SECTION("hash_height encoded big-endian at bytes [12..15]")
    {
        uint32_t nHashHeight = 0x11223344u;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(0, 0, 0, nHashHeight, 0, 0, hash);

        REQUIRE(v[12] == 0x11u);
        REQUIRE(v[13] == 0x22u);
        REQUIRE(v[14] == 0x33u);
        REQUIRE(v[15] == 0x44u);
    }

    SECTION("stake_height encoded big-endian at bytes [16..19]")
    {
        uint32_t nStakeHeight = 0xFEDCBA98u;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(0, 0, 0, 0, nStakeHeight, 0, hash);

        REQUIRE(v[16] == 0xFEu);
        REQUIRE(v[17] == 0xDCu);
        REQUIRE(v[18] == 0xBAu);
        REQUIRE(v[19] == 0x98u);
    }

    SECTION("nBits encoded big-endian at bytes [20..23]")
    {
        uint32_t nBits = 0x1D00FFFFu;
        uint1024_t hash;
        std::vector<uint8_t> v = BuildBestCurrentResponse(0, 0, 0, 0, 0, nBits, hash);

        REQUIRE(v[20] == 0x1Du);
        REQUIRE(v[21] == 0x00u);
        REQUIRE(v[22] == 0xFFu);
        REQUIRE(v[23] == 0xFFu);
    }

    SECTION("hashBestChain_prefix occupies bytes [24..27] (first 4 bytes of GetBytes())")
    {
        /* Construct a uint1024_t where the first 4 bytes of GetBytes() are known.
         * GetBytes() returns the internal limb storage in little-endian word order.
         * The simplest way is to set the lowest-order limb to a known value. */
        uint1024_t hash(0u);

        /* Set to a known 32-bit value so the first limb has predictable bytes */
        hash.SetHex("AABBCCDD");  /* lowest limb = 0x0000...AABBCCDD */

        std::vector<uint8_t> v = BuildBestCurrentResponse(0, 0, 0, 0, 0, 0, hash);

        /* GetBytes() returns LE bytes of the internal representation.
         * For a value 0xAABBCCDD the first 4 bytes should be DD CC BB AA (LE). */
        std::vector<uint8_t> vHashBytes = hash.GetBytes();
        REQUIRE(vHashBytes.size() >= 4u);

        REQUIRE(v[24] == vHashBytes[0]);
        REQUIRE(v[25] == vHashBytes[1]);
        REQUIRE(v[26] == vHashBytes[2]);
        REQUIRE(v[27] == vHashBytes[3]);
    }

    SECTION("All fields together - full round-trip sanity check")
    {
        uint32_t nSessionId     = 0x00000042u;
        uint32_t nUnifiedHeight = 100u;
        uint32_t nPrimeHeight   = 200u;
        uint32_t nHashHeight    = 300u;
        uint32_t nStakeHeight   = 400u;
        uint32_t nBits          = 0x1D00FFFFu;
        uint1024_t hash(0u);

        std::vector<uint8_t> v = BuildBestCurrentResponse(
            nSessionId, nUnifiedHeight, nPrimeHeight,
            nHashHeight, nStakeHeight, nBits, hash);

        REQUIRE(v.size() == 28u);

        /* session_id LE */
        REQUIRE(v[0] == 0x42u);
        REQUIRE(v[1] == 0x00u);
        REQUIRE(v[2] == 0x00u);
        REQUIRE(v[3] == 0x00u);

        /* unified_height BE */
        REQUIRE(v[4] == 0x00u);
        REQUIRE(v[5] == 0x00u);
        REQUIRE(v[6] == 0x00u);
        REQUIRE(v[7] == 100u);

        /* prime_height BE */
        REQUIRE(v[8]  == 0x00u);
        REQUIRE(v[9]  == 0x00u);
        REQUIRE(v[10] == 0x00u);
        REQUIRE(v[11] == 200u);

        /* hash_height BE */
        REQUIRE(v[12] == 0x00u);
        REQUIRE(v[13] == 0x00u);
        REQUIRE(v[14] == 0x01u);   /* 300 = 0x0000012C */
        REQUIRE(v[15] == 0x2Cu);

        /* stake_height BE */
        REQUIRE(v[16] == 0x00u);
        REQUIRE(v[17] == 0x00u);
        REQUIRE(v[18] == 0x01u);   /* 400 = 0x00000190 */
        REQUIRE(v[19] == 0x90u);

        /* nBits BE */
        REQUIRE(v[20] == 0x1Du);
        REQUIRE(v[21] == 0x00u);
        REQUIRE(v[22] == 0xFFu);
        REQUIRE(v[23] == 0xFFu);
    }
}


TEST_CASE("KeepaliveV2 - backward compatibility", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("v1 keepalive leaves suffix at 0 (does not write into miner prevblock slot)")
    {
        std::vector<uint8_t> data = { 0xAA, 0xBB, 0xCC, 0xDD };

        uint32_t nSessionId = 0;
        uint32_t nSuffix    = 0x12345678u;  /* pre-loaded with garbage */

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == false);
        REQUIRE(nSuffix == 0u);  /* MUST be zeroed */
    }

    SECTION("v2 keepalive with zero suffix is valid (miner has no template yet)")
    {
        std::vector<uint8_t> data = {
            0x01, 0x00, 0x00, 0x00,  /* session_id = 1 */
            0x00, 0x00, 0x00, 0x00   /* suffix = 0 */
        };

        uint32_t nSessionId = 0;
        uint32_t nSuffix    = 0xDEAD;

        bool fIsV2 = ParsePayload(data, nSessionId, nSuffix);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSuffix == 0u);
    }
}
