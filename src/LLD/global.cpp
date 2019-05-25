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
        if(!regDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!legDB->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(!locDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!trustDB->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(!legacyDB->TxnRecovery())
            fRecovery = false;

        /* Commit the transactions if journals are recovered. */
        if(fRecovery)
        {
            debug::log(0, FUNCTION, "all transactions are complete, recovering...");

            /* Commit register DB transaction. */
            regDB->TxnCommit();

            /* Commit legacy DB transaction. */
            legDB->TxnCommit();

            /* Commit the local DB transaction. */
            locDB->TxnCommit();

            /* Commit the trust DB transaction. */
            trustDB->TxnCommit();

            /* Commit the legacy DB transaction. */
            legacyDB->TxnCommit();
        }

        /* Abort all the transactions. */
        TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnBegin()
    {
        /* Start the register DB transacdtion. */
        regDB->TxnBegin();

        /* Start the ledger DB transaction. */
        legDB->TxnBegin();

        /* Start the local DB transaction. */
        locDB->TxnBegin();

        /* Start the trust DB transaction. */
        trustDB->TxnBegin();

        /* Start the legacy DB transaction. */
        legacyDB->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort()
    {
        /* Abort the register DB transaction. */
        regDB->TxnAbort();

        /* Abort the ledger DB transaction. */
        legDB->TxnAbort();

        /* Abort the local DB transaction. */
        locDB->TxnAbort();

        /* Abort the trust DB transaction. */
        trustDB->TxnAbort();

        /* Abort the legacy DB transaction. */
        legacyDB->TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit()
    {
        /* Set a checkpoint for register DB. */
        regDB->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        legDB->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        locDB->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        trustDB->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        legacyDB->TxnCheckpoint();


        /* Commit register DB transaction. */
        regDB->TxnCommit();

        /* Commit legacy DB transaction. */
        legDB->TxnCommit();

        /* Commit the local DB transaction. */
        locDB->TxnCommit();

        /* Commit the trust DB transaction. */
        trustDB->TxnCommit();

        /* Commit the legacy DB transaction. */
        legacyDB->TxnCommit();


        /* Release the register DB journal. */
        regDB->TxnRelease();

        /* Release the ledger DB journal. */
        legDB->TxnRelease();

        /* Release the local DB journal. */
        locDB->TxnRelease();

        /* Release the trust DB journal. */
        trustDB->TxnRelease();

        /* Release the legacy DB journal. */
        legacyDB->TxnRelease();
    }
}
