/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/API/types/commands.h>

#include <Util/templates/singleton.h>
#include <Util/types/lock_unique_ptr.h>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

//forward declarations
namespace TAO::Ledger { class Transaction; }

/* Global TAO namespace. */
namespace TAO::API
{

    /** @class
     *
     *  This class is responsible for dispatching indexing events to the various API calls.
     *  These events are transactions that are unpacked into their according command-sets.
     *
     **/
    class Indexing
    {
        /** Queue to handle dispatch requests. **/
        static util::atomic::lock_unique_ptr<std::queue<uint512_t>> DISPATCH;


        /** Thread for running dispatch. **/
        static std::thread EVENTS_THREAD;


        /** Condition variable to wake up the indexing thread. **/
        static std::condition_variable CONDITION;


        /** Queue to handle dispatch requests. **/
        static util::atomic::lock_unique_ptr<std::queue<uint256_t>> INITIALIZE;


        /** Thread for running dispatch. **/
        static std::thread INITIALIZE_THREAD;


        /** Condition variable to wake up the indexing thread. **/
        static std::condition_variable INITIALIZE_CONDITION;


        /** Set to track active indexing entries. **/
        static std::set<std::string> REGISTERED;


        /** Mutex around registration. **/
        static std::mutex REGISTERED_MUTEX;


    public:

        /** Initialize
         *
         *  Initializes the current indexing systems.
         *
         **/
        static void Initialize();


        /** Initialize
         *
         *  Initialize a user's indexing entries.
         *
         *  @param[in] hashGenesis The genesis-id to initialize.
         *
         **/
        static void Initialize(const uint256_t& hashGenesis);


        /** RefreshEvents
         *
         *  Checks current events against transaction history to ensure we are up to date.
         *
         **/
        static void RefreshEvents();


        /** PushTransaction
         *
         *  Index a new block to relay thread.
         *
         *  @param[in] hashTx The txid to dispatch indexing for.
         *
         **/
        static void PushTransaction(const uint512_t& hashTx);


        /** Register
         *
         *  Register a new command-set to indexing by class type.
         *
         **/
        template<typename Type>
        static void Register()
        {
            /* Grab a copy of our name. */
            const std::string strCommands = Type::Name();
            if(!Commands::Has(strCommands))
                return; //we just exit if already registered

            LOCK(REGISTERED_MUTEX);
            REGISTERED.insert(strCommands);

            debug::log(0, FUNCTION, "Registered ", VARIABLE(strCommands));
        }


        /** Manager Thread
         *
         *  Handle indexing of all events for API.
         *
         **/
        static void Manager();


        /** Initialize Thread
         *
         *  Handle indexing of all events for API.
         *
         **/
        static void InitializeThread();


        /** IndexDependant
         *
         *  Index transaction level events for logged in sessions.
         *
         *  @param[in] hash The txid of the transactioun
         *  @param[in] tx The transaction to index events for.
         *
         **/
        static void IndexDependant(const uint512_t& hash, const TAO::Ledger::Transaction& tx);


        /** Shutdown
         *
         *  Shuts down the current indexing systems.
         *
         **/
        static void Shutdown();


    private:


        /** index_events
         *
         *  Index transaction level events for logged in sessions.
         *
         *  @param[in] hash The txid of the transactioun
         *  @param[in] tx The transaction to index events for.
         *
         **/
        static void index_transaction(const uint512_t& hash, const TAO::Ledger::Transaction& tx);


    };
}
