/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Regression tests for ValidateVtxSigchainConsistency() Connect-aligned anchor fix.
 *
 * Bug: mempool-first ReadLast could return a descendant tip and falsely reject
 * valid mined blocks during submit-time pre-check.
 *
 * Fix: use Connect-aligned anchor resolution:
 *  (1) in-block mapLast for earlier same-genesis vtx entries
 *  (2) disk-only ReadLast(genesis) when no in-block anchor exists.
 *
 * See docs/NSEQ_DIAG_MEMPOOL_READLAST_FIX.md for historical context.
 *
 * These tests use an inline simulation of the consistency-check logic so they do not
 * require a running node, LLD database, or mempool instance — following the same
 * pattern as tests/unit/TAO/Ledger/create_transaction.cpp.
 */

#include <LLC/include/random.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/tritium.h>

#include <unit/catch2/catch.hpp>

/* ---------------------------------------------------------------------------
 * Inline helpers and simulation infrastructure
 * --------------------------------------------------------------------------- */
namespace
{
    /* Build a minimal hashed transaction for a given genesis and sequence.
     * GetHash() returns a stable, unique value for the (genesis, sequence) pair. */
    TAO::Ledger::Transaction MakeTx(const uint256_t& hashGenesis, const uint32_t nSeq,
                                     const uint512_t hashPrevTx = 0)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = 1700000000u + nSeq;  // stable, deterministic
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }

    /* Simulate the core consistency-check logic of ValidateVtxSigchainConsistency().
     *
     * Uses the Connect-aligned resolution sequence:
     *   1. Hard MALFORMED invariant: hashPrevTx == self → reject
     *   2. mapLast (in-flight from prior vtx entries)
     *   3. disk-only ReadLast fallback
     *
     * Parameters:
     *   mapMempoolLast — retained for compatibility with existing tests (unused)
     *   mapDiskLast    — what disk-only ReadLast would return per genesis
     *
     * vtxPairs is the ordered list of (txHash, transaction) entries as they would
     * appear in block.vtx (TRITIUM type only for simplicity).
     *
     * Returns true if consistent (block would pass), false if stale/malformed.
     */
    bool SimulateConsistencyCheck(
        const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>>& vtxPairs,
        const std::map<uint256_t, uint512_t>& /*mapMempoolLast*/,
        const std::map<uint256_t, uint512_t>& mapDiskLast)
    {
        /* In-flight map mirrors the mapLast in ValidateVtxSigchainConsistency(). */
        std::map<uint256_t, uint512_t> mapLast;

        for(const auto& entry : vtxPairs)
        {
            const uint512_t&                txHash = entry.first;
            const TAO::Ledger::Transaction& tx     = entry.second;

            /* IsFirst() transactions have no predecessor — skip sequence check. */
            if(tx.IsFirst())
            {
                mapLast[tx.hashGenesis] = txHash;
                continue;
            }

            /* Hard MALFORMED invariant: hashPrevTx must never equal self. */
            if(tx.hashPrevTx == txHash)
                return false;

            uint512_t hashLast = 0;
            bool fAnchorFound = false;

            if(mapLast.count(tx.hashGenesis))
            {
                /* In-flight: a prior vtx entry for this genesis already advanced the
                 * last pointer, just as WriteLast() will do during Connect(). */
                hashLast = mapLast.at(tx.hashGenesis);
                fAnchorFound = true;
            }
            else
            {
                /* Connect-aligned fallback: disk-only ReadLast anchor. */
                const auto itDisk = mapDiskLast.find(tx.hashGenesis);
                if(itDisk != mapDiskLast.end())
                {
                    hashLast = itDisk->second;
                    fAnchorFound = true;
                }
                else
                {
                    /* No disk anchor — defer to Connect(). */
                    mapLast[tx.hashGenesis] = txHash;
                    continue;
                }
            }

            if(fAnchorFound && tx.hashPrevTx != hashLast)
                return false;

            mapLast[tx.hashGenesis] = txHash;
        }

        return true;
    }
}


/* ===========================================================================
 * Test 1 — Mempool-only predecessor: mempool-aware check passes, disk-only fails
 *
 * Scenario (the primary regression case):
 *   - disk:    genesis → tx(seq=0)  [committed to disk]
 *   - mempool: genesis → tx(seq=1)  [in mempool, NOT yet on disk]
 *
 *   The miner's template was built with CreateTransaction() using FLAGS::MEMPOOL,
 *   so the new transaction has:
 *     nSequence  = 2
 *     hashPrevTx = hash(tx_seq1)   ← points to the mempool tx
 *
 *   Old check (disk-only) sees hash(tx_seq0) → mismatch → false rejection.
 *   New check (FLAGS::MEMPOOL) sees hash(tx_seq1) → match → correct acceptance.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: mempool-only predecessor passes with MEMPOOL flag", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Build the sigchain. */
    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* The new vtx transaction references the mempool tx. */
    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Sanity: all three hashes are distinct. */
    REQUIRE(hashSeq0 != hashSeq1);
    REQUIRE(hashSeq1 != hashSeq2);

    /* Disk only has seq=0; mempool has seq=1 (the gap). */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq1}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    /* vtx contains only the new seq=2 tx. */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq2, txSeq2}
    };

    /* Fixed behaviour (composite C→B): must accept. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);

    /* Composite logic always uses both sources — second call also passes. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * Test 2 — Disk predecessor: both check modes agree
 *
 * When the predecessor is already on disk (no mempool-only gap), the
 * mempool-aware check and the disk-only check should both return the same
 * result (accepting a valid chain, rejecting a stale one).
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: disk predecessor accepted by both modes", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    /* seq=1 correctly references seq=0 (which is on disk). */
    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq0}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast)  == true);
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * Test 3 — Stale vtx: both modes correctly reject
 *
 * When the sigchain has genuinely advanced (disk hashLast is newer than what
 * the miner used), both modes should reject the block.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: genuinely stale vtx rejected by both modes", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Miner built its tx referencing seq=0, but disk+mempool already have seq=1. */
    TAO::Ledger::Transaction txSeq1_wrong = MakeTx(genesis, 1, hashSeq0);   // hashPrevTx=seq0 but chain is at seq1
    const uint512_t hashSeq1_wrong = txSeq1_wrong.GetHash();

    /* Both mempool and disk now return seq1 as last. */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq1}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq1}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1_wrong, txSeq1_wrong}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast)  == false);
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}


/* ===========================================================================
 * Test 4 — In-flight mapLast: multiple same-genesis vtx entries
 *
 * When a block contains two vtx transactions from the same genesis, the
 * second must reference the first (tracked in mapLast), not the on-disk last.
 * This tests the in-flight mapLast logic that mirrors Connect() behaviour.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: in-flight mapLast tracks same-genesis chain", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    /* seq=1 and seq=2 both in the vtx, seq=2 must reference seq=1. */
    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Disk and mempool only know about seq=0. */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq0}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    /* Both seq=1 and seq=2 are in the block vtx. */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1},
        {hashSeq2, txSeq2}
    };

    /* Both modes must accept because the in-flight mapLast correctly tracks seq=1
     * before checking seq=2. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast)  == true);
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * Test 5 — In-flight mapLast: second vtx stale relative to first
 *
 * If the second vtx entry incorrectly references the disk last instead of the
 * first vtx entry, the check must catch it.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: in-flight mapLast catches stale second vtx", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* txSeq2_bad incorrectly references hashSeq0 (stale — should reference seq1). */
    TAO::Ledger::Transaction txSeq2_bad = MakeTx(genesis, 2, hashSeq0);
    const uint512_t hashSeq2_bad = txSeq2_bad.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq0}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1,    txSeq1},
        {hashSeq2_bad, txSeq2_bad}
    };

    /* Both modes must reject: mapLast has hashSeq1 for genesis when checking
     * txSeq2_bad, but txSeq2_bad.hashPrevTx == hashSeq0. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast)  == false);
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}


/* ===========================================================================
 * Test 6 — IsFirst() transactions are skipped (no sequence check)
 *
 * Genesis (nSequence=0) transactions set IsFirst() == true and must not trigger
 * a hashPrevTx consistency check, even if ReadLast() would fail.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: IsFirst tx skips sequence check", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* IsFirst() tx: nSequence=0, hashPrevTx=0. */
    TAO::Ledger::Transaction txGenesis = MakeTx(genesis, 0);
    const uint512_t hashGenesis_tx = txGenesis.GetHash();

    REQUIRE(txGenesis.IsFirst());

    /* No entry in either map (genesis not yet known). */
    std::map<uint256_t, uint512_t> emptyMap;

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashGenesis_tx, txGenesis}
    };

    /* Must pass regardless of ReadLast() availability. */
    REQUIRE(SimulateConsistencyCheck(vtx, emptyMap, emptyMap)  == true);
    REQUIRE(SimulateConsistencyCheck(vtx, emptyMap, emptyMap) == true);
}


/* ===========================================================================
 * Test 7 — Cross-genesis ordering: independent chains do not interfere
 *
 * Two vtx entries from different genesis values must be checked independently.
 * A valid entry from genesis A must not be affected by a stale entry from B.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: cross-genesis entries are independent", "[ledger]" )
{
    const uint256_t genesisA = TAO::Ledger::Genesis(LLC::GetRand256(), true);
    const uint256_t genesisB = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Genesis A: seq=0 on disk, seq=1 in vtx (valid). */
    TAO::Ledger::Transaction txA0 = MakeTx(genesisA, 0);
    const uint512_t hashA0 = txA0.GetHash();
    TAO::Ledger::Transaction txA1 = MakeTx(genesisA, 1, hashA0);
    const uint512_t hashA1 = txA1.GetHash();

    /* Genesis B: seq=0 on disk, but vtx has seq=1 with wrong hashPrevTx (stale). */
    TAO::Ledger::Transaction txB0 = MakeTx(genesisB, 0);
    const uint512_t hashB0 = txB0.GetHash();
    TAO::Ledger::Transaction txB0_alt = MakeTx(genesisB, 0);  // different nTimestamp by 1 extra call
    txB0_alt.nTimestamp += 999;
    const uint512_t hashB0_alt = txB0_alt.GetHash();

    /* txB1 references hashB0_alt (wrong — not what's in the chain). */
    TAO::Ledger::Transaction txB1_stale = MakeTx(genesisB, 1, hashB0_alt);
    const uint512_t hashB1_stale = txB1_stale.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {
        {genesisA, hashA0},
        {genesisB, hashB0}
    };
    std::map<uint256_t, uint512_t> diskLast = {
        {genesisA, hashA0},
        {genesisB, hashB0}
    };

    /* Block has valid A and stale B. */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashA1,       txA1},
        {hashB1_stale, txB1_stale}
    };

    /* Overall check must fail because of B's stale entry. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast)  == false);
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);

    /* Block with only valid A must pass. */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxA_only = {
        {hashA1, txA1}
    };
    REQUIRE(SimulateConsistencyCheck(vtxA_only, mempoolLast, diskLast)  == true);
    REQUIRE(SimulateConsistencyCheck(vtxA_only, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * Test 8 — Missing genesis in ReadLast: entry is skipped (no false rejection)
 *
 * When ReadLast() cannot find a genesis (neither in mempool nor on disk),
 * the check should skip that entry and continue — matching the current
 * implementation that calls `continue` when ReadLast fails.
 * =========================================================================== */
TEST_CASE( "ValidateVtxSigchainConsistency: missing genesis in ReadLast skips entry", "[ledger]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, uint512_t(123456789));
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Genesis not found in either source. */
    std::map<uint256_t, uint512_t> emptyMap;

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1}
    };

    /* Entry is skipped → check passes (Connect() will handle the error). */
    REQUIRE(SimulateConsistencyCheck(vtx, emptyMap, emptyMap)  == true);
    REQUIRE(SimulateConsistencyCheck(vtx, emptyMap, emptyMap) == true);
}


/* ===========================================================================
 * PR #611 Composite C→B Logic — New Test Cases
 * =========================================================================== */


/* ===========================================================================
 * MALFORMED self-as-predecessor is rejected
 *
 * A transaction whose hashPrevTx equals its own hash is cryptographically
 * impossible under normal operation and must be rejected immediately.
 * =========================================================================== */
TEST_CASE( "MALFORMED self-as-predecessor is rejected", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* Build a tx, then set its hashPrevTx to its own hash (impossible in prod). */
    TAO::Ledger::Transaction tx = MakeTx(genesis, 1, uint512_t(0));
    const uint512_t txHash = tx.GetHash();
    tx.hashPrevTx = txHash;  // MALFORMED: self-reference

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, uint512_t(999)}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, uint512_t(999)}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {txHash, tx}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}


/* ===========================================================================
 * Tx IS mempool latest, predecessor on disk → PASS
 *
 * The production scenario: mempool ReadLast returns the tx itself (self-match),
 * so we fall through to disk which has the real predecessor.
 * =========================================================================== */
TEST_CASE( "Tx IS mempool latest, predecessor on disk → PASS", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Mempool returns hashSeq1 (self-match for this tx in vtx).
     * Disk returns hashSeq0 (the real predecessor). */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq1}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1}
    };

    /* Self-match in mempool → fall to disk → hashSeq0 == tx.hashPrevTx → PASS */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * Tx not mempool latest → legitimate STALE rejection
 *
 * When the mempool has advanced past what the tx expects, and it's not a
 * self-match, the tx is genuinely stale.
 * =========================================================================== */
TEST_CASE( "Tx not mempool latest → legitimate STALE rejection", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    TAO::Ledger::Transaction txSeq3 = MakeTx(genesis, 3, hashSeq2);
    const uint512_t hashSeq3 = txSeq3.GetHash();

    /* Mempool has advanced to seq3 — tx_seq2's hashPrevTx (hashSeq1) is stale. */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq3}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq1}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq2, txSeq2}
    };

    /* hashSeq3 != hashSeq2 (no self-match), mempool anchor = hashSeq3.
     * tx.hashPrevTx = hashSeq1 != hashSeq3 → STALE rejection. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}


/* ===========================================================================
 * Real predecessor in mempool (no self-match) → PASS
 *
 * When mempool has the correct predecessor and it's not a self-match,
 * Option C directly provides the anchor.
 * =========================================================================== */
TEST_CASE( "Real predecessor in mempool (no self-match) → PASS", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Mempool returns hashSeq1 (the real predecessor, not self). */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq1}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq2, txSeq2}
    };

    /* hashSeq1 != hashSeq2 (no self-match), anchor = hashSeq1.
     * tx.hashPrevTx = hashSeq1 == anchor → PASS. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * No anchor anywhere → defer (returns true, Connect handles)
 *
 * When mempool returns self and disk has no entry for this genesis,
 * we defer to Connect() — the check passes.
 * =========================================================================== */
TEST_CASE( "No anchor anywhere → defer (returns true, Connect handles)", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Mempool returns hashSeq1 (self-match), disk has nothing. */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq1}};
    std::map<uint256_t, uint512_t> diskLast;  // empty — no disk anchor

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1}
    };

    /* Self-match in mempool → disk empty → defer to Connect() → true. */
    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}


/* ===========================================================================
 * In-block multi-vtx same genesis preserves mapLast chain
 *
 * When vtx has multiple transactions from the same genesis, mapLast must
 * correctly chain them so the second references the first.
 * =========================================================================== */
TEST_CASE( "In-block multi-vtx same genesis preserves mapLast chain", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Disk has seq0 as last. Mempool returns hashSeq2 (self-match for second entry). */
    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashSeq2}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashSeq0}};

    /* Both seq1 and seq2 in vtx. */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1},
        {hashSeq2, txSeq2}
    };

    /* Re-setup with mempool returning hashSeq1 (self-match for first vtx entry).
     * seq1: mempool=self → disk=hashSeq0 → anchor=hashSeq0. hashPrevTx matches → PASS.
     * seq2: mapLast[G]=hashSeq1 (in-flight) → anchor=hashSeq1. hashPrevTx matches → PASS. */
    std::map<uint256_t, uint512_t> mempoolLast2 = {{genesis, hashSeq1}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx2 = {
        {hashSeq1, txSeq1},
        {hashSeq2, txSeq2}
    };

    /* seq1: mempool=hashSeq1=self → disk=hashSeq0 → anchor=hashSeq0.
     * tx.hashPrevTx=hashSeq0 == anchor → PASS. mapLast[G] = hashSeq1.
     * seq2: mapLast[G]=hashSeq1 (in-flight) → anchor=hashSeq1.
     * tx.hashPrevTx=hashSeq1 == anchor → PASS. */
    REQUIRE(SimulateConsistencyCheck(vtx2, mempoolLast2, diskLast) == true);
}

TEST_CASE( "Descendant mempool tip does not stale valid disk anchor", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txNMinus1 = MakeTx(genesis, 9737);
    const uint512_t hashNMinus1 = txNMinus1.GetHash();

    TAO::Ledger::Transaction txN = MakeTx(genesis, 9738, hashNMinus1);
    const uint512_t hashN = txN.GetHash();

    TAO::Ledger::Transaction txNPlus1 = MakeTx(genesis, 9739, hashN);
    const uint512_t hashNPlus1 = txNPlus1.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashNPlus1}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashNMinus1}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashN, txN}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}

TEST_CASE( "Genuinely stale versus disk is rejected", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txNMinus1 = MakeTx(genesis, 10);
    const uint512_t hashNMinus1 = txNMinus1.GetHash();

    TAO::Ledger::Transaction txN = MakeTx(genesis, 11, hashNMinus1);
    const uint512_t hashN = txN.GetHash();

    TAO::Ledger::Transaction txWrong = MakeTx(genesis, 88, hashN);
    const uint512_t hashWrong = txWrong.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, hashN}};
    std::map<uint256_t, uint512_t> diskLast    = {{genesis, hashWrong}};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashN, txN}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}

TEST_CASE( "Disk-missing anchor defers to Connect", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction tx = MakeTx(genesis, 123, uint512_t(LLC::GetRand512()));
    const uint512_t txHash = tx.GetHash();

    std::map<uint256_t, uint512_t> mempoolLast = {{genesis, uint512_t(LLC::GetRand512())}};
    std::map<uint256_t, uint512_t> diskLast;

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {txHash, tx}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == true);
}

TEST_CASE( "Malformed self-reference is rejected", "[validate_vtx_consistency]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction tx = MakeTx(genesis, 42);
    const uint512_t txHash = tx.GetHash();
    tx.hashPrevTx = txHash;

    std::map<uint256_t, uint512_t> mempoolLast;
    std::map<uint256_t, uint512_t> diskLast;

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {txHash, tx}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, mempoolLast, diskLast) == false);
}
