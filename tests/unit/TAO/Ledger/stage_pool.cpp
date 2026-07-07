/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/* [Option A] Tests for the block-context transaction staging pool that decouples
 * block completion from tip-relative mempool admission policy (the "coinbase is
 * immature" deadlock fix). */

#include <TAO/Ledger/include/stagepool.h>
#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

namespace
{
    /* Build a minimal distinct transaction (distinct hash via timestamp). */
    TAO::Ledger::Transaction MakeTx(const uint64_t nTimestamp)
    {
        TAO::Ledger::Transaction tx;
        tx.nTimestamp = nTimestamp;
        tx.nSequence  = 1;

        return tx;
    }
}


TEST_CASE("StagePool only stages transactions wanted by an incomplete block", "[stagepool]")
{
    TAO::Ledger::StagePool::Clear();

    const TAO::Ledger::Transaction tx = MakeTx(700000001);
    const uint512_t hashTx = tx.GetHash();

    SECTION("unwanted transactions are refused")
    {
        REQUIRE_FALSE(TAO::Ledger::StagePool::Wanted(hashTx));
        REQUIRE_FALSE(TAO::Ledger::StagePool::Stage(tx));
        REQUIRE(TAO::Ledger::StagePool::Count() == 0);

        TAO::Ledger::Transaction txOut;
        REQUIRE_FALSE(TAO::Ledger::StagePool::Get(hashTx, txOut));
    }

    SECTION("registered transactions are staged and retrievable")
    {
        TAO::Ledger::StagePool::Register(hashTx);
        REQUIRE(TAO::Ledger::StagePool::Wanted(hashTx));

        REQUIRE(TAO::Ledger::StagePool::Stage(tx));
        REQUIRE(TAO::Ledger::StagePool::Count() == 1);

        TAO::Ledger::Transaction txOut;
        REQUIRE(TAO::Ledger::StagePool::Get(hashTx, txOut));
        REQUIRE(txOut.GetHash() == hashTx);
    }

    TAO::Ledger::StagePool::Clear();
}


TEST_CASE("StagePool erase releases both wanted and staged state", "[stagepool]")
{
    TAO::Ledger::StagePool::Clear();

    const TAO::Ledger::Transaction tx = MakeTx(700000002);
    const uint512_t hashTx = tx.GetHash();

    TAO::Ledger::StagePool::Register(hashTx);
    REQUIRE(TAO::Ledger::StagePool::Stage(tx));
    REQUIRE(TAO::Ledger::StagePool::Count() == 1);

    /* Simulates the release after a block commit writes the tx to disk. */
    TAO::Ledger::StagePool::Erase(hashTx);

    REQUIRE(TAO::Ledger::StagePool::Count() == 0);
    REQUIRE_FALSE(TAO::Ledger::StagePool::Wanted(hashTx));

    TAO::Ledger::Transaction txOut;
    REQUIRE_FALSE(TAO::Ledger::StagePool::Get(hashTx, txOut));

    /* Once erased, re-staging requires re-registration (a new incomplete-block
     * sighting), so stale wanted-state can't accumulate. */
    REQUIRE_FALSE(TAO::Ledger::StagePool::Stage(tx));

    TAO::Ledger::StagePool::Clear();
}


TEST_CASE("StagePool wanted map is bounded by clear-all DoS guard", "[stagepool]")
{
    TAO::Ledger::StagePool::Clear();

    /* Fill the wanted map to its cap. */
    for(uint64_t i = 1; i <= TAO::Ledger::StagePool::MAX_WANTED_ENTRIES; ++i)
        TAO::Ledger::StagePool::Register(uint512_t(i));

    REQUIRE(TAO::Ledger::StagePool::Wanted(uint512_t(1)));

    /* One more unique entry trips the clear-all guard; only the new entry
     * survives. */
    const uint512_t hashOverflow = uint512_t(TAO::Ledger::StagePool::MAX_WANTED_ENTRIES + 1);
    TAO::Ledger::StagePool::Register(hashOverflow);

    REQUIRE(TAO::Ledger::StagePool::Wanted(hashOverflow));
    REQUIRE_FALSE(TAO::Ledger::StagePool::Wanted(uint512_t(1)));

    TAO::Ledger::StagePool::Clear();
}
