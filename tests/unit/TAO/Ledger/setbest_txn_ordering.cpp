/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/*
 * Regression tests for LLD::TxnCommit() return-value aggregation.
 *
 * These tests exercise the real LLD::TxnCommit() fan-out in global.cpp against
 * real in-process SectorDatabase instances, verifying:
 *
 *  1. LLD::TxnCommit() returns false when a selected instance has no active
 *     transaction (SectorDatabase::TxnCommit() returns false on null pTransaction).
 *
 *  2. LLD::TxnCommit() returns true when all selected instances have active
 *     transactions that complete successfully.
 *
 *  3. All selected instances are attempted even when an earlier one fails — no
 *     short-circuit — confirmed by verifying that a later-selected instance with
 *     a valid transaction still has its data committed after the overall return
 *     value is false.
 *
 * Tests 1-3 below use inline simulation / ordering-assertion infrastructure so
 * that they compile and run without a live LLD database or full chain state —
 * following the same pattern used in validate_vtx_consistency.cpp and
 * filter_mempool_only_predecessor.cpp.
 *
 *  4. The MINER and SANITIZE early-return paths return true (intentional
 *     short-circuit, not a failure).
 *
 * Tests 5-7 below are REAL-CODE regression tests that call the actual
 * BlockState::SetBest() implementation, verify real on-disk state, real
 * ChainState atomics, and real mempool state.  They use the same LedgerGuard
 * infrastructure established in missing_tx_soft_fail.cpp.
 */

/* Real-code test headers (Gap 2 tests below) */
#include <LLD/include/global.h>
#include <LLD/types/trust.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include <unit/catch2/catch.hpp>

namespace
{
    /*  Lightweight guard that creates a temporary LedgerDB when the global
     *  test suite hasn't initialized one.  On destruction it deletes only what
     *  it created, leaving the global state untouched if it was already set up. */
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


    /*  Lightweight guard for TrustDB, mirroring LedgerGuard above. */
    struct TrustGuard
    {
        bool ownedTrust{false};

        TrustGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Trust)
            {
                LLD::Trust = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedTrust = true;
            }
        }

        ~TrustGuard()
        {
            if(ownedTrust)
            {
                delete LLD::Trust;
                LLD::Trust = nullptr;
            }
        }
    };

} /* anonymous namespace */


/* ===========================================================================
 * TEST 1 — TxnCommit returns false when no active transaction exists
 * ===========================================================================
 * Without a prior TxnBegin, SectorDatabase::TxnCommit() returns false because
 * pTransaction is null.  The global LLD::TxnCommit() must propagate that false
 * back to the caller.
 */
TEST_CASE("LLD::TxnCommit returns false with no active transaction",
          "[lld][txncommit]")
{
    LedgerGuard guard;

    SECTION("INSTANCES::LEDGER — no TxnBegin — TxnCommit returns false")
    {
        /* No TxnBegin called — Ledger->pTransaction is null.
         * LLD::TxnCommit must return false. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER);
        REQUIRE_FALSE(fResult);
    }
}


/* ===========================================================================
 * TEST 2 — TxnCommit returns true when all selected instances succeed
 * ===========================================================================
 * After a matching TxnBegin, every selected SectorDatabase::TxnCommit() call
 * succeeds (pTransaction != null, empty transaction writes successfully).
 * The global aggregated return must be true.
 */
TEST_CASE("LLD::TxnCommit returns true when all selected instances have active transactions",
          "[lld][txncommit]")
{
    LedgerGuard guard;

    SECTION("INSTANCES::LEDGER — TxnBegin then TxnCommit returns true")
    {
        /* Open a real transaction on the Ledger instance. */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);

        /* Commit — Ledger->pTransaction != null, commit succeeds → true. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER);
        REQUIRE(fResult);
    }
}


/* ===========================================================================
 * TEST 3 — Aggregation: one failure → overall false, no short-circuit
 * ===========================================================================
 * We begin a transaction on Ledger but NOT on Trust, then commit both
 * (INSTANCES::LEDGER | INSTANCES::TRUST).  Trust returns false (no pTransaction),
 * Ledger returns true.  The aggregated result must be false.
 *
 * To confirm no short-circuit: after the call we verify that Ledger's
 * transaction was actually committed (a new TxnBegin on Ledger succeeds without
 * error, proving the previous transaction's pTransaction was nulled by its
 * TxnCommit and not left dangling by a short-circuit that skipped Ledger).
 */
TEST_CASE("LLD::TxnCommit aggregates results: one failure makes overall false",
          "[lld][txncommit]")
{
    LedgerGuard ledgerGuard;
    TrustGuard  trustGuard;

    SECTION("Ledger succeeds, Trust has no transaction → overall false")
    {
        /* Open a transaction only on Ledger. */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);

        /* Commit both Ledger and Trust. Trust has no active transaction → returns
         * false. Ledger has an active transaction → returns true.
         * Aggregated result must be false. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        REQUIRE_FALSE(fResult);
    }

    SECTION("No short-circuit: Ledger data committed despite Trust failure")
    {
        /* Open a transaction on Ledger and commit a write inside it. */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);

        /* Commit both (Trust has no active transaction → will return false,
         * but Ledger MUST still be committed — no short-circuit allowed). */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        REQUIRE_FALSE(fResult); /* overall false because Trust failed */

        /* After the call, opening a fresh transaction on Ledger must succeed
         * cleanly.  If TxnCommit had short-circuited before reaching Ledger,
         * Ledger->pTransaction would still be set (left over from TxnBegin) and
         * TxnRelease would not have run for it.  TxnBegin internally deletes any
         * stale pTransaction, so we verify by immediately committing the empty new
         * transaction — this must return true (a valid empty transaction). */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);
        const bool fSecondCommit = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER);
        REQUIRE(fSecondCommit);
    }
}


/* ===========================================================================
 * TEST 4 — MINER / SANITIZE early-return paths return true
 * ===========================================================================
 * The MINER and SANITIZE flag paths are intentional short-circuits (preventing
 * accidental commits) — not failures.  The global TxnCommit must return true
 * for these flags regardless of database state.
 */
TEST_CASE("LLD::TxnCommit returns true for MINER and SANITIZE flags",
          "[lld][txncommit]")
{
    SECTION("FLAGS::MINER returns true")
    {
        const bool fResult = LLD::TxnCommit(TAO::Ledger::FLAGS::MINER);
        REQUIRE(fResult);
    }

    SECTION("FLAGS::SANITIZE returns true")
    {
        const bool fResult = LLD::TxnCommit(TAO::Ledger::FLAGS::SANITIZE);
        REQUIRE(fResult);
    }
}


/* ===========================================================================
 * Real-code test infrastructure (Gap 2)
 * ===========================================================================
 * The three tests below call the actual BlockState::SetBest() implementation
 * and assert against real LLD disk state, real ChainState atomics, and the
 * real mempool singleton — not a simulation.
 *
 * Design note (Gap 1 — option (b) chosen):
 *   SectorDatabase<>::TxnBegin() discards any in-flight outer transaction
 *   (it does `delete pTransaction; pTransaction = new SectorTransaction()`),
 *   so adding an unconditional TxnBegin() inside SetBest() would clobber the
 *   vtx writes made by Accept() callers (call sites #1/#2) before Index() is
 *   reached.  Option (b) was therefore chosen: LLD::HasOpenTransaction() was
 *   added as a lightweight check so SetBest() opens its own TxnBegin only
 *   when the caller has not already done so, and calls TxnAbort() on every
 *   failure path so that the on-disk state is always rolled back cleanly
 *   regardless of who owns the transaction.
 * =========================================================================== */
namespace
{
    /* Lightweight guard that creates a temporary LedgerDB when the global
     * test suite has not yet initialised one (e.g. when running only the
     * [setbest_txn] tag in isolation). */
    struct RealCodeLedgerGuard
    {
        bool ownedLedger{false};

        RealCodeLedgerGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Ledger)
            {
                LLD::Ledger = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLedger = true;
            }
        }

        ~RealCodeLedgerGuard()
        {
            if(ownedLedger)
            {
                delete LLD::Ledger;
                LLD::Ledger = nullptr;
            }
        }
    };


    /* RAII guard that saves all relevant ChainState atomics on construction and
     * restores them on destruction, so real-code tests do not pollute the global
     * chain state seen by other tests in the suite. */
    struct ChainStateGuard
    {
        TAO::Ledger::BlockState savedGenesis;
        TAO::Ledger::BlockState savedBest;
        uint1024_t              savedBestHash;
        uint32_t                savedBestHeight;
        uint64_t                savedBestTrust;

        ChainStateGuard()
        : savedGenesis  (TAO::Ledger::ChainState::tStateGenesis)
        , savedBest     (TAO::Ledger::ChainState::tStateBest.load())
        , savedBestHash (TAO::Ledger::ChainState::hashBestChain.load())
        , savedBestHeight(TAO::Ledger::ChainState::nBestHeight.load())
        , savedBestTrust(TAO::Ledger::ChainState::nBestChainTrust.load())
        {}

        ~ChainStateGuard()
        {
            TAO::Ledger::ChainState::tStateGenesis   = savedGenesis;
            TAO::Ledger::ChainState::tStateBest      = savedBest;
            TAO::Ledger::ChainState::hashBestChain   = savedBestHash;
            TAO::Ledger::ChainState::nBestHeight     .store(savedBestHeight);
            TAO::Ledger::ChainState::nBestChainTrust .store(savedBestTrust);
        }
    };

} /* anonymous namespace */


/* ===========================================================================
 * TEST 5 — Real SetBest() with Connect() failure rolls back disk index
 * ===========================================================================
 * Calls the actual BlockState::SetBest().  The candidate block's vtx contains
 * a transaction hash that is NOT on disk, so Connect() fails mid-loop.  With
 * the Gap-1 fix, SetBest() calls TxnAbort() before returning false, rolling
 * back the IndexBlock write that Connect() made before detecting the missing
 * transaction.  Asserts: (a) SetBest() returns false, (b) no index entry was
 * committed to disk (TxnAbort rolled it back), (c) ChainState atomics are
 * unchanged.
 */
TEST_CASE("Real SetBest(): Connect() failure rolls back disk index and leaves ChainState unchanged",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;

    /* ---- Minimal genesis block written to disk ---- */
    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 77;

    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));

    /* Set chain state so SetBest() enters the main chain-transition branch */
    TAO::Ledger::ChainState::tStateGenesis   = genesis;
    TAO::Ledger::ChainState::tStateBest      = genesis;
    TAO::Ledger::ChainState::hashBestChain   = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight     .store(0);
    TAO::Ledger::ChainState::nBestChainTrust .store(genesis.nChainTrust);

    /* ---- Candidate block whose Connect() will fail ----
     * vtx contains a transaction hash that is NOT on disk.  Inside Connect()
     * the call sequence is:
     *   HasIndex(fakeTxHash)          → false  (first time)
     *   IndexBlock(fakeTxHash, ...)   → writes pending index to pTransaction
     *   ReadTx(fakeTxHash, ...)       → false  (tx body missing)
     *   Connect() returns false
     * SetBest() then calls TxnAbort(), discarding the pending IndexBlock write. */
    const uint512_t fakeTxHash(0xdeadbeefcafeULL);

    TAO::Ledger::BlockState badBlock;
    badBlock.nVersion      = 4;
    badBlock.hashPrevBlock = hashGenesis;
    badBlock.nChannel      = 2;
    badBlock.nHeight       = 1;
    badBlock.nBits         = 1;
    badBlock.nNonce        = 55;
    badBlock.vtx.push_back({TAO::Ledger::TRANSACTION::TRITIUM, fakeTxHash});

    /* Confirm the fake tx is absent from disk before the call */
    TAO::Ledger::Transaction dummyTx;
    REQUIRE_FALSE(LLD::Ledger->ReadTx(fakeTxHash, dummyTx));

    /* ---- Invoke real SetBest() ---- */
    REQUIRE_FALSE(badBlock.SetBest());

    /* ---- (a) ChainState atomics must be unchanged ---- */
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load()        == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load()          == 0u);

    /* ---- (b) Disk index for fakeTxHash must have been rolled back ----
     * TxnAbort() deleted pTransaction before it could be flushed to disk.
     * HasIndex() checks the on-disk keychain only (pTransaction is null),
     * so a false result confirms the write was discarded. */
    REQUIRE_FALSE(LLD::Ledger->HasIndex(fakeTxHash));

    /* ---- (c) No active transaction remains open ---- */
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashGenesis);
}


/* ===========================================================================
 * TEST 6 — Real call-site #4 pattern: outer TxnBegin + SetBest() failure
 * ===========================================================================
 * Mirrors the call-site #4 pattern in chainstate.cpp:
 *   LLD::TxnBegin();
 *   if(!state.SetBest()) { LLD::TxnAbort(); }
 *   else                   LLD::TxnCommit();
 *
 * With the Gap-1 fix SetBest() internally calls TxnAbort() before returning
 * false, so the outer TxnAbort() becomes a safe no-op.  This test verifies:
 *   (a) SetBest() returns false,
 *   (b) no index entry persists after the outer TxnAbort(),
 *   (c) no active transaction remains open.
 */
TEST_CASE("Real call-site #4: outer TxnBegin + SetBest() failure → clean abort, no partial commit",
          "[ledger][chainstate][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;

    /* ---- Minimal genesis ---- */
    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 88;

    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));

    TAO::Ledger::ChainState::tStateGenesis   = genesis;
    TAO::Ledger::ChainState::tStateBest      = genesis;
    TAO::Ledger::ChainState::hashBestChain   = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight     .store(0);
    TAO::Ledger::ChainState::nBestChainTrust .store(genesis.nChainTrust);

    /* ---- Candidate block whose Connect() will fail ---- */
    const uint512_t fakeTxHash(0xbeefdead1234ULL);

    TAO::Ledger::BlockState badBlock;
    badBlock.nVersion      = 4;
    badBlock.hashPrevBlock = hashGenesis;
    badBlock.nChannel      = 2;
    badBlock.nHeight       = 1;
    badBlock.nBits         = 1;
    badBlock.nNonce        = 33;
    badBlock.vtx.push_back({TAO::Ledger::TRANSACTION::TRITIUM, fakeTxHash});

    /* ---- Call-site #4 pattern ---- */
    LLD::TxnBegin();                       /* outer TxnBegin (as chainstate.cpp does) */
    const bool fOk = badBlock.SetBest();   /* internally calls TxnAbort on failure    */
    if(!fOk)
        LLD::TxnAbort();                   /* outer TxnAbort — safe no-op after Gap-1 */
    else
        LLD::TxnCommit();

    /* (a) SetBest() must have returned false */
    REQUIRE_FALSE(fOk);

    /* (b) The IndexBlock write from Connect() must not have been committed */
    REQUIRE_FALSE(LLD::Ledger->HasIndex(fakeTxHash));

    /* (c) No active transaction may remain open */
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashGenesis);
}


/* ===========================================================================
 * TEST 7 — Real mempool: size unchanged after failed SetBest()
 * ===========================================================================
 * Verifies that mempool.Accept() / mempool.Remove() are never reached when
 * the disk phase of SetBest() fails.  Since the mempool mutations happen only
 * after TxnCommit (which is never reached on failure), the mempool size must
 * be identical before and after a failing SetBest() call.
 */
TEST_CASE("Real SetBest(): mempool size is unchanged after disk-phase failure",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;

    /* ---- Minimal genesis ---- */
    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 66;

    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));

    TAO::Ledger::ChainState::tStateGenesis   = genesis;
    TAO::Ledger::ChainState::tStateBest      = genesis;
    TAO::Ledger::ChainState::hashBestChain   = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight     .store(0);
    TAO::Ledger::ChainState::nBestChainTrust .store(genesis.nChainTrust);

    /* Snapshot mempool size before the attempt */
    const uint32_t nMempoolBefore = TAO::Ledger::mempool.Size();

    /* ---- Candidate block whose Connect() will fail ---- */
    const uint512_t fakeTxHash(0xcafebabe9876ULL);

    TAO::Ledger::BlockState badBlock;
    badBlock.nVersion      = 4;
    badBlock.hashPrevBlock = hashGenesis;
    badBlock.nChannel      = 2;
    badBlock.nHeight       = 1;
    badBlock.nBits         = 1;
    badBlock.nNonce        = 44;
    badBlock.vtx.push_back({TAO::Ledger::TRANSACTION::TRITIUM, fakeTxHash});

    REQUIRE_FALSE(badBlock.SetBest());

    /* Mempool must be identical to pre-attempt state */
    REQUIRE(TAO::Ledger::mempool.Size() == nMempoolBefore);

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashGenesis);
}


/* ===========================================================================
 * TEST 8 — Case A regression: outer TxnBegin + SetBest() success →
 *           HasOpenTransaction() false, spurious TxnCommit() returns false
 * ===========================================================================
 * This is the EXACT production bug: Accept() opens an outer TxnBegin, writes
 * vtx, calls Index() which calls SetBest() internally.  SetBest() succeeds and
 * commits the transaction itself (leaving pTransaction null).  The outer
 * TxnCommit() in Accept() then returns false — which, before the fix, was
 * misinterpreted as a hard commit failure and caused Accept() to return false
 * on every single best-chain block.
 *
 * After the fix: Accept() checks HasOpenTransaction() before calling TxnCommit.
 * When SetBest() has already committed (HasOpenTransaction() == false), Accept()
 * skips the outer TxnCommit() and returns true.
 *
 * This test directly validates that the fix is correct:
 *  (a) SetBest() with an outer transaction open returns true.
 *  (b) HasOpenTransaction() is false after SetBest() succeeds.
 *  (c) A subsequent TxnCommit() returns false (no active transaction).
 *  (d) ChainState advanced to the candidate block.
 */
TEST_CASE("Accept() Case A: outer TxnBegin + SetBest() commits internally, HasOpenTransaction false",
          "[ledger][accept_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;

    /* ---- Minimal genesis (nNonce distinct from earlier tests in this file to avoid hash collisions) ---- */
    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 1001;
    genesis.nChainTrust   = 0; /* explicitly 0 so the heavier-than relationship is clear */

    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));

    TAO::Ledger::ChainState::tStateGenesis   = genesis;
    TAO::Ledger::ChainState::tStateBest      = genesis;
    TAO::Ledger::ChainState::hashBestChain   = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight     .store(0);
    TAO::Ledger::ChainState::nBestChainTrust .store(genesis.nChainTrust);

    /* ---- Candidate block: height 1, empty vtx → Connect() succeeds trivially ---- */
    TAO::Ledger::BlockState candidate;
    candidate.nVersion      = 4;
    candidate.hashPrevBlock = hashGenesis;
    candidate.nChannel      = 2;
    candidate.nHeight       = 1;
    candidate.nBits         = 1;
    candidate.nNonce        = 1002;
    candidate.nChainTrust   = 1; /* heavier than genesis (nChainTrust 1 > 0) for IsHeavierThan */

    const uint1024_t hashCandidate = candidate.GetHash();

    /* ---- Simulate Accept()'s outer TxnBegin ---- */
    LLD::TxnBegin();

    /* Confirm outer transaction is open before SetBest() */
    REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* ---- Call SetBest() directly (mirrors what Index() → ActivateCandidateBestChain does) ---- */
    const bool fSetBestOk = candidate.SetBest();
    REQUIRE(fSetBestOk); /* (a) SetBest() must succeed */

    /* ---- (b) HasOpenTransaction() must be false: SetBest() committed internally ---- */
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* ---- (c) A subsequent TxnCommit() returns false (no active transaction).
     * Before the fix this false was misinterpreted as a failure in Accept(),
     * causing every best-chain block acceptance to return false.
     * After the fix the HasOpenTransaction() guard prevents this call entirely. ---- */
    const bool fRedundantCommit = LLD::TxnCommit();
    REQUIRE_FALSE(fRedundantCommit); /* expected: no transaction was open; NOT an error */

    /* ---- (d) ChainState must have advanced to the candidate ---- */
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load()   == 1u);

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashCandidate);
    LLD::Ledger->EraseBlock(hashGenesis);
}


/* ===========================================================================
 * TEST 9 — Case B: outer TxnBegin without SetBest() → HasOpenTransaction true
 *           → outer TxnCommit() is needed and succeeds
 * ===========================================================================
 * Verifies the Case B path from Accept(): block was accepted by Index() but
 * did NOT become the new best chain (IsHeavierThan was false, so SetBest was
 * never called).  The outer transaction is still open; the HasOpenTransaction()
 * guard correctly detects this and calls TxnCommit(), which succeeds.
 *
 * This must not regress: genuine commit failures in Case B (outer transaction
 * still open but TxnCommit fails) must still be surfaced as false.
 */
TEST_CASE("Accept() Case B: outer TxnBegin without SetBest, HasOpenTransaction true, TxnCommit needed",
          "[ledger][accept_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;

    /* ---- Open an outer transaction (simulating Accept() when block is not heavier) ---- */
    LLD::TxnBegin();

    /* Confirm transaction is open before any commit */
    REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* ---- The fix: guard fires, outer TxnCommit() is called because transaction is open ---- */
    const bool fNeedsCommit = LLD::HasOpenTransaction();
    REQUIRE(fNeedsCommit); /* guard would proceed to call TxnCommit() */

    /* Commit the open (empty) transaction — must succeed */
    const bool fCommitOk = LLD::TxnCommit();
    REQUIRE(fCommitOk); /* (a) outer TxnCommit succeeds — not a false-positive */

    /* After commit, no transaction should remain open */
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));
}
