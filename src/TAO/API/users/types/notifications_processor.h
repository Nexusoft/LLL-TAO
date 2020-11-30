/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/users/types/notifications_thread.h>

#include <LLC/types/uint1024.h>

#include <Util/include/mutex.h>


#include <vector>



/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /** NotificationsProcessor Class
         *
         *  Manages the notifications processing threads.
         *
         **/
        class NotificationsProcessor 
        {

        public:

            /** Default Constructor. **/
            NotificationsProcessor(const uint16_t& nThreads);


            /** Destructor. **/
            ~NotificationsProcessor();


            /** Add
             *
             *  Finds the least used notifications thread and adds the session ID to it
             *
             *  @param[in] nSession The session ID to process notifications for
             * 
             **/
            void Add(const uint256_t& nSession);


            /** Remove
             *
             *  Removes a session ID from the processor threads
             *
             *  @param[in] nSession The session ID to remove
             * 
             **/
            void Remove(const uint256_t& nSession);


            /** FindThread
             *
             *  Finds the notifications thread that is processing the given session ID
             *
             *  @param[in] nSession The session ID to search for
             * 
             **/
            NotificationsThread* FindThread(const uint256_t& nSession) const;
 

          private:

            uint16_t MAX_THREADS;


            /** Mutex to prevent threads being added/removed concurrently. **/
            mutable std::mutex MUTEX;


            /** Vector of notifications processor threads **/
            std::vector<NotificationsThread*> NOTIFICATIONS_THREADS;

        };

    }
}
