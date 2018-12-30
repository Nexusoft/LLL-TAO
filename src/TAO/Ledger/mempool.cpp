/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/Ledger/types/mempool.h>

namespace TAO::Ledger
{
    Mempool mempool;

    /* Add a transaction to the memory pool without validation checks. */
    bool Mempool::AddUnchecked(TAO::Ledger::Transaction tx)
    {
        /* Get the transaction hash. */
        uint512_t hash = tx.GetHash();

        /* Check the mempool. */
        if(mapLedger.count(hash))
            return debug::error(FUNCTION "%s already exists", __PRETTY_FUNCTION__);

        /* Add to the map. */
        mapLedger[hash] = tx;

        return true;
    }


    /* Accepts a transaction with validation rules. */
    bool Mempool::Accept(TAO::Ledger::Transaction tx)
    {
        /* Check that the transaction is in a valid state. */
        if(!tx.IsValid())
            return false;

        /* Calculate the future potential states. */
        //TODO: execute the operations layer
    }


    /* Checks if a transaction exists. */
    bool Mempool::Has(uint512_t hashTx)
    {
        return mapLedger.count(hashTx);
    }


    /* Gets a transaction from mempool */
    bool Mempool::Get(uint512_t hashTx, TAO::Ledger::Transaction& tx)
    {
        if(!mapLedger.count(hashTx))
            return false;

        tx = mapLedger[hashTx];
        return true;
    }
}
