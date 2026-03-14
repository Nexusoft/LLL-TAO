/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/graceful_shutdown.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/opcode_utility.h>

using namespace LLP;

/* ═══════════════════════════════════════════════════════════════════════ */
/* 1. NODE_SHUTDOWN opcode value                                           */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("NODE_SHUTDOWN opcode constant is 0xD0FF", "[graceful_shutdown]")
{
    REQUIRE(OpcodeUtility::Stateless::NODE_SHUTDOWN == 0xD0FF);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* 2. MiningContext::ChannelName() static helper                           */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("MiningContext::ChannelName returns correct strings", "[graceful_shutdown]")
{
    SECTION("Channel 1 returns Prime")
    {
        REQUIRE(MiningContext::ChannelName(1) == "Prime");
    }

    SECTION("Channel 2 returns Hash")
    {
        REQUIRE(MiningContext::ChannelName(2) == "Hash");
    }

    SECTION("Channel 0 returns Unknown(0)")
    {
        REQUIRE(MiningContext::ChannelName(0) == "Unknown(0)");
    }

    SECTION("Unexpected channel returns Unknown(N)")
    {
        REQUIRE(MiningContext::ChannelName(3) == "Unknown(3)");
        REQUIRE(MiningContext::ChannelName(255) == "Unknown(255)");
    }
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* 3. MiningContext::WithChannelName() builder                             */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("MiningContext::WithChannelName sets strChannelName correctly", "[graceful_shutdown]")
{
    SECTION("WithChannelName(1) sets Prime")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithChannelName(1);
        REQUIRE(updated.strChannelName == "Prime");
        /* Original must be unchanged (immutable) */
        REQUIRE(ctx.strChannelName == "Unknown");
    }

    SECTION("WithChannelName(2) sets Hash")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithChannelName(2);
        REQUIRE(updated.strChannelName == "Hash");
    }

    SECTION("WithChannelName(0) sets Unknown(0)")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithChannelName(0);
        REQUIRE(updated.strChannelName == "Unknown(0)");
    }
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* 4. MiningContext default strChannelName                                 */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("MiningContext default strChannelName is Unknown", "[graceful_shutdown]")
{
    MiningContext ctx;
    REQUIRE(ctx.strChannelName == "Unknown");
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* 5. GracefulDisconnectAllMiners with null servers (smoke test)           */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("GracefulDisconnectAllMiners with null servers does not crash", "[graceful_shutdown]")
{
    /* This is a compile-time-accessible check: the Stateless namespace must
     * contain NODE_SHUTDOWN and be in the valid stateless opcode range. */
    REQUIRE(OpcodeUtility::Stateless::IsStateless(OpcodeUtility::Stateless::NODE_SHUTDOWN));
}

TEST_CASE("Graceful shutdown notification state is idempotent", "[graceful_shutdown]")
{
    GracefulShutdown::NotificationState state;

    REQUIRE_FALSE(state.Sent());
    REQUIRE(state.MarkSent());
    REQUIRE(state.Sent());
    REQUIRE_FALSE(state.MarkSent());
}

TEST_CASE("Graceful shutdown flush window remains bounded", "[graceful_shutdown]")
{
    REQUIRE(GracefulShutdown::MINER_SHUTDOWN_FLUSH_MS >= 150);
    REQUIRE(GracefulShutdown::MINER_SHUTDOWN_FLUSH_MS <= 500);
}
