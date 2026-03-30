/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/push_notification.h>
#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/mining_constants.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/opcode_utility.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <Util/include/convert.h>

#include <vector>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

/* ===========================================================================
 *
 *  NODE Burst Mode Push Notification Tests
 *
 *  Validates push notification broadcast behaviour under high-load conditions
 *  with 500-1000 active miners.  These tests simulate the channel-filtering,
 *  deduplication, throttle, and payload-construction paths that run inside
 *  Server<T>::NotifyChannelMiners() and MinerPushDispatcher without requiring
 *  live sockets or a running node.
 *
 *  Key test areas:
 *    1. Channel filtering efficiency at scale (500-1000 miners)
 *    2. Burst deduplication under rapid SetBest() events
 *    3. Push throttle behaviour during fork-resolution bursts
 *    4. Payload construction throughput at scale
 *    5. Cross-channel isolation with large miner populations
 *    6. Mixed protocol lanes (Legacy + Stateless) under burst load
 *
 * =========================================================================== */


/* ===========================================================================
 *  Local simulation helpers
 *  Mirror the filtering logic from Server<T>::NotifyChannelMiners() and the
 *  throttle logic from SendChannelNotification().
 * =========================================================================== */
namespace
{
    using steady_clock = std::chrono::steady_clock;
    using time_point   = steady_clock::time_point;

    /* --- Miner subscription state (mirrors MiningContext fields) ---------- */
    struct SimMinerCtx
    {
        bool     fSubscribedToNotifications{false};
        uint32_t nSubscribedChannel{0};
        bool     fConnected{true};
    };

    /* --- Per-connection push throttle state ------------------------------- */
    enum class ThrottleResult { SEND, THROTTLED };

    struct PushThrottleState
    {
        time_point m_last_template_push_time{};
        bool       m_force_next_push{false};
    };

    ThrottleResult SimulateSend(PushThrottleState& state, time_point now)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - state.m_last_template_push_time).count();

        if (state.m_force_next_push)
        {
            state.m_force_next_push = false;
            state.m_last_template_push_time = now;
            return ThrottleResult::SEND;
        }
        else if (state.m_last_template_push_time != time_point{} &&
                 elapsed < LLP::MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS)
        {
            return ThrottleResult::THROTTLED;
        }

        state.m_last_template_push_time = now;
        return ThrottleResult::SEND;
    }

    /* --- Channel filter (mirrors NotifyChannelMiners inner loop) --------- */
    struct BroadcastResult
    {
        uint32_t nNotified{0};
        uint32_t nSkippedWrongChannel{0};
        uint32_t nSkippedUnsubscribed{0};
        uint32_t nSkippedDisconnected{0};
    };

    BroadcastResult SimulateBroadcast(const std::vector<SimMinerCtx>& vMiners,
                                      uint32_t nChannel)
    {
        BroadcastResult result;
        for (const auto& ctx : vMiners)
        {
            if (!ctx.fConnected)
            {
                ++result.nSkippedDisconnected;
                continue;
            }
            if (!ctx.fSubscribedToNotifications)
            {
                ++result.nSkippedUnsubscribed;
                continue;
            }
            if (ctx.nSubscribedChannel != nChannel)
            {
                ++result.nSkippedWrongChannel;
                continue;
            }
            ++result.nNotified;
        }
        return result;
    }

    /* --- Dedup key builder (mirrors DispatchPushEvent logic) ------------- */
    uint64_t MakeDedupKey(uint32_t nHeight, uint32_t hashPrefix4)
    {
        return (static_cast<uint64_t>(nHeight) << 32) | hashPrefix4;
    }

    /* --- Decode big-endian uint32_t from payload bytes -------------------- */
    uint32_t DecodeBE32(const std::vector<uint8_t>& vData, size_t offset)
    {
        return (static_cast<uint32_t>(vData[offset])     << 24) |
               (static_cast<uint32_t>(vData[offset + 1]) << 16) |
               (static_cast<uint32_t>(vData[offset + 2]) <<  8) |
                static_cast<uint32_t>(vData[offset + 3]);
    }

    /* --- Generate a mixed miner pool ------------------------------------- */
    std::vector<SimMinerCtx> GenerateMinerPool(uint32_t nTotal,
                                               double dPrimeFraction = 0.6,
                                               double dPollingFraction = 0.05,
                                               double dDisconnectedFraction = 0.02)
    {
        std::vector<SimMinerCtx> vMiners(nTotal);
        std::mt19937 rng(42);  /* deterministic for reproducibility */

        const uint32_t nPolling      = static_cast<uint32_t>(nTotal * dPollingFraction);
        const uint32_t nDisconnected = static_cast<uint32_t>(nTotal * dDisconnectedFraction);
        const uint32_t nSubscribed   = nTotal - nPolling - nDisconnected;
        const uint32_t nPrime        = static_cast<uint32_t>(nSubscribed * dPrimeFraction);

        uint32_t idx = 0;

        /* Prime subscribers */
        for (uint32_t i = 0; i < nPrime && idx < nTotal; ++i, ++idx)
            vMiners[idx] = { true, 1, true };

        /* Hash subscribers */
        for (; idx < nPolling + nDisconnected + nSubscribed && idx < nTotal; ++idx)
            vMiners[idx] = { true, 2, true };

        /* Polling miners (unsubscribed) */
        for (uint32_t i = 0; i < nPolling && idx < nTotal; ++i, ++idx)
            vMiners[idx] = { false, 1, true };

        /* Disconnected miners */
        for (; idx < nTotal; ++idx)
            vMiners[idx] = { false, 0, false };

        /* Shuffle for realism */
        std::shuffle(vMiners.begin(), vMiners.end(), rng);
        return vMiners;
    }

} // anonymous namespace


/* ===========================================================================
 *  Section 1: Channel Filtering at Scale (500-1000 Miners)
 * =========================================================================== */

TEST_CASE("Burst Mode — Channel filtering with 500 miners", "[burst_mode][push_notification][llp]")
{
    const uint32_t N = 500;
    auto vMiners = GenerateMinerPool(N);

    /* Count expected distribution */
    uint32_t nExpPrime = 0, nExpHash = 0, nExpPolling = 0, nExpDisconnected = 0;
    for (const auto& m : vMiners)
    {
        if (!m.fConnected) ++nExpDisconnected;
        else if (!m.fSubscribedToNotifications) ++nExpPolling;
        else if (m.nSubscribedChannel == 1) ++nExpPrime;
        else if (m.nSubscribedChannel == 2) ++nExpHash;
    }

    SECTION("Prime broadcast reaches only Prime subscribers")
    {
        auto result = SimulateBroadcast(vMiners, 1);
        REQUIRE(result.nNotified == nExpPrime);
        REQUIRE(result.nSkippedWrongChannel == nExpHash);
        REQUIRE(result.nSkippedUnsubscribed == nExpPolling);
        REQUIRE(result.nSkippedDisconnected == nExpDisconnected);
        /* Invariant: all miners accounted for */
        REQUIRE((result.nNotified + result.nSkippedWrongChannel +
                 result.nSkippedUnsubscribed + result.nSkippedDisconnected) == N);
    }

    SECTION("Hash broadcast reaches only Hash subscribers")
    {
        auto result = SimulateBroadcast(vMiners, 2);
        REQUIRE(result.nNotified == nExpHash);
        REQUIRE(result.nSkippedWrongChannel == nExpPrime);
        REQUIRE(result.nSkippedUnsubscribed == nExpPolling);
        REQUIRE(result.nSkippedDisconnected == nExpDisconnected);
        REQUIRE((result.nNotified + result.nSkippedWrongChannel +
                 result.nSkippedUnsubscribed + result.nSkippedDisconnected) == N);
    }
}

TEST_CASE("Burst Mode — Channel filtering with 1000 miners", "[burst_mode][push_notification][llp]")
{
    const uint32_t N = 1000;
    auto vMiners = GenerateMinerPool(N);

    uint32_t nExpPrime = 0, nExpHash = 0, nExpPolling = 0, nExpDisconnected = 0;
    for (const auto& m : vMiners)
    {
        if (!m.fConnected) ++nExpDisconnected;
        else if (!m.fSubscribedToNotifications) ++nExpPolling;
        else if (m.nSubscribedChannel == 1) ++nExpPrime;
        else if (m.nSubscribedChannel == 2) ++nExpHash;
    }

    SECTION("Full broadcast cycle — both channels")
    {
        auto resultPrime = SimulateBroadcast(vMiners, 1);
        auto resultHash  = SimulateBroadcast(vMiners, 2);

        /* No miner receives notifications for both channels */
        REQUIRE(resultPrime.nNotified == nExpPrime);
        REQUIRE(resultHash.nNotified  == nExpHash);

        /* Total unique notifications < total miners (polling + disconnected excluded) */
        REQUIRE((resultPrime.nNotified + resultHash.nNotified) < N);

        /* All miners accounted for in each pass */
        REQUIRE((resultPrime.nNotified + resultPrime.nSkippedWrongChannel +
                 resultPrime.nSkippedUnsubscribed + resultPrime.nSkippedDisconnected) == N);
        REQUIRE((resultHash.nNotified + resultHash.nSkippedWrongChannel +
                 resultHash.nSkippedUnsubscribed + resultHash.nSkippedDisconnected) == N);
    }

    SECTION("No cross-channel pollution at scale")
    {
        /* Build a pool of only Prime miners */
        std::vector<SimMinerCtx> vPrimeOnly(N, { true, 1, true });
        auto resultHash = SimulateBroadcast(vPrimeOnly, 2);
        REQUIRE(resultHash.nNotified == 0);
        REQUIRE(resultHash.nSkippedWrongChannel == N);

        /* Build a pool of only Hash miners */
        std::vector<SimMinerCtx> vHashOnly(N, { true, 2, true });
        auto resultPrime = SimulateBroadcast(vHashOnly, 1);
        REQUIRE(resultPrime.nNotified == 0);
        REQUIRE(resultPrime.nSkippedWrongChannel == N);
    }
}


/* ===========================================================================
 *  Section 2: Deduplication Under Rapid Burst Events
 * =========================================================================== */

TEST_CASE("Burst Mode — Dedup key uniqueness across heights", "[burst_mode][dedup][llp]")
{
    /* Simulate rapid SetBest() events at consecutive heights with same hash prefix */
    const uint32_t hashPrefix = 0xDEADBEEF;

    SECTION("Different heights produce different dedup keys")
    {
        uint64_t key1 = MakeDedupKey(1000, hashPrefix);
        uint64_t key2 = MakeDedupKey(1001, hashPrefix);
        uint64_t key3 = MakeDedupKey(1002, hashPrefix);

        REQUIRE(key1 != key2);
        REQUIRE(key2 != key3);
        REQUIRE(key1 != key3);
    }

    SECTION("Same height + same hash prefix produces identical dedup key")
    {
        uint64_t keyA = MakeDedupKey(5000, 0xAABBCCDD);
        uint64_t keyB = MakeDedupKey(5000, 0xAABBCCDD);
        REQUIRE(keyA == keyB);
    }

    SECTION("Same height + different hash prefix produces different dedup keys")
    {
        uint64_t keyA = MakeDedupKey(5000, 0xAABBCCDD);
        uint64_t keyB = MakeDedupKey(5000, 0xEEFF0011);
        REQUIRE(keyA != keyB);
    }
}

TEST_CASE("Burst Mode — Dedup prevents duplicate dispatches during fork burst", "[burst_mode][dedup][llp]")
{
    /* Simulate a fork-resolution burst: 10 rapid SetBest() events, some with
     * the same (height, hashPrefix) pair that should be deduplicated. */
    std::atomic<uint64_t> dedupState{0};

    struct DispatchAttempt
    {
        uint32_t nHeight;
        uint32_t hashPrefix;
        bool     fExpectedSend;
    };

    std::vector<DispatchAttempt> vBurst = {
        { 1000, 0xAAAAAAAA, true  },  /* First at height 1000: dispatches */
        { 1000, 0xAAAAAAAA, false },  /* Duplicate: suppressed */
        { 1000, 0xAAAAAAAA, false },  /* Duplicate: suppressed */
        { 1001, 0xBBBBBBBB, true  },  /* New height: dispatches */
        { 1001, 0xBBBBBBBB, false },  /* Duplicate: suppressed */
        { 1002, 0xCCCCCCCC, true  },  /* New height: dispatches */
        { 1002, 0xCCCCCCCC, false },  /* Duplicate: suppressed */
        { 1002, 0xCCCCCCCC, false },  /* Duplicate: suppressed */
        { 1002, 0xCCCCCCCC, false },  /* Duplicate: suppressed */
        { 1003, 0xDDDDDDDD, true  },  /* New height: dispatches */
    };

    uint32_t nDispatched = 0;
    uint32_t nSuppressed = 0;

    for (const auto& attempt : vBurst)
    {
        uint64_t newKey = MakeDedupKey(attempt.nHeight, attempt.hashPrefix);
        uint64_t expected = dedupState.load();

        /* CAS: only first call with a new key value wins */
        if (expected != newKey && dedupState.compare_exchange_strong(expected, newKey))
        {
            ++nDispatched;
            REQUIRE(attempt.fExpectedSend == true);
        }
        else
        {
            ++nSuppressed;
            REQUIRE(attempt.fExpectedSend == false);
        }
    }

    REQUIRE(nDispatched == 4);
    REQUIRE(nSuppressed == 6);
    REQUIRE((nDispatched + nSuppressed) == vBurst.size());
}


/* ===========================================================================
 *  Section 3: Push Throttle Under Burst Conditions with Many Miners
 * =========================================================================== */

TEST_CASE("Burst Mode — Throttle behaviour with 500 miner connections during fork burst", "[burst_mode][throttle][llp]")
{
    const uint32_t N = 500;
    std::vector<PushThrottleState> vStates(N);
    auto tBase = steady_clock::now();

    SECTION("First push to all 500 miners succeeds")
    {
        uint32_t nSent = 0;
        for (auto& state : vStates)
        {
            if (SimulateSend(state, tBase) == ThrottleResult::SEND)
                ++nSent;
        }
        REQUIRE(nSent == N);
    }

    SECTION("Rapid second push within 1ms is throttled for all miners")
    {
        /* First push for all */
        for (auto& state : vStates)
            SimulateSend(state, tBase);

        /* Second push 1ms later */
        auto t1 = tBase + std::chrono::milliseconds(1);
        uint32_t nThrottled = 0;
        for (auto& state : vStates)
        {
            if (SimulateSend(state, t1) == ThrottleResult::THROTTLED)
                ++nThrottled;
        }
        REQUIRE(nThrottled == N);
    }

    SECTION("Force flag bypasses throttle for re-subscribing miners during burst")
    {
        /* First push for all */
        for (auto& state : vStates)
            SimulateSend(state, tBase);

        /* Simulate 50 miners re-subscribing (MINER_READY) during the burst */
        const uint32_t nResubscribers = 50;
        for (uint32_t i = 0; i < nResubscribers; ++i)
            vStates[i].m_force_next_push = true;

        auto t1 = tBase + std::chrono::milliseconds(1);
        uint32_t nSent = 0, nThrottled = 0;
        for (auto& state : vStates)
        {
            auto result = SimulateSend(state, t1);
            if (result == ThrottleResult::SEND) ++nSent;
            else ++nThrottled;
        }

        REQUIRE(nSent == nResubscribers);
        REQUIRE(nThrottled == (N - nResubscribers));
    }

    SECTION("All miners receive push after throttle interval expires")
    {
        /* First push */
        for (auto& state : vStates)
            SimulateSend(state, tBase);

        /* Wait for throttle to expire */
        auto tAfter = tBase + std::chrono::milliseconds(
            LLP::MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS + 1);

        uint32_t nSent = 0;
        for (auto& state : vStates)
        {
            if (SimulateSend(state, tAfter) == ThrottleResult::SEND)
                ++nSent;
        }
        REQUIRE(nSent == N);
    }
}

TEST_CASE("Burst Mode — Throttle behaviour with 1000 miner connections", "[burst_mode][throttle][llp]")
{
    const uint32_t N = 1000;
    std::vector<PushThrottleState> vStates(N);
    auto tBase = steady_clock::now();

    SECTION("Staggered re-subscription during burst — only re-subscribers bypass throttle")
    {
        /* First push for all */
        for (auto& state : vStates)
            SimulateSend(state, tBase);

        /* Simulate 200 miners re-subscribing at different offsets within the burst */
        const uint32_t nWave1 = 100;
        const uint32_t nWave2 = 100;

        /* Wave 1 at +10ms */
        for (uint32_t i = 0; i < nWave1; ++i)
            vStates[i].m_force_next_push = true;

        auto t10ms = tBase + std::chrono::milliseconds(10);
        uint32_t nSentWave1 = 0;
        for (uint32_t i = 0; i < N; ++i)
        {
            if (SimulateSend(vStates[i], t10ms) == ThrottleResult::SEND)
                ++nSentWave1;
        }
        REQUIRE(nSentWave1 == nWave1);

        /* Wave 2 at +50ms (still within throttle interval) */
        for (uint32_t i = nWave1; i < nWave1 + nWave2; ++i)
            vStates[i].m_force_next_push = true;

        auto t50ms = tBase + std::chrono::milliseconds(50);
        uint32_t nSentWave2 = 0;
        for (uint32_t i = 0; i < N; ++i)
        {
            if (SimulateSend(vStates[i], t50ms) == ThrottleResult::SEND)
                ++nSentWave2;
        }
        /* Only the wave-2 re-subscribers should bypass; everyone else still throttled */
        REQUIRE(nSentWave2 == nWave2);
    }
}


/* ===========================================================================
 *  Section 4: Payload Construction Throughput at Scale
 * =========================================================================== */

TEST_CASE("Burst Mode — Build 500 push notification payloads", "[burst_mode][payload][llp]")
{
    TAO::Ledger::BlockState stateBest;
    stateBest.nHeight = 7500000;

    TAO::Ledger::BlockState statePrime;
    statePrime.nChannelHeight = 2500000;

    TAO::Ledger::BlockState stateHash;
    stateHash.nChannelHeight = 4800000;

    uint32_t nPrimeBits = 0x03C00000;
    uint32_t nHashBits  = 0x1D00FFFF;

    SECTION("500 legacy Prime notifications — correct opcode and size")
    {
        for (uint32_t i = 0; i < 500; ++i)
        {
            stateBest.nHeight = 7500000 + i;

            LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
                1, LLP::ProtocolLane::LEGACY, stateBest, statePrime, nPrimeBits);

            REQUIRE(notification.HEADER == 0xD9);
            REQUIRE(notification.LENGTH == 148);

            /* Verify encoded height matches */
            uint32_t decodedHeight = DecodeBE32(notification.DATA, 0);
            REQUIRE(decodedHeight == stateBest.nHeight);
        }
    }

    SECTION("500 stateless Hash notifications — correct opcode and size")
    {
        for (uint32_t i = 0; i < 500; ++i)
        {
            stateBest.nHeight = 7500000 + i;

            LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
                2, LLP::ProtocolLane::STATELESS, stateBest, stateHash, nHashBits);

            REQUIRE(notification.HEADER == 0xD0DA);
            REQUIRE(notification.LENGTH == 148);

            uint32_t decodedHeight = DecodeBE32(notification.DATA, 0);
            REQUIRE(decodedHeight == stateBest.nHeight);
        }
    }
}

TEST_CASE("Burst Mode — Build 1000 mixed-lane notifications", "[burst_mode][payload][llp]")
{
    TAO::Ledger::BlockState stateBest;
    stateBest.nHeight = 8000000;

    TAO::Ledger::BlockState stateChannel;
    stateChannel.nChannelHeight = 3000000;

    uint32_t nBits = 0x03C00000;

    /* Simulate building notifications for 1000 miners split across both lanes */
    uint32_t nLegacyCount    = 0;
    uint32_t nStatelessCount = 0;

    for (uint32_t i = 0; i < 1000; ++i)
    {
        stateBest.nHeight = 8000000 + (i / 4);  /* ~4 miners per height (fork burst) */
        uint32_t nChannel = (i % 2 == 0) ? 1 : 2;

        if (i % 3 == 0)
        {
            /* Legacy lane */
            LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
                nChannel, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nBits);

            REQUIRE(notification.LENGTH == 148);
            uint8_t expectedOpcode = (nChannel == 1) ? 0xD9 : 0xDA;
            REQUIRE(notification.HEADER == expectedOpcode);
            ++nLegacyCount;
        }
        else
        {
            /* Stateless lane */
            LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
                nChannel, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nBits);

            REQUIRE(notification.LENGTH == 148);
            uint16_t expectedOpcode = (nChannel == 1) ? 0xD0D9 : 0xD0DA;
            REQUIRE(notification.HEADER == expectedOpcode);
            ++nStatelessCount;
        }
    }

    /* Verify distribution */
    REQUIRE((nLegacyCount + nStatelessCount) == 1000);
    REQUIRE(nLegacyCount > 0);
    REQUIRE(nStatelessCount > 0);
}


/* ===========================================================================
 *  Section 5: Payload Consistency Under Burst Load
 * =========================================================================== */

TEST_CASE("Burst Mode — Payload bytes identical across lanes at same height", "[burst_mode][payload][llp]")
{
    SECTION("148-byte payload consistency across 500 sequential heights")
    {
        TAO::Ledger::BlockState stateBest;
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 1000000;

        uint32_t nBits = 0x1D00FFFF;

        for (uint32_t h = 5000000; h < 5000500; ++h)
        {
            stateBest.nHeight = h;

            LLP::Packet legacy = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
                1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nBits);

            LLP::StatelessPacket stateless = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
                1, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nBits);

            /* Same payload size */
            REQUIRE(legacy.LENGTH == 148);
            REQUIRE(stateless.LENGTH == 148);

            /* Identical payload bytes (first 20 header bytes + 128 hash bytes) */
            REQUIRE(legacy.DATA.size() == stateless.DATA.size());
            for (size_t b = 0; b < legacy.DATA.size(); ++b)
            {
                REQUIRE(legacy.DATA[b] == stateless.DATA[b]);
            }

            /* Different opcodes per lane */
            REQUIRE(legacy.HEADER == 0xD9);
            REQUIRE(stateless.HEADER == 0xD0D9);
        }
    }
}


/* ===========================================================================
 *  Section 6: Full Burst Simulation — Broadcast + Dedup + Throttle
 * =========================================================================== */

TEST_CASE("Burst Mode — Full broadcast simulation with 750 miners and fork-resolution burst",
          "[burst_mode][integration][llp]")
{
    const uint32_t N = 750;
    auto vMiners = GenerateMinerPool(N, 0.6, 0.05, 0.02);

    /* Count expected per-channel subscribers */
    uint32_t nExpPrime = 0, nExpHash = 0;
    for (const auto& m : vMiners)
    {
        if (m.fConnected && m.fSubscribedToNotifications)
        {
            if (m.nSubscribedChannel == 1) ++nExpPrime;
            else if (m.nSubscribedChannel == 2) ++nExpHash;
        }
    }

    /* Simulate 5 rapid SetBest() events (fork-resolution burst) */
    std::atomic<uint64_t> dedupPrime{0};
    std::atomic<uint64_t> dedupHash{0};

    struct BlockEvent
    {
        uint32_t nHeight;
        uint32_t hashPrefix;
    };

    /* 5 events: 3 unique heights (heights 100, 101, 102), 2 duplicates */
    const std::vector<BlockEvent> vEvents = {
        { 100, 0x11111111 },
        { 100, 0x11111111 },  /* duplicate */
        { 101, 0x22222222 },
        { 102, 0x33333333 },
        { 102, 0x33333333 },  /* duplicate */
    };

    uint32_t nTotalPrimeNotifications = 0;
    uint32_t nTotalHashNotifications  = 0;
    uint32_t nDedupSkips = 0;

    for (const auto& event : vEvents)
    {
        /* Prime channel dedup */
        uint64_t primeKey = MakeDedupKey(event.nHeight, event.hashPrefix);
        uint64_t prevPrime = dedupPrime.load();
        bool fPrimeNew = (prevPrime != primeKey) &&
                         dedupPrime.compare_exchange_strong(prevPrime, primeKey);

        /* Hash channel dedup */
        uint64_t hashKey = MakeDedupKey(event.nHeight, event.hashPrefix);
        uint64_t prevHash = dedupHash.load();
        bool fHashNew = (prevHash != hashKey) &&
                        dedupHash.compare_exchange_strong(prevHash, hashKey);

        if (fPrimeNew)
        {
            auto result = SimulateBroadcast(vMiners, 1);
            nTotalPrimeNotifications += result.nNotified;
        }
        else
        {
            ++nDedupSkips;
        }

        if (fHashNew)
        {
            auto result = SimulateBroadcast(vMiners, 2);
            nTotalHashNotifications += result.nNotified;
        }
        else
        {
            ++nDedupSkips;
        }
    }

    /* 3 unique heights × 2 channels = 6 dispatches, 5 events × 2 channels = 10 attempts → 4 skips */
    REQUIRE(nDedupSkips == 4);

    /* Each unique dispatch notifies exactly the expected per-channel count */
    REQUIRE(nTotalPrimeNotifications == (nExpPrime * 3));  /* 3 unique heights */
    REQUIRE(nTotalHashNotifications  == (nExpHash  * 3));
}


/* ===========================================================================
 *  Section 7: Bandwidth Estimation Under Burst Load
 * =========================================================================== */

TEST_CASE("Burst Mode — Bandwidth calculation for 500-1000 miner push burst", "[burst_mode][bandwidth][llp]")
{
    /* Each push notification is 148 bytes payload.
     * Legacy Packet overhead: 1-byte HEADER + 4-byte LENGTH = 5 bytes.
     * StatelessPacket overhead: 2-byte HEADER + 4-byte LENGTH = 6 bytes.
     *
     * Total per notification:
     *   Legacy:    148 + 5 = 153 bytes
     *   Stateless: 148 + 6 = 154 bytes
     */
    constexpr uint32_t PAYLOAD_SIZE = 148;
    constexpr uint32_t LEGACY_OVERHEAD = 5;     /* 1B opcode + 4B length */
    constexpr uint32_t STATELESS_OVERHEAD = 6;  /* 2B opcode + 4B length */

    SECTION("500-miner burst bandwidth stays under 100 KB")
    {
        const uint32_t nMiners = 500;
        /* Worst case: all 500 miners on stateless lane */
        uint64_t nTotalBytes = static_cast<uint64_t>(nMiners) * (PAYLOAD_SIZE + STATELESS_OVERHEAD);
        /* 500 × 154 = 77,000 bytes = ~75.2 KB */
        REQUIRE(nTotalBytes < 100 * 1024);
    }

    SECTION("1000-miner burst bandwidth stays under 200 KB")
    {
        const uint32_t nMiners = 1000;
        uint64_t nTotalBytes = static_cast<uint64_t>(nMiners) * (PAYLOAD_SIZE + STATELESS_OVERHEAD);
        /* 1000 × 154 = 154,000 bytes = ~150.4 KB */
        REQUIRE(nTotalBytes < 200 * 1024);
    }

    SECTION("Dual-channel burst bandwidth (both Prime and Hash)")
    {
        const uint32_t nPrime = 600;
        const uint32_t nHash  = 400;

        uint64_t nLegacyBytes    = static_cast<uint64_t>(nPrime + nHash) * (PAYLOAD_SIZE + LEGACY_OVERHEAD);
        uint64_t nStatelessBytes = static_cast<uint64_t>(nPrime + nHash) * (PAYLOAD_SIZE + STATELESS_OVERHEAD);

        /* Combined: both lanes broadcast to all their respective subscribers */
        uint64_t nTotalBothLanes = nLegacyBytes + nStatelessBytes;

        /* 1000 × (153 + 154) = 307,000 bytes = ~299.8 KB — manageable for any modern link */
        REQUIRE(nTotalBothLanes < 400 * 1024);

        /* Per-miner bandwidth is minimal */
        REQUIRE((PAYLOAD_SIZE + STATELESS_OVERHEAD) == 154);
        REQUIRE((PAYLOAD_SIZE + LEGACY_OVERHEAD) == 153);
    }
}


/* ===========================================================================
 *  Section 9: Burst Notification Ordering Guarantees
 * =========================================================================== */

TEST_CASE("Burst Mode — Notifications carry monotonically increasing unified heights",
          "[burst_mode][ordering][llp]")
{
    SECTION("500 notifications at sequential heights — monotonic encoded heights")
    {
        TAO::Ledger::BlockState stateBest;
        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 1000;
        uint32_t nBits = 0x03C00000;

        uint32_t nPrevHeight = 0;
        for (uint32_t h = 6000000; h < 6000500; ++h)
        {
            stateBest.nHeight = h;

            LLP::Packet notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::Packet>(
                1, LLP::ProtocolLane::LEGACY, stateBest, stateChannel, nBits);

            uint32_t nDecoded = DecodeBE32(notification.DATA, 0);
            REQUIRE(nDecoded == h);
            REQUIRE(nDecoded > nPrevHeight);
            nPrevHeight = nDecoded;
        }
    }

    SECTION("Channel height correctly encoded for each miner in burst")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 9000000;
        uint32_t nBits = 0x1D00FFFF;

        /* Simulate varying channel heights as different miners have different views */
        for (uint32_t ch = 100000; ch < 100500; ++ch)
        {
            TAO::Ledger::BlockState stateChannel;
            stateChannel.nChannelHeight = ch;

            LLP::StatelessPacket notification = LLP::PushNotificationBuilder::BuildChannelNotification<LLP::StatelessPacket>(
                2, LLP::ProtocolLane::STATELESS, stateBest, stateChannel, nBits);

            uint32_t nDecodedUnified = DecodeBE32(notification.DATA, 0);
            uint32_t nDecodedChannel = DecodeBE32(notification.DATA, 4);
            uint32_t nDecodedBits    = DecodeBE32(notification.DATA, 8);

            REQUIRE(nDecodedUnified == 9000000);
            REQUIRE(nDecodedChannel == ch);
            REQUIRE(nDecodedBits    == 0x1D00FFFF);
        }
    }
}
