/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_TRIGGER_H
#define NEXUS_LLP_TEMPLATES_TRIGGER_H

#include <condition_variable>
#include <cstdint>

namespace LLP
{
    class Trigger
    {
        /** Internal locking. **/
        mutable std::mutex MUTEX;

        /** The condition variable that is wrapped by this class. **/
        std::condition_variable CONDITION;

        /** The nonce associated with the trigger. **/
        std::atomic<uint64_t> nNonce;

    public:

        /** Default Constructor. **/
        Trigger()
        : CONDITION ( )
        , nNonce    (0)
        {
        }


        /** Constructor taking nonce. **/
        Trigger(const uint64_t nNonceIn)
        : CONDITION ( )
        , nNonce    (nNonceIn)
        {
        }


        /** Copy Constructor. **/
        Trigger(const Trigger& in)            = delete;


        /** Move Constructor. **/
        Trigger(Trigger&& in)                 = delete;


        /** Copy Assignment. **/
        Trigger& operator=(const Trigger& in) = delete;


        /** Move Assignment. **/
        Trigger& operator=(Trigger&& in)      = delete;


        /** Destructor. **/
        ~Trigger()
        {
        }


        /** Wrapper for notify_one(). **/
        void notify_one()
        {
            CONDITION.notify_one();
        }


        /** Wrapper for notify_all(). **/
        void notify_all()
        {
            CONDITION.notify_all();
        }


        /** SetNonce
         *
         *  Set's the internal nonce's value.
         *
         *  @param[in] nNonceIn The nonce value to set.
         *
         **/
        void SetNonce(const uint64_t nNonceIn)
        {
            nNonce.store(nNonceIn);
        }


        /** GetNonce
         *
         *  Gets the internal nonce's value.
         *
         *  @return the current nonce's value.
         *
         **/
        uint64_t GetNonce() const
        {
            return nNonce;
        }


        /** wait_for
         *
         *  wrapper for std::condition_variable::wait_for with Predicate
         *
         **/
        template <class Rep, class Period, class Predicate>
        bool wait_for (std::unique_lock<std::mutex>& lck, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
        {
            return CONDITION.wait_for(lck, rel_time, pred);
        }


        /** wait_for
         *
         *  wrapper for std::condition_variable::wait_for without Predicate
         *
         **/
        template <class Rep, class Period>
        bool wait_for (std::unique_lock<std::mutex>& lck, const std::chrono::duration<Rep, Period>& rel_time)
        {
            return CONDITION.wait_for(lck, rel_time);
        }
    };
}

#endif
