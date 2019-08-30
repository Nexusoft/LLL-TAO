/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

namespace LLD
{
    /* The LLD global instance pointers. */
    ContractDB*   Contract;
    RegisterDB*   Register;
    LedgerDB*     Ledger;
    LocalDB*      Local;

    /* For Legacy LLD objects. */
    TrustDB*      Trust;
    LegacyDB*     Legacy;


    /*  Initialize the global LLD instances. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing LLD");

        /* Create the ledger database instance. */
        Ledger    = new LedgerDB(
                        FLAGS::CREATE | FLAGS::WRITE,
                        256 * 256 * 64,
                        4 * 1024 * 1024);

        /* Create the legacy database instance. */
        Legacy = new LegacyDB(
                        FLAGS::CREATE | FLAGS::WRITE,
                        256 * 256 * 64,
                        4 * 1024 * 1024);

        /* Create the trust database instance. */
        Trust  = new TrustDB(
                        FLAGS::CREATE | FLAGS::WRITE);

        /* Create the contract database instance. */
        Contract = new ContractDB(
                        FLAGS::CREATE | FLAGS::WRITE);

        /* Create the contract database instance. */
        Register = new RegisterDB(
                        FLAGS::CREATE | FLAGS::WRITE);

        /* Create the local database instance. */
        Local    = new LocalDB(
                        FLAGS::CREATE | FLAGS::WRITE);

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

        /* Check the register DB journal. */
        if(Register && !Register->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(Ledger && !Ledger->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(Local && !Local->TxnRecovery())
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

            /* Commit register DB transaction. */
            if(Register)
                Register->TxnCommit();

            /* Commit legacy DB transaction. */
            if(Ledger)
                Ledger->TxnCommit();

            /* Commit the local DB transaction. */
            if(Local)
                Local->TxnCommit();

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
    void TxnBegin()
    {
        /* Start the register DB transacdtion. */
        if(Register)
            Register->TxnBegin();

        /* Start the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnBegin();

        /* Start the local DB transaction. */
        if(Local)
            Local->TxnBegin();

        /* Start the trust DB transaction. */
        if(Trust)
            Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort()
    {
        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit()
    {
        /* Set a checkpoint for register DB. */
        if(Register)
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger)
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        if(Local)
            Local->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust)
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy)
            Legacy->TxnCheckpoint();


        /* Commit register DB transaction. */
        if(Register)
            Register->TxnCommit();

        /* Commit legacy DB transaction. */
        if(Ledger)
            Ledger->TxnCommit();

        /* Commit the local DB transaction. */
        if(Local)
            Local->TxnCommit();

        /* Commit the trust DB transaction. */
        if(Trust)
            Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnCommit();


        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }
}
