/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_CONDITION_VARIABLE_H
#define NEXUS_UTIL_INCLUDE_CONDITION_VARIABLE_H

#include <map>
#include <functional>

#include <Util/include/mutex.h>


/** condition_variable
 *
 *  Class to handle condition variable style thread wakeups and sleeps for synchronization that does not
 *  have the issues with being woken up or put to sleep due to OS limitations. This is portable and uses
 *  mutexes to manage per-thread sleeping and waking. It does not support wait_for or wait_if.
 *
 **/
class condition_variable
{
    /** Internal mutex for locking between thread signals. **/
    std::map<std::thread::id, std::mutex> CONDITION_MUTEX;


    /** Internal memory state mutex for locking internal value accesses. **/
    std::mutex INTERNAL_MUTEX;


    /** Function predicate for signaling logic. **/
    std::map<std::thread::id, std::function<bool(void)>> PREDICATE;


    /** Rotation variable for notifying single threads round robin. **/
    uint32_t ROTATION;

public:

    /** Default Constructor. **/
    condition_variable( )
    : CONDITION_MUTEX ( )
    , INTERNAL_MUTEX  ( )
    , PREDICATE       ( )
    , ROTATION        (0)
    {
    }


    /** notify_one
     *
     *  Notify the sleeping thread to wake up if predicate is satisfied.
     *
     **/
    void notify_one()
    {
        LOCK(INTERNAL_MUTEX);

        /* Select a thread. */
        uint32_t SLOT = 0;
        for(auto& MUTEX : CONDITION_MUTEX)
        {
            /* Check for rotation slot. */
            if(SLOT++ != ROTATION)
                continue;

            /* Unlock mutex. */
            if(PREDICATE[MUTEX.first]())
                MUTEX.second.unlock();

            break;
        }

        /* Handle rotation cycle reset. */
        if(++ROTATION == CONDITION_MUTEX.size())
            ROTATION = 0;
    }


    /** notify_all
     *
     *  Notify all sleeping threads to wake up if their predicates are satisfied.
     *
     **/
    void notify_all()
    {
        LOCK(INTERNAL_MUTEX);

        /* Select a thread. */
        for(auto& MUTEX : CONDITION_MUTEX)
        {
            /* Unlock mutex. */
            if(PREDICATE[MUTEX.first]())
                MUTEX.second.unlock();
        }
    }


    /** Wait
     *
     *  Set a thread to be in sleep mode until woken up.
     *
     **/
    void wait()
    {
        /* Protect internal memory maps. */
        INTERNAL_MUTEX.lock();

        /* Get thread identifier. */
        std::thread::id ID = std::this_thread::get_id();

        /* Add values for wait. */
        PREDICATE[ID]     = []{ return true; };
        std::mutex& MUTEX = CONDITION_MUTEX[ID];

        /* Unlock before we wait. */
        INTERNAL_MUTEX.unlock();

        /* Lock to wait. */
        MUTEX.lock();
    }


    /** Wait
     *
     *  Wait with a predicate function that defines wake up behavior.
     *
     *  @param[in] predicate The function defining wake up conditions.
     *
     **/
    void wait(std::function<bool(void)> predicate)
    {
        /* Protect internal memory maps. */
        INTERNAL_MUTEX.lock();

        /* Get thread identifier. */
        std::thread::id ID = std::this_thread::get_id();

        /* Apply the predicate to current thread. */
        PREDICATE[ID]     = predicate;
        std::mutex& MUTEX = CONDITION_MUTEX[ID];

        /* Unlock before we wait. */
        INTERNAL_MUTEX.unlock();

        /* Lock to wait. */
        MUTEX.lock();
    }
};

#endif
