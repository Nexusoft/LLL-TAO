/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/stake_manager.h>

#include <TAO/API/include/session.h>
#include <TAO/API/include/sessionmanager.h>

#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>


namespace TAO
{
    namespace Ledger
    {
        /* Retrieves the singleton instance of the stake manager. */
        StakeManager& StakeManager::GetInstance()
        {
            static StakeManager STAKE_MANAGER(config::GetArg("-stakingthreads", 1));

            return STAKE_MANAGER;
        }


        /* Constructor. */
        StakeManager::StakeManager(const uint16_t& nThreads)
        : fPooled(false)
        , MAX_THREADS(nThreads)
        , THREADS_MUTEX()
        , STAKING_THREADS()
        {
            if(config::fPoolStaking.load())
                fPooled = true;
        }


        /* Destructor. */
        StakeManager::~StakeManager()
        {
            /* lock the  mutex so we can access the threads */
            LOCK(THREADS_MUTEX);

            /* Delete the staking threads (shuts down all staking minters and ends threads). */
            for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
            {
                delete STAKING_THREADS[nIndex];
                STAKING_THREADS[nIndex] = nullptr;
            }

            STAKING_THREADS.clear();
        }


        /* Starts a stake minter for the provided session, if one is not already running. */
        bool StakeManager::Start(uint256_t& nSession)
        {
            /* Pooled staking activates with block version 9 */
            if(fPooled && runtime::unifiedtimestamp() < TAO::Ledger::StartBlockTimelock(9))
            {
                debug::error(FUNCTION, "Pooled staking not yet available");

                return false;
            }

            /* Disable stake minter in lite mode. */
            if(config::fClient.load())
            {
                debug::log(0, FUNCTION, "Stake minter disabled in lite mode.");
                return false;
            }

            /* Check that stake minter is configured to run. */
            if(!config::fStaking.load())
            {
                debug::log(0, "Stake Minter not configured. Start cancelled.");
                return false;
            }

            /* Stake minter does not run in private or hybrid mode (at least for now) */
            if(config::GetBoolArg("-private") || config::GetBoolArg("-hybrid"))
            {
                debug::log(0, "Stake Minter does not run in private/hybrid mode. Start cancelled.");
                return false;
            }

            const TAO::API::Session& session = TAO::API::GetSessionManager().Get(nSession, false);

            /* Verify that account is unlocked and can mint. */
            if(session.Locked() || !session.CanStake())
            {
                debug::log(0, FUNCTION, "User account not unlocked for staking. Stake minter not started.");
                return false;
            }

            /* The staking thread where stake minter will be added */
            TAO::Ledger::StakeThread* pThread = nullptr;

            {
                /* Lock the mutex to process the threads vector */
                LOCK(THREADS_MUTEX);

                /* Verify no stake minter running for given session */
                for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
                {
                    TAO::Ledger::StakeThread* pThread = STAKING_THREADS[nIndex];

                    /* If the thread is configured for this session, stake minter is already running */
                    if(pThread->Has(nSession))
                    {
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Stake minter aready running for session");

                        return true;
                    }
                }

                /* Start a new staking thread if have not reached maximum amount */
                if(STAKING_THREADS.size() < MAX_THREADS)
                {
                    pThread = new TAO::Ledger::StakeThread(fPooled);
                    STAKING_THREADS.push_back(pThread);
                }

                /* Find the thread managing the least number of staking sessions */
                else
                {
                    uint32_t nLeast = std::numeric_limits<uint32_t>::max();

                    for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
                    {
                        TAO::Ledger::StakeThread* pCurrentThread = STAKING_THREADS[nIndex];

                        /* Get the number of stake minters running on the thread */
                        const uint32_t nSize = pCurrentThread->Size();

                        /* Find least loaded thread */
                        if(nSize < nLeast)
                        {
                            pThread = pCurrentThread;
                            nLeast = nSize;
                        }
                    }
                }
            } //end lock scope

            if(pThread)
            {
                /* Add a stake minter for the session to the staking thread. */
                pThread->Add(nSession);
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake minter started");

                return true;
            }

            return false;
        }


        /* Stops the stake minter for a session. */
        bool StakeManager::Stop(const uint256_t& nSession)
        {
            /* Lock the mutex to process the threads vector */
            LOCK(THREADS_MUTEX);

            /* Iterate through staking threads to find the one containing the requested session */
             for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
            {
                TAO::Ledger::StakeThread* pThread = STAKING_THREADS[nIndex];

                /* If the thread is running with this session then remove it */
                if(pThread->Has(nSession))
                {
                    pThread->Remove(nSession);
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Stake minter stopped");

                    /* Stop the thread if no remaining stake minters */
                    if(pThread->Size() == 0)
                    {
                        pThread->ShutdownThread();
                        STAKING_THREADS.erase(STAKING_THREADS.begin() + nIndex);
                        delete pThread;
                    }

                    return true;
                }
            }

            return false;
        }


        /* Stop and removes all running stake minters. */
        void StakeManager::StopAll()
        {
            /* Vector to hold session Id list */
            std::vector<uint256_t> vSessions;

            /* Lock the mutex to process the threads vector */
            LOCK(THREADS_MUTEX);

            /* Iterate through staking threads to find the one containing the requested session */
            for(auto& pThread : STAKING_THREADS)
            {
                if(pThread->Size() > 0)
                {
                    /* Get and remove all sessions from the thread */
                    pThread->GetSessions(vSessions);

                    for(const auto& nSession : vSessions)
                    {
                        pThread->Remove(nSession);
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Stake minter stopped");
                    }

                    /* Shutdown the thread */
                    pThread->ShutdownThread();
                    delete pThread;
                    pThread = nullptr;
                }
            }

            STAKING_THREADS.clear();

            return;
        }


        /* Tests whether or not a stake minter is running for a given session. */
        bool StakeManager::IsStaking(const uint256_t& nSession) const
        {
            /* lock the threads mutex so we can access the staking threads */
            LOCK(THREADS_MUTEX);

            /* Iterate through staking threads to find a match */
            for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
            {
                TAO::Ledger::StakeThread* pThread = STAKING_THREADS[nIndex];

                /* If the thread is configured for this session then return it */
                if(pThread->Has(nSession))
                    return true;
            }

            /* Not found */
            return false;
        }


        /* Tests whether or not the stake manager is using pooled staking or solo staking. */
        bool StakeManager::IsPoolStaking() const
        {
            return fPooled;
        }


        /* Returns a count of the number of stake minters currently staking. */
        uint32_t StakeManager::GetActiveCount() const
        {
            /* lock the threads mutex so we can access the staking threads */
            LOCK(THREADS_MUTEX);

            uint32_t nCount = 0;

            /* Add up the count of minters in each staking thread */
            for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
                nCount += STAKING_THREADS[nIndex]->Size();

            return nCount;
        }


        /* Finds the stake minter for a given session ID */
        TAO::Ledger::StakeMinter* StakeManager::FindStakeMinter(const uint256_t& nSession) const
        {
            /* lock the threads mutex so we can access the staking threads */
            LOCK(THREADS_MUTEX);

            /* Iterate through staking threads to find a match */
            for(uint16_t nIndex = 0; nIndex < STAKING_THREADS.size(); ++nIndex)
            {
                TAO::Ledger::StakeThread* pThread = STAKING_THREADS[nIndex];

                /* If the thread is configured for this session then return it */
                if(pThread->Has(nSession))
                    return pThread->GetStakeMinter(nSession);
            }

            /* Not found */
            return nullptr;
        }
    }
}
