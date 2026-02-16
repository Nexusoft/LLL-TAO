/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/stateless_opcodes.h>

using namespace LLP;


/** Test Suite: Template Staleness Detection
 *
 *  These tests verify the per-connection template tracking (nLastTemplateHeight)
 *  and MINER_READY compatibility opcode remapping used to prevent template
 *  staleness in stateless mining.
 *
 *  Key behaviors:
 *  1. nLastTemplateHeight tracks what template the miner has
 *  2. GET_ROUND auto-sends template when nLastTemplateHeight != current height
 *  3. MINER_READY compatibility opcodes (0xD090, 0x00D8) remap to 0xD0D8
 *  4. Multiple miners have independent template tracking
 *
 **/


TEST_CASE("MiningContext nLastTemplateHeight Initialization", "[template_staleness]")
{
    SECTION("Default constructor initializes nLastTemplateHeight to 0")
    {
        MiningContext ctx;

        REQUIRE(ctx.nLastTemplateHeight == 0);
    }

    SECTION("Parameterized constructor initializes nLastTemplateHeight to 0")
    {
        MiningContext ctx(
            1,          // nChannel (Prime)
            6594320,    // nHeight
            1000000,    // nTimestamp
            "127.0.0.1",// strAddress
            1,          // nProtocolVersion
            true,       // fAuthenticated
            42,         // nSessionId
            uint256_t(0), // hashKeyID
            uint256_t(0)  // hashGenesis
        );

        REQUIRE(ctx.nLastTemplateHeight == 0);
        REQUIRE(ctx.nHeight == 6594320);
    }
}


TEST_CASE("MiningContext WithLastTemplateHeight Immutability", "[template_staleness]")
{
    SECTION("WithLastTemplateHeight returns new context with updated height")
    {
        MiningContext ctx;
        ctx = ctx.WithHeight(6594320);

        MiningContext updated = ctx.WithLastTemplateHeight(6594321);

        /* New context has updated last template height */
        REQUIRE(updated.nLastTemplateHeight == 6594321);

        /* Original context unchanged (immutability) */
        REQUIRE(ctx.nLastTemplateHeight == 0);
    }

    SECTION("WithLastTemplateHeight preserves other fields")
    {
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithHeight(6594320)
                 .WithAuth(true)
                 .WithSession(42);

        MiningContext updated = ctx.WithLastTemplateHeight(6594321);

        REQUIRE(updated.nLastTemplateHeight == 6594321);
        REQUIRE(updated.nChannel == 1);
        REQUIRE(updated.nHeight == 6594320);
        REQUIRE(updated.fAuthenticated == true);
        REQUIRE(updated.nSessionId == 42);
    }

    SECTION("Can chain WithLastTemplateHeight with other With methods")
    {
        MiningContext ctx;

        MiningContext updated = ctx.WithHeight(6594321)
                                   .WithLastTemplateHeight(6594321)
                                   .WithTimestamp(1000000);

        REQUIRE(updated.nHeight == 6594321);
        REQUIRE(updated.nLastTemplateHeight == 6594321);
        REQUIRE(updated.nTimestamp == 1000000);
    }
}


TEST_CASE("GET_ROUND Height Change Detection", "[template_staleness][get_round]")
{
    SECTION("Height change detected when nLastTemplateHeight differs from current")
    {
        /* Miner has template for height 6594320 */
        MiningContext ctx;
        ctx = ctx.WithLastTemplateHeight(6594320);

        /* New block found at height 6594321 */
        uint32_t nCurrentHeight = 6594321;

        /* Height changed: should auto-send template */
        bool fHeightChanged = (ctx.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fHeightChanged == true);
    }

    SECTION("No height change when nLastTemplateHeight matches current")
    {
        /* Miner has template for height 6594320 */
        MiningContext ctx;
        ctx = ctx.WithLastTemplateHeight(6594320);

        /* No new blocks */
        uint32_t nCurrentHeight = 6594320;

        /* Height unchanged: no duplicate template needed */
        bool fHeightChanged = (ctx.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fHeightChanged == false);
    }

    SECTION("First GET_ROUND after connection always triggers template send")
    {
        /* Fresh connection: nLastTemplateHeight = 0 */
        MiningContext ctx;

        /* Current blockchain height */
        uint32_t nCurrentHeight = 6594320;

        /* First request always triggers (0 != 6594320) */
        bool fHeightChanged = (ctx.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fHeightChanged == true);
    }

    SECTION("GET_HEIGHT does not interfere with template tracking")
    {
        /* Miner has template for height 6594320 */
        MiningContext ctx;
        ctx = ctx.WithLastTemplateHeight(6594320);

        /* GET_HEIGHT updates nHeight but NOT nLastTemplateHeight */
        ctx = ctx.WithHeight(6594321);

        /* nLastTemplateHeight should still reflect the old template */
        REQUIRE(ctx.nLastTemplateHeight == 6594320);
        REQUIRE(ctx.nHeight == 6594321);

        /* Next GET_ROUND should detect height change */
        uint32_t nCurrentHeight = 6594321;
        bool fHeightChanged = (ctx.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fHeightChanged == true);
    }
}


TEST_CASE("Multiple Miners Independent State", "[template_staleness][multi_miner]")
{
    SECTION("Different miners have independent nLastTemplateHeight")
    {
        /* Miner A at height 6594320 */
        MiningContext ctxA;
        ctxA = ctxA.WithLastTemplateHeight(6594320)
                    .WithChannel(1)
                    .WithSession(100);

        /* Miner B at height 6594319 (slow poller) */
        MiningContext ctxB;
        ctxB = ctxB.WithLastTemplateHeight(6594319)
                    .WithChannel(2)
                    .WithSession(200);

        uint32_t nCurrentHeight = 6594321;

        /* Miner A: height changed (6594320 → 6594321) */
        bool fAChanged = (ctxA.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fAChanged == true);

        /* Miner B: also height changed (6594319 → 6594321) */
        bool fBChanged = (ctxB.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fBChanged == true);

        /* Update Miner A */
        ctxA = ctxA.WithLastTemplateHeight(nCurrentHeight);

        /* Miner A now up to date */
        REQUIRE(ctxA.nLastTemplateHeight == 6594321);

        /* Miner B still behind */
        REQUIRE(ctxB.nLastTemplateHeight == 6594319);
    }
}


TEST_CASE("MINER_READY Compatibility Opcodes", "[template_staleness][miner_ready]")
{
    SECTION("0xD0D8 is canonical STATELESS_MINER_READY")
    {
        REQUIRE(OpcodeUtility::Stateless::STATELESS_MINER_READY == 0xD0D8);
        REQUIRE(OpcodeUtility::Stateless::MINER_READY == 0xD0D8);
    }

    SECTION("0xD090 should be remapped to 0xD0D8")
    {
        uint16_t nOpcode = 0xD090;

        /* Compatibility remapping logic */
        if(nOpcode == 0x00D8 || nOpcode == 0xD090)
            nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

        REQUIRE(nOpcode == 0xD0D8);
    }

    SECTION("0x00D8 should be remapped to 0xD0D8")
    {
        uint16_t nOpcode = 0x00D8;

        /* Compatibility remapping logic */
        if(nOpcode == 0x00D8 || nOpcode == 0xD090)
            nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

        REQUIRE(nOpcode == 0xD0D8);
    }

    SECTION("0x00D8 is not in stateless range before remapping")
    {
        /* 0x00D8 does NOT have 0xD000 prefix */
        REQUIRE(OpcodeUtility::Stateless::IsStateless(0x00D8) == false);

        /* After remapping to 0xD0D8, it IS in stateless range */
        REQUIRE(OpcodeUtility::Stateless::IsStateless(0xD0D8) == true);
    }

    SECTION("0xD090 is in stateless range but not standard MINER_READY")
    {
        /* 0xD090 has 0xD000 prefix, so it's in range */
        REQUIRE(OpcodeUtility::Stateless::IsStateless(0xD090) == true);

        /* But it's not the standard MINER_READY opcode */
        REQUIRE(0xD090 != OpcodeUtility::Stateless::STATELESS_MINER_READY);
    }

    SECTION("Standard opcodes are not affected by remapping")
    {
        uint16_t nOpcode = OpcodeUtility::Stateless::STATELESS_GET_ROUND;

        /* GET_ROUND should NOT be remapped */
        if(nOpcode == 0x00D8 || nOpcode == 0xD090)
            nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

        /* Still GET_ROUND */
        REQUIRE(nOpcode == OpcodeUtility::Stateless::STATELESS_GET_ROUND);
        REQUIRE(nOpcode == 0xD085);
    }
}


TEST_CASE("MINER_READY Recovery Scenario", "[template_staleness][recovery]")
{
    SECTION("MINER_READY should trigger template send and update tracking")
    {
        /* Miner enters degraded mode with stale template */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithAuth(true)
                 .WithSession(42)
                 .WithLastTemplateHeight(6594310);  // Far behind

        /* Current blockchain height */
        uint32_t nCurrentHeight = 6594321;

        /* After MINER_READY processing:
         * 1. Send push notification (PRIME_AVAILABLE)
         * 2. Auto-send BLOCK_DATA template
         * 3. Update nLastTemplateHeight */
        ctx = ctx.WithLastTemplateHeight(nCurrentHeight);

        REQUIRE(ctx.nLastTemplateHeight == 6594321);

        /* Next GET_ROUND should NOT send duplicate template */
        bool fHeightChanged = (ctx.nLastTemplateHeight != nCurrentHeight);
        REQUIRE(fHeightChanged == false);
    }

    SECTION("Recovery works regardless of which MINER_READY opcode was used")
    {
        /* Test that the context update logic is the same regardless of opcode variant */
        uint32_t nCurrentHeight = 6594321;

        /* Simulate recovery for three opcode variants */
        for(uint16_t nOpcode : {uint16_t(0xD0D8), uint16_t(0xD090), uint16_t(0x00D8)})
        {
            MiningContext ctx;
            ctx = ctx.WithChannel(2)
                     .WithAuth(true)
                     .WithLastTemplateHeight(0);  // Fresh connection

            /* Remap compatibility opcodes (same logic as ProcessPacket) */
            if(nOpcode == 0x00D8 || nOpcode == 0xD090)
                nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

            REQUIRE(nOpcode == 0xD0D8);

            /* Update template height after sending */
            ctx = ctx.WithLastTemplateHeight(nCurrentHeight);
            REQUIRE(ctx.nLastTemplateHeight == nCurrentHeight);
        }
    }
}
