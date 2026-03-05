/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/push_notification.h>

/* Test suite for MiningContext ProtocolLane field
 *
 * Tests the new nProtocolLane field added to MiningContext as part of Phase 3
 * unified mining server configuration refactor.
 *
 * Key test areas:
 * - Default constructor initializes to LEGACY lane
 * - Parameterized constructor initializes to LEGACY lane
 * - WithProtocolLane() factory method updates lane correctly
 * - IsStateless() helper method returns correct boolean
 * - Chaining multiple factory methods preserves lane
 */

TEST_CASE("MiningContext - ProtocolLane Field", "[mining_context][protocol_lane][llp]")
{
    SECTION("Default constructor initializes to LEGACY lane")
    {
        LLP::MiningContext ctx;

        REQUIRE(ctx.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx.IsStateless());
    }

    SECTION("Parameterized constructor initializes to LEGACY lane")
    {
        LLP::MiningContext ctx(
            1,              // nChannel (Prime)
            1000,           // nHeight
            1234567890,     // nTimestamp
            "127.0.0.1",    // strAddress
            1,              // nProtocolVersion
            false,          // fAuthenticated
            0,              // nSessionId
            uint256_t(0),   // hashKeyID
            uint256_t(0)    // hashGenesis
        );

        REQUIRE(ctx.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx.IsStateless());
    }

    SECTION("WithProtocolLane() sets LEGACY lane")
    {
        LLP::MiningContext ctx;
        LLP::MiningContext ctx2 = ctx.WithProtocolLane(LLP::ProtocolLane::LEGACY);

        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx2.IsStateless());
    }

    SECTION("WithProtocolLane() sets STATELESS lane")
    {
        LLP::MiningContext ctx;
        LLP::MiningContext ctx2 = ctx.WithProtocolLane(LLP::ProtocolLane::STATELESS);

        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::STATELESS);
        REQUIRE(ctx2.IsStateless());
    }

    SECTION("IsStateless() returns false for LEGACY lane")
    {
        LLP::MiningContext ctx;
        ctx = ctx.WithProtocolLane(LLP::ProtocolLane::LEGACY);

        REQUIRE_FALSE(ctx.IsStateless());
    }

    SECTION("IsStateless() returns true for STATELESS lane")
    {
        LLP::MiningContext ctx;
        ctx = ctx.WithProtocolLane(LLP::ProtocolLane::STATELESS);

        REQUIRE(ctx.IsStateless());
    }

    SECTION("Chaining WithProtocolLane() with other factory methods")
    {
        LLP::MiningContext ctx;

        /* Chain multiple factory methods */
        LLP::MiningContext ctx2 = ctx
            .WithChannel(1)                                  // Set to Prime channel
            .WithProtocolLane(LLP::ProtocolLane::STATELESS) // Set to stateless lane
            .WithHeight(5000)                                // Set height
            .WithAuth(true);                                 // Set authenticated

        /* Verify all fields were updated */
        REQUIRE(ctx2.nChannel == 1);
        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::STATELESS);
        REQUIRE(ctx2.nHeight == 5000);
        REQUIRE(ctx2.fAuthenticated == true);
        REQUIRE(ctx2.IsStateless());
    }

    SECTION("Chaining WithProtocolLane() after WithChannel() preserves channel")
    {
        LLP::MiningContext ctx;

        /* Set channel first, then protocol lane */
        LLP::MiningContext ctx2 = ctx
            .WithChannel(2)                                  // Set to Hash channel
            .WithProtocolLane(LLP::ProtocolLane::LEGACY);   // Set to legacy lane

        /* Verify both fields */
        REQUIRE(ctx2.nChannel == 2);
        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx2.IsStateless());
    }

    SECTION("Switching lanes preserves other context fields")
    {
        LLP::MiningContext ctx;

        /* Create a context with multiple fields set */
        ctx = ctx.WithChannel(1)
                 .WithHeight(1000)
                 .WithAuth(true)
                 .WithSession(12345);

        /* Switch from LEGACY to STATELESS */
        LLP::MiningContext ctx2 = ctx.WithProtocolLane(LLP::ProtocolLane::STATELESS);

        /* Verify all original fields preserved */
        REQUIRE(ctx2.nChannel == 1);
        REQUIRE(ctx2.nHeight == 1000);
        REQUIRE(ctx2.fAuthenticated == true);
        REQUIRE(ctx2.nSessionId == 12345);
        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::STATELESS);
        REQUIRE(ctx2.IsStateless());

        /* Switch back to LEGACY */
        LLP::MiningContext ctx3 = ctx2.WithProtocolLane(LLP::ProtocolLane::LEGACY);

        /* Verify all fields still preserved */
        REQUIRE(ctx3.nChannel == 1);
        REQUIRE(ctx3.nHeight == 1000);
        REQUIRE(ctx3.fAuthenticated == true);
        REQUIRE(ctx3.nSessionId == 12345);
        REQUIRE(ctx3.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx3.IsStateless());
    }

    SECTION("Original context unmodified by WithProtocolLane() (immutability)")
    {
        LLP::MiningContext ctx;
        ctx = ctx.WithProtocolLane(LLP::ProtocolLane::STATELESS);

        /* Create new context with different lane */
        LLP::MiningContext ctx2 = ctx.WithProtocolLane(LLP::ProtocolLane::LEGACY);

        /* Verify original unchanged (immutability) */
        REQUIRE(ctx.nProtocolLane == LLP::ProtocolLane::STATELESS);
        REQUIRE(ctx.IsStateless());

        /* Verify new context has updated lane */
        REQUIRE(ctx2.nProtocolLane == LLP::ProtocolLane::LEGACY);
        REQUIRE_FALSE(ctx2.IsStateless());
    }
}

TEST_CASE("MiningContext - ProtocolLane Enum Values", "[mining_context][protocol_lane][llp]")
{
    SECTION("ProtocolLane::LEGACY has value 0")
    {
        REQUIRE(static_cast<uint8_t>(LLP::ProtocolLane::LEGACY) == 0);
    }

    SECTION("ProtocolLane::STATELESS has value 1")
    {
        REQUIRE(static_cast<uint8_t>(LLP::ProtocolLane::STATELESS) == 1);
    }
}
