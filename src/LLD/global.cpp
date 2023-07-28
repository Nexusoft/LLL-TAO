/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/enum.h> //for internal flags

namespace LLD
{
    /* The LLD global instance pointers. */
    LogicalDB*    Logical;
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
        uint32_t nContractCacheSize = config::GetArg("-contractcache", 1);
        Contract = new ContractDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        77773,
                        nContractCacheSize * 1024 * 1024);

        /* Create the contract database instance. */
        uint32_t nRegisterCacheSize = config::GetArg("-registercache", 2);
        Register = new RegisterDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        77773,
                        nRegisterCacheSize * 1024 * 1024);

        /* Create the ledger database instance. */
        uint32_t nLedgerCacheSize = config::GetArg("-ledgercache", 2);
        Ledger    = new LedgerDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        config::fClient.load() ? 77773 : (256 * 256 * 64),
                        nLedgerCacheSize * 1024 * 1024);


        /* Create the legacy database instance. */
        uint32_t nLegacyCacheSize = config::GetArg("-legacycache", 1);
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
        Logical    = new LogicalDB(
                        FLAGS::CREATE | FLAGS::FORCE,
                        (256 * 256 * 16));


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


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Cleanup the contract database. */
        if(Logical)
            delete Logical;

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

                /* Commit the legacy DB transaction. */
                if(Legacy)
                    Legacy->TxnCommit();
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


    /* Global handler for all LLD instances. */
    void TxnBegin(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Start the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryBegin(nFlags);

        /* Start the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryBegin(nFlags);

        /* Start the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryBegin(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
            return;

        /* Start the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnBegin();

        /* Start the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnBegin();

        /* Start the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnBegin();

        /* Start the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnBegin();

        /* Start the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnBegin();

        /* Start the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryRelease(nFlags);

        /* Abort the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryRelease(nFlags);

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryRelease(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
            return;

        /* Abort the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnRelease();

        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Commit the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryCommit();

        /* Commit the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryCommit();

        /* Commit the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryCommit();

        /* Handle memory commits if in memory mode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
            return;

        /* Set a checkpoint for Logical DB. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnCheckpoint();

        /* Set a checkpoint for contract DB. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for client DB. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnCheckpoint();


        /* Commit Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnCommit();

        /* Commit contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnCommit();

        /* Commit register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnCommit();

        /* Commit legacy DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnCommit();

        /* Commit the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnCommit();

        /* Commit the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnCommit();


        /* Abort the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnRelease();

        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnRelease();
    }
}
