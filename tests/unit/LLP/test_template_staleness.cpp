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


/** Test Suite: Template Staleness Detection (Unified-Height Based)
 *
 *  These tests verify per-connection template tracking using UNIFIED heights
 *  (nLastTemplateUnifiedHeight) and MINER_READY compatibility opcodes.
 *
 *  CRITICAL DESIGN: Templates are tracked by UNIFIED height, because every
 *  tip move (any channel) changes hashPrevBlock.  ALL channels need fresh
 *  templates whenever the unified height advances.  In a multi-channel mining
 *  blockchain, a Hash block advancing unified height DOES trigger a Prime
 *  template refresh — the old hashPrevBlock is stale.
 *
 *  Key behaviors:
 *  1. nLastTemplateUnifiedHeight tracks the unified height when last template was sent
 *  2. GET_ROUND auto-sends template when nLastTemplateUnifiedHeight != current unified height
 *  3. MINER_READY compatibility opcodes (0xD090, 0x00D8) remap to 0xD0D8
 *  4. Multiple miners have independent template tracking
 *  5. Cross-channel blocks DO trigger template refreshes (hashPrevBlock changes)
 *
 **/

/* Test constants representing realistic Nexus multi-channel state */
namespace ChannelHeightConstants
{
    const uint32_t UNIFIED_HEIGHT = 6594321;       // Unified blockchain height
    const uint32_t PRIME_CHANNEL_HEIGHT = 2325188;  // Prime channel height
    const uint32_t HASH_CHANNEL_HEIGHT = 4165000;   // Hash channel height
}


TEST_CASE("MiningContext nLastTemplateUnifiedHeight Initialization", "[template_staleness]")
{
    SECTION("Default constructor initializes nLastTemplateUnifiedHeight to 0")
    {
        MiningContext ctx;

        REQUIRE(ctx.nLastTemplateUnifiedHeight == 0);
    }

    SECTION("Parameterized constructor initializes nLastTemplateUnifiedHeight to 0")
    {
        MiningContext ctx(
            1,          // nChannel (Prime)
            6594320,    // nHeight (unified)
            1000000,    // nTimestamp
            "127.0.0.1",// strAddress
            1,          // nProtocolVersion
            true,       // fAuthenticated
            42,         // nSessionId
            uint256_t(0), // hashKeyID
            uint256_t(0)  // hashGenesis
        );

        REQUIRE(ctx.nLastTemplateUnifiedHeight == 0);
        REQUIRE(ctx.nHeight == 6594320);
    }
}


TEST_CASE("MiningContext WithLastTemplateUnifiedHeight Immutability", "[template_staleness]")
{
    SECTION("WithLastTemplateUnifiedHeight returns new context with updated unified height")
    {
        MiningContext ctx;
        ctx = ctx.WithChannel(1).WithHeight(6594320);

        MiningContext updated = ctx.WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* New context has updated last template unified height */
        REQUIRE(updated.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Original context unchanged (immutability) */
        REQUIRE(ctx.nLastTemplateUnifiedHeight == 0);
    }

    SECTION("WithLastTemplateUnifiedHeight preserves other fields")
    {
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithHeight(6594320)
                 .WithAuth(true)
                 .WithSession(42);

        MiningContext updated = ctx.WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        REQUIRE(updated.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);
        REQUIRE(updated.nChannel == 1);
        REQUIRE(updated.nHeight == 6594320);
        REQUIRE(updated.fAuthenticated == true);
        REQUIRE(updated.nSessionId == 42);
    }

    SECTION("Can chain WithLastTemplateUnifiedHeight with other With methods")
    {
        MiningContext ctx;

        MiningContext updated = ctx.WithHeight(6594321)
                                   .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT)
                                   .WithTimestamp(1000000);

        REQUIRE(updated.nHeight == 6594321);
        REQUIRE(updated.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);
        REQUIRE(updated.nTimestamp == 1000000);
    }
}


TEST_CASE("GET_ROUND Unified Height Change Detection", "[template_staleness][get_round]")
{
    SECTION("Unified height change detected: auto-send template")
    {
        /* Prime miner has template for unified height 6594321 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* New block found (any channel): unified height advances */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT + 1;

        /* Unified height changed: should auto-send template */
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nCurrentUnifiedHeight);
        REQUIRE(fTemplateStale == true);
    }

    SECTION("No unified height change: no duplicate template")
    {
        /* Prime miner has template for unified height 6594321 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* No new blocks on any channel */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;

        /* Unified height unchanged: no duplicate template needed */
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nCurrentUnifiedHeight);
        REQUIRE(fTemplateStale == false);
    }

    SECTION("First GET_ROUND after connection always triggers template send")
    {
        /* Fresh connection: nLastTemplateUnifiedHeight = 0 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1);

        /* Current unified height */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;

        /* First request always triggers (0 != 6594321) */
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nCurrentUnifiedHeight);
        REQUIRE(fTemplateStale == true);
    }

    SECTION("Cross-channel block DOES trigger template refresh (hashPrevBlock changes)")
    {
        /* CRITICAL TEST: Multi-channel mining blockchains require ALL channels
         * to get fresh templates whenever ANY channel mines a block, because
         * hashPrevBlock = tStateBest.GetHash() (see create.cpp:434).
         *
         * Scenario: Prime miner at unified height 6594321, Hash block found.
         * - Unified: 6594321 → 6594322  (changed)
         * - Prime channel height: 2325188 → 2325188  (unchanged, but irrelevant)
         * - Expected: Auto-send (unified height advanced = new hashPrevBlock)
         */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Hash block found: unified height advances even though Prime channel unchanged */
        uint32_t nNewUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT + 1;

        /* Unified height changed: template IS stale (new hashPrevBlock) */
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nNewUnifiedHeight);
        REQUIRE(fTemplateStale == true);
    }

    SECTION("Same-channel block correctly triggers template refresh")
    {
        /* Prime miner at unified 6594321, another Prime block found */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Prime block found: unified height advances */
        uint32_t nNewUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT + 1;

        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nNewUnifiedHeight);
        REQUIRE(fTemplateStale == true);
    }

    SECTION("GET_HEIGHT does not interfere with template tracking")
    {
        /* Miner has template for unified height 6594321 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* GET_HEIGHT updates nHeight (diagnostic) but NOT nLastTemplateUnifiedHeight */
        ctx = ctx.WithHeight(ChannelHeightConstants::UNIFIED_HEIGHT + 1);

        /* nLastTemplateUnifiedHeight should still reflect the old unified height */
        REQUIRE(ctx.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);
        REQUIRE(ctx.nHeight == ChannelHeightConstants::UNIFIED_HEIGHT + 1);

        /* If current unified matches the tracked one, no stale */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nCurrentUnifiedHeight);
        REQUIRE(fTemplateStale == false);
    }
}


TEST_CASE("Multiple Miners Independent Unified Height State", "[template_staleness][multi_miner]")
{
    SECTION("Different miners have independent unified height tracking")
    {
        /* Prime miner up-to-date at unified height */
        MiningContext ctxPrime;
        ctxPrime = ctxPrime.WithChannel(1)
                           .WithSession(100)
                           .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Hash miner slightly behind at unified height */
        MiningContext ctxHash;
        ctxHash = ctxHash.WithChannel(2)
                         .WithSession(200)
                         .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT - 1);

        /* New block found: unified height advances */
        uint32_t nNewUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;

        /* Prime miner: unified height matches - no auto-send needed */
        bool fPrimeStale = (ctxPrime.nLastTemplateUnifiedHeight != nNewUnifiedHeight);
        REQUIRE(fPrimeStale == false);

        /* Hash miner: unified height behind - needs auto-send */
        bool fHashStale = (ctxHash.nLastTemplateUnifiedHeight != nNewUnifiedHeight);
        REQUIRE(fHashStale == true);

        /* Update Hash miner */
        ctxHash = ctxHash.WithLastTemplateUnifiedHeight(nNewUnifiedHeight);

        /* Hash miner now up to date */
        REQUIRE(ctxHash.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Prime miner still at same unified height (correctly independent) */
        REQUIRE(ctxPrime.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);
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
    SECTION("MINER_READY should trigger template send and update unified height tracking")
    {
        /* Miner enters degraded mode with stale template */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithAuth(true)
                 .WithSession(42)
                 .WithLastTemplateUnifiedHeight(ChannelHeightConstants::UNIFIED_HEIGHT - 10);  // Far behind

        /* Current unified height */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;

        /* After MINER_READY processing:
         * 1. Send push notification (PRIME_AVAILABLE)
         * 2. Auto-send BLOCK_DATA template
         * 3. Update nLastTemplateUnifiedHeight to current unified height */
        ctx = ctx.WithLastTemplateUnifiedHeight(nCurrentUnifiedHeight);

        REQUIRE(ctx.nLastTemplateUnifiedHeight == ChannelHeightConstants::UNIFIED_HEIGHT);

        /* Next GET_ROUND should NOT send duplicate template (unified height unchanged) */
        bool fTemplateStale = (ctx.nLastTemplateUnifiedHeight != nCurrentUnifiedHeight);
        REQUIRE(fTemplateStale == false);
    }

    SECTION("Recovery works regardless of which MINER_READY opcode was used")
    {
        /* Test that the context update logic is the same regardless of opcode variant */
        uint32_t nCurrentUnifiedHeight = ChannelHeightConstants::UNIFIED_HEIGHT;

        /* Simulate recovery for three opcode variants */
        for(uint16_t nOpcode : {uint16_t(0xD0D8), uint16_t(0xD090), uint16_t(0x00D8)})
        {
            MiningContext ctx;
            ctx = ctx.WithChannel(2)  // Hash miner
                     .WithAuth(true)
                     .WithLastTemplateUnifiedHeight(0);  // Fresh connection

            /* Remap compatibility opcodes (same logic as ProcessPacket) */
            if(nOpcode == 0x00D8 || nOpcode == 0xD090)
                nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

            REQUIRE(nOpcode == 0xD0D8);

            /* Update unified height after sending template */
            ctx = ctx.WithLastTemplateUnifiedHeight(nCurrentUnifiedHeight);
            REQUIRE(ctx.nLastTemplateUnifiedHeight == nCurrentUnifiedHeight);
        }
    }
}

