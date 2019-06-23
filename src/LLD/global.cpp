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
    RegisterDB*   regDB;
    LedgerDB*     legDB;
    LocalDB*      locDB;

    //for legacy objects
    TrustDB*      trustDB;
    LegacyDB*     legacyDB;


    /* Check the transactions for recovery. */
    void TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Check the register DB journal. */
        if(regDB && !regDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(legDB && !legDB->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(locDB && !locDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(trustDB && !trustDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(legacyDB && !legacyDB->TxnRecovery())
            fRecovery = false;

        /* Commit the transactions if journals are recovered. */
        if(fRecovery)
        {
            debug::log(0, FUNCTION, "all transactions are complete, recovering...");

            /* Commit register DB transaction. */
            if(regDB)
                regDB->TxnCommit();

            /* Commit legacy DB transaction. */
            if(legDB)
                legDB->TxnCommit();

            /* Commit the local DB transaction. */
            if(locDB)
                locDB->TxnCommit();

            /* Commit the trust DB transaction. */
            if(trustDB)
                trustDB->TxnCommit();

            /* Commit the legacy DB transaction. */
            if(legacyDB)
                legacyDB->TxnCommit();
        }

        /* Abort all the transactions. */
        TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnBegin()
    {
        /* Start the register DB transacdtion. */
        if(regDB)
            regDB->TxnBegin();

        /* Start the ledger DB transaction. */
        if(legDB)
            legDB->TxnBegin();

        /* Start the local DB transaction. */
        if(locDB)
            locDB->TxnBegin();

        /* Start the trust DB transaction. */
        if(trustDB)
            trustDB->TxnBegin();

        /* Start the legacy DB transaction. */
        if(legacyDB)
            legacyDB->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort()
    {
        /* Abort the register DB transaction. */
        if(regDB)
            regDB->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(legDB)
            legDB->TxnRelease();

        /* Abort the local DB transaction. */
        if(locDB)
            locDB->TxnRelease();

        /* Abort the trust DB transaction. */
        if(trustDB)
            trustDB->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(legacyDB)
            legacyDB->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit()
    {
        /* Set a checkpoint for register DB. */
        if(regDB)
            regDB->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(legDB)
            legDB->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        if(locDB)
            locDB->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(trustDB)
            trustDB->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(legacyDB)
            legacyDB->TxnCheckpoint();


        /* Commit register DB transaction. */
        if(regDB)
            regDB->TxnCommit();

        /* Commit legacy DB transaction. */
        if(legDB)
            legDB->TxnCommit();

        /* Commit the local DB transaction. */
        if(locDB)
            locDB->TxnCommit();

        /* Commit the trust DB transaction. */
        if(trustDB)
            trustDB->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(legacyDB)
            legacyDB->TxnCommit();


        /* Abort the register DB transaction. */
        if(regDB)
            regDB->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(legDB)
            legDB->TxnRelease();

        /* Abort the local DB transaction. */
        if(locDB)
            locDB->TxnRelease();

        /* Abort the trust DB transaction. */
        if(trustDB)
            trustDB->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(legacyDB)
            legacyDB->TxnRelease();
    }
}
