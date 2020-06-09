/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>
#include <TAO/API/include/session.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/pinunlock.h>

#include <TAO/Register/types/state.h>

#include <Legacy/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** NotificationsThread Class
         *
         *  Processes notifications for a subset of logged in sessions.
         *
         **/
        class NotificationsThread 
        {

        private:

            /** the events flag for active oustanding events. **/
            std::atomic<bool> fEvent;


            /** the shutdown flag for gracefully shutting down events thread. **/
            std::atomic<bool> fShutdown;

            
            /** The mutex for events processing. **/
            mutable std::mutex NOTIFICATIONS_MUTEX;


            /** The condition variable to awaken sleeping notification thread. **/
            std::condition_variable CONDITION;


            /** The sigchain notifications processing thread. **/
            std::thread NOTIFICATIONS_THREAD;



        public:


            /** Default Constructor. **/
            NotificationsThread();


            /** Destructor. **/
            ~NotificationsThread();


            /** NotifyEvent
             *
             *  Notifies the processor that an event has occurred so it can check and update it's state.
             *
             **/
            void NotifyEvent();

            

          private:


            /** Thread
             *
             *  Background thread to handle/suppress sigchain notifications.
             *
             **/
            void Thread();

            

            /** auto_process_notifications
            *
            *  Process notifications for the currently logged in user(s)
            *
            **/
            void auto_process_notifications();

        };
    }
}
