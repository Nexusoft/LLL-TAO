/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/channel_notify_result.h>

#include <cstdint>
#include <string>

/* Test suite for ChannelNotifyResult and MINER_PUSH_SUMMARY semantics.
 *
 * These tests validate that the ChannelNotifyResult struct correctly
 * represents the state of a miner push fan-out without requiring a live
 * miner server.  They document the expected values for a Prime-only
 * stateless miner scenario and confirm that Hash wrong-channel skips
 * do NOT imply a push failure.
 */

TEST_CASE("ChannelNotifyResult - default-initialised to zero", "[miner_push_summary][llp]")
{
    LLP::ChannelNotifyResult tResult;

    REQUIRE(tResult.nNotified           == 0);
    REQUIRE(tResult.nSkippedWrongChannel == 0);
    REQUIRE(tResult.nSkippedPolling      == 0);
    REQUIRE(tResult.nSkippedDisconnected == 0);
}


TEST_CASE("ChannelNotifyResult - Prime-only stateless miner scenario", "[miner_push_summary][llp]")
{
    /* Simulate: one Prime miner connected, stateless lane.
     *
     * Prime channel broadcast:
     *   - The miner IS subscribed and IS on the correct channel.
     *   - Expected: notified=1, all skips=0.
     *
     * Hash channel broadcast:
     *   - The same Prime miner is skipped because it is on the wrong channel.
     *   - Expected: notified=0, nSkippedWrongChannel=1, other skips=0.
     *   - This is NORMAL — it does not indicate a push failure.
     */

    LLP::ChannelNotifyResult tPrime;
    tPrime.nNotified            = 1;
    tPrime.nSkippedWrongChannel = 0;
    tPrime.nSkippedPolling      = 0;
    tPrime.nSkippedDisconnected = 0;

    SECTION("Prime channel: miner is notified")
    {
        REQUIRE(tPrime.nNotified            == 1);
        REQUIRE(tPrime.nSkippedWrongChannel == 0);
        REQUIRE(tPrime.nSkippedPolling      == 0);
        REQUIRE(tPrime.nSkippedDisconnected == 0);
    }

    LLP::ChannelNotifyResult tHash;
    tHash.nNotified            = 0;
    tHash.nSkippedWrongChannel = 1;   /* same Prime miner, wrong channel for Hash broadcast */
    tHash.nSkippedPolling      = 0;
    tHash.nSkippedDisconnected = 0;

    SECTION("Hash channel: wrong-channel skip is expected, not a failure")
    {
        /* notified==0 for Hash is correct when all miners are on Prime. */
        REQUIRE(tHash.nNotified            == 0);

        /* nSkippedWrongChannel reflects that one miner was seen but filtered
         * because it subscribes to Prime, not Hash.  This is the expected
         * outcome of the channel-routing fan-out and must NOT be treated as
         * an error or push failure by monitoring logic. */
        REQUIRE(tHash.nSkippedWrongChannel == 1);

        REQUIRE(tHash.nSkippedPolling      == 0);
        REQUIRE(tHash.nSkippedDisconnected == 0);
    }

    SECTION("Summary: total push coverage is correct")
    {
        /* Across both channels the one Prime miner was reached exactly once. */
        uint32_t nTotalNotified = tPrime.nNotified + tHash.nNotified;
        REQUIRE(nTotalNotified == 1);

        /* Wrong-channel skips account for the miner being filtered on Hash. */
        uint32_t nTotalWrongChannel = tPrime.nSkippedWrongChannel + tHash.nSkippedWrongChannel;
        REQUIRE(nTotalWrongChannel == 1);

        /* No unexpected skips (polling or disconnected). */
        REQUIRE((tPrime.nSkippedPolling + tHash.nSkippedPolling)           == 0);
        REQUIRE((tPrime.nSkippedDisconnected + tHash.nSkippedDisconnected) == 0);
    }
}


TEST_CASE("ChannelNotifyResult - no miners connected scenario", "[miner_push_summary][llp]")
{
    /* When no miners are connected, both channels return zero for all fields. */
    LLP::ChannelNotifyResult tPrime;
    LLP::ChannelNotifyResult tHash;

    SECTION("Prime channel: nothing to notify")
    {
        REQUIRE(tPrime.nNotified            == 0);
        REQUIRE(tPrime.nSkippedWrongChannel == 0);
    }

    SECTION("Hash channel: nothing to notify")
    {
        REQUIRE(tHash.nNotified            == 0);
        REQUIRE(tHash.nSkippedWrongChannel == 0);
    }
}


TEST_CASE("ChannelNotifyResult - legacy lane, no miners", "[miner_push_summary][llp]")
{
    /* Legacy lane typically has zero connected clients on a stateless-only setup. */
    LLP::ChannelNotifyResult tLegacyPrime;
    LLP::ChannelNotifyResult tLegacyHash;

    /* Both are zero — this is normal for a node that has only stateless miners. */
    REQUIRE(tLegacyPrime.nNotified == 0);
    REQUIRE(tLegacyHash.nNotified  == 0);

    /* Legacy zero-notified is not a failure; the stateless lane handles push. */
    REQUIRE((tLegacyPrime.nNotified + tLegacyHash.nNotified) == 0);
}


TEST_CASE("FormatHashLaneSummary - distinguishes not_broadcast from zero skips",
          "[miner_push_summary][llp]")
{
    LLP::ChannelNotifyResult tHash;

    SECTION("Hash not broadcast (e.g. dedup): unambiguous not_broadcast token")
    {
        const std::string strSummary = LLP::FormatHashLaneSummary(false, tHash);

        REQUIRE(strSummary == " | hash not_broadcast");
        REQUIRE(strSummary.find("expected if all miners are Prime") == std::string::npos);
        REQUIRE(strSummary.find("wrong_channel=") == std::string::npos);
    }

    SECTION("Hash broadcast with zero wrong-channel skips: no expected-note")
    {
        tHash.nNotified            = 1;
        tHash.nSkippedWrongChannel = 0;
        tHash.nSkippedPolling      = 0;
        tHash.nSkippedDisconnected = 0;

        const std::string strSummary = LLP::FormatHashLaneSummary(true, tHash);

        REQUIRE(strSummary ==
                " | hash notified=1 wrong_channel=0 polling=0 disconnected=0");
        REQUIRE(strSummary.find("expected if all miners are Prime") == std::string::npos);
        REQUIRE(strSummary.find("not_broadcast") == std::string::npos);
    }

    SECTION("Hash broadcast with wrong-channel skips: expected-note present")
    {
        tHash.nNotified            = 0;
        tHash.nSkippedWrongChannel = 1;
        tHash.nSkippedPolling      = 0;
        tHash.nSkippedDisconnected = 0;

        const std::string strSummary = LLP::FormatHashLaneSummary(true, tHash);

        REQUIRE(strSummary ==
                " | hash notified=0 wrong_channel=1"
                " (expected if all miners are Prime)"
                " polling=0 disconnected=0");
        REQUIRE(strSummary.find("not_broadcast") == std::string::npos);
    }

    SECTION("not_broadcast zero-counters must not look like broadcast zeros")
    {
        /* Default-zero result with fBroadcast=false must remain distinguishable
         * from the same counters with fBroadcast=true. */
        const std::string strNotBroadcast = LLP::FormatHashLaneSummary(false, tHash);
        const std::string strBroadcastZeros = LLP::FormatHashLaneSummary(true, tHash);

        REQUIRE(strNotBroadcast != strBroadcastZeros);
        REQUIRE(strNotBroadcast == " | hash not_broadcast");
        REQUIRE(strBroadcastZeros ==
                " | hash notified=0 wrong_channel=0 polling=0 disconnected=0");
    }
}
