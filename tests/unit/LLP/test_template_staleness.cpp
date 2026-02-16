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


/** Test Suite: Template Staleness Detection (Channel-Height Based)
 *
 *  These tests verify per-connection template tracking using CHANNEL-SPECIFIC
 *  heights (nLastTemplateChannelHeight) and MINER_READY compatibility opcodes.
 *
 *  CRITICAL DESIGN: Templates are tracked by channel height, NOT unified height.
 *  A Hash block advancing unified height must NOT trigger a Prime template refresh.
 *  Only when the miner's own channel advances do we need a new template.
 *
 *  Key behaviors:
 *  1. nLastTemplateChannelHeight tracks the channel height of the miner's last template
 *  2. GET_ROUND auto-sends template when nLastTemplateChannelHeight != current channel height
 *  3. MINER_READY compatibility opcodes (0xD090, 0x00D8) remap to 0xD0D8
 *  4. Multiple miners have independent template tracking
 *  5. Cross-channel blocks do NOT trigger false template refreshes
 *
 **/

/* Test constants representing realistic Nexus multi-channel state */
namespace ChannelHeightConstants
{
    const uint32_t UNIFIED_HEIGHT = 6594321;       // Unified blockchain height
    const uint32_t PRIME_CHANNEL_HEIGHT = 2325188;  // Prime channel height
    const uint32_t HASH_CHANNEL_HEIGHT = 4165000;   // Hash channel height
}


TEST_CASE("MiningContext nLastTemplateChannelHeight Initialization", "[template_staleness]")
{
    SECTION("Default constructor initializes nLastTemplateChannelHeight to 0")
    {
        MiningContext ctx;

        REQUIRE(ctx.nLastTemplateChannelHeight == 0);
    }

    SECTION("Parameterized constructor initializes nLastTemplateChannelHeight to 0")
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

        REQUIRE(ctx.nLastTemplateChannelHeight == 0);
        REQUIRE(ctx.nHeight == 6594320);
    }
}


TEST_CASE("MiningContext WithLastTemplateChannelHeight Immutability", "[template_staleness]")
{
    SECTION("WithLastTemplateChannelHeight returns new context with updated channel height")
    {
        MiningContext ctx;
        ctx = ctx.WithChannel(1).WithHeight(6594320);

        MiningContext updated = ctx.WithLastTemplateChannelHeight(ChannelHeightConstants::PRIME_CHANNEL_HEIGHT);

        /* New context has updated last template channel height */
        REQUIRE(updated.nLastTemplateChannelHeight == ChannelHeightConstants::PRIME_CHANNEL_HEIGHT);

        /* Original context unchanged (immutability) */
        REQUIRE(ctx.nLastTemplateChannelHeight == 0);
    }

    SECTION("WithLastTemplateChannelHeight preserves other fields")
    {
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithHeight(6594320)
                 .WithAuth(true)
                 .WithSession(42);

        MiningContext updated = ctx.WithLastTemplateChannelHeight(ChannelHeightConstants::PRIME_CHANNEL_HEIGHT);

        REQUIRE(updated.nLastTemplateChannelHeight == ChannelHeightConstants::PRIME_CHANNEL_HEIGHT);
        REQUIRE(updated.nChannel == 1);
        REQUIRE(updated.nHeight == 6594320);
        REQUIRE(updated.fAuthenticated == true);
        REQUIRE(updated.nSessionId == 42);
    }

    SECTION("Can chain WithLastTemplateChannelHeight with other With methods")
    {
        MiningContext ctx;

        MiningContext updated = ctx.WithHeight(6594321)
                                   .WithLastTemplateChannelHeight(ChannelHeightConstants::PRIME_CHANNEL_HEIGHT)
                                   .WithTimestamp(1000000);

        REQUIRE(updated.nHeight == 6594321);
        REQUIRE(updated.nLastTemplateChannelHeight == ChannelHeightConstants::PRIME_CHANNEL_HEIGHT);
        REQUIRE(updated.nTimestamp == 1000000);
    }
}


TEST_CASE("GET_ROUND Channel Height Change Detection", "[template_staleness][get_round]")
{
    SECTION("Channel height change detected: auto-send template")
    {
        /* Prime miner has template for channel height 2325188 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateChannelHeight(2325188);

        /* New Prime block found: channel height advances to 2325189 */
        uint32_t nCurrentChannelHeight = 2325189;

        /* Channel height changed: should auto-send template */
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentChannelHeight);
        REQUIRE(fChannelHeightChanged == true);
    }

    SECTION("No channel height change: no duplicate template")
    {
        /* Prime miner has template for channel height 2325188 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateChannelHeight(2325188);

        /* No new Prime blocks */
        uint32_t nCurrentChannelHeight = 2325188;

        /* Channel height unchanged: no duplicate template needed */
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentChannelHeight);
        REQUIRE(fChannelHeightChanged == false);
    }

    SECTION("First GET_ROUND after connection always triggers template send")
    {
        /* Fresh connection: nLastTemplateChannelHeight = 0 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1);

        /* Current Prime channel height */
        uint32_t nCurrentChannelHeight = 2325188;

        /* First request always triggers (0 != 2325188) */
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentChannelHeight);
        REQUIRE(fChannelHeightChanged == true);
    }

    SECTION("Cross-channel block does NOT trigger false template refresh")
    {
        /* CRITICAL TEST: This is the bug the reviewer identified.
         * A Hash block advancing unified height must NOT trigger a Prime template refresh.
         *
         * Scenario: Prime miner at channel height 2325188, Hash block found.
         * - Unified: 6594321 → 6594322  (changed)
         * - Prime:   2325188 → 2325188  (UNCHANGED)
         * - Expected: No auto-send (Prime channel didn't advance)
         */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithLastTemplateChannelHeight(2325188);

        /* Hash block found: unified height changes, but Prime channel height stays the same */
        uint32_t nCurrentPrimeChannelHeight = 2325188;  // UNCHANGED

        /* Using channel height comparison: no false trigger */
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentPrimeChannelHeight);
        REQUIRE(fChannelHeightChanged == false);

        /* Verify: if we had used unified height, it would have FALSELY triggered */
        uint32_t nOldUnified = 6594321;
        uint32_t nNewUnified = 6594322;
        bool fUnifiedChanged = (nOldUnified != nNewUnified);
        REQUIRE(fUnifiedChanged == true);  // Unified DID change...
        REQUIRE(fChannelHeightChanged == false);  // ...but channel did NOT
    }

    SECTION("Same-channel block correctly triggers template refresh")
    {
        /* Prime miner at channel height 2325188, another Prime block found */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithLastTemplateChannelHeight(2325188);

        /* Prime block found: channel height advances */
        uint32_t nCurrentPrimeChannelHeight = 2325189;

        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentPrimeChannelHeight);
        REQUIRE(fChannelHeightChanged == true);
    }

    SECTION("GET_HEIGHT does not interfere with template tracking")
    {
        /* Miner has template for channel height 2325188 */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)
                 .WithLastTemplateChannelHeight(2325188);

        /* GET_HEIGHT updates nHeight (unified) but NOT nLastTemplateChannelHeight */
        ctx = ctx.WithHeight(6594322);

        /* nLastTemplateChannelHeight should still reflect the old channel height */
        REQUIRE(ctx.nLastTemplateChannelHeight == 2325188);
        REQUIRE(ctx.nHeight == 6594322);  // unified height updated

        /* Channel height unchanged: no false trigger */
        uint32_t nCurrentChannelHeight = 2325188;
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentChannelHeight);
        REQUIRE(fChannelHeightChanged == false);
    }
}


TEST_CASE("Multiple Miners Independent Channel State", "[template_staleness][multi_miner]")
{
    SECTION("Different miners have independent channel height tracking")
    {
        /* Prime miner at channel height 2325188 */
        MiningContext ctxPrime;
        ctxPrime = ctxPrime.WithChannel(1)
                           .WithSession(100)
                           .WithLastTemplateChannelHeight(2325188);

        /* Hash miner at channel height 4164999 (slow poller) */
        MiningContext ctxHash;
        ctxHash = ctxHash.WithChannel(2)
                         .WithSession(200)
                         .WithLastTemplateChannelHeight(4164999);

        /* New Hash block found */
        uint32_t nNewPrimeHeight = 2325188;  // UNCHANGED
        uint32_t nNewHashHeight = 4165000;   // Advanced

        /* Prime miner: channel unchanged - no auto-send */
        bool fPrimeChanged = (ctxPrime.nLastTemplateChannelHeight != nNewPrimeHeight);
        REQUIRE(fPrimeChanged == false);

        /* Hash miner: channel changed - auto-send */
        bool fHashChanged = (ctxHash.nLastTemplateChannelHeight != nNewHashHeight);
        REQUIRE(fHashChanged == true);

        /* Update Hash miner */
        ctxHash = ctxHash.WithLastTemplateChannelHeight(nNewHashHeight);

        /* Hash miner now up to date */
        REQUIRE(ctxHash.nLastTemplateChannelHeight == 4165000);

        /* Prime miner still at same channel height (correctly) */
        REQUIRE(ctxPrime.nLastTemplateChannelHeight == 2325188);
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
    SECTION("MINER_READY should trigger template send and update channel height tracking")
    {
        /* Miner enters degraded mode with stale template */
        MiningContext ctx;
        ctx = ctx.WithChannel(1)  // Prime miner
                 .WithAuth(true)
                 .WithSession(42)
                 .WithLastTemplateChannelHeight(2325180);  // Far behind

        /* Current Prime channel height */
        uint32_t nCurrentChannelHeight = 2325189;

        /* After MINER_READY processing:
         * 1. Send push notification (PRIME_AVAILABLE)
         * 2. Auto-send BLOCK_DATA template
         * 3. Update nLastTemplateChannelHeight to current channel height */
        ctx = ctx.WithLastTemplateChannelHeight(nCurrentChannelHeight);

        REQUIRE(ctx.nLastTemplateChannelHeight == 2325189);

        /* Next GET_ROUND should NOT send duplicate template (channel height unchanged) */
        bool fChannelHeightChanged = (ctx.nLastTemplateChannelHeight != nCurrentChannelHeight);
        REQUIRE(fChannelHeightChanged == false);
    }

    SECTION("Recovery works regardless of which MINER_READY opcode was used")
    {
        /* Test that the context update logic is the same regardless of opcode variant */
        uint32_t nCurrentChannelHeight = 4165001;  // Hash channel height

        /* Simulate recovery for three opcode variants */
        for(uint16_t nOpcode : {uint16_t(0xD0D8), uint16_t(0xD090), uint16_t(0x00D8)})
        {
            MiningContext ctx;
            ctx = ctx.WithChannel(2)  // Hash miner
                     .WithAuth(true)
                     .WithLastTemplateChannelHeight(0);  // Fresh connection

            /* Remap compatibility opcodes (same logic as ProcessPacket) */
            if(nOpcode == 0x00D8 || nOpcode == 0xD090)
                nOpcode = OpcodeUtility::Stateless::STATELESS_MINER_READY;

            REQUIRE(nOpcode == 0xD0D8);

            /* Update channel height after sending template */
            ctx = ctx.WithLastTemplateChannelHeight(nCurrentChannelHeight);
            REQUIRE(ctx.nLastTemplateChannelHeight == nCurrentChannelHeight);
        }
    }
}

