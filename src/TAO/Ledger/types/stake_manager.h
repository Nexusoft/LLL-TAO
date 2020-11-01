/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKE_MANAGER_H
#define NEXUS_TAO_LEDGER_TYPES_STAKE_MANAGER_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/stake_thread.h>

#include <Util/include/mutex.h>

#include <vector>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

    /** StakeManager Class
     *
     *  Manages the stake minter threads.
     *  The maximum number of threads it may start is determined by the stakingthreads parameter, defaulting to 1.
     *
     *  The StakeManager will check the setting for config::fPoolStaking to determine whether to use pooled or solo staking.
     *  All sessions starting stake minters will use the same form.
     *
     **/
    class StakeManager
    {

    public:

        /** GetInstance
         *
         * Retrieves the singleton instance of the stake manager.
         *
         * @return pointer to the StakeMinter instance
         *
         **/
        static StakeManager& GetInstance();


        /** Destructor. **/
        ~StakeManager();


        /** Start
         *
         *  Starts a stake minter for the provided session, if one is not already running.
         *
         *  @param[in] nSession the session Id to stake
         *
         *  @return true if the stake minter was successfully started (or already running), false otherwise
         **/
        bool Start(uint256_t& nSession);


        /** Stop
         *
         *  Stops and removes the stake minter for a session.
         *
         *  @param[in] nSession The session Id of the stake minter to remove
         *
         *  @return true if the stake minter was found and stopped, false otherwise
         *
         **/
        bool Stop(const uint256_t& nSession);


        /** StopAll
         *
         *  Stop and removes all running stake minters. Call during system shutdown
         *
         **/
        void StopAll();


        /** IsStaking
         *
         *  Tests whether or not a stake minter is running for a given session.
         *
         *  @param[in] nSession The session ID to search for
         *
         *  @return true if stake minter is found (running), false otherwise
         *
         **/
        bool IsStaking(const uint256_t& nSession) const;


        /** IsPoolStaking
         *
         *  Tests whether or not the stake manager is using pooled staking.
         *  All stake minters use the same form.
         *
         *  @return true if using pooled staking, false if solo staking
         *
         **/
        bool IsPoolStaking() const;


        /** GetActiveCount
         *
         *  Returns a count of the number of stake minters currently staking.
         *
         *  @return the active stake minter count
         *
         **/
        uint32_t GetActiveCount() const;


        /** FindStakeMinter
         *
         *  Finds the stake minter for a given session ID
         *
         *  @param[in] nSession The session ID to search for
         *
         *  @return pointer to stake minter instance, nullptr if stake minter not found
         *
         **/
        TAO::Ledger::StakeMinter* FindStakeMinter(const uint256_t& nSession) const;


      private:

        /** Sets whether to use pooled or solo staking **/
        bool fPooled;


        /** Maximum number of staking threads managed by the stake manager. **/
        uint16_t MAX_THREADS;


        /** Mutex for accessing threads vector. **/
        mutable std::mutex THREADS_MUTEX;


        /** Vector of running staking threads **/
        std::vector<TAO::Ledger::StakeThread*> STAKING_THREADS;


        /** Constructor.
         *
         *  When constructed, the StakeManager will record whether to use pooled or solo staking based on system configuration.
         *  All stake threads will use the same staking form.
         *
         *  @param[in] nThreads The maximum number of staking threads to operate.
         *
         **/
        StakeManager(const uint16_t& nThreads = 1);

    };

    }
}

#endif
