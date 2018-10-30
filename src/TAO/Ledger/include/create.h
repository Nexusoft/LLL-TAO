/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_CREATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CREATE_H

#include <TAO/Ledger/types/transaction.h>

namespace TAO
{
    namespace Ledger
    {

        /** Create Transaction
         *
         *  Create a new transaction object from signature chain.
         *
         *  @param[in] sigchain The signature chain to generate this tx
         *  @param[in] hashPrevTx The previous transaction being linked.
         *  @param[out] tx The traansaction object being created
         *
         **/
         bool CreateTransaction(TAO::Ledger::SignatureChain sigchain, uint512_t hashPrevTx, TAO::Ledger::Transaction& tx);

    }
}

#endif
