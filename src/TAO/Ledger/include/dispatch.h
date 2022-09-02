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
#include <Util/types/lock_unique_ptr.h>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    /** @class
     *
     *  This class is responsible for dispatching events triggered by a new block.
     *  These events could be best chain pointers, transactions, contracts, or other relevant data.
     *
     **/
    class Dispatch : public Singleton<Dispatch>
    {
        /** Queue to handle dispatch requests. **/
        util::atomic::lock_unique_ptr<std::queue<uint1024_t>> DISPATCH_QUEUE;


        /** Thread for running dispatch. **/
        std::thread DISPATCH_THREAD;


        /** Condition variable to wake up the relay thread. **/
        std::condition_variable CONDITION;


    public:

        /** Default Constructor. **/
        Dispatch();


        /** Default Destructor. **/
        ~Dispatch();


        /** PushRelay
         *
         *  Dispatch a new block hash to relay thread.
         *
         *  @param[in] hashBlock The block hash to dispatch.
         *
         **/
        void PushRelay(const uint1024_t& hashBlock);


        /** Relay Thread
         *
         *  Handle relays of all events for LLP when processing block.
         *
         **/
        void Relay();
    };
}
