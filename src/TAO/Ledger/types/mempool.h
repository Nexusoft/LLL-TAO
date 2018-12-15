/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H
#define NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Legacy/types/transaction.h>

namespace TAO::Ledger
{

    class Mempool
    {
        std::map<uint512_t, TAO::Ledger::Transaction> mapLedger;

        std::map<uint512_t, Legacy::Transaction> mapLegacy;

        //TODO: for tritium transaction add the state change in memory

        bool AddUnchecked(TAO::Ledger::Transaction txIn)
        {
            /* Get the transaction hash. */
            uint512_t hash = txIn.GetHash();

            /* Check the mempool. */
            if(mapLedger.count(hash))
                return debug::error(FUNCTION "%s already exists", __PRETTY_FUNCTION__);

            /* Add to the map. */
            mapLedger[hash] = txIn;

            return true;
        }


        bool AddUnchecked(Legacy::Transaction txIn)
        {
            /* Get the transaction hash. */
            uint512_t hash = txIn.GetHash();

            /* Check the mempool. */
            if(mapLegacy.count(hash))
                return debug::error(FUNCTION "%s already exists", __PRETTY_FUNCTION__);

            /* Add to the map. */
            mapLegacy[hash] = txIn;

            return true;
        }


        bool Add(TAO::Ledger::Transaction txIn)
        {
            /* Check that the transaction is in a valid state. */
            if(!txIn.IsValid())
                return false;
        }
    };
}

#endif
