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
        uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE;

        debug::log(0, FUNCTION, "Initializing LLD");

        /* Create the database instances. */
        Contract = new ContractDB(nFlags);
        Register = new RegisterDB(nFlags);
        Local    = new LocalDB(nFlags);
        Ledger   = new LedgerDB(nFlags);
        Trust    = new TrustDB(nFlags);
        Legacy   = new LegacyDB(nFlags);
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

        /* Check the contract DB journal. */
        if(!Contract->TxnRecovery())
            fRecovery = false;

        /* Check the register DB journal. */
        if(!Register->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!Ledger->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(!Local->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!Trust->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!Legacy->TxnRecovery())
            fRecovery = false;

        /* Commit the transactions if journals are recovered. */
        if(fRecovery)
        {
            debug::log(0, FUNCTION, "all transactions are complete, recovering...");

            /* Commit contract DB transaction. */
            Contract->TxnCommit();

            /* Commit register DB transaction. */
            Register->TxnCommit();

            /* Commit legacy DB transaction. */
            Ledger->TxnCommit();

            /* Commit the local DB transaction. */
            Local->TxnCommit();

            /* Commit the trust DB transaction. */
            Trust->TxnCommit();

            /* Commit the legacy DB transaction. */
            Legacy->TxnCommit();
        }

        /* Abort all the transactions. */
        TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnBegin()
    {
        /* Start the contract DB transacdtion. */
        Contract->TxnBegin();

        /* Start the register DB transacdtion. */
        Register->TxnBegin();

        /* Start the ledger DB transaction. */
        Ledger->TxnBegin();

        /* Start the local DB transaction. */
        Local->TxnBegin();

        /* Start the trust DB transaction. */
        Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort()
    {
        /* Abort the contract DB transacdtion. */
        Contract->TxnAbort();

        /* Abort the register DB transaction. */
        Register->TxnAbort();

        /* Abort the ledger DB transaction. */
        Ledger->TxnAbort();

        /* Abort the local DB transaction. */
        Local->TxnAbort();

        /* Abort the trust DB transaction. */
        Trust->TxnAbort();

        /* Abort the legacy DB transaction. */
        Legacy->TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit()
    {
        /* Set a checkpoint for contract DB. */
        Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        Ledger->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        Local->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        Legacy->TxnCheckpoint();


        /* Commit contract DB transaction. */
        Contract->TxnCommit();

        /* Commit register DB transaction. */
        Register->TxnCommit();

        /* Commit legacy DB transaction. */
        Ledger->TxnCommit();

        /* Commit the local DB transaction. */
        Local->TxnCommit();

        /* Commit the trust DB transaction. */
        Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        Legacy->TxnCommit();


        /* Release the contract DB journal. */
        Contract->TxnRelease();

        /* Release the register DB journal. */
        Register->TxnRelease();

        /* Release the ledger DB journal. */
        Ledger->TxnRelease();

        /* Release the local DB journal. */
        Local->TxnRelease();

        /* Release the trust DB journal. */
        Trust->TxnRelease();

        /* Release the legacy DB journal. */
        Legacy->TxnRelease();
    }
}
