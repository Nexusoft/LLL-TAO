/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>

#include <helpers/test_fixtures.h>

#include <Util/include/runtime.h>

using namespace LLP;
using namespace TestFixtures;

TEST_CASE("Session Management - Session field initialization", "[session][initialization]")
{
    SECTION("Session builders populate expected fields")
    {
        const uint64_t startTime = runtime::unifiedtimestamp();

        MiningContext ctx = MiningContext()
            .WithSessionStart(startTime)
            .WithSession(12345)
            .WithKeepaliveCount(0);

        REQUIRE(ctx.nSessionStart == startTime);
        REQUIRE(ctx.nSessionId == 12345);
        REQUIRE(ctx.nKeepaliveCount == 0);
    }

    SECTION("Default session fields remain zero")
    {
        MiningContext ctx;

        REQUIRE(ctx.nSessionStart == 0);
        REQUIRE(ctx.nSessionId == 0);
        REQUIRE(ctx.nKeepaliveCount == 0);
        REQUIRE(ctx.nLastKeepaliveTime == 0);
    }
}

TEST_CASE("Session Management - Keepalive updates are immutable", "[session][keepalive]")
{
    const uint64_t nNow = runtime::unifiedtimestamp();

    MiningContext original = MiningContext()
        .WithSessionStart(nNow - 60)
        .WithTimestamp(nNow - 60);

    MiningContext updated = original
        .WithTimestamp(nNow)
        .WithKeepaliveCount(original.nKeepaliveCount + 1)
        .WithLastKeepaliveTime(nNow);

    REQUIRE(original.nTimestamp == nNow - 60);
    REQUIRE(original.nKeepaliveCount == 0);
    REQUIRE(original.nLastKeepaliveTime == 0);

    REQUIRE(updated.nTimestamp == nNow);
    REQUIRE(updated.nKeepaliveCount == 1);
    REQUIRE(updated.nLastKeepaliveTime == nNow);
}

TEST_CASE("Session Management - Stateless manager preserves session metadata", "[session][manager]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    const std::string strAddress = "192.168.1.100:9325";
    const uint64_t startTime = runtime::unifiedtimestamp();

    MiningContext ctx = CreateAuthenticatedContext()
        .WithSessionStart(startTime)
        .WithSession(777)
        .WithKeepaliveCount(2)
        .WithLastKeepaliveTime(startTime);
    ctx.strAddress = strAddress;

    manager.UpdateMiner(strAddress, ctx, 1);

    auto retrieved = manager.GetMinerContext(strAddress);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->nSessionStart == startTime);
    REQUIRE(retrieved->nSessionId == 777);
    REQUIRE(retrieved->nKeepaliveCount == 2);
    REQUIRE(retrieved->nLastKeepaliveTime == startTime);

    auto bySession = manager.GetMinerContextBySessionID(777);
    REQUIRE(bySession.has_value());
    REQUIRE(bySession->strAddress == strAddress);

    manager.RemoveMiner(strAddress);
}

TEST_CASE("Session Management - Session data survives context refreshes", "[session][refresh]")
{
    const uint64_t startTime = runtime::unifiedtimestamp() - 300;
    const uint64_t refreshTime = runtime::unifiedtimestamp();

    MiningContext original = CreateFullMiningContext(2)
        .WithSessionStart(startTime)
        .WithSession(4242)
        .WithKeepaliveCount(3);

    MiningContext refreshed = original
        .WithTimestamp(refreshTime)
        .WithHeight(original.nHeight + 1)
        .WithKeepaliveCount(original.nKeepaliveCount + 1);

    REQUIRE(refreshed.nSessionStart == startTime);
    REQUIRE(refreshed.nSessionId == 4242);
    REQUIRE(refreshed.nKeepaliveCount == 4);
    REQUIRE(refreshed.nHeight == original.nHeight + 1);

    REQUIRE(original.nSessionStart == startTime);
    REQUIRE(original.nKeepaliveCount == 3);
}

TEST_CASE("Session Management - CleanupInactive removes stale disconnected entries", "[session][cleanup]")
{
    StatelessMinerManager& manager = StatelessMinerManager::Get();

    MiningContext stale = CreateAuthenticatedContext()
        .WithTimestamp(runtime::unifiedtimestamp() - 1000)
        .WithSessionStart(runtime::unifiedtimestamp() - 1000)
        .WithSession(9090);
    stale.strAddress = "192.168.1.101:9325";

    manager.UpdateMiner(stale.strAddress, stale, 1);
    REQUIRE(manager.GetMinerContext(stale.strAddress).has_value());

    REQUIRE(manager.RemoveMiner(stale.strAddress));
    REQUIRE_FALSE(manager.GetMinerContext(stale.strAddress).has_value());

    /* Cleanup should safely handle already-removed inactive entries. */
    REQUIRE(manager.CleanupInactive(0) >= 0);
}
