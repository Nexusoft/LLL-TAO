/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/dispatch.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Legacy/include/evaluate.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Default Constructor. */
        Dispatch::Dispatch()
        : DISPATCH_MUTEX ( )
        , vBlockDispatch ( )
        , vTransactionDispatch ( )
        {

        }


        /* Default destructor. */
        Dispatch::~Dispatch()
        {
            LOCK(DISPATCH_MUTEX);
            vBlockDispatch.clear();
            vTransactionDispatch.clear();
        }


        /* Singleton instance. */
        Dispatch& Dispatch::GetInstance()
        {
            static Dispatch ret;
            return ret;
        }


        /* Adds a subscripton for new blocks. */
        void Dispatch::SubscribeBlock(const BlockDispatchFunction& function)
        {
            /* Lock the mutex  */
            LOCK(DISPATCH_MUTEX);

            /* Add the function callback to our internal list */
            vBlockDispatch.push_back(function);
        }


        /* Adds a subscripton for new transactions. */
        void Dispatch::SubscribeTransaction(const TransactionDispatchFunction& function)
        {
            /* Lock the mutex  */
            LOCK(DISPATCH_MUTEX);

            /* Add the function callback to our internal list */
            vTransactionDispatch.push_back(function);
        }


        /* Notify all subscribers of a new block.*/
        void Dispatch::DispatchBlock(const uint1024_t& hashBlock)
        {
            /* Lock the mutex to synchronize access to the function pointer list */
            LOCK(DISPATCH_MUTEX);

            /* Call each one in turn in their own ephemeral thread */
            for(const auto& pBlockDispatch : vBlockDispatch)
            {
                std::thread([&]()
                {
                    pBlockDispatch(hashBlock);

                }).detach();
            }

        }


        /* Notify all subscribers of a new transaction.*/
        void Dispatch::DispatchTransaction(const uint512_t& hashTx, bool fConnect)
        {
            /* Lock the mutex to synchronize access to the function pointer list */
            LOCK(DISPATCH_MUTEX);

            /* Call each one in turn in their own ephemeral thread */
            for(const auto& pTransactionDispatch : vTransactionDispatch)
            {
                std::thread([&]()
                {
                    pTransactionDispatch(hashTx, fConnect);

                }).detach();
            }

        }

    }
}
