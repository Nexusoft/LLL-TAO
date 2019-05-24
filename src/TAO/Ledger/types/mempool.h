/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H
#define NEXUS_TAO_LEDGER_TYPES_MEMPOOL_H

#include <LLC/types/uint1024.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/types/transaction.h>
#include <Legacy/types/transaction.h>

#include <Legacy/types/inpoint.h>
#include <Legacy/types/outpoint.h>

#include <Util/include/mutex.h>

namespace LLP
{
    class TritiumNode;
}

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Mempool
         *
         *  The memory pool class where transactions are stored until they are validated
         *  and added to the ledger.
         *
         **/
        class Mempool
        {
            mutable std::recursive_mutex MUTEX;

            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, Legacy::Transaction> mapLegacy;


            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapLedger;


            /** Oprhan transactions in queue. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapOrphans;


            /** Record of next hashes in the mempool. **/
            std::map<uint256_t, uint512_t> mapPrevHashes;


            /** Record of legacy inputs in the mempool. **/
            std::map<Legacy::OutPoint, uint512_t> mapInputs;

        public:

            /** Default Constructor. **/
            Mempool();


            /** Default Destructor. **/
            ~Mempool();


            /** AddUnchecked.
             *
             *  Add a transaction to the memory pool without validation checks.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool AddUnchecked(const TAO::Ledger::Transaction& tx);


            /** AddUnchecked
             *
             *  Add a legacy transaction to the memory pool without validation checks.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool AddUnchecked(const Legacy::Transaction& tx);


            /** Accept
             *
             *  Accepts a transaction with validation rules.
             *
             *  @param[in] tx The transaction to add.
             *  @param[in] pnode The node that transaction is accepted from.
             *
             *  @return true if added.
             *
             **/
            bool Accept(TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode = nullptr);


            /** Accept
             *
             *  Accepts a legacy transaction with validation rules.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool Accept(const Legacy::Transaction& tx);


            /** Get
             *
             *  Gets a transaction from mempool
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The retrieved transaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const;


            /** Get
             *
             *  Gets a transaction by genesis.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] vTx The list of retrieved transaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint256_t& hashGenesis, std::vector<TAO::Ledger::Transaction> &vTx) const;


            /** Get
             *
             *  Gets a transaction by genesis.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The last tx by genesistransaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint256_t& hashGenesis, TAO::Ledger::Transaction &tx) const;


            /** Get
             *
             *  Gets a legacy transaction from mempool
             *
             *  @param[in] hashTx Hash of legacy transaction to get.
             *
             *  @param[out] tx The retrieved legacy transaction
             *
             *  @return true if pool contained legacy transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, Legacy::Transaction &tx) const;


            /** Has
             *
             *  Checks if a transaction exists.
             *
             *  @param[in] hashTx Hash of transaction to check.
             *
             *  @return true if transaction in mempool.
             *
             **/
            bool Has(const uint512_t& hashTx) const;


            /** Has
             *
             *  Checks if a genesis exists.
             *
             *  @param[in] hashGenesis Hash of genesis to check.
             *
             *  @return true if transaction in mempool.
             *
             **/
            bool Has(const uint256_t& hashGenesis) const;


            /** HasLegacy
             *
             *  Checks if a legacy transaction exists.
             *
             *  @param[in] hashTx Hash of legacy transaction to check.
             *
             *  @return true if legacy transaction in mempool.
             *
             **/
            bool HasLegacy(const uint512_t& hashTx) const;


            /** Remove
             *
             *  Remove a transaction from pool.
             *
             *  @param[in] hashTx Hash of transaction to remove.
             *
             *  @return true if removed.
             *
             **/
            bool Remove(const uint512_t& hashTx);


            /** RemoveLegacy
             *
             *  Remove a legacy transaction from pool.
             *
             *  @param[in] hashTx Hash of legacy transaction to remove.
             *
             *  @return true if removed.
             *
             **/
            bool RemoveLegacy(const uint512_t& hashTx);


            /** List
             *
             *  List transactions in memory pool.
             *
             *  @param[out] vHashes List of transaction hashes.
             *  @param[in] nCount The total transactions to get.
             *
             *  @return true if list is not empty.
             *
             **/
            bool List(std::vector<uint512_t> &vHashes, uint32_t nCount = std::numeric_limits<uint32_t>::max()) const;


            /** ListLegacy
             *
             *  List legacy transactions in memory pool.
             *
             *  @param[out] vHashes List of legacy transaction hashes.
             *  @param[in] nCount The total transactions to get.
             *
             *  @return true if list is not empty.
             *
             **/
            bool ListLegacy(std::vector<uint512_t> &vHashes, uint32_t nCount = std::numeric_limits<uint32_t>::max()) const;


            /** Size
             *
             *  Gets the size of the memory pool.
             *
             **/
            uint32_t Size();


            /** SizeLegacy
             *
             *  Gets the size of the legacy memory pool.
             *
             **/
            uint32_t SizeLegacy();
        };

        extern Mempool mempool;
    }
}

#endif
