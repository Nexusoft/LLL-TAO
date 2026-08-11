/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/mining_template_delivery.h>

#include <chrono>
#include <cstdint>

/*
 * Push-throttle invariants.
 *
 * The throttle decision is a pure function of five state variables:
 *   - m_last_template_push_time  (time_point, zero-initialised ≡ "never sent")
 *   - m_force_next_push          (bool, false by default)
 *   - m_hashLastPushedChain      (most-recent pushed hash; diagnostic-only)
 *   - m_pushedTipHistory         (time-windowed ring of recent tips — Option B)
 *   - elapsed                    (ms since last push)
 *
 * Invariants verified:
 *   1. First push always sends regardless of force flag.
 *   2. Second push within TEMPLATE_PUSH_MIN_INTERVAL_MS for the SAME tip is
 *      THROTTLED when m_force_next_push == false.
 *   3. m_force_next_push bypasses the throttle and is one-shot.
 *   4. After TEMPLATE_PUSH_MIN_INTERVAL_MS elapses, an unchanged tip passes
 *      with reason INTERVAL_ELAPSED.
 *   5. A tip whose hash is NOT in the recent-push ring bypasses the time floor
 *      with reason CHAIN_TIP_CHANGED.
 *   6. Fork-storm pre-poison fix: a tip whose hash IS in the recent-push ring
 *      (i.e. was delivered earlier in the same TTL window) is treated as a
 *      recurrence and THROTTLED — closes the bug where SetBest cycling
 *      A→B→C→D→A would silently strand the miner because the single-slot
 *      "last pushed" field had been overwritten with D and so A "looked new".
 *   7. After PushedTipHistory::TTL_MS has elapsed, a previously-recorded hash
 *      is treated as new again and bypasses the time floor.
 *   8. Option E: PushedTipHistory::Clear() (called on SUBMIT_BLOCK SUCCESS)
 *      drops the entire history so any subsequent tip — even one that was in
 *      the ring before the clear — bypasses the time floor.
 *   9. Ring-overflow: pushing more than CAPACITY distinct hashes evicts the
 *      oldest entry, which is then treated as new again.
 */

namespace
{
    using steady_clock = std::chrono::steady_clock;
    using time_point   = steady_clock::time_point;
    using LLP::TemplatePushDecision;
    using LLP::TemplatePushDecisionReason;
    using LLP::PushedTipHistory;

    /** PushThrottleState — mirrors the relevant per-connection members. */
    struct PushThrottleState
    {
        time_point       m_last_template_push_time{};   // zero = never sent
        bool             m_force_next_push{false};
        uint1024_t       m_hashLastPushedChain{0};
        PushedTipHistory m_pushedTipHistory{};
    };

    /** SimulateSend — apply the throttle logic to the given state object at "now". */
    TemplatePushDecision SimulateSend(PushThrottleState& state, time_point now,
                                      const uint1024_t& hashBestChain)
    {
        return LLP::ApplyTemplatePushThrottle(
            state.m_last_template_push_time,
            state.m_force_next_push,
            state.m_hashLastPushedChain,
            state.m_pushedTipHistory,
            hashBestChain,
            now);
    }
}

TEST_CASE("Push throttle — first send always passes", "[push_throttle][llp]")
{
    PushThrottleState state;    // m_last_template_push_time == time_point{} (never sent)
    auto now = steady_clock::now();

    const TemplatePushDecision decision = SimulateSend(state, now, uint1024_t(0xA1));
    REQUIRE(decision.fShouldSend);
    REQUIRE(decision.eReason == TemplatePushDecisionReason::FIRST_PUSH);
}

TEST_CASE("Push throttle — rapid second send is throttled without force flag", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    auto t1 = t0 + std::chrono::milliseconds(1);
    const TemplatePushDecision decision = SimulateSend(state, t1, uint1024_t(0xA1));
    REQUIRE_FALSE(decision.fShouldSend);
    REQUIRE(decision.eReason == TemplatePushDecisionReason::THROTTLED);
}

TEST_CASE("Push throttle — rapid second send bypasses throttle with force flag", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    state.m_force_next_push = true;

    auto t1 = t0 + std::chrono::milliseconds(1);
    const TemplatePushDecision decision = SimulateSend(state, t1, uint1024_t(0xA1));
    REQUIRE(decision.fShouldSend);
    REQUIRE(decision.eReason == TemplatePushDecisionReason::FORCE_BYPASS);

    REQUIRE(state.m_force_next_push == false);
}

TEST_CASE("Push throttle — force flag is one-shot: subsequent send obeys throttle", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    state.m_force_next_push = true;
    auto t1 = t0 + std::chrono::milliseconds(1);
    REQUIRE(SimulateSend(state, t1, uint1024_t(0xA1)).fShouldSend);
    REQUIRE(state.m_force_next_push == false);

    auto t2 = t1 + std::chrono::milliseconds(1);
    REQUIRE_FALSE(SimulateSend(state, t2, uint1024_t(0xA1)).fShouldSend);
}

TEST_CASE("Push throttle — send passes after interval expires", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    auto t1 = t0 + std::chrono::milliseconds(LLP::MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS + 1);
    const TemplatePushDecision decision = SimulateSend(state, t1, uint1024_t(0xA1));
    REQUIRE(decision.fShouldSend);
    REQUIRE(decision.eReason == TemplatePushDecisionReason::INTERVAL_ELAPSED);
}

TEST_CASE("Push throttle — rapid send bypasses time floor when chain tip changes", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    auto t1 = t0 + std::chrono::milliseconds(1);
    const TemplatePushDecision decision = SimulateSend(state, t1, uint1024_t(0xB2));
    REQUIRE(decision.fShouldSend);
    REQUIRE(decision.eReason == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
}

/* ---------------------------------------------------------------------------
 *  OPTION B regression coverage — fork-storm pre-poison fix.
 *  These tests document the exact scenario that motivated PushedTipHistory.
 * ------------------------------------------------------------------------ */

TEST_CASE("Push throttle — fork storm with hash recurrence (Option B)", "[push_throttle][llp][fork_storm]")
{
    PushThrottleState state;
    auto t = steady_clock::now();

    /* Storm: SetBest fires for A, B, C, D in rapid succession.  Each is a
     * genuinely new tip — none has been in the ring — so all pass with
     * CHAIN_TIP_CHANGED. */
    REQUIRE(SimulateSend(state, t,                          uint1024_t(0xAA)).eReason
            == TemplatePushDecisionReason::FIRST_PUSH);
    REQUIRE(SimulateSend(state, t + std::chrono::milliseconds(8),  uint1024_t(0xBB)).eReason
            == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
    REQUIRE(SimulateSend(state, t + std::chrono::milliseconds(16), uint1024_t(0xCC)).eReason
            == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
    REQUIRE(SimulateSend(state, t + std::chrono::milliseconds(24), uint1024_t(0xDD)).eReason
            == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);

    /* Reorg back to A.  In the OLD single-slot logic this would falsely
     * trigger CHAIN_TIP_CHANGED (because the recorded "last hash" was D, so
     * A != D), exhausting the bypass.  In the NEW ring-history logic A is
     * still in the ring, so the throttle correctly time-gates it. */
    const TemplatePushDecision recur = SimulateSend(state,
        t + std::chrono::milliseconds(32), uint1024_t(0xAA));
    REQUIRE_FALSE(recur.fShouldSend);
    REQUIRE(recur.eReason == TemplatePushDecisionReason::THROTTLED);

    /* A genuinely new post-storm tip E still bypasses immediately. */
    const TemplatePushDecision postStorm = SimulateSend(state,
        t + std::chrono::milliseconds(40), uint1024_t(0xEE));
    REQUIRE(postStorm.fShouldSend);
    REQUIRE(postStorm.eReason == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
}

TEST_CASE("Push throttle — recorded tip ages out of ring after TTL", "[push_throttle][llp]")
{
    PushThrottleState state;
    auto t0 = steady_clock::now();

    REQUIRE(SimulateSend(state, t0, uint1024_t(0xA1)).fShouldSend);

    /* Advance past TTL — the ring entry for 0xA1 should be considered aged-out. */
    auto t1 = t0 + std::chrono::milliseconds(PushedTipHistory::TTL_MS + 1);
    const TemplatePushDecision decision = SimulateSend(state, t1, uint1024_t(0xA1));
    REQUIRE(decision.fShouldSend);
    /* Either CHAIN_TIP_CHANGED (treated as new because aged-out) or
     * INTERVAL_ELAPSED is acceptable; the ring path runs first so we expect
     * CHAIN_TIP_CHANGED. */
    REQUIRE(decision.eReason == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
}

TEST_CASE("Push throttle — Clear() restores bypass for previously-pushed tip (Option E)",
          "[push_throttle][llp][submit_block]")
{
    PushThrottleState state;
    auto t = steady_clock::now();

    /* Storm pre-poisons the ring with multiple tips. */
    REQUIRE(SimulateSend(state, t,                                 uint1024_t(0xAA)).fShouldSend);
    REQUIRE(SimulateSend(state, t + std::chrono::milliseconds(5),  uint1024_t(0xBB)).fShouldSend);
    REQUIRE(SimulateSend(state, t + std::chrono::milliseconds(10), uint1024_t(0xCC)).fShouldSend);

    /* Without Clear, immediate re-delivery of an in-ring tip is throttled. */
    const TemplatePushDecision blocked = SimulateSend(state,
        t + std::chrono::milliseconds(15), uint1024_t(0xAA));
    REQUIRE_FALSE(blocked.fShouldSend);
    REQUIRE(blocked.eReason == TemplatePushDecisionReason::THROTTLED);

    /* SUBMIT_BLOCK SUCCESS clears the ring. */
    state.m_pushedTipHistory.Clear();

    /* Same hash now bypasses the time floor as if never seen. */
    const TemplatePushDecision unblocked = SimulateSend(state,
        t + std::chrono::milliseconds(20), uint1024_t(0xAA));
    REQUIRE(unblocked.fShouldSend);
    REQUIRE(unblocked.eReason == TemplatePushDecisionReason::CHAIN_TIP_CHANGED);
}

TEST_CASE("PushedTipHistory — ContainsRecent / Record direct API", "[push_throttle][llp][history]")
{
    PushedTipHistory history;
    auto t = steady_clock::now();

    REQUIRE_FALSE(history.ContainsRecent(uint1024_t(0xAA), t));
    REQUIRE(history.SizeRecent(t) == 0);

    history.Record(uint1024_t(0xAA), t);
    REQUIRE(history.ContainsRecent(uint1024_t(0xAA), t));
    REQUIRE(history.SizeRecent(t) == 1);

    /* Re-recording the same hash refreshes its timestamp without duplicating. */
    history.Record(uint1024_t(0xAA), t + std::chrono::milliseconds(100));
    REQUIRE(history.SizeRecent(t + std::chrono::milliseconds(100)) == 1);

    /* After TTL the entry ages out. */
    REQUIRE_FALSE(history.ContainsRecent(uint1024_t(0xAA),
        t + std::chrono::milliseconds(100 + PushedTipHistory::TTL_MS + 1)));

    /* Clear empties the ring. */
    history.Record(uint1024_t(0xBB), t);
    history.Record(uint1024_t(0xCC), t);
    REQUIRE(history.SizeRecent(t) >= 2);
    history.Clear();
    REQUIRE(history.SizeRecent(t) == 0);
    REQUIRE_FALSE(history.ContainsRecent(uint1024_t(0xBB), t));
}

TEST_CASE("PushedTipHistory — ring overflow evicts oldest", "[push_throttle][llp][history]")
{
    PushedTipHistory history;
    auto t = steady_clock::now();

    /* Fill the ring with CAPACITY distinct, still-fresh hashes. */
    for(std::size_t i = 0; i < PushedTipHistory::CAPACITY; ++i)
    {
        history.Record(uint1024_t(0x1000 + i), t + std::chrono::milliseconds(static_cast<int>(i)));
    }
    REQUIRE(history.SizeRecent(t + std::chrono::milliseconds(static_cast<int>(PushedTipHistory::CAPACITY)))
            == PushedTipHistory::CAPACITY);

    /* All recorded hashes are present. */
    for(std::size_t i = 0; i < PushedTipHistory::CAPACITY; ++i)
    {
        REQUIRE(history.ContainsRecent(uint1024_t(0x1000 + i),
            t + std::chrono::milliseconds(static_cast<int>(PushedTipHistory::CAPACITY))));
    }

    /* Push one more distinct hash — the round-robin slot 0 gets overwritten,
     * and that slot's prior occupant (0x1000) is evicted. */
    const auto tOverflow = t + std::chrono::milliseconds(static_cast<int>(PushedTipHistory::CAPACITY + 1));
    history.Record(uint1024_t(0x2000), tOverflow);
    REQUIRE(history.ContainsRecent(uint1024_t(0x2000), tOverflow));
    REQUIRE_FALSE(history.ContainsRecent(uint1024_t(0x1000), tOverflow));

    /* The other CAPACITY-1 originals are still tracked. */
    for(std::size_t i = 1; i < PushedTipHistory::CAPACITY; ++i)
    {
        REQUIRE(history.ContainsRecent(uint1024_t(0x1000 + i), tOverflow));
    }
}
