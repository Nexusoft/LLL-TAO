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


    /* Global handler for all LLD instances. */
    void TxnBegin()
    {
        //write to journal begin
        regDB->TxnBegin();
        legDB->TxnBegin();
        locDB->TxnBegin();

        trustDB->TxnBegin();
        legacyDB->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort()
    {
        regDB->TxnAbort();
        legDB->TxnAbort();
        locDB->TxnAbort();

        trustDB->TxnAbort();
        legacyDB->TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit()
    {
        regDB->TxnCommit();
        legDB->TxnCommit();
        locDB->TxnCommit();

        trustDB->TxnCommit();
        legacyDB->TxnCommit();

        //delete the journal begin and db files
    }
}
