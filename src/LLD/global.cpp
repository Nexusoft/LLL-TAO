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
        Contract = new ContractDB(
                        FLAGS::CREATE | FLAGS::FORCE);

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
        if(!config::fClient.load())
        {

            /* Create the legacy database instance. */
            uint32_t nLegacyCacheSize = config::GetArg("-legacycache", 1);
            Legacy = new LegacyDB(
                            FLAGS::CREATE | FLAGS::FORCE,
                            256 * 256 * 64,
                            nLegacyCacheSize * 1024 * 1024);


            /* Create the trust database instance. */
            Trust  = new TrustDB(
                            FLAGS::CREATE | FLAGS::FORCE);


            /* Create the local database instance. */
            Local    = new LocalDB(
                            FLAGS::CREATE | FLAGS::FORCE);
        }
        else
        {
            /* Create the local database instance. */
            Local    = new LocalDB(
                            FLAGS::CREATE | FLAGS::FORCE);

            /* Create new client database if enabled. */
            Client    = new ClientDB(
                            FLAGS::CREATE | FLAGS::FORCE,
                            77773);
        }

        /* Handle database recovery mode. */
        TxnRecovery();
    }


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Cleanup the contract database. */
        if(Contract)
        {
            debug::log(2, FUNCTION, "Shutting down ContractDB");
            delete Contract;
        }

        /* Cleanup the ledger database. */
        if(Ledger)
        {
            debug::log(2, FUNCTION, "Shutting down LedgerDB");
            delete Ledger;
        }


        /* Cleanup the register database. */
        if(Register)
        {
            debug::log(2, FUNCTION, "Shutting down RegisterDB");
            delete Register;
        }


        /* Cleanup the local database. */
        if(Local)
        {
            debug::log(2, FUNCTION, "Shutting down LocalDB");
            delete Local;
        }


        /* Cleanup the client database. */
        if(Client)
        {
            debug::log(2, FUNCTION, "Shutting down ClientDB");
            delete Client;
        }


        /* Cleanup the legacy database. */
        if(Legacy)
        {
            debug::log(2, FUNCTION, "Shutting down LegacyDB");
            delete Legacy;
        }


        /* Cleanup the trust database. */
        if(Trust)
        {
            debug::log(2, FUNCTION, "Shutting down TrustDB");
            delete Trust;
        }
    }


    /* Check the transactions for recovery. */
    void TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Check the contract DB journal. */
        if(Contract && !Contract->TxnRecovery())
            fRecovery = false;

        /* Check the register DB journal. */
        if(Register && !Register->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(Ledger && !Ledger->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(Local && !Local->TxnRecovery())
            fRecovery = false;

        /* Check the client DB journal. */
        if(Client && !Client->TxnRecovery())
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

            /* Commit the local DB transaction. */
            if(Local)
                Local->TxnCommit();

            /* Commit the client DB transaction. */
            if(Client)
                Client->TxnCommit();

            /* Commit the trust DB transaction. */
            if(Trust)
                Trust->TxnCommit();

            /* Commit the legacy DB transaction. */
            if(Legacy)
                Legacy->TxnCommit();
        }

        /* Abort all the transactions. */
        TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnBegin(const uint8_t nFlags)
    {
        /* Start the contract DB transaction. */
        if(Contract)
            Contract->MemoryBegin(nFlags);

        /* Start the register DB transacdtion. */
        if(Register)
            Register->MemoryBegin(nFlags);

        /* Start the ledger DB transaction. */
        if(Ledger)
            Ledger->MemoryBegin(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
            return;

        /* Start the contract DB transaction. */
        if(Contract)
            Contract->TxnBegin();

        /* Start the register DB transacdtion. */
        if(Register)
            Register->TxnBegin();

        /* Start the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnBegin();

        /* Start the local DB transaction. */
        if(Local)
            Local->TxnBegin();

        /* Start the client DB transaction. */
        if(Client)
            Client->TxnBegin();

        /* Start the trust DB transaction. */
        if(Trust)
            Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort(const uint8_t nFlags)
    {
        /* Abort the contract DB transaction. */
        if(Contract)
            Contract->MemoryRelease(nFlags);

        /* Abort the register DB transacdtion. */
        if(Register)
            Register->MemoryRelease(nFlags);

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->MemoryRelease(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
            return;

        /* Abort the contract DB transaction. */
        if(Contract)
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client)
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit(const uint8_t nFlags)
    {
        /* Commit the contract DB transaction. */
        if(Contract)
            Contract->MemoryCommit();

        /* Commit the register DB transacdtion. */
        if(Register)
            Register->MemoryCommit();

        /* Commit the ledger DB transaction. */
        if(Ledger)
            Ledger->MemoryCommit();

        /* Handle memory commits if in memory mode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
            return;

        /* Set a checkpoint for contract DB. */
        if(Contract)
            Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        if(Register)
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger)
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        if(Local)
            Local->TxnCheckpoint();

        /* Set a checkpoint for client DB. */
        if(Client)
            Client->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust)
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy)
            Legacy->TxnCheckpoint();


        /* Commit contract DB transaction. */
        if(Contract)
            Contract->TxnCommit();

        /* Commit register DB transaction. */
        if(Register)
            Register->TxnCommit();

        /* Commit legacy DB transaction. */
        if(Ledger)
            Ledger->TxnCommit();

        /* Commit the local DB transaction. */
        if(Local)
            Local->TxnCommit();

        /* Commit the client DB transaction. */
        if(Client)
            Client->TxnCommit();

        /* Commit the trust DB transaction. */
        if(Trust)
            Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnCommit();


        /* Abort the contract DB transaction. */
        if(Contract)
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client)
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }
}
