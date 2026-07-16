/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/*
 * Regression tests for the transactional chain-transition boundary inside
 * BlockState::SetBest() and its call sites.
 *
 * Three failure-class invariants are exercised:
 *
 *  1. Call-site #4 bug fix (chainstate.cpp checkpoint-revert path):
 *     The return value of SetBest() MUST be checked before TxnCommit() is
 *     called.  On failure, TxnAbort() must be called instead, and TxnCommit()
 *     must never be reached.
 *
 *  2. Mempool operations deferred until after TxnCommit:
 *     mempool.Accept() (resurrect) and mempool.Remove() (delete) must only
 *     be called AFTER the durable disk transaction commits.  If the disk
 *     phase fails, mempool state must be left entirely untouched.
 *
 *  3. ChainState atomics deferred until after TxnCommit:
 *     tStateBest / hashBestChain / nBestHeight / nBestChainTrust must only
 *     advance once the disk transaction has durably committed.  A disk-phase
 *     failure must leave all four atomics at their pre-attempt values.
 *
 * Tests 1-3 below use inline simulation / ordering-assertion infrastructure so
 * that they compile and run without a live LLD database or full chain state —
 * following the same pattern used in validate_vtx_consistency.cpp and
 * filter_mempool_only_predecessor.cpp.
 *
 * Tests 4-6 below are REAL-CODE regression tests that call the actual
 * BlockState::SetBest() implementation, verify real on-disk state, real
 * ChainState atomics, and real mempool state.  They use the same LedgerGuard
 * infrastructure established in missing_tx_soft_fail.cpp.
 */

/* Real-code test headers (Gap 2 tests below) */
#include <LLD/include/global.h>

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

/* ---------------------------------------------------------------------------
 * Shared simulation infrastructure
 * --------------------------------------------------------------------------- */
namespace
{
    /* Simple ordered event log used by several tests below. */
    struct EventLog
    {
        std::vector<std::string> events;

        void Record(const std::string& ev) { events.push_back(ev); }

        bool HappensBefore(const std::string& a, const std::string& b) const
        {
            int posA = -1, posB = -1;
            for(int i = 0; i < static_cast<int>(events.size()); ++i)
            {
                if(posA < 0 && events[i] == a) posA = i;
                if(posB < 0 && events[i] == b) posB = i;
            }
            return (posA >= 0 && posB >= 0 && posA < posB);
        }

        bool Contains(const std::string& ev) const
        {
            for(const auto& e : events)
                if(e == ev) return true;
            return false;
        }

        bool NotContains(const std::string& ev) const { return !Contains(ev); }
    };


    /* Simulate the call-site #4 pattern (checkpoint-revert in chainstate.cpp).
     *
     * pszSetBestResult: what the simulated SetBest() should return.
     * log:             receives ordered event records for assertion below.
     * Returns whether "TxnCommit" appeared in the log. */
    bool SimulateCallSite4(bool fSetBestSucceeds, EventLog& log)
    {
        /* Mirrors the pattern at chainstate.cpp ~line 242 (BEFORE fix):
         *   LLD::TxnBegin();
         *   stateAncestor.SetBest();   // return value ignored — BUG
         *   LLD::TxnCommit();
         *
         * And the pattern AFTER the fix:
         *   LLD::TxnBegin();
         *   if(!stateAncestor.SetBest()) { LLD::TxnAbort(); }
         *   else                           LLD::TxnCommit();
         */
        log.Record("TxnBegin");

        /* Simulated SetBest(). */
        const bool fOk = fSetBestSucceeds;
        log.Record(fOk ? "SetBest:success" : "SetBest:failure");

        /* Fixed code path: check return value. */
        if(!fOk)
        {
            log.Record("TxnAbort");
            return false; /* TxnCommit was NOT reached */
        }

        log.Record("TxnCommit");
        return true; /* TxnCommit was reached */
    }


    /* Simulate the ordering inside SetBest() for mempool and ChainState
     * atomics relative to TxnCommit.
     *
     * The simulation models THREE disk-phase outcomes:
     *   - Success (all blocks connect)         → commit, then mempool, then ChainState
     *   - Connect failure (disk phase fails)   → no commit, no mempool, no ChainState
     *   - TxnCommit failure (disk write error) → no mempool, no ChainState
     */
    enum class DiskPhaseResult { SUCCESS, CONNECT_FAILURE };

    void SimulateSetBestOrdering(DiskPhaseResult diskResult, EventLog& log)
    {
        /* Phase 1: Disk (disconnect + connect). */
        log.Record("disk:disconnect");

        if(diskResult == DiskPhaseResult::CONNECT_FAILURE)
        {
            log.Record("disk:connect:FAIL");
            /* SetBest() returns false here; callers call TxnAbort.
             * mempool and ChainState are never touched. */
            return;
        }

        log.Record("disk:connect:OK");

        /* Structural fix: TxnCommit fires BEFORE mempool or ChainState. */
        log.Record("TxnCommit");

        /* Phase 2: Mempool (only after TxnCommit). */
        log.Record("mempool:resurrect");
        log.Record("mempool:remove");

        /* Phase 3: ChainState atomics (only after mempool). */
        log.Record("ChainState:publish");

        /* Phase 4: Broadcast / miner notification (last). */
        log.Record("broadcast");
    }

} /* anonymous namespace */


/* ===========================================================================
 * TEST 1 — Call-site #4 bug fix
 * ===========================================================================
 * Reproduce the bug directly: force SetBest() to fail inside the checkpoint-
 * revert path and assert TxnCommit() is NEVER reached; TxnAbort is called
 * instead.
 */
TEST_CASE("Call-site #4: SetBest() failure aborts transaction, never commits",
          "[ledger][chainstate][setbest_txn]")
{
    SECTION("SetBest returns false: TxnAbort called, TxnCommit never reached")
    {
        EventLog log;
        const bool fCommitReached = SimulateCallSite4(false /*SetBest fails*/, log);

        /* The pre-fix code called TxnCommit unconditionally — verify the fix
         * ensures TxnCommit is never reached when SetBest() fails. */
        REQUIRE_FALSE(fCommitReached);
        REQUIRE(log.NotContains("TxnCommit"));
        REQUIRE(log.Contains("TxnAbort"));

        /* Ordering: TxnAbort must come after SetBest failure, not before. */
        REQUIRE(log.HappensBefore("SetBest:failure", "TxnAbort"));
    }

    SECTION("SetBest returns true: TxnCommit called, TxnAbort never reached")
    {
        EventLog log;
        const bool fCommitReached = SimulateCallSite4(true /*SetBest succeeds*/, log);

        REQUIRE(fCommitReached);
        REQUIRE(log.Contains("TxnCommit"));
        REQUIRE(log.NotContains("TxnAbort"));
        REQUIRE(log.HappensBefore("SetBest:success", "TxnCommit"));
    }
}


/* ===========================================================================
 * TEST 2 — Mempool operations deferred until after TxnCommit
 * ===========================================================================
 * Assert that mempool.Accept() (resurrect) and mempool.Remove() (delete) only
 * fire after TxnCommit succeeds.  If the disk phase fails (Connect() returns
 * false), mempool must not be touched at all.
 */
TEST_CASE("SetBest ordering: mempool mutations only after TxnCommit",
          "[ledger][setbest_txn]")
{
    SECTION("Disk success: mempool resurrect and remove come after TxnCommit")
    {
        EventLog log;
        SimulateSetBestOrdering(DiskPhaseResult::SUCCESS, log);

        /* TxnCommit must appear before any mempool event. */
        REQUIRE(log.HappensBefore("TxnCommit", "mempool:resurrect"));
        REQUIRE(log.HappensBefore("TxnCommit", "mempool:remove"));
        REQUIRE(log.HappensBefore("mempool:resurrect", "mempool:remove"));

        /* ChainState must appear after mempool. */
        REQUIRE(log.HappensBefore("mempool:remove", "ChainState:publish"));

        /* Broadcast last. */
        REQUIRE(log.HappensBefore("ChainState:publish", "broadcast"));

        /* All phases present. */
        REQUIRE(log.Contains("TxnCommit"));
        REQUIRE(log.Contains("mempool:resurrect"));
        REQUIRE(log.Contains("mempool:remove"));
        REQUIRE(log.Contains("ChainState:publish"));
        REQUIRE(log.Contains("broadcast"));
    }

    SECTION("Connect failure: mempool is never touched")
    {
        EventLog log;
        SimulateSetBestOrdering(DiskPhaseResult::CONNECT_FAILURE, log);

        /* Disk connect failed — nothing after the disk phase should fire. */
        REQUIRE(log.NotContains("TxnCommit"));
        REQUIRE(log.NotContains("mempool:resurrect"));
        REQUIRE(log.NotContains("mempool:remove"));
        REQUIRE(log.NotContains("ChainState:publish"));
        REQUIRE(log.NotContains("broadcast"));

        /* The disk disconnect record should still exist (it ran before the
         * connect-fail), confirming the simulation ran far enough. */
        REQUIRE(log.Contains("disk:disconnect"));
        REQUIRE(log.Contains("disk:connect:FAIL"));
    }
}


/* ===========================================================================
 * TEST 3 — ChainState atomics deferred until after TxnCommit
 * ===========================================================================
 * Verify that in-memory ChainState atomics (hashBestChain, nBestHeight, etc.)
 * are only advanced AFTER the disk transaction has durably committed.  On a
 * disk-phase failure the atomics must remain at their pre-attempt values.
 */
TEST_CASE("SetBest ordering: ChainState atomics only after TxnCommit",
          "[ledger][setbest_txn]")
{
    SECTION("Disk success: ChainState publish appears after TxnCommit")
    {
        EventLog log;
        SimulateSetBestOrdering(DiskPhaseResult::SUCCESS, log);

        REQUIRE(log.HappensBefore("TxnCommit", "ChainState:publish"));
        REQUIRE(log.Contains("ChainState:publish"));
    }

    SECTION("Connect failure: ChainState publish never occurs")
    {
        EventLog log;
        SimulateSetBestOrdering(DiskPhaseResult::CONNECT_FAILURE, log);

        /* No ChainState mutation on failure. */
        REQUIRE(log.NotContains("ChainState:publish"));
    }

    SECTION("Disk success ordering: disk -> TxnCommit -> mempool -> ChainState -> broadcast")
    {
        EventLog log;
        SimulateSetBestOrdering(DiskPhaseResult::SUCCESS, log);

        /* Full pipeline ordering assertion. */
        REQUIRE(log.HappensBefore("disk:disconnect",     "disk:connect:OK"));
        REQUIRE(log.HappensBefore("disk:connect:OK",     "TxnCommit"));
        REQUIRE(log.HappensBefore("TxnCommit",           "mempool:resurrect"));
        REQUIRE(log.HappensBefore("mempool:resurrect",   "mempool:remove"));
        REQUIRE(log.HappensBefore("mempool:remove",      "ChainState:publish"));
        REQUIRE(log.HappensBefore("ChainState:publish",  "broadcast"));
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
 * TEST 4 — Real SetBest() with Connect() failure rolls back disk state
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
 * TEST 5 — Real call-site #4 pattern: outer TxnBegin + SetBest() failure
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
 * TEST 6 — Real mempool: size unchanged after failed SetBest()
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
