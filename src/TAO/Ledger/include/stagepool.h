/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STAGEPOOL_H
#define NEXUS_TAO_LEDGER_INCLUDE_STAGEPOOL_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/transaction.h>

#include <cstdint>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** StagePool
         *
         *  [Option A] Block-context staging pool for missing block transactions.
         *
         *  A tritium block is not self-contained on the wire: TritiumBlock::Check()
         *  resolves the tx hashes in vtx from disk/mempool, and missing txs are
         *  re-requested from peers.  Historically the ONLY door those re-requested
         *  txs could enter through was mempool.Accept(), which enforces
         *  tip-relative admission policy (e.g. coinbase maturity measured at the
         *  local tip).  A tx that is consensus-valid inside its containing block
         *  but policy-invalid at the local tip therefore deadlocks the node:
         *  the block can't complete without the tx, the tx can't be admitted
         *  until the tip advances, and the tip can't advance without the block.
         *
         *  The StagePool decouples block completion from mempool policy.  When a
         *  block is marked PROCESS::INCOMPLETE, its missing tritium txids are
         *  Register()-ed as "wanted".  When such a tx later arrives and is
         *  rejected by mempool policy, the LLP layer stages it here (after basic
         *  tx.Check() validation) instead of dropping it.  TritiumBlock::Check()
         *  and TritiumBlock::Accept() fall back to the StagePool when a vtx entry
         *  is in neither disk nor mempool, so the tx is finally validated in the
         *  proper BLOCK context (Transaction::Connect(FLAGS::BLOCK, pblock))
         *  where maturity is measured at the containing block's height.
         *
         *  Security: a txid can only become "wanted" via a block that has passed
         *  every other Check() validation (including proof of work) except tx
         *  resolution, so an attacker cannot cheaply force arbitrary data to be
         *  staged; staged txs must additionally pass Transaction::Check()
         *  (signature/format) before staging, and full contextual validation
         *  still happens at block connect inside the database transaction.
         *
         *  All entries are TTL-evicted and both maps are size-bounded (same cheap
         *  clear-all DoS guard rationale as mapLastMissing in process.h).
         **/
        namespace StagePool
        {

            /** Maximum number of "wanted" txids tracked before the map is cleared. **/
            const uint64_t MAX_WANTED_ENTRIES = 10000;


            /** Maximum number of staged transactions held before the map is cleared. **/
            const uint64_t MAX_STAGED_ENTRIES = 1000;


            /** Seconds a wanted/staged entry lives before TTL eviction. **/
            const uint64_t STAGE_TTL_SECONDS = 3600;


            /** Register
             *
             *  Mark a transaction hash as wanted by an incomplete block, making
             *  it eligible for staging when it arrives.
             *
             *  @param[in] hashTx The transaction hash referenced by the block's vMissing.
             *
             **/
            void Register(const uint512_t& hashTx);


            /** Wanted
             *
             *  Check whether a transaction hash has been registered as missing
             *  from an incomplete block (and has not TTL-expired).
             *
             *  @param[in] hashTx The transaction hash to check.
             *
             *  @return true if the transaction is wanted.
             *
             **/
            bool Wanted(const uint512_t& hashTx);


            /** Stage
             *
             *  Stage a transaction for block-context validation.  Only succeeds
             *  if the txid was previously Register()-ed as wanted.  The caller is
             *  responsible for running tx.Check() (signature/format validation)
             *  before staging.
             *
             *  @param[in] tx The transaction to stage.
             *
             *  @return true if the transaction was staged.
             *
             **/
            bool Stage(const TAO::Ledger::Transaction& tx);


            /** Get
             *
             *  Retrieve a staged transaction by hash.
             *
             *  @param[in] hashTx The transaction hash to look up.
             *  @param[out] tx The staged transaction, if found.
             *
             *  @return true if a non-expired staged transaction was found.
             *
             **/
            bool Get(const uint512_t& hashTx, TAO::Ledger::Transaction& tx);


            /** Erase
             *
             *  Remove a transaction from both the staged and wanted maps
             *  (e.g. after it has been committed to disk by block accept).
             *
             *  @param[in] hashTx The transaction hash to remove.
             *
             **/
            void Erase(const uint512_t& hashTx);


            /** Count
             *
             *  @return the number of currently staged transactions.
             *
             **/
            uint64_t Count();


            /** Clear
             *
             *  Remove all wanted and staged entries (used by tests and shutdown).
             *
             **/
            void Clear();
        }
    }
}

#endif
