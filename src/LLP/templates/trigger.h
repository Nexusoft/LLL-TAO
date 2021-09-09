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

#include <LLP/include/version.h>

#include <Util/templates/datastream.h>

namespace LLP
{
    class Trigger
    {
        /** The condition variable that is wrapped by this class. **/
        std::condition_variable CONDITION;


        /** Mutex to protect data stream. **/
        mutable std::mutex TRIGGER_MUTEX;


        /** The data stream for args. **/
        DataStream ssArgs;

    public:

        /** Default Constructor. **/
        Trigger()
        : CONDITION     ( )
        , TRIGGER_MUTEX ( )
        , ssArgs        (SER_NETWORK, MIN_PROTO_VERSION)
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


        /** HasArgs
         *
         *  Checks if a trigger has arguments packed into trigger.
         *
         **/
        bool HasArgs() const
        {
            LOCK(TRIGGER_MUTEX);
            return (ssArgs.size() > 0);
        }


        /** SetArgs
         *
         *  Set's the internal arguments
         *
         *  @param[in] nNonceIn The nonce value to set.
         *
         **/
        template<typename... Args>
        void SetArgs(Args&&... args)
        {
            LOCK(TRIGGER_MUTEX);

            /* Clear the stream and unpack args. */
            ssArgs.clear();
            message_args(ssArgs, std::forward<Args>(args)...);

            /* Reset read pointer. */
            ssArgs.Reset();
        }


        /** wait_for_nonce
         *
         *  Wait until designated nonce has been fired with a trigger.
         *
         *  @param[in] nTriggerNonce The nonce to wait for in trigger response.
         *
         **/
        bool wait_for_nonce(const uint64_t nTriggerNonce, const uint64_t nTimeout = 10000)
        {
            /* Create the mutex for the condition variable. */
            std::mutex REQUEST_MUTEX;
            std::unique_lock<std::mutex> REQUEST_LOCK(REQUEST_MUTEX);

            /* Wait for trigger to complete. */
            return CONDITION.wait_for(REQUEST_LOCK, std::chrono::milliseconds(nTimeout),
            [this, nTriggerNonce]
            {
                LOCK(TRIGGER_MUTEX);

                /* Reset the stream. */
                ssArgs.Reset();
                if(ssArgs.size() == 0)
                    return false;

                /* Check for genesis. */
                uint64_t nNonce = 0;
                ssArgs >> nNonce;

                /* Check the nonce for trigger. */
                if(nNonce == nTriggerNonce)
                    return true;

                return false;
            });
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


        /** Operator Overload >>
         *
         *  Serializes data into vchOperations.
         *
         *  @param[out] obj The object to de-serialize from ledger data.
         *
         **/
        template<typename Type>
        const Trigger& operator>>(Type& obj) const
        {
            LOCK(TRIGGER_MUTEX);
            ssArgs >> obj;

            return *this;
        }
    };
}

#endif
