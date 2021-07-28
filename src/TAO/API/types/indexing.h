/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <Util/templates/singleton.h>
#include <Util/types/lock_shared_ptr.h>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class
     *
     *  This class is responsible for dispatching indexing events to the various API calls.
     *  These events are transactions that are unpacked into their according command-sets.
     *
     **/
    class Indexing : public Singleton<Indexing>
    {
        /** Queue to handle dispatch requests. **/
        util::atomic::lock_shared_ptr<std::queue<uint512_t>> EVENTS_QUEUE;


        /** Thread for running dispatch. **/
        std::thread EVENTS_THREAD;


        /** Condition variable to wake up the relay thread. **/
        std::condition_variable CONDITION;


    public:

        /** Default Constructor. **/
        Indexing();


        /** Default Destructor. **/
        ~Indexing();


        /** Push
         *
         *  Indexing a new transaction to relay thread.
         *
         *  @param[in] hashTx The txid to dispatch indexing for.
         *
         **/
        void Push(const uint512_t& hashTx);


        /** Relay Thread
         *
         *  Handle indexing of all events for API.
         *
         **/
        void Manager();
    };
}
