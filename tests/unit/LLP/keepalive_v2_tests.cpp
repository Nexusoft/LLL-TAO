/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/keepalive_v2.h>

#include <array>
#include <cstdint>
#include <vector>

/*
 * Unit tests for KeepaliveV2 utility.
 *
 * Covers:
 *   - ParsePayload: requires exactly 8 bytes (v1 4-byte no longer supported)
 *   - ParsePayload: session_id parsed as big-endian, suffix returned as raw bytes
 *   - BuildUnifiedResponse: correct size (32 bytes), field positions, all big-endian
 */

TEST_CASE("KeepaliveV2::ParsePayload", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("4-byte payload returns false (must be 8 bytes)")
    {
        std::vector<uint8_t> data = { 0x01, 0x02, 0x03, 0x04 };

        uint32_t nSessionId = 0xFFFFFFFF;
        std::array<uint8_t, 4> suffixBytes = {0xFF, 0xFF, 0xFF, 0xFF};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == false);
        /* suffix bytes must all be zeroed on rejection */
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }

    SECTION("8-byte payload returns true with session_id and suffix parsed")
    {
        /* session_id  = 0xDEADBEEF (BE: DE AD BE EF)
         * suffix bytes as-sent: 78 56 34 12 */
        std::vector<uint8_t> data = {
            0xDE, 0xAD, 0xBE, 0xEF,   /* session_id BE */
            0x78, 0x56, 0x34, 0x12    /* prevblock_suffix raw bytes */
        };

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 0xDEADBEEFu);
        /* Bytes returned exactly as-sent (no endian conversion) */
        REQUIRE(suffixBytes[0] == 0x78u);
        REQUIRE(suffixBytes[1] == 0x56u);
        REQUIRE(suffixBytes[2] == 0x34u);
        REQUIRE(suffixBytes[3] == 0x12u);
    }

    SECTION("8-byte payload with zero suffix (no template)")
    {
        std::vector<uint8_t> data = {
            0x00, 0x00, 0x00, 0x01,   /* session_id = 1 (BE) */
            0x00, 0x00, 0x00, 0x00    /* suffix = all zeros */
        };

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {0xFF, 0xFF, 0xFF, 0xFF};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 1u);
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }

    SECTION("Payload shorter than 8 bytes returns false")
    {
        std::vector<uint8_t> data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };

        uint32_t nSessionId = 0xAA;
        std::array<uint8_t, 4> suffixBytes = {0xBB, 0xBB, 0xBB, 0xBB};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == false);
        /* suffix bytes must be zeroed even on short input */
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }

    SECTION("Empty payload returns false")
    {
        std::vector<uint8_t> data;

        uint32_t nSessionId = 0xAA;
        std::array<uint8_t, 4> suffixBytes = {0xBB, 0xBB, 0xBB, 0xBB};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == false);
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }

    SECTION("suffix bytes preserve wire-order (no endian swap)")
    {
        /* Bytes [4..7] on the wire: AA BB CC DD
         * Expected: suffixBytes[0]=AA, [1]=BB, [2]=CC, [3]=DD */
        std::vector<uint8_t> data = {
            0x00, 0x00, 0x00, 0x01,  /* session_id = 1 (BE) */
            0xAA, 0xBB, 0xCC, 0xDD   /* suffix bytes in wire order */
        };

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 1u);
        REQUIRE(suffixBytes[0] == 0xAAu);
        REQUIRE(suffixBytes[1] == 0xBBu);
        REQUIRE(suffixBytes[2] == 0xCCu);
        REQUIRE(suffixBytes[3] == 0xDDu);
    }
}


TEST_CASE("KeepaliveV2::BuildUnifiedResponse", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("Response is exactly 32 bytes")
    {
        auto v = BuildUnifiedResponse(1, 0, 100, 0xCAFEBABEu, 10, 20, 30, 0);
        REQUIRE(v.size() == 32u);
    }

    SECTION("session_id encoded big-endian at bytes [0-3]")
    {
        /* session_id = 0xDEADBEEF
         * Expected BE bytes: DE AD BE EF */
        auto v = BuildUnifiedResponse(0xDEADBEEFu, 0, 0, 0, 0, 0, 0, 0);
        REQUIRE(v[0] == 0xDEu);
        REQUIRE(v[1] == 0xADu);
        REQUIRE(v[2] == 0xBEu);
        REQUIRE(v[3] == 0xEFu);
    }

    SECTION("hashPrevBlock_lo32 big-endian at bytes [4-7]")
    {
        auto v = BuildUnifiedResponse(0, 0x12345678u, 0, 0, 0, 0, 0, 0);
        REQUIRE(v[4] == 0x12u);
        REQUIRE(v[5] == 0x34u);
        REQUIRE(v[6] == 0x56u);
        REQUIRE(v[7] == 0x78u);
    }

    SECTION("unified_height encoded big-endian at bytes [8-11]")
    {
        /* unified_height = 0x01020304 → BE bytes: 01 02 03 04 */
        auto v = BuildUnifiedResponse(0, 0, 0x01020304u, 0, 0, 0, 0, 0);
        REQUIRE(v[8]  == 0x01u);
        REQUIRE(v[9]  == 0x02u);
        REQUIRE(v[10] == 0x03u);
        REQUIRE(v[11] == 0x04u);
    }

    SECTION("hash_tip_lo32 encoded big-endian at bytes [12-15]")
    {
        auto v = BuildUnifiedResponse(0, 0, 0, 0xAABBCCDDu, 0, 0, 0, 0);
        REQUIRE(v[12] == 0xAAu);
        REQUIRE(v[13] == 0xBBu);
        REQUIRE(v[14] == 0xCCu);
        REQUIRE(v[15] == 0xDDu);
    }

    SECTION("prime_height encoded big-endian at bytes [16-19]")
    {
        auto v = BuildUnifiedResponse(0, 0, 0, 0, 0xAABBCCDDu, 0, 0, 0);
        REQUIRE(v[16] == 0xAAu);
        REQUIRE(v[17] == 0xBBu);
        REQUIRE(v[18] == 0xCCu);
        REQUIRE(v[19] == 0xDDu);
    }

    SECTION("hash_height encoded big-endian at bytes [20-23]")
    {
        auto v = BuildUnifiedResponse(0, 0, 0, 0, 0, 0x11223344u, 0, 0);
        REQUIRE(v[20] == 0x11u);
        REQUIRE(v[21] == 0x22u);
        REQUIRE(v[22] == 0x33u);
        REQUIRE(v[23] == 0x44u);
    }

    SECTION("stake_height encoded big-endian at bytes [24-27]")
    {
        auto v = BuildUnifiedResponse(0, 0, 0, 0, 0, 0, 0xFEDCBA98u, 0);
        REQUIRE(v[24] == 0xFEu);
        REQUIRE(v[25] == 0xDCu);
        REQUIRE(v[26] == 0xBAu);
        REQUIRE(v[27] == 0x98u);
    }

    SECTION("fork_score encoded big-endian at bytes [28-31]")
    {
        auto v = BuildUnifiedResponse(0, 0, 0, 0, 0, 0, 0, 0x1D00FFFFu);
        REQUIRE(v[28] == 0x1Du);
        REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0xFFu);
        REQUIRE(v[31] == 0xFFu);
    }

    SECTION("legacy path: hashPrevBlock_lo32=0, fork_score=0")
    {
        /* Verify legacy path passes zeros for fields not available on legacy path */
        auto v = BuildUnifiedResponse(1, 0u, 6000000u, 0xCAFEBABEu, 450u, 800u, 999u, 0u);
        REQUIRE(v.size() == 32u);
        /* hashPrevBlock_lo32 = 0 */
        REQUIRE(v[4] == 0x00u); REQUIRE(v[5] == 0x00u);
        REQUIRE(v[6] == 0x00u); REQUIRE(v[7] == 0x00u);
        /* fork_score = 0 */
        REQUIRE(v[28] == 0x00u); REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u); REQUIRE(v[31] == 0x00u);
    }

    SECTION("full round-trip all fields")
    {
        auto v = BuildUnifiedResponse(
            0x00000001u,   // session_id
            0xDEADBEEFu,   // hashPrevBlock_lo32
            6000000u,      // unified_height
            0xCAFEBABEu,   // hash_tip_lo32
            450u,          // prime_height
            800u,          // hash_height
            999u,          // stake_height
            7u);           // fork_score
        REQUIRE(v.size() == 32u);
        /* session_id BE: 00 00 00 01 */
        REQUIRE(v[0] == 0x00u); REQUIRE(v[1] == 0x00u);
        REQUIRE(v[2] == 0x00u); REQUIRE(v[3] == 0x01u);
        /* hashPrevBlock_lo32 BE: DE AD BE EF */
        REQUIRE(v[4] == 0xDEu); REQUIRE(v[5] == 0xADu);
        REQUIRE(v[6] == 0xBEu); REQUIRE(v[7] == 0xEFu);
        /* stake_height = 999 = 0x000003E7 at [24-27] */
        REQUIRE(v[24] == 0x00u); REQUIRE(v[25] == 0x00u);
        REQUIRE(v[26] == 0x03u); REQUIRE(v[27] == 0xE7u);
        /* fork_score = 7 at [28-31] */
        REQUIRE(v[28] == 0x00u); REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u); REQUIRE(v[31] == 0x07u);
    }
}


TEST_CASE("KeepaliveV2 - backward compatibility", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("8-byte keepalive with zero suffix is valid (miner has no template yet)")
    {
        std::vector<uint8_t> data = {
            0x00, 0x00, 0x00, 0x01,  /* session_id = 1 (BE) */
            0x00, 0x00, 0x00, 0x00   /* suffix = 0 */
        };

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {0xDE, 0xAD, 0x00, 0x00};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 1u);
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }
}


TEST_CASE("KeepaliveV2 legacy path request-parsing", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("4-byte request: ParsePayload returns false (must send 8 bytes)")
    {
        /* Old miners send only session_id (4 bytes); now rejected — must send 8 bytes */
        std::vector<uint8_t> data = { 0x00, 0x00, 0x00, 0x01 };   /* session_id = 1 (BE) */

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {0xFF, 0xFF, 0xFF, 0xFF};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == false);
        /* suffix bytes zeroed on rejection */
        REQUIRE(suffixBytes[0] == 0u);
        REQUIRE(suffixBytes[1] == 0u);
        REQUIRE(suffixBytes[2] == 0u);
        REQUIRE(suffixBytes[3] == 0u);
    }

    SECTION("8-byte request: nMinerPrevHashLo32 derived as BE uint32 from raw suffix bytes")
    {
        /* Miners send session_id (BE) + hashPrevBlock_lo32 (BE on wire).
         * Both legacy and stateless paths now echo this value and compute fork_score. */
        std::vector<uint8_t> data = {
            0x00, 0x00, 0x00, 0x05,   /* session_id = 5 (BE) */
            0xDE, 0xAD, 0xBE, 0xEF    /* hashPrevBlock_lo32 = 0xDEADBEEF (BE on wire) */
        };

        uint32_t nSessionId = 0;
        std::array<uint8_t, 4> suffixBytes = {};

        bool fIsV2 = ParsePayload(data, nSessionId, suffixBytes);

        REQUIRE(fIsV2 == true);
        REQUIRE(nSessionId == 5u);

        /* Derive nMinerPrevHashLo32 as big-endian uint32 from raw bytes — matches miner.cpp logic */
        uint32_t nMinerPrevHashLo32 =
            (uint32_t(suffixBytes[0]) << 24) |
            (uint32_t(suffixBytes[1]) << 16) |
            (uint32_t(suffixBytes[2]) <<  8) |
             uint32_t(suffixBytes[3]);

        REQUIRE(nMinerPrevHashLo32 == 0xDEADBEEFu);

        /* Both lanes echo hashPrevBlock_lo32 and compute fork_score */
        uint32_t nHashTipLo32 = 0xCAFEBABEu;
        uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;
        auto vReply = BuildUnifiedResponse(nSessionId, nMinerPrevHashLo32, 6000000u, nHashTipLo32, 100u, 200u, 999u, nForkScore);
        REQUIRE(vReply.size() == 32u);
        /* hashPrevBlock_lo32 field [4..7] = 0xDEADBEEF echoed */
        REQUIRE(vReply[4] == 0xDEu); REQUIRE(vReply[5] == 0xADu);
        REQUIRE(vReply[6] == 0xBEu); REQUIRE(vReply[7] == 0xEFu);
        /* fork_score field [28..31] = 1 (miner prevhash != node tip) */
        REQUIRE(vReply[28] == 0x00u); REQUIRE(vReply[29] == 0x00u);
        REQUIRE(vReply[30] == 0x00u); REQUIRE(vReply[31] == 0x01u);
        /* stake_height = 999 = 0x000003E7 at [24..27] big-endian */
        REQUIRE(vReply[24] == 0x00u); REQUIRE(vReply[25] == 0x00u);
        REQUIRE(vReply[26] == 0x03u); REQUIRE(vReply[27] == 0xE7u);
    }

    SECTION("legacy path: fork_score=1 when miner prevhash != node tip")
    {
        /* Simulate: miner's prevhash_lo32 = 0xDEADBEEF, node's hashBestChain lo32 = 0xCAFEBABE */
        uint32_t nMinerPrevHashLo32 = 0xDEADBEEFu;
        uint32_t nHashTipLo32       = 0xCAFEBABEu;
        uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;

        auto v = BuildUnifiedResponse(1u, nMinerPrevHashLo32, 6000u, nHashTipLo32, 450u, 800u, 999u, nForkScore);
        REQUIRE(v.size() == 32u);
        /* hashPrevBlock_lo32 echoed at [4..7] big-endian */
        REQUIRE(v[4] == 0xDEu); REQUIRE(v[5] == 0xADu);
        REQUIRE(v[6] == 0xBEu); REQUIRE(v[7] == 0xEFu);
        /* fork_score = 1 at [28..31] big-endian */
        REQUIRE(v[28] == 0x00u); REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u); REQUIRE(v[31] == 0x01u);
    }

    SECTION("legacy path: fork_score=0 when miner prevhash matches node tip")
    {
        uint32_t nMinerPrevHashLo32 = 0xCAFEBABEu;
        uint32_t nHashTipLo32       = 0xCAFEBABEu;
        uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;

        auto v = BuildUnifiedResponse(1u, nMinerPrevHashLo32, 6000u, nHashTipLo32, 450u, 800u, 999u, nForkScore);
        REQUIRE(nForkScore == 0u);
        REQUIRE(v[28] == 0x00u); REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u); REQUIRE(v[31] == 0x00u);
    }

    SECTION("legacy path: fork_score=0 when miner prevhash is 0 (v1 miner)")
    {
        uint32_t nMinerPrevHashLo32 = 0u;  /* v1 miner sends no canary */
        uint32_t nHashTipLo32       = 0xCAFEBABEu;
        uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;

        REQUIRE(nForkScore == 0u);  /* v1 miners never trigger fork alert */
    }
}


TEST_CASE("KeepaliveV2::KeepAliveV2AckFrame::Serialize", "[keepalive_v2][llp]")
{
    using namespace LLP::KeepaliveV2;

    SECTION("Serialize produces exactly 32 bytes")
    {
        KeepAliveV2AckFrame ack;
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v.size() == 32u);
    }

    SECTION("PAYLOAD_SIZE constant is 32")
    {
        REQUIRE(KeepAliveV2AckFrame::PAYLOAD_SIZE == 32u);
    }

    SECTION("session_id encoded big-endian at bytes [0-3]")
    {
        KeepAliveV2AckFrame ack;
        ack.session_id = 0xDEADBEEFu;
        std::vector<uint8_t> v = ack.Serialize();
        // BE: DE AD BE EF
        REQUIRE(v[0] == 0xDEu);
        REQUIRE(v[1] == 0xADu);
        REQUIRE(v[2] == 0xBEu);
        REQUIRE(v[3] == 0xEFu);
    }

    SECTION("hashPrevBlock_lo32 encoded big-endian at bytes [4-7]")
    {
        KeepAliveV2AckFrame ack;
        ack.hashPrevBlock_lo32 = 0x12345678u;
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[4] == 0x12u);
        REQUIRE(v[5] == 0x34u);
        REQUIRE(v[6] == 0x56u);
        REQUIRE(v[7] == 0x78u);
    }

    SECTION("unified_height encoded big-endian at bytes [8-11]")
    {
        KeepAliveV2AckFrame ack;
        ack.unified_height = 0x01020304u;
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[8]  == 0x01u);
        REQUIRE(v[9]  == 0x02u);
        REQUIRE(v[10] == 0x03u);
        REQUIRE(v[11] == 0x04u);
    }

    SECTION("hash_tip_lo32 encoded big-endian at bytes [12-15]")
    {
        KeepAliveV2AckFrame ack;
        ack.hash_tip_lo32 = 0xAABBCCDDu;
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[12] == 0xAAu);
        REQUIRE(v[13] == 0xBBu);
        REQUIRE(v[14] == 0xCCu);
        REQUIRE(v[15] == 0xDDu);
    }

    SECTION("prime_height encoded big-endian at bytes [16-19]")
    {
        KeepAliveV2AckFrame ack;
        ack.prime_height = 0x00000064u;  /* 100 */
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[16] == 0x00u);
        REQUIRE(v[17] == 0x00u);
        REQUIRE(v[18] == 0x00u);
        REQUIRE(v[19] == 100u);
    }

    SECTION("hash_height encoded big-endian at bytes [20-23]")
    {
        KeepAliveV2AckFrame ack;
        ack.hash_height = 0x00000096u;  /* 150 */
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[20] == 0x00u);
        REQUIRE(v[21] == 0x00u);
        REQUIRE(v[22] == 0x00u);
        REQUIRE(v[23] == 150u);
    }

    SECTION("stake_height encoded big-endian at bytes [24-27]")
    {
        KeepAliveV2AckFrame ack;
        ack.stake_height = 0xAABBCCDDu;
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[24] == 0xAAu);
        REQUIRE(v[25] == 0xBBu);
        REQUIRE(v[26] == 0xCCu);
        REQUIRE(v[27] == 0xDDu);
    }

    SECTION("fork_score encoded big-endian at bytes [28-31]")
    {
        KeepAliveV2AckFrame ack;
        ack.fork_score = 0x00000000u;  /* 0 = healthy */
        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v[28] == 0x00u);
        REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u);
        REQUIRE(v[31] == 0x00u);
    }

    SECTION("All fields together - full round-trip sanity check")
    {
        KeepAliveV2AckFrame ack;
        ack.session_id         = 0x00000001u;
        ack.hashPrevBlock_lo32 = 0xDEADBEEFu;
        ack.unified_height     = 1000u;
        ack.hash_tip_lo32      = 0xCAFEBABEu;
        ack.prime_height       = 500u;
        ack.hash_height        = 600u;
        ack.stake_height       = 999u;
        ack.fork_score         = 0u;

        std::vector<uint8_t> v = ack.Serialize();
        REQUIRE(v.size() == 32u);

        /* session_id BE: 00 00 00 01 */
        REQUIRE(v[0] == 0x00u); REQUIRE(v[1] == 0x00u);
        REQUIRE(v[2] == 0x00u); REQUIRE(v[3] == 0x01u);

        /* hashPrevBlock_lo32 BE */
        REQUIRE(v[4] == 0xDEu); REQUIRE(v[5] == 0xADu);
        REQUIRE(v[6] == 0xBEu); REQUIRE(v[7] == 0xEFu);

        /* unified_height = 1000 = 0x000003E8 BE */
        REQUIRE(v[8]  == 0x00u); REQUIRE(v[9]  == 0x00u);
        REQUIRE(v[10] == 0x03u); REQUIRE(v[11] == 0xE8u);

        /* hash_tip_lo32 BE */
        REQUIRE(v[12] == 0xCAu); REQUIRE(v[13] == 0xFEu);
        REQUIRE(v[14] == 0xBAu); REQUIRE(v[15] == 0xBEu);

        /* prime_height = 500 = 0x000001F4 BE */
        REQUIRE(v[16] == 0x00u); REQUIRE(v[17] == 0x00u);
        REQUIRE(v[18] == 0x01u); REQUIRE(v[19] == 0xF4u);

        /* hash_height = 600 = 0x00000258 BE */
        REQUIRE(v[20] == 0x00u); REQUIRE(v[21] == 0x00u);
        REQUIRE(v[22] == 0x02u); REQUIRE(v[23] == 0x58u);

        /* stake_height = 999 = 0x000003E7 BE */
        REQUIRE(v[24] == 0x00u); REQUIRE(v[25] == 0x00u);
        REQUIRE(v[26] == 0x03u); REQUIRE(v[27] == 0xE7u);

        /* fork_score = 0 */
        REQUIRE(v[28] == 0x00u); REQUIRE(v[29] == 0x00u);
        REQUIRE(v[30] == 0x00u); REQUIRE(v[31] == 0x00u);
    }
}
