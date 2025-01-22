/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2023

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/indexing.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /*  Index a new block hash to relay thread.*/
    void Indexing::PushTransaction(const uint512_t& hashTx)
    {
        DISPATCH->push(hashTx);
        CONDITION.notify_all();

        debug::log(3, FUNCTION, "Pushing ", hashTx.SubString(), " To Indexing Queue.");
    }


    /* Handle relays of all events for LLP when processing block. */
    void Indexing::Manager()
    {
        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            []
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                /* Check for suspended state. */
                if(config::fSuspended.load())
                    return false;

                return Indexing::DISPATCH->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Grab the next entry in the queue. */
            const uint512_t hashTx = DISPATCH->front();
            DISPATCH->pop();

            /* Fire off indexing now. */
            IndexSigchain(hashTx);

            /* Write our last index now. */
            LLD::Sessions->WriteLastIndex(hashTx);
        }
    }
}
