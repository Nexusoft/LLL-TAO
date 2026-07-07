/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/* [Option C] Tests for the time-decaying missing-transaction retry counter.
 * Previously, once a block exhausted MAX_MISSING_TRANSACTIONS_RETRIES its
 * counter was only erased on ACCEPT, so recovery was permanently disabled
 * (silent dead-end) until a node restart.  The decay makes exhaustion a
 * temporary cool-down instead. */

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/process.h>

#include <Util/include/runtime.h>

#include <unit/catch2/catch.hpp>


TEST_CASE("UpdateMissingRetry counts attempts and stamps them", "[process][missing-retry]")
{
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();

    const uint1024_t hashBlock = uint1024_t(0xABCDE);

    REQUIRE(TAO::Ledger::UpdateMissingRetry(hashBlock) == 1);
    REQUIRE(TAO::Ledger::UpdateMissingRetry(hashBlock) == 2);
    REQUIRE(TAO::Ledger::UpdateMissingRetry(hashBlock) == 3);

    REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 3);
    REQUIRE(TAO::Ledger::mapLastMissingStamp.count(hashBlock) == 1);

    /* Not exhausted while under the cap. */
    REQUIRE_FALSE(TAO::Ledger::MissingRetryExhausted(hashBlock));

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();
}


TEST_CASE("Exhausted retries cool down and then decay back to recovery", "[process][missing-retry]")
{
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();

    const uint1024_t hashBlock = uint1024_t(0xBEEF);
    const uint64_t nCap = LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES;

    SECTION("exhaustion is active inside the cool-down window")
    {
        /* Simulate a block that just blew past the retry cap. */
        TAO::Ledger::mapLastMissing[hashBlock]      = nCap + 1;
        TAO::Ledger::mapLastMissingStamp[hashBlock] = runtime::timestamp();

        REQUIRE(TAO::Ledger::MissingRetryExhausted(hashBlock));
    }

    SECTION("exhaustion expires once the decay window has passed")
    {
        /* Backdate the last attempt beyond the decay window. */
        TAO::Ledger::mapLastMissing[hashBlock]      = nCap + 1;
        TAO::Ledger::mapLastMissingStamp[hashBlock] =
            runtime::timestamp() - TAO::Ledger::MISSING_RETRY_DECAY_SECONDS - 1;

        REQUIRE_FALSE(TAO::Ledger::MissingRetryExhausted(hashBlock));

        /* And the next attempt decay-resets the counter to 1, so re-requesting
         * resumes instead of staying dead-ended forever. */
        REQUIRE(TAO::Ledger::UpdateMissingRetry(hashBlock) == 1);
    }

    SECTION("a missing stamp never blocks recovery")
    {
        TAO::Ledger::mapLastMissing[hashBlock] = nCap + 1;
        TAO::Ledger::mapLastMissingStamp.erase(hashBlock);

        REQUIRE_FALSE(TAO::Ledger::MissingRetryExhausted(hashBlock));
    }

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();
}


TEST_CASE("EraseMissingRetry clears both counter and stamp", "[process][missing-retry]")
{
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();

    const uint1024_t hashBlock = uint1024_t(0xF00D);

    TAO::Ledger::UpdateMissingRetry(hashBlock);
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 1);
    REQUIRE(TAO::Ledger::mapLastMissingStamp.count(hashBlock) == 1);

    TAO::Ledger::EraseMissingRetry(hashBlock);
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
    REQUIRE(TAO::Ledger::mapLastMissingStamp.count(hashBlock) == 0);

    /* Unknown hashes are never considered exhausted. */
    REQUIRE_FALSE(TAO::Ledger::MissingRetryExhausted(hashBlock));
}


TEST_CASE("Bounding guard clears both retry maps together", "[process][missing-retry]")
{
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();

    /* Fill to the cap. */
    for(uint64_t i = 1; i <= TAO::Ledger::MAX_MISSING_MAP_ENTRIES; ++i)
    {
        TAO::Ledger::mapLastMissing[uint1024_t(i)]      = 1;
        TAO::Ledger::mapLastMissingStamp[uint1024_t(i)] = runtime::timestamp();
    }

    /* A new unique hash trips the clear-all guard; only it survives. */
    const uint1024_t hashNew = uint1024_t(TAO::Ledger::MAX_MISSING_MAP_ENTRIES + 1);
    REQUIRE(TAO::Ledger::UpdateMissingRetry(hashNew) == 1);

    REQUIRE(TAO::Ledger::mapLastMissing.size() == 1);
    REQUIRE(TAO::Ledger::mapLastMissingStamp.size() == 1);
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashNew) == 1);

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingStamp.clear();
}
