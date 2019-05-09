/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace TAO
{
    namespace API
    {
        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            while(!config::fShutdown.load())
            {
                //uint32_t nSequence = 0;


                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lk(EVENTS_MUTEX);
                CONDITION.wait(lk, [this]{ return fEvent.load();});


                /* Scan sigchain by hashLast to hashPrev == 0 */


                /* Unpack all registers */


                /* Write in localDB with a genesis index (sequence, genesis) */


                /* Use the local DB cache for scanning events to any owned register address */


                /* Respond to all TRANSFER notifications with CLAIM. */
                //debug::log(0, FUNCTION, "Respond to TRANSFER notifications with CLAIM");
                //debug::log(0, FUNCTION, "Respond to DEBIT notifications with CREDIT");


                /* Respond to all DEBIT notifications with CREDIT. */


                /* Reset the events flag. */
                fEvent = false;
            }
        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void Users::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }
    }
}
