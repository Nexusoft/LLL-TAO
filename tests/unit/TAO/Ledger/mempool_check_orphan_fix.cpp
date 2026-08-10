/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Regression tests for two bugs in Mempool::Check()'s orphan-eviction loop:
 *
 * Bug #1 — Force-evict on Disconnect failure
 *   When Disconnect()/Rollback() fails for an orphan transaction (e.g. because
 *   a referenced DEBIT only exists in memory, causing ReadContract to throw),
 *   the transaction was never removed from mapLedger.  Every subsequent
 *   SetBest → mempool.Check() call re-triggered the same failed
 *   disconnect/rollback, burning CPU indefinitely.
 *
 *   Fix: after TxnAbort(), force-call Remove(hashTx) so the stuck orphan is
 *   evicted regardless of the Rollback outcome.  Also guard TxnCommit so it
 *   is never called on an already-aborted transaction.
 *
 * Bug #2 — ReadLast failure for genesis A must not prevent genesis B
 *   A ReadLast failure for one genesis used break to exit the entire outer
 *   genesis loop, silently skipping all later genesis hashes in the same
 *   Check() sweep.
 *
 *   Fix: change break → continue so only the failing genesis is skipped.
 *
 * Both tests use Mempool::AddUnchecked() to inject specific transaction state
 * directly into mapLedger without going through Accept()'s signature/contract
 * validation, mirroring the approach used by other isolated ledger unit tests
 * (e.g. filter_mempool_only_predecessor.cpp).
 *
 * Related: src/TAO/Ledger/mempool.cpp Mempool::Check() orphan-eviction loop.
 */

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/enum.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>

#include <unit/catch2/catch.hpp>


namespace
{
    /*  Lightweight guard that creates a temporary LedgerDB when the global
     *  test suite has not initialised one (e.g. when running only
     *  [mempool_orphan] tagged tests in isolation).  On destruction it
     *  deletes only what it created, leaving global state untouched if it
     *  was already set up. */
    struct LedgerGuard
    {
        bool ownedLedger{false};

        LedgerGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Ledger)
            {
                LLD::Ledger = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLedger = true;
            }
        }

        ~LedgerGuard()
        {
            if(ownedLedger)
            {
                delete LLD::Ledger;
                LLD::Ledger = nullptr;
            }
        }
    };


    /* Build a minimal Transaction for testing.  The transaction is NOT signed
     * and NOT validated — it is injected via AddUnchecked() to test specific
     * internal mempool state transitions rather than the accept path.
     *
     * hashGenesis must be a non-zero uint256_t.  nSeq 0 → IsFirst()==true,
     * nSeq > 0 → IsFirst()==false and hashPrevTx should be set by the caller. */
    TAO::Ledger::Transaction MakeTx(const uint256_t& hashGenesis,
                                    const uint32_t   nSeq,
                                    const uint512_t  hashPrevTx = 0)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }


    /* Return a non-zero uint512_t that is guaranteed different from any of
     * the provided values (re-samples until distinct). */
    uint512_t UniqueHash512(std::initializer_list<uint512_t> avoid)
    {
        for(;;)
        {
            const uint512_t h = LLC::GetRand512();
            if(h == 0)
                continue;
            bool ok = true;
            for(const auto& a : avoid)
                if(h == a) { ok = false; break; }
            if(ok)
                return h;
        }
    }
}


/*
 * Test 1: Force-evict after failed Disconnect/Rollback
 *
 * Scenario:
 *   • A single transaction T is in mapLedger for genesis G.
 *   • T is NOT a genesis transaction (nSequence = 1).
 *   • T has one empty contract in vContracts.  An empty contract has no
 *     operation bytes; TAO::Operation::Contract::Primitive() throws when the
 *     stream is empty, which TAO::Register::Rollback() catches and converts
 *     to return false.  This causes Disconnect() to return false — exactly
 *     the same failure mode as a CREDIT whose referenced DEBIT is only in
 *     memory and not on disk.
 *   • Note: print()/ToString()/IsGenesis() can also throw on this fixture, but
 *     production keeps that diagnostic outside the Disconnect try/catch so the
 *     failure path exercised here is Disconnect() returning false.
 *   • ReadLast for G returns a hash that differs from T.hashPrevTx so that
 *     the orphan check fires (T.hashPrevTx != hashLastDisk).
 *
 * Expected behaviour after Mempool::Check():
 *   • T is removed from the mempool (force-evicted via Remove() in the
 *     Disconnect-failure path, not left permanently stuck).
 *
 * Without the fix (old code):
 *   • TxnAbort was called but Remove() was never reached because break only
 *     exited the inner reverse loop; T survived every Check() sweep forever.
 */
TEST_CASE("Mempool::Check force-evicts unrollbackable orphan when Disconnect fails",
    "[mempool_orphan][ledger]")
{
    LedgerGuard env;

    /* Pick a unique genesis hash for this test to avoid interference. */
    const uint256_t hashGenesis = LLC::GetRand256();

    /* Construct a disk hash that is different from the tx's hashPrevTx so
     * the orphan check fires. */
    const uint512_t hashPrevTxOrphan = LLC::GetRand512();
    const uint512_t hashDiskLast     = UniqueHash512({hashPrevTxOrphan});

    /* Write the disk-last hash for this genesis so ReadLast() succeeds. */
    REQUIRE(LLD::Ledger->WriteLast(hashGenesis, hashDiskLast));

    /* Build the orphan transaction: nSequence=1 so IsFirst()==false and the
     * orphan detection triggers; hashPrevTx differs from hashDiskLast. */
    TAO::Ledger::Transaction txOrphan = MakeTx(hashGenesis, 1, hashPrevTxOrphan);

    /* Append one empty contract to ensure Disconnect()/Rollback() fails.
     * Transaction::operator[](0) creates a new default-constructed Contract at
     * index 0.  The contract is bound but has an empty operation stream, so:
     *   • Sanitize() → Execute() → contract >> nOP throws on the empty stream
     *     → fContractInvalid = true  (orphan detection triggers)
     *   • Disconnect() → Rollback() → Contract::Primitive() throws on the
     *     empty stream → Rollback returns false → Disconnect returns false
     * Both paths together confirm that: (1) the orphan is detected, and (2)
     * the fix correctly removes it even though Disconnect cannot complete. */
    txOrphan[0]; // creates one empty, failing contract

    const uint512_t hashOrphan = txOrphan.GetHash();

    /* Inject directly into mapLedger, bypassing signature/contract checks. */
    REQUIRE(TAO::Ledger::mempool.AddUnchecked(txOrphan));
    REQUIRE(TAO::Ledger::mempool.Has(hashOrphan));

    /* Run the consistency sweep — this is the code under test.
     * Regression target: Disconnect() returns false for the unrollbackable
     * orphan (Rollback catches the empty-stream exception); Check() must
     * still force-evict and not leave the tx stuck in mapLedger. */
    CHECK_NOTHROW(TAO::Ledger::mempool.Check());

    /* The fix: the transaction must be removed even though Disconnect failed.
     * Without the fix it would remain in mapLedger and re-trigger the same
     * failed disconnect on every subsequent SetBest event. */
    INFO("Orphan tx hash: " << hashOrphan.SubString());
    CHECK_FALSE(TAO::Ledger::mempool.Has(hashOrphan));

    /* Clean-up (guard in case the tx survived due to a regression). */
    TAO::Ledger::mempool.Remove(hashOrphan);
    LLD::Ledger->EraseLast(hashGenesis);
}


/*
 * Test 2: ReadLast failure for genesis A must not prevent genesis B
 *
 * Scenario:
 *   • The mempool contains two transactions for two different geneses:
 *       – T_A for genesis A (no ReadLast on disk → ReadLast fails for A)
 *       – T_B for genesis B (ReadLast exists but returns a hash that differs
 *         from T_B.hashPrevTx, so T_B is an orphan)
 *   • Genesis A sorts before genesis B in the ordered map (hashGenesis_A < B).
 *
 * Expected behaviour after Mempool::Check():
 *   • T_A is unchanged (its genesis had no disk state; the loop skips it).
 *   • T_B is removed (its orphan check fires and Disconnect succeeds because
 *     T_B has no contracts).
 *
 * Without the fix (old code using break):
 *   • The ReadLast failure for genesis A would break the entire outer genesis
 *     loop, so genesis B's transactions were never processed and T_B remained
 *     in the mempool.
 *
 * The test asserts the post-condition that T_B was processed (removed) even
 * though A's ReadLast failed first.  This implicitly verifies continue
 * behaviour: if the code still used break, T_B would survive and the CHECK
 * below would fail.
 */
TEST_CASE("Mempool::Check continues to next genesis after ReadLast failure",
    "[mempool_orphan][ledger]")
{
    LedgerGuard env;

    /* Choose two distinct genesis hashes and order them so A < B.
     * Repeat until the ordering condition is satisfied. */
    uint256_t hashGenesis_A, hashGenesis_B;
    do
    {
        hashGenesis_A = LLC::GetRand256();
        hashGenesis_B = LLC::GetRand256();
    }
    while(hashGenesis_A == 0 || hashGenesis_B == 0 || hashGenesis_A >= hashGenesis_B);

    /* T_A: genesis tx (nSequence=0, IsFirst=true).  No ReadLast written to
     * disk → ReadLast will fail for genesis A. */
    const TAO::Ledger::Transaction txA = MakeTx(hashGenesis_A, 0);
    const uint512_t hashTxA = txA.GetHash();

    REQUIRE(TAO::Ledger::mempool.AddUnchecked(txA));
    REQUIRE(TAO::Ledger::mempool.Has(hashTxA));

    /* T_B: non-genesis tx (nSequence=1, IsFirst=false).  ReadLast returns a
     * disk hash that differs from T_B.hashPrevTx so the orphan check fires.
     * No contracts → Disconnect() succeeds, Remove() is called normally. */
    const uint512_t hashPrevTxB = LLC::GetRand512();
    const uint512_t hashDiskB   = UniqueHash512({hashPrevTxB});

    REQUIRE(LLD::Ledger->WriteLast(hashGenesis_B, hashDiskB));

    const TAO::Ledger::Transaction txB = MakeTx(hashGenesis_B, 1, hashPrevTxB);
    const uint512_t hashTxB = txB.GetHash();

    REQUIRE(TAO::Ledger::mempool.AddUnchecked(txB));
    REQUIRE(TAO::Ledger::mempool.Has(hashTxB));

    /* Run the consistency sweep. */
    CHECK_NOTHROW(TAO::Ledger::mempool.Check());

    /* T_B should be removed: its orphan was detected because Check() continued
     * past genesis A's ReadLast failure (continue, not break).
     * Without the fix, T_B would still be in the mempool here. */
    INFO("hashGenesis_A: " << hashGenesis_A.SubString() << "  hashGenesis_B: " << hashGenesis_B.SubString());
    CHECK_FALSE(TAO::Ledger::mempool.Has(hashTxB));

    /* T_A should still be in the mempool (it was skipped, not removed). */
    CHECK(TAO::Ledger::mempool.Has(hashTxA));

    /* Clean-up. */
    TAO::Ledger::mempool.Remove(hashTxA);
    TAO::Ledger::mempool.Remove(hashTxB);
    LLD::Ledger->EraseLast(hashGenesis_A);
    LLD::Ledger->EraseLast(hashGenesis_B);
}
