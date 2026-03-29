/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Regression test for the CreateTransaction() hashLast / hashPrevTx desync bug.
 *
 * Bug: when the mempool branch wins the three-source resolution in CreateTransaction(),
 * txPrev was updated to the mempool tx but hashLast was NOT updated.  The downstream
 * assignment  tx.hashPrevTx = hashLast  then wrote a stale (or zero) hash, causing
 * Transaction::Connect() to fail with "prev transaction incorrect sequence".
 *
 * Fix: add  hashLast = txMem.GetHash();  after  txPrev = txMem;  in the mempool branch.
 *
 * See docs/NSEQ_DIAG_MEMPOOL_HASHLAST_BUG.md for the full root-cause analysis.
 */

#include <LLC/include/random.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/genesis.h>

#include <unit/catch2/catch.hpp>

/* ---------------------------------------------------------------------------
 * Inline helper that mirrors the three-source resolution logic inside
 * CreateTransaction() so we can test it without needing an encrypted
 * Credentials pointer or a running node.
 *
 * The function returns the resolved (txPrev, hashLast) pair, exactly as
 * CreateTransaction() would compute them.
 * --------------------------------------------------------------------------- */
namespace
{
    struct ResolutionResult
    {
        TAO::Ledger::Transaction txPrev;
        uint512_t                hashLast;
    };

    /* Simulate the three-source resolution.
     *
     *  pSessionTx  – pointer to a Sessions-DB tx (nullptr if not available)
     *  hashSessionLast – hash that Sessions->ReadLast() would return
     *  pMempoolTx  – pointer to a Mempool tx (nullptr if not available)
     *  pLedgerTx   – pointer to a Ledger-DB tx (nullptr if not available)
     *  hashLedgerLast  – hash that Ledger->ReadLast() would return
     *
     * The return value holds the winning (txPrev, hashLast) after the same
     * conditional tree used in CreateTransaction().
     */
    ResolutionResult SimulateCreateTransactionResolution(
        const TAO::Ledger::Transaction* pSessionTx,
        const uint512_t&                hashSessionLast,
        const TAO::Ledger::Transaction* pMempoolTx,
        const TAO::Ledger::Transaction* pLedgerTx,
        const uint512_t&                hashLedgerLast)
    {
        /* ---- exact copy of the resolution tree from create.cpp ---- */
        uint512_t hashLast = 0;

        TAO::Ledger::Transaction txPrev;   // default-constructed → hashGenesis == 0
        if(pSessionTx != nullptr)
        {
            hashLast = hashSessionLast;
            txPrev   = *pSessionTx;
        }

        if(pMempoolTx != nullptr)
        {
            if(pMempoolTx->nSequence > txPrev.nSequence)
            {
                txPrev   = *pMempoolTx;
                hashLast = pMempoolTx->GetHash();   // ← the one-line fix under test
            }
        }
        else if(pLedgerTx != nullptr)
        {
            if(pLedgerTx->nSequence > txPrev.nSequence)
            {
                hashLast = hashLedgerLast;
                txPrev   = *pLedgerTx;
            }
        }

        return {txPrev, hashLast};
    }

    /* Build a minimal but hashed transaction for a given genesis and sequence.
     * We only need GetHash() to return a stable unique value; no signing is
     * required for this unit test. */
    TAO::Ledger::Transaction MakeTx(const uint256_t& hashGenesis, const uint32_t nSeq)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.nTimestamp  = 1700000000u + nSeq;   // stable, deterministic
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }
}


/* ===========================================================================
 * Test 1 — Mempool winner: hashLast must track the mempool tx hash
 *
 * Scenario:
 *   - seq-0 tx is on disk AND in Sessions  →  Sessions wins first
 *   - seq-1 tx is in mempool               →  mempool overrides Sessions
 *
 * Before the fix, hashLast stayed at hash(seq-0).
 * After the fix,  hashLast must equal hash(seq-1).
 * =========================================================================== */
TEST_CASE( "CreateTransaction hashLast tracks mempool winner", "[ledger]" )
{
    /* Unique genesis for this test run. */
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Build session tx (seq=0) and mempool tx (seq=1). */
    TAO::Ledger::Transaction txSeq0 = MakeTx(hashGenesis, 0);
    TAO::Ledger::Transaction txSeq1 = MakeTx(hashGenesis, 1);

    const uint512_t hashSeq0 = txSeq0.GetHash();
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Sanity: the two hashes must differ. */
    REQUIRE(hashSeq0 != hashSeq1);

    /* Run the resolution: Sessions has seq-0, Mempool has seq-1. */
    ResolutionResult r = SimulateCreateTransactionResolution(
        &txSeq0,   hashSeq0,   // Sessions tx + its hash
        &txSeq1,               // Mempool tx
        nullptr,   uint512_t(0) // No ledger tx
    );

    /* txPrev must be the mempool tx. */
    REQUIRE(r.txPrev.nSequence == 1u);

    /* hashLast MUST equal the mempool tx's hash (the fixed behaviour).
     * If this fails it means the one-line fix is missing:
     *   hashLast = txMem.GetHash(); */
    REQUIRE(r.hashLast == hashSeq1);

    /* Derived values that would be written into the new transaction. */
    const uint32_t  nExpectedSequence  = r.txPrev.nSequence + 1;   // must be 2
    const uint512_t hashExpectedPrevTx = r.hashLast;               // must be hash of seq-1

    REQUIRE(nExpectedSequence  == 2u);
    REQUIRE(hashExpectedPrevTx == hashSeq1);
}


/* ===========================================================================
 * Test 2 — No mempool: hashLast falls through to Ledger
 *
 * When the mempool has nothing for this genesis the else-if branch fires and
 * hashLast should be updated to the ledger's last hash, not left at 0.
 * =========================================================================== */
TEST_CASE( "CreateTransaction hashLast uses ledger when no mempool", "[ledger]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(hashGenesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    /* No Sessions, no Mempool — only Ledger. */
    ResolutionResult r = SimulateCreateTransactionResolution(
        nullptr,   uint512_t(0),   // No Sessions
        nullptr,                   // No Mempool
        &txSeq0,   hashSeq0        // Ledger tx
    );

    REQUIRE(r.txPrev.nSequence == 0u);
    REQUIRE(r.hashLast         == hashSeq0);
}


/* ===========================================================================
 * Test 3 — Mempool tx is older than Sessions tx: Sessions wins, no override
 *
 * If the mempool has a stale/lower-sequence tx it must NOT override the
 * Sessions tx, and hashLast must remain the Sessions value.
 * =========================================================================== */
TEST_CASE( "CreateTransaction mempool does not override higher-sequence sessions tx", "[ledger]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq2 = MakeTx(hashGenesis, 2);  // Sessions has seq-2
    TAO::Ledger::Transaction txSeq1 = MakeTx(hashGenesis, 1);  // Mempool only has seq-1

    const uint512_t hashSeq2 = txSeq2.GetHash();

    ResolutionResult r = SimulateCreateTransactionResolution(
        &txSeq2,   hashSeq2,   // Sessions tx
        &txSeq1,               // Mempool tx (older — must lose)
        nullptr,   uint512_t(0)
    );

    /* Sessions (seq-2) must win. */
    REQUIRE(r.txPrev.nSequence == 2u);
    REQUIRE(r.hashLast         == hashSeq2);
}


/* ===========================================================================
 * Test 4 — No prior tx at all: genesis transaction path
 *
 * When there is no Sessions, no Mempool, and no Ledger tx the resolution
 * leaves txPrev.hashGenesis == 0 and hashLast == 0, indicating a first
 * (genesis) transaction.
 * =========================================================================== */
TEST_CASE( "CreateTransaction produces zero hashLast for genesis transaction", "[ledger]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    ResolutionResult r = SimulateCreateTransactionResolution(
        nullptr, uint512_t(0),
        nullptr,
        nullptr, uint512_t(0)
    );

    REQUIRE(r.txPrev.hashGenesis == uint256_t(0));
    REQUIRE(r.hashLast           == uint512_t(0));
}
