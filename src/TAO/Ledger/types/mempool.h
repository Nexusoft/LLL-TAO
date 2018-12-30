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
#include <Legacy/types/transaction.h>

#include <Legacy/types/inpoint.h>
#include <Legacy/types/outpoint.h>

#include <Util/include/mutex.h>

namespace TAO::Ledger
{

    class Mempool
    {
        std::recursive_mutex MUTEX;

        /** The transactions in the ledger memory pool. **/
        std::map<uint512_t, Legacy::Transaction> mapLegacy;


        /** The transactions in the ledger memory pool. **/
        std::map<uint512_t, TAO::Ledger::Transaction> mapLedger;


        /** Record of next hashes in the mempool. **/
        std::map<uint256_t, uint512_t> mapPrevHashes;


        /** Record of legacy inputs in the mempool. **/
        std::map<Legacy::COutPoint, Legacy::CInPoint> mapInputs;

    public:

        /** Default Constructor. **/
        Mempool()
        : mapLedger()
        {

        }

        /** Accept.
         *
         *  Accepts a legacy transaction with validation rules.
         *
         *  @param[in] tx The transaction to add.
         *
         *  @return true if added.
         *
         **/
        bool Accept(Legacy::Transaction tx);


        /** Add Unchecked.
         *
         *  Add a transaction to the memory pool without validation checks.
         *
         *  @param[in] tx The transaction to add.
         *
         *  @return true if added.
         *
         **/
        bool AddUnchecked(TAO::Ledger::Transaction tx);


        /** Accept.
         *
         *  Accepts a transaction with validation rules.
         *
         *  @param[in] tx The transaction to add.
         *
         *  @return true if added.
         *
         **/
        bool Accept(TAO::Ledger::Transaction tx);


        /** Has.
         *
         *  Checks if a transaction exists.
         *
         *  @param[in] hashTx The transaction to add.
         *
         *  @return true if added.
         *
         **/
        bool Has(uint512_t hashTx);


        /** Remove.
         *
         *  Remove a transaction from pool.
         *
         *  @param[in] hashTx The transaction to remove
         *
         *  @return true if added.
         *
         **/
        bool Remove(uint512_t hashTx);


        /** Get.
         *
         *  Gets a transaction from mempool
         *
         *  @param[in] tx The transaction to add.
         *
         *  @return true if added.
         *
         **/
        bool Get(uint512_t hashTx, TAO::Ledger::Transaction& tx);
    };

    extern Mempool mempool;
}

#endif
