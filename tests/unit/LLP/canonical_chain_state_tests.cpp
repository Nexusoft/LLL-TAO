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
 *  These tests verify the canonical chain state snapshot struct
 *  used by the Stateless Block Utility and Colin Mining Agent.
 *
 *  Key functionality tested:
 *  1. Default construction (all fields zero / epoch)
 *  2. is_initialized() — false when default, true after population
 *  3. is_canonically_stale() — returns true for default / old snapshots
 *  4. height_drift_from_canonical() — signed drift computation
 *  5. Field storage and retrieval
 *
 *  Note: height_drift_from_canonical() reads ChainState::tStateBest,
 *  which defaults to height 0 in the unit-test environment.
 **/


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
}


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
}


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
}


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
}


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
}
