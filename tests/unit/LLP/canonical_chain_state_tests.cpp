/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/canonical_chain_state.h>

#include <chrono>
#include <thread>

using namespace LLP;


/** Test Suite: CanonicalChainState
 *
 *  Comprehensive tests for the canonical chain state snapshot struct
 *  used by the Stateless Block Utility and Colin Mining Agent.
 *
 *  Key functionality tested:
 *  1. Default construction (all fields zero / epoch)
 *  2. is_initialized() — false when default, true after population
 *  3. is_canonically_stale() — returns true for default / old snapshots
 *  4. height_drift_from_canonical() — signed drift computation
 *  5. Field storage and retrieval
 *  6. from_chain_state() factory method
 *  7. Copy and assignment semantics
 *  8. Staleness transition timing
 *  9. Method interaction scenarios
 * 10. Edge cases (uint32_t boundary, overwrite)
 *
 *  Note: height_drift_from_canonical() reads ChainState::tStateBest,
 *  which defaults to height 0 in the unit-test environment.
 **/


/* ═══════════════════════════════════════════════════════════════════════ */
/* 1. DEFAULT CONSTRUCTION                                                */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState Default Construction", "[canonical_chain_state]")
{
    SECTION("All fields are zero / default after construction")
    {
        CanonicalChainState state;

        REQUIRE(state.canonical_unified_height == 0);
        REQUIRE(state.canonical_channel_height == 0);
        REQUIRE(state.canonical_difficulty_nbits == 0);
        REQUIRE(state.canonical_channel_target == 0);
        REQUIRE(state.canonical_hash_prev_block == uint1024_t(0));

        /* steady_clock default time_point is the epoch */
        REQUIRE(state.canonical_received_at == std::chrono::steady_clock::time_point{});
    }

    SECTION("CANONICAL_STALE_SECONDS constant is 30")
    {
        REQUIRE(CanonicalChainState::CANONICAL_STALE_SECONDS == 30);
    }

    SECTION("Default state satisfies aggregate initialization contract")
    {
        CanonicalChainState a{};
        CanonicalChainState b;

        REQUIRE(a.canonical_unified_height == b.canonical_unified_height);
        REQUIRE(a.canonical_channel_height == b.canonical_channel_height);
        REQUIRE(a.canonical_difficulty_nbits == b.canonical_difficulty_nbits);
        REQUIRE(a.canonical_channel_target == b.canonical_channel_target);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 2. is_initialized()                                                    */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState is_initialized", "[canonical_chain_state]")
{
    SECTION("Default-constructed state is not initialized")
    {
        CanonicalChainState state;
        REQUIRE(state.is_initialized() == false);
    }

    SECTION("State with non-zero unified height is initialized")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 6537420;

        REQUIRE(state.is_initialized() == true);
    }

    SECTION("State with unified height 1 is initialized")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 1;

        REQUIRE(state.is_initialized() == true);
    }

    SECTION("Only unified height drives initialization, not other fields")
    {
        CanonicalChainState state;
        state.canonical_channel_height = 9999;
        state.canonical_difficulty_nbits = 0x1d00ffff;
        state.canonical_channel_target = 42;

        /* unified_height is still 0 — not initialized */
        REQUIRE(state.is_initialized() == false);
    }

    SECTION("Maximum uint32 height is considered initialized")
    {
        CanonicalChainState state;
        state.canonical_unified_height = UINT32_MAX;

        REQUIRE(state.is_initialized() == true);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 3. is_canonically_stale()                                              */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState is_canonically_stale", "[canonical_chain_state]")
{
    SECTION("Default-constructed state is stale (not initialized)")
    {
        CanonicalChainState state;
        REQUIRE(state.is_canonically_stale() == true);
    }

    SECTION("Recently-received state is not stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        state.canonical_received_at = std::chrono::steady_clock::now();

        REQUIRE(state.is_canonically_stale() == false);
    }

    SECTION("State received long ago is stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        state.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(60);

        REQUIRE(state.is_canonically_stale() == true);
    }

    SECTION("State at exactly the stale boundary is not stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        /* Set to exactly CANONICAL_STALE_SECONDS ago — should NOT be stale
         * because the comparison is strictly greater-than. */
        state.canonical_received_at =
            std::chrono::steady_clock::now()
            - std::chrono::seconds(CanonicalChainState::CANONICAL_STALE_SECONDS);

        REQUIRE(state.is_canonically_stale() == false);
    }

    SECTION("State one second past the boundary is stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        state.canonical_received_at =
            std::chrono::steady_clock::now()
            - std::chrono::seconds(CanonicalChainState::CANONICAL_STALE_SECONDS + 1);

        REQUIRE(state.is_canonically_stale() == true);
    }

    SECTION("Uninitialized state with recent timestamp is still stale")
    {
        CanonicalChainState state;
        state.canonical_received_at = std::chrono::steady_clock::now();
        /* unified_height == 0 → not initialized → stale */

        REQUIRE(state.is_canonically_stale() == true);
    }

    SECTION("Snapshot received 1 second ago is not stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 500;
        state.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(1);

        REQUIRE(state.is_canonically_stale() == false);
    }

    SECTION("Snapshot received 29 seconds ago is not stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 500;
        state.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(29);

        REQUIRE(state.is_canonically_stale() == false);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 4. height_drift_from_canonical()                                       */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState height_drift_from_canonical", "[canonical_chain_state]")
{
    /* In the unit-test environment, ChainState::tStateBest defaults to height 0. */

    SECTION("Drift is zero when canonical height matches live chain (both 0)")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 0;

        REQUIRE(state.height_drift_from_canonical() == 0);
    }

    SECTION("Drift is negative when canonical height exceeds live chain")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;

        /* Live chain is at 0 in tests → drift = 0 - 100 = -100 */
        REQUIRE(state.height_drift_from_canonical() == -100);
    }

    SECTION("Large canonical height produces large negative drift")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 6537420;

        /* Live chain is at 0 → drift = 0 - 6537420 = -6537420 */
        REQUIRE(state.height_drift_from_canonical() == -6537420);
    }

    SECTION("Drift of 1 height signals single block advance")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 1;

        /* drift = 0 - 1 = -1 */
        REQUIRE(state.height_drift_from_canonical() == -1);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 5. FIELD STORAGE AND RETRIEVAL                                         */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState Field Storage", "[canonical_chain_state]")
{
    SECTION("All fields are stored and retrievable")
    {
        CanonicalChainState state;

        state.canonical_unified_height   = 6537420;
        state.canonical_channel_height   = 2302664;
        state.canonical_difficulty_nbits = 0x1d00ffff;
        state.canonical_channel_target   = 42;
        state.canonical_hash_prev_block.SetHex("abcdef1234567890");
        state.canonical_received_at = std::chrono::steady_clock::now();

        REQUIRE(state.canonical_unified_height   == 6537420);
        REQUIRE(state.canonical_channel_height   == 2302664);
        REQUIRE(state.canonical_difficulty_nbits == 0x1d00ffff);
        REQUIRE(state.canonical_channel_target   == 42);
        REQUIRE(state.canonical_hash_prev_block  != uint1024_t(0));
        REQUIRE(state.is_initialized() == true);
        REQUIRE(state.is_canonically_stale() == false);
    }

    SECTION("Copy semantics work correctly")
    {
        CanonicalChainState a;
        a.canonical_unified_height = 500;
        a.canonical_channel_height = 200;
        a.canonical_received_at = std::chrono::steady_clock::now();

        /* Struct is trivially copyable for data fields */
        CanonicalChainState b = a;

        REQUIRE(b.canonical_unified_height == 500);
        REQUIRE(b.canonical_channel_height == 200);
        REQUIRE(b.is_initialized() == true);
    }

    SECTION("Assignment overwrites all fields")
    {
        CanonicalChainState a;
        a.canonical_unified_height = 100;
        a.canonical_channel_height = 50;
        a.canonical_difficulty_nbits = 0xABCD;
        a.canonical_received_at = std::chrono::steady_clock::now();

        CanonicalChainState b;
        b.canonical_unified_height = 999;
        b.canonical_channel_height = 444;
        b.canonical_difficulty_nbits = 0x1234;
        b.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(60);

        /* Overwrite b with a */
        b = a;

        REQUIRE(b.canonical_unified_height == 100);
        REQUIRE(b.canonical_channel_height == 50);
        REQUIRE(b.canonical_difficulty_nbits == 0xABCD);
        REQUIRE(b.is_canonically_stale() == false);
    }

    SECTION("Difficulty nBits round-trips common values")
    {
        CanonicalChainState state;

        /* Bitcoin-style initial difficulty */
        state.canonical_difficulty_nbits = 0x1d00ffff;
        REQUIRE(state.canonical_difficulty_nbits == 0x1d00ffff);

        /* Minimum difficulty */
        state.canonical_difficulty_nbits = 0x03000000;
        REQUIRE(state.canonical_difficulty_nbits == 0x03000000);

        /* Zero difficulty */
        state.canonical_difficulty_nbits = 0;
        REQUIRE(state.canonical_difficulty_nbits == 0);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 6. from_chain_state() FACTORY METHOD                                   */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState from_chain_state factory", "[canonical_chain_state][factory]")
{
    SECTION("Factory produces initialized snapshot")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 6537420;

        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 2302664;

        uint32_t nDifficulty = 0x1d00ffff;

        CanonicalChainState snap = CanonicalChainState::from_chain_state(
            stateBest, stateChannel, nDifficulty);

        REQUIRE(snap.is_initialized() == true);
        REQUIRE(snap.canonical_unified_height == 6537420);
        REQUIRE(snap.canonical_channel_height == 2302664);
        REQUIRE(snap.canonical_difficulty_nbits == 0x1d00ffff);
        REQUIRE(snap.is_canonically_stale() == false);
    }

    SECTION("Factory sets received_at to approximately now")
    {
        auto before = std::chrono::steady_clock::now();

        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 100;

        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 50;

        CanonicalChainState snap = CanonicalChainState::from_chain_state(
            stateBest, stateChannel, 0);

        auto after = std::chrono::steady_clock::now();

        /* received_at should be between before and after */
        REQUIRE(snap.canonical_received_at >= before);
        REQUIRE(snap.canonical_received_at <= after);
    }

    SECTION("Factory with zero-height BlockState produces uninitialized snapshot")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 0;

        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 0;

        CanonicalChainState snap = CanonicalChainState::from_chain_state(
            stateBest, stateChannel, 0);

        REQUIRE(snap.is_initialized() == false);
        REQUIRE(snap.is_canonically_stale() == true);
    }

    SECTION("Factory preserves difficulty value exactly")
    {
        TAO::Ledger::BlockState stateBest;
        stateBest.nHeight = 1;

        TAO::Ledger::BlockState stateChannel;
        stateChannel.nChannelHeight = 1;

        /* Test various difficulty values */
        for(uint32_t nBits : {0u, 1u, 0x1d00ffffu, 0xFFFFFFFFu})
        {
            CanonicalChainState snap = CanonicalChainState::from_chain_state(
                stateBest, stateChannel, nBits);
            REQUIRE(snap.canonical_difficulty_nbits == nBits);
        }
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 7. METHOD INTERACTION SCENARIOS                                        */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState Method Interactions", "[canonical_chain_state][interaction]")
{
    SECTION("Fresh snapshot: initialized, not stale, drift computable")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        state.canonical_received_at = std::chrono::steady_clock::now();

        REQUIRE(state.is_initialized() == true);
        REQUIRE(state.is_canonically_stale() == false);

        /* In test env, live height is 0 → drift = 0 - 100 = -100 */
        REQUIRE(state.height_drift_from_canonical() == -100);
    }

    SECTION("Stale but initialized snapshot still computes drift")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 50;
        state.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(60);

        REQUIRE(state.is_initialized() == true);
        REQUIRE(state.is_canonically_stale() == true);

        /* Drift is still computable even when stale */
        REQUIRE(state.height_drift_from_canonical() == -50);
    }

    SECTION("Default snapshot: all three methods agree on empty state")
    {
        CanonicalChainState state;

        REQUIRE(state.is_initialized() == false);
        REQUIRE(state.is_canonically_stale() == true);
        REQUIRE(state.height_drift_from_canonical() == 0);
    }

    SECTION("Overwrite stale snapshot with fresh data transitions correctly")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        state.canonical_received_at =
            std::chrono::steady_clock::now() - std::chrono::seconds(60);

        REQUIRE(state.is_canonically_stale() == true);

        /* Refresh the snapshot */
        state.canonical_unified_height = 200;
        state.canonical_received_at = std::chrono::steady_clock::now();

        REQUIRE(state.is_initialized() == true);
        REQUIRE(state.is_canonically_stale() == false);
        REQUIRE(state.height_drift_from_canonical() == -200);
    }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* 8. EDGE CASES                                                          */
/* ═══════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalChainState Edge Cases", "[canonical_chain_state][edge]")
{
    SECTION("uint32_t max values are stored correctly")
    {
        CanonicalChainState state;
        state.canonical_unified_height   = UINT32_MAX;
        state.canonical_channel_height   = UINT32_MAX;
        state.canonical_difficulty_nbits = UINT32_MAX;
        state.canonical_channel_target   = UINT32_MAX;

        REQUIRE(state.canonical_unified_height   == UINT32_MAX);
        REQUIRE(state.canonical_channel_height   == UINT32_MAX);
        REQUIRE(state.canonical_difficulty_nbits == UINT32_MAX);
        REQUIRE(state.canonical_channel_target   == UINT32_MAX);
        REQUIRE(state.is_initialized() == true);
    }

    SECTION("Hash prev block can store full 1024-bit value")
    {
        CanonicalChainState state;
        state.canonical_hash_prev_block.SetHex(
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        REQUIRE(state.canonical_hash_prev_block != uint1024_t(0));
    }

    SECTION("Multiple copies are independent")
    {
        CanonicalChainState a;
        a.canonical_unified_height = 100;
        a.canonical_received_at = std::chrono::steady_clock::now();

        CanonicalChainState b = a;
        b.canonical_unified_height = 200;

        /* Modifying b should not affect a */
        REQUIRE(a.canonical_unified_height == 100);
        REQUIRE(b.canonical_unified_height == 200);
    }

    SECTION("Snapshot with epoch time_point and initialized height is stale")
    {
        CanonicalChainState state;
        state.canonical_unified_height = 100;
        /* canonical_received_at is still epoch → very old → stale */

        REQUIRE(state.is_initialized() == true);
        REQUIRE(state.is_canonically_stale() == true);
    }
}
