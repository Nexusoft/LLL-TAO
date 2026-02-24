/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_constants.h>

#include <chrono>
#include <cstdint>

/*
 * Minimal simulation of the push throttle + force-bypass logic used in
 * StatelessMinerConnection::SendChannelNotification() and
 * StatelessMinerConnection::SendStatelessTemplate() / Miner::SendChannelNotification().
 *
 * The actual methods cannot be called in unit tests without a live socket, but the
 * throttle decision is a pure function of three state variables:
 *   - m_last_template_push_time  (time_point, zero-initialised ≡ "never sent")
 *   - m_force_next_push          (bool, false by default)
 *   - elapsed                    (ms since last push)
 *
 * This test suite verifies the three invariants required by the doom-loop fix:
 *   1. First push always sends (no prior timestamp → elapsed undefined, no throttle).
 *   2. Second push within TEMPLATE_PUSH_MIN_INTERVAL_MS is throttled when
 *      m_force_next_push == false.
 *   3. Second push within TEMPLATE_PUSH_MIN_INTERVAL_MS is NOT throttled (bypassed)
 *      when m_force_next_push == true, and the flag is cleared afterwards.
 */

namespace
{
    using steady_clock = std::chrono::steady_clock;
    using time_point   = steady_clock::time_point;

    /** ThrottleResult — outcome of a simulated SendChannelNotification() call. */
    enum class ThrottleResult { SEND, THROTTLED };

    /** PushThrottleState — mirrors the relevant per-connection members. */
    struct PushThrottleState
    {
        time_point m_last_template_push_time{};   // zero = never sent
        bool       m_force_next_push{false};
    };

    /**
     * SimulateSend — apply the throttle logic from SendChannelNotification() /
     * SendStatelessTemplate() to the given state object and a caller-supplied "now".
     *
     * Returns SEND if the push would be transmitted, THROTTLED if it would be dropped.
     * Updates state in-place exactly as the production code does.
     */
    ThrottleResult SimulateSend(PushThrottleState& state, time_point now)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - state.m_last_template_push_time).count();

        if(state.m_force_next_push)
        {
            /* Re-subscription bypass: clear the flag and send. */
            state.m_force_next_push = false;
            state.m_last_template_push_time = now;
            return ThrottleResult::SEND;
        }
        else if(state.m_last_template_push_time != time_point{} &&
                elapsed < LLP::MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS)
        {
            return ThrottleResult::THROTTLED;
        }

        state.m_last_template_push_time = now;
        return ThrottleResult::SEND;
    }
}

TEST_CASE("Push throttle — first send always passes", "[push_throttle][llp]")
{
    PushThrottleState state;    // m_last_template_push_time == time_point{} (never sent)
    auto now = steady_clock::now();

    /* No prior timestamp: should always send regardless of m_force_next_push. */
    REQUIRE(SimulateSend(state, now) == ThrottleResult::SEND);
}

TEST_CASE("Push throttle — rapid second send is throttled without force flag", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    /* First send establishes the timestamp. */
    REQUIRE(SimulateSend(state, t0) == ThrottleResult::SEND);

    /* Second send 1 ms later — well within TEMPLATE_PUSH_MIN_INTERVAL_MS (1000 ms). */
    auto t1 = t0 + std::chrono::milliseconds(1);
    REQUIRE(SimulateSend(state, t1) == ThrottleResult::THROTTLED);
}

TEST_CASE("Push throttle — rapid second send bypasses throttle with force flag", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    /* First send establishes the timestamp. */
    REQUIRE(SimulateSend(state, t0) == ThrottleResult::SEND);

    /* Set force flag (simulates STATELESS_MINER_READY / MINER_READY handler). */
    state.m_force_next_push = true;

    /* Second send 1 ms later — would normally be throttled but force flag bypasses it. */
    auto t1 = t0 + std::chrono::milliseconds(1);
    REQUIRE(SimulateSend(state, t1) == ThrottleResult::SEND);

    /* Force flag must be cleared after the bypassed send. */
    REQUIRE(state.m_force_next_push == false);
}

TEST_CASE("Push throttle — force flag is one-shot: subsequent send obeys throttle", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    /* First send. */
    REQUIRE(SimulateSend(state, t0) == ThrottleResult::SEND);

    /* Force bypass: second send passes. */
    state.m_force_next_push = true;
    auto t1 = t0 + std::chrono::milliseconds(1);
    REQUIRE(SimulateSend(state, t1) == ThrottleResult::SEND);
    REQUIRE(state.m_force_next_push == false);

    /* Third send immediately after — flag cleared, should be throttled again. */
    auto t2 = t1 + std::chrono::milliseconds(1);
    REQUIRE(SimulateSend(state, t2) == ThrottleResult::THROTTLED);
}

TEST_CASE("Push throttle — send passes after interval expires", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    /* First send. */
    REQUIRE(SimulateSend(state, t0) == ThrottleResult::SEND);

    /* Second send after TEMPLATE_PUSH_MIN_INTERVAL_MS + 1 ms — throttle expired. */
    auto t1 = t0 + std::chrono::milliseconds(LLP::MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS + 1);
    REQUIRE(SimulateSend(state, t1) == ThrottleResult::SEND);
}
