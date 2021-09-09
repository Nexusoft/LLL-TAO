/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>

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
    Index::Index()
    : EVENTS_QUEUE  (new std::queue<uint512_t>())
    , EVENTS_THREAD (std::bind(&Index::Manager, this))
    , CONDITION     ( )
    {
    }


    /* Default destructor. */
    Index::~Index()
    {
        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();
    }


    /* Checks current events against transaction history to ensure we are up to date. */
    void Index::RefreshEvents()
    {
        /* Our list of transactions to read. */
        std::vector<TAO::Ledger::Transaction> vtx;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Check if we have a last entry. */
        uint512_t hashLast;

        /* See if we need to bootstrap indexes. */
        const bool fIndexed =
            LLD::Logical->ReadLastIndex(hashLast);

        /* Handle first key if needed. */
        if(!fIndexed)
        {
            /* Grab our starting txid by quick read. */
            if(LLD::Ledger->BatchRead("tx", vtx, 1))
                hashLast = vtx[0].GetHash();

            debug::log(0, FUNCTION, "Initializing indexing tx ", hashLast.SubString());
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::log(0, FUNCTION, "Scanning from tx ", hashLast.SubString());
        while(!config::fShutdown.load())
        {
            /* Read the next batch of inventory. */
            std::vector<TAO::Ledger::Transaction> vtx;
            if(!LLD::Ledger->BatchRead(hashLast, "tx", vtx, 1000, fIndexed))
                break;

            /* Loop through found transactions. */
            for(const auto& tx : vtx)
            {
                /* Iterate the transaction contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Process our command-set indexing. */
                    Commands::Get("names") ->Index(rContract, nContract);
                    Commands::Get("market")->Index(rContract, nContract);
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Processed ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Set hash Last. */
            hashLast = vtx.back().GetHash();

            /* Check for end. */
            if(vtx.size() != 1000)
                break;
        }

        /* Write our last index now. */
        LLD::Logical->WriteLastIndex(hashLast);

        debug::log(0, FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }


    /*  Index a new block hash to relay thread.*/
    void Index::Push(const uint512_t& hashTx)
    {
        EVENTS_QUEUE->push(hashTx);
        CONDITION.notify_one();
    }


    /* Handle relays of all events for LLP when processing block. */
    void Index::Manager()
    {
        /* Refresh our events. */
        RefreshEvents();

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

            /* Iterate the transaction contracts. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Grab contract reference. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                /* Process our command-set indexing. */
                Commands::Get("names") ->Index(rContract, nContract);
                Commands::Get("market")->Index(rContract, nContract);
            }

            /* Write our last index now. */
            if(!LLD::Logical->WriteLastIndex(hashTx))
                continue;
        }
    }
}
