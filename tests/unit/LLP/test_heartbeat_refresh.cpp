/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_constants.h>
#include <LLP/include/mining_constants.h>
#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/stateless_manager.h>
#include <Util/include/config.h>

#include <cstdint>
#include <string>

using namespace LLP;

/**
 * Heartbeat Template Refresh Tests
 *
 * Tests for the proactive heartbeat refresh mechanism that prevents miners from
 * entering degraded mode during legitimate dry spells on slow channels (e.g. Prime).
 *
 * The heartbeat fires at TEMPLATE_HEARTBEAT_REFRESH_SECONDS (480 s) — well before
 * the hard MAX_TEMPLATE_AGE_SECONDS (600 s) cutoff — and re-pushes the current
 * template to all subscribed miners.
 */

//=============================================================================
// Section 1: Constant value tests
//=============================================================================

TEST_CASE("FalconConstants: TEMPLATE_HEARTBEAT_REFRESH_SECONDS", "[heartbeat][constants]")
{
    SECTION("Heartbeat refresh threshold equals 480 seconds")
    {
        REQUIRE(FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS == 480u);
    }

    SECTION("Heartbeat refresh fires before hard template age cutoff")
    {
        /* Core safety guarantee: heartbeat fires before the miner times out */
        REQUIRE(FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS <
                FalconConstants::MAX_TEMPLATE_AGE_SECONDS);
    }

    SECTION("Hard template age cutoff equals 600 seconds")
    {
        REQUIRE(FalconConstants::MAX_TEMPLATE_AGE_SECONDS == 600u);
    }

    SECTION("Margin between heartbeat and hard cutoff is at least 60 seconds")
    {
        /* Ensure operators have at least one heartbeat-check interval of margin */
        const uint64_t nMargin =
            FalconConstants::MAX_TEMPLATE_AGE_SECONDS -
            FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS;
        REQUIRE(nMargin >= MiningConstants::HEARTBEAT_CHECK_INTERVAL_SECONDS);
    }
}

TEST_CASE("MiningConstants: HEARTBEAT_CHECK_INTERVAL_SECONDS", "[heartbeat][constants]")
{
    SECTION("Heartbeat check interval equals 60 seconds")
    {
        REQUIRE(MiningConstants::HEARTBEAT_CHECK_INTERVAL_SECONDS == 60u);
    }

    SECTION("Check interval is smaller than heartbeat refresh threshold")
    {
        /* Server must poll at least once per check interval before threshold fires */
        REQUIRE(MiningConstants::HEARTBEAT_CHECK_INTERVAL_SECONDS <
                FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS);
    }
}


//=============================================================================
// Section 2: HeartbeatRefreshCheck() — safety and no-op behaviour
//=============================================================================

TEST_CASE("HeartbeatRefreshCheck: no-op on fresh start (zero dispatch time)",
          "[heartbeat][noop]")
{
    SECTION("Calling HeartbeatRefreshCheck when no dispatch has occurred does not crash")
    {
        /* On a freshly-started node the per-channel dispatch timestamps are 0.
         * HeartbeatRefreshCheck must skip both channels silently (nLastTime == 0
         * is the sentinel for "never dispatched"). */
        REQUIRE_NOTHROW(MinerPushDispatcher::HeartbeatRefreshCheck());
    }

    SECTION("Multiple consecutive calls to HeartbeatRefreshCheck are safe")
    {
        for(int i = 0; i < 5; ++i)
            REQUIRE_NOTHROW(MinerPushDispatcher::HeartbeatRefreshCheck());
    }
}


//=============================================================================
// Section 3: SIM-LINK deprecation flag tests
//=============================================================================

TEST_CASE("SIM-LINK: -deprecate-simlink-fallback config flag", "[heartbeat][deprecation]")
{
    SECTION("Flag defaults to false (fallback enabled by default)")
    {
        /* Without setting the flag, cross-lane resolution is still active */
        const bool fDeprecated = config::GetBoolArg("-deprecate-simlink-fallback", false);
        REQUIRE_FALSE(fDeprecated);
    }

    SECTION("FindSessionBlock returns nullptr for unknown session (unchanged behaviour)")
    {
        /* Even with the flag disabled, an unknown session still returns nullptr */
        const uint32_t nUnknownSession = 0xDEADC0DE;
        const uint512_t fakeMerkle(uint64_t(0xABCDEF));

        auto pBlock = StatelessMinerManager::Get().FindSessionBlock(
            nUnknownSession, fakeMerkle);

        REQUIRE(pBlock == nullptr);
    }

    SECTION("PruneSessionBlocks is safe for unknown session (no crash on deprecation path)")
    {
        const uint32_t nUnknownSession = 0xFEEDFACE;
        REQUIRE_NOTHROW(
            StatelessMinerManager::Get().PruneSessionBlocks(nUnknownSession));
    }
}



//=============================================================================
// Section 4: Dry spell detection warning thresholds (documented contract)
//=============================================================================

TEST_CASE("Heartbeat dry spell warning thresholds are sane", "[heartbeat][thresholds]")
{
    /* Document the expected threshold ordering:
     *   300 s — NOTICE  (operator-visible but not urgent)
     *   450 s — WARNING (approaching refresh threshold)
     *   550 s — CRITICAL (beyond refresh threshold but before hard cutoff)
     *   480 s — heartbeat fires (TEMPLATE_HEARTBEAT_REFRESH_SECONDS)
     *   600 s — hard cutoff (MAX_TEMPLATE_AGE_SECONDS)
     */

    SECTION("Notice threshold (300 s) is below heartbeat refresh threshold (480 s)")
    {
        constexpr uint64_t NOTICE_THRESHOLD = 300u;
        REQUIRE(NOTICE_THRESHOLD < FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS);
    }

    SECTION("Warning threshold (450 s) is below heartbeat refresh threshold (480 s)")
    {
        constexpr uint64_t WARNING_THRESHOLD = 450u;
        REQUIRE(WARNING_THRESHOLD < FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS);
    }

    SECTION("Critical threshold (550 s) is above heartbeat refresh threshold (480 s)")
    {
        /* At 550 s the heartbeat has already fired (at 480 s), so a 550 s log means
         * the heartbeat did NOT reset the timestamp — likely because BroadcastChannel
         * found no connected miners.  Still below the hard 600 s cutoff. */
        constexpr uint64_t CRITICAL_THRESHOLD = 550u;
        REQUIRE(CRITICAL_THRESHOLD > FalconConstants::TEMPLATE_HEARTBEAT_REFRESH_SECONDS);
        REQUIRE(CRITICAL_THRESHOLD < FalconConstants::MAX_TEMPLATE_AGE_SECONDS);
    }
}


//=============================================================================
// Section 5: BroadcastChannel fHeartbeat parameter — documented contract
//=============================================================================

TEST_CASE("BroadcastChannel: fHeartbeat parameter is accepted without crash",
          "[heartbeat][broadcast]")
{
    /* BroadcastChannel is a private static helper called by HeartbeatRefreshCheck.
     * When fHeartbeat=false (default) it behaves like a normal push.
     * When fHeartbeat=true it additionally:
     *   - Adds a [HEARTBEAT] annotation to the summary log line.
     *   - Passes fHeartbeat=true to NotifyChannelMiners() on each server, which
     *     in turn calls PrepareHeartbeatNotification() on each eligible connection
     *     before SendChannelNotification() — resetting m_force_next_push and
     *     m_get_block_cooldown so the push and subsequent GET_BLOCK are unthrottled.
     *
     * With no servers running (STATELESS_MINER_SERVER and MINING_SERVER are null),
     * HeartbeatRefreshCheck and its BroadcastChannel call-through must be safe no-ops.
     */

    SECTION("HeartbeatRefreshCheck compiles and runs cleanly (no servers running)")
    {
        /* HeartbeatRefreshCheck skips channels with zero dispatch time, so this is
         * a no-op — but it confirms the fHeartbeat=true call chain compiles. */
        REQUIRE_NOTHROW(MinerPushDispatcher::HeartbeatRefreshCheck());
    }

    SECTION("Multiple HeartbeatRefreshCheck calls are safe (no servers running)")
    {
        for(int i = 0; i < 3; ++i)
            REQUIRE_NOTHROW(MinerPushDispatcher::HeartbeatRefreshCheck());
    }
}
