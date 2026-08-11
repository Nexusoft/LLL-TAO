/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Design-contract tests for the proposed Option B filter:
 *
 *   FilterMempoolOnlyPredecessor(vtx, hasDisk, hasMempool)
 *
 *     Drop any TRITIUM vtx whose hashPrevTx is mempool-only (present in
 *     mempool, absent from disk), with the following exemptions:
 *
 *       (1) Genesis transactions: IsFirst() == true, hashPrevTx == 0
 *           are unconditionally kept (no predecessor by design).
 *
 *       (2) In-block chained transactions: if hashPrevTx matches a vtx
 *           already accepted earlier in the same candidate block, the
 *           predecessor is treated as on-disk-equivalent for filter
 *           purposes (it will be persisted together with this tx).
 *
 *     The filter is channel-agnostic — it is intended to be applied
 *     inside AddTransactions() in src/TAO/Ledger/create.cpp so that
 *     Prime (1), Hash (2), Stake (0), and Private (3) channels all
 *     receive the same correctness-positive behaviour.
 *
 * These tests use the same "inline simulation" pattern as
 *   tests/unit/TAO/Ledger/validate_vtx_consistency.cpp
 *   tests/unit/TAO/Ledger/create_transaction.cpp
 * so they do not require a running node, LLD database, or mempool
 * instance. They pin down the desired contract before the production
 * implementation lands; a follow-up PR is expected to bind the real
 * AddTransactions() filter to this contract.
 *
 * Related research notes: docs/architecture/TEMPLATE_IMMUTABILITY_AND_CACHE_CONSTRAINTS.md
 * and prior session research on Option B feasibility.
 */

#include <LLC/include/random.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/include/enum.h>

#include <unit/catch2/catch.hpp>

#include <set>
#include <vector>
#include <utility>


/* ---------------------------------------------------------------------------
 * Inline helpers
 * --------------------------------------------------------------------------- */
namespace
{
    /* Build a minimal hashed transaction for a given genesis and sequence.
     * GetHash() returns a stable, unique value for the (genesis, sequence,
     * hashPrevTx) tuple. Mirrors helpers in validate_vtx_consistency.cpp. */
    TAO::Ledger::Transaction MakeTx(const uint256_t& hashGenesis,
                                    const uint32_t   nSeq,
                                    const uint512_t  hashPrevTx = 0)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = 1700000000u + nSeq;
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }


    /* Simulated filter mirroring the proposed AddTransactions() insertion
     * point. Inputs model the two cheap classification primitives a real
     * implementation would call inside CreateBlock:
     *
     *   setDisk    — { h : LLD::Ledger->HasTx(h, FLAGS::BLOCK) == true }
     *   setMempool — { h : TAO::Ledger::mempool.Has(h)         == true }
     *
     * Output is the candidate vtx list with mempool-only-predecessor
     * entries removed. Genesis (IsFirst) and in-block-chained entries
     * are preserved. Input order of kept entries is preserved.
     *
     * NOTE: this helper is intentionally local to the test file. It
     * documents the desired contract; the production implementation
     * is expected to honour the same semantics in src/TAO/Ledger/create.cpp.
     */
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>>
    SimulateMempoolOnlyPredecessorFilter(
        const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>>& vtxIn,
        const std::set<uint512_t>& setDisk,
        const std::set<uint512_t>& setMempool)
    {
        std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxOut;
        std::set<uint512_t> setInBlock;

        for(const auto& entry : vtxIn)
        {
            const uint512_t&                txHash = entry.first;
            const TAO::Ledger::Transaction& tx     = entry.second;

            /* (1) Genesis transactions are unconditionally kept. */
            if(tx.IsFirst())
            {
                vtxOut.push_back(entry);
                setInBlock.insert(txHash);
                continue;
            }

            /* (2) In-block chained: predecessor is an earlier accepted vtx. */
            if(setInBlock.count(tx.hashPrevTx))
            {
                vtxOut.push_back(entry);
                setInBlock.insert(txHash);
                continue;
            }

            /* (3) Predecessor is disk-confirmed — keep. */
            if(setDisk.count(tx.hashPrevTx))
            {
                vtxOut.push_back(entry);
                setInBlock.insert(txHash);
                continue;
            }

            /* (4) Mempool-only predecessor — drop.
             * (5) Predecessor unknown to both disk and mempool — also drop.
             *     The producer of this template should not have selected an
             *     unknown predecessor, but if it did we drop it: AddTransactions()
             *     would otherwise carry a dangling reference into the block.
             *     Either way, falling off the loop body without push_back drops
             *     the entry. */
            (void)setMempool;
        }

        return vtxOut;
    }


    /* Convenience: returns true iff vtxOut contains an entry with txHash. */
    bool Contains(
        const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>>& vtx,
        const uint512_t& txHash)
    {
        for(const auto& entry : vtx)
            if(entry.first == txHash)
                return true;
        return false;
    }
}


/* ===========================================================================
 * Test 1 — Genesis (IsFirst) transactions are always kept
 *
 * A first-tx (nSequence==0, hashPrevTx==0) has no predecessor by design,
 * and must never be dropped on the grounds of "predecessor not on disk".
 * =========================================================================== */
TEST_CASE( "Option B filter: IsFirst genesis transactions are exempt",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txGen = MakeTx(genesis, 0);
    const uint512_t hashGen = txGen.GetHash();

    REQUIRE(txGen.IsFirst());

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashGen, txGen}
    };

    /* No disk, no mempool — yet genesis must survive. */
    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, {}, {});

    REQUIRE(vtxOut.size() == 1);
    REQUIRE(Contains(vtxOut, hashGen));
}


/* ===========================================================================
 * Test 2 — Disk-confirmed predecessor is kept
 *
 * The common steady-state case: a sigchain whose tip has already been
 * persisted to disk advances by one tx, the new tx enters mempool, and
 * the template build picks it up. Its predecessor is on disk — keep.
 * =========================================================================== */
TEST_CASE( "Option B filter: disk-confirmed predecessor is accepted",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Disk has seq=0; mempool has seq=1 (the candidate vtx). */
    const std::set<uint512_t> setDisk    = {hashSeq0};
    const std::set<uint512_t> setMempool = {hashSeq1};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashSeq1, txSeq1}
    };

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, setDisk, setMempool);

    REQUIRE(vtxOut.size() == 1);
    REQUIRE(Contains(vtxOut, hashSeq1));
}


/* ===========================================================================
 * Test 3 — Mempool-only predecessor is dropped (the core motivation)
 *
 * The production incident: a high-velocity sigchain's predecessor lives
 * only in mempool (not yet flushed to disk) at template build time. By the
 * time the block is signed, that mempool tx has been superseded and the
 * vtx in the template references a dead hashPrevTx, causing BLOCK_REJECTED
 * via ValidateVtxSigchainConsistency. The filter pre-empts that class of
 * failure by refusing to carry such vtx in the first place.
 * =========================================================================== */
TEST_CASE( "Option B filter: mempool-only predecessor is dropped",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Disk only has seq=0. Mempool has seq=1 AND seq=2 — the candidate vtx
     * (seq=2) has a mempool-only predecessor (seq=1). The filter must drop
     * seq=2 from the block; the producer should not include it. seq=1 is
     * not in vtxIn because, for this test, the simulator received the
     * tail-most chained tx only. */
    const std::set<uint512_t> setDisk    = {hashSeq0};
    const std::set<uint512_t> setMempool = {hashSeq1, hashSeq2};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashSeq2, txSeq2}
    };

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, setDisk, setMempool);

    REQUIRE(vtxOut.empty());
    REQUIRE_FALSE(Contains(vtxOut, hashSeq2));
}


/* ===========================================================================
 * Test 4 — In-block chained transactions are preserved
 *
 * Critical correctness case: if vtx[0] and vtx[1] are from the same
 * sigchain and vtx[1].hashPrevTx == vtx[0].GetHash(), then vtx[1]'s
 * predecessor IS technically mempool-only. The filter must NOT drop
 * vtx[1] on that basis — its predecessor is already in this block and
 * will be persisted atomically with it.
 *
 * A naive implementation that only consults disk/mempool sets without
 * tracking in-block predecessors would fail this test. The simulator
 * encodes the correct behaviour.
 * =========================================================================== */
TEST_CASE( "Option B filter: in-block chained predecessor is preserved",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    /* Disk has seq=0; mempool has both seq=1 and seq=2. Both are present
     * in vtxIn — the in-block chaining must keep both, in order. */
    const std::set<uint512_t> setDisk    = {hashSeq0};
    const std::set<uint512_t> setMempool = {hashSeq1, hashSeq2};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashSeq1, txSeq1},
        {hashSeq2, txSeq2}
    };

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, setDisk, setMempool);

    REQUIRE(vtxOut.size() == 2);
    REQUIRE(vtxOut[0].first == hashSeq1);
    REQUIRE(vtxOut[1].first == hashSeq2);
}


/* ===========================================================================
 * Test 5 — Mixed input: drop bad, keep good, preserve order
 *
 * Realistic template: one sigchain (A) advances cleanly from disk, another
 * (B) presents an isolated tail tx whose predecessor lives only in
 * mempool. The filter must drop B and keep A, preserving A's position.
 * =========================================================================== */
TEST_CASE( "Option B filter: mixed input drops mempool-only and preserves valid",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesisA = TAO::Ledger::Genesis(LLC::GetRand256(), true);
    const uint256_t genesisB = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* A: clean chain off disk. */
    TAO::Ledger::Transaction txA0 = MakeTx(genesisA, 0);
    const uint512_t hashA0 = txA0.GetHash();
    TAO::Ledger::Transaction txA1 = MakeTx(genesisA, 1, hashA0);
    const uint512_t hashA1 = txA1.GetHash();

    /* B: predecessor exists only in mempool (and is NOT in this block). */
    TAO::Ledger::Transaction txB0 = MakeTx(genesisB, 0);
    const uint512_t hashB0 = txB0.GetHash();
    TAO::Ledger::Transaction txB1 = MakeTx(genesisB, 1, hashB0);
    const uint512_t hashB1 = txB1.GetHash();
    TAO::Ledger::Transaction txB2 = MakeTx(genesisB, 2, hashB1);
    const uint512_t hashB2 = txB2.GetHash();

    const std::set<uint512_t> setDisk    = {hashA0};
    const std::set<uint512_t> setMempool = {hashA1, hashB1, hashB2};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashA1, txA1},
        {hashB2, txB2}
    };

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, setDisk, setMempool);

    REQUIRE(vtxOut.size() == 1);
    REQUIRE(vtxOut[0].first == hashA1);
    REQUIRE_FALSE(Contains(vtxOut, hashB2));
}


/* ===========================================================================
 * Test 6 — Channel-agnostic behaviour
 *
 * The user's design intent: the filter is correctness-positive on every
 * channel (Stake=0, Prime=1, Hash=2, Private=3), so it lives in the
 * shared AddTransactions() helper without channel gating. The filter
 * function exposes no channel parameter — this test simply re-asserts
 * the same predicate produces the same result regardless of which
 * channel the caller is building for.
 * =========================================================================== */
TEST_CASE( "Option B filter: channel-agnostic — identical results across channels",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();
    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* Disk-confirmed predecessor: must be accepted for every channel. */
    const std::set<uint512_t> setDisk    = {hashSeq0};
    const std::set<uint512_t> setMempool = {hashSeq1};

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashSeq1, txSeq1}
    };

    /* The simulator is by construction channel-agnostic. We iterate the
     * known channels to encode the design contract: same vtx, same
     * classification, same result on each channel. */
    const std::vector<uint8_t> channels = {
        TAO::Ledger::CHANNEL::STAKE,
        TAO::Ledger::CHANNEL::PRIME,
        TAO::Ledger::CHANNEL::HASH,
        TAO::Ledger::CHANNEL::PRIVATE
    };

    for(const uint8_t nChannel : channels)
    {
        (void)nChannel; /* filter takes no channel argument by design */
        const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, setDisk, setMempool);

        REQUIRE(vtxOut.size() == 1);
        REQUIRE(vtxOut[0].first == hashSeq1);
    }
}


/* ===========================================================================
 * Test 7 — Empty input is a no-op
 *
 * An empty mempool / template-build with no candidate vtx must produce
 * an empty result without error. This matches the "empty-vtx block is
 * consensus-legal" observation from the Option B feasibility research.
 * =========================================================================== */
TEST_CASE( "Option B filter: empty candidate list yields empty result",
           "[filter_mempool_only_predecessor]" )
{
    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn;

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, {}, {});

    REQUIRE(vtxOut.empty());
}


/* ===========================================================================
 * Test 8 — Unknown predecessor (not on disk, not in mempool) is dropped
 *
 * Defensive case: if a candidate vtx references a predecessor that the
 * node knows nothing about, the filter drops it. This matches the
 * "don't carry dangling references" property that AddTransactions()
 * already enforces today via ReadLast/Connect checks (see
 * src/TAO/Ledger/create.cpp:671-678).
 * =========================================================================== */
TEST_CASE( "Option B filter: unknown predecessor is dropped",
           "[filter_mempool_only_predecessor]" )
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    /* hashPrevTx points to a hash that exists nowhere. */
    TAO::Ledger::Transaction txOrphan =
        MakeTx(genesis, 1, uint512_t("0xdeadbeefcafe"));
    const uint512_t hashOrphan = txOrphan.GetHash();

    std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxIn = {
        {hashOrphan, txOrphan}
    };

    const auto vtxOut = SimulateMempoolOnlyPredecessorFilter(vtxIn, {}, {});

    REQUIRE(vtxOut.empty());
}
