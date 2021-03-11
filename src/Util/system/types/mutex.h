/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <mutex>
#include <atomic>
#include <thread>

#include <shared_mutex>

/* Macro preprocessor definitions for debug purposes. */
#define LOCK(mut) std::unique_lock<std::mutex> lk(mut)
#define LOCK2(mut) std::unique_lock<std::mutex> lk2(mut)
#define RLOCK(mut) std::lock_guard<std::recursive_mutex> lk(mut)

/* Macro to support reader/writer locks. */
#define READER_LOCK(mut) std::shared_lock lk(mut)
#define WRITER_LOCK(mut) std::unique_lock lk(mut)

/* Variadic macro to support multiple locks in same macro. */
#define CRITICAL(...) std::scoped_lock<std::mutex>            __LOCK(__VA_ARGS__)
#define RECURSIVE(...) std::scoped_lock<std::recursive_mutex> __LOCK(__VA_ARGS__)


namespace util::system
{

    /** shared_recursive_mutex
     *
     *  A wrapper around shared_mutex to reference count and allow recursive shared locks.
     *
     **/
    class shared_recursive_mutex : public std::shared_mutex
    {
    public:

        /** lock
         *
         *  Locks the shared mutex if not increasing reference count.
         *
         **/
        void lock()
        {
            /* Take a peek at our current thread-id. */
            std::thread::id nThis = std::this_thread::get_id();

            /* Check if we are locking for current owner. */
            if(nOwner == nThis)
            {
                /* Increment reference count on same thread. */
                ++nCount;
            }
            else
            {
                /* Lock shared mutex on unique thread. */
                shared_mutex::lock();

                /* Set our new values. */
                nOwner = nThis;
                nCount.store(1);
            }
        }

        /** unlock
         *
         *  Unlocks the shared mutex while decreasing reference count.
         *
         **/
        void unlock()
        {
            /* Reduce reference count. */
            if(nCount > 1)
            {
                /* Decrement reference counter on unlock sequence. */
                --nCount;
            }
            else
            {
                /* Update our new values. */
                nOwner = std::thread::id();
                nCount.store(0);

                /* Unlock this thread now. */
                shared_mutex::unlock();
            }
        }

    private:

        /** Internal atomic value to track what thread we are locked for. */
        std::atomic<std::thread::id> nOwner;

        /** Track our internal reference counts. **/
        std::atomic<uint32_t> nCount;
    };
}
