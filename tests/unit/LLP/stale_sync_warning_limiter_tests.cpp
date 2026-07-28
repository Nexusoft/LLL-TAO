/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#include <LLP/include/stale_sync_diagnostics.h>

#include <unit/catch2/catch.hpp>

using namespace LLP;

TEST_CASE("Stale sync warning limiter emits immediately for first stale block",
    "[llp][sync_warning]")
{
    std::map<uint64_t, StaleSyncWarningState> mapStates;

    const StaleSyncWarningDecision decision =
        RecordStaleSyncWarningEvent(mapStates, 0x1111, 100);

    REQUIRE(decision.fEmitWarning);
    REQUIRE(decision.nSuppressedBlocks == 1);
    REQUIRE(mapStates.size() == 1);
    REQUIRE(mapStates.at(0x1111).fHasEmittedWarning == true);
    REQUIRE(mapStates.at(0x1111).nLastWarningTime == 100);
    REQUIRE(mapStates.at(0x1111).nSuppressedBlocks == 0);
}

TEST_CASE("Stale sync warning limiter suppresses repeated events inside interval",
    "[llp][sync_warning]")
{
    std::map<uint64_t, StaleSyncWarningState> mapStates;

    REQUIRE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 100).fEmitWarning);

    const StaleSyncWarningDecision second =
        RecordStaleSyncWarningEvent(mapStates, 0x1111, 120);
    const StaleSyncWarningDecision third =
        RecordStaleSyncWarningEvent(mapStates, 0x1111, 130);

    REQUIRE_FALSE(second.fEmitWarning);
    REQUIRE(second.nSuppressedBlocks == 0);
    REQUIRE_FALSE(third.fEmitWarning);
    REQUIRE(third.nSuppressedBlocks == 0);
    REQUIRE(mapStates.at(0x1111).nSuppressedBlocks == 2);
}

TEST_CASE("Stale sync warning limiter reports accumulated count after interval",
    "[llp][sync_warning]")
{
    std::map<uint64_t, StaleSyncWarningState> mapStates;

    REQUIRE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 100).fEmitWarning);
    REQUIRE_FALSE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 120).fEmitWarning);
    REQUIRE_FALSE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 130).fEmitWarning);

    const StaleSyncWarningDecision decision =
        RecordStaleSyncWarningEvent(mapStates, 0x1111,
            100 + STALE_SYNC_WARNING_THROTTLE_SECONDS);

    REQUIRE(decision.fEmitWarning);
    REQUIRE(decision.nSuppressedBlocks == 3);
    REQUIRE(mapStates.at(0x1111).nSuppressedBlocks == 0);
    REQUIRE(mapStates.at(0x1111).nLastWarningTime
        == 100 + STALE_SYNC_WARNING_THROTTLE_SECONDS);
}

TEST_CASE("Stale sync warning limiter tracks sessions independently",
    "[llp][sync_warning]")
{
    std::map<uint64_t, StaleSyncWarningState> mapStates;

    REQUIRE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 100).fEmitWarning);
    REQUIRE(RecordStaleSyncWarningEvent(mapStates, 0x2222, 100).fEmitWarning);

    REQUIRE_FALSE(RecordStaleSyncWarningEvent(mapStates, 0x1111, 110).fEmitWarning);
    REQUIRE_FALSE(RecordStaleSyncWarningEvent(mapStates, 0x2222, 110).fEmitWarning);

    const StaleSyncWarningDecision otherSession =
        RecordStaleSyncWarningEvent(mapStates, 0x2222,
            100 + STALE_SYNC_WARNING_THROTTLE_SECONDS);

    REQUIRE(otherSession.fEmitWarning);
    REQUIRE(otherSession.nSuppressedBlocks == 2);
    REQUIRE(mapStates.at(0x1111).nSuppressedBlocks == 1);
    REQUIRE(mapStates.at(0x2222).nSuppressedBlocks == 0);
}

TEST_CASE("Stale sync warning limiter bounds and resets session state",
    "[llp][sync_warning]")
{
    std::map<uint64_t, StaleSyncWarningState> mapStates;

    for(std::size_t i = 0; i < STALE_SYNC_WARNING_MAX_ENTRIES; ++i)
        REQUIRE(RecordStaleSyncWarningEvent(mapStates, i + 1, 100).fEmitWarning);

    REQUIRE(mapStates.size() == STALE_SYNC_WARNING_MAX_ENTRIES);

    const StaleSyncWarningDecision decision =
        RecordStaleSyncWarningEvent(mapStates,
            STALE_SYNC_WARNING_MAX_ENTRIES + 1, 100);

    REQUIRE(decision.fEmitWarning);
    REQUIRE(decision.nSuppressedBlocks == 1);
    REQUIRE(mapStates.size() == 1);
    REQUIRE(mapStates.count(STALE_SYNC_WARNING_MAX_ENTRIES + 1) == 1);

    REQUIRE(ResetStaleSyncWarningEvent(mapStates,
        STALE_SYNC_WARNING_MAX_ENTRIES + 1));
    REQUIRE(mapStates.empty());
}
