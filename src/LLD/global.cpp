/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/enum.h> //for internal flags

#include <condition_variable>
#include <mutex>
#include <thread>

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


    /*  Initialize the global LLD instances. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing LLD");

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
        TxnRecovery();

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


    static void AbortOwnedByThisThread();
    static void BeginTxnShutdown();
    static void WaitAndCloseTxnLifetime();


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Stop new begins first. Abort slots this thread still owns, then wait
         * until every other in-flight call has left and every slot is released.
         * HasOpenTransaction holds that same barrier, so a caller inside the
         * read cannot race these deletes. Deleting earlier lets a worker pass
         * its ownership check and use a freed database, and makes an exiting
         * owner's guard skip cleanup. */
        BeginTxnShutdown();
        AbortOwnedByThisThread();
        WaitAndCloseTxnLifetime();

        /* Cleanup the contract database. */
        if(Contract)
            delete Contract;

        /* Cleanup the ledger database. */
        if(Ledger)
            delete Ledger;

        /* Cleanup the register database. */
        if(Register)
            delete Register;

        /* Cleanup the local database. */
        if(Local)
            delete Local;

        /* Cleanup the client database. */
        if(Client)
            delete Client;

        /* Cleanup the legacy database. */
        if(Legacy)
            delete Legacy;

        /* Cleanup the trust database. */
        if(Trust)
            delete Trust;

        /* Cleanup the logical database. */
        if(Logical)
            delete Logical;

        /* Cleanup the sessions database. */
        if(Sessions)
            delete Sessions;
    }


    /* Check the transactions for recovery. */
    void TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Special handle for -client mode. */
        if(config::fClient.load())
        {
            /* Check the contract DB journal. */
            if(Contract && !Contract->TxnRecovery())
                fRecovery = false;

            /* Check the register DB journal. */
            if(Register && !Register->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Client && !Client->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Logical && !Logical->TxnRecovery())
                fRecovery = false;

            /* Commit the transactions if journals are recovered. */
            if(fRecovery)
            {
                debug::log(0, FUNCTION, "all transactions are complete, recovering...");

                /* Commit Contract DB transaction. */
                if(Contract)
                    Contract->TxnCommit();

                /* Commit Register DB transaction. */
                if(Register)
                    Register->TxnCommit();

                /* Commit the Client DB transaction. */
                if(Client)
                    Client->TxnCommit();

                /* Commit the Logical DB transaction. */
                if(Logical)
                    Logical->TxnCommit();
            }

            /* Abort all the transactions. */
            TxnAbort(TAO::Ledger::FLAGS::BLOCK, INSTANCES::MERKLE);
        }

        /* Regular mainnet mode recovery. */
        else
        {
            /* Check the contract DB journal. */
            if(Contract && !Contract->TxnRecovery())
                fRecovery = false;

            /* Check the register DB journal. */
            if(Register && !Register->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Ledger && !Ledger->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Trust && !Trust->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Legacy && !Legacy->TxnRecovery())
                fRecovery = false;

            /* Commit the transactions if journals are recovered. */
            if(fRecovery)
            {
                debug::log(0, FUNCTION, "all transactions are complete, recovering...");

                /* Commit contract DB transaction. */
                if(Contract)
                    Contract->TxnCommit();

                /* Commit register DB transaction. */
                if(Register)
                    Register->TxnCommit();

                /* Commit ledger DB transaction. */
                if(Ledger)
                    Ledger->TxnCommit();

                /* Commit the trust DB transaction. */
                if(Trust)
                    Trust->TxnCommit();

                /* Commit the legacy DB transaction. */
                if(Legacy)
                    Legacy->TxnCommit();
            }

            /* Abort all the transactions. */
            TxnAbort(TAO::Ledger::FLAGS::BLOCK, INSTANCES::CONSENSUS);
        }


    }


    /* Process-wide owner of each in-flight transaction mode.
     *
     * pMemory, pMiner, pSanitize, and the physical journal are process-wide,
     * not thread-local. MINER and SANITIZE stay on distinct slots so they can
     * run beside MEMPOOL. MEMPOOL and physical BLOCK both map to pMemory, so
     * those two modes are coordinated at begin time: the second begin is
     * rejected before MemoryBegin. A duplicate begin of a slot this thread
     * already owns is also rejected before MemoryBegin, so ClaimSlot cannot
     * be followed by a replacement of the existing overlay. There is no
     * cross-mode coordinator and no post-acquire recovery-required flag. A
     * slot mutex is held across the mutation of that slot only. Shared-overlay
     * checks lock MEMPOOL before PHYSICAL, matching abort and commit.
     *
     * An unowned memory abort or MEMPOOL commit is rejected and does not touch
     * the overlay. MINER and SANITIZE commits remain a no-mutation short-circuit
     * when this thread owns that slot, including when MEMPOOL coexists with it.
     * Sole ownership is not required. A foreign owner, another thread holding
     * the requested slot, rejects that commit even if this thread owns nothing,
     * and does not release the owner. A physical abort or commit is rejected
     * before TxnRelease or TxnCommit unless this thread owns the physical slot.
     * Owning a different mode with no physical slot is a mismatch, not the
     * unowned recovery no-op. TxnBegin returns false on rejection so callers
     * stop before writing into the existing overlay. A redundant physical
     * commit after the journal is already gone returns false and does not
     * checkpoint. MemoryRelease/MemoryCommit of pMemory is skipped when
     * another thread owns the MEMPOOL slot. A commit or abort whose instance
     * mask omits an instance opened by the matching begin is rejected before
     * any overlay or journal mutation and does not release the owner. A
     * covering mask, including a wider one, still applies to every opened
     * instance. An unowned physical abort is unchanged.
     *
     * Ownership is not a bare thread id. ClaimSlot arms a thread-local guard
     * that aborts every slot this thread still owns when the thread exits, so
     * fOwned cannot outlive the owner and a reused thread id cannot act on the
     * stale overlay. Shutdown rejects new begins, aborts slots owned by the
     * calling thread, then waits until in-flight transaction calls, including
     * HasOpenTransaction, and every owned slot have finished before deleting
     * the instances. A physical begin
     * is rejected when a selected journal is already open and this thread does
     * not own it, so SectorDatabase::TxnBegin cannot replace that journal.
     * After the slot is claimed, a throw from MemoryBegin or a selected journal
     * TxnBegin releases only the instances that finished starting, then clears
     * that slot, before the exception leaves TxnBegin. A caller that catches
     * the exception did not observe a successful begin and must not abort it. */
    struct TxnSlot
    {
        std::mutex MUTEX;
        bool fOwned = false;
        std::thread::id idOwner{};
        uint8_t nFlags = 0;
        uint16_t nInstances = 0;
    };


    /* Defined below TxnAbort. Constructed only after a slot is claimed, and
     * destroyed when that thread exits. */
    struct ThreadTxnOwnerGuard
    {
        ~ThreadTxnOwnerGuard();
    };


    /* Barrier shared by TxnBegin, TxnCommit, TxnAbort, HasOpenTransaction, and
     * Shutdown. nInFlight counts callers inside those functions. nOwned counts
     * claimed slots. Shutdown sets fShuttingDown, then waits for both counters
     * before clearing fAlive and deleting the instances. Slot mutexes are never
     * taken while this mutex is held. HasOpenTransaction uses the mutate entry
     * so an in-progress read finishes before deletion, and a closed instance
     * set returns false without being touched. */
    struct TxnLifetime
    {
        std::mutex MUTEX;
        std::condition_variable CV;
        bool fShuttingDown = false;
        bool fAlive = true;
        uint32_t nInFlight = 0;
        uint32_t nOwned = 0;
    };


    static TxnLifetime& Lifetime()
    {
        static TxnLifetime life;
        return life;
    }


    enum class TxnEntry : uint8_t
    {
        BEGIN,
        MUTATE
    };


    /* Holds an in-flight count for one transaction call. Begin is rejected once
     * shutdown has started. Commit and abort stay allowed until the instances
     * are deleted, so an owner can still release its slot. */
    class TxnLifetimeGuard
    {
        bool fHeld;

    public:
        explicit TxnLifetimeGuard(const TxnEntry entry)
        : fHeld(false)
        {
            std::lock_guard<std::mutex> lk(Lifetime().MUTEX);
            TxnLifetime& life = Lifetime();
            if(!life.fAlive)
                return;

            if(entry == TxnEntry::BEGIN && life.fShuttingDown)
                return;

            ++life.nInFlight;
            fHeld = true;
        }

        ~TxnLifetimeGuard()
        {
            if(!fHeld)
                return;

            std::lock_guard<std::mutex> lk(Lifetime().MUTEX);
            --Lifetime().nInFlight;
            Lifetime().CV.notify_all();
        }

        TxnLifetimeGuard(const TxnLifetimeGuard&) = delete;
        TxnLifetimeGuard& operator=(const TxnLifetimeGuard&) = delete;

        explicit operator bool() const
        {
            return fHeld;
        }
    };


    /* Global handler for all LLD instances. */
    bool HasOpenTransaction(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Memory-only flag modes do not use physical SectorDatabase transactions. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return false;

        /* Same barrier as TxnBegin, TxnCommit, and TxnAbort. Shutdown waits for
         * this read before deleting the instances. After they are closed, do
         * not touch the pointers. */
        const TxnLifetimeGuard life(TxnEntry::MUTATE);
        if(!life)
            return false;

        /* Check each database instance that would be opened by TxnBegin(nFlags, nInstances).
         * Any single instance having pTransaction != nullptr means a transaction is open. */
        if(Ledger   && (nInstances & INSTANCES::LEDGER)   && Ledger->HasTransaction())
            return true;
        if(Contract && (nInstances & INSTANCES::CONTRACT) && Contract->HasTransaction())
            return true;
        if(Register && (nInstances & INSTANCES::REGISTER) && Register->HasTransaction())
            return true;
        if(Logical  && (nInstances & INSTANCES::LOGICAL)  && Logical->HasTransaction())
            return true;
        if(Client   && (nInstances & INSTANCES::CLIENT)   && Client->HasTransaction())
            return true;
        if(Trust    && (nInstances & INSTANCES::TRUST)    && Trust->HasTransaction())
            return true;
        if(Legacy   && (nInstances & INSTANCES::LEGACY)   && Legacy->HasTransaction())
            return true;

        return false;
    }


    static void BeginTxnShutdown()
    {
        std::lock_guard<std::mutex> lk(Lifetime().MUTEX);
        Lifetime().fShuttingDown = true;
        Lifetime().CV.notify_all();
    }


    static void WaitAndCloseTxnLifetime()
    {
        std::unique_lock<std::mutex> lk(Lifetime().MUTEX);
        Lifetime().CV.wait(lk, []
        {
            return Lifetime().nInFlight == 0 && Lifetime().nOwned == 0;
        });
        Lifetime().fAlive = false;
    }


    /* Caller holds slot.MUTEX. */
    static void NoteSlotReleased()
    {
        std::lock_guard<std::mutex> lk(Lifetime().MUTEX);
        if(Lifetime().nOwned > 0)
            --Lifetime().nOwned;
        Lifetime().CV.notify_all();
    }


    static void ArmThreadTxnOwnerGuard()
    {
        static thread_local ThreadTxnOwnerGuard guard;
        (void)guard;
    }


    enum class TxnKind : uint8_t
    {
        MEMPOOL,
        MINER,
        SANITIZE,
        PHYSICAL
    };


    static TxnSlot& MempoolSlot()
    {
        static TxnSlot slot;
        return slot;
    }


    static TxnSlot& MinerSlot()
    {
        static TxnSlot slot;
        return slot;
    }


    static TxnSlot& SanitizeSlot()
    {
        static TxnSlot slot;
        return slot;
    }


    static TxnSlot& PhysicalSlot()
    {
        static TxnSlot slot;
        return slot;
    }


    static TxnKind KindOf(const uint8_t nFlags)
    {
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
            return TxnKind::MEMPOOL;

        if(nFlags == TAO::Ledger::FLAGS::MINER)
            return TxnKind::MINER;

        if(nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return TxnKind::SANITIZE;

        return TxnKind::PHYSICAL;
    }


    static TxnSlot& SlotFor(const TxnKind kind)
    {
        switch(kind)
        {
        case TxnKind::MEMPOOL:
            return MempoolSlot();
        case TxnKind::MINER:
            return MinerSlot();
        case TxnKind::SANITIZE:
            return SanitizeSlot();
        default:
            return PhysicalSlot();
        }
    }


    /* Caller holds slot.MUTEX. */
    static bool OwnedByThisThread(const TxnSlot& slot)
    {
        return slot.fOwned && slot.idOwner == std::this_thread::get_id();
    }


    /* Caller holds slot.MUTEX. */
    static bool OwnedByOther(const TxnSlot& slot)
    {
        return slot.fOwned && slot.idOwner != std::this_thread::get_id();
    }


    /* Caller holds slot.MUTEX. True when nInstances includes every instance
     * this begin opened; extras are harmless only when DB operations use slot.nInstances. */
    static bool CoversOwnedInstances(const TxnSlot& slot, const uint16_t nInstances)
    {
        return (slot.nInstances & static_cast<uint16_t>(~nInstances)) == 0;
    }


    /* Bits this thread currently owns. Updated only by this thread, under the
     * corresponding slot mutex, so a same-thread mode mismatch can be rejected
     * without locking every slot. */
    static thread_local uint8_t nThreadOwnedMask = 0;


    static uint8_t MaskOf(const TxnKind kind)
    {
        switch(kind)
        {
        case TxnKind::MEMPOOL:
            return 0x01;
        case TxnKind::MINER:
            return 0x02;
        case TxnKind::SANITIZE:
            return 0x04;
        default:
            return 0x08;
        }
    }


    /* Caller holds slot.MUTEX. Rechecks shutdown admission while accounting for
     * the claim, so a waiting begin cannot claim after shutdown starts. */
    static bool ClaimSlot(TxnSlot& slot, const TxnKind kind, const uint8_t nFlags, const uint16_t nInstances)
    {
        {
            std::lock_guard<std::mutex> lk(Lifetime().MUTEX);
            if(!Lifetime().fAlive || Lifetime().fShuttingDown)
                return false;

            slot.fOwned = true;
            slot.idOwner = std::this_thread::get_id();
            slot.nFlags = nFlags;
            slot.nInstances = nInstances;
            ++Lifetime().nOwned;
        }

        nThreadOwnedMask |= MaskOf(kind);
        ArmThreadTxnOwnerGuard();
        return true;
    }


    /* Caller holds slot.MUTEX. */
    static void ReleaseSlot(TxnSlot& slot, const TxnKind kind)
    {
        slot.fOwned = false;
        slot.idOwner = std::thread::id();
        slot.nFlags = 0;
        slot.nInstances = 0;
        nThreadOwnedMask &= static_cast<uint8_t>(~MaskOf(kind));
        NoteSlotReleased();
    }


    /* Instances whose begin calls returned. The throwing call is not included:
     * MemoryBegin and SectorDatabase::TxnBegin allocate the replacement before
     * deleting the previous pointer, so a throw leaves that instance unchanged. */
    struct BeginProgress
    {
        bool fContractMemory = false;
        bool fRegisterMemory = false;
        bool fLedgerMemory = false;
        bool fLogical = false;
        bool fContract = false;
        bool fRegister = false;
        bool fLedger = false;
        bool fClient = false;
        bool fTrust = false;
        bool fLegacy = false;
    };


    /* Caller must not hold a slot mutex. Releases finished overlays and journals,
     * then the claimed slot. A cleanup failure still clears the slot so a later
     * begin is not rejected forever; the original begin exception is propagated
     * by the caller. */
    static void ReleaseStartedBegin(const uint8_t nFlags, const TxnKind kind, const BeginProgress& started)
    {
        auto ReleaseMemory = [&](const bool fStarted, auto* pDB)
        {
            if(!fStarted || !pDB)
                return;

            try
            {
                pDB->MemoryRelease(nFlags);
            }
            catch(...)
            {
                debug::error(FUNCTION, "failed to release memory overlay during begin rollback");
            }
        };

        ReleaseMemory(started.fContractMemory, Contract);
        ReleaseMemory(started.fRegisterMemory, Register);
        ReleaseMemory(started.fLedgerMemory, Ledger);

        if(kind == TxnKind::PHYSICAL)
        {
            auto ReleaseJournal = [](const bool fStarted, auto* pDB)
            {
                if(!fStarted || !pDB)
                    return;

                try
                {
                    pDB->TxnRelease();
                }
                catch(...)
                {
                    debug::error(FUNCTION, "failed to release journal during begin rollback");
                }
            };

            /* Only journals this begin opened. TxnRelease truncates journal.dat,
             * so an instance whose TxnBegin did not return must be left alone. */
            ReleaseJournal(started.fLogical, Logical);
            ReleaseJournal(started.fContract, Contract);
            ReleaseJournal(started.fRegister, Register);
            ReleaseJournal(started.fLedger, Ledger);
            ReleaseJournal(started.fClient, Client);
            ReleaseJournal(started.fTrust, Trust);
            ReleaseJournal(started.fLegacy, Legacy);
        }

        TxnSlot& slot = SlotFor(kind);
        std::lock_guard<std::mutex> lk(slot.MUTEX);
        if(OwnedByThisThread(slot))
            ReleaseSlot(slot, kind);
    }


    static void MemoryReleaseSelected(const uint8_t nFlags, const uint16_t nInstances)
    {
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryRelease(nFlags);

        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryRelease(nFlags);

        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryRelease(nFlags);
    }


    static void MemoryCommitSelected(const uint16_t nInstances)
    {
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryCommit();

        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryCommit();

        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryCommit();
    }


    static bool TxnReleaseSelected(const uint16_t nInstances)
    {
        bool fSucceeded = true;
        auto Release = [&](auto* pDB, const uint16_t nInstance, const char* strName)
        {
            if(!pDB || !(nInstances & nInstance))
                return;

            try
            {
                pDB->TxnRelease();
            }
            catch(...)
            {
                debug::error(FUNCTION, "failed to release ", strName, " DB transaction");
                fSucceeded = false;
            }
        };

        Release(Logical, INSTANCES::LOGICAL, "Logical");
        Release(Contract, INSTANCES::CONTRACT, "Contract");
        Release(Register, INSTANCES::REGISTER, "Register");
        Release(Ledger, INSTANCES::LEDGER, "Ledger");
        Release(Client, INSTANCES::CLIENT, "Client");
        Release(Trust, INSTANCES::TRUST, "Trust");
        Release(Legacy, INSTANCES::LEGACY, "Legacy");

        return fSucceeded;
    }


    /* True when this thread owns the process-wide slot for nFlags. */
    bool ThreadOwnsTransaction(const uint8_t nFlags)
    {
        TxnSlot& slot = SlotFor(KindOf(nFlags));
        std::lock_guard<std::mutex> lk(slot.MUTEX);
        return OwnedByThisThread(slot);
    }


    /* True when this thread owns the slot and its begin included nInstances. */
    bool ThreadOwnsTransaction(const uint8_t nFlags, const uint16_t nInstances)
    {
        TxnSlot& slot = SlotFor(KindOf(nFlags));
        std::lock_guard<std::mutex> lk(slot.MUTEX);
        return OwnedByThisThread(slot)
            && (slot.nInstances & nInstances) == nInstances;
    }


    /* True when TxnBegin would open a physical journal that is already open.
     * The caller must not own PhysicalSlot, so an open journal has no matching
     * owner. */
    static bool UnownedJournalOpen(const uint16_t nInstances)
    {
        return HasOpenTransaction(0, nInstances);
    }


    /* Global handler for all LLD instances. */
    bool TxnBegin(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Participate in the shutdown barrier before taking a slot. A begin
         * after Shutdown has started must not create an overlay the delete
         * path can free out from under this thread. */
        const TxnLifetimeGuard life(TxnEntry::BEGIN);
        if(!life)
            return debug::error(FUNCTION, "LLD is shutting down");

        /* Claim the slot before creating its overlay or journal. A second begin,
         * including one from the current owner, must not reach MemoryBegin:
         * that call deletes the existing process-wide overlay. MEMPOOL and
         * physical BLOCK share pMemory, so either mode being owned rejects the
         * other begin before MemoryBegin. Callers that need both, including
         * TritiumBlock::Accept producer-orphan processing, must commit and
         * release the physical slot first. Lock MEMPOOL before PHYSICAL. */
        const TxnKind kind = KindOf(nFlags);
        std::unique_lock<std::mutex> lkPrimary;
        std::unique_lock<std::mutex> lkSecondary;

        if(kind == TxnKind::MEMPOOL || kind == TxnKind::PHYSICAL)
        {
            lkPrimary = std::unique_lock<std::mutex>(MempoolSlot().MUTEX);
            lkSecondary = std::unique_lock<std::mutex>(PhysicalSlot().MUTEX);

            TxnSlot& slot = (kind == TxnKind::MEMPOOL) ? MempoolSlot() : PhysicalSlot();
            const TxnSlot& other = (kind == TxnKind::MEMPOOL) ? PhysicalSlot() : MempoolSlot();
            /* OwnedByOther misses a same-thread duplicate. slot.fOwned covers both. */
            if(slot.fOwned)
            {
                return debug::error(FUNCTION, OwnedByThisThread(slot)
                    ? "transaction mode is already open"
                    : "transaction mode is owned by another thread");
            }

            if(other.fOwned)
            {
                return debug::error(FUNCTION, "shared memory overlay is already owned");
            }

            /* SectorDatabase::TxnBegin replaces an existing pTransaction after
             * the new journal is allocated. A raw recovery journal has no
             * PhysicalSlot owner, so claim nothing and leave that journal in place. */
            if(kind == TxnKind::PHYSICAL && UnownedJournalOpen(nInstances))
                return debug::error(FUNCTION, "unowned journal is already open");

            if(!ClaimSlot(slot, kind, nFlags, nInstances))
                return debug::error(FUNCTION, "LLD is shutting down");

            /* The claim makes a concurrent begin of the other mode fail. Drop
             * the unused lock before creating the overlay. */
            if(kind == TxnKind::MEMPOOL)
                lkSecondary.unlock();
            else
                lkPrimary.unlock();
        }
        else
        {
            TxnSlot& slot = SlotFor(kind);
            lkPrimary = std::unique_lock<std::mutex>(slot.MUTEX);
            /* Same check as the shared-overlay path: a same-thread duplicate
             * would otherwise delete pMiner or pSanitize inside MemoryBegin. */
            if(slot.fOwned)
            {
                return debug::error(FUNCTION, OwnedByThisThread(slot)
                    ? "transaction mode is already open"
                    : "transaction mode is owned by another thread");
            }

            if(!ClaimSlot(slot, kind, nFlags, nInstances))
                return debug::error(FUNCTION, "LLD is shutting down");
        }

        /* The slot is claimed before these calls. A throw must not return to a
         * caller whose active flag is set only after TxnBegin returns: release
         * finished instances and the slot, then propagate. Drop the slot locks
         * first so the rollback can take them again. */
        BeginProgress started;
        try
        {
            /* Start the contract DB transaction. */
            if(Contract && (nInstances & INSTANCES::CONTRACT))
            {
                Contract->MemoryBegin(nFlags);
                started.fContractMemory = true;
            }

            /* Start the register DB transacdtion. */
            if(Register && (nInstances & INSTANCES::REGISTER))
            {
                Register->MemoryBegin(nFlags);
                started.fRegisterMemory = true;
            }

            /* Start the ledger DB transaction. */
            if(Ledger && (nInstances & INSTANCES::LEDGER))
            {
                Ledger->MemoryBegin(nFlags);
                started.fLedgerMemory = true;
            }

            /* Handle memory commits if in memory m ode. */
            if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
                return true;

            /* Start the Logical DB transaction. */
            if(Logical && (nInstances & INSTANCES::LOGICAL))
            {
                Logical->TxnBegin();
                started.fLogical = true;
            }

            /* Start the contract DB transaction. */
            if(Contract && (nInstances & INSTANCES::CONTRACT))
            {
                Contract->TxnBegin();
                started.fContract = true;
            }

            /* Start the register DB transacdtion. */
            if(Register && (nInstances & INSTANCES::REGISTER))
            {
                Register->TxnBegin();
                started.fRegister = true;
            }

            /* Start the ledger DB transaction. */
            if(Ledger && (nInstances & INSTANCES::LEDGER))
            {
                Ledger->TxnBegin();
                started.fLedger = true;
            }

            /* Start the client DB transaction. */
            if(Client && (nInstances & INSTANCES::CLIENT))
            {
                Client->TxnBegin();
                started.fClient = true;
            }

            /* Start the trust DB transaction. */
            if(Trust && (nInstances & INSTANCES::TRUST))
            {
                Trust->TxnBegin();
                started.fTrust = true;
            }

            /* Start the legacy DB transaction. */
            if(Legacy && (nInstances & INSTANCES::LEGACY))
            {
                Legacy->TxnBegin();
                started.fLegacy = true;
            }
        }
        catch(...)
        {
            if(lkPrimary.owns_lock())
                lkPrimary.unlock();
            if(lkSecondary.owns_lock())
                lkSecondary.unlock();

            ReleaseStartedBegin(nFlags, kind, started);
            throw;
        }

        return true;
    }


    /* Global handler for all LLD instances. */
    void TxnAbort(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Allowed through shutdown until the instances are deleted, so an owner
         * can release its slot. After that, do not touch the databases. */
        const TxnLifetimeGuard life(TxnEntry::MUTATE);
        if(!life)
            return;

        const TxnKind kind = KindOf(nFlags);

        /* Memory overlays are process-wide. An unowned thread, or a thread that
         * owns a different mode, must not MemoryRelease this slot. */
        if(kind != TxnKind::PHYSICAL)
        {
            TxnSlot& slot = SlotFor(kind);
            std::lock_guard<std::mutex> lk(slot.MUTEX);
            if(!OwnedByThisThread(slot))
            {
                debug::error(FUNCTION, "transaction mode does not match current owner");
                return;
            }

            if(!CoversOwnedInstances(slot, nInstances))
            {
                debug::error(FUNCTION, "transaction instance mask does not cover current owner");
                return;
            }

            /* The begin mask is authoritative: never release overlays outside
             * the set opened by this owner, even if the caller passes a wider mask. */
            MemoryReleaseSelected(nFlags, slot.nInstances);
            ReleaseSlot(slot, kind);
            return;
        }

        /* MemoryRelease(BLOCK) deletes pMemory, including the overlay created by
         * TxnBegin(BLOCK). Do that only for the physical owner, and skip it when
         * another thread owns the MEMPOOL slot. An unowned abort must not delete
         * either overlay. Lock MEMPOOL before PHYSICAL. */
        std::unique_lock<std::mutex> lkMem(MempoolSlot().MUTEX);
        std::lock_guard<std::mutex> lkPhys(PhysicalSlot().MUTEX);
        if(OwnedByOther(PhysicalSlot()))
        {
            debug::error(FUNCTION, "transaction mode does not match current owner");
            return;
        }

        /* A thread that owns another mode and has no physical slot must not
         * fall through to TxnRelease. An unowned call remains the recovery and
         * post-abort no-op, which still releases a selected journal. */
        if(!OwnedByThisThread(PhysicalSlot()) && nThreadOwnedMask != 0)
        {
            debug::error(FUNCTION, "transaction mode does not match current owner");
            return;
        }

        const bool fReleasePhysical = OwnedByThisThread(PhysicalSlot());
        if(fReleasePhysical && !CoversOwnedInstances(PhysicalSlot(), nInstances))
        {
            debug::error(FUNCTION, "transaction instance mask does not cover current owner");
            return;
        }

        /* The begin mask is authoritative: do not release any unowned journal. */
        const uint16_t nOwnedInstances = fReleasePhysical ? PhysicalSlot().nInstances : nInstances;

        if(fReleasePhysical && !OwnedByOther(MempoolSlot()))
        {
            const bool fReleaseMempool = OwnedByThisThread(MempoolSlot());
            MemoryReleaseSelected(nFlags, nOwnedInstances);
            if(fReleaseMempool)
                ReleaseSlot(MempoolSlot(), TxnKind::MEMPOOL);
        }

        lkMem.unlock();

        /* Attempt every selected journal release. If one cleanup throws, its
         * journal remains intact for recovery while the other DBs still get
         * cleaned up before ownership is released. */
        TxnReleaseSelected(nOwnedInstances);

        if(fReleasePhysical)
            ReleaseSlot(PhysicalSlot(), TxnKind::PHYSICAL);
    }


    /* Abort every slot this thread still owns. Used when the owner thread exits
     * and when LLD shuts down, so a reused thread id cannot resume the overlay
     * and a later begin is not rejected forever. The stored begin flags and
     * instance mask are used so a narrower begin does not release other journals.
     * Once Shutdown has deleted the instances this returns without touching them. */
    static void AbortOwnedByThisThread()
    {
        const TxnLifetimeGuard life(TxnEntry::MUTATE);
        if(!life)
            return;

        struct Pending
        {
            uint8_t nFlags;
            uint16_t nInstances;
        };

        Pending vPending[4];
        uint32_t nPending = 0;

        auto Collect = [&](TxnSlot& slot)
        {
            std::lock_guard<std::mutex> lk(slot.MUTEX);
            if(!OwnedByThisThread(slot) || nPending >= 4)
                return;

            vPending[nPending].nFlags = slot.nFlags;
            vPending[nPending].nInstances = slot.nInstances;
            ++nPending;
        };

        /* One lock at a time. TxnAbort locks MEMPOOL before PHYSICAL. */
        Collect(MinerSlot());
        Collect(SanitizeSlot());
        Collect(MempoolSlot());
        Collect(PhysicalSlot());

        for(uint32_t n = 0; n < nPending; ++n)
        {
            debug::log(1, FUNCTION, "aborting transaction left open by this thread");
            TxnAbort(vPending[n].nFlags, vPending[n].nInstances);
        }
    }


    ThreadTxnOwnerGuard::~ThreadTxnOwnerGuard()
    {
        /* Instances may already have been deleted if this is the main thread
         * exiting after Shutdown. Worker threads release their slots here. */
        AbortOwnedByThisThread();
    }


    /* Global handler for all LLD instances. */
    bool TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Allowed through shutdown until the instances are deleted, so an owner
         * can finish and release its slot. After that, do not touch the databases. */
        const TxnLifetimeGuard life(TxnEntry::MUTATE);
        if(!life)
            return debug::error(FUNCTION, "LLD is shutting down");

        const TxnKind kind = KindOf(nFlags);

        /* MINER and SANITIZE commits do not call MemoryCommit and do not release
        * the owner. Decide from the requested slot, not sole ownership: MEMPOOL
        * may coexist with this mode. Reject a foreign owner, and reject when
        * this thread owns a different mode but not this slot. A globally
        * unowned slot on a thread that owns nothing stays a true short-circuit. */
        if(kind == TxnKind::MINER || kind == TxnKind::SANITIZE)
        {
           TxnSlot& slot = SlotFor(kind);
           std::lock_guard<std::mutex> lk(slot.MUTEX);
           if(OwnedByOther(slot) || (!OwnedByThisThread(slot) && nThreadOwnedMask != 0))
               return debug::error(FUNCTION, "transaction mode does not match current owner");

           return true;
        }

        /* An unowned MEMPOOL commit, or one owned by another thread, must not
        * apply pMemory. Owning MINER or SANITIZE as well is not a conflict and
        * does not release those slots. */
        if(kind == TxnKind::MEMPOOL)
        {
           std::lock_guard<std::mutex> lk(MempoolSlot().MUTEX);
           if(!OwnedByThisThread(MempoolSlot()))
               return debug::error(FUNCTION, "transaction mode does not match current owner");

           if(!CoversOwnedInstances(MempoolSlot(), nInstances))
               return debug::error(FUNCTION, "transaction instance mask does not cover current owner");

           MemoryCommitSelected(MempoolSlot().nInstances);
            ReleaseSlot(MempoolSlot(), TxnKind::MEMPOOL);
            return true;
        }

        /* TxnBegin(BLOCK) creates pMemory before the physical journal. Commit that
         * overlay only when this thread owns the physical slot, and never when
         * another thread owns MEMPOOL. Lock MEMPOOL before PHYSICAL, then drop the
         * MEMPOOL lock before the disk commit. */
        std::unique_lock<std::mutex> lkMem(MempoolSlot().MUTEX);
        std::lock_guard<std::mutex> lkPhys(PhysicalSlot().MUTEX);
        if(!OwnedByThisThread(PhysicalSlot()))
        {
            /* Reject a foreign physical owner, a caller that owns a different
             * mode, and an open journal this thread did not begin. The only
             * retained call is the redundant commit after that journal is
             * already gone: it returns false and does not checkpoint. */
            if(OwnedByOther(PhysicalSlot()) || nThreadOwnedMask != 0
            || HasOpenTransaction(nFlags, nInstances))
                return debug::error(FUNCTION, "transaction mode does not match current owner");

            return false;
        }

        if(!CoversOwnedInstances(PhysicalSlot(), nInstances))
            return debug::error(FUNCTION, "transaction instance mask does not cover current owner");

        /* Use the begin mask for every DB operation so commits and release never
         * touch journals that this transaction did not open. */
        const uint16_t nOwnedInstances = PhysicalSlot().nInstances;

        const bool fReleasePhysical = OwnedByThisThread(PhysicalSlot());
        if(fReleasePhysical && !OwnedByOther(MempoolSlot()))
        {
            const bool fReleaseMempool = OwnedByThisThread(MempoolSlot());
            MemoryCommitSelected(nOwnedInstances);
            if(fReleaseMempool)
                ReleaseSlot(MempoolSlot(), TxnKind::MEMPOOL);
        }

        lkMem.unlock();

        /* Set a checkpoint for Logical DB. */
        if(Logical && (nOwnedInstances & INSTANCES::LOGICAL))
            Logical->TxnCheckpoint();

        /* Set a checkpoint for contract DB. */
        if(Contract && (nOwnedInstances & INSTANCES::CONTRACT))
            Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        if(Register && (nOwnedInstances & INSTANCES::REGISTER))
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger && (nOwnedInstances & INSTANCES::LEDGER))
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for client DB. */
        if(Client && (nOwnedInstances & INSTANCES::CLIENT))
            Client->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust && (nOwnedInstances & INSTANCES::TRUST))
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy && (nOwnedInstances & INSTANCES::LEGACY))
            Legacy->TxnCheckpoint();


        /* Aggregate the per-instance TxnCommit() results.  All selected
         * instances are attempted regardless of individual failures — do NOT
         * short-circuit on the first failure, so that state doesn't diverge
         * further than necessary if one instance fails. */
        bool fAllSucceeded = true;

        /* Commit Logical DB transaction. */
        if(Logical && (nOwnedInstances & INSTANCES::LOGICAL))
        {
            if(!Logical->TxnCommit())
            {
                debug::error(FUNCTION, "Logical DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit contract DB transaction. */
        if(Contract && (nOwnedInstances & INSTANCES::CONTRACT))
        {
            if(!Contract->TxnCommit())
            {
                debug::error(FUNCTION, "Contract DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit register DB transaction. */
        if(Register && (nOwnedInstances & INSTANCES::REGISTER))
        {
            if(!Register->TxnCommit())
            {
                debug::error(FUNCTION, "Register DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit ledger DB transaction. */
        if(Ledger && (nOwnedInstances & INSTANCES::LEDGER))
        {
            if(!Ledger->TxnCommit())
            {
                debug::error(FUNCTION, "Ledger DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the client DB transaction. */
        if(Client && (nOwnedInstances & INSTANCES::CLIENT))
        {
            if(!Client->TxnCommit())
            {
                debug::error(FUNCTION, "Client DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the trust DB transaction. */
        if(Trust && (nOwnedInstances & INSTANCES::TRUST))
        {
            if(!Trust->TxnCommit())
            {
                debug::error(FUNCTION, "Trust DB commit failed");
                fAllSucceeded = false;
            }
        }

        /* Commit the legacy DB transaction. */
        if(Legacy && (nOwnedInstances & INSTANCES::LEGACY))
        {
            if(!Legacy->TxnCommit())
            {
                debug::error(FUNCTION, "Legacy DB commit failed");
                fAllSucceeded = false;
            }
        }


        /* Release the checkpoint markers for all selected instances.
         * TxnRelease runs unconditionally after TxnCommit, regardless of commit
         * outcome.  On success, SectorDatabase::TxnCommit() already nulled
         * pTransaction; on failure it may still be set and TxnRelease will clean
         * it up here.  Leaving a failed transaction intact for retry is not
         * supported at the LLD layer — callers that receive false should abort
         * their higher-level operation and rebuild rather than retrying the same
         * transaction object.
         *
         * Note: TxnRelease is called for every selected instance below,
         * regardless of whether that instance's TxnCommit succeeded or failed. */
        if(!TxnReleaseSelected(nOwnedInstances))
            fAllSucceeded = false;

        if(fReleasePhysical)
            ReleaseSlot(PhysicalSlot(), TxnKind::PHYSICAL);

        return fAllSucceeded;
    }
}
