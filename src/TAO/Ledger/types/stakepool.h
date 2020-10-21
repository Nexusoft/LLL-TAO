/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKEPOOL_H
#define NEXUS_TAO_LEDGER_TYPES_STAKEPOOL_H

#include <LLC/types/uint1024.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/types/transaction.h>

#include <map>
#include <mutex>
#include <tuple>
#include <vector>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Stakepool
         *
         *  A specialized memory pool for storing coinstake transactions used to build blocks
         *  by the pool stake minter.
         *
         **/
        class Stakepool
        {
        public:

            /* Mutex to local access to the mempool */
            mutable std::recursive_mutex MUTEX;

        private:

            /** The transactions in the stake pool.
             *  This maps the tx hash to the transaction plus saved metadata (tx, block age, balance, fee, fTrust)
             **/
            std::map<uint512_t, std::tuple<TAO::Ledger::Transaction, uint64_t, uint64_t, uint64_t, bool>> mapPool;


            /** Transactions received by the stake pool when its internal proof values are stale.
             *  This map acts as a holding area for any tx received before the proofs are set for the current mining round.
             **/
            std::map<uint512_t, TAO::Ledger::Transaction> mapPending;


            /** Maps user genesis to hashTx. Used to verify user only has one coinstake in pool. **/
            std::map<uint256_t, uint512_t> mapGenesis;


            /** The hash of the best block for the current mining round.
             *  Current proof values (hashProof, nTimeBegin, nTimeEnd) apply only for this hash.
             *  If the global hash best chain changes and no longer matches this value, proof values are stale.
             **/
            uint1024_t hashLastBlock;


            /** The coinstake produced by the local node. This transaction will not be included in Select results. **/
            uint512_t hashLocal;


            /** Proof in use by the pooled stake minter for the current mining round. **/
            uint256_t hashProof;


            /** Start time associated with hashProof **/
            uint64_t nTimeBegin;


            /** End time associated with hashProof **/
            uint64_t nTimeEnd;


            /** Maximum number of coinstake transactions added to the pool. Initially TAO::Ledger::POOL_MAX_SIZE_BASE **/
            uint64_t nSizeMax;


            /** AddToPool
             *
             *  Adds a transaction to the stake pool, making it available for pooled staking
             *
             *  @param[in] tx The transaction to add.
             *
             *  @return true if tx added successfully
             *
             **/
            bool AddToPool(const TAO::Ledger::Transaction& tx);


            /** CheckProofs
             *
             *  Verifies the proofs in a pooled coinstake transaction against those currently in use by the stake pool.
             *
             *  @param[in] tx The transaction to check.
             *
             *  @return true if proofs are valid.
             *
             **/
            bool CheckProofs(const TAO::Ledger::Transaction& tx);


        public:

            /** Default Constructor. **/
            Stakepool();


            /** Default Destructor. **/
            ~Stakepool();


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


            /** Get
             *
             *  Gets a transaction from stake pool
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @param[out] tx The retrieved transaction
             *
             *  @return true if pool contained transaction.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction &tx) const;


            /** Has
             *
             *  Check if a transaction exists in the stake pool.
             *
             *  @param[in] hashTx Hash of transaction to check.
             *
             *  @return true if pool contains the transaction.
             *
             **/
            bool Has(const uint512_t& hashTx) const;


            /** IsTrust
             *
             *  Checks whether a particular pool entry is a trust transaction.
             *
             *  @param[in] hashTx Hash of transaction to get.
             *
             *  @return true if in pool and a trust transaction, false otherwise (not in pool or genesis transaction)
             *
             **/
            bool IsTrust(const uint512_t& hashTx) const;


            /** Remove
             *
             *  Removes a transaction from stake pool.
             *
             *  @param[in] hashTx Hash of transaction to remove.
             *
             *  @return true if removed.
             *
             **/
            bool Remove(const uint512_t& hashTx);


            /** Clear
             *
             *  Clears all transactions and metadata from the stake pool to start fresh (new stake block).
             *
             *  This does not reset the maximum pool size.
             *
             **/
            void Clear();


            /** List
             *
             *  List coinstake transactions in stake pool.
             *
             *  This is a simple list of transactions in the order they were received.
             *
             *  @param[out] vHashes List of transaction hashes.
             *  @param[in] nCount The number of transactions to get.
             *
             *  @return true if list is not empty.
             *
             **/
            bool List(std::vector<uint512_t> &vHashes, const uint32_t nCount = std::numeric_limits<uint32_t>::max());


            /** Select
             *
             *  Select a list of coinstake transactions to use from the stake pool.
             *
             *  This method applies a weighted selection mechanism to create a list of transactions to use from the pool.
             *  Weightings are based on block age and stake balance for the user account producing the coinstake. Then, individual
             *  transactions are selected for inclusion based on probability determined by their relative weights.
             *
             *  If nCount asks for the entire pool (or more), this method simply returns a list of all transactions currently
             *  in the stake pool. In this case, the number in the results may be fewer than requested.
             *
             *  The value for nStakeTotal is determined from selected coinstakes. Usable stake balance for each pool
             *  transaction is capped to the amount of the local stake amount (nStake), then added to the total.
             *
             *  @param[out] vHashes List of transaction hashes.
             *  @param[out] nStakeTotal Net stake balance of selected coinstakes available to use.
             *  @param[out] nFeeTotal Net fees paid by the coinstakes in the results.
             *  @param[in] nStake Stake balance of local account.
             *  @param[in] nCount The number of transactions to select.
             *
             *  @return true if list is not empty.
             *
             **/
            bool Select(std::vector<uint512_t> &vHashes, uint64_t &nStakeTotal, uint64_t &nFeeTotal, const uint64_t& nStake,
                        const uint32_t& nCount = std::numeric_limits<uint32_t>::max());


            /** SetProofs
             *
             *  Updates the stake pool with data for the current mining round. The pool will only accept coinstakes that
             *  contain matching values.
             *
             *  @param[in] hashLastBlock Hash of the current best block at the start of the mining round.
             *  @param[in] hashProof Pooled coinstake proof value.
             *  @param[in] nTimeBegin Begin time for coinstake proof.
             *  @param[in] nTimeEnd  End time for coinstake proof.
             *
             **/
            void SetProofs(const uint1024_t& hashLastBlock, const uint256_t& hashProof,
                           const uint64_t& nTimeBegin, const uint64_t& nTimeEnd);


            /** GetMaxSize
             *
             *  Retrieve the current setting for maximum number of transactions to accept into the pool.
             *
             *  @return maximum size for pool
             *
             **/
            uint32_t GetMaxSize() const;


            /** SetMaxSize
             *
             *  Assigns the maximum number of transactions to accept into the pool.
             *
             *  @param[in] nSizeMax New maximum size for pool
             *
             **/
            void SetMaxSize(const uint32_t nSizeMax);


            /** Size
             *
             *  Gets the current size of the stake pool.
             *
             **/
            uint32_t Size();

        };

        extern Stakepool stakepool;
    }
}

#endif
