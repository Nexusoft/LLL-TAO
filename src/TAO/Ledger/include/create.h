/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_CREATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CREATE_H

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

namespace TAO::Ledger
{

    /** Create Transaction
     *
     *  Create a new transaction object from signature chain.
     *
     *  @param[in] user The signature chain to generate this tx
     *  @param[out] tx The traansaction object being created
     *
     **/
    bool CreateTransaction(TAO::Ledger::SignatureChain* user, TAO::Ledger::Transaction& tx);


    /** Create Block
     *
     *  Create a new block object from the chain.
     *
     *  @param[in] user The signature chain to generate this block
     *  @param[out] block The block object being created.
     *
     **/
    bool CreateBlock(TAO::Ledger::SignatureChain* user, TAO::Ledger::TritiumBlock& block);

}

#endif
