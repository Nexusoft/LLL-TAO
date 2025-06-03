/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint512_t>> Indexing::DISPATCH;


    /* Thread for running dispatch. */
    std::thread Indexing::EVENTS_THREAD;


    /* Thread for refreshing events. */
    std::thread Indexing::REFRESH_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::CONDITION;


    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint256_t>> Indexing::INITIALIZE;


    /* Thread for running dispatch. */
    std::thread Indexing::INITIALIZE_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::INITIALIZE_CONDITION;


    /* Set to track active indexing entries. */
    std::set<std::string> Indexing::REGISTERED;


    /* Mutex around registration. */
    std::mutex Indexing::REGISTERED_MUTEX;


    /* Initializes the current indexing systems. */
    void Indexing::Initialize()
    {
        /* Read our list of active login sessions. */

        /* Initialize our thread objects now. */
        Indexing::DISPATCH       = util::atomic::lock_unique_ptr<std::queue<uint512_t>>(new std::queue<uint512_t>());
        Indexing::EVENTS_THREAD  = std::thread(&Indexing::Manager);
        Indexing::REFRESH_THREAD = std::thread(&Indexing::RefreshEvents);


        /* Initialize our indexing thread now. */
        Indexing::INITIALIZE    = util::atomic::lock_unique_ptr<std::queue<uint256_t>>(new std::queue<uint256_t>());
        Indexing::INITIALIZE_THREAD = std::thread(&Indexing::InitializeThread);
    }


    /* Initialize a user's indexing entries. */
    void Indexing::Initialize(const uint256_t& hashSession)
    {
        /* Add our genesis to initialize ordering. */
        INITIALIZE->push(hashSession);
        INITIALIZE_CONDITION.notify_all();
    }


    /* Default destructor. */
    void Indexing::InitializeThread()
    {
        /* List our current active sessions. */
        std::map<uint256_t, uint64_t> mapSessions;
        if(LLD::Sessions->ListAccesses(mapSessions, SESSION_TIMEOUT))
        {
            /* Loop through our active sessions and build indexes. */
            for(const auto& rSession : mapSessions)
            {
                /* Build our indexes if we are not in -client mode. */
                if(!config::fClient.load())
                    BuildIndexes(rSession.first);
            }
        }

        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            INITIALIZE_CONDITION.wait_for(CONDITION_LOCK, std::chrono::milliseconds(500),
            [&]
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                return Indexing::INITIALIZE->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Check that we have items in the queue. */
            if(Indexing::INITIALIZE->empty())
                continue;

            /* Get the current genesis-id to initialize for. */
            const uint256_t hashSession = INITIALIZE->front();
            INITIALIZE->pop();

            /* Get our current genesis-id. */
            const uint256_t hashGenesis =
                Authentication::Caller(hashSession);

            /* Give us some debug info so we know this has triggered. */
            debug::log(0, FUNCTION, "Initializing Dynamic Indexing Services for ", hashGenesis.SubString());

            /* Write our current time to the database. */
            LLD::Sessions->WriteAccess(hashGenesis, runtime::unifiedtimestamp());

            /* Check that our indexes are built. */
            BuildIndexes(hashGenesis);

            /* Set our indexing status to ready now. */
            Authentication::SetReady(hashSession);

            /* Debug output to track our sequences. */
            debug::log(0, FUNCTION, "Dynamic Indexing Services Initialized for ", hashGenesis.SubString());
            //debug::log(0, FUNCTION, "Completed building indexes at ", VARIABLE(nLegacySequence), " | ", VARIABLE(nLogicalSequence), " | ", VARIABLE(nLedgerHeight), " | ", VARIABLE(nLogicalHeight), " for genesis=", hashGenesis.SubString());
        }
    }


    /* Default destructor. */
    void Indexing::Shutdown()
    {
        /* Cleanup our initialization thread. */
        INITIALIZE_CONDITION.notify_all();
        if(INITIALIZE_THREAD.joinable())
            INITIALIZE_THREAD.join();

        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();

        /* Cleanup our dispatch thread. */
        if(REFRESH_THREAD.joinable())
            REFRESH_THREAD.join();

        /* Clear open registrations. */
        {
            LOCK(REGISTERED_MUTEX);
            REGISTERED.clear();
        }
    }
}
