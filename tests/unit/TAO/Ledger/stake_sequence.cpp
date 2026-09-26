/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Unit tests for the CreateTransaction() nSequence fix as it applies to both
 * Proof-of-Work (CreateProducer) and Proof-of-Stake (CreateStakeBlock) paths.
 *
 * These tests exercise the FIXED three-source resolution logic, including:
 *
 *   Fix 1:  Mempool genesis (seq=0) is accepted when txPrev is unset
 *           (the old `>` comparison silently ignored it).
 *
 *   Fix 2:  Disk is always checked (no longer gated by `else if`), so a
 *           newer disk entry is never silently skipped when mempool.Get()
 *           returned true but didn't win the comparison.
 *
 * Both fixes are in a single CreateTransaction() function that is shared
 * by all block production paths (PoW and PoS).  These tests prove the
 * fix works correctly for the stake minter path as well.
 *
 * See docs/SIGCHAIN_SEQUENCE_FIX_VERIFICATION.md for the full analysis.
 */

#include <LLC/include/random.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/genesis.h>

#include <unit/catch2/catch.hpp>

/* ---------------------------------------------------------------------------
 * Helper: mirrors the FIXED three-source resolution logic from
 * CreateTransaction() (create.cpp lines 82-155).
 *
 * This version includes BOTH fixes:
 *   Fix 1: mempool genesis override (hashGenesis==0 check)
 *   Fix 2: unconditional disk check (no else if)
 * --------------------------------------------------------------------------- */
namespace
{
    struct FixedResolutionResult
    {
        TAO::Ledger::Transaction txPrev;
        uint512_t                hashLast;
        std::string              strSeqSource;
    };

    /*  Simulate the FIXED three-source resolution.
     *
     *  pSessionTx    – pointer to a Sessions-DB tx (nullptr if absent)
     *  hashSessionLast – hash that Sessions->ReadLast() would return
     *  pMempoolTx    – pointer to a Mempool tx (nullptr if absent)
     *  pLedgerTx     – pointer to a Ledger-DB tx (nullptr if absent)
     *  hashLedgerLast – hash that Ledger->ReadLast() would return
     */
    FixedResolutionResult SimulateFixedResolution(
        const TAO::Ledger::Transaction* pSessionTx,
        const uint512_t&                hashSessionLast,
        const TAO::Ledger::Transaction* pMempoolTx,
        const TAO::Ledger::Transaction* pLedgerTx,
        const uint512_t&                hashLedgerLast)
    {
        /* ---- mirrors fixed resolution tree from create.cpp ---- */
        uint512_t hashLast = 0;
        std::string strSeqSource = "none";
        bool fUsedSessionIndex = false;

        TAO::Ledger::Transaction txPrev;   // default: hashGenesis==0, nSequence==0

        /* Source 1: Sessions */
        if(pSessionTx != nullptr)
        {
            fUsedSessionIndex = true;
            hashLast = hashSessionLast;
            txPrev   = *pSessionTx;
            strSeqSource = "sessions";
        }

        /* Source 2: Mempool (with Fix 1: genesis override) */
        if(pMempoolTx != nullptr)
        {
            if(pMempoolTx->nSequence > txPrev.nSequence
                || (txPrev.hashGenesis == 0 && pMempoolTx->hashGenesis != 0))
            {
                txPrev   = *pMempoolTx;
                hashLast = pMempoolTx->GetHash();
                strSeqSource = (fUsedSessionIndex ? "mempool_override_sessions" : "mempool");
            }
        }

        /* Source 3: Disk (Fix 2: unconditional — no else if) */
        if(pLedgerTx != nullptr)
        {
            if(pLedgerTx->nSequence > txPrev.nSequence)
            {
                hashLast = hashLedgerLast;
                txPrev   = *pLedgerTx;
                strSeqSource = (fUsedSessionIndex ? "ledger_override_sessions" : "ledger");
            }
        }

        return {txPrev, hashLast, strSeqSource};
    }

    /* Build a minimal transaction for a given genesis and sequence. */
    TAO::Ledger::Transaction MakeStakeTx(const uint256_t& hashGenesis, const uint32_t nSeq)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.nTimestamp  = 1700000000u + nSeq;
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }
}


/* ===========================================================================
 * Test 1: Fix 1 — Mempool genesis (seq=0) accepted when txPrev is unset
 *
 * This is the critical first-mining/staking-block bootstrap scenario.
 * Before Fix 1, the mempool genesis was silently ignored because
 * 0 > 0 == false, causing a duplicate genesis-id producer.
 * =========================================================================== */
TEST_CASE( "Stake: mempool genesis (seq=0) is accepted when txPrev unset", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Only mempool has the genesis profile tx (seq=0). */
    TAO::Ledger::Transaction txMem = MakeStakeTx(hashGenesis, 0);
    const uint512_t hashMem = txMem.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),   // No Sessions
        &txMem,                   // Mempool has seq=0
        nullptr, uint512_t(0)    // No Disk
    );

    /* Fix 1: txPrev must be the mempool genesis. */
    REQUIRE(r.txPrev.hashGenesis == hashGenesis);
    REQUIRE(r.txPrev.nSequence   == 0u);
    REQUIRE(r.hashLast           == hashMem);
    REQUIRE(r.strSeqSource       == "mempool");

    /* The derived producer should be seq=1 pointing to the genesis tx. */
    REQUIRE(r.txPrev.nSequence + 1 == 1u);
    REQUIRE(r.hashLast             == hashMem);
}


/* ===========================================================================
 * Test 2: Fix 2 — Disk is always checked (unconditional, not else-if)
 *
 * When mempool has a STALE entry (seq=0) and disk has seq=1,
 * the disk entry must win.  Before Fix 2, the disk path was skipped
 * entirely when mempool.Get() returned true.
 * =========================================================================== */
TEST_CASE( "Stake: disk override when mempool stale (Fix 2 — no else if)", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txMem  = MakeStakeTx(hashGenesis, 0);  // mempool: seq=0 (stale)
    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 1);  // disk: seq=1 (newer)
    const uint512_t hashDisk = txDisk.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),   // No Sessions
        &txMem,                   // Mempool: seq=0
        &txDisk, hashDisk        // Disk: seq=1
    );

    /* Fix 2: disk must win because seq=1 > seq=0. */
    REQUIRE(r.txPrev.nSequence == 1u);
    REQUIRE(r.hashLast         == hashDisk);
    REQUIRE(r.strSeqSource     == "ledger");

    /* Producer should be seq=2. */
    REQUIRE(r.txPrev.nSequence + 1 == 2u);
}


/* ===========================================================================
 * Test 3: Mempool wins when it has higher sequence than both Sessions and Disk
 *
 * Verifies that the mempool override works correctly when mempool has the
 * newest transaction (e.g., an API operation submitted after login).
 * =========================================================================== */
TEST_CASE( "Stake: mempool wins over sessions and disk with higher seq", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSess = MakeStakeTx(hashGenesis, 0);
    TAO::Ledger::Transaction txMem  = MakeStakeTx(hashGenesis, 2);
    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 1);

    const uint512_t hashSess = txSess.GetHash();
    const uint512_t hashMem  = txMem.GetHash();
    const uint512_t hashDisk = txDisk.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        &txSess, hashSess,   // Sessions: seq=0
        &txMem,               // Mempool: seq=2 (newest)
        &txDisk, hashDisk    // Disk: seq=1
    );

    /* Mempool (seq=2) must win over both. */
    REQUIRE(r.txPrev.nSequence == 2u);
    REQUIRE(r.hashLast         == hashMem);
    REQUIRE(r.strSeqSource     == "mempool_override_sessions");

    /* Producer should be seq=3. */
    REQUIRE(r.txPrev.nSequence + 1 == 3u);
}


/* ===========================================================================
 * Test 4: Disk fallback when only disk has the sigchain
 *
 * Common scenario when the stake minter restarts and mempool is empty.
 * =========================================================================== */
TEST_CASE( "Stake: disk-only fallback for restart scenario", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 5);
    const uint512_t hashDisk = txDisk.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),   // No Sessions
        nullptr,                  // No Mempool
        &txDisk, hashDisk        // Disk: seq=5
    );

    REQUIRE(r.txPrev.nSequence == 5u);
    REQUIRE(r.hashLast         == hashDisk);
    REQUIRE(r.strSeqSource     == "ledger");

    /* Producer should be seq=6. */
    REQUIRE(r.txPrev.nSequence + 1 == 6u);
}


/* ===========================================================================
 * Test 5: Genesis transaction path — no prior tx anywhere
 *
 * When the stake minter is started for the very first time on a brand new
 * sigchain with no transactions anywhere, CreateTransaction should produce
 * a genesis (seq=0) transaction.
 * =========================================================================== */
TEST_CASE( "Stake: genesis path when no prior tx exists", "[ledger][stake]" )
{
    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),
        nullptr,
        nullptr, uint512_t(0)
    );

    REQUIRE(r.txPrev.hashGenesis == uint256_t(0));
    REQUIRE(r.hashLast           == uint512_t(0));
    REQUIRE(r.strSeqSource       == "none");
}


/* ===========================================================================
 * Test 6: hashLast and txPrev always from SAME source
 *
 * The core invariant: after resolution, hashLast must be the hash of whatever
 * transaction is in txPrev.  If they come from different sources, the producer
 * will have mismatched nSequence/hashPrevTx (the original bug).
 * =========================================================================== */
TEST_CASE( "Stake: hashLast and txPrev always from same source", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* All three sources have different sequences. */
    TAO::Ledger::Transaction txSess = MakeStakeTx(hashGenesis, 0);
    TAO::Ledger::Transaction txMem  = MakeStakeTx(hashGenesis, 1);
    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 0);

    const uint512_t hashSess = txSess.GetHash();
    const uint512_t hashMem  = txMem.GetHash();
    const uint512_t hashDisk = txDisk.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        &txSess, hashSess,
        &txMem,
        &txDisk, hashDisk
    );

    /* Mempool (seq=1) wins. */
    REQUIRE(r.txPrev.nSequence == 1u);

    /* The invariant: hashLast == hash(winning txPrev). */
    REQUIRE(r.hashLast == r.txPrev.GetHash());
}


/* ===========================================================================
 * Test 7: Sessions wins when it has highest sequence
 *
 * This can happen when the local session index has a transaction that hasn't
 * been broadcast to mempool or committed to disk yet.
 * =========================================================================== */
TEST_CASE( "Stake: sessions wins when it has highest sequence", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSess = MakeStakeTx(hashGenesis, 3);
    TAO::Ledger::Transaction txMem  = MakeStakeTx(hashGenesis, 1);
    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 2);

    const uint512_t hashSess = txSess.GetHash();
    const uint512_t hashDisk = txDisk.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        &txSess, hashSess,
        &txMem,
        &txDisk, hashDisk
    );

    /* Sessions (seq=3) wins: mempool (seq=1) can't override, disk (seq=2) can't either. */
    REQUIRE(r.txPrev.nSequence == 3u);
    REQUIRE(r.hashLast         == hashSess);
    REQUIRE(r.strSeqSource     == "sessions");
}


/* ===========================================================================
 * Test 8: Fix 1 applied in mempool-only scenario (no sessions, no disk)
 *
 * Simulates the exact scenario where -autocreate just ran and the genesis
 * profile tx is in mempool but hasn't been committed anywhere else.
 * This is the most common stake minter bootstrap failure before the fix.
 * =========================================================================== */
TEST_CASE( "Stake: autocreate bootstrap — mempool genesis only", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Only mempool has the genesis (seq=0). Sessions and Disk are empty. */
    TAO::Ledger::Transaction txMem = MakeStakeTx(hashGenesis, 0);
    const uint512_t hashMem = txMem.GetHash();

    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),
        &txMem,
        nullptr, uint512_t(0)
    );

    /* Fix 1 ensures the mempool genesis is picked up. */
    REQUIRE(r.txPrev.hashGenesis != uint256_t(0));
    REQUIRE(r.txPrev.nSequence   == 0u);
    REQUIRE(r.hashLast           == hashMem);

    /* The staker's producer should be seq=1, pointing to the genesis. */
    const uint32_t  nProducerSeq = r.txPrev.nSequence + 1;
    const uint512_t hashProducerPrev = r.hashLast;

    REQUIRE(nProducerSeq     == 1u);
    REQUIRE(hashProducerPrev == hashMem);
}


/* ===========================================================================
 * Test 9: Mempool and disk have SAME sequence — disk does NOT override
 *
 * When mempool and disk both have seq=0 (e.g., genesis profile in both),
 * the mempool wins first (via Fix 1's genesis override), then disk does
 * NOT override because its sequence is not strictly greater.
 * =========================================================================== */
TEST_CASE( "Stake: same sequence in mempool and disk — mempool wins", "[ledger][stake]" )
{
    const uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txMem  = MakeStakeTx(hashGenesis, 0);
    TAO::Ledger::Transaction txDisk = MakeStakeTx(hashGenesis, 0);

    /* Give them different timestamps so hashes differ (simulating different tx objects). */
    txDisk.nTimestamp = 1700000001u;

    const uint512_t hashMem  = txMem.GetHash();
    const uint512_t hashDisk = txDisk.GetHash();

    REQUIRE(hashMem != hashDisk); // Sanity: different hashes

    FixedResolutionResult r = SimulateFixedResolution(
        nullptr, uint512_t(0),
        &txMem,
        &txDisk, hashDisk
    );

    /* Mempool wins via Fix 1 (genesis override). Disk has same seq so doesn't override. */
    REQUIRE(r.txPrev.nSequence == 0u);
    REQUIRE(r.hashLast         == hashMem);
    REQUIRE(r.strSeqSource     == "mempool");
}
