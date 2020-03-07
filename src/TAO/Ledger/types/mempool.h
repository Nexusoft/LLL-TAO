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
        public:

            /* Mutex to local access to the mempool */
            mutable std::recursive_mutex MUTEX;

        private:

            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, Legacy::Transaction> mapLegacy;


            /** The transactions in conflicted legacy memory pool. */
            std::map<uint512_t, Legacy::Transaction> mapLegacyConflicts;


            /** Temporary holding area for orphan legacy transctions. **/
            std::map<uint512_t, Legacy::Transaction> mapLegacyOrphans;


            /** The transactions in the ledger memory pool. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapLedger;


            /** The transactions in the conflicted ledger memory pool. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapConflicts;


            /** Oprhan transactions in queue. **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapOrphans;


            /** Record of conflicted transactions in mempool. **/
            std::map<uint512_t, uint512_t> mapClaimed;


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
            bool Accept(const TAO::Ledger::Transaction& tx, LLP::TritiumNode* pnode = nullptr);


            /** Accept
             *
             *  Accepts a legacy transaction with validation rules.
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if added.
             *
             **/
            bool Accept(const Legacy::Transaction& tx, LLP::TritiumNode* pnode = nullptr);


            /** ProcessOrphans
             *
             *  Process orphan transactions if triggered in queue.
             *
             *  @param[in] hash The hash of spent output
             *
             *  @return true if spent.
             *
             **/
            void ProcessOrphans(const uint512_t& hash);


            /** ProcessLegacyOrphans
             *
             *  Process legacy orphan transactions if triggered in queue.
             *
             *  @param[in] hash The hash of spent output
             *  @param[in] pnode The node that orphans are being processed from.
             *
             *  @return true if spent.
             *
             **/
            void ProcessLegacyOrphans(const uint512_t& hash, LLP::TritiumNode* pnode = nullptr);


            /** IsSpent
             *
             *  Checks if a given output is spent in memory.
             *
             *  @param[in] hash The hash of spent output
             *  @param[in] n The output number being checked
             *
             *  @return true if spent.
             *
             **/
            bool IsSpent(const uint512_t& hash, const uint32_t n);


            /** Get
             *
             *  Gets a transaction from mempool including conflicted memory.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The retrieved transaction
             *  @param[out] fConflicted Flag to determine if transaction is conflicted
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx, bool &fConflicted) const;


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
             *  @param[out] fConflicted Flag to determine if transaction is conflicted
             *
             *  @return true if pool contained legacy transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, Legacy::Transaction &tx, bool &fConflicted) const;


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


            /** Check
             *
             *  Check the memory pool for consistency.
             *
             **/
            void Check();


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
            bool List(std::vector<uint512_t> &vHashes, uint32_t nCount = std::numeric_limits<uint32_t>::max(), bool fLegacy = false);


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
