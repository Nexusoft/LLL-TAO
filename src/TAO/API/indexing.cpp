/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/indexing.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::API
{

    /* Default Constructor. */
    Indexing::Indexing()
    : EVENTS_QUEUE  (new std::queue<uint512_t>())
    , EVENTS_THREAD (std::bind(&Indexing::Manager, this))
    , CONDITION     ( )
    {
    }


    /* Default destructor. */
    Indexing::~Indexing()
    {
        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();
    }


    /*  Indexing a new block hash to relay thread.*/
    void Indexing::Push(const uint512_t& hashTx)
    {
        EVENTS_QUEUE->push(hashTx);
        CONDITION.notify_one();
    }


    /* Handle relays of all events for LLP when processing block. */
    void Indexing::Manager()
    {
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                return EVENTS_QUEUE->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Start a stopwatch. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Grab the next entry in the queue. */
            const uint512_t hashTx = EVENTS_QUEUE->front();
            EVENTS_QUEUE->pop();

            /* Make sure the transaction is on disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx))
                continue;

            /* Check all the tx contracts. */
            for(uint32_t n = 0; n < tx.Size(); ++n)
            {
                const TAO::Operation::Contract& rContract = tx[n];

                /* Check the contract's primitive. */
                const uint8_t nOP =
                    rContract.Operations()[0];

                debug::log(0, "Handling Contract for OP ", uint32_t(nOP));
            }
        }
    }
}
