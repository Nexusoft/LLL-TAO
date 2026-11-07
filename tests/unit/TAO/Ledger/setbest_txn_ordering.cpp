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
 *  3. Checkpoint failure prevents disk commits; partial commit failure retains
 *     journals and fences both further admission and restart.
 *
 * The TxnCommit return-value tests use inline simulation / ordering-assertion
 * infrastructure so that they compile and run without a live LLD database or
 * full chain state — following the same pattern used in
 * validate_vtx_consistency.cpp and filter_mempool_only_predecessor.cpp.
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
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/sector.h>
#include <LLD/types/trust.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/client.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <unit/catch2/catch.hpp>

namespace
{
    /* Failure tests deliberately trip the process-wide restart fence. Only
     * tests may discard this evidence and resume using the fixture databases. */
    struct CommitFailureCleanup
    {
        bool savedShutdown{config::fShutdown.load()};
        ~CommitFailureCleanup()
        {
            if(LLD::Contract) LLD::Contract->TxnRelease();
            if(LLD::Register) LLD::Register->TxnRelease();
            if(LLD::Ledger) LLD::Ledger->TxnRelease();
            if(LLD::Trust) LLD::Trust->TxnRelease();
            if(LLD::Legacy) LLD::Legacy->TxnRelease();
            std::error_code ec;
            std::filesystem::remove(std::filesystem::path(config::GetDataDir()) / "commit-incomplete", ec);
            config::fShutdown.store(savedShutdown);
        }
    };

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


    struct LogicalGuard
    {
        bool ownedLogical{false};

        LogicalGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Logical)
            {
                LLD::Logical = new LLD::LogicalDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLogical = true;
            }
        }

        ~LogicalGuard()
        {
            if(ownedLogical)
            {
                delete LLD::Logical;
                LLD::Logical = nullptr;
            }
        }
    };


    struct ClientGuard
    {
        bool ownedClient{false};

        ClientGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Client)
            {
                LLD::Client = new LLD::ClientDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1000000);
                ownedClient = true;
            }
        }

        ~ClientGuard()
        {
            if(ownedClient)
            {
                delete LLD::Client;
                LLD::Client = nullptr;
            }
        }
    };

} /* anonymous namespace */

TEST_CASE("Sector journal replay preserves writes and indexes after an erase",
    "[ledger][setbest_txn][journal]")
{
    using Database = LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>;
    Database db("journal-replay-regression", LLD::FLAGS::CREATE | LLD::FLAGS::FORCE,
        257, 1024 * 1024);
    const std::string dataKey = "replay-data";
    const std::string flagKey = "replay-flag";
    const std::string indexKey = "replay-index";
    const std::string targetKey = "replay-target";
    struct Cleanup
    {
        Database& db;
        const std::string& dataKey;
        const std::string& flagKey;
        const std::string& indexKey;
        const std::string& targetKey;
        ~Cleanup()
        {
            db.TxnRelease();
            db.Erase(dataKey);
            db.Erase(flagKey);
            db.Erase(indexKey);
            db.Erase(targetKey);
        }
    } cleanup{db, dataKey, flagKey, indexKey, targetKey};

    REQUIRE(db.Write(dataKey, std::string("old")));
    REQUIRE(db.Write(flagKey));
    REQUIRE(db.Write(indexKey, std::string("old-index")));
    REQUIRE(db.Write(targetKey, std::string("target")));
    db.TxnBegin();
    REQUIRE(db.Erase(dataKey));
    REQUIRE(db.Erase(flagKey));
    REQUIRE(db.Erase(indexKey));
    REQUIRE(db.Write(dataKey, std::string("new")));
    REQUIRE(db.Write(flagKey));
    REQUIRE(db.Index(indexKey, targetKey));

    auto CheckState = [&]()
    {
        std::string value;
        REQUIRE(db.Read(dataKey, value));
        REQUIRE(value == "new");
        REQUIRE(db.Exists(flagKey));
        REQUIRE(db.Read(indexKey, value));
        REQUIRE(value == "target");
    };
    CheckState();
    REQUIRE(db.TxnCheckpoint());
    REQUIRE(db.TxnRecovery());
    CheckState();
    REQUIRE(db.TxnCommit());
    CheckState();
}


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


TEST_CASE("HasOpenTransaction checks Logical and Client journals",
          "[lld][txncommit][ownership]")
{
    SECTION("Logical journal")
    {
        LogicalGuard logicalGuard;
        LLD::TransactionGuard txn(0, LLD::INSTANCES::LOGICAL);
        REQUIRE(txn);
        REQUIRE(LLD::HasOpenTransaction(0, LLD::INSTANCES::LOGICAL));

        LLD::TxnAbort(0, LLD::INSTANCES::LOGICAL);
        REQUIRE_FALSE(LLD::HasOpenTransaction(0, LLD::INSTANCES::LOGICAL));
    }

    SECTION("Client journal")
    {
        ClientGuard clientGuard;
        LLD::TransactionGuard txn(0, LLD::INSTANCES::CLIENT);
        REQUIRE(txn);
        REQUIRE(LLD::HasOpenTransaction(0, LLD::INSTANCES::CLIENT));

        LLD::TxnAbort(0, LLD::INSTANCES::CLIENT);
        REQUIRE_FALSE(LLD::HasOpenTransaction(0, LLD::INSTANCES::CLIENT));
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
    CommitFailureCleanup cleanup;

    SECTION("A missing Trust checkpoint stops before any disk commit")
    {
        /* Open both selected transactions, then remove Trust's transaction so
         * its commit fails while Ledger still commits successfully. */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        LLD::Trust->TxnRelease();

        /* Aggregated result must include Trust's commit failure. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        REQUIRE_FALSE(fResult);
        REQUIRE(config::fShutdown.load());
        REQUIRE(LLD::Ledger->HasTransaction());
        REQUIRE_FALSE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));
    }

    SECTION("Ledger commits before an invalid Trust index fails and restart is fenced")
    {
        REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST));
        const std::string missing = "txncommit-missing-index-target";
        REQUIRE_FALSE(LLD::Trust->Exists(missing));
        REQUIRE(LLD::Trust->Index(std::string("txncommit-invalid-index"), missing));

        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        REQUIRE_FALSE(fResult);
        REQUIRE_FALSE(LLD::Ledger->HasTransaction());
        REQUIRE(LLD::Trust->HasTransaction());
        REQUIRE(config::fShutdown.load());
        REQUIRE_FALSE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));
        REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
        LLD::TxnAbort(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST);
        REQUIRE(LLD::Trust->HasTransaction());
        REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
        REQUIRE_THROWS_AS(LLD::TxnRecovery(), std::runtime_error);
        REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
    }
}

TEST_CASE("Journal recovery fail-stops when a replay commit fails",
          "[lld][txncommit][journal]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    CommitFailureCleanup cleanup;
    /* Produce complete consensus checkpoints without a global owner, as on
     * startup. An invalid Trust index fails only when the journal is applied. */
    auto Checkpoint = [](auto* db)
    {
        if(db)
        {
            db->TxnBegin();
            REQUIRE(db->TxnCheckpoint());
        }
    };
    Checkpoint(LLD::Contract);
    Checkpoint(LLD::Register);
    Checkpoint(LLD::Ledger);
    Checkpoint(LLD::Legacy);
    LLD::Trust->TxnBegin();
    REQUIRE_FALSE(LLD::Trust->Exists(std::string("recovery-missing-index-target")));
    REQUIRE(LLD::Trust->Index(std::string("recovery-invalid-index"),
        std::string("recovery-missing-index-target")));
    REQUIRE(LLD::Trust->TxnCheckpoint());
    REQUIRE_THROWS_AS(LLD::TxnRecovery(), std::runtime_error);
    REQUIRE(config::fShutdown.load());
    REQUIRE(std::filesystem::exists(std::filesystem::path(config::GetDataDir()) / "commit-incomplete"));
    REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
    LLD::TxnAbort(0, LLD::INSTANCES::CONSENSUS);
    REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
    REQUIRE_THROWS_AS(LLD::TxnRecovery(), std::runtime_error);
}


/* A narrower commit or abort must not release the owner while journals or
 * overlays from the begin are still open. Wider masks operate only on the
 * instances opened by the matching begin. */
TEST_CASE("LLD commit and abort reject an instance mask narrower than the begin",
          "[lld][txncommit][ownership]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;

    struct OwnerAbort
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fActive{false};

        ~OwnerAbort()
        {
            if(fActive)
                LLD::TxnAbort(nFlags, nInstances);
        }
    };

    SECTION("physical subset commit leaves the owner and the other journal")
    {
        const uint16_t nOpened = LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST;
        OwnerAbort owner{0, nOpened};

        REQUIRE(LLD::TxnBegin(0, nOpened));
        owner.fActive = true;

        REQUIRE_FALSE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::ThreadOwnsTransaction(0));
        REQUIRE(LLD::HasOpenTransaction(0, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::HasOpenTransaction(0, LLD::INSTANCES::TRUST));

        REQUIRE(LLD::TxnCommit(0, nOpened));
        owner.fActive = false;
        REQUIRE_FALSE(LLD::ThreadOwnsTransaction(0));
        REQUIRE_FALSE(LLD::HasOpenTransaction(0, nOpened));

        REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
    }

    SECTION("physical subset abort leaves the owner")
    {
        const uint16_t nOpened = LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST;
        OwnerAbort owner{0, nOpened};

        REQUIRE(LLD::TxnBegin(0, nOpened));
        owner.fActive = true;

        LLD::TxnAbort(0, LLD::INSTANCES::LEDGER);
        REQUIRE(LLD::ThreadOwnsTransaction(0));
        REQUIRE(LLD::HasOpenTransaction(0, LLD::INSTANCES::TRUST));

        LLD::TxnAbort(0, nOpened);
        owner.fActive = false;
        REQUIRE_FALSE(LLD::ThreadOwnsTransaction(0));
        REQUIRE_FALSE(LLD::HasOpenTransaction(0, nOpened));
    }

    SECTION("mempool subset abort and commit leave the owner")
    {
        const uint16_t nOpened = LLD::INSTANCES::MEMORY;
        OwnerAbort owner{TAO::Ledger::FLAGS::MEMPOOL, nOpened};

        REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, nOpened));
        owner.fActive = true;

        LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        REQUIRE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::MEMPOOL));

        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, nOpened));
        owner.fActive = false;
        REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::MEMPOOL));
    }
}


TEST_CASE("LLD owned transaction masks protect unselected database journals",
          "[lld][txncommit][ownership]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;

    struct TrustTxnRelease
    {
        ~TrustTxnRelease()
        {
            if(LLD::Trust && LLD::Trust->HasTransaction())
                LLD::Trust->TxnRelease();
        }
    } cleanup;

    SECTION("a wider commit mask does not commit an unselected Trust journal")
    {
        LLD::Trust->TxnBegin();
        REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));

        REQUIRE(LLD::TxnCommit(0, LLD::INSTANCES::CONSENSUS));
        REQUIRE(LLD::Trust->HasTransaction());
    }

    SECTION("a wider abort mask does not release an unselected Trust journal")
    {
        LLD::Trust->TxnBegin();
        REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));

        LLD::TxnAbort(0, LLD::INSTANCES::CONSENSUS);
        REQUIRE(LLD::Trust->HasTransaction());
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


TEST_CASE("LedgerDB hash-keyed block reads reject mismatched record identity",
          "[lld][ledger][integrity]")
{
    LedgerGuard guard;

    TAO::Ledger::BlockState stored;
    stored.nVersion = 4;
    stored.nChannel = 2;
    stored.nHeight = 7;
    stored.nBits = 1;
    stored.nNonce = std::chrono::steady_clock::now().time_since_epoch().count();

    const uint1024_t hashExpected = stored.GetHash();
    uint1024_t hashWrongKey = hashExpected;
    ++hashWrongKey;
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashWrongKey));

    struct BlockRecordDiskGuard
    {
        uint1024_t hash;

        ~BlockRecordDiskGuard()
        {
            LLD::Ledger->EraseBlock(hash);
        }
    } diskGuard{hashWrongKey};

    REQUIRE(LLD::Ledger->WriteBlock(hashWrongKey, stored));

    TAO::Ledger::BlockState result;
    result.nHeight = 42;
    REQUIRE_FALSE(LLD::Ledger->ReadBlock(hashWrongKey, result));
    REQUIRE(result.nHeight == 42);

    /* Sentinel must differ from the mismatched record. Copying the stored block
     * would hide an overwrite that happens before the identity check fails. */
    TAO::Ledger::BlockState atomicInitial;
    atomicInitial.nHeight = 99;
    atomicInitial.nNonce = 1;
    memory::atomic<TAO::Ledger::BlockState> atomicResult;
    atomicResult.store(atomicInitial);
    REQUIRE_FALSE(LLD::Ledger->ReadBlock(hashWrongKey, atomicResult));
    REQUIRE(atomicResult.load().nHeight == 99);
    REQUIRE(atomicResult.load().nNonce == 1);
}


TEST_CASE("LLD::TxnCommit rejects a memory mode that does not own the transaction",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    /* Ownership is process-wide per mode. A mismatched commit must be rejected
     * without releasing the owner; abort of the owning mode is what releases it. */
    struct OwnerAbort
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fActive{true};

        ~OwnerAbort()
        {
            if(fActive)
                LLD::TxnAbort(nFlags, nInstances);
        }
    } owner{TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER};

    LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));

    /* A mismatched abort must not release the owner or touch database state. */
    LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
    LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER);

    /* Ownership is still held: another mismatched commit is still rejected. */
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));

    LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
    owner.fActive = false;

    /* Released owner: a new memory transaction can begin and commit. */
    LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
}


TEST_CASE("Coexisting MEMPOOL and MINER or SANITIZE transactions can commit",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    struct OwnerAbort
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fActive{false};

        ~OwnerAbort()
        {
            if(fActive)
                LLD::TxnAbort(nFlags, nInstances);
        }
    };

    SECTION("MINER short-circuit succeeds without releasing a coexisting MEMPOOL owner")
    {
        OwnerAbort mempool{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};
        OwnerAbort miner{TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER};

        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        mempool.fActive = true;
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = true;

        const uint512_t hashTx(0x4238406082ULL);
        const uint32_t nContract = 1;
        const uint64_t nClaimed = 42;
        REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, 7, TAO::Ledger::FLAGS::MINER));

        /* Owning both modes must not reject either commit. MINER does not release. */
        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER));
        REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));

        uint64_t nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MINER));
        REQUIRE(nRead == 7);

        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
        mempool.fActive = false;

        nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);

        LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = false;
        nRead = 0;
        REQUIRE_FALSE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MINER));
    }

    SECTION("SANITIZE short-circuit and MEMPOOL commit succeed when begun in either order")
    {
        OwnerAbort mempool{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};
        OwnerAbort sanitize{TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER};

        LLD::TxnBegin(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER);
        sanitize.fActive = true;
        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        mempool.fActive = true;

        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
        mempool.fActive = false;

        LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER);
        sanitize.fActive = false;
    }

    SECTION("another thread cannot short-circuit commit an owned MINER slot")
    {
        OwnerAbort miner{TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = true;

        std::atomic<bool> fRejected{false};
        std::exception_ptr pError;
        std::thread other([&]()
        {
            try
            {
                fRejected.store(!LLD::TxnCommit(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER));
            }
            catch(...)
            {
                pError = std::current_exception();
            }
        });
        other.join();
        if(pError)
            std::rethrow_exception(pError);

        REQUIRE(fRejected.load());
        LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = false;
    }
}


TEST_CASE("Mismatched abort and commit leave another thread's mempool overlay intact",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    const uint512_t hashTx(0x4237889625ULL);
    const uint32_t nContract = 1;
    const uint64_t nClaimed = 77;

    std::atomic<bool> fStaged{false};
    std::atomic<bool> fMismatchedDone{false};
    std::atomic<bool> fError{false};
    std::exception_ptr pError;
    bool fStillStaged = false;
    bool fCommitted = true;

    std::thread mempool([&]()
    {
        try
        {
            LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
            if(!LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                throw std::runtime_error("failed to stage mempool claim");

            fStaged.store(true);
            while(!fMismatchedDone.load())
                std::this_thread::yield();

            uint64_t nRead = 0;
            fStillStaged = LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL)
                && nRead == nClaimed;

            LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);

            nRead = 0;
            fCommitted = LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL);
        }
        catch(...)
        {
            pError = std::current_exception();
            fError.store(true);
            fStaged.store(true);
            fMismatchedDone.store(true);
        }
    });

    while(!fStaged.load())
        std::this_thread::yield();

    struct MinerAbort
    {
        bool fActive{false};

        ~MinerAbort()
        {
            if(fActive)
                LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        }
    } miner;

    struct JoinGuard
    {
        std::thread& tThread;
        std::atomic<bool>& fDone;

        ~JoinGuard()
        {
            fDone.store(true);
            if(tThread.joinable())
                tThread.join();
        }
    } joinGuard{mempool, fMismatchedDone};

    /* pError is written by the worker. Decide before join with the atomic flag,
     * and read the exception pointer only after join. */
    if(!fError.load())
    {
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = true;
        LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
    }

    fMismatchedDone.store(true);
    if(mempool.joinable())
        mempool.join();

    if(pError)
        std::rethrow_exception(pError);

    REQUIRE(fStillStaged);
    REQUIRE_FALSE(fCommitted);
}


TEST_CASE("Unowned thread cannot abort, commit, or replace another thread's mempool overlay",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    const uint512_t hashTx(0x4237997049ULL);
    const uint32_t nContract = 1;
    const uint64_t nClaimed = 91;

    struct OwnerAbort
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fActive{true};

        ~OwnerAbort()
        {
            if(fActive)
                LLD::TxnAbort(nFlags, nInstances);
        }
    } owner{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};

    LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
    REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));

    std::atomic<bool> fCommitRejected{false};
    std::exception_ptr pError;

    std::thread unowned([&]()
    {
        try
        {
            /* Never called TxnBegin. These must not delete, commit, or replace pMemory.
             * BLOCK shares that overlay, so its begin is part of the same check. */
            LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
            fCommitRejected.store(!LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
            LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
            LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
            LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
            LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        }
        catch(...)
        {
            pError = std::current_exception();
        }
    });

    unowned.join();
    if(pError)
        std::rethrow_exception(pError);

    REQUIRE(fCommitRejected.load());

    uint64_t nRead = 0;
    REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
    REQUIRE(nRead == nClaimed);
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));

    LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
    owner.fActive = false;
    nRead = 0;
    REQUIRE_FALSE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
}


TEST_CASE("MEMPOOL and BLOCK begins cannot replace each other's shared overlay",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    struct OwnerAbort
    {
        uint8_t nFlags;
        uint16_t nInstances;
        bool fActive{false};

        ~OwnerAbort()
        {
            if(fActive)
                LLD::TxnAbort(nFlags, nInstances);
        }
    };

    const uint32_t nContract = 1;
    const uint64_t nClaimed = 55;

    SECTION("BLOCK begin does not delete another thread's MEMPOOL overlay")
    {
        const uint512_t hashTx(0x4238195488ULL);
        OwnerAbort owner{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        owner.fActive = true;
        REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));

        std::atomic<bool> fBeginRejected{false};
        std::thread attacker([&]()
        {
            fBeginRejected.store(!LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
            if(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER))
                LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        });
        attacker.join();
        REQUIRE(fBeginRejected.load());

        uint64_t nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    }

    SECTION("MEMPOOL begin does not delete another thread's BLOCK overlay")
    {
        const uint512_t hashTx(0x4238195489ULL);
        OwnerAbort owner{TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        owner.fActive = true;
        REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));

        std::atomic<bool> fBeginRejected{false};
        std::thread attacker([&]()
        {
            fBeginRejected.store(!LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
            LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        });
        attacker.join();
        REQUIRE(fBeginRejected.load());

        uint64_t nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    }

    SECTION("same thread cannot open the second shared mode")
    {
        const uint512_t hashTx(0x4238195490ULL);
        OwnerAbort mempool{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};
        OwnerAbort physical{TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER};

        REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
        mempool.fActive = true;
        REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));

        REQUIRE_FALSE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        physical.fActive = LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);

        uint64_t nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        REQUIRE_FALSE(physical.fActive);
    }

    SECTION("same thread cannot begin an owned mode again")
    {
        const uint512_t hashMempool(0x4238514288ULL);
        const uint512_t hashBlock(0x4238514289ULL);
        const uint512_t hashMiner(0x4238514290ULL);

        OwnerAbort mempool{TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
        mempool.fActive = true;
        REQUIRE(LLD::Ledger->WriteClaimed(hashMempool, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));
        LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);

        uint64_t nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashMempool, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
        mempool.fActive = false;

        OwnerAbort physical{TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        physical.fActive = true;
        REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteClaimed(hashBlock, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));
        LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);

        nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashBlock, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(nRead == nClaimed);
        REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        physical.fActive = false;

        OwnerAbort miner{TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER};
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = true;
        REQUIRE(LLD::Ledger->WriteClaimed(hashMiner, nContract, nClaimed, TAO::Ledger::FLAGS::MINER));
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);

        nRead = 0;
        REQUIRE(LLD::Ledger->ReadClaimed(hashMiner, nContract, nRead, TAO::Ledger::FLAGS::MINER));
        REQUIRE(nRead == nClaimed);
        LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
        miner.fActive = false;
    }
}


TEST_CASE("BLOCK abort and commit reject a caller that does not own the physical slot",
          "[lld][txncommit][ownership]")
{
    LedgerGuard guard;

    struct Cleanup
    {
        bool fMempool{false};

        ~Cleanup()
        {
            if(fMempool)
                LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);

            /* Unowned physical abort is the recovery no-op and releases a journal
             * opened outside the ownership slot. */
            LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
        }
    } cleanup;

    /* Open a journal without claiming PhysicalSlot, as recovery can. */
    LLD::Ledger->TxnBegin();
    REQUIRE(LLD::Ledger->HasTransaction());

    const uint512_t hashTx(0x4238628335ULL);
    const uint32_t nContract = 1;
    const uint64_t nClaimed = 17;
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
    cleanup.fMempool = true;
    REQUIRE(LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL));

    /* Owning MEMPOOL is not physical ownership. Neither call may release or
     * commit the journal, or drop the mempool overlay. */
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
    REQUIRE(LLD::Ledger->HasTransaction());
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->HasTransaction());

    uint64_t nRead = 0;
    REQUIRE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
    REQUIRE(nRead == nClaimed);

    LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);
    cleanup.fMempool = false;

    /* A physical begin must not replace the unowned recovery journal. */
    REQUIRE_FALSE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->HasTransaction());
    REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK));

    /* Recovery abort releases that journal without an owner. */
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
    REQUIRE_FALSE(LLD::Ledger->HasTransaction());

    /* The redundant commit after a completed physical transaction stays false
     * and must not be reported as an ownership error by mutating state. */
    REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::Ledger->HasTransaction());
}


TEST_CASE("Owner thread exit releases the process-wide transaction slot",
          "[lld][txncommit][ownership]")
{
    LedgerGuard guard;

    const uint512_t hashTx(0x4239100840ULL);
    const uint32_t nContract = 1;
    const uint64_t nClaimed = 19;
    std::exception_ptr pError;

    std::thread owner([&]()
    {
        try
        {
            if(!LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER))
                throw std::runtime_error("mempool begin failed");
            if(!LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::MEMPOOL))
                throw std::runtime_error("mempool begin did not own the slot");
            if(!LLD::Ledger->WriteClaimed(hashTx, nContract, nClaimed, TAO::Ledger::FLAGS::MEMPOOL))
                throw std::runtime_error("failed to stage mempool claim");
            /* Exit without abort or commit. The owner guard must release the slot. */
        }
        catch(...)
        {
            pError = std::current_exception();
        }
    });
    owner.join();
    if(pError)
        std::rethrow_exception(pError);

    REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::MEMPOOL));
    uint64_t nRead = 0;
    REQUIRE_FALSE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));

    /* A later begin must not be rejected as foreign, and must not resume the overlay. */
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));
    nRead = 0;
    REQUIRE_FALSE(LLD::Ledger->ReadClaimed(hashTx, nContract, nRead, TAO::Ledger::FLAGS::MEMPOOL));
    LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER);

    std::thread physical([&]()
    {
        try
        {
            if(!LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER))
                throw std::runtime_error("block begin failed");
            if(!LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK))
                throw std::runtime_error("block begin did not own the slot");
            if(!LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER))
                throw std::runtime_error("block begin did not open a journal");
        }
        catch(...)
        {
            pError = std::current_exception();
        }
    });
    physical.join();
    if(pError)
        std::rethrow_exception(pError);

    REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK));
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
}


TEST_CASE("LedgerDB client-mode hash-keyed reads reject mismatched record identity",
          "[lld][ledger][integrity][client]")
{
    LedgerGuard guard;

    struct ClientModeGuard
    {
        bool fPrevious{false};
        LLD::ClientDB* pPrevious{nullptr};
        bool fOwned{false};

        ClientModeGuard()
            : fPrevious(config::fClient.load())
            , pPrevious(LLD::Client)
        {
            config::fClient.store(true);
            if(!LLD::Client)
            {
                /* Match LLD::Initialize(): production ClientDB uses 1000000 buckets.
                 * 77773 is the client-mode LedgerDB count, not ClientDB. */
                LLD::Client = new LLD::ClientDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1000000);
                fOwned = true;
            }
        }

        ~ClientModeGuard()
        {
            config::fClient.store(fPrevious);
            if(fOwned)
            {
                delete LLD::Client;
                LLD::Client = pPrevious;
            }
        }
    } clientGuard;

    TAO::Ledger::ClientBlock stored;
    stored.nVersion = 4;
    stored.nChannel = 2;
    stored.nHeight = 9;
    stored.nBits = 1;
    stored.nNonce = std::chrono::steady_clock::now().time_since_epoch().count();

    const uint1024_t hashExpected = stored.GetHash();
    uint1024_t hashWrongKey = hashExpected;
    ++hashWrongKey;

    struct ClientRecordDiskGuard
    {
        uint1024_t hash;
        bool fHadPrevious{false};
        TAO::Ledger::ClientBlock previous;

        explicit ClientRecordDiskGuard(const uint1024_t& hashIn)
            : hash(hashIn)
        {
            if(LLD::Client)
                fHadPrevious = LLD::Client->ReadBlock(hash, previous);
        }

        ~ClientRecordDiskGuard()
        {
            if(!LLD::Client)
                return;

            if(fHadPrevious)
                LLD::Client->WriteBlock(hash, previous);
            else
                LLD::Client->EraseBlock(hash);
        }
    } diskGuard{hashWrongKey};

    REQUIRE(LLD::Client->WriteBlock(hashWrongKey, stored));

    TAO::Ledger::BlockState result;
    result.nHeight = 42;
    REQUIRE_FALSE(LLD::Ledger->ReadBlock(hashWrongKey, result));
    REQUIRE(result.nHeight == 42);

    TAO::Ledger::BlockState atomicInitial;
    atomicInitial.nHeight = 42;
    memory::atomic<TAO::Ledger::BlockState> atomicResult;
    atomicResult.store(atomicInitial);
    REQUIRE_FALSE(LLD::Ledger->ReadBlock(hashWrongKey, atomicResult));
    REQUIRE(atomicResult.load().nHeight == 42);
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
 *   when the caller has not already done so. Its self-owned guard aborts
 *   failures only when SetBest() opened the transaction; an outer owner remains
 *   responsible for deciding whether to commit or abort its journal.
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


TEST_CASE("Checkpoint hardening preserves an intermediate connected block's timespan boundary",
          "[ledger][setbest_txn][checkpoints][real]")
{
    RealCodeLedgerGuard ledgerGuard;

    struct Cleanup
    {
        uint1024_t savedCheckpoint{TAO::Ledger::ChainState::hashCheckpoint.load()};
        uint32_t savedHeight{TAO::Ledger::ChainState::nCheckpointHeight.load()};
        std::vector<uint1024_t> hashes;

        ~Cleanup()
        {
            TAO::Ledger::ChainState::nCheckpointHeight = savedHeight;
            TAO::Ledger::ChainState::hashCheckpoint = savedCheckpoint;
            for(const auto& hash : hashes)
                LLD::Ledger->EraseBlock(hash);
        }
    } cleanup;

    /* The intermediate parent crosses 30 minutes; the final two parents
     * remain in the same minute, so checking only the final parent misses it. */
    std::vector<TAO::Ledger::BlockState> blocks(5);
    const uint64_t times[] = {100000, 101799, 101800, 101810, 101820};
    for(size_t i = 0; i < blocks.size(); ++i)
    {
        auto& state = blocks[i];
        state.nVersion = 7;
        state.nChannel = 2;
        state.nHeight = i;
        state.nBits = 1;
        state.nNonce = 46130 + i;
        state.nTime = times[i];
        if(i > 0)
        {
            state.hashPrevBlock = blocks[i - 1].GetHash();
            state.hashCheckpoint = blocks[0].GetHash();
        }

        const uint1024_t hash = state.GetHash();
        REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));
        cleanup.hashes.push_back(hash);
        REQUIRE(LLD::Ledger->WriteBlock(hash, state));
    }

    TAO::Ledger::ChainState::hashCheckpoint = blocks[1].GetHash();
    TAO::Ledger::ChainState::nCheckpointHeight = blocks[1].nHeight;
    REQUIRE(TAO::Ledger::IsNewTimespan(blocks[2]));
    REQUIRE_FALSE(TAO::Ledger::HardenCheckpoint(blocks.back().Prev()));
    REQUIRE(TAO::Ledger::ChainState::hashCheckpoint.load() == blocks[1].GetHash());

    /* SetBest stores vConnect in descending order and visits every parent
     * in ascending connection order after the chain transaction commits. */
    std::vector<TAO::Ledger::BlockState> vConnect{blocks[4], blocks[3], blocks[2]};
    TAO::Ledger::HardenConnectedCheckpoints(vConnect);
    REQUIRE(TAO::Ledger::ChainState::hashCheckpoint.load() == blocks[0].GetHash());
    REQUIRE(TAO::Ledger::ChainState::nCheckpointHeight.load() == blocks[0].nHeight);
}


TEST_CASE("SetBest publishes the staged block state on extension and rewind",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard chainGuard;

    uint1024_t savedBest;
    REQUIRE(LLD::Ledger->ReadBestChain(savedBest));

    TAO::Ledger::BlockState genesis;
    genesis.nVersion = 4;
    genesis.nChannel = 2;
    genesis.nHeight = 0;
    genesis.nBits = 1;
    genesis.nNonce = 46119;
    genesis.nMoneySupply = 100000;
    const uint1024_t hashGenesis = genesis.GetHash();

    TAO::Ledger::BlockState candidate;
    candidate.nVersion = 4;
    candidate.hashPrevBlock = hashGenesis;
    candidate.nChannel = 2;
    candidate.nHeight = 1;
    candidate.nBits = 1;
    candidate.nNonce = 46120;
    candidate.nMint = 25;
    candidate.nChainTrust = 1;
    candidate.hashNextBlock = uint1024_t(0x46122);
    const uint1024_t hashCandidate = candidate.GetHash();

    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashGenesis));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashCandidate));
    struct Cleanup
    {
        uint1024_t savedBest;
        uint1024_t genesis;
        uint1024_t candidate;
        ~Cleanup()
        {
            LLD::Ledger->WriteBestChain(savedBest);
            LLD::Ledger->EraseBlock(candidate);
            LLD::Ledger->EraseBlock(genesis);
        }
    } cleanup{savedBest, hashGenesis, hashCandidate};

    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    TAO::Ledger::ChainState::tStateGenesis = genesis;
    TAO::Ledger::ChainState::tStateBest = genesis;
    TAO::Ledger::ChainState::hashBestChain = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight.store(0);
    TAO::Ledger::ChainState::nBestChainTrust.store(0);

    REQUIRE(candidate.SetBest());
    TAO::Ledger::BlockState stored;
    REQUIRE(LLD::Ledger->ReadBlock(hashCandidate, stored));
    REQUIRE(stored.nMoneySupply == 100025);
    REQUIRE(stored.hashNextBlock == 0);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().nMoneySupply == stored.nMoneySupply);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().hashNextBlock == 0);
    REQUIRE(candidate.nMoneySupply == stored.nMoneySupply);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == 1);
    uint1024_t persistedBest;
    REQUIRE(LLD::Ledger->ReadBestChain(persistedBest));
    REQUIRE(persistedBest == hashCandidate);

    TAO::Ledger::BlockState bad;
    bad.nVersion = 4;
    bad.hashPrevBlock = hashCandidate;
    bad.nChannel = 2;
    bad.nHeight = 2;
    bad.nBits = 1;
    bad.nNonce = 46121;
    bad.vtx.push_back({TAO::Ledger::TRANSACTION::TRITIUM, uint512_t(0x46121)});
    REQUIRE_FALSE(bad.SetBest());
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
    REQUIRE(LLD::Ledger->ReadBestChain(persistedBest));
    REQUIRE(persistedBest == hashCandidate);

    REQUIRE(LLD::Ledger->ReadBlock(hashGenesis, genesis));
    REQUIRE(genesis.hashNextBlock == hashCandidate);
    REQUIRE(genesis.SetBest());
    REQUIRE(LLD::Ledger->ReadBlock(hashGenesis, stored));
    REQUIRE(stored.hashNextBlock == 0);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().hashNextBlock == 0);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == 0);
    REQUIRE(LLD::Ledger->ReadBestChain(persistedBest));
    REQUIRE(persistedBest == hashGenesis);
}

TEST_CASE("Real SetBest fail-stops after Ledger commits but Trust fails",
          "[ledger][setbest_txn][real][commit-failure]")
{
    RealCodeLedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    ChainStateGuard chainGuard;
    CommitFailureCleanup failureCleanup;

    uint1024_t savedBest;
    REQUIRE(LLD::Ledger->ReadBestChain(savedBest));
    TAO::Ledger::BlockState genesis;
    genesis.nVersion = 4;
    genesis.nChannel = 2;
    genesis.nBits = 1;
    genesis.nNonce = 46201;
    const uint1024_t hashGenesis = genesis.GetHash();
    TAO::Ledger::BlockState candidate;
    candidate.nVersion = 4;
    candidate.nChannel = 2;
    candidate.nBits = 1;
    candidate.nNonce = 46202;
    candidate.nHeight = 1;
    candidate.nChainTrust = 1;
    candidate.hashPrevBlock = hashGenesis;
    const uint1024_t hashCandidate = candidate.GetHash();
    struct DiskCleanup
    {
        uint1024_t savedBest, genesis, candidate;
        ~DiskCleanup()
        {
            LLD::Ledger->WriteBestChain(savedBest);
            LLD::Ledger->EraseBlock(genesis);
            LLD::Ledger->EraseBlock(candidate);
        }
    } diskCleanup{savedBest, hashGenesis, hashCandidate};
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    REQUIRE(LLD::Ledger->WriteBestChain(hashGenesis));
    TAO::Ledger::ChainState::tStateGenesis = genesis;
    TAO::Ledger::ChainState::tStateBest = genesis;
    TAO::Ledger::ChainState::hashBestChain = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight = 0;
    TAO::Ledger::ChainState::nBestChainTrust = 0;

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));
    REQUIRE_FALSE(LLD::Trust->Exists(std::string("setbest-missing-index-target")));
    REQUIRE(LLD::Trust->Index(std::string("setbest-invalid-index"),
        std::string("setbest-missing-index-target")));
    const auto mempoolSize = TAO::Ledger::mempool.Size();
    REQUIRE_FALSE(candidate.SetBest());
    REQUIRE(config::fShutdown.load());
    REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK));
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == 0);
    REQUIRE(TAO::Ledger::mempool.Size() == mempoolSize);
    uint1024_t persistedBest;
    REQUIRE(LLD::Ledger->ReadBestChain(persistedBest));
    REQUIRE(persistedBest == hashCandidate);
    REQUIRE_FALSE(genesis.SetBest());
    REQUIRE_FALSE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));
    REQUIRE_FALSE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL));
    REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
    REQUIRE(LLD::Trust->HasTransaction());
    REQUIRE_THROWS_AS(LLD::TxnRecovery(), std::runtime_error);
    REQUIRE(filesystem::size(config::GetDataDir() + "_LEDGER/journal.dat") > 0);
    REQUIRE_THROWS_AS(LLD::Initialize(), std::runtime_error);
    /* Even absent journals cannot make a partial commit safe to accept. */
    LLD::Ledger->TxnRelease();
    REQUIRE_THROWS_AS(LLD::TxnRecovery(), std::runtime_error);
}


/* ===========================================================================
 * TEST 5 — Real SetBest() with Connect() failure rolls back disk index
 * ===========================================================================
 * Calls the actual BlockState::SetBest().  The candidate block's vtx contains
 * a transaction hash that is NOT on disk, so Connect() fails mid-loop.  With
 *  the self-owned guard aborts before returning false, rolling back the
 *  IndexBlock write that Connect() made before detecting the missing
 *  transaction. Asserts: (a) SetBest() returns false, (b) no index entry was
 *  committed to disk (the guard rolled it back), (c) ChainState atomics are
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
     * SetBest()'s self-owned guard then aborts, discarding the pending IndexBlock
     * write. */
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
     * The self-owned guard deleted pTransaction before it could be flushed to disk.
     * HasIndex() checks the on-disk keychain only (pTransaction is null),
     * so a false result confirms the write was discarded. */
    REQUIRE_FALSE(LLD::Ledger->HasIndex(fakeTxHash));

    /* ---- (c) No active transaction remains open ---- */
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashGenesis);
}


TEST_CASE("Real SetBest(): rejects partial outer consensus transaction",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;

    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 79;

    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));

    TAO::Ledger::ChainState::tStateGenesis   = genesis;
    TAO::Ledger::ChainState::tStateBest      = genesis;
    TAO::Ledger::ChainState::hashBestChain   = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight     .store(0);
    TAO::Ledger::ChainState::nBestChainTrust .store(genesis.nChainTrust);

    const uint512_t fakeTxHash(0xfaceb00cULL);
    TAO::Ledger::BlockState candidate;
    candidate.nVersion      = 4;
    candidate.hashPrevBlock = hashGenesis;
    candidate.nChannel      = 2;
    candidate.nHeight       = 1;
    candidate.nBits         = 1;
    candidate.nNonce        = 56;
    candidate.vtx.push_back({TAO::Ledger::TRANSACTION::TRITIUM, fakeTxHash});

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(candidate.SetBest());

    REQUIRE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK));
    REQUIRE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::ThreadOwnsTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));
    REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));

    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER);

    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE_FALSE(LLD::Ledger->HasIndex(fakeTxHash));
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashGenesis);

    LLD::Ledger->EraseBlock(hashGenesis);
}


/* ===========================================================================
 * TEST 6 — Real call-site #4 pattern: guarded SetBest() failure
 * ===========================================================================
 * Mirrors the call-site #4 pattern in chainstate.cpp:
 *   LLD::TransactionGuard txn;
 *   state.SetBest();
 *
 * SetBest() leaves an outer transaction open when it fails, so the caller's
 * guard remains responsible for rollback. This test verifies:
 *   (a) SetBest() returns false,
 *   (b) no index entry persists after guard cleanup,
 *   (c) no active transaction remains open.
 */
TEST_CASE("Real call-site #4: guarded SetBest() failure → clean abort, no partial commit",
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
    bool fOk = false;
    {
        LLD::TransactionGuard txn;
        REQUIRE(txn);
        fOk = badBlock.SetBest();
        REQUIRE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));
    }

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
