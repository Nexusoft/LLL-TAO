/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/sync_profile.h>

#include <TAO/Ledger/include/enum.h> //for internal flags

#include <Util/include/runtime.h>
#include <Util/include/signals.h>

#include <mutex>

namespace LLD
{
    /* The LLD global instance pointers. */
    LogicalDB*    Logical;
    SessionDB*    Sessions;
    ContractDB*   Contract;
    RegisterDB*   Register;
    LedgerDB*     Ledger;
    LocalDB*      Local;
    ClientDB*     Client;

    /* For Legacy LLD objects. */
    TrustDB*      Trust;
    LegacyDB*     Legacy;

    /* A failed apply after every journal reached its durable commit marker must
     * preserve those journals for startup recovery and stop further mutation. */
    static std::atomic<bool> fTxnRecoveryRequired{false};


    /* Serialize every transaction that uses shared in-memory transaction objects. */
    static std::recursive_mutex TRANSACTION_COORDINATOR;
    static thread_local bool fTxnOwner = false;
    static thread_local bool fTxnMemoryOnly = false;
    static thread_local uint8_t nTxnOwnerFlags = 0;
    static thread_local uint16_t nTxnOwnerInstances = 0;

    namespace
    {
        uint32_t CountParticipants(const uint16_t nInstances)
        {
            uint32_t nCount = 0;

            if(Logical && (nInstances & INSTANCES::LOGICAL))
                ++nCount;
            if(Contract && (nInstances & INSTANCES::CONTRACT))
                ++nCount;
            if(Register && (nInstances & INSTANCES::REGISTER))
                ++nCount;
            if(Ledger && (nInstances & INSTANCES::LEDGER))
                ++nCount;
            if(Client && (nInstances & INSTANCES::CLIENT))
                ++nCount;
            if(Trust && (nInstances & INSTANCES::TRUST))
                ++nCount;
            if(Legacy && (nInstances & INSTANCES::LEGACY))
                ++nCount;

            return nCount;
        }


        uint32_t CountTouchedParticipants(const uint16_t nInstances)
        {
            uint32_t nCount = 0;

            if(Logical && (nInstances & INSTANCES::LOGICAL) && Logical->HasPendingTransactionWork())
                ++nCount;
            if(Contract && (nInstances & INSTANCES::CONTRACT) && Contract->HasPendingTransactionWork())
                ++nCount;
            if(Register && (nInstances & INSTANCES::REGISTER) && Register->HasPendingTransactionWork())
                ++nCount;
            if(Ledger && (nInstances & INSTANCES::LEDGER) && Ledger->HasPendingTransactionWork())
                ++nCount;
            if(Client && (nInstances & INSTANCES::CLIENT) && Client->HasPendingTransactionWork())
                ++nCount;
            if(Trust && (nInstances & INSTANCES::TRUST) && Trust->HasPendingTransactionWork())
                ++nCount;
            if(Legacy && (nInstances & INSTANCES::LEGACY) && Legacy->HasPendingTransactionWork())
                ++nCount;

            return nCount;
        }
    }

    #ifdef UNIT_TESTS
    static std::function<void()> fnTxnCoordinatorWaitHook;
    #endif

    static void AcquireTransactionCoordinator()
    {
        std::unique_lock<std::recursive_mutex> lock(TRANSACTION_COORDINATOR, std::defer_lock);
        const bool fProfile = TAO::Ledger::SyncProfile::Enabled();
        runtime::timer timerWait;
        if(fProfile)
            timerWait.Start();
        #ifdef UNIT_TESTS
        if(!lock.try_lock())
        {
            if(fnTxnCoordinatorWaitHook)
                fnTxnCoordinatorWaitHook();

            lock.lock();
        }
        #else
        lock.lock();
        #endif
        if(fProfile)
        {
            timerWait.Stop();
            TAO::Ledger::SyncProfile::RecordTxnCoordinatorWait(timerWait.ElapsedMicroseconds());
        }
        lock.release();
    }


    TransactionCoordinatorGuard::TransactionCoordinatorGuard()
    {
        AcquireTransactionCoordinator();
    }


    TransactionCoordinatorGuard::~TransactionCoordinatorGuard()
    {
        TRANSACTION_COORDINATOR.unlock();
    }


    static void ReleaseMemoryTransactions(const uint8_t nFlags, const uint16_t nInstances)
    {
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryRelease(nFlags);

        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryRelease(nFlags);

        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryRelease(nFlags);
    }


    static bool ReleasePhysicalTransactions(const uint16_t nInstances)
    {
        bool fReleased = true;

        if(Logical && (nInstances & INSTANCES::LOGICAL))
            fReleased = Logical->TxnRelease() && fReleased;

        if(Contract && (nInstances & INSTANCES::CONTRACT))
            fReleased = Contract->TxnRelease() && fReleased;

        if(Register && (nInstances & INSTANCES::REGISTER))
            fReleased = Register->TxnRelease() && fReleased;

        if(Ledger && (nInstances & INSTANCES::LEDGER))
            fReleased = Ledger->TxnRelease() && fReleased;

        if(Client && (nInstances & INSTANCES::CLIENT))
            fReleased = Client->TxnRelease() && fReleased;

        if(Trust && (nInstances & INSTANCES::TRUST))
            fReleased = Trust->TxnRelease() && fReleased;

        if(Legacy && (nInstances & INSTANCES::LEGACY))
            fReleased = Legacy->TxnRelease() && fReleased;

        return fReleased;
    }


    static void ReleaseTransactionOwnership()
    {
        if(!fTxnOwner)
            return;

        fTxnOwner = false;
        fTxnMemoryOnly = false;
        nTxnOwnerFlags = 0;
        nTxnOwnerInstances = 0;
        TRANSACTION_COORDINATOR.unlock();
    }


    /*  Initialize the global LLD instances. */
    bool Initialize()
    {
        debug::log(0, FUNCTION, "Initializing LLD");

        try
        {
        /* Create the contract database instance. */
        const uint32_t nContractCacheSize = config::GetArg("-contractcache", 1);
        Contract = new ContractDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        77773,
                        nContractCacheSize * 1024 * 1024);

        /* Create the contract database instance. */
        const uint32_t nRegisterCacheSize = config::GetArg("-registercache", 2);
        Register = new RegisterDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        77773,
                        nRegisterCacheSize * 1024 * 1024);

        /* Create the ledger database instance.
         * Default cache raised to 64 MB for mining node workloads.
         * Blocks average 216 bytes, so 64 MB holds ~300K typical blocks in
         * BinaryLRU cache, dramatically reducing SECTOR_MUTEX contention
         * between P2P block-serving and mining template creation. */
        const uint32_t nLedgerCacheSize = config::GetArg("-ledgercache", 64);
        Ledger    = new LedgerDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        config::fClient.load() ? 77773 : (256 * 256 * 64),
                        nLedgerCacheSize * 1024 * 1024);


        /* Create the legacy database instance. */
        const uint32_t nLegacyCacheSize = config::GetArg("-legacycache", 1);
        Legacy = new LegacyDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        config::fClient.load() ? 77773 : 256 * 256 * 64,
                        nLegacyCacheSize * 1024 * 1024);


        /* Create the trust database instance. */
        Trust  = new TrustDB(
                        FLAGS::CREATE | FLAGS::FORCE);


        /* Create the local database instance. */
        Local    = new LocalDB(
                        FLAGS::CREATE | FLAGS::FORCE);


        /* Create the local database instance. */
        const uint32_t nLogicalCacheSize = config::GetArg("-logicalcache", 2);
        Logical    = new LogicalDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        256 * 256 * config::GetArg("-logicalbuckets", 16), nLogicalCacheSize * 1024 * 1024);

        /* Create the local database instance. */
        const uint32_t nSessionsCacheSize = config::GetArg("-sessionscache", 2);
        Sessions    = new SessionDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        256 * 256 * config::GetArg("-sessionsbuckets", 16), nSessionsCacheSize * 1024 * 1024);

        if(config::fClient.load())
        {
            /* Create new client database if enabled. */
            Client    = new ClientDB(
                            FLAGS::CREATE | FLAGS::FORCE,
                            1000000);
        }

        /* Handle database recovery mode. */
        if(!TxnRecovery())
            return false;

        return true;
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "failed to initialize LLD: ", e.what());
            Shutdown();
            return false;
        }
    }


    /* Run our indexing entries and routines. */
    void Indexing() //TODO: combine all of these into one single indexing routine (include -indexheight)
    {
        debug::log(0, FUNCTION, "Indexing LLD");


        /* Check for reindexing entries. */
        Logical->IndexRegisters();


        /* Check for reindexing entries. */
        Register->IndexAddress();


        /* Check for reindexing entries. */
        Ledger->IndexProofs();
    }


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Cleanup the contract database. */
        delete Contract;
        Contract = nullptr;

        /* Cleanup the ledger database. */
        delete Ledger;
        Ledger = nullptr;

        /* Cleanup the register database. */
        delete Register;
        Register = nullptr;

        /* Cleanup the local database. */
        delete Local;
        Local = nullptr;

        /* Cleanup the client database. */
        delete Client;
        Client = nullptr;

        /* Cleanup the legacy database. */
        delete Legacy;
        Legacy = nullptr;

        /* Cleanup the trust database. */
        delete Trust;
        Trust = nullptr;

        /* Cleanup the logical database. */
        delete Logical;
        Logical = nullptr;

        /* Cleanup the sessions database. */
        delete Sessions;
        Sessions = nullptr;
    }


    /* Check the transactions for recovery. */
    bool TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Check one participant without confusing an incomplete journal with an error. */
        const auto CheckRecovery = [&fRecovery](auto* pDatabase)
        {
            if(!pDatabase)
                return true;

            const RECOVERY nRecovery = pDatabase->TxnRecovery();
            if(nRecovery == RECOVERY::FAILED)
            {
                fTxnRecoveryRequired.store(true);
                return false;
            }

            if(nRecovery == RECOVERY::INCOMPLETE)
                fRecovery = false;

            return true;
        };

        /* Special handle for -client mode. */
        if(config::fClient.load())
        {
            /* Check the contract DB journal. */
            if(!CheckRecovery(Contract))
                return debug::error(FUNCTION, "failed to recover Contract DB journal");

            /* Check the register DB journal. */
            if(!CheckRecovery(Register))
                return debug::error(FUNCTION, "failed to recover Register DB journal");

            /* Check the ledger DB journal. */
            if(!CheckRecovery(Client))
                return debug::error(FUNCTION, "failed to recover Client DB journal");

            /* Check the ledger DB journal. */
            if(!CheckRecovery(Logical))
                return debug::error(FUNCTION, "failed to recover Logical DB journal");

            /* Commit the transactions if journals are recovered. */
            if(fRecovery)
            {
                debug::log(0, FUNCTION, "all transactions are complete, recovering...");

                /* Commit Contract DB transaction. */
                if(Contract && !Contract->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Contract DB recovery commit failed");

                /* Commit Register DB transaction. */
                if(fRecovery && Register && !Register->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Register DB recovery commit failed");

                /* Commit the Logical DB transaction. */
                if(fRecovery && Logical && !Logical->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Logical DB recovery commit failed");

                /* Commit the authoritative Client DB last. */
                if(fRecovery && Client && !Client->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Client DB recovery commit failed");

                if(!fRecovery)
                {
                    fTxnRecoveryRequired.store(true);
                    return debug::error(FUNCTION,
                        "client transaction recovery failed; journals retained for restart");
                }
            }

            /* Clear either the fully applied journals or an incomplete transaction
             * that never reached a durable decision on every participant. */
            if(!ReleasePhysicalTransactions(INSTANCES::MERKLE))
                return debug::error(FUNCTION, "failed to durably release client transaction journals");
        }

        /* Regular mainnet mode recovery. */
        else
        {
            /* Check the contract DB journal. */
            if(!CheckRecovery(Contract))
                return debug::error(FUNCTION, "failed to recover Contract DB journal");

            /* Check the register DB journal. */
            if(!CheckRecovery(Register))
                return debug::error(FUNCTION, "failed to recover Register DB journal");

            /* Check the ledger DB journal. */
            if(!CheckRecovery(Ledger))
                return debug::error(FUNCTION, "failed to recover Ledger DB journal");

            /* Check the ledger DB journal. */
            if(!CheckRecovery(Trust))
                return debug::error(FUNCTION, "failed to recover Trust DB journal");

            /* Check the ledger DB journal. */
            if(!CheckRecovery(Legacy))
                return debug::error(FUNCTION, "failed to recover Legacy DB journal");

            /* Commit the transactions if journals are recovered. */
            if(fRecovery)
            {
                debug::log(0, FUNCTION, "all transactions are complete, recovering...");

                /* Commit contract DB transaction. */
                if(Contract && !Contract->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Contract DB recovery commit failed");

                /* Commit register DB transaction. */
                if(fRecovery && Register && !Register->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Register DB recovery commit failed");

                /* Commit the trust DB transaction. */
                if(fRecovery && Trust && !Trust->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Trust DB recovery commit failed");

                /* Commit the legacy DB transaction. */
                if(fRecovery && Legacy && !Legacy->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Legacy DB recovery commit failed");

                /* Commit the authoritative Ledger DB last. */
                if(fRecovery && Ledger && !Ledger->TxnCommit())
                    fRecovery = debug::error(FUNCTION, "Ledger DB recovery commit failed");

                if(!fRecovery)
                {
                    fTxnRecoveryRequired.store(true);
                    return debug::error(FUNCTION,
                        "consensus transaction recovery failed; journals retained for restart");
                }
            }

            /* Clear either the fully applied journals or an incomplete transaction
             * that never reached a durable decision on every participant. */
            if(!ReleasePhysicalTransactions(INSTANCES::CONSENSUS))
                return debug::error(FUNCTION, "failed to durably release consensus transaction journals");
        }

        return true;
    }


    /* Global handler for all LLD instances. */
    bool HasOpenTransaction(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Memory-only flag modes do not use physical SectorDatabase transactions. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return false;

        /* An open transaction on another thread is not owned by this caller.
         * TxnBegin() will wait for it rather than joining or replacing it. */
        if(!fTxnOwner)
            return false;

        /* Check each database instance that would be opened by TxnBegin(nFlags, nInstances).
         * Any single instance having pTransaction != nullptr means a transaction is open. */
        if(Logical  && (nInstances & INSTANCES::LOGICAL)  && Logical->HasTransaction())
            return true;
        if(Ledger   && (nInstances & INSTANCES::LEDGER)   && Ledger->HasTransaction())
            return true;
        if(Client   && (nInstances & INSTANCES::CLIENT)   && Client->HasTransaction())
            return true;
        if(Contract && (nInstances & INSTANCES::CONTRACT) && Contract->HasTransaction())
            return true;
        if(Register && (nInstances & INSTANCES::REGISTER) && Register->HasTransaction())
            return true;
        if(Trust    && (nInstances & INSTANCES::TRUST)    && Trust->HasTransaction())
            return true;
        if(Legacy   && (nInstances & INSTANCES::LEGACY)   && Legacy->HasTransaction())
            return true;

        return false;
    }


    /* Global handler for all LLD instances. */
    bool TxnBegin(const uint8_t nFlags, const uint16_t nInstances)
    {
        const bool fMemoryOnly =
            (nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE);
        uint16_t nOwnedInstances = nInstances;
        if(!fMemoryOnly)
        {
            if(nInstances & (INSTANCES::CLIENT | INSTANCES::LOGICAL))
                nOwnedInstances = INSTANCES::MERKLE;
            else if(nInstances & (INSTANCES::LEDGER | INSTANCES::TRUST | INSTANCES::LEGACY))
                nOwnedInstances = INSTANCES::CONSENSUS;
            else
                nOwnedInstances = config::fClient.load() ? INSTANCES::MERKLE : INSTANCES::CONSENSUS;
        }

        {
            /* Existing callers intentionally flatten ownership through SetBest(),
             * which checks HasOpenTransaction() before beginning. Refuse any other
             * nested begin rather than deleting the active transaction. */
            if(fTxnOwner)
            {
                debug::error(FUNCTION, "nested transaction begin refused");
                return false;
            }

            AcquireTransactionCoordinator();

            if(fTxnRecoveryRequired.load())
            {
                TRANSACTION_COORDINATOR.unlock();
                debug::error(FUNCTION, "transaction recovery is required; refusing to begin");
                return false;
            }

            fTxnOwner = true;
            fTxnMemoryOnly = fMemoryOnly;
            nTxnOwnerFlags = nFlags;
            nTxnOwnerInstances = nOwnedInstances;
        }

        if(fTxnRecoveryRequired.load())
        {
            ReleaseTransactionOwnership();
            debug::error(FUNCTION, "transaction recovery is required; refusing to begin");
            return false;
        }

        try
        {
            /* Start the contract DB transaction. */
            if(Contract && (nOwnedInstances & INSTANCES::CONTRACT))
                Contract->MemoryBegin(nFlags);

            /* Start the register DB transacdtion. */
            if(Register && (nOwnedInstances & INSTANCES::REGISTER))
                Register->MemoryBegin(nFlags);

            /* Start the ledger DB transaction. */
            if(Ledger && (nOwnedInstances & INSTANCES::LEDGER))
                Ledger->MemoryBegin(nFlags);

            /* Handle memory commits if in memory m ode. */
            if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
                return true;

            /* Start the Logical DB transaction. */
            if(Logical && (nOwnedInstances & INSTANCES::LOGICAL))
                Logical->TxnBegin();

            /* Start the contract DB transaction. */
            if(Contract && (nOwnedInstances & INSTANCES::CONTRACT))
                Contract->TxnBegin();

            /* Start the register DB transacdtion. */
            if(Register && (nOwnedInstances & INSTANCES::REGISTER))
                Register->TxnBegin();

            /* Start the ledger DB transaction. */
            if(Ledger && (nOwnedInstances & INSTANCES::LEDGER))
                Ledger->TxnBegin();

            /* Start the client DB transaction. */
            if(Client && (nOwnedInstances & INSTANCES::CLIENT))
                Client->TxnBegin();

            /* Start the trust DB transaction. */
            if(Trust && (nOwnedInstances & INSTANCES::TRUST))
                Trust->TxnBegin();

            /* Start the legacy DB transaction. */
            if(Legacy && (nOwnedInstances & INSTANCES::LEGACY))
                Legacy->TxnBegin();
        }
        catch(const std::exception& e)
        {
            TxnAbort(nFlags, nOwnedInstances);
            return debug::error(FUNCTION, "failed to start transaction participants: ", e.what());
        }
        catch(...)
        {
            TxnAbort(nFlags, nOwnedInstances);
            return debug::error(FUNCTION, "failed to start transaction participants");
        }

        return true;
    }


    /* Global handler for all LLD instances. */
    bool TxnAbort(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* A redundant outer abort after SetBest() consumed the transaction is a
         * safe no-op, and another thread must never release the owner's state. */
        if(!fTxnOwner)
            return true;

        const bool fMemoryOnly =
            (nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE);
        if(fMemoryOnly != fTxnMemoryOnly || (fMemoryOnly && nFlags != nTxnOwnerFlags))
        {
            debug::error(FUNCTION, "transaction mode does not match current owner");
            return TxnAbort(nTxnOwnerFlags, nTxnOwnerInstances);
        }

        const uint16_t nReleaseInstances = (nInstances | nTxnOwnerInstances);
        ReleaseMemoryTransactions(nFlags, nReleaseInstances);

        /* Handle memory commits if in memory mode. */
        if(fMemoryOnly)
        {
            ReleaseTransactionOwnership();
            return true;
        }

        /* Once a fully checkpointed apply fails, its physical journals are the
         * recovery source of truth and must never be truncated by a caller's
         * ordinary failure cleanup. */
        if(!fTxnRecoveryRequired.load() && !ReleasePhysicalTransactions(nReleaseInstances))
        {
            fTxnRecoveryRequired.store(true);
            ReleaseTransactionOwnership();
            ::Shutdown();
            return debug::error(FUNCTION,
                "failed to durably release aborted transaction journals; shutdown requested");
        }

        ReleaseTransactionOwnership();
        return true;
    }


    /* Global handler for all LLD instances. */
    bool TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Special check if using MINER or SANITIZE flags — intentional short-circuit,
         * not a failure: callers use these flags to prevent accidental commits. */
        if(nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return TxnAbort(nFlags, nInstances);

        if(!fTxnOwner)
            return false;

        const bool fMemoryOnly = (nFlags == TAO::Ledger::FLAGS::MEMPOOL);
        if(fMemoryOnly != fTxnMemoryOnly || (fMemoryOnly && nFlags != nTxnOwnerFlags))
        {
            debug::error(FUNCTION, "transaction mode does not match current owner");
            TxnAbort(nTxnOwnerFlags, nTxnOwnerInstances);
            return false;
        }

        const uint16_t nReleaseInstances = (nInstances | nTxnOwnerInstances);

        /* Memory-pool transactions have no physical journal. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            if(Contract && (nReleaseInstances & INSTANCES::CONTRACT))
                Contract->MemoryCommit();
            if(Register && (nReleaseInstances & INSTANCES::REGISTER))
                Register->MemoryCommit();
            if(Ledger && (nReleaseInstances & INSTANCES::LEDGER))
                Ledger->MemoryCommit();

            ReleaseTransactionOwnership();
            return true;
        }

        const uint32_t nTouchedParticipants = CountTouchedParticipants(nReleaseInstances);
        if(TAO::Ledger::SyncProfile::Enabled())
            TAO::Ledger::SyncProfile::RecordTxnParticipants(
                CountParticipants(nReleaseInstances),
                nTouchedParticipants);

        if(fTxnRecoveryRequired.load())
        {
            ReleaseMemoryTransactions(nFlags, nReleaseInstances);
            ReleaseTransactionOwnership();
            return debug::error(FUNCTION, "transaction recovery is required; refusing to commit");
        }

        /* Every selected journal must reach its commit marker before any participant
         * is applied. A checkpoint failure has no durable global decision and is
         * therefore safe to abort in full. */
        bool fCheckpointsComplete = true;
        const auto Checkpoint = [nTouchedParticipants](auto* pDatabase)
        {
            /* A wholly empty transaction has no recovery decision to record.
             * Still validate ownership and run apply below to drain pending
             * durability obligations. Mixed transactions need every marker. */
            return nTouchedParticipants == 0
                ? pDatabase->HasTransaction()
                : pDatabase->TxnCheckpoint();
        };

        /* Set a checkpoint for Logical DB. */
        if(Logical && (nReleaseInstances & INSTANCES::LOGICAL))
            fCheckpointsComplete = Checkpoint(Logical) && fCheckpointsComplete;

        /* Set a checkpoint for contract DB. */
        if(Contract && (nReleaseInstances & INSTANCES::CONTRACT))
            fCheckpointsComplete = Checkpoint(Contract) && fCheckpointsComplete;

        /* Set a checkpoint for register DB. */
        if(Register && (nReleaseInstances & INSTANCES::REGISTER))
            fCheckpointsComplete = Checkpoint(Register) && fCheckpointsComplete;

        /* Set a checkpoint for ledger DB. */
        if(Ledger && (nReleaseInstances & INSTANCES::LEDGER))
            fCheckpointsComplete = Checkpoint(Ledger) && fCheckpointsComplete;

        /* Set a checkpoint for client DB. */
        if(Client && (nReleaseInstances & INSTANCES::CLIENT))
            fCheckpointsComplete = Checkpoint(Client) && fCheckpointsComplete;

        /* Set a checkpoint for trust DB. */
        if(Trust && (nReleaseInstances & INSTANCES::TRUST))
            fCheckpointsComplete = Checkpoint(Trust) && fCheckpointsComplete;

        /* Set a checkpoint for legacy DB. */
        if(Legacy && (nReleaseInstances & INSTANCES::LEGACY))
            fCheckpointsComplete = Checkpoint(Legacy) && fCheckpointsComplete;

        if(!fCheckpointsComplete)
        {
            TxnAbort(nFlags, nReleaseInstances);
            return debug::error(FUNCTION, "transaction checkpoint failed; all staged changes aborted");
        }

        /* Apply participants in a deterministic order, with the database carrying
         * the authoritative best-chain pointer last. Stop on the first failure;
         * the complete journals are retained so startup can roll the decision
         * forward before the node resumes. */
        bool fAllSucceeded = true;

        /* Commit Logical DB transaction. */
        if(fAllSucceeded && Logical && (nReleaseInstances & INSTANCES::LOGICAL))
        {
            if(!Logical->TxnCommit())
            {
                debug::error(FUNCTION, "Logical DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit contract DB transaction. */
        if(fAllSucceeded && Contract && (nReleaseInstances & INSTANCES::CONTRACT))
        {
            if(!Contract->TxnCommit())
            {
                debug::error(FUNCTION, "Contract DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit register DB transaction. */
        if(fAllSucceeded && Register && (nReleaseInstances & INSTANCES::REGISTER))
        {
            if(!Register->TxnCommit())
            {
                debug::error(FUNCTION, "Register DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the trust DB transaction. */
        if(fAllSucceeded && Trust && (nReleaseInstances & INSTANCES::TRUST))
        {
            if(!Trust->TxnCommit())
            {
                debug::error(FUNCTION, "Trust DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the legacy DB transaction. */
        if(fAllSucceeded && Legacy && (nReleaseInstances & INSTANCES::LEGACY))
        {
            if(!Legacy->TxnCommit())
            {
                debug::error(FUNCTION, "Legacy DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the authoritative full-node pointer last. */
        if(fAllSucceeded && Ledger && (nReleaseInstances & INSTANCES::LEDGER))
        {
            if(!Ledger->TxnCommit())
            {
                debug::error(FUNCTION, "Ledger DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the authoritative client pointer last in MERKLE mode. */
        if(fAllSucceeded && Client && (nReleaseInstances & INSTANCES::CLIENT))
        {
            if(!Client->TxnCommit())
            {
                debug::error(FUNCTION, "Client DB commit failed");
                fAllSucceeded = false;
            }
        }

        if(!fAllSucceeded)
        {
            /* Physical state may now be partially applied. Keep every journal for
             * deterministic roll-forward and stop the process before publication
             * or another transaction can build on the partial state. */
            ReleaseMemoryTransactions(nFlags, nReleaseInstances);

            fTxnRecoveryRequired.store(true);
            ReleaseTransactionOwnership();
            ::Shutdown();
            return debug::error(FUNCTION,
                "durable transaction apply failed; journals retained and shutdown requested");
        }

        /* Publish the in-memory database deltas only after durable apply succeeds. */
        if(Contract && (nReleaseInstances & INSTANCES::CONTRACT))
            Contract->MemoryCommit();
        if(Register && (nReleaseInstances & INSTANCES::REGISTER))
            Register->MemoryCommit();
        if(Ledger && (nReleaseInstances & INSTANCES::LEDGER))
            Ledger->MemoryCommit();

        /* Release the checkpoint markers after every participant succeeds. */
        if(!ReleasePhysicalTransactions(nReleaseInstances))
        {
            fTxnRecoveryRequired.store(true);
            ReleaseTransactionOwnership();
            ::Shutdown();
            return debug::error(FUNCTION,
                "failed to durably release transaction journals; shutdown requested");
        }
        ReleaseTransactionOwnership();

        return fAllSucceeded;
    }


    TransactionGuard::TransactionGuard(const uint8_t nFlagsIn, const uint16_t nInstancesIn)
    : nFlags(nFlagsIn)
    , nInstances(nInstancesIn)
    , fAcquired(TxnBegin(nFlags, nInstances))
    {
    }


    TransactionGuard::~TransactionGuard()
    {
        if(fAcquired)
            TxnAbort(nFlags, nInstances);
    }


    TransactionGuard::operator bool() const
    {
        return fAcquired;
    }


    #ifdef UNIT_TESTS
    void SetTxnCoordinatorWaitHook(const std::function<void()>& fnHook)
    {
        fnTxnCoordinatorWaitHook = fnHook;
    }


    void ResetTxnRecoveryRequired()
    {
        fTxnRecoveryRequired.store(false);
    }
    #endif
}
