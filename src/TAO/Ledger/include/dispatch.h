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

#include <LLC/types/uint1024.h>

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

        class Dispatch
        {
            /** Mutex to protect the queue. **/
            std::mutex DISPATCH_MUTEX;


            /** Queue to handle dispatch requests. **/
            std::queue<uint1024_t> queueDispatch;


            /** Thread for running dispatch. **/
            std::thread DISPATCH_THREAD;


            /** Condition variable to wake up the relay thread. **/
            std::condition_variable CONDITION;

        public:

            /** Default Constructor. **/
            Dispatch();


            /** Default Destructor. **/
            ~Dispatch();


            /** Singleton instance. **/
            static Dispatch& GetInstance();


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
}

#endif
