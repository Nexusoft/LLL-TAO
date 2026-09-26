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
 *  3. No participant is applied unless every selected journal reaches its
 *     checkpoint, preventing a missing participant from producing a partial commit.
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
#include <LLD/include/version.h>
#include <LLD/types/contract.h>
#include <LLD/types/register.h>
#include <LLD/types/legacy.h>
#include <LLD/types/trust.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/cache.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/genesis_block.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/sync_profile.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/client.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <Util/templates/datastream.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <future>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32
#include <csignal>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

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


    struct LogicalGuard
    {
        bool ownedLogical{false};

        LogicalGuard()
        {
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
            if(!LLD::Client)
            {
                LLD::Client = new LLD::ClientDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
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


    struct ClientModeGuard
    {
        bool savedClient;
        bool savedHybrid;
        bool savedTestNet;

        ClientModeGuard()
        : savedClient(config::fClient.load())
        , savedHybrid(config::fHybrid.load())
        , savedTestNet(config::fTestNet.load())
        {
            config::fClient.store(true);
            config::fHybrid.store(false);
            config::fTestNet.store(false);
        }

        ~ClientModeGuard()
        {
            config::fClient.store(savedClient);
            config::fHybrid.store(savedHybrid);
            config::fTestNet.store(savedTestNet);
        }
    };


    struct ContractGuard
    {
        bool ownedContract{false};

        ContractGuard()
        {
            if(!LLD::Contract)
            {
                LLD::Contract = new LLD::ContractDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedContract = true;
            }
        }

        ~ContractGuard()
        {
            if(ownedContract)
            {
                delete LLD::Contract;
                LLD::Contract = nullptr;
            }
        }
    };


    struct RegisterGuard
    {
        bool ownedRegister{false};

        RegisterGuard()
        {
            if(!LLD::Register)
            {
                LLD::Register = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedRegister = true;
            }
        }

        ~RegisterGuard()
        {
            if(ownedRegister)
            {
                delete LLD::Register;
                LLD::Register = nullptr;
            }
        }
    };


    struct LegacyGuard
    {
        bool ownedLegacy{false};

        LegacyGuard()
        {
            if(!LLD::Legacy)
            {
                LLD::Legacy = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLegacy = true;
            }
        }

        ~LegacyGuard()
        {
            if(ownedLegacy)
            {
                delete LLD::Legacy;
                LLD::Legacy = nullptr;
            }
        }
    };


    /* Write a complete recovery journal with a durable commit marker. */
    bool WriteRecoveryJournal(const std::string& strName, const DataStream& ssJournal)
    {
        const std::string strPath =
            debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat");

        FILE* stream = std::fopen(strPath.c_str(), "wb");
        if(!stream)
            return false;

        const std::vector<uint8_t>& vBytes = ssJournal.Bytes();
        const bool fWrote =
            std::fwrite(vBytes.data(), 1, vBytes.size(), stream) == vBytes.size()
            && std::fflush(stream) == 0;

        return (std::fclose(stream) == 0 && fWrote);
    }


    /* Build a journal that applies a simple key/value write. */
    DataStream MakeWriteJournal(const std::pair<std::string, uint32_t>& key,
                                const uint32_t nValue, const bool fCommit = true)
    {
        DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
        ssKey << key;

        DataStream ssData(SER_LLD, LLD::DATABASE_VERSION);
        ssData << std::string("NONE");
        ssData << nValue;

        DataStream ssJournal(SER_LLD, LLD::DATABASE_VERSION);
        ssJournal << std::string("write") << ssKey.Bytes() << ssData.Bytes();
        if(fCommit)
            ssJournal << std::string("commit");
        return ssJournal;
    }


    /* Build a journal whose apply path fails while still reaching commit. */
    DataStream MakeFailingIndexJournal()
    {
        const std::vector<uint8_t> vKey = { 0x01, 0x02, 0x03, 0x04 };
        const std::vector<uint8_t> vMissing = { 0xde, 0xad, 0xbe, 0xef };

        DataStream ssJournal(SER_LLD, LLD::DATABASE_VERSION);
        ssJournal << std::string("index") << vKey << vMissing;
        ssJournal << std::string("commit");
        return ssJournal;
    }


    /* Build a journal containing an invalid entry type. */
    DataStream MakeInvalidRecoveryJournal()
    {
        DataStream ssJournal(SER_LLD, LLD::DATABASE_VERSION);
        ssJournal << std::string("invalid");
        return ssJournal;
    }


    uint64_t JournalSize(const std::string& strName)
    {
        const std::string strPath =
            debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat");

        FILE* stream = std::fopen(strPath.c_str(), "rb");
        if(!stream)
            return 0;

        if(std::fseek(stream, 0, SEEK_END) != 0)
        {
            std::fclose(stream);
            return 0;
        }

        const long nSize = std::ftell(stream);
        std::fclose(stream);
        return nSize > 0 ? static_cast<uint64_t>(nSize) : 0;
    }


    struct SyncProfileGuard
    {
        const std::map<std::string, std::string> savedArgs = config::mapArgs;
        const std::map<std::string, std::vector<std::string>> savedMultiArgs = config::mapMultiArgs;

        SyncProfileGuard()
        {
            config::mapArgs["-syncprofile"] = "1";
            TAO::Ledger::SyncProfile::Reset();
        }

        ~SyncProfileGuard()
        {
            TAO::Ledger::SyncProfile::Reset();
            config::mapArgs = savedArgs;
            config::mapMultiArgs = savedMultiArgs;
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


TEST_CASE("LLD::TxnCommit applies every instance owned by the transaction",
          "[lld][txncommit]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;

    const std::pair<std::string, uint32_t> ledgerKey =
        std::make_pair(std::string("txn-owned-ledger"), 1);
    const std::pair<std::string, uint32_t> trustKey =
        std::make_pair(std::string("txn-owned-trust"), 1);

    LLD::Ledger->Erase(ledgerKey);
    LLD::Trust->Erase(trustKey);

    REQUIRE(LLD::TxnBegin(
        0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST));
    REQUIRE(LLD::Ledger->Write(ledgerKey, uint32_t(42)));
    REQUIRE(LLD::Trust->Write(trustKey, uint32_t(43)));

    REQUIRE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->Exists(ledgerKey));
    REQUIRE(LLD::Trust->Exists(trustKey));

    LLD::Ledger->Erase(ledgerKey);
    LLD::Trust->Erase(trustKey);
}


TEST_CASE("LLD::TxnBegin opens every crash-recovery participant",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;

    REQUIRE(LLD::TxnBegin(0, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->HasTransaction());
    REQUIRE(LLD::Trust->HasTransaction());
    REQUIRE(LLD::TxnCommit(0, LLD::INSTANCES::LEDGER));
}


TEST_CASE("Sync profile skips empty applies but preserves every recovery marker",
          "[lld][txncommit][syncprofile]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    SyncProfileGuard syncProfileGuard;

    const auto keyLedger = std::make_pair(std::string("syncprofile-ledger-only"), 1u);
    const uint64_t nTrustJournalBefore = JournalSize("_TRUST");

    LLD::Ledger->Erase(keyLedger);

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->HasTransaction());
    REQUIRE(LLD::Trust->HasTransaction());
    REQUIRE(LLD::Ledger->Write(keyLedger, uint32_t(99)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));

    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnOpenedParticipants == 5);
    REQUIRE(snapshot.nTxnTouchedParticipants == 1);
    REQUIRE(snapshot.nTxnCheckpointParticipants == 5);
    REQUIRE(snapshot.nTxnApplyParticipants == 1);
    REQUIRE(snapshot.nTxnReleaseParticipants == 5);
    REQUIRE(JournalSize("_TRUST") == nTrustJournalBefore);

    LLD::Ledger->Erase(keyLedger);
}


TEST_CASE("Sync profile parses bare switches and explicit intervals",
          "[ledger][syncprofile]")
{
    SyncProfileGuard syncProfileGuard;
    config::mapArgs.erase("-syncprofile");
    config::mapMultiArgs.erase("-syncprofile");
    REQUIRE_FALSE(TAO::Ledger::SyncProfile::Enabled());

    const auto option = GENERATE("-syncprofile", "-syncprofile=", "-syncprofile=3",
                                 "-syncprofile=0", "-syncprofile=-1");
    const char* argv[] = {"nexus", option};
    config::ParseParameters(2, argv);

    const bool enabled = std::string(option) != "-syncprofile=0"
                      && std::string(option) != "-syncprofile=-1";
    REQUIRE(TAO::Ledger::SyncProfile::Enabled() == enabled);
    TAO::Ledger::SyncProfile::RecordBlockReceived();
    REQUIRE(TAO::Ledger::SyncProfile::GetSnapshot().nBlocksReceived == (enabled ? 1 : 0));
}


TEST_CASE("Wholly empty transactions leave every participant journal untouched",
          "[lld][txncommit][syncprofile]")
{
    ClientGuard clientGuard;
    SyncProfileGuard syncProfileGuard;
    const bool fClient = GENERATE(false, true);
    const uint16_t nInstances = fClient ? LLD::INSTANCES::MERKLE : LLD::INSTANCES::CONSENSUS;
    const std::vector<std::string> names = fClient
        ? std::vector<std::string>{"_CONTRACT", "_REGISTER", "_CLIENT", "_API"}
        : std::vector<std::string>{"_CONTRACT", "_REGISTER", "_LEDGER", "_TRUST", "_LEGACY"};
    std::map<std::string, std::filesystem::file_time_type> times;
    for(const auto& name : names)
    {
        const auto path = config::GetDataDir() + name + "/journal.dat";
        if(std::filesystem::exists(path))
            times[name] = std::filesystem::last_write_time(path);
    }

    for(uint32_t n = 0; n < 8; ++n)
    {
        LLD::TransactionGuard transaction(TAO::Ledger::FLAGS::BLOCK, nInstances);
        REQUIRE(transaction);
        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, nInstances));
        REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, nInstances));
    }

    for(const auto& name : names)
    {
        const auto path = config::GetDataDir() + name + "/journal.dat";
        if(times.count(name))
            REQUIRE(std::filesystem::last_write_time(path) == times.at(name));
        else
            REQUIRE_FALSE(std::filesystem::exists(path));
    }
    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnOpenedParticipants == names.size() * 8);
    REQUIRE(snapshot.nTxnTouchedParticipants == 0);
    REQUIRE(snapshot.nTxnCheckpointParticipants == 0);
    REQUIRE(snapshot.nTxnApplyParticipants == 0);
    REQUIRE(snapshot.nTxnReleaseParticipants == 0);
}


TEST_CASE("Empty physical commits still publish memory-only changes",
          "[lld][txncommit][syncprofile]")
{
    SyncProfileGuard profileGuard;
    const uint256_t address(0x737465);
    TAO::Register::State state;
    REQUIRE(LLD::Register->WriteState(address, state, TAO::Ledger::FLAGS::MEMPOOL));
    struct StateGuard
    {
        uint256_t address;
        ~StateGuard()
        {
            LLD::Register->WriteState(address, TAO::Register::State(), TAO::Ledger::FLAGS::ERASE);
        }
    } guard{address};
    REQUIRE(LLD::Register->ReadState(address, state, TAO::Ledger::FLAGS::MEMPOOL));
    LLD::TransactionGuard transaction;
    REQUIRE(transaction);
    REQUIRE(LLD::Register->WriteState(address, state, TAO::Ledger::FLAGS::ERASE));
    REQUIRE_FALSE(LLD::Register->HasPendingTransactionWork());
    REQUIRE(LLD::TxnCommit());
    REQUIRE_FALSE(LLD::Register->ReadState(address, state, TAO::Ledger::FLAGS::MEMPOOL));
    REQUIRE(TAO::Ledger::SyncProfile::GetSnapshot().nTxnCheckpointParticipants == 0);
    REQUIRE(TAO::Ledger::SyncProfile::GetSnapshot().nTxnReleaseParticipants == 0);
}


TEST_CASE("Sync profile counts only explicitly sync-originated Process deliveries",
          "[ledger][syncprofile]")
{
    LedgerGuard ledgerGuard;
    SyncProfileGuard syncProfileGuard;
    const bool fSyncOrigin = GENERATE(false, true);
    const bool fSkipCheck = GENERATE(false, true);

    struct ProfileBlock : TAO::Ledger::Block
    {
        bool fAccept = true;

        bool Check(bool = false) const override { return true; }
        bool Accept() const override { return fAccept; }
    } block;
    block.hashPrevBlock = TAO::Ledger::ChainState::hashBestChain.load();
    block.nNonce = 707;
    REQUIRE(LLD::Ledger->HasBlock(block.hashPrevBlock));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(block.GetHash()));

    uint8_t expectedStatus = 0;
    SECTION("accepted") { expectedStatus = TAO::Ledger::PROCESS::ACCEPTED; }
    SECTION("rejected")
    {
        block.fAccept = false;
        expectedStatus = TAO::Ledger::PROCESS::REJECTED;
    }
    SECTION("orphaned")
    {
        block.hashPrevBlock = uint1024_t(707);
        REQUIRE_FALSE(LLD::Ledger->HasBlock(block.hashPrevBlock));
        expectedStatus = TAO::Ledger::PROCESS::ORPHAN;
    }

    struct OrphanGuard
    {
        uint1024_t hash;
        ~OrphanGuard()
        {
            std::lock_guard<std::mutex> lock(TAO::Ledger::PROCESSING_MUTEX);
            TAO::Ledger::mapOrphans.Remove(hash);
        }
    } orphanGuard{block.GetHash()};

    uint8_t nStatus = 0;
    if(fSyncOrigin)
        TAO::Ledger::Process(block, nStatus, nullptr, fSkipCheck, true);
    else
        TAO::Ledger::Process(block, nStatus, nullptr, fSkipCheck);

    REQUIRE(nStatus == expectedStatus);
    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nBlocksReceived == (fSyncOrigin ? 1 : 0));
    REQUIRE(snapshot.nBlocksAccepted == (fSyncOrigin && (nStatus & TAO::Ledger::PROCESS::ACCEPTED) ? 1 : 0));
    REQUIRE(snapshot.nBlocksRejected == (fSyncOrigin && (nStatus & TAO::Ledger::PROCESS::REJECTED) ? 1 : 0));
    REQUIRE(snapshot.nBlocksOrphaned == (fSyncOrigin && (nStatus & TAO::Ledger::PROCESS::ORPHAN) ? 1 : 0));
}


TEST_CASE("Sync profile counts every touched consensus participant",
          "[lld][txncommit][syncprofile]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    SyncProfileGuard syncProfileGuard;

    const auto keyLedger = std::make_pair(std::string("syncprofile-ledger"), 2u);
    const auto keyTrust  = std::make_pair(std::string("syncprofile-trust"), 2u);

    LLD::Ledger->Erase(keyLedger);
    LLD::Trust->Erase(keyTrust);

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST));
    REQUIRE(LLD::Ledger->Write(keyLedger, uint32_t(100)));
    REQUIRE(LLD::Trust->Write(keyTrust, uint32_t(101)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER | LLD::INSTANCES::TRUST));

    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnOpenedParticipants == 5);
    REQUIRE(snapshot.nTxnTouchedParticipants == 2);
    REQUIRE(snapshot.nTxnCheckpointParticipants == 5);
    REQUIRE(snapshot.nTxnApplyParticipants == 2);
    REQUIRE(snapshot.nTxnReleaseParticipants == 5);
    REQUIRE(LLD::Ledger->Exists(keyLedger));
    REQUIRE(LLD::Trust->Exists(keyTrust));

    LLD::Ledger->Erase(keyLedger);
    LLD::Trust->Erase(keyTrust);
}


TEST_CASE("Disabled sync profiling leaves transaction counters and timers unchanged",
          "[lld][txncommit][syncprofile]")
{
    LedgerGuard ledgerGuard;
    SyncProfileGuard syncProfileGuard;
    config::mapArgs.erase("-syncprofile");
    REQUIRE_FALSE(TAO::Ledger::SyncProfile::Enabled());
    const auto key = std::make_pair(std::string("syncprofile-disabled"), 1u);
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->Write(key, uint32_t(1)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnOpenedParticipants == 0);
    REQUIRE(snapshot.nTxnTouchedParticipants == 0);
    REQUIRE(snapshot.nTxnCoordinatorWaitUs == 0);
    REQUIRE(snapshot.nTxnCheckpointUs == 0);
    REQUIRE(snapshot.nTxnApplyUs == 0);
    REQUIRE(snapshot.nTxnReleaseUs == 0);
    LLD::Ledger->Erase(key);
}


TEST_CASE("Sync profile avoids empty applies for sequential ledger-only commits",
          "[lld][txncommit][syncprofile]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    SyncProfileGuard syncProfileGuard;

    constexpr uint32_t nIterations = 8;
    for(uint32_t n = 0; n < nIterations; ++n)
    {
        const auto keyLedger = std::make_pair(std::string("syncprofile-batch"), n);
        LLD::Ledger->Erase(keyLedger);

        REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->Write(keyLedger, n + 1));
        REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
        LLD::Ledger->Erase(keyLedger);
    }

    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnOpenedParticipants == 5 * nIterations);
    REQUIRE(snapshot.nTxnTouchedParticipants == nIterations);
    REQUIRE(snapshot.nTxnCheckpointParticipants == 5 * nIterations);
    REQUIRE(snapshot.nTxnApplyParticipants == nIterations);
    REQUIRE(snapshot.nTxnReleaseParticipants == 5 * nIterations);
}


/* ===========================================================================
 * TEST 3 — Checkpoint barrier: an unowned participant aborts every participant
 * ===========================================================================
 * We begin a consensus transaction on Ledger, then attempt to commit Logical
 * as well. Logical belongs to the MERKLE recovery group and therefore has no
 * transaction, so the coordinator must abort before applying Ledger.
 */
TEST_CASE("LLD::TxnCommit checkpoint barrier prevents partial apply",
          "[lld][txncommit]")
{
    LedgerGuard ledgerGuard;
    LogicalGuard logicalGuard;

    SECTION("Logical has no transaction → overall false")
    {
        /* Open a transaction only on Ledger. */
        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);

        /* Logical has no active transaction, so the checkpoint set is incomplete. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::LOGICAL);
        REQUIRE_FALSE(fResult);
    }

    SECTION("Ledger data is not applied when Logical cannot checkpoint")
    {
        const std::pair<std::string, uint32_t> keyTest =
            std::make_pair(std::string("txn-checkpoint-barrier"), 1);
        LLD::Ledger->Erase(keyTest);

        LLD::TxnBegin(0, LLD::INSTANCES::LEDGER);
        const bool fWrite = LLD::Ledger->Write(keyTest, uint32_t(42));

        /* Logical has no transaction, so no selected database may be applied. */
        const bool fResult = LLD::TxnCommit(0, LLD::INSTANCES::LEDGER | LLD::INSTANCES::LOGICAL);
        const bool fApplied = LLD::Ledger->Exists(keyTest);

        REQUIRE(fWrite);
        REQUIRE_FALSE(fResult);
        REQUIRE_FALSE(fApplied);

        LLD::Ledger->Erase(keyTest);
    }
}


TEST_CASE("LLD transaction coordinator serializes MINER and SANITIZE overlays",
          "[lld][txncommit][concurrency]")
{
    LedgerGuard guard;

    std::mutex mutex;
    std::condition_variable condition;
    bool fContenderWaiting = false;
    std::atomic<bool> fContenderAcquired{false};

    LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
    LLD::SetTxnCoordinatorWaitHook([&]()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            fContenderWaiting = true;
        }
        condition.notify_one();
    });

    std::thread contender([&]()
    {
        const bool fAcquired =
            LLD::TxnBegin(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER);
        fContenderAcquired.store(fAcquired);
        if(fAcquired)
            LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER);
    });

    bool fSawContenderWaiting = false;
    {
        std::unique_lock<std::mutex> lock(mutex);
        fSawContenderWaiting = condition.wait_for(lock, std::chrono::seconds(2),
            [&](){ return fContenderWaiting; });
    }

    const bool fAcquiredBeforeRelease = fContenderAcquired.load();

    LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
    if(contender.joinable())
        contender.join();
    LLD::SetTxnCoordinatorWaitHook({});

    REQUIRE(fSawContenderWaiting);
    REQUIRE_FALSE(fAcquiredBeforeRelease);
    REQUIRE(fContenderAcquired.load());
}


TEST_CASE("Mempool entrypoints only wait for transactional work without holding the mempool mutex",
          "[lld][txncommit][concurrency][mempool]")
{
    LedgerGuard guard;
    TAO::Ledger::Mempool pool;
    const uint8_t nFlags = GENERATE(
        TAO::Ledger::FLAGS::BLOCK, TAO::Ledger::FLAGS::MINER, TAO::Ledger::FLAGS::SANITIZE);
    const uint32_t nOperation = GENERATE(0u, 1u, 2u);
    REQUIRE(LLD::TxnBegin(nFlags, LLD::INSTANCES::LEDGER));
    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    LLD::SetTxnCoordinatorWaitHook([&]()
    {
        if(!notified.exchange(true))
            waiting.set_value();
    });

    auto contender = std::async(std::launch::async, [&]()
    {
        if(nOperation == 0)
            pool.Accept(TAO::Ledger::Transaction());
        else if(nOperation == 1)
            pool.ProcessOrphans(uint512_t(0xCAFF01));
        else
            pool.Check();
    });
    const bool fReachedExpectedPoint = (nOperation == 2
        ? waitFuture.wait_for(std::chrono::seconds(2))
        : contender.wait_for(std::chrono::seconds(2))) == std::future_status::ready;
    const bool fWaiting = waitFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
    auto reader = std::async(std::launch::async, [&]() { return pool.Has(uint512_t(0xCAFF02)); });
    const bool fReadWhileOwned = reader.wait_for(std::chrono::seconds(2)) == std::future_status::ready;

    LLD::TxnAbort(nFlags, LLD::INSTANCES::LEDGER);
    contender.get();
    const bool fFound = reader.get();
    LLD::SetTxnCoordinatorWaitHook({});
    REQUIRE(fReachedExpectedPoint);
    REQUIRE(fWaiting == (nOperation == 2));
    REQUIRE(fReadWhileOwned);
    REQUIRE_FALSE(fFound);
}


TEST_CASE("Coordinator reservation survives recursive guards and transaction release",
          "[lld][txncommit][concurrency][mempool]")
{
    LedgerGuard guard;
    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::future<bool> contender;
    bool fWaiting = false;
    bool fBlockedAfterCommit = false;
    {
        LLD::TransactionCoordinatorGuard outer;
        {
            LLD::TransactionCoordinatorGuard inner;
            REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY));
            REQUIRE_FALSE(LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY));
            REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::MEMORY));
        }
        LLD::SetTxnCoordinatorWaitHook([&]() { waiting.set_value(); });
        contender = std::async(std::launch::async, [&]()
        {
            const bool fAcquired = LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
            if(fAcquired)
                LLD::TxnAbort(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER);
            return fAcquired;
        });
        fWaiting = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        fBlockedAfterCommit = contender.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout;
    }
    const bool fAcquired = contender.get();
    LLD::SetTxnCoordinatorWaitHook({});
    REQUIRE(fWaiting);
    REQUIRE(fBlockedAfterCommit);
    REQUIRE(fAcquired);
}


TEST_CASE("LLD::TxnAbort mode mismatch releases coordinator ownership",
          "[lld][txncommit][coordinator]")
{
    LedgerGuard guard;

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::MINER, LLD::INSTANCES::LEDGER));

    /* Mismatched flags must not strand coordinator ownership. */
    REQUIRE(LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));

    /* If ownership leaked, this begin would fail as nested/blocked. */
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::TxnAbort(TAO::Ledger::FLAGS::SANITIZE, LLD::INSTANCES::LEDGER));
}


TEST_CASE("LLD::TxnCommit mode mismatch aborts owner and releases coordinator",
          "[lld][txncommit][coordinator]")
{
    LedgerGuard guard;

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));

    /* Mismatched commit mode should fail but still release owner state. */
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::LEDGER));

    /* If ownership leaked, this begin would fail as nested/blocked. */
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
}


TEST_CASE("LLD::TxnCommit MEMPOOL requires the owner's exact memory mode",
          "[lld][txncommit][coordinator]")
{
    ContractGuard guard;
    uint8_t nOwnerFlags = TAO::Ledger::FLAGS::MEMPOOL;
    SECTION("MINER owner is aborted") { nOwnerFlags = TAO::Ledger::FLAGS::MINER; }
    SECTION("SANITIZE owner is aborted") { nOwnerFlags = TAO::Ledger::FLAGS::SANITIZE; }
    SECTION("MEMPOOL owner is committed") {}

    const auto key = std::make_pair(uint512_t(0x74786e6d6f6465), uint32_t(1));
    const uint256_t hashCaller(42);
    const bool fMatchingMode = (nOwnerFlags == TAO::Ledger::FLAGS::MEMPOOL);
    uint256_t hashRead;

    LLD::TransactionGuard owner(nOwnerFlags, LLD::INSTANCES::CONTRACT);
    REQUIRE(owner);
    REQUIRE(LLD::Contract->WriteContract(key, hashCaller, nOwnerFlags));
    REQUIRE(LLD::Contract->ReadContract(key, hashRead, nOwnerFlags));
    REQUIRE(hashRead == hashCaller);

    const bool fCommitted =
        LLD::TxnCommit(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::CONTRACT);
    const bool fOwnerVisible = LLD::Contract->ReadContract(key, hashRead, nOwnerFlags);
    const bool fMempoolVisible =
        LLD::Contract->ReadContract(key, hashRead, TAO::Ledger::FLAGS::MEMPOOL);
    LLD::Contract->EraseContract(key, TAO::Ledger::FLAGS::MEMPOOL);

    CHECK(fCommitted == fMatchingMode);
    CHECK(fOwnerVisible == fMatchingMode);
    CHECK(fMempoolVisible == fMatchingMode);
    CHECK_FALSE(LLD::Contract->ReadContract(key, hashRead, TAO::Ledger::FLAGS::BLOCK));

    LLD::TransactionGuard next(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::CONTRACT);
    REQUIRE(next);
    REQUIRE(LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL, LLD::INSTANCES::CONTRACT));
}


TEST_CASE("LLD::HasOpenTransaction covers every MERKLE database",
          "[lld][txncommit][merkle]")
{
    LogicalGuard logicalGuard;
    ClientGuard clientGuard;

    LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LOGICAL);
    const bool fLogicalDetected =
        LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LOGICAL);
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LOGICAL);

    LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CLIENT);
    const bool fClientDetected =
        LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CLIENT);
    LLD::TxnAbort(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CLIENT);

    REQUIRE(fLogicalDetected);
    REQUIRE(fClientDetected);
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

            TAO::Ledger::SetHardenCheckpointHook(
                [](const TAO::Ledger::BlockState&, bool* pfHardened)
                {
                    if(pfHardened)
                        *pfHardened = false;

                    return true;
                });
        }

        ~RealCodeLedgerGuard()
        {
            TAO::Ledger::SetHardenCheckpointHook({});

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
        uint1024_t              savedCheckpointHash;
        uint32_t                savedCheckpointHeight;
        uint32_t                savedBlockCounter;

        ChainStateGuard()
        : savedGenesis  (TAO::Ledger::ChainState::tStateGenesis)
        , savedBest     (TAO::Ledger::ChainState::tStateBest.load())
        , savedBestHash (TAO::Ledger::ChainState::hashBestChain.load())
        , savedBestHeight(TAO::Ledger::ChainState::nBestHeight.load())
        , savedBestTrust(TAO::Ledger::ChainState::nBestChainTrust.load())
        , savedCheckpointHash(TAO::Ledger::ChainState::hashCheckpoint.load())
        , savedCheckpointHeight(TAO::Ledger::ChainState::nCheckpointHeight.load())
        , savedBlockCounter(TAO::API::nBlockCounter.load())
        {}

        ~ChainStateGuard()
        {
            TAO::Ledger::ChainState::tStateGenesis   = savedGenesis;
            TAO::Ledger::ChainState::tStateBest      = savedBest;
            TAO::Ledger::ChainState::hashBestChain   = savedBestHash;
            TAO::Ledger::ChainState::nBestHeight     .store(savedBestHeight);
            TAO::Ledger::ChainState::nBestChainTrust .store(savedBestTrust);
            TAO::Ledger::ChainState::hashCheckpoint  = savedCheckpointHash;
            TAO::Ledger::ChainState::nCheckpointHeight.store(savedCheckpointHeight);
            TAO::API::nBlockCounter.store(savedBlockCounter);
        }
    };


    struct ShutdownGuard
    {
        const bool savedShutdown{config::fShutdown.load()};

        ~ShutdownGuard()
        {
            config::fShutdown.store(savedShutdown);
        }
    };


    struct BestChainDiskGuard
    {
        bool hadBest{false};
        uint1024_t hashBest;

        BestChainDiskGuard()
        : hadBest(LLD::Ledger->Read(std::string("hashbestchain"), hashBest))
        {
        }

        ~BestChainDiskGuard()
        {
            if(hadBest)
                LLD::Ledger->Write(std::string("hashbestchain"), hashBest);
            else
                LLD::Ledger->Erase(std::string("hashbestchain"));
        }
    };


    struct GenesisDiskGuard
    {
        const uint1024_t hash = TAO::Ledger::ChainState::Genesis();
        TAO::Ledger::BlockState saved;
        const bool hadGenesis = LLD::Ledger->ReadBlock(hash, saved);

        ~GenesisDiskGuard()
        {
            if(hadGenesis)
                LLD::Ledger->WriteBlock(hash, saved);
            else
                LLD::Ledger->EraseBlock(hash);
        }
    };


    struct ArgsMapGuard
    {
        const std::map<std::string, std::string> saved = config::mapArgs;
        ~ArgsMapGuard()
        {
            config::mapArgs = saved;
        }
    };


    struct FlagGuard
    {
        const bool savedClient  = config::fClient.load();
        const bool savedHybrid  = config::fHybrid.load();
        const bool savedTestNet = config::fTestNet.load();

        ~FlagGuard()
        {
            config::fClient.store(savedClient);
            config::fHybrid.store(savedHybrid);
            config::fTestNet.store(savedTestNet);
        }
    };


    struct CheckpointChainFixture
    {
        TAO::Ledger::BlockState genesis;
        TAO::Ledger::BlockState one;
        TAO::Ledger::BlockState two;
        TAO::Ledger::BlockState three;
        uint1024_t hashGenesis;
        uint1024_t hashOne;
        uint1024_t hashTwo;
        uint1024_t hashThree;
    };

    struct CheckpointBlocksDiskGuard
    {
        std::vector<uint1024_t> hashes;

        ~CheckpointBlocksDiskGuard()
        {
            for(const auto& hash : hashes)
                LLD::Ledger->EraseBlock(hash);
        }
    };


    struct CheckpointRepairHookGuard
    {
        ~CheckpointRepairHookGuard()
        {
            TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook({});
        }
    };


    struct HeightIndexDiskGuard
    {
        const std::pair<std::string, uint32_t> key;
        const std::pair<std::string, uint32_t> backup;
        const bool hadIndex;

        explicit HeightIndexDiskGuard(const uint32_t nHeight)
        : key(std::make_pair(std::string("height"), nHeight))
        , backup(std::make_pair(std::string("startup-height-backup"), nHeight))
        , hadIndex(LLD::Ledger->Exists(key))
        {
            if(hadIndex)
            {
                REQUIRE(LLD::Ledger->Index(backup, key));
                REQUIRE(LLD::Ledger->Erase(key, true));
            }
        }

        ~HeightIndexDiskGuard()
        {
            if(hadIndex)
            {
                LLD::Ledger->Index(key, backup);
                LLD::Ledger->Erase(backup, true);
            }
            else
                LLD::Ledger->Erase(key, true);
        }
    };


    CheckpointChainFixture BuildCheckpointChainFixture(const uint64_t nNonceBase,
                                                       CheckpointBlocksDiskGuard& blocksGuard)
    {
        CheckpointChainFixture fixture;

        fixture.genesis = TAO::Ledger::ChainState::tStateGenesis;
        REQUIRE_FALSE(fixture.genesis.IsNull());
        fixture.genesis.hashPrevBlock = 0;
        fixture.genesis.hashNextBlock = 0;
        fixture.hashGenesis = fixture.genesis.GetHash();

        fixture.one = fixture.genesis;
        fixture.one.hashPrevBlock = fixture.hashGenesis;
        fixture.one.hashNextBlock = 0;
        fixture.one.nHeight = 1;
        fixture.one.nNonce = nNonceBase + 1;
        fixture.one.nTime = nNonceBase + 1;
        fixture.one.nChainTrust = fixture.genesis.nChainTrust + 10;
        fixture.hashOne = fixture.one.GetHash();

        fixture.two = fixture.one;
        fixture.two.hashPrevBlock = fixture.hashOne;
        fixture.two.hashNextBlock = 0;
        fixture.two.nHeight = 2;
        fixture.two.nNonce = nNonceBase + 2;
        fixture.two.nTime = nNonceBase + 2;
        fixture.two.nChainTrust = fixture.one.nChainTrust + 10;
        fixture.hashTwo = fixture.two.GetHash();

        fixture.three = fixture.two;
        fixture.three.hashPrevBlock = fixture.hashTwo;
        fixture.three.hashNextBlock = 0;
        fixture.three.nHeight = 3;
        fixture.three.nNonce = nNonceBase + 3;
        fixture.three.nTime = nNonceBase + 3;
        fixture.three.nChainTrust = fixture.two.nChainTrust + 10;
        fixture.hashThree = fixture.three.GetHash();

        fixture.genesis.hashPrevBlock = 0;
        fixture.genesis.hashNextBlock = fixture.hashOne;
        fixture.one.hashNextBlock = fixture.hashTwo;
        fixture.two.hashNextBlock = fixture.hashThree;
        fixture.three.hashNextBlock = 0;

        blocksGuard.hashes.insert(blocksGuard.hashes.end(),
            {fixture.hashOne, fixture.hashTwo, fixture.hashThree});

        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashGenesis, fixture.genesis));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashOne, fixture.one));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, fixture.two));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashThree, fixture.three));
        REQUIRE(LLD::Ledger->WriteBestChain(fixture.hashThree));

        TAO::Ledger::ChainState::tStateGenesis = fixture.genesis;
        TAO::Ledger::ChainState::tStateBest = fixture.three;
        TAO::Ledger::ChainState::hashBestChain = fixture.hashThree;
        TAO::Ledger::ChainState::nBestHeight = fixture.three.nHeight;
        TAO::Ledger::ChainState::nBestChainTrust = fixture.three.nChainTrust;
        TAO::Ledger::ChainState::hashCheckpoint = 0;
        TAO::Ledger::ChainState::nCheckpointHeight = 0;

        return fixture;
    }

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
    BestChainDiskGuard  bestChainGuard;
    SyncProfileGuard syncProfileGuard;
    const bool fProfile = GENERATE(false, true);
    if(!fProfile)
        config::mapArgs.erase("-syncprofile");

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
    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    if(fProfile)
        REQUIRE(snapshot.nSetBestUs > 0);
    else
    {
        REQUIRE(snapshot.nSetBestUs == 0);
        REQUIRE(snapshot.nConnectUs == 0);
    }

    /* ---- (a) ChainState atomics must be unchanged ---- */
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load()        == hashGenesis);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load()          == 0u);
    REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter);

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
    BestChainDiskGuard  bestChainGuard;

    /* ---- Minimal genesis (nNonce distinct from earlier tests in this file to avoid hash collisions) ---- */
    TAO::Ledger::BlockState genesis;
    genesis.nVersion      = 4;
    genesis.hashPrevBlock = uint1024_t(0);
    genesis.nChannel      = 2;
    genesis.nHeight       = 0;
    genesis.nBits         = 1;
    genesis.nNonce        = 1001;
    genesis.nChainTrust   = 0; /* explicitly 0 so the heavier-than relationship is clear */
    genesis.nMoneySupply  = 1000;
    genesis.nFeesBurned   = 25;

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
    candidate.nMint         = 100;

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

    uint1024_t hashBestOnDisk;
    REQUIRE(LLD::Ledger->ReadBestChain(hashBestOnDisk));
    REQUIRE(hashBestOnDisk == hashCandidate);

    TAO::Ledger::BlockState committed;
    REQUIRE(LLD::Ledger->ReadBlock(hashCandidate, committed));
    REQUIRE(committed.nMoneySupply == 1100);
    REQUIRE(committed.nFeesBurned == 25);
    REQUIRE(candidate.nMoneySupply == committed.nMoneySupply);
    REQUIRE(candidate.nFeesBurned == committed.nFeesBurned);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().nMoneySupply == committed.nMoneySupply);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().nFeesBurned == committed.nFeesBurned);

    /* Cleanup */
    LLD::Ledger->EraseBlock(hashCandidate);
    LLD::Ledger->EraseBlock(hashGenesis);
}


TEST_CASE("Real SetBest(): rewind publishes the updated tip without its disconnected successor",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    GenesisDiskGuard    genesisDiskGuard;

    const TAO::Ledger::BlockState genesis = TAO::Ledger::ChainState::tStateGenesis;
    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    TAO::Ledger::ChainState::tStateBest = genesis;
    TAO::Ledger::ChainState::hashBestChain = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight = genesis.nHeight;
    TAO::Ledger::ChainState::nBestChainTrust = genesis.nChainTrust;

    TAO::Ledger::BlockState first;
    first.nVersion = 4;
    first.hashPrevBlock = hashGenesis;
    first.nChannel = 2;
    first.nHeight = genesis.nHeight + 1;
    first.nBits = 1;
    first.nNonce = 2201;
    first.nChainTrust = genesis.nChainTrust + 1;
    first.nMint = 100;
    REQUIRE(first.SetBest());

    TAO::Ledger::BlockState second = first;
    second.hashPrevBlock = first.GetHash();
    second.nHeight++;
    second.nNonce++;
    second.nChainTrust++;
    REQUIRE(second.SetBest());

    REQUIRE(LLD::Ledger->ReadBlock(first.GetHash(), first));
    REQUIRE(first.hashNextBlock == second.GetHash());
    REQUIRE(first.SetBest());

    TAO::Ledger::BlockState committed;
    REQUIRE(LLD::Ledger->ReadBlock(first.GetHash(), committed));
    REQUIRE(committed.hashNextBlock == 0);
    REQUIRE(first.hashNextBlock == 0);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().hashNextBlock == 0);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().nMoneySupply == committed.nMoneySupply);
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == first.GetHash());
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == first.nHeight);
    REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter + 3);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(second.GetHash()));
    REQUIRE_FALSE(TAO::Ledger::ChainState::fChainReorg.load());

    uint1024_t hashBestOnDisk;
    REQUIRE(LLD::Ledger->ReadBestChain(hashBestOnDisk));
    REQUIRE(hashBestOnDisk == first.GetHash());

    LLD::Ledger->EraseBlock(first.GetHash());
}


TEST_CASE("Real SetBest(): mismatched tip identity aborts before commit and publication",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard chainGuard;
    BestChainDiskGuard bestChainGuard;
    GenesisDiskGuard genesisDiskGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    auto fixture = BuildCheckpointChainFixture(2300, blocksGuard);
    const bool fOuterTxn = GENERATE(false, true);

    TAO::Ledger::BlockState corrupt = fixture.three;
    ++corrupt.nNonce;
    REQUIRE(corrupt.GetHash() != fixture.hashThree);
    if(fOuterTxn)
        REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    REQUIRE(LLD::Ledger->WriteBlock(fixture.hashThree, corrupt));

    REQUIRE_FALSE(fixture.three.SetBest());
    REQUIRE(debug::GetLastError().find("failed to read updated best chain state") != std::string::npos);
    REQUIRE_FALSE(LLD::HasOpenTransaction(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS));

    uint1024_t hashBestOnDisk;
    REQUIRE(LLD::Ledger->ReadBestChain(hashBestOnDisk));
    REQUIRE(hashBestOnDisk == fixture.hashThree);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == fixture.three.nHeight);
    REQUIRE(TAO::Ledger::ChainState::nBestChainTrust.load() == fixture.three.nChainTrust);
    REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter);
    REQUIRE(fixture.three.GetHash() == fixture.hashThree);

    TAO::Ledger::BlockState stored;
    REQUIRE(LLD::Ledger->ReadBlock(fixture.hashThree, stored));
    REQUIRE(stored.GetHash() == (fOuterTxn ? fixture.hashThree : corrupt.GetHash()));
}


TEST_CASE("Real SetBest(): debugreorg reads transactions before rewind erases them",
          "[ledger][setbest_txn][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    GenesisDiskGuard    genesisDiskGuard;
    struct ArgsGuard
    {
        const std::map<std::string, std::string> saved = config::mapArgs;
        ~ArgsGuard() { config::mapArgs = saved; }
    } argsGuard;

    SECTION("diagnostics disabled")
    {
        config::mapArgs["-debugreorg"] = "0";
    }
    SECTION("diagnostics enabled")
    {
        config::mapArgs["-debugreorg"] = "1";
    }

    TAO::Ledger::BlockState genesis = TAO::Ledger::ChainState::tStateGenesis;
    const uint1024_t hashGenesis = genesis.GetHash();

    TAO::Ledger::Transaction firstTx;
    firstTx.nSequence = 1;
    firstTx.hashGenesis = uint256_t(0x2203);
    TAO::Ledger::Transaction secondTx = firstTx;
    secondTx.nSequence = 2;
    secondTx.hashPrevTx = firstTx.GetHash();
    const uint512_t hashFirst = firstTx.GetHash();
    const uint512_t hashSecond = secondTx.GetHash();
    REQUIRE(LLD::Ledger->WriteTx(hashFirst, firstTx));
    REQUIRE(LLD::Ledger->WriteTx(hashSecond, secondTx));
    REQUIRE(LLD::Ledger->WriteLast(firstTx.hashGenesis, hashSecond));

    TAO::Ledger::BlockState tip;
    tip.nVersion = 4;
    tip.hashPrevBlock = hashGenesis;
    tip.nChannel = 2;
    tip.nHeight = genesis.nHeight + 1;
    tip.nBits = 1;
    tip.nNonce = 2203;
    tip.nChainTrust = genesis.nChainTrust + 1;
    tip.vtx = {{TAO::Ledger::TRANSACTION::TRITIUM, hashFirst},
               {TAO::Ledger::TRANSACTION::TRITIUM, hashSecond}};
    const uint1024_t hashTip = tip.GetHash();
    genesis.hashNextBlock = hashTip;
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    REQUIRE(LLD::Ledger->WriteBlock(hashTip, tip));
    REQUIRE(LLD::Ledger->IndexBlock(hashFirst, hashTip));
    REQUIRE(LLD::Ledger->IndexBlock(hashSecond, hashTip));
    REQUIRE(LLD::Ledger->WriteBestChain(hashTip));
    TAO::Ledger::ChainState::tStateBest = tip;
    TAO::Ledger::ChainState::hashBestChain = hashTip;
    TAO::Ledger::ChainState::nBestHeight = tip.nHeight;
    TAO::Ledger::ChainState::nBestChainTrust = tip.nChainTrust;

    REQUIRE(genesis.SetBest());
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashTip));
    REQUIRE_FALSE(LLD::Ledger->HasTx(hashFirst));
    REQUIRE_FALSE(LLD::Ledger->HasTx(hashSecond));
    REQUIRE_FALSE(LLD::Ledger->HasIndex(hashFirst));
    REQUIRE_FALSE(LLD::Ledger->HasIndex(hashSecond));
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis);
    REQUIRE_FALSE(LLD::HasOpenTransaction());
    REQUIRE_FALSE(TAO::Ledger::ChainState::fChainReorg.load());
    LLD::Ledger->EraseLast(firstTx.hashGenesis);
}


TEST_CASE("Real SetBest(): checkpoint hardening runs after commit and gates best-tip publication",
          "[ledger][setbest_txn][checkpoint][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    ShutdownGuard       shutdownGuard;

    config::fShutdown.store(false);

    TAO::Ledger::BlockState genesis =
        TAO::Ledger::ChainState::tStateGenesis;
    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(hashGenesis == TAO::Ledger::ChainState::Genesis());
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    REQUIRE(LLD::Ledger->WriteBestChain(hashGenesis));

    TAO::Ledger::ChainState::tStateGenesis      = genesis;
    TAO::Ledger::ChainState::tStateBest         = genesis;
    TAO::Ledger::ChainState::hashBestChain      = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight        = genesis.nHeight;
    TAO::Ledger::ChainState::nBestChainTrust    = genesis.nChainTrust;
    TAO::Ledger::ChainState::hashCheckpoint     = uint1024_t(0x1234);
    TAO::Ledger::ChainState::nCheckpointHeight  = 0;

    TAO::Ledger::BlockState candidate;
    candidate.nVersion      = 4;
    candidate.hashPrevBlock = hashGenesis;
    candidate.nChannel      = 2;
    candidate.nHeight       = genesis.nHeight + 1;
    candidate.nTime         = genesis.nTime + 1;
    candidate.nBits         = 1;
    candidate.nNonce        = 2001;
    candidate.nChainTrust   = genesis.nChainTrust + 1;

    const uint1024_t hashCandidate = candidate.GetHash();
    bool fHookCalledAfterCommit = false;
    bool fBestUnpublishedAtHook = false;

    SECTION("successful hardening publishes the checkpoint before the best tip")
    {
        TAO::Ledger::SetHardenCheckpointHook(
            [&](const TAO::Ledger::BlockState& state, bool* pfHardened)
            {
                uint1024_t hashBestOnDisk;
                fHookCalledAfterCommit =
                    !LLD::HasOpenTransaction(
                        TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS)
                    && LLD::Ledger->ReadBestChain(hashBestOnDisk)
                    && hashBestOnDisk == hashCandidate;
                fBestUnpublishedAtHook =
                    TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis;

                TAO::Ledger::ChainState::nCheckpointHeight = state.nHeight;
                TAO::Ledger::ChainState::hashCheckpoint = state.hashCheckpoint;
                *pfHardened = true;
                return true;
            });

        REQUIRE(candidate.SetBest());
        REQUIRE(fHookCalledAfterCommit);
        REQUIRE(fBestUnpublishedAtHook);
        REQUIRE(TAO::Ledger::ChainState::hashCheckpoint.load() == genesis.hashCheckpoint);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
        REQUIRE_FALSE(config::fShutdown.load());
    }

    SECTION("hardening read failure requests shutdown without publishing the best tip")
    {
        const uint1024_t hashCheckpointBefore =
            TAO::Ledger::ChainState::hashCheckpoint.load();

        TAO::Ledger::SetHardenCheckpointHook(
            [&](const TAO::Ledger::BlockState&, bool*)
            {
                uint1024_t hashBestOnDisk;
                fHookCalledAfterCommit =
                    !LLD::HasOpenTransaction(
                        TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS)
                    && LLD::Ledger->ReadBestChain(hashBestOnDisk)
                    && hashBestOnDisk == hashCandidate;
                fBestUnpublishedAtHook =
                    TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis;
                return false;
            });

        REQUIRE_FALSE(candidate.SetBest());
        REQUIRE(fHookCalledAfterCommit);
        REQUIRE(fBestUnpublishedAtHook);
        REQUIRE(config::fShutdown.load());
        REQUIRE(TAO::Ledger::ChainState::hashCheckpoint.load() == hashCheckpointBefore);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis);

        uint1024_t hashBestOnDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestOnDisk));
        REQUIRE(hashBestOnDisk == hashCandidate);
    }

    LLD::Ledger->EraseBlock(hashCandidate);
}


TEST_CASE("Real SetBest(): multi-block reorg evaluates checkpoint candidates in order",
          "[ledger][setbest_txn][checkpoint][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    ShutdownGuard       shutdownGuard;

    config::fShutdown.store(false);

    TAO::Ledger::BlockState genesis =
        TAO::Ledger::ChainState::tStateGenesis;
    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashGenesis, genesis));
    REQUIRE(LLD::Ledger->WriteBestChain(hashGenesis));

    TAO::Ledger::ChainState::tStateGenesis      = genesis;
    TAO::Ledger::ChainState::tStateBest         = genesis;
    TAO::Ledger::ChainState::hashBestChain      = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight        = genesis.nHeight;
    TAO::Ledger::ChainState::nBestChainTrust    = genesis.nChainTrust;

    TAO::Ledger::BlockState first;
    first.nVersion      = 4;
    first.hashPrevBlock = hashGenesis;
    first.nChannel      = 2;
    first.nHeight       = genesis.nHeight + 1;
    first.nTime         = genesis.nTime + 1;
    first.nBits         = 1;
    first.nNonce        = 2101;
    first.nChainTrust   = genesis.nChainTrust + 1;
    const uint1024_t hashFirst = first.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashFirst, first));

    TAO::Ledger::BlockState second;
    second.nVersion      = 4;
    second.hashPrevBlock = hashFirst;
    second.nChannel      = 2;
    second.nHeight       = first.nHeight + 1;
    second.nTime         = first.nTime + 1;
    second.nBits         = 1;
    second.nNonce        = 2102;
    second.nChainTrust   = first.nChainTrust + 1;

    std::vector<uint1024_t> vCandidates;
    TAO::Ledger::SetHardenCheckpointHook(
        [&](const TAO::Ledger::BlockState& state, bool* pfHardened)
        {
            vCandidates.push_back(state.GetHash());
            *pfHardened = false;
            return true;
        });

    REQUIRE(second.SetBest());
    const std::vector<uint1024_t> vExpected{hashGenesis, hashFirst};
    REQUIRE(vCandidates == vExpected);
    REQUIRE_FALSE(config::fShutdown.load());

    LLD::Ledger->EraseBlock(second.GetHash());
    LLD::Ledger->EraseBlock(hashFirst);
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


TEST_CASE("Client SetBest commits or rolls back the block, links, and best pointer atomically",
          "[ledger][client][setbest_txn][real]")
{
    ClientModeGuard modeGuard;
    ClientGuard clientGuard;
    LogicalGuard logicalGuard;
    ChainStateGuard chainGuard;
    BestChainDiskGuard ledgerBestGuard;

    const TAO::Ledger::ClientBlock genesis(TAO::Ledger::TritiumGenesis());
    const uint1024_t hashGenesis = genesis.GetHash();
    REQUIRE(hashGenesis == TAO::Ledger::ChainState::Genesis());

    TAO::Ledger::ClientBlock savedGenesis;
    const bool hadGenesis = LLD::Client->ReadBlock(hashGenesis, savedGenesis);
    uint1024_t savedBest;
    const bool hadBest = LLD::Client->ReadBestChain(savedBest);

    REQUIRE(LLD::Client->WriteBlock(hashGenesis, genesis));
    REQUIRE(LLD::Client->WriteBestChain(hashGenesis));

    TAO::Ledger::ChainState::tStateGenesis = genesis;
    TAO::Ledger::ChainState::tStateBest = genesis;
    TAO::Ledger::ChainState::hashBestChain = hashGenesis;
    TAO::Ledger::ChainState::nBestHeight.store(genesis.nHeight);

    TAO::Ledger::ClientBlock candidate(genesis);
    candidate.hashPrevBlock = hashGenesis;
    candidate.hashNextBlock = 0;
    candidate.nHeight = genesis.nHeight + 1;
    candidate.nTime = genesis.nTime + 1;
    candidate.nNonce++;
    candidate.nChannelWeight[0]++;
    const uint1024_t hashCandidate = candidate.GetHash();

    uint1024_t hashLedgerBest = hashCandidate;
    ++hashLedgerBest;
    REQUIRE(LLD::Ledger->Write(std::string("hashbestchain"), hashLedgerBest));

    SECTION("success commits every client-chain record before publication")
    {
        REQUIRE(candidate.Index());

        TAO::Ledger::ClientBlock committedCandidate;
        REQUIRE(LLD::Client->ReadBlock(hashCandidate, committedCandidate));
        REQUIRE(committedCandidate == candidate);

        TAO::Ledger::ClientBlock committedGenesis;
        REQUIRE(LLD::Client->ReadBlock(hashGenesis, committedGenesis));
        REQUIRE(committedGenesis.hashNextBlock == hashCandidate);

        uint1024_t hashBest;
        REQUIRE(LLD::Client->ReadBestChain(hashBest));
        REQUIRE(hashBest == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == candidate.nHeight);
        REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter + 1);
    }

    SECTION("rewind publishes the updated client tip and advances cache generation")
    {
        REQUIRE(candidate.Index());
        TAO::Ledger::ClientBlock rewind;
        REQUIRE(LLD::Client->ReadBlock(hashGenesis, rewind));
        REQUIRE(rewind.hashNextBlock == hashCandidate);
        REQUIRE(rewind.SetBest());
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().hashNextBlock == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == genesis.nHeight);
        REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter + 2);
    }

    SECTION("startup restores the committed client tip")
    {
        REQUIRE(candidate.Index());

        TAO::Ledger::ChainState::tStateGenesis = TAO::Ledger::BlockState();
        TAO::Ledger::ChainState::tStateBest = TAO::Ledger::BlockState();
        TAO::Ledger::ChainState::hashBestChain = 0;
        TAO::Ledger::ChainState::nBestHeight.store(0);
        TAO::Ledger::ChainState::nBestChainTrust.store(0);
        TAO::Ledger::ChainState::hashCheckpoint = 0;
        TAO::Ledger::ChainState::nCheckpointHeight.store(0);

        REQUIRE(TAO::Ledger::ChainState::Initialize());
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == candidate.nHeight);

        uint1024_t hashLedgerOnDisk;
        REQUIRE(LLD::Ledger->Read(std::string("hashbestchain"), hashLedgerOnDisk));
        REQUIRE(hashLedgerOnDisk == hashLedgerBest);
    }

    SECTION("startup recovery selects the last linked client block")
    {
        REQUIRE(candidate.Index());

        uint1024_t hashMissing = hashCandidate;
        ++hashMissing;
        REQUIRE(LLD::Client->WriteBestChain(hashMissing));

        TAO::Ledger::ChainState::tStateGenesis = TAO::Ledger::BlockState();
        TAO::Ledger::ChainState::tStateBest = TAO::Ledger::BlockState();
        TAO::Ledger::ChainState::hashBestChain = 0;
        TAO::Ledger::ChainState::nBestHeight.store(0);
        TAO::Ledger::ChainState::nBestChainTrust.store(0);
        TAO::Ledger::ChainState::hashCheckpoint = 0;
        TAO::Ledger::ChainState::nCheckpointHeight.store(0);

        REQUIRE(TAO::Ledger::ChainState::Initialize());
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashCandidate);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == candidate.nHeight);

        uint1024_t hashRecovered;
        REQUIRE(LLD::Client->ReadBestChain(hashRecovered));
        REQUIRE(hashRecovered == hashCandidate);

        uint1024_t hashLedgerOnDisk;
        REQUIRE(LLD::Ledger->Read(std::string("hashbestchain"), hashLedgerOnDisk));
        REQUIRE(hashLedgerOnDisk == hashLedgerBest);
    }

    SECTION("checkpoint failure rolls back every record and leaves ChainState unpublished")
    {
        REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::MERKLE));
        REQUIRE(LLD::Client->WriteBlock(hashCandidate, candidate));

        /* Remove one recovery-group participant to force the real checkpoint
         * barrier to reject the transition before any staged record is applied. */
        REQUIRE(LLD::Logical->TxnRelease());
        REQUIRE_FALSE(candidate.SetBest());

        TAO::Ledger::ClientBlock missingCandidate;
        REQUIRE_FALSE(LLD::Client->ReadBlock(hashCandidate, missingCandidate));

        TAO::Ledger::ClientBlock unchangedGenesis;
        REQUIRE(LLD::Client->ReadBlock(hashGenesis, unchangedGenesis));
        REQUIRE(unchangedGenesis.hashNextBlock == 0);

        uint1024_t hashBest;
        REQUIRE(LLD::Client->ReadBestChain(hashBest));
        REQUIRE(hashBest == hashGenesis);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashGenesis);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == hashGenesis);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == genesis.nHeight);
        REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter);
    }

    LLD::Client->EraseBlock(hashCandidate);
    if(hadGenesis)
        LLD::Client->WriteBlock(hashGenesis, savedGenesis);
    else
        LLD::Client->EraseBlock(hashGenesis);

    if(hadBest)
        LLD::Client->WriteBestChain(savedBest);
    else
        LLD::Client->Erase(std::string("hashbestchain"));
}


TEST_CASE("Checkpoint fixture teardown restores genesis and removes synthetic blocks",
          "[ledger][chainstate][checkpoint][startup][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard chainGuard;
    BestChainDiskGuard bestChainGuard;
    const uint1024_t hashGenesis = TAO::Ledger::ChainState::Genesis();
    TAO::Ledger::BlockState savedGenesis;
    REQUIRE(LLD::Ledger->ReadBlock(hashGenesis, savedGenesis));
    std::vector<uint1024_t> hashes;

    {
        GenesisDiskGuard genesisGuard;
        CheckpointBlocksDiskGuard blocksGuard;
        const auto fixture = BuildCheckpointChainFixture(30000, blocksGuard);
        hashes = blocksGuard.hashes;
        REQUIRE(LLD::Ledger->EraseBlock(fixture.hashTwo));
    }

    for(const auto& hash : hashes)
        REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));

    TAO::Ledger::BlockState restoredGenesis;
    REQUIRE(LLD::Ledger->ReadBlock(hashGenesis, restoredGenesis));
    REQUIRE(restoredGenesis.GetHash() == savedGenesis.GetHash());
    REQUIRE(restoredGenesis.hashNextBlock == savedGenesis.hashNextBlock);
}


TEST_CASE("ChainState hardcoded-checkpoint startup recovery is safe-by-default and preflighted",
          "[ledger][chainstate][checkpoint][startup][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    GenesisDiskGuard    genesisGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    ArgsMapGuard        argsGuard;
    FlagGuard           flagGuard;
    CheckpointRepairHookGuard repairHookGuard;

    config::fClient.store(false);
    config::fHybrid.store(false);
    config::fTestNet.store(true);

    SECTION("all applicable checkpoints present: startup recovery path is a no-op")
    {
        const auto fixture = BuildCheckpointChainFixture(31000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState&)
            {
                ++nSetBestCalls;
                return true;
            });
        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, fixture.hashTwo}
        };

        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, false));
        REQUIRE(nSetBestCalls == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("newest applicable checkpoint missing: default startup is non-destructive")
    {
        const auto fixture = BuildCheckpointChainFixture(32000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState&)
            {
                ++nSetBestCalls;
                return true;
            });
        const uint1024_t hashMissingCheckpoint(0x77770001);
        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, hashMissingCheckpoint}
        };

        SECTION("checkpoint is missing") {}
        SECTION("checkpoint record is invalid")
        {
            checkpoints[2] = fixture.hashThree;
        }

        debug::GetLastError();
        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, false));
        REQUIRE(debug::GetLastError().empty());
        REQUIRE(nSetBestCalls == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("earliest applicable checkpoint missing: iterator boundary is safe")
    {
        const auto fixture = BuildCheckpointChainFixture(33000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState&)
            {
                ++nSetBestCalls;
                return true;
            });
        const uint1024_t hashMissingEarliest(0x77770002);
        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, hashMissingEarliest}
        };

        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, false));
        REQUIRE(nSetBestCalls == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }



    SECTION("opt-in repair refuses rollback when ancestry walk is unreadable")
    {
        const auto fixture = BuildCheckpointChainFixture(34000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState&)
            {
                ++nSetBestCalls;
                return true;
            });
        const uint1024_t hashMissingCheckpoint(0x77770003);
        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, hashMissingCheckpoint}
        };

        REQUIRE(LLD::Ledger->EraseBlock(fixture.hashTwo));
        REQUIRE_FALSE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("opt-in repair refuses an ancestor stored under the wrong hash")
    {
        const auto fixture = BuildCheckpointChainFixture(34100, blocksGuard);
        auto corrupt = fixture.two;
        ++corrupt.nNonce;
        REQUIRE(corrupt.GetHash() != fixture.hashTwo);
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, corrupt));
        auto ancestor = fixture.one;
        ancestor.hashNextBlock = corrupt.GetHash();
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashOne, ancestor));

        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState&)
            {
                ++nSetBestCalls;
                return true;
            });
        const std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, uint1024_t(0x77770007)}
        };

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 0);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("failed opt-in repair aborts staged changes without publishing a new tip")
    {
        TrustGuard trustGuard;
        const auto fixture = BuildCheckpointChainFixture(34200, blocksGuard);
        bool fFailCommit = false;
        SECTION("repair hook returns false") {}
        SECTION("outer transaction commit fails") { fFailCommit = true; }

        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&](const TAO::Ledger::BlockState& stateAncestor)
            {
                ++nSetBestCalls;
                REQUIRE(LLD::HasOpenTransaction());
                REQUIRE(LLD::Ledger->WriteBestChain(stateAncestor.GetHash()));
                REQUIRE(LLD::Ledger->EraseBlock(fixture.hashThree));
                if(fFailCommit)
                {
                    /* A missing participant forces commit to abort before durable apply. */
                    REQUIRE(LLD::Trust->TxnRelease());
                    return true;
                }

                return false;
            });
        const std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, uint1024_t(0x77770008)}
        };

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 1);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == fixture.three.nHeight);
        REQUIRE(TAO::Ledger::ChainState::nBestChainTrust.load() == fixture.three.nChainTrust);
        REQUIRE(TAO::Ledger::ChainState::hashCheckpoint.load() == 0);
        REQUIRE(TAO::Ledger::ChainState::nCheckpointHeight.load() == 0);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
        TAO::Ledger::BlockState stateBestDisk;
        REQUIRE(LLD::Ledger->ReadBlock(fixture.hashThree, stateBestDisk));
        REQUIRE(stateBestDisk.GetHash() == fixture.hashThree);
    }

    SECTION("opt-in repair succeeds only after full rollback path preflight")
    {
        const auto fixture = BuildCheckpointChainFixture(35000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState& stateAncestor)
            {
                ++nSetBestCalls;
                TAO::Ledger::ChainState::tStateBest = stateAncestor;
                TAO::Ledger::ChainState::hashBestChain = stateAncestor.GetHash();
                TAO::Ledger::ChainState::nBestHeight = stateAncestor.nHeight;
                TAO::Ledger::ChainState::nBestChainTrust = stateAncestor.nChainTrust;
                return LLD::Ledger->WriteBestChain(stateAncestor.GetHash());
            });
        const uint1024_t hashMissingCheckpoint(0x77770004);
        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, hashMissingCheckpoint}
        };

        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 1);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashOne);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashOne);
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == 1);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashOne);
    }

    SECTION("opt-in repair can use the first older readable checkpoint after consecutive gaps")
    {
        const auto fixture = BuildCheckpointChainFixture(36000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState& stateAncestor)
            {
                ++nSetBestCalls;
                TAO::Ledger::ChainState::tStateBest = stateAncestor;
                TAO::Ledger::ChainState::hashBestChain = stateAncestor.GetHash();
                TAO::Ledger::ChainState::nBestHeight = stateAncestor.nHeight;
                TAO::Ledger::ChainState::nBestChainTrust = stateAncestor.nChainTrust;
                return LLD::Ledger->WriteBestChain(stateAncestor.GetHash());
            });

        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, uint1024_t(0x77770005)},
            {3, uint1024_t(0x77770006)}
        };

        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 1);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashOne);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashOne);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("opt-in repair treats invalid checkpoint records like missing while searching older fallback")
    {
        const auto fixture = BuildCheckpointChainFixture(37000, blocksGuard);
        uint32_t nSetBestCalls = 0;
        TAO::Ledger::ChainState::SetCheckpointRepairSetBestHook(
            [&nSetBestCalls](const TAO::Ledger::BlockState& stateAncestor)
            {
                ++nSetBestCalls;
                TAO::Ledger::ChainState::tStateBest = stateAncestor;
                TAO::Ledger::ChainState::hashBestChain = stateAncestor.GetHash();
                TAO::Ledger::ChainState::nBestHeight = stateAncestor.nHeight;
                TAO::Ledger::ChainState::nBestChainTrust = stateAncestor.nChainTrust;
                return LLD::Ledger->WriteBestChain(stateAncestor.GetHash());
            });

        std::map<uint32_t, uint1024_t> checkpoints =
        {
            {1, fixture.hashOne},
            {2, fixture.hashThree}
        };

        REQUIRE(TAO::Ledger::ChainState::RunHardcodedCheckpointRecoveryForTests(checkpoints, true));
        REQUIRE(nSetBestCalls == 1);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashOne);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashOne);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }
}


TEST_CASE("Tritium acceptance checks predecessor identity before height and difficulty",
          "[ledger][tritium][accept][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    FlagGuard flagGuard;
    config::fClient.store(false);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion = 7;
    statePrev.nHeight = 10;
    statePrev.nNonce = 38300;
    const uint1024_t hashPrev = statePrev.GetHash();
    blocksGuard.hashes.push_back(hashPrev);

    TAO::Ledger::TritiumBlock block;
    block.hashPrevBlock = hashPrev;
    block.nHeight = 12;
    std::string strExpectedError = "previous block identity mismatch";
    uint32_t nExpectedBits = 0;
    bool fDifficultyMismatch = false;

    SECTION("wrong identity at the expected height")
    {
        ++statePrev.nNonce;
        block.nHeight = 11;
    }
    SECTION("wrong identity at a different height")
    {
        ++statePrev.nNonce;
    }
    SECTION("correct identity at a different height")
    {
        strExpectedError = "incorrect block height.";
    }
    SECTION("correct predecessor with mismatched difficulty")
    {
        block.nVersion = 7;
        block.nHeight = statePrev.nHeight + 1;
        block.nChannel = TAO::Ledger::CHANNEL::HASH;
        nExpectedBits = TAO::Ledger::GetNextTargetRequired(statePrev, block.GetChannel());
        block.nBits = nExpectedBits ^ 1;
        strExpectedError = "incorrect proof-of-work/proof-of-stake";
        fDifficultyMismatch = true;
    }

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));
    debug::GetLastError();
    REQUIRE_FALSE(block.Accept());
    const std::string strError = debug::GetLastError();
    REQUIRE(strError.find(strExpectedError) != std::string::npos);
    if(fDifficultyMismatch)
    {
        REQUIRE(strError.find("block=" + block.GetHash().SubString()) != std::string::npos);
        REQUIRE(strError.find(" prev=" + hashPrev.SubString()) != std::string::npos);
        REQUIRE(strError.find(" prev_height=" + std::to_string(statePrev.nHeight)) != std::string::npos);
        REQUIRE(strError.find(" block_height=" + std::to_string(block.nHeight)) != std::string::npos);
        REQUIRE(strError.find(" channel=" + std::to_string(block.GetChannel())) != std::string::npos);
        std::ostringstream bits;
        bits << " nBits=0x" << std::hex << block.nBits << " expected=0x" << nExpectedBits;
        REQUIRE(strError.find(bits.str()) != std::string::npos);
    }
    REQUIRE_FALSE(LLD::HasOpenTransaction());
}


TEST_CASE("ChainState startup best-chain audit localizes and repairs near-tip predecessor holes",
          "[ledger][chainstate][startup][repair][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    ArgsMapGuard        argsGuard;
    FlagGuard           flagGuard;

    config::fClient.store(false);
    config::fHybrid.store(false);
    config::fTestNet.store(false);

    GenesisDiskGuard    genesisGuard;
    HeightIndexDiskGuard heightOneGuard(1);
    HeightIndexDiskGuard heightTwoGuard(2);
    TAO::Ledger::ChainState::tStateGenesis = TAO::Ledger::LegacyGenesis();
    REQUIRE(TAO::Ledger::ChainState::tStateGenesis.GetHash() == TAO::Ledger::ChainState::Genesis());

    SECTION("healthy ancestry passes at genesis and at the scan bound")
    {
        const auto fixture = BuildCheckpointChainFixture(37900, blocksGuard);
        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 16));
        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 3));
        REQUIRE(LLD::Ledger->EraseBlock(fixture.hashOne));
        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 1));

        TAO::Ledger::ChainState::tStateBest = fixture.genesis;
        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 16));
    }

    SECTION("ancestry must terminate at the configured genesis")
    {
        TAO::Ledger::ChainState::tStateGenesis.nNonce++;
        const auto fixture = BuildCheckpointChainFixture(37910, blocksGuard);
        blocksGuard.hashes.push_back(fixture.hashGenesis);
        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 16));
        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 3));
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("a nonzero-height tip with no predecessor is not genesis")
    {
        auto fixture = BuildCheckpointChainFixture(37920, blocksGuard);
        fixture.three.hashPrevBlock = 0;
        TAO::Ledger::ChainState::tStateBest = fixture.three;
        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 16));
        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("missing predecessor near best tip fails startup audit without mutation")
    {
        const auto fixture = BuildCheckpointChainFixture(38000, blocksGuard);
        REQUIRE(LLD::Ledger->EraseBlock(fixture.hashTwo));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 16));
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("repairchain restores missing hash-key lookup from height recovery data")
    {
        const auto fixture = BuildCheckpointChainFixture(38100, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));

        SECTION("hash key is absent") {}
        SECTION("hash key exists but cannot be read")
        {
            REQUIRE(LLD::Ledger->Write(fixture.hashTwo, uint8_t(0)));
            REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));
            TAO::Ledger::BlockState unreadable;
            REQUIRE_FALSE(LLD::Ledger->ReadBlock(fixture.hashTwo, unreadable));
        }

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));

        TAO::Ledger::BlockState restored;
        REQUIRE(LLD::Ledger->ReadBlock(fixture.hashTwo, restored));
        REQUIRE(restored.GetHash() == fixture.hashTwo);
        REQUIRE(restored.hashNextBlock == fixture.hashThree);

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        restored.hashNextBlock = 0;
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, restored));
        TAO::Ledger::BlockState byHeight;
        REQUIRE(LLD::Ledger->ReadBlock(uint32_t(2), byHeight));
        REQUIRE(byHeight.GetHash() == fixture.hashTwo);
        REQUIRE(byHeight.hashNextBlock == 0);
    }

    SECTION("repairchain restores a consecutive missing suffix when the height index proves it")
    {
        const auto fixture = BuildCheckpointChainFixture(38105, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(1), fixture.hashOne));
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));
        REQUIRE(LLD::Ledger->Erase(fixture.hashOne, true));

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashOne));
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));

        TAO::Ledger::BlockState restoredOne;
        TAO::Ledger::BlockState restoredTwo;
        REQUIRE(LLD::Ledger->ReadBlock(fixture.hashOne, restoredOne));
        REQUIRE(LLD::Ledger->ReadBlock(fixture.hashTwo, restoredTwo));
        REQUIRE(restoredOne.GetHash() == fixture.hashOne);
        REQUIRE(restoredTwo.GetHash() == fixture.hashTwo);
        REQUIRE(restoredOne.hashNextBlock == fixture.hashTwo);
        REQUIRE(restoredTwo.hashNextBlock == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("repairchain requires an anchor within the exact remaining scan depth")
    {
        const auto fixture = BuildCheckpointChainFixture(38106, blocksGuard);
        std::vector<uint1024_t> vMissing;
        uint32_t nDepth = 3;

        SECTION("one missing alias at the tip")
        {
            vMissing = {fixture.hashTwo};
            nDepth = 2;
        }
        SECTION("one missing alias after a readable predecessor")
        {
            vMissing = {fixture.hashOne};
        }
        SECTION("two consecutive missing aliases")
        {
            vMissing = {fixture.hashTwo, fixture.hashOne};
        }

        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(1), fixture.hashOne));
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        for(const auto& hash : vMissing)
            REQUIRE(LLD::Ledger->Erase(hash, true));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, nDepth - 1));
        for(const auto& hash : vMissing)
            REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));
        REQUIRE_FALSE(LLD::HasOpenTransaction());

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, nDepth));
        for(const auto& hash : vMissing)
        {
            TAO::Ledger::BlockState restored;
            REQUIRE(LLD::Ledger->ReadBlock(hash, restored));
            REQUIRE(restored.GetHash() == hash);
        }
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("repairchain uses only a verified genesis as a terminal height-index anchor")
    {
        HeightIndexDiskGuard heightZeroGuard(0);
        bool fValidRoot = true;
        bool fRecoverSuffix = false;
        bool fWrongSuccessor = false;

        SECTION("missing genesis alias") {}
        SECTION("missing suffix through genesis")
        {
            fRecoverSuffix = true;
        }
        SECTION("non-genesis zero-prev root")
        {
            ++TAO::Ledger::ChainState::tStateGenesis.nNonce;
            fValidRoot = false;
        }
        SECTION("genesis with the wrong successor")
        {
            fWrongSuccessor = true;
            fValidRoot = false;
        }

        auto fixture = BuildCheckpointChainFixture(38107, blocksGuard);
        if(fixture.hashGenesis != TAO::Ledger::ChainState::Genesis())
            blocksGuard.hashes.push_back(fixture.hashGenesis);
        if(fWrongSuccessor)
        {
            fixture.genesis.hashNextBlock = fixture.hashTwo;
            REQUIRE(LLD::Ledger->WriteBlock(fixture.hashGenesis, fixture.genesis));
        }

        std::vector<uint1024_t> vMissing = {fixture.hashGenesis};
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(0), fixture.hashGenesis));
        if(fRecoverSuffix)
        {
            REQUIRE(LLD::Ledger->IndexBlock(uint32_t(1), fixture.hashOne));
            REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
            vMissing.insert(vMissing.end(), {fixture.hashOne, fixture.hashTwo});
        }
        for(const auto& hash : vMissing)
            REQUIRE(LLD::Ledger->Erase(hash, true));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 3));
        for(const auto& hash : vMissing)
            REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 3) == fValidRoot);
        for(const auto& hash : vMissing)
            REQUIRE(LLD::Ledger->HasBlock(hash) == fValidRoot);
        if(fValidRoot)
        {
            TAO::Ledger::BlockState restored;
            REQUIRE(LLD::Ledger->ReadBlock(fixture.hashGenesis, restored));
            REQUIRE(restored.GetHash() == TAO::Ledger::ChainState::Genesis());
            REQUIRE(restored.nHeight == 0);
            REQUIRE(restored.hashPrevBlock == 0);
            REQUIRE(restored.hashNextBlock == fixture.hashOne);
            REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(false, 3));
        }
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("failed ancestry revalidation aborts the staged predecessor repair")
    {
        auto fixture = BuildCheckpointChainFixture(38110, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));
        bool fExpectRepair = false;
        bool fExpectHashOnePresent = true;

        SECTION("another predecessor is unavailable")
        {
            REQUIRE(LLD::Ledger->EraseBlock(fixture.hashOne));
            REQUIRE_FALSE(LLD::Ledger->Exists(std::make_pair(std::string("height"), uint32_t(1))));
            fExpectHashOnePresent = false;
        }
        SECTION("another predecessor is recoverable")
        {
            REQUIRE(LLD::Ledger->IndexBlock(uint32_t(1), fixture.hashOne));
            REQUIRE(LLD::Ledger->Erase(fixture.hashOne, true));
            fExpectRepair = true;
        }
        SECTION("an older predecessor has an invalid successor link")
        {
            fixture.one.hashNextBlock = 0;
            REQUIRE(LLD::Ledger->WriteBlock(fixture.hashOne, fixture.one));
        }
        SECTION("an older predecessor has an invalid height")
        {
            fixture.one.nHeight = 9;
            REQUIRE(LLD::Ledger->WriteBlock(fixture.hashOne, fixture.one));
        }

        REQUIRE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16) == fExpectRepair);
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo) == fExpectRepair);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        TAO::Ledger::BlockState byHeight;
        REQUIRE(LLD::Ledger->ReadBlock(uint32_t(2), byHeight));
        REQUIRE(byHeight.GetHash() == fixture.hashTwo);
        REQUIRE(byHeight.hashNextBlock == fixture.hashThree);
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashOne) == fExpectHashOnePresent);
        uint1024_t hashBestDisk;
        REQUIRE(LLD::Ledger->ReadBestChain(hashBestDisk));
        REQUIRE(hashBestDisk == fixture.hashThree);
    }

    SECTION("invalid genesis behind a recoverable hole leaves the hash key missing")
    {
        TAO::Ledger::ChainState::tStateGenesis.nNonce++;
        const auto fixture = BuildCheckpointChainFixture(38120, blocksGuard);
        blocksGuard.hashes.push_back(fixture.hashGenesis);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashTwo));
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("repairchain refuses mutation when predecessor data is genuinely unavailable")
    {
        const auto fixture = BuildCheckpointChainFixture(38200, blocksGuard);
        REQUIRE(LLD::Ledger->EraseBlock(fixture.hashTwo));
        REQUIRE_FALSE(LLD::Ledger->Exists(std::make_pair(std::string("height"), uint32_t(2))));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashTwo));
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == fixture.hashThree);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }

    SECTION("repairchain refuses a consecutive suffix repair that never re-anchors to a verified predecessor")
    {
        auto fixture = BuildCheckpointChainFixture(38210, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(1), fixture.hashOne));
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));
        REQUIRE(LLD::Ledger->Erase(fixture.hashOne, true));

        auto genesis = fixture.genesis;
        genesis.hashNextBlock = 0;
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashGenesis, genesis));

        REQUIRE_FALSE(TAO::Ledger::ChainState::RunBestChainIntegrityAuditForTests(true, 16));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashOne));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashTwo));
        REQUIRE_FALSE(LLD::HasOpenTransaction());
    }
}


TEST_CASE("Read-only sector database preserves missing keychains and reads non-writable files",
          "[lld][auditblock][readonly]")
{
    const std::string strName = "_AUDIT_READONLY_TEST";
    const std::filesystem::path pathBase = config::GetDataDir() + strName;
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            if(!std::filesystem::exists(path))
                return;

            std::filesystem::permissions(path, std::filesystem::perms::owner_all);
            for(const auto& entry : std::filesystem::recursive_directory_iterator(path))
                std::filesystem::permissions(entry.path(), std::filesystem::perms::owner_all);
            std::filesystem::remove_all(path);
        }
    } directoryGuard{pathBase};

    using Database = LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>;
    {
        Database writer(strName, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
        REQUIRE(writer.Write(uint32_t(1), uint32_t(11)));
        REQUIRE(writer.Write(uint32_t(2), uint32_t(22)));
    }

    SECTION("read-only streams support cached and lazily opened collision files")
    {
        std::map<std::string, std::string> contents;
        for(const auto& entry : std::filesystem::recursive_directory_iterator(pathBase))
        {
            if(entry.is_regular_file())
            {
                std::ifstream stream(entry.path(), std::ios::binary);
                contents[entry.path().string()] =
                    std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
                std::filesystem::permissions(entry.path(), std::filesystem::perms::owner_read);
            }
            else
                std::filesystem::permissions(entry.path(),
                    std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec);
        }
        std::filesystem::permissions(pathBase,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec);

        {
            Database reader(strName, 0, 1, 1024);
            uint32_t value = 0;
            REQUIRE(reader.Read(uint32_t(1), value));
            REQUIRE(value == 11);
            REQUIRE(reader.Read(uint32_t(2), value));
            REQUIRE(value == 22);
            REQUIRE_FALSE(reader.Write(uint32_t(3), uint32_t(33)));
            REQUIRE_FALSE(reader.Index(uint32_t(3), uint32_t(1)));
            REQUIRE_FALSE(reader.Erase(uint32_t(1)));
        }

        size_t nFiles = 0;
        for(const auto& entry : std::filesystem::recursive_directory_iterator(pathBase))
        {
            if(!entry.is_regular_file())
                continue;
            ++nFiles;
            std::ifstream stream(entry.path(), std::ios::binary);
            const std::string actual(
                (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            REQUIRE(contents.at(entry.path().string()) == actual);
        }
        REQUIRE(nFiles == contents.size());
    }

    SECTION("missing index is not recreated")
    {
        const auto path = pathBase / "keychain/_hashmap.index";
        REQUIRE(std::filesystem::remove(path));
        Database reader(strName, 0, 1, 1024);
        REQUIRE_FALSE(reader.Exists(uint32_t(1)));
        REQUIRE_FALSE(std::filesystem::exists(path));
    }

    SECTION("missing hashmap is not recreated and other collision files remain readable")
    {
        const auto path = pathBase / "keychain/_hashmap.00000";
        REQUIRE(std::filesystem::remove(path));
        Database reader(strName, 0, 1, 1024);
        REQUIRE_FALSE(reader.Exists(uint32_t(1)));
        uint32_t value = 0;
        REQUIRE(reader.Read(uint32_t(2), value));
        REQUIRE(value == 22);
        REQUIRE_FALSE(std::filesystem::exists(path));
    }

    SECTION("missing database is not created even with an explicit create flag")
    {
        std::filesystem::remove_all(pathBase);
        Database reader(strName, LLD::FLAGS::CREATE | LLD::FLAGS::READONLY, 1, 1024);
        REQUIRE_FALSE(reader.Exists(uint32_t(1)));
        REQUIRE_FALSE(std::filesystem::exists(pathBase));
    }

    SECTION("meter stops on database destruction without global shutdown")
    {
        ArgsMapGuard argsGuard;
        config::mapArgs["-lldmeters"] = "1";
        REQUIRE_FALSE(config::fShutdown.load());
        {
            Database reader(strName, 0, 1, 1024);
            uint32_t value = 0;
            REQUIRE(reader.Read(uint32_t(1), value));
        }
        REQUIRE_FALSE(config::fShutdown.load());
    }
}


#ifndef WIN32
/* Run explicitly with NEXUS_AUDIT_BINARY pointing to the production nexus executable. */
TEST_CASE("Offline audit CLI validates arguments and reports exact source evidence",
          "[.][auditblock-cli]")
{
    const char* pszBinary = std::getenv("NEXUS_AUDIT_BINARY");
    REQUIRE(pszBinary != nullptr);
    const std::string strBinary = std::filesystem::absolute(pszBinary).string();
    REQUIRE(std::filesystem::is_regular_file(strBinary));

    const bool fClient = GENERATE(false, true);
    const std::string strName = "_AUDIT_CLI_TEST";
    const std::filesystem::path pathBase = config::GetDataDir() + strName;
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard() { std::filesystem::remove_all(path); }
    } directoryGuard{pathBase};
    const std::string strDatabase = strName + "/_LEDGER";
    const std::filesystem::path pathLedger = config::GetDataDir() + strDatabase;

    TAO::Ledger::BlockState state;
    state.nHeight = 42;
    state.nNonce = 705;
    const uint1024_t hash = state.GetHash();
    TAO::Ledger::BlockState other = state;
    ++other.nNonce;

    const auto Run = [&](const std::vector<std::string>& options, const int nExpectedExit)
    {
        std::vector<std::string> args{strBinary, "-datadir=" + pathBase.string() + "/",
            "-client=" + std::string(fClient ? "1" : "0"), "-verbose=0"};
        args.insert(args.end(), options.begin(), options.end());
        std::vector<char*> argv;
        for(auto& arg : args)
            argv.push_back(arg.data());
        argv.push_back(nullptr);

        std::unique_ptr<FILE, decltype(&std::fclose)> output(std::tmpfile(), &std::fclose);
        REQUIRE(output != nullptr);
        posix_spawn_file_actions_t actions;
        REQUIRE(posix_spawn_file_actions_init(&actions) == 0);
        REQUIRE(posix_spawn_file_actions_adddup2(&actions, fileno(output.get()), STDOUT_FILENO) == 0);
        REQUIRE(posix_spawn_file_actions_adddup2(&actions, fileno(output.get()), STDERR_FILENO) == 0);
        REQUIRE(posix_spawn_file_actions_addclose(&actions, fileno(output.get())) == 0);
        pid_t pid = 0;
        const int nSpawn = posix_spawn(&pid, strBinary.c_str(), &actions, nullptr, argv.data(), environ);
        posix_spawn_file_actions_destroy(&actions);
        REQUIRE(nSpawn == 0);

        int nStatus = 0;
        pid_t nWait = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while((nWait = waitpid(pid, &nStatus, WNOHANG)) == 0
           && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if(nWait == 0)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &nStatus, 0);
        }

        std::rewind(output.get());
        std::string text;
        char buffer[4096];
        while(const size_t nRead = std::fread(buffer, 1, sizeof(buffer), output.get()))
            text.append(buffer, nRead);
        INFO(text);
        REQUIRE(nWait == pid);
        REQUIRE(WIFEXITED(nStatus));
        REQUIRE(WEXITSTATUS(nStatus) == nExpectedExit);

        encoding::json summary;
        const size_t nBegin = text.find('{');
        if(nExpectedExit == 0)
        {
            REQUIRE(nBegin != std::string::npos);
            const size_t nEnd = text.find_last_of('}');
            summary = encoding::json::parse(text.substr(nBegin, nEnd - nBegin + 1));
        }
        else
        {
            REQUIRE(nBegin == std::string::npos);
            if(fClient)
                REQUIRE(text.find("-auditblock is NODE-only and does not support -client mode") != std::string::npos);
        }
        return summary;
    };
    const std::string target = "-auditblock=" + hash.ToString();
    if(fClient)
    {
        Run({target}, 2);
        REQUIRE_FALSE(std::filesystem::exists(pathLedger));
        return;
    }

    for(const std::string& invalid : std::vector<std::string>{"", "00", std::string(256, '0'), std::string(256, 'g')})
        Run({"-auditblock=" + invalid}, 2);
    for(const std::string& invalid : {"-auditblockheight=-1", "-auditblockheight=4294967296",
            "-auditblockheight=1x", "-auditblockfiles=0", "-auditblockfiles=",
            "-auditblockstartfile=100000", "-auditblockendfile=100000", "-auditblockchild=0"})
        Run({target, invalid}, 2);
    Run({target, "-auditblockstartfile=2", "-auditblockendfile=1"}, 2);
    Run({target}, 3);

    const uint32_t nBuckets = 256 * 256 * 64;
    const auto pathIndex = pathLedger / "keychain/_hashmap.index";
    std::filesystem::create_directories(pathIndex.parent_path());
    /* Pre-size the empty index independently of other test databases' bucket counts. */
    std::ofstream(pathIndex, std::ios::binary).close();
    std::filesystem::resize_file(pathIndex, uint64_t(nBuckets) * 4);
    using Database = LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>;
    {
        Database writer(strDatabase, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, nBuckets, 1024);
        REQUIRE(writer.Write(hash, state, "block"));
        REQUIRE(writer.Write(other.GetHash(), other, "block"));
        const auto heightKey = std::make_pair(std::string("height"), state.nHeight);
        REQUIRE(writer.Index(heightKey, hash));

        auto summary = Run({target, "-auditblockchild=" + std::string(256, '0'), "-lldmeters=1"}, 0);
        REQUIRE(summary["status"] == "FOUND");
        REQUIRE(summary["classification"] == "HASH_KEY_READABLE");
        REQUIRE(summary["hash_key"]["matches"] == true);
        REQUIRE(summary["height_index"]["matches"] == true);
        REQUIRE(summary["alias_relation"]["same_sector"] == true);
        REQUIRE(summary["candidate"]["valid_child_link"] == true);
        REQUIRE(summary["child_link"]["checked"] == true);
        REQUIRE(summary["child_link"]["child_prev_matches_target"] == false);
        REQUIRE(summary["child_link"]["hash_next_matches_child"] == true);
        REQUIRE(summary["child_link"]["height_next_matches_child"] == true);

        summary = Run({target, "-auditblockchild=" + other.GetHash().ToString()}, 0);
        REQUIRE(summary["candidate"]["valid_child_link"] == false);
        REQUIRE(summary["child_link"]["child_prev_matches_target"] == false);
        REQUIRE(summary["child_link"]["hash_next_matches_child"] == false);
        REQUIRE(summary["child_link"]["height_next_matches_child"] == false);

        TAO::Ledger::BlockState child = other;
        child.hashPrevBlock = hash;
        REQUIRE(writer.Write(child.GetHash(), child, "block"));
        summary = Run({target, "-auditblockchild=" + child.GetHash().ToString()}, 0);
        REQUIRE(summary["child_link"]["child_prev_matches_target"] == true);
        REQUIRE(writer.Index(other.GetHash(), child.GetHash()));
        summary = Run({target, "-auditblockchild=" + other.GetHash().ToString()}, 0);
        REQUIRE(summary["child_link"]["readable"] == true);
        REQUIRE(summary["child_link"]["child_prev_matches_target"] == false);
        REQUIRE(writer.Write(other.GetHash(), other, "block"));

        REQUIRE(writer.Index(heightKey, other.GetHash()));
        summary = Run({target}, 0);
        REQUIRE(summary["status"] == "FOUND");
        REQUIRE(summary["classification"] == "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK");
        REQUIRE(summary["hash_key"]["matches"] == true);
        REQUIRE(summary["height_index"]["matches"] == false);
        REQUIRE(writer.Index(heightKey, hash));
        REQUIRE(writer.Index(std::make_pair(std::string("height"), uint32_t(43)), other.GetHash()));
        summary = Run({target, "-auditblockheight=43"}, 0);
        REQUIRE(summary["classification"] == "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK");
        REQUIRE(summary["height_index"]["expected_height_check"]["matches"] == false);

        REQUIRE(writer.Index(std::make_pair(std::string("height"), uint32_t(43)), hash));
        summary = Run({target, "-auditblockheight=43"}, 0);
        REQUIRE(summary["classification"] == "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK");
        REQUIRE(summary["height_index"]["expected_height_check"]["readable"] == true);
        REQUIRE(summary["height_index"]["expected_height_check"]["matches"] == false);

        REQUIRE(writer.Erase(hash, true));
        summary = Run({target, "-auditblockheight=43", "-auditblockstartfile=99999"}, 0);
        REQUIRE(summary["status"] == "NOT_FOUND");
        REQUIRE(summary["classification"] == "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK");
        REQUIRE(summary["height_index"]["readable"] == true);
        REQUIRE(summary["height_index"]["matches"] == false);

        summary = Run({target}, 0);
        REQUIRE(summary["classification"] == "HASH_ALIAS_MISSING_HEIGHT_INDEX_PRESENT");
        REQUIRE(summary["height_index"]["checked"] == true);
        REQUIRE(summary["height_index"]["height"] == 42);
        REQUIRE(summary["height_index"]["matches"] == true);
        REQUIRE(summary["raw_scan"]["found"] == true);
        REQUIRE(summary["alias_relation"]["same_sector"].is_null());

        REQUIRE(writer.Index(heightKey, other.GetHash()));
        summary = Run({target}, 0);
        REQUIRE(summary["classification"] == "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK");
        REQUIRE(summary["height_index"]["checked"] == true);
        REQUIRE(summary["height_index"]["matches"] == false);
        REQUIRE(writer.Index(heightKey, std::make_pair(std::string("height"), uint32_t(43))));

        summary = Run({target, "-auditblockheight=42", "-auditblockstartfile=99999"}, 0);
        REQUIRE(summary["status"] == "FOUND");
        REQUIRE(summary["classification"] == "HASH_ALIAS_MISSING_HEIGHT_INDEX_PRESENT");
        REQUIRE(summary["raw_scan"]["found"] == false);

        REQUIRE(writer.Write(hash, other, "block"));
        summary = Run({target, "-auditblockstartfile=99999",
            "-auditblockchild=" + std::string(256, '0')}, 0);
        REQUIRE(summary["status"] == "FOUND");
        REQUIRE(summary["classification"] == "HASH_KEY_READABLE_MISMATCH");
        REQUIRE(summary["hash_key"]["matches"] == false);
        REQUIRE(summary["height_index"]["matches"] == true);
        REQUIRE(summary["child_link"]["hash_next_matches_child"] == false);
        REQUIRE(summary["child_link"]["height_next_matches_child"] == true);
        REQUIRE(writer.Erase(heightKey, true));
        summary = Run({target, "-auditblockstartfile=99999"}, 0);
        REQUIRE(summary["status"] == "NOT_FOUND");
        REQUIRE(summary["classification"] == "HASH_KEY_READABLE_MISMATCH");

        REQUIRE(writer.Write(hash));
        REQUIRE(writer.Write(heightKey));
        summary = Run({target, "-auditblockheight=42", "-auditblockstartfile=99999"}, 0);
        for(const std::string& key : {"hash_key", "height_index"})
        {
            REQUIRE(summary[key]["exists"] == true);
            REQUIRE(summary[key]["keychain_only"] == true);
            REQUIRE(summary[key]["readable"] == false);
            REQUIRE(summary[key]["oversized"] == false);
        }
        REQUIRE(summary["alias_relation"]["comparable"] == false);
        REQUIRE(summary["alias_relation"]["same_sector"].is_null());
    }

    {
        /* Claim a sector size the writer could never produce so the bounded alias reads must
         * report the damaged aliases instead of allocating the claimed record. */
        LLD::BinaryHashMap keychain(config::GetDataDir() + strDatabase + "/keychain/",
            LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, nBuckets);

        DataStream ssHashKey(SER_LLD, LLD::DATABASE_VERSION);
        ssHashKey << hash;

        LLD::SectorKey cKey;
        REQUIRE(keychain.Get(ssHashKey.Bytes(), cKey));
        cKey.nSectorSize = uint32_t(MAX_SIZE) + 64;
        REQUIRE(keychain.Put(cKey));

        DataStream ssHeightKey(SER_LLD, LLD::DATABASE_VERSION);
        ssHeightKey << std::make_pair(std::string("height"), uint32_t(42));

        LLD::SectorKey cHeight(cKey);
        cHeight.SetKey(ssHeightKey.Bytes());
        REQUIRE(keychain.Put(cHeight));
    }

    {
        auto oversized = Run({target, "-auditblockheight=42", "-auditblockstartfile=99999"}, 0);
        REQUIRE(oversized["status"] == "NOT_FOUND");
        REQUIRE(oversized["classification"] == "HASH_KEY_PRESENT_UNREADABLE_RAW_RECORD_MISSING");
        REQUIRE(oversized["hash_key"]["exists"] == true);
        REQUIRE(oversized["hash_key"]["readable"] == false);
        REQUIRE(oversized["hash_key"]["oversized"] == true);
        REQUIRE(oversized["height_index"]["exists"] == true);
        REQUIRE(oversized["height_index"]["readable"] == false);
        REQUIRE(oversized["height_index"]["oversized"] == true);
    }

    std::filesystem::remove_all(pathLedger / "keychain");
    std::filesystem::remove(pathLedger / "datachain/_block.00000");
    const auto pathSector = pathLedger / "datachain/_block.99999";
    DataStream record(SER_LLD, LLD::DATABASE_VERSION);
    record << std::string("block") << state;
    {
        std::ofstream stream(pathSector, std::ios::binary);
        WriteCompactSize(stream, record.size());
        stream.write(reinterpret_cast<const char*>(record.Bytes().data()), record.size());
    }
    const auto ReadSector = [&]()
    {
        std::ifstream stream(pathSector, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    const std::string sectorBefore = ReadSector();
    auto summary = Run({target, "-auditblockstartfile=99999", "-auditblockfiles=2", "-lldmeters=1"}, 0);
    REQUIRE(summary["status"] == "FOUND");
    REQUIRE(summary["classification"] == "HASH_ALIAS_MISSING_RAW_RECORD_PRESENT");
    REQUIRE(summary["raw_scan"]["scan_start_file"] == 99999);
    REQUIRE(summary["raw_scan"]["scan_end_file"] == 99999);
    REQUIRE(summary["raw_scan"]["files_scanned"] == 1);
    REQUIRE(summary["hash_key"]["exists"] == false);
    REQUIRE(summary["height_index"]["checked"] == true);
    REQUIRE(summary["height_index"]["height"] == 42);
    REQUIRE(summary["height_index"]["exists"] == false);
    REQUIRE(ReadSector() == sectorBefore);
    REQUIRE_FALSE(std::filesystem::exists(pathLedger / "keychain"));
    REQUIRE_FALSE(std::filesystem::exists(pathLedger / "datachain/_block.00000"));

    summary = Run({target, "-auditblockstartfile=99998"}, 0);
    REQUIRE(summary["status"] == "NOT_FOUND");
    REQUIRE(summary["classification"] == "BLOCK_NOT_FOUND_IN_BOUNDED_SCAN");
    {
        std::ofstream stream(pathSector, std::ios::binary | std::ios::trunc);
        WriteCompactSize(stream, record.size() + 1);
        stream.write(reinterpret_cast<const char*>(record.Bytes().data()), record.size());
        stream.put('\0');
    }
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["status"] == "NOT_FOUND");
    REQUIRE(summary["classification"] == "RAW_RECORD_TRUNCATED_OR_MALFORMED");
    REQUIRE(summary["raw_scan"]["found"] == false);
    REQUIRE(summary["candidate"]["serialized_complete"] == false);
    REQUIRE(summary["height_index"]["checked"] == false);

    {
        std::ofstream stream(pathSector, std::ios::binary | std::ios::trunc);
        WriteCompactSize(stream, MAX_SIZE);
        const std::string type = "other";
        WriteCompactSize(stream, type.size());
        stream.write(type.data(), type.size());
    }
    std::filesystem::resize_file(pathSector, uint64_t(MAX_SIZE) + GetSizeOfCompactSize(MAX_SIZE));
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["status"] == "NOT_FOUND");
    REQUIRE(summary["raw_scan"]["malformed_record"] == false);
    REQUIRE(summary["raw_scan"]["truncated_record"] == false);
    REQUIRE(summary["raw_scan"]["records_scanned"] == 1);

    for(const uint64_t nPayloadSize : {uint64_t(MAX_SIZE) + 1, uint64_t(LLD::MAX_SECTOR_FILE_SIZE)})
    {
        {
            std::ofstream stream(pathSector, std::ios::binary | std::ios::trunc);
            WriteCompactSize(stream, nPayloadSize);
        }
        std::filesystem::resize_file(pathSector, std::filesystem::file_size(pathSector) + nPayloadSize);
        summary = Run({target, "-auditblockstartfile=99999"}, 0);
        REQUIRE(summary["status"] == "NOT_FOUND");
        REQUIRE(summary["raw_scan"]["malformed_record"] == true);
        REQUIRE(summary["raw_scan"]["truncated_record"] == false);
        REQUIRE(summary["raw_scan"]["records_scanned"] == 0);
    }

    std::ofstream(pathSector, std::ios::binary | std::ios::trunc).close();
    std::filesystem::resize_file(pathSector, LLD::MAX_SECTOR_FILE_SIZE);
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["status"] == "NOT_FOUND");
    REQUIRE(summary["raw_scan"]["records_scanned"] == 0);
    REQUIRE(summary["raw_scan"]["malformed_record"] == false);
    REQUIRE(summary["raw_scan"]["truncated_record"] == false);

    const std::vector<uint64_t> offsets{1, 65537, 131074,
        uint64_t(LLD::MAX_SECTOR_FILE_SIZE) - record.size() - GetSizeOfCompactSize(record.size())};
    {
        std::fstream stream(pathSector, std::ios::binary | std::ios::in | std::ios::out);
        for(const auto nOffset : offsets)
        {
            stream.seekp(nOffset);
            WriteCompactSize(stream, record.size());
            stream.write(reinterpret_cast<const char*>(record.Bytes().data()), record.size());
        }
    }
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["status"] == "FOUND");
    REQUIRE(summary["classification"] == "MULTIPLE_RAW_MATCHES");
    REQUIRE(summary["raw_scan"]["records_scanned"] == offsets.size());
    REQUIRE(summary["raw_scan"]["matches"] == offsets.size());
    REQUIRE(summary["raw_scan"]["malformed_record"] == false);
    REQUIRE(summary["raw_scan"]["truncated_record"] == false);
    REQUIRE(summary["height_index"]["matches"] == false);
    for(size_t i = 0; i < offsets.size(); ++i)
        REQUIRE(summary["candidates"][i]["sector_offset"] == offsets[i]);

    const uint64_t nMaxScanSize = uint64_t(LLD::MAX_SECTOR_FILE_SIZE) + MAX_SIZE + GetSizeOfCompactSize(MAX_SIZE);
    std::ofstream(pathSector, std::ios::binary | std::ios::trunc).close();
    std::filesystem::resize_file(pathSector, nMaxScanSize);
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["raw_scan"]["files_scanned"] == 1);
    REQUIRE(summary["raw_scan"]["files_skipped"] == 0);
    REQUIRE(summary["raw_scan"]["oversized_file"] == false);

    std::filesystem::resize_file(pathSector, nMaxScanSize + 1);
    summary = Run({target, "-auditblockstartfile=99999"}, 0);
    REQUIRE(summary["status"] == "NOT_FOUND");
    REQUIRE(summary["classification"] == "RAW_SECTOR_FILE_OVERSIZED");
    REQUIRE(summary["raw_scan"]["oversized_file"] == true);
    REQUIRE(summary["raw_scan"]["files_scanned"] == 0);
    REQUIRE(summary["raw_scan"]["files_skipped"] == 1);
    REQUIRE(summary["raw_scan"]["records_scanned"] == 0);

    std::filesystem::remove(pathSector);
    std::filesystem::create_directory(pathSector);
    Run({target, "-auditblockstartfile=99999", "-lldmeters=1"}, 3);

    std::filesystem::remove(pathSector);
    std::filesystem::create_directories(pathIndex.parent_path());
    std::ofstream(pathIndex, std::ios::binary).close();
    std::filesystem::resize_file(pathIndex, uint64_t(nBuckets) * 4);
    {
        Database writer(strDatabase, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, nBuckets, 1024);
        REQUIRE(writer.Write(hash, state, "block"));
    }

    {
        const auto pathDuplicate = pathLedger / "datachain/_block.65535";
        DataStream recordDuplicate(SER_LLD, LLD::DATABASE_VERSION);
        recordDuplicate << std::string("block") << state;
        std::ofstream stream(pathDuplicate, std::ios::binary);
        WriteCompactSize(stream, recordDuplicate.size());
        stream.write(reinterpret_cast<const char*>(recordDuplicate.Bytes().data()), recordDuplicate.size());
        stream.close();

        LLD::BinaryHashMap keychain(config::GetDataDir() + strDatabase + "/keychain/",
            LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, nBuckets);
        DataStream ssHashKey(SER_LLD, LLD::DATABASE_VERSION);
        ssHashKey << hash;

        DataStream ssHeightKey(SER_LLD, LLD::DATABASE_VERSION);
        ssHeightKey << std::make_pair(std::string("height"), uint32_t(42));

        LLD::SectorKey cHash;
        REQUIRE(keychain.Get(ssHashKey.Bytes(), cHash));
        LLD::SectorKey cHeight(cHash);
        cHeight.nSectorFile = 65535;
        cHeight.nSectorStart = 0;
        cHeight.nSectorSize = recordDuplicate.size() + GetSizeOfCompactSize(recordDuplicate.size());
        cHeight.SetKey(ssHeightKey.Bytes());
        REQUIRE(keychain.Put(cHeight));

        auto summary = Run({target, "-auditblockheight=42", "-auditblockstartfile=65535",
            "-auditblockendfile=65535"}, 0);
        REQUIRE(summary["status"] == "FOUND");
        REQUIRE(summary["classification"] == "HASH_HEIGHT_ALIAS_DIFFERENT_SECTORS");
        REQUIRE(summary["hash_key"]["matches"] == true);
        REQUIRE(summary["height_index"]["matches"] == true);
        REQUIRE(summary["alias_relation"]["comparable"] == true);
        REQUIRE(summary["alias_relation"]["same_sector"] == false);
    }
}
#endif


TEST_CASE("Ledger raw block audit scan reports hash/height/raw availability without mutation",
          "[ledger][auditblock][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard     chainGuard;
    BestChainDiskGuard  bestChainGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    ArgsMapGuard        argsGuard;
    FlagGuard           flagGuard;
    GenesisDiskGuard    genesisGuard;
    HeightIndexDiskGuard heightOneGuard(1);
    HeightIndexDiskGuard heightTwoGuard(2);

    config::fClient.store(false);
    config::fHybrid.store(false);
    config::fTestNet.store(false);
    TAO::Ledger::ChainState::tStateGenesis = TAO::Ledger::LegacyGenesis();
    REQUIRE(TAO::Ledger::ChainState::tStateGenesis.GetHash() == TAO::Ledger::ChainState::Genesis());

    SECTION("hash key readable and consistent")
    {
        const auto fixture = BuildCheckpointChainFixture(38300, blocksGuard);
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));

        TAO::Ledger::BlockState byHash;
        REQUIRE(LLD::Ledger->ReadBlock(fixture.hashTwo, byHash));
        REQUIRE(byHash.GetHash() == fixture.hashTwo);
        REQUIRE(byHash.nHeight == 2);

        LLD::BlockAuditScanOptions options;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fFound);
        REQUIRE_FALSE(result.vMatches.empty());
        REQUIRE(result.vMatches.front().hashBlock == fixture.hashTwo);
    }

    SECTION("default bounded scan window derived from current file finds a recent block")
    {
        const auto fixture = BuildCheckpointChainFixture(38350, blocksGuard);
        REQUIRE(LLD::TxnBegin(LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashThree, fixture.three));
        REQUIRE(LLD::TxnCommit());

        LLD::BlockAuditScanOptions options;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashThree, options, result));
        REQUIRE(result.nScanEndFile >= result.nScanStartFile);
        REQUIRE(result.nFilesScanned >= 1);
        REQUIRE(result.fFound);
    }

    SECTION("hash missing but height alias locates exact block")
    {
        const auto fixture = BuildCheckpointChainFixture(38400, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashTwo));

        TAO::Ledger::BlockState byHeight;
        REQUIRE(LLD::Ledger->ReadBlock(uint32_t(2), byHeight));
        REQUIRE(byHeight.GetHash() == fixture.hashTwo);
    }

    SECTION("hash and height aliases missing but raw scan finds physical location")
    {
        const auto fixture = BuildCheckpointChainFixture(38500, blocksGuard);
        REQUIRE(LLD::TxnBegin(LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, fixture.two));
        REQUIRE(LLD::TxnCommit());

        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        REQUIRE(LLD::Ledger->Erase(fixture.hashTwo, true));
        REQUIRE(LLD::Ledger->Erase(std::make_pair(std::string("height"), uint32_t(2)), true));
        REQUIRE_FALSE(LLD::Ledger->HasBlock(fixture.hashTwo));
        REQUIRE_FALSE(LLD::Ledger->Exists(std::make_pair(std::string("height"), uint32_t(2))));

        LLD::BlockAuditScanOptions options;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fFound);
        REQUIRE(result.vMatches.size() == 1);
        REQUIRE(result.vMatches.front().nHeight == 2);
        REQUIRE(result.vMatches.front().nSectorSize > 0);
    }

    SECTION("hash key unreadable while raw scan still finds a matching block record")
    {
        const auto fixture = BuildCheckpointChainFixture(38600, blocksGuard);
        REQUIRE(LLD::TxnBegin(LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, fixture.two));
        REQUIRE(LLD::TxnCommit());

        REQUIRE(LLD::Ledger->Write(fixture.hashTwo, uint8_t(0)));
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));
        TAO::Ledger::BlockState unreadable;
        REQUIRE_FALSE(LLD::Ledger->ReadBlock(fixture.hashTwo, unreadable));

        LLD::BlockAuditScanOptions options;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fFound);
    }

    SECTION("height index points to a different block")
    {
        const auto fixture = BuildCheckpointChainFixture(38700, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashOne));

        TAO::Ledger::BlockState byHeight;
        REQUIRE(LLD::Ledger->ReadBlock(uint32_t(2), byHeight));
        REQUIRE(byHeight.GetHash() == fixture.hashOne);
        REQUIRE(byHeight.GetHash() != fixture.hashTwo);
    }

    SECTION("bounded scan misses block and reports searched range")
    {
        const auto fixture = BuildCheckpointChainFixture(38800, blocksGuard);
        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = true;
        options.fHasEndFile = true;
        options.nStartFile = 77777;
        options.nEndFile = 77778;

        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE_FALSE(result.fFound);
        REQUIRE(result.nScanStartFile == 77777);
        REQUIRE(result.nScanEndFile == 77778);
    }

    SECTION("read-only ledger aliases remain readable in client mode")
    {
        const auto fixture = BuildCheckpointChainFixture(38810, blocksGuard);
        REQUIRE(LLD::Ledger->IndexBlock(uint32_t(2), fixture.hashTwo));
        config::fClient.store(true);
        LLD::LedgerDB reader(0);
        REQUIRE(reader.Exists(fixture.hashTwo));
        TAO::Ledger::BlockState state;
        REQUIRE(reader.Read(fixture.hashTwo, state));
        REQUIRE(state.GetHash() == fixture.hashTwo);
        REQUIRE(reader.ReadBlock(uint32_t(2), state));
        REQUIRE(state.GetHash() == fixture.hashTwo);
        config::fClient.store(false);
    }

    SECTION("raw scan reads non-writable sectors and rejects inaccessible sectors")
    {
        const auto fixture = BuildCheckpointChainFixture(38820, blocksGuard);
        const std::filesystem::path path = config::GetDataDir() + "_LEDGER/datachain/_block.99997";
        struct SectorGuard
        {
            std::filesystem::path path;
            ~SectorGuard()
            {
                std::filesystem::permissions(path, std::filesystem::perms::owner_all);
                std::filesystem::remove_all(path);
            }
        } sectorGuard{path};

        {
            std::ofstream stream(path, std::ios::binary);
            REQUIRE(stream.is_open());
            DataStream record(SER_LLD, LLD::DATABASE_VERSION);
            record << std::string("block") << fixture.two;
            WriteCompactSize(stream, record.size());
            stream.write(reinterpret_cast<const char*>(record.Bytes().data()), record.size());
        }
        std::filesystem::permissions(path, std::filesystem::perms::owner_read);

        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = options.fHasEndFile = true;
        options.nStartFile = options.nEndFile = 99997;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fFound);
        REQUIRE(result.nFilesScanned == 1);
        REQUIRE_FALSE(result.fInfrastructureFailure);

        std::filesystem::permissions(path, std::filesystem::perms::none);
        std::ifstream probe(path, std::ios::binary);
        if(!probe.is_open())
        {
            REQUIRE_FALSE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
            REQUIRE(result.fInfrastructureFailure);
        }
        probe.close();

        std::filesystem::permissions(path, std::filesystem::perms::owner_all);
        REQUIRE(std::filesystem::remove(path));
        REQUIRE(std::filesystem::create_directory(path));
        REQUIRE_FALSE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fInfrastructureFailure);
    }

    SECTION("malformed and truncated records are reported without scan infrastructure failure")
    {
        const auto fixture = BuildCheckpointChainFixture(38900, blocksGuard);

        const std::string strCorruptPath = debug::safe_printstr(
            config::GetDataDir(), "_LEDGER/datachain/_block.99999");

        {
            std::ofstream out(strCorruptPath, std::ios::binary | std::ios::out | std::ios::trunc);
            REQUIRE(out.is_open());

            DataStream ssMalformed(SER_LLD, LLD::DATABASE_VERSION);
            ssMalformed << std::string("block");
            ssMalformed << uint8_t(33);
            WriteCompactSize(out, ssMalformed.size());
            const std::vector<uint8_t>& vMalformed = ssMalformed.Bytes();
            out.write(reinterpret_cast<const char*>(vMalformed.data()), static_cast<std::streamsize>(vMalformed.size()));

            WriteCompactSize(out, uint64_t(64));
            const uint8_t pShort[3] = {1, 2, 3};
            out.write(reinterpret_cast<const char*>(pShort), 3);
        }

        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = true;
        options.fHasEndFile = true;
        options.nStartFile = 99999;
        options.nEndFile = 99999;

        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE((result.fMalformedRecord || result.fTruncatedRecord));

        std::remove(strCorruptPath.c_str());
    }

    SECTION("scan continues past malformed record when boundaries are known")
    {
        const auto fixture = BuildCheckpointChainFixture(38950, blocksGuard);

        const std::string strCorruptPath = debug::safe_printstr(
            config::GetDataDir(), "_LEDGER/datachain/_block.99998");

        {
            std::ofstream out(strCorruptPath, std::ios::binary | std::ios::out | std::ios::trunc);
            REQUIRE(out.is_open());

            DataStream ssMalformed(SER_LLD, LLD::DATABASE_VERSION);
            ssMalformed << std::string("block");
            ssMalformed << uint8_t(33);
            WriteCompactSize(out, ssMalformed.size());
            const std::vector<uint8_t>& vMalformed = ssMalformed.Bytes();
            out.write(reinterpret_cast<const char*>(vMalformed.data()), static_cast<std::streamsize>(vMalformed.size()));

            DataStream ssValid(SER_LLD, LLD::DATABASE_VERSION);
            ssValid << std::string("block");
            ssValid << fixture.three;
            WriteCompactSize(out, ssValid.size());
            const std::vector<uint8_t>& vValid = ssValid.Bytes();
            out.write(reinterpret_cast<const char*>(vValid.data()), static_cast<std::streamsize>(vValid.size()));
        }

        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = true;
        options.fHasEndFile = true;
        options.nStartFile = 99998;
        options.nEndFile = 99998;

        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashThree, options, result));
        REQUIRE(result.fMalformedRecord);
        REQUIRE(result.fFound);
        REQUIRE(result.vMatches.size() >= 1);

        std::remove(strCorruptPath.c_str());
    }

    SECTION("trailing bytes are malformed evidence, not a complete raw match")
    {
        const auto fixture = BuildCheckpointChainFixture(38975, blocksGuard);
        const std::filesystem::path path = config::GetDataDir() + "_LEDGER/datachain/_block.99998";
        struct SectorGuard
        {
            std::filesystem::path path;
            ~SectorGuard() { std::filesystem::remove(path); }
        } sectorGuard{path};

        DataStream record(SER_LLD, LLD::DATABASE_VERSION);
        record << std::string("block") << fixture.three;
        const auto WriteRecord = [&](const bool fTrailing, const bool fAppend)
        {
            std::ofstream stream(path, std::ios::binary | (fAppend ? std::ios::app : std::ios::trunc));
            REQUIRE(stream.is_open());
            WriteCompactSize(stream, record.size() + (fTrailing ? 1 : 0));
            stream.write(reinterpret_cast<const char*>(record.Bytes().data()), record.size());
            if(fTrailing)
                stream.put('\0');
        };

        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = options.fHasEndFile = true;
        options.nStartFile = options.nEndFile = 99998;
        LLD::BlockAuditScanResult result;
        WriteRecord(true, false);
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashThree, options, result));
        REQUIRE(result.fMalformedRecord);
        REQUIRE_FALSE(result.fFound);
        REQUIRE(result.vMatches.size() == 1);
        REQUIRE_FALSE(result.vMatches.front().fSerializedComplete);

        WriteRecord(false, true);
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashThree, options, result));
        REQUIRE(result.fMalformedRecord);
        REQUIRE(result.fFound);
        REQUIRE(result.vMatches.size() == 2);
        REQUIRE(result.vMatches.back().fSerializedComplete);

        WriteRecord(false, false);
        WriteRecord(true, true);
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashThree, options, result));
        REQUIRE(result.fMalformedRecord);
        REQUIRE(result.fFound);
        REQUIRE(result.vMatches.front().fSerializedComplete);
        REQUIRE_FALSE(result.vMatches.back().fSerializedComplete);
    }

    SECTION("multiple physical block records matching one hash are reported")
    {
        const auto fixture = BuildCheckpointChainFixture(39000, blocksGuard);
        REQUIRE(LLD::TxnBegin(LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->WriteBlock(fixture.hashTwo, fixture.two));
        REQUIRE(LLD::TxnCommit());

        const std::pair<std::string, uint32_t> duplicateKey =
            std::make_pair(std::string("audit-duplicate-block"), uint32_t(2));
        REQUIRE(LLD::TxnBegin(LLD::INSTANCES::LEDGER));
        REQUIRE(LLD::Ledger->Write(duplicateKey, fixture.two, "block"));
        REQUIRE(LLD::TxnCommit());

        LLD::BlockAuditScanOptions options;
        LLD::BlockAuditScanResult result;
        REQUIRE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.vMatches.size() >= 2);

        LLD::Ledger->Erase(duplicateKey);
    }

    SECTION("invalid scan range fails cleanly without mutation")
    {
        const auto fixture = BuildCheckpointChainFixture(39100, blocksGuard);
        const uint1024_t hashBestBefore = TAO::Ledger::ChainState::hashBestChain.load();

        LLD::BlockAuditScanOptions options;
        options.fHasStartFile = true;
        options.fHasEndFile = true;
        options.nStartFile = 8;
        options.nEndFile = 7;

        LLD::BlockAuditScanResult result;
        REQUIRE_FALSE(LLD::Ledger->AuditScanBlockRecords(fixture.hashTwo, options, result));
        REQUIRE(result.fRangeInvalid);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashBestBefore);
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == fixture.hashThree);
        REQUIRE(LLD::Ledger->HasBlock(fixture.hashTwo));
    }
}


TEST_CASE("LLD::TxnRecovery retains complete journals after partial CONSENSUS apply failure",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;

    /* Apply earlier CONSENSUS participants successfully, then fail a later one
     * so recovery stops after a real partial apply and retains every journal. */
    const std::pair<std::string, uint32_t> contractKey =
        std::make_pair(std::string("recovery-contract"), 1);
    const std::pair<std::string, uint32_t> registerKey =
        std::make_pair(std::string("recovery-register"), 1);
    const std::pair<std::string, uint32_t> trustKey =
        std::make_pair(std::string("recovery-trust"), 1);

    LLD::Contract->Erase(contractKey);
    LLD::Register->Erase(registerKey);
    LLD::Trust->Erase(trustKey);

    REQUIRE(WriteRecoveryJournal("_CONTRACT", MakeWriteJournal(contractKey, 10)));
    REQUIRE(WriteRecoveryJournal("_REGISTER", MakeWriteJournal(registerKey, 11)));
    REQUIRE(WriteRecoveryJournal("_TRUST", MakeFailingIndexJournal()));
    REQUIRE(WriteRecoveryJournal("_LEGACY", MakeWriteJournal(
        std::make_pair(std::string("recovery-legacy"), 1), 13)));
    REQUIRE(WriteRecoveryJournal("_LEDGER", MakeWriteJournal(
        std::make_pair(std::string("recovery-ledger"), 1), 14)));

    const uint64_t nContractJournal = JournalSize("_CONTRACT");
    const uint64_t nRegisterJournal = JournalSize("_REGISTER");
    const uint64_t nTrustJournal = JournalSize("_TRUST");
    const uint64_t nLegacyJournal = JournalSize("_LEGACY");
    const uint64_t nLedgerJournal = JournalSize("_LEDGER");

    REQUIRE(nContractJournal > 0);
    REQUIRE(nRegisterJournal > 0);
    REQUIRE(nTrustJournal > 0);
    REQUIRE(nLegacyJournal > 0);
    REQUIRE(nLedgerJournal > 0);

    REQUIRE_FALSE(LLD::TxnRecovery());

    REQUIRE(LLD::Contract->Exists(contractKey));
    REQUIRE(LLD::Register->Exists(registerKey));
    REQUIRE_FALSE(LLD::Trust->Exists(trustKey));

    REQUIRE(JournalSize("_CONTRACT") == nContractJournal);
    REQUIRE(JournalSize("_REGISTER") == nRegisterJournal);
    REQUIRE(JournalSize("_TRUST") == nTrustJournal);
    REQUIRE(JournalSize("_LEGACY") == nLegacyJournal);
    REQUIRE(JournalSize("_LEDGER") == nLedgerJournal);

    LLD::ResetTxnRecoveryRequired();
    REQUIRE(LLD::Contract->TxnRelease());
    REQUIRE(LLD::Register->TxnRelease());
    REQUIRE(LLD::Trust->TxnRelease());
    REQUIRE(LLD::Legacy->TxnRelease());
    REQUIRE(LLD::Ledger->TxnRelease());

    LLD::Contract->Erase(contractKey);
    LLD::Register->Erase(registerKey);
}


TEST_CASE("Recovery rolls forward a partial apply with empty group participants",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    ClientGuard clientGuard;
    LogicalGuard logicalGuard;
    struct ModeGuard
    {
        const bool saved = config::fClient.load();
        ~ModeGuard() { config::fClient.store(saved); }
    } modeGuard;
    const bool fClient = GENERATE(false, true);
    config::fClient.store(fClient);
    const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK;
    const uint16_t nInstances = fClient ? LLD::INSTANCES::MERKLE : LLD::INSTANCES::CONSENSUS;
    const auto key = std::make_pair(std::string("recovery-partial-empty"), uint32_t(fClient));
    LLD::Contract->Erase(key);
    LLD::Ledger->Erase(key);
    LLD::Client->Erase(key);

    LLD::TransactionGuard transaction(nFlags, nInstances);
    REQUIRE(transaction);
    REQUIRE(LLD::Contract->Write(key, uint32_t(71)));
    if(fClient)
        REQUIRE(LLD::Client->Write(key, uint32_t(72)));
    else
        REQUIRE(LLD::Ledger->Write(key, uint32_t(72)));

    REQUIRE(LLD::Contract->TxnCheckpoint());
    REQUIRE(LLD::Register->TxnCheckpoint());
    if(fClient)
    {
        REQUIRE(LLD::Logical->TxnCheckpoint());
        REQUIRE(LLD::Client->TxnCheckpoint());
    }
    else
    {
        REQUIRE(LLD::Trust->TxnCheckpoint());
        REQUIRE(LLD::Legacy->TxnCheckpoint());
        REQUIRE(LLD::Ledger->TxnCheckpoint());
    }
    REQUIRE(LLD::Contract->TxnCommit());
    REQUIRE(LLD::TxnRecovery());
    LLD::TxnAbort(nFlags, nInstances);

    uint32_t nValue = 0;
    REQUIRE(LLD::Contract->Read(key, nValue));
    REQUIRE(nValue == 71);
    if(fClient)
        REQUIRE(LLD::Client->Read(key, nValue));
    else
        REQUIRE(LLD::Ledger->Read(key, nValue));
    REQUIRE(nValue == 72);
    LLD::Contract->Erase(key);
    LLD::Ledger->Erase(key);
    LLD::Client->Erase(key);
}


#ifdef __linux__
TEST_CASE("Failed journal directory sync is retried after abort",
          "[lld][txncommit][durability]")
{
    if(geteuid() == 0)
    {
        WARN("Directory permission failures require a non-root user");
        return;
    }

    const std::string strName = "_JOURNAL_DIRECTORY_SYNC_TEST";
    const std::filesystem::path pathBase = config::GetDataDir() + strName;
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            std::filesystem::permissions(path, std::filesystem::perms::owner_all);
            std::filesystem::remove_all(path);
        }
    } directoryGuard{pathBase};
    LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU> database(
        strName, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
    const auto pathJournal = pathBase / "journal.dat";

    /* Allow journal creation and truncation, but deny opening its directory for sync. */
    std::filesystem::permissions(pathBase,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    for(unsigned int nAttempt = 0; nAttempt < 2; ++nAttempt)
    {
        database.TxnBegin();
        REQUIRE(database.Write(uint32_t(1), uint32_t(77)));
        REQUIRE_FALSE(database.TxnCheckpoint());
        REQUIRE(std::filesystem::file_size(pathJournal) > 0);
        REQUIRE(database.TxnRelease());
        REQUIRE(std::filesystem::file_size(pathJournal) == 0);
        REQUIRE_FALSE(database.Exists(uint32_t(1)));
    }

    std::filesystem::permissions(pathBase, std::filesystem::perms::owner_all);
    database.TxnBegin();
    REQUIRE(database.Write(uint32_t(1), uint32_t(78)));
    REQUIRE(database.TxnCheckpoint());
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
    uint32_t nValue = 0;
    REQUIRE(database.Read(uint32_t(1), nValue));
    REQUIRE(nValue == 78);

    /* A successfully synced journal needs no directory sync on subsequent checkpoints. */
    std::filesystem::permissions(pathBase,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    database.TxnBegin();
    REQUIRE(database.TxnCheckpoint());
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
}


TEST_CASE("Empty and key-only applies sync newly created sector directories",
          "[lld][txncommit][durability]")
{
    if(geteuid() == 0)
    {
        WARN("Directory permission failures require a non-root user");
        return;
    }

    const bool fKeyOnly = GENERATE(false, true);
    const std::string strName = "_SECTOR_DIRECTORY_SYNC_TEST";
    const std::filesystem::path pathBase = config::GetDataDir() + strName;
    const auto pathSector = pathBase / "datachain";
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            std::filesystem::permissions(path / "datachain", std::filesystem::perms::owner_all);
            std::filesystem::remove_all(path);
        }
    } directoryGuard{pathBase};
    LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU> database(
        strName, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
    REQUIRE(std::filesystem::exists(pathSector / "_block.00000"));

    std::filesystem::permissions(pathSector, std::filesystem::perms::owner_exec);
    database.TxnBegin();
    if(fKeyOnly)
        REQUIRE(database.Write(uint32_t(1)));
    REQUIRE(database.TxnCheckpoint());
    REQUIRE_FALSE(database.TxnCommit());
    REQUIRE_FALSE(database.TxnCommit());
    REQUIRE(std::filesystem::file_size(pathBase / "journal.dat") > 0);

    std::filesystem::permissions(pathSector, std::filesystem::perms::owner_all);
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
    REQUIRE(std::filesystem::file_size(pathBase / "journal.dat") == 0);
    if(fKeyOnly)
        REQUIRE(database.Exists(uint32_t(1)));

    database.TxnBegin();
    REQUIRE(database.Write(uint32_t(2), uint32_t(79)));
    REQUIRE(database.TxnCheckpoint());
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
    uint32_t nValue = 0;
    REQUIRE(database.Read(uint32_t(2), nValue));
    REQUIRE(nValue == 79);
}


TEST_CASE("Transaction commits release journals only after durable apply",
          "[lld][txncommit][durability]")
{
    LedgerGuard ledgerGuard;

    const auto key = std::make_pair(std::string("durable-apply"), 1u);
    LLD::Ledger->Erase(key);

    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->Write(key, uint32_t(77)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(JournalSize("_LEDGER") == 0);

    uint32_t nValue = 0;
    REQUIRE(LLD::Ledger->Read(key, nValue));
    REQUIRE(nValue == 77);

    /* Consecutive commits must each apply durably before releasing journals. */
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->Write(key, uint32_t(78)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(JournalSize("_LEDGER") == 0);
    REQUIRE(LLD::Ledger->Read(key, nValue));
    REQUIRE(nValue == 78);

    LLD::Ledger->Erase(key);
}


TEST_CASE("Later participant sync failures retain all journals for recovery",
          "[lld][txncommit][durability][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    ShutdownGuard shutdownGuard;
    config::fShutdown.store(false);

    const auto key = std::make_pair(std::string("durable-sync-failure"), 1u);
    LLD::Ledger->Erase(key);
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(LLD::Ledger->Write(key, uint32_t(77)));
    REQUIRE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));

    std::filesystem::path path;
    bool fErase = false;
    SECTION("sector data must be synced")
    {
        path = config::GetDataDir() + "_LEDGER/datachain";
    }
    SECTION("keychain data must be synced")
    {
        path = config::GetDataDir() + "_LEDGER/keychain";
        fErase = true;
    }
    const std::filesystem::path backup = path.string() + ".sync-test";
    struct RestoreGuard
    {
        std::filesystem::path path;
        std::filesystem::path backup;
        ~RestoreGuard()
        {
            if(std::filesystem::exists(backup))
                std::filesystem::rename(backup, path);
            LLD::ResetTxnRecoveryRequired();
            LLD::TxnAbort();
            LLD::Contract->TxnRelease();
            LLD::Register->TxnRelease();
            LLD::Trust->TxnRelease();
            LLD::Legacy->TxnRelease();
            LLD::Ledger->TxnRelease();
        }
    } restoreGuard{path, backup};

    /* Cached streams still accept writes, but reopening the file for sync fails.
     * The empty Contract participant must not suppress the later Ledger sync. */
    std::filesystem::rename(path, backup);
    REQUIRE(LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    if(fErase)
        REQUIRE(LLD::Ledger->Erase(key));
    else
        REQUIRE(LLD::Ledger->Write(key, uint32_t(78)));
    REQUIRE_FALSE(LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK, LLD::INSTANCES::LEDGER));
    REQUIRE(config::fShutdown.load());
    for(const auto* name : {"_CONTRACT", "_REGISTER", "_TRUST", "_LEGACY", "_LEDGER"})
        REQUIRE(JournalSize(name) > 0);

    std::filesystem::rename(backup, path);
    REQUIRE(LLD::TxnRecovery());
    for(const auto* name : {"_CONTRACT", "_REGISTER", "_TRUST", "_LEGACY", "_LEDGER"})
        REQUIRE(JournalSize(name) == 0);
    if(fErase)
        REQUIRE_FALSE(LLD::Ledger->Exists(key));
    else
    {
        uint32_t nValue = 0;
        REQUIRE(LLD::Ledger->Read(key, nValue));
        REQUIRE(nValue == 78);
    }
    LLD::Ledger->Erase(key);
}


TEST_CASE("Failed checkpoint writes still require durable journal release",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    const std::filesystem::path path = config::GetDataDir() + "_LEDGER/journal.dat";
    struct JournalGuard
    {
        std::filesystem::path path;
        ~JournalGuard()
        {
            std::filesystem::remove(path);
            LLD::Ledger->TxnRelease();
        }
    } journalGuard{path};
    REQUIRE(LLD::Ledger->TxnRelease());
    std::filesystem::remove(path);
    std::filesystem::create_symlink("/dev/full", path);

    LLD::Ledger->TxnBegin();
    REQUIRE(LLD::Ledger->Write(std::make_pair(std::string("checkpoint-write-failure"), 1u), uint32_t(1)));
    REQUIRE_FALSE(LLD::Ledger->TxnCheckpoint());
    /* /dev/full also rejects fsync: release must attempt it, not silently succeed. */
    REQUIRE_FALSE(LLD::Ledger->TxnRelease());
    REQUIRE(std::filesystem::remove(path));
    REQUIRE(LLD::Ledger->TxnRelease());
    REQUIRE(JournalSize("_LEDGER") == 0);
}
#endif


TEST_CASE("Recovered touched participant still truncates its durable journal on release",
          "[lld][txncommit][recovery][syncprofile]")
{
    LedgerGuard ledgerGuard;
    SyncProfileGuard syncProfileGuard;

    const auto keyLedger = std::make_pair(std::string("syncprofile-recovery-ledger"), 1u);
    LLD::Ledger->Erase(keyLedger);

    REQUIRE(WriteRecoveryJournal("_LEDGER", MakeWriteJournal(keyLedger, 55)));
    REQUIRE(JournalSize("_LEDGER") > 0);
    REQUIRE(LLD::Ledger->TxnRecovery() == LLD::RECOVERY::COMPLETE);
    REQUIRE(LLD::Ledger->TxnCommit());
    REQUIRE(LLD::Ledger->Exists(keyLedger));
    REQUIRE(JournalSize("_LEDGER") > 0);
    REQUIRE(LLD::Ledger->TxnRelease());
    REQUIRE(JournalSize("_LEDGER") == 0);

    const auto snapshot = TAO::Ledger::SyncProfile::GetSnapshot();
    REQUIRE(snapshot.nTxnApplyParticipants == 1);
    REQUIRE(snapshot.nTxnReleaseParticipants == 1);

    LLD::Ledger->Erase(keyLedger);
}


TEST_CASE("LLD::TxnRecovery discards uncommitted journals before a later checkpoint",
          "[lld][txncommit][recovery][syncprofile]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;
    SyncProfileGuard syncProfileGuard;

    const auto abortedKey = std::make_pair(std::string("recovery-aborted"), 1u);
    const auto committedKey = std::make_pair(std::string("recovery-next"), 1u);
    LLD::Ledger->Erase(abortedKey);
    LLD::Ledger->Erase(committedKey);

    for(const auto* name : {"_CONTRACT", "_REGISTER", "_TRUST", "_LEGACY"})
        REQUIRE(WriteRecoveryJournal(name, MakeWriteJournal(abortedKey, 41)));
    REQUIRE(WriteRecoveryJournal("_LEDGER", MakeWriteJournal(abortedKey, 42, false)));
    REQUIRE(JournalSize("_LEDGER") > 0);

    REQUIRE(LLD::TxnRecovery());
    REQUIRE(JournalSize("_LEDGER") == 0);
    REQUIRE_FALSE(LLD::Ledger->Exists(abortedKey));
    REQUIRE_FALSE(LLD::Contract->Exists(abortedKey));
    REQUIRE_FALSE(LLD::Register->Exists(abortedKey));
    REQUIRE_FALSE(LLD::Trust->Exists(abortedKey));
    REQUIRE_FALSE(LLD::Legacy->Exists(abortedKey));
    REQUIRE(TAO::Ledger::SyncProfile::GetSnapshot().nTxnReleaseParticipants == 5);

    /* Simulate another crash after a new checkpoint, without applying it first. */
    LLD::Ledger->TxnBegin();
    REQUIRE(LLD::Ledger->Write(committedKey, uint32_t(43)));
    REQUIRE(LLD::Ledger->TxnCheckpoint());
    REQUIRE(LLD::Ledger->TxnRecovery() == LLD::RECOVERY::COMPLETE);
    REQUIRE(LLD::Ledger->TxnCommit());
    REQUIRE(LLD::Ledger->TxnRelease());
    REQUIRE_FALSE(LLD::Ledger->Exists(abortedKey));
    uint32_t nValue = 0;
    REQUIRE(LLD::Ledger->Read(committedKey, nValue));
    REQUIRE(nValue == 43);
    LLD::Ledger->Erase(committedKey);
}


TEST_CASE("Recovery release leaves missing and empty participant journals untouched",
          "[lld][txncommit][recovery][syncprofile]")
{
    LedgerGuard ledgerGuard;
    SyncProfileGuard syncProfileGuard;
    const std::filesystem::path path = config::GetDataDir() + "_LEDGER/journal.dat";
    REQUIRE(LLD::Ledger->TxnRelease());

    SECTION("missing journal")
    {
        std::filesystem::remove(path);
        REQUIRE(LLD::Ledger->TxnRecovery() == LLD::RECOVERY::INCOMPLETE);
        REQUIRE(LLD::Ledger->TxnRelease());
        REQUIRE_FALSE(std::filesystem::exists(path));
    }
    SECTION("empty journal")
    {
        REQUIRE(WriteRecoveryJournal("_LEDGER", DataStream(SER_LLD, LLD::DATABASE_VERSION)));
        const auto lastWrite = std::filesystem::last_write_time(path);
        REQUIRE(LLD::Ledger->TxnRecovery() == LLD::RECOVERY::INCOMPLETE);
        REQUIRE(LLD::Ledger->TxnRelease());
        REQUIRE(std::filesystem::last_write_time(path) == lastWrite);
        REQUIRE(std::filesystem::file_size(path) == 0);
    }
    REQUIRE(TAO::Ledger::SyncProfile::GetSnapshot().nTxnReleaseParticipants == 0);
}


TEST_CASE("LLD::TxnRecovery retains journals after a CONSENSUS parse failure",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;

    const std::pair<std::string, uint32_t> contractKey =
        std::make_pair(std::string("recovery-parse-contract"), 1);

    REQUIRE(WriteRecoveryJournal("_CONTRACT", MakeWriteJournal(contractKey, 30)));
    REQUIRE(WriteRecoveryJournal("_REGISTER", MakeInvalidRecoveryJournal()));

    const uint64_t nContractJournal = JournalSize("_CONTRACT");
    const uint64_t nRegisterJournal = JournalSize("_REGISTER");
    REQUIRE(nContractJournal > 0);
    REQUIRE(nRegisterJournal > 0);

    REQUIRE_FALSE(LLD::TxnRecovery());
    REQUIRE(JournalSize("_CONTRACT") == nContractJournal);
    REQUIRE(JournalSize("_REGISTER") == nRegisterJournal);

    LLD::ResetTxnRecoveryRequired();
    REQUIRE(LLD::Contract->TxnRelease());
    REQUIRE(LLD::Register->TxnRelease());
}


TEST_CASE("LLD::TxnRecovery retains journals after a CONSENSUS read failure",
          "[lld][txncommit][recovery]")
{
    LedgerGuard ledgerGuard;
    TrustGuard trustGuard;
    LegacyGuard legacyGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;

    const std::pair<std::string, uint32_t> contractKey =
        std::make_pair(std::string("recovery-read-contract"), 1);
    const std::string strRegisterJournal =
        debug::safe_printstr(config::GetDataDir(), "_REGISTER/journal.dat");

    REQUIRE(WriteRecoveryJournal("_CONTRACT", MakeWriteJournal(contractKey, 31)));
    REQUIRE(filesystem::remove(strRegisterJournal));
    REQUIRE(filesystem::create_directories(strRegisterJournal + "/"));

    const uint64_t nContractJournal = JournalSize("_CONTRACT");
    REQUIRE(nContractJournal > 0);

    REQUIRE_FALSE(LLD::TxnRecovery());
    REQUIRE(JournalSize("_CONTRACT") == nContractJournal);
    REQUIRE(filesystem::is_directory(strRegisterJournal));

    LLD::ResetTxnRecoveryRequired();
    REQUIRE(filesystem::remove_directories(strRegisterJournal));
    REQUIRE(LLD::Contract->TxnRelease());
    REQUIRE(LLD::Register->TxnRelease());
}


TEST_CASE("LLD::TxnRecovery retains complete journals after partial MERKLE apply failure",
          "[lld][txncommit][recovery][merkle]")
{
    ClientModeGuard modeGuard;
    ClientGuard clientGuard;
    LogicalGuard logicalGuard;
    ContractGuard contractGuard;
    RegisterGuard registerGuard;

    /* Apply earlier MERKLE participants successfully, then fail a later one. */
    const std::pair<std::string, uint32_t> contractKey =
        std::make_pair(std::string("recovery-merkle-contract"), 1);
    const std::pair<std::string, uint32_t> registerKey =
        std::make_pair(std::string("recovery-merkle-register"), 1);
    const std::pair<std::string, uint32_t> logicalKey =
        std::make_pair(std::string("recovery-logical"), 1);

    LLD::Contract->Erase(contractKey);
    LLD::Register->Erase(registerKey);
    LLD::Logical->Erase(logicalKey);

    REQUIRE(WriteRecoveryJournal("_CONTRACT", MakeWriteJournal(contractKey, 20)));
    REQUIRE(WriteRecoveryJournal("_REGISTER", MakeWriteJournal(registerKey, 21)));
    REQUIRE(WriteRecoveryJournal("_API", MakeFailingIndexJournal()));
    REQUIRE(WriteRecoveryJournal("_CLIENT", MakeWriteJournal(
        std::make_pair(std::string("recovery-client"), 1), 23)));

    const uint64_t nContractJournal = JournalSize("_CONTRACT");
    const uint64_t nRegisterJournal = JournalSize("_REGISTER");
    const uint64_t nLogicalJournal = JournalSize("_API");
    const uint64_t nClientJournal = JournalSize("_CLIENT");

    REQUIRE(nContractJournal > 0);
    REQUIRE(nRegisterJournal > 0);
    REQUIRE(nLogicalJournal > 0);
    REQUIRE(nClientJournal > 0);

    REQUIRE_FALSE(LLD::TxnRecovery());

    REQUIRE(LLD::Contract->Exists(contractKey));
    REQUIRE(LLD::Register->Exists(registerKey));
    REQUIRE_FALSE(LLD::Logical->Exists(logicalKey));

    REQUIRE(JournalSize("_CONTRACT") == nContractJournal);
    REQUIRE(JournalSize("_REGISTER") == nRegisterJournal);
    REQUIRE(JournalSize("_API") == nLogicalJournal);
    REQUIRE(JournalSize("_CLIENT") == nClientJournal);

    LLD::ResetTxnRecoveryRequired();
    REQUIRE(LLD::Contract->TxnRelease());
    REQUIRE(LLD::Register->TxnRelease());
    REQUIRE(LLD::Logical->TxnRelease());
    REQUIRE(LLD::Client->TxnRelease());

    LLD::Contract->Erase(contractKey);
    LLD::Register->Erase(registerKey);
    LLD::Logical->Erase(logicalKey);
}


TEST_CASE("SetBestHeight publishes one supplied head and invalidates caches for every transition",
          "[ledger][setbest_txn][head]")
{
    ChainStateGuard chainGuard;
    TAO::Ledger::BlockState state = chainGuard.savedBest;
    TAO::API::ResponseCache cache({TAO::API::ENABLE::CACHING, &TAO::API::nBlockCounter});
    const encoding::json params = encoding::json::object();
    encoding::json cached, response;
    uint32_t generation = chainGuard.savedBlockCounter;
    for(const uint32_t height : {201u, 201u, 7u, 201u, 0u})
    {
        cached = {{"hash", state.GetHash().ToString()}};
        cache.Insert(params, cached);
        REQUIRE(cache.Get(params, response));
        REQUIRE(response == cached);
        state.nHeight = height;
        ++state.nNonce;
        ++state.nChainTrust;
        TAO::Ledger::ChainState::SetBestHeight(state);
        ++generation;
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load(std::memory_order_acquire) == height);
        REQUIRE(TAO::API::nBlockCounter.load(std::memory_order_acquire) == generation);
        REQUIRE_FALSE(cache.Get(params, response));
        REQUIRE_FALSE(cache.Get(params, response));
        REQUIRE(TAO::Ledger::ChainState::nBestChainTrust.load() == state.nChainTrust);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == state.GetHash());
        const auto snapshot = TAO::Ledger::ChainState::tStateBest.load();
        REQUIRE(snapshot.GetHash() == state.GetHash());
        REQUIRE(snapshot.nHeight == height);
    }

    state.nHeight = 400;
    ++generation;
    const auto expectedHash = state.GetHash();
    auto reader = std::async(std::launch::async, [&]()
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while(TAO::Ledger::ChainState::nBestHeight.load(std::memory_order_acquire) != state.nHeight)
        {
            if(std::chrono::steady_clock::now() >= deadline)
                return false;
            std::this_thread::yield();
        }
        const auto snapshot = TAO::Ledger::ChainState::tStateBest.load();
        return snapshot.GetHash() == expectedHash
            && TAO::Ledger::ChainState::hashBestChain.load() == expectedHash
            && TAO::Ledger::ChainState::nBestChainTrust.load() == state.nChainTrust
            && TAO::API::nBlockCounter.load() == generation;
    });
    TAO::Ledger::ChainState::SetBestHeight(state);
    REQUIRE(reader.get());
}


TEST_CASE("Genesis SetBest publishes the complete head only after commit",
          "[ledger][setbest_txn][head][real]")
{
    RealCodeLedgerGuard ledgerGuard;
    ChainStateGuard chainGuard;
    BestChainDiskGuard bestGuard;
    CheckpointBlocksDiskGuard blocksGuard;
    auto genesis = chainGuard.savedGenesis;
    genesis.nNonce = 0x737465;
    genesis.hashNextBlock = 0;
    blocksGuard.hashes.push_back(genesis.GetHash());
    TAO::Ledger::ChainState::tStateGenesis = TAO::Ledger::BlockState();
    const bool fail = GENERATE(false, true);
    LLD::TransactionGuard transaction;
    REQUIRE(transaction);
    if(fail)
        REQUIRE(LLD::Trust->TxnRelease());
    REQUIRE(genesis.SetBest() == !fail);
    if(fail)
    {
        REQUIRE(TAO::Ledger::ChainState::tStateGenesis.IsNull());
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == chainGuard.savedBestHeight);
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == chainGuard.savedBestHash);
        REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter);
    }
    else
    {
        REQUIRE(TAO::Ledger::ChainState::tStateGenesis.GetHash() == genesis.GetHash());
        REQUIRE(TAO::Ledger::ChainState::tStateBest.load().GetHash() == genesis.GetHash());
        REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == genesis.GetHash());
        REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == genesis.nHeight);
        REQUIRE(TAO::API::nBlockCounter.load() == chainGuard.savedBlockCounter + 1);
    }
}


TEST_CASE("BinaryHashMap sparsely initializes correctly sized collision files",
          "[lld][txncommit][durability][sparse]")
{
    const uint32_t buckets = GENERATE(1u, 4096u);
    const std::filesystem::path path = config::GetDataDir() + "_SPARSE_HASHMAP_TEST";
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard() { std::filesystem::remove_all(path); }
    } guard{path};
    std::filesystem::remove_all(path);
    const std::string base = path.string() + "/";
    std::vector<uint8_t> first, second;
    {
        LLD::BinaryHashMap map(base, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, buckets);
        REQUIRE(std::filesystem::file_size(path / "_hashmap.index") == uint64_t(buckets) * 4);
        REQUIRE(std::filesystem::file_size(path / "_hashmap.00000") == uint64_t(buckets) * 45);
        std::ifstream stream(path / "_hashmap.00000", std::ios::binary);
        const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        REQUIRE(std::all_of(bytes.begin(), bytes.end(), [](char value) { return value == 0; }));

        std::map<uint32_t, std::vector<uint8_t>> seen;
        for(uint32_t n = 0; n <= buckets; ++n)
        {
            DataStream key(SER_LLD, LLD::DATABASE_VERSION);
            key << n;
            const uint32_t bucket = map.GetBucket(key.Bytes());
            const auto result = seen.emplace(bucket, key.Bytes());
            if(!result.second)
            {
                first = result.first->second;
                second = key.Bytes();
                break;
            }
        }
        REQUIRE_FALSE(first.empty());
        REQUIRE(map.Put(LLD::SectorKey(LLD::STATE::READY, first, 0, 12, 9)));
        REQUIRE(map.Put(LLD::SectorKey(LLD::STATE::READY, second, 0, 21, 9)));
        REQUIRE(map.SyncTouchedFiles());
        REQUIRE(std::filesystem::file_size(path / "_hashmap.00001") == uint64_t(buckets) * 45);
        #ifdef __linux__
        if(buckets == 4096)
        {
            struct stat info {};
            REQUIRE(::stat((path / "_hashmap.00001").c_str(), &info) == 0);
            REQUIRE(uint64_t(info.st_blocks) * 512 < uint64_t(info.st_size));
        }
        #endif
    }
    {
        LLD::BinaryHashMap map(base, LLD::FLAGS::READONLY, buckets);
        LLD::SectorKey key;
        REQUIRE(map.Get(first, key));
        REQUIRE(key.nSectorStart == 12);
        REQUIRE(map.Get(second, key));
        REQUIRE(key.nSectorStart == 21);
    }
}


#ifdef __linux__
TEST_CASE("Journal ancestry remains dirty through retry and database reopen",
          "[lld][txncommit][durability]")
{
    if(geteuid() == 0)
    {
        WARN("Directory permission failures require a non-root user");
        return;
    }
    const bool reopen = GENERATE(false, true);
    const std::string name = "_JOURNAL_ANCESTRY_TEST/child";
    const std::filesystem::path parent = config::GetDataDir() + "_JOURNAL_ANCESTRY_TEST";
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            std::filesystem::permissions(path, std::filesystem::perms::owner_all);
            std::filesystem::remove_all(path);
        }
    } guard{parent};
    using Database = LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>;
    auto database = std::make_unique<Database>(name, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
    if(reopen)
    {
        database->TxnBegin();
        REQUIRE(database->TxnCheckpoint());
        REQUIRE(database->TxnCommit());
        REQUIRE(database->TxnRelease());
        database.reset();
        database = std::make_unique<Database>(name, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
    }
    std::filesystem::permissions(parent,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    for(unsigned int attempt = 0; attempt < 2; ++attempt)
    {
        database->TxnBegin();
        REQUIRE(database->Write(uint32_t(1), uint32_t(77)));
        REQUIRE_FALSE(database->TxnCheckpoint());
        REQUIRE(database->TxnRelease());
        REQUIRE_FALSE(database->Exists(uint32_t(1)));
    }
    std::filesystem::permissions(parent, std::filesystem::perms::owner_all);
    database->TxnBegin();
    REQUIRE(database->Write(uint32_t(1), uint32_t(78)));
    REQUIRE(database->TxnCheckpoint());
    REQUIRE(database->TxnCommit());
    REQUIRE(database->TxnRelease());

    /* Warm commits do not rewalk the ancestry for existing files. */
    std::filesystem::permissions(parent,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    database->TxnBegin();
    REQUIRE(database->Write(uint32_t(1), uint32_t(79)));
    REQUIRE(database->TxnCheckpoint());
    REQUIRE(database->TxnCommit());
    REQUIRE(database->TxnRelease());
}


TEST_CASE("Keychain ancestry sync obligations survive repeated failures",
          "[lld][txncommit][durability]")
{
    if(geteuid() == 0)
    {
        WARN("Directory permission failures require a non-root user");
        return;
    }
    const std::filesystem::path parent = config::GetDataDir() + "_KEYCHAIN_ANCESTRY_TEST";
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            std::filesystem::permissions(path, std::filesystem::perms::owner_all);
            std::filesystem::remove_all(path);
        }
    } guard{parent};
    LLD::BinaryHashMap map((parent / "child" / "keychain").string() + "/",
        LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1);
    std::filesystem::permissions(parent,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    REQUIRE_FALSE(map.SyncTouchedFiles());
    map.BeginDurabilityTracking();
    REQUIRE_FALSE(map.SyncTouchedFiles());
    std::filesystem::permissions(parent, std::filesystem::perms::owner_all);
    REQUIRE(map.SyncTouchedFiles());
    std::filesystem::permissions(parent,
        std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec);
    REQUIRE(map.SyncTouchedFiles());
}
#endif


TEST_CASE("Empty apply retains sector writes made outside a transaction until synced",
          "[lld][txncommit][durability]")
{
    const std::string name = "_PENDING_SECTOR_TEST";
    const std::filesystem::path path = config::GetDataDir() + name;
    struct DirectoryGuard
    {
        std::filesystem::path path;
        ~DirectoryGuard()
        {
            if(std::filesystem::exists(path / "backup"))
                std::filesystem::rename(path / "backup", path / "datachain");
            std::filesystem::remove_all(path);
        }
    } guard{path};
    LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU> database(
        name, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE, 1, 1024);
    database.TxnBegin();
    REQUIRE(database.Write(uint32_t(1), uint32_t(77)));
    REQUIRE(database.TxnCheckpoint());
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
    REQUIRE(database.Write(uint32_t(1), uint32_t(78)));
    std::filesystem::rename(path / "datachain", path / "backup");
    database.TxnBegin();
    REQUIRE_FALSE(database.TxnCommit());
    REQUIRE_FALSE(database.TxnCommit());
    std::filesystem::rename(path / "backup", path / "datachain");
    REQUIRE(database.TxnCommit());
    REQUIRE(database.TxnRelease());
    uint32_t value = 0;
    REQUIRE(database.Read(uint32_t(1), value));
    REQUIRE(value == 78);
}
