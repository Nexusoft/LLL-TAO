/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/failover_connection_tracker.h>

#include <string>
#include <utility>
#include <vector>

TEST_CASE("Failover tracker releases slots and expires abandoned IPs", "[llp][failover]")
{
    using Tracker = LLP::FailoverConnectionTracker;
    auto& tracker = Tracker::Get();
    const auto now = Tracker::Clock::now();
    const std::string strIP = "203.0.113.10";
    const std::string strOther = "203.0.113.11";

    uint64_t nFirst = 0;
    uint64_t nSecond = 0;
    REQUIRE(tracker.RecordConnection(strIP, nFirst, now));
    REQUIRE(tracker.IsFailover(strIP, now));

    /* A second concurrent handshake must not be erased by the first release. */
    REQUIRE(tracker.RecordConnection(strIP, nSecond, now));
    REQUIRE(nFirst == nSecond);
    tracker.ReleaseConnection(strIP, nFirst);
    REQUIRE(tracker.IsFailover(strIP, now));
    tracker.ReleaseConnection(strIP, nSecond);
    REQUIRE_FALSE(tracker.IsFailover(strIP, now));

    /* A stale generation must not clear a later connection from the same IP. */
    uint64_t nStale = 0;
    REQUIRE(tracker.RecordConnection(strIP, nStale, now));
    tracker.ClearConnection(strIP);
    uint64_t nFresh = 0;
    REQUIRE(tracker.RecordConnection(strIP, nFresh, now));
    REQUIRE(nFresh != nStale);
    tracker.ReleaseConnection(strIP, nStale);
    REQUIRE(tracker.IsFailover(strIP, now));
    tracker.ReleaseConnection(strIP, nFresh);
    REQUIRE_FALSE(tracker.IsFailover(strIP, now));

    /* Missed disconnects expire instead of living for the process lifetime. */
    uint64_t nAbandoned = 0;
    REQUIRE(tracker.RecordConnection(strOther, nAbandoned, now));
    REQUIRE(tracker.IsFailover(strOther, now + Tracker::PENDING_TTL - std::chrono::seconds(1)));
    REQUIRE_FALSE(tracker.IsFailover(strOther, now + Tracker::PENDING_TTL));
    tracker.ReleaseConnection(strOther, nAbandoned);

    /* The cap refuses new IPs instead of growing without bound. Release every
     * slot so later tests are not left with a full tracker. */
    std::vector<std::pair<std::string, uint64_t>> vHeld;
    vHeld.reserve(Tracker::MAX_PENDING_IPS);
    for(std::size_t i = 0; i < Tracker::MAX_PENDING_IPS; ++i)
    {
        uint64_t nGeneration = 0;
        const std::string strKey = "failover-cap-" + std::to_string(i);
        REQUIRE(tracker.RecordConnection(strKey, nGeneration, now));
        vHeld.emplace_back(strKey, nGeneration);
    }
    uint64_t nRejected = 0;
    REQUIRE_FALSE(tracker.RecordConnection("failover-cap-overflow", nRejected, now));
    REQUIRE_FALSE(tracker.IsFailover("failover-cap-overflow", now));
    for(const auto& held : vHeld)
        tracker.ReleaseConnection(held.first, held.second);
    REQUIRE_FALSE(tracker.IsFailover(vHeld.front().first, now));
}
