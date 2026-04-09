/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/indexing.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/chainstate.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /*  Index a new block hash to relay thread.*/
    void Indexing::PushTransaction(const uint512_t& hashTx)
    {
        /* Let's push the sessions indexes in the main processing thread. */
        if(!TAO::Ledger::ChainState::Synchronizing())
            IndexSession(hashTx);

        /* Next lets push to uur manager thread to handle the global indexes. */
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

            /* Check if handling legacy or tritium. */
            if(hashTx.GetType() == TAO::Ledger::TRITIUM)
            {
                /* Index our sessions code when syncing here in seperate thread. */
                if(TAO::Ledger::ChainState::Synchronizing())
                    IndexSession(hashTx);

                /* Make sure the transaction is on disk. */
                TAO::Ledger::Transaction tx;
                if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    /* Iterate the transaction contracts. */
                    for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
                    {
                        /* Grab contract reference. */
                        const TAO::Operation::Contract& rContract = tx[nContract];

                        {
                            LOCK(REGISTERED_MUTEX);

                            /* Loop through registered commands. */
                            for(const auto& strCommands : REGISTERED)
                                Commands::Instance(strCommands)->Index(rContract, nContract);
                        }
                    }

                    /* Write our last index now. */
                    LLD::Logical->WriteLastIndex(hashTx);
                }
            }
        }
    }
}
