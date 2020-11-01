/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/stake_thread.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Ledger/types/stake_minter_solo.h>
#include <TAO/Ledger/types/stake_minter_pooled.h>
#include <TAO/Ledger/types/stakepool.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>


namespace TAO
{
    namespace Ledger
    {
        /* Constructor */
        StakeThread::StakeThread(const bool fPooled)
        : mapStakeMinter()
        , fShutdown(false)
        , fPooled(fPooled)
        , THREAD_MUTEX()
        , STAKING_THREAD()
        {
            STAKING_THREAD = std::thread(&TAO::Ledger::StakeThread::StakingThread, this);
        }


        /* Destructor */
        StakeThread::~StakeThread()
        {
            /* Set the shutdown flag */
            fShutdown.store(true);

            /* Wait for staking thread to end if it has not already been joined via call to ShutdownThread */
            if(STAKING_THREAD.joinable())
                STAKING_THREAD.join();

            /* Remove any stake minters that are still running in this thread */
            for(uint16_t nIndex = 0; nIndex < mapStakeMinter.size(); ++nIndex)
            {
                delete mapStakeMinter[nIndex];
                mapStakeMinter[nIndex] = nullptr;
            }

            mapStakeMinter.clear();
        }


        /* Adds a session to this thread for staking */
        bool StakeThread::Add(uint256_t& nSession)
        {
            /* Pooled staking activates with block version 9 */
            if(fPooled && runtime::unifiedtimestamp() < TAO::Ledger::StartBlockTimelock(9))
            {
                debug::error(FUNCTION, "Pooled staking not yet available");
                return false;
            }

            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            /* Add a stake minter for the requested session if we don't already have one */
            if(mapStakeMinter.find(nSession) == mapStakeMinter.end())
            {
                TAO::Ledger::StakeMinter* pStakeMinter = nullptr;

                if(fPooled)
                {
                    /* Create a pooled stake minter for the requested session and store in map for this thread */
                    pStakeMinter = new TAO::Ledger::StakeMinterPooled(nSession);
                }

                else
                {
                    /* Create a pooled stake minter for the requested session and store in map for this thread */
                    pStakeMinter = new TAO::Ledger::StakeMinterSolo(nSession);
                }

                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake Minter added to staking thread");

                mapStakeMinter[nSession] = pStakeMinter;
            }

            return true;
        }


        /* Removes a session if it is staking in this tread */
        bool StakeThread::Remove(const uint256_t& nSession)
        {
            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            /* Remove the session from the staking thread and ends the stake minter. */
            if(mapStakeMinter.find(nSession) != mapStakeMinter.end())
            {
                TAO::Ledger::StakeMinter* pStakeMinter = mapStakeMinter[nSession];
                mapStakeMinter.erase(nSession);

                delete pStakeMinter;
                pStakeMinter = nullptr;

                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake Minter removed from staking thread");

                return true;
            }

            return false;
        }


        /* Checks to see if session ID is staking in this thread */
        bool StakeThread::Has(const uint256_t& nSession) const
        {
            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            /* Check if the map contains a stake minter for the session id */
            return mapStakeMinter.find(nSession) != mapStakeMinter.end();
        }


        /* Returns the number of stake minters managed by this thread */
        uint32_t StakeThread::Size() const
        {
            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            return mapStakeMinter.size();
        }


        /* Retrieve a list of all sessions with a stake minter running in this thread */
        void StakeThread::GetSessions(std::vector<uint256_t>& vSessions) const
        {
            vSessions.clear();

            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            for(const auto& item : mapStakeMinter)
                vSessions.push_back(item.first);

            return;
        }


        /* Retrieves the stake minter for a given session */
       TAO::Ledger::StakeMinter* StakeThread::GetStakeMinter(const uint256_t& nSession)
        {
            /* lock the thread mutex so we can access the sessions */
            LOCK(THREAD_MUTEX);

            if(mapStakeMinter.find(nSession) != mapStakeMinter.end())
                return mapStakeMinter[nSession];

            return nullptr;
        }


        /* Shuts down an empty thread. */
        bool StakeThread::ShutdownThread()
        {
            uint32_t nSize = 0;

            /* Check if thread has already been shut down */
            if(!STAKING_THREAD.joinable())
                return false;

            {
                /* lock the thread mutex so we can access the sessions */
                LOCK(THREAD_MUTEX);

                nSize = mapStakeMinter.size();
            }

            if(nSize == 0)
            {
                /* Set the shutdown flag */
                fShutdown.store(true);

                /* Wait for staking thread to end */
                STAKING_THREAD.join();

                debug::log(0, FUNCTION, "Staking thread shut down");
                return true;
            }

            return false;
        }


        /*  Background thread to perform staking on all added stake minters */
        void StakeThread::StakingThread()
        {
            bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

            /* If the system is still syncing/connecting when thread starts, wait to process staking.
             * Local testnet does not wait for connections.
             */
            while(!fShutdown.load()
            && (TAO::Ledger::ChainState::Synchronizing() || (!fLocalTestnet && LLP::TRITIUM_SERVER->GetConnectionCount() == 0)))
                runtime::sleep(5000);

            /* Stake thread will likely start in the middle of a block round. Wait until round ends before continuing */
            hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

            while(!fShutdown.load() && !fLocalTestnet && (hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load()))
                runtime::sleep(500);


            /* Staking thread will continue repeating this loop until thread is shut down (destructor) */
            while(!fShutdown.load())
            {
                /* Save the best block hash for the current iteration. */
                hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

                debug::log(2, FUNCTION, "Stake thread starting new minting round for stake minters");

                /* Cannot lock mutex for entire staking phase to iterate the minter map or it would
                 * would block if attempting Add, Remove, Has, etc. until the current phase completes.
                 *
                 * Instead, create a working copy of map at start of each minting round and work from it.
                 * Any minter added in the middle of a round will begin staking at start of next setup phase (next block).
                 */
                std::map<uint256_t, TAO::Ledger::StakeMinter*> mapWorking;

                {
                    /* lock the thread mutex so we can access the sessions */
                    LOCK(THREAD_MUTEX);

                    mapWorking = mapStakeMinter;
                }

                if(fPooled)
                {
                    /* Lock staking mutex to access stakepool */
                    LOCK(TAO::Ledger::STAKING_MUTEX);

                    /* Reset the pool for new block. Only needs to be done by one thread per round, so check first */
                    if(!TAO::Ledger::stakepool.IsCleared() && TAO::Ledger::stakepool.GetHashLastBlock() != hashLastBlock)
                        TAO::Ledger::stakepool.Clear();
                }

                /* SETUP PHASE - loop through stake minters and have each set up a new block  */
                bool fSetup = false; //indicates at least one stake minter has completed setup and requires hashing phase

                debug::log(2, FUNCTION, "Performing setup phase for ", mapWorking.size(),
                           " stake minter(s) for block ", hashLastBlock.SubString());

                for(const auto& item : mapWorking)
                {
                    const uint256_t& nSession = item.first;
                    TAO::Ledger::StakeMinter* pStakeMinter = item.second;

                    /* Verify minter not removed since working copy created */
                    if(!Has(nSession))
                        continue;

                    /* Check that user account still unlocked for minting (locking account should remove, but still verify) */
                    if(!pStakeMinter->CheckUser())
                    {
                        /* If fail user check, remove current stake minter from thread */
                        Remove(nSession);
                        continue;
                    }

                    /* Retrieve the latest trust account data */
                    if(!pStakeMinter->FindTrustAccount())
                    {
                        /* If fail retrieving trust account, remove current stake minter from thread */
                        Remove(nSession);
                        continue;
                    }

                    /* Set up the candidate block the minter is attempting to mine */
                    if(!pStakeMinter->CreateCandidateBlock())
                        continue;

                    /* Update weights for new candidate block */
                    if(!pStakeMinter->CalculateWeights())
                        continue;

                    /* Set the last block for the current minting round */
                    pStakeMinter->SetLastBlock(hashLastBlock);

                    fSetup = true;
                }

                /* HASHING PHASE - loop through stake minters and have each hash the new block.
                 * This phase repeats as long as best block does not change.
                 */
                bool fHash = false; //indicates at least one stake minter is executing hash algorithm

                debug::log(2, FUNCTION, "Begin hashing phase for ", mapWorking.size(),
                           " stake minter(s) for block ", hashLastBlock.SubString());

                while(!fShutdown.load() && fSetup && (hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load()))
                {
                    /* Reset hash flag for each iteration. If no minters hash, loop will sleep) */
                    fHash = false;

                    for(const auto& item : mapWorking)
                    {
                        const uint256_t& nSession = item.first;
                        TAO::Ledger::StakeMinter* pStakeMinter = item.second;

                        /* Verify minter not removed since working copy created */
                        if(!Has(nSession))
                            continue;

                        /* Attempt to mine the current proof of stake block. Stake minter must return control
                         * when it reaches a hashing threshold so thread can process the next minter.
                         */
                        if(pStakeMinter->MintBlock())
                            fHash = true; //minter performed hashing

                        if(fShutdown.load())
                            break;
                    }

                    if(!fShutdown.load())
                    {
                        /* When no minters are hashing, this loop needs to wait for the next block before
                         * returning to setup phase, and should sleep here when in that state.
                         * (Example: all minters are waiting for minimum stake interval to pass, so none are hashing)
                         */
                        if(!fHash)
                            runtime::sleep(1000);

                        /* When one or more minters are hashing, still include a short sleep so they can advance their thresholds */
                        else
                            runtime::sleep(100);
                    }
                }

                /* Sleep between minting rounds, but only if didn't already do a full second sleep in hashing phase.
                 * This is especially important if all minters in the thread are removed or in a wait phase (fSetup == false).
                 * Without this sleep, thread could cycle continually using high CPU resources while doing nothing.
                 */
                if(!fShutdown.load() && !fHash)
                    runtime::sleep(1000);
            }

            /* Thread shutdown has been issued. End the thread. */
        }

    }
}
