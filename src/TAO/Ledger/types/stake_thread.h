/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_STAKE_THREAD_H
#define NEXUS_TAO_LEDGER_TYPES_STAKE_THREAD_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>

#include <Util/include/mutex.h>

#include <atomic>
#include <map>
#include <thread>
#include <vector>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /* forward declarations */
    class StakeMinter;

    /** @class StakeThread
     *
     *  Operates the stake minter for one or more staking sessions.
     *
     *  Calls to execute the setup and hashing phases for all added minters are interwoven on the thread
     *  such that the minters may stake simultaneously.
     *
     **/
    class StakeThread
    {

    public:

        /** Constructor.
         *
         *  @param[in] fPooled Indicates whether or not pooled staking is active.
         *
         **/
        StakeThread(const bool fPooled = false);


        /** Destructor. **/
        ~StakeThread();


        /** Add
         *
         *  Adds a session to this thread for staking. This starts the stake minter for that session.
         *
         *  @param[in] nSession The session ID of the staking session
         *
         *  @return true if the session was added and stake minter started successfully
         *
         **/
        bool Add(uint256_t& nSession);


        /** Remove
         *
         *  Removes a session if it is staking in this tread. This destroys the stake minter for that session.
         *
         *  Use Has() if need to check whether or not the session is in this thread.
         *
         *  @param[in] nSession The session ID to remove
         *
         *  @return true if the session was removed
         *
         **/
        bool Remove(const uint256_t& nSession);


        /** Has
         *
         *  Checks to see if session ID is staking in this thread.
         *
         *  @param[in] nSession The session ID to search for
         *
         *  @return true if this thread is staking for the requested session
         *
         **/
        bool Has(const uint256_t& nSession) const;


        /** Size
         *
         *  Returns the number of stake minters managed by this thread.
         *
         *  @return the current stake minter count
         *
         **/
        uint32_t Size() const;


        /** GetSessions
         *
         *  Retrieve a list of all sessions with a stake minter running in this thread.
         *
         *  @param[out] vSessions session Id list
         *
         **/
        void GetSessions(std::vector<uint256_t>& vSessions) const;


        /** GetStakeMinter
         *
         *  Retrieves the stake minter for a given session.
         *
         *  @param[in] nSession The session ID to search for
         *
         *  @return pointer to the stake minter for the requested session, nullptr if stake minter not running in this thread
         *
         **/
        TAO::Ledger::StakeMinter* GetStakeMinter(const uint256_t& nSession);


        /** ShutdownThread
         *
         *  Shuts down an empty thread.
         *
         *  This method will not shut down if there are one or more running stake minters, Size() > 0
         *
         *  @return true if the thread was shut down, false otherwise
         *
         **/
        bool ShutdownThread();


    private:

        /** Sessions and their corresponding stake minter that are processed by this thread.
         *  The StakeMinter is non-copyable, so a reference wrapper is used to store a reference to it.
         **/
        std::map<uint256_t, TAO::Ledger::StakeMinter*> mapStakeMinter;


        /** the shutdown flag for gracefully shutting down events thread. **/
        std::atomic<bool> fShutdown;


        /** Indicates whether or not the thread is using pooled staking. **/
        bool fPooled;


        /** Hash of the best chain when the staking thread begins a new staking round.
         *  If current hashBestChain changes, the thread must start all stake minters over with a new setup phase.
         **/
        uint1024_t hashLastBlock;


        /** Mutex for accessing stake minter map **/
        mutable std::mutex THREAD_MUTEX;


        /** The staking thread. **/
        std::thread STAKING_THREAD;


        /** StakingThread
         *
         *  Background thread to perform staking on all added stake minters.
         *
         **/
        void StakingThread();

    };

    }
}

#endif
