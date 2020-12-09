/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_DISPATCH_H
#define NEXUS_TAO_LEDGER_INCLUDE_DISPATCH_H

#include <Common/types/uint1024.h>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        
        /** Function prototype for methods wanting to be notified of new blocks being added to the chain **/
        typedef std::function<void(const uint1024_t&)> BlockDispatchFunction;

        /** Function prototype for methods wanting to be notified of transactions **/
        typedef std::function<void(const uint512_t&, bool)> TransactionDispatchFunction;

        class Dispatch
        {
            /** Mutex to protect the queue. **/
            std::mutex DISPATCH_MUTEX;


            /** List of subscribers to Block events  **/
            std::vector<BlockDispatchFunction> vBlockDispatch;

            /** List of subscribers to transaction events  **/
            std::vector<TransactionDispatchFunction> vTransactionDispatch;


        public:

            /** Default Constructor. **/
            Dispatch();


            /** Default Destructor. **/
            ~Dispatch();


            /** Singleton instance. **/
            static Dispatch& GetInstance();


            /** SubscribeBlock
             *
             *  Adds a subscripton for new blocks.
             *
             *  @param[in] notify The function to call for new block notifications.
             *
             **/
            void SubscribeBlock(const BlockDispatchFunction& notify);


            /** SubscribeTransaction
             *
             *  Adds a subscripton for new transactions.
             *
             *  @param[in] notify The function to call for new transaction notifications.
             *
             **/
            void SubscribeTransaction(const TransactionDispatchFunction& notify);


            /** DispatchBlock
             *
             *  Notify all subscribers of a new block .
             *
             *  @param[in] hashBlock The block hash of the new block.
             * 
             **/
            void DispatchBlock(const uint1024_t& hashBlock);


            /** DispatchTransaction
             *
             *  Notify all subscribers of a new transaction .
             *
             *  @param[in] hashTx The hash of the new transaction.
             *  @param[in] fConnect Flag indicating whether the transaction is being connected or disconnected from the chain.
             *
             **/
            void DispatchTransaction(const uint512_t& hashTx, bool fConnect);


        };

    }
}

#endif
