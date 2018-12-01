/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_GLOBAL_H
#define NEXUS_LLD_INCLUDE_GLOBAL_H

#include <LLD/include/register.h>
#include <LLD/include/ledger.h>
#include <LLD/include/local.h>

namespace LLD
{
    extern RegisterDB*   regDB;
    extern LedgerDB*     legDB;
    extern LocalDB*      locDB;
}

#endif
