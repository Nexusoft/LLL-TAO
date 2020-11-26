/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/users/types/notifications_processor.h>

namespace TAO
{
    namespace API
    {
        /* Default Constructor. */
        NotificationsProcessor::NotificationsProcessor(const uint16_t& nThreads)
        : MAX_THREADS(nThreads)
        , MUTEX()
        , NOTIFICATIONS_THREADS()
        {
        }


        /* Destructor. */
        NotificationsProcessor::~NotificationsProcessor()
        {
            /* lock the  mutex so we can access the threads */
            LOCK(MUTEX);

            /* Delete the notifications processor threads. */
            for(uint16_t nIndex = 0; nIndex < NOTIFICATIONS_THREADS.size(); ++nIndex)
            {
                delete NOTIFICATIONS_THREADS[nIndex];
                NOTIFICATIONS_THREADS[nIndex] = nullptr;
            }
        }

        /* Finds the least used notifications thread and adds the session ID to it*/
        void NotificationsProcessor::Add(const uint256_t& nSession)
        {
            /* lock the  mutex so we can access the sessions */
            LOCK(MUTEX);

            /* The notifications thread to add the session to */
            NotificationsThread* pThread = nullptr;

            /* Add notifications threads on demand if current thread count is less than max threads */
            if(NOTIFICATIONS_THREADS.size() < MAX_THREADS)
            {
                pThread = new NotificationsThread();
                NOTIFICATIONS_THREADS.push_back(pThread);
            }
            else
            {
                /* Find the thread managing the least number of sessions */    
            
                /* The number of sessions being managed by the least loaded thread */
                uint32_t nSessions = std::numeric_limits<uint32_t>::max();

                for(uint16_t nIndex = 0; nIndex < NOTIFICATIONS_THREADS.size(); ++nIndex)
                {
                    /* Find least loaded thread */
                    if(NOTIFICATIONS_THREADS[nIndex]->SESSIONS.size() < nSessions)
                    {
                        pThread = NOTIFICATIONS_THREADS[nIndex];
                        nSessions = pThread->SESSIONS.size();
                    }
                }
            }

            /* Check that we found a thread */
            if(pThread)
                /* Add the session to the thread */
                pThread->Add(nSession);
        }


        /* Removes a session ID from the processor threads */
        void NotificationsProcessor::Remove(const uint256_t& nSession)
        {
            /* lock the notifications mutex so we can access the threads */
            LOCK(MUTEX);

            /* Iterate through notifications threads to find a match */
            for(uint16_t nIndex = 0; nIndex < NOTIFICATIONS_THREADS.size(); ++nIndex)
            {
                /* If the thread is configured for this session then remove it */
                if(NOTIFICATIONS_THREADS[nIndex]->Has(nSession))
                    NOTIFICATIONS_THREADS[nIndex]->Remove(nSession);
            }

        }


        /* Finds the notifications thread that is processing the given session ID */
        NotificationsThread* NotificationsProcessor::FindThread(const uint256_t& nSession) const
        {
            /* lock the notifications mutex so we can access the threads */
            LOCK(MUTEX);

            /* Iterate through notifications threads to find a match */
            for(uint16_t nIndex = 0; nIndex < NOTIFICATIONS_THREADS.size(); ++nIndex)
            {
                /* If the thread is configured for this session then return it */
                if(NOTIFICATIONS_THREADS[nIndex]->Has(nSession))
                    return NOTIFICATIONS_THREADS[nIndex];
            }

            /* Not found so return null */
            return nullptr;
        }
    }
}
