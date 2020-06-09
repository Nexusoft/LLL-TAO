/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/include/sessionmanager.h>
#include <TAO/API/types/users.h>

#include <TAO/Ledger/include/chainstate.h>


namespace TAO
{
    namespace API
    {
        /*  Background thread to initiate user events . */
        void Users::NotificationsThread()
        {
            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* Reset the events flag. */
                fEvent = false;

                /* If mining is enabled, notify miner LLP that events processor is finished processing transactions so mined blocks
                   can include these transactions and not orphan a mined block. */
                if(LLP::MINING_SERVER)
                    LLP::MINING_SERVER.load()->NotifyEvent();

                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lock(EVENTS_MUTEX);
                CONDITION.wait_for(lock, std::chrono::milliseconds(5000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                try
                {
                    /* Ensure that the user is logged, in, wallet unlocked, and unlocked for notifications. */
                    if(LoggedIn())
                    { 
                        Session& session = GetSessionManager().Get(0);
                        if(!session.Locked() && session.CanProcessNotifications() && !TAO::Ledger::ChainState::Synchronizing())
                            auto_process_notifications();
                    }

                }
                catch(const std::exception& e)
                {
                    /* Log the error and attempt to continue processing */
                    debug::error(FUNCTION, e.what());
                }

            }
        }



        


        /* Process notifications for the currently logged in user(s) */
        void Users::auto_process_notifications()
        {
            /* Dummy params to pass into ProcessNotifications call */
            json::json params;

            /* Flag indicating the process should immediately retry upon failure.  This is flagged by certain exception codes */
            bool fRetry = false;

            /* As a safety measure we will break out after 100 retries */
            uint8_t nRetries = 0;

            do
            {
                try
                {
                    /* Invoke the process notifications method to process all oustanding */
                    ProcessNotifications(params, false);
                }
                catch(const APIException& ex)
                {
                    /* Absorb certain errors and write them to the log instead */
                    switch (ex.id)
                    {
                    case -202: // Signature chain not mature after your previous mined/stake block. X more confirmation(s) required
                    case -255: // Cannot process notifications until peers are connected
                    case -256: // Cannot process notifications whilst synchronizing
                    {
                        debug::log(2, FUNCTION, ex.what());
                        break;
                    }
                    case -257: // Contract failed peer validation
                    {
                        debug::log(2, FUNCTION, ex.what());

                        /* Immediately retry processing if a contract failed peer validation, as it will now be suppressed */
                        fRetry = true;

                        /* Increment the retry counter, so that we only retry 100 times */
                        nRetries++;

                        break;
                    }
                    default:
                        throw ex;
                    }
                }
            }
            while(fRetry && nRetries < 100);

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
