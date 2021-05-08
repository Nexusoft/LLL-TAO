/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/users/types/notifications_thread.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/sessionmanager.h>
#include <TAO/API/users/types/users.h>

#include <TAO/Ledger/include/chainstate.h>

#include <functional>

namespace TAO
{
    namespace API
    {
        /* Default Constructor. */
        NotificationsThread::NotificationsThread()
        : SESSIONS()
        , fEvent(false)
        , fShutdown(false)
        , NOTIFICATIONS_MUTEX()
        , CONDITION()
        , NOTIFICATIONS_THREAD(std::bind(&NotificationsThread::Thread, this))
        {

        }


        /* Destructor. */
        NotificationsThread::~NotificationsThread()
        {
            /* Set the shutdown flag and join events processing thread. */
            fShutdown = true;

            /* Events processor only enabled if multi-user session is disabled. */
            if(NOTIFICATIONS_THREAD.joinable())
            {
                NotifyEvent();
                NOTIFICATIONS_THREAD.join();
            }

        }

        /* Adds a session ID to be processed by this thread */
        void NotificationsThread::Add(const uint256_t& nSession)
        {
            /* lock the notifications mutex so we can access the sessions */
            LOCK(NOTIFICATIONS_MUTEX);

            /* Add the session if it is not already in the vector*/
            if(std::find(SESSIONS.begin(), SESSIONS.end(), nSession) == SESSIONS.end() )
                SESSIONS.push_back(nSession);

        }


        /* Removes a session ID from the list processed by this thread */
        void NotificationsThread::Remove(const uint256_t& nSession)
        {
            /* lock the notifications mutex so we can access the sessions */
            LOCK(NOTIFICATIONS_MUTEX);

            /* Remove the session if it is in the vector*/
            SESSIONS.erase(std::remove(SESSIONS.begin(), SESSIONS.end(), nSession));
        }


        /* Checks to see if session ID is being processed by this thread*/
        bool NotificationsThread::Has(const uint256_t& nSession) const
        {
            /* lock the notifications mutex so we can access the sessions */
            LOCK(NOTIFICATIONS_MUTEX);

            /* Check if the sessions vector contains the session id */
            return std::find(SESSIONS.begin(), SESSIONS.end(), nSession) != SESSIONS.end();
        }


        /*  Background thread to initiate user events . */
        void NotificationsThread::Thread()
        {
            /** The interval between processing notifications in milliseconds, defaults to 5s if not specified in config **/
            uint64_t nInterval = config::GetArg("-notificationsinterval", 1) * 1000;

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* Reset the events flag. */
                fEvent = false;

                /* If mining is enabled, notify miner LLP that events processor is finished processing transactions so mined blocks
                   can include these transactions and not orphan a mined block. */
                if(LLP::MINING_SERVER)
                    LLP::MINING_SERVER->NotifyEvent();

                runtime::sleep(nInterval);

                /* Wait for the events processing thread to be woken up (such as a login) */
                //std::unique_lock<std::mutex> lock(NOTIFICATIONS_MUTEX);
                //CONDITION.wait_for(lock, std::chrono::milliseconds(nInterval), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                /* Check we're not synchronizing */
                if(TAO::Ledger::ChainState::Synchronizing())
                    continue;

                /* Iterate through all sessions */
                for(const auto nSession : SESSIONS)
                {
                    try
                    {
                        /* Ensure that the user is logged, in, wallet unlocked, and unlocked for notifications. */
                        if(GetSessionManager().Has(nSession))
                        {
                            Session& session = GetSessionManager().Get(nSession, false);
                            if(!session.Locked() && session.CanProcessNotifications())
                                auto_process_notifications(session.ID());
                        }

                    }
                    catch(const std::exception& e)
                    {
                        /* Log the error and attempt to continue processing */
                        debug::error(FUNCTION, e.what());
                    }
                }

            }
        }






        /* Process notifications for the currently logged in user(s) */
        void NotificationsThread::auto_process_notifications(const uint256_t& nSession)
        {
            /* Dummy params to pass into ProcessNotifications call */
            json::json params;

            /* Set the session ID in the params */
            params["session"] = nSession.ToString();

            /* Set flag to not reset the session activity, since this is an automated process */
            params["logactivity"] = false;

            /* Flag indicating the process should immediately retry upon failure.  This is flagged by certain exception codes */
            bool fRetry = false;

            /* As a safety measure we will break out after 100 retries */
            uint8_t nRetries = 0;

            do
            {
                try
                {
                    /* Reset the retry flag */
                    fRetry = false;

                    /* Invoke the process notifications method to process all oustanding */
                    TAO::API::users->ProcessNotifications(params, false);
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

                        /* Ensure we don't retry */
                        fRetry = false;

                        break;
                    }
                    case -257: // Contract failed peer validation
                    {
                        debug::log(2, FUNCTION, ex.what());

                        /* Immediately retry processing if a contract failed peer validation, as it will now be suppressed */
                        fRetry = true;

                        /* Increment the retry counter, so that we only retry 10 times */
                        nRetries++;

                        /* Sleep to yield CPU cycles */
                        runtime::sleep(10);

                        break;
                    }
                    default:
                        throw ex;
                    }
                }
            }
            while(fRetry && nRetries < 10);

        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void NotificationsThread::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }


    }
}
