/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_BASE_MINTER_H
#define NEXUS_TAO_LEDGER_TYPES_BASE_MINTER_H


#include <LLC/types/uint1024.h>

#include <atomic>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /** @class StakeMinter
      *
      * This class provides the base class and process metrics for mining blocks on the Proof of Stake channel.
      * Implementations will provide the details for mining each specific PoS block type. 
      *
      * Staking starts when StartStakeMinter() is called.
      *
      * Staking operations can be suspended by calling StopStakeMinter and restarted by calling StartStakeMinter() again.
      *
      **/
    class StakeMinter
    {
    public:

        /** Destructor **/
        virtual ~StakeMinter() {}


        /** IsStarted
          *
          * Tests whether or not the stake minter is currently running.
          *
          * @return true if the stake minter is started, false otherwise
          *
          */
        virtual bool IsStarted() const = 0;


        /** GetBlockWeight
          *
          * Retrieves the current internal value for the block weight metric.
          *
          * @return value of current block weight
          *
          */
        double GetBlockWeight() const;


        /** GetBlockWeightPercent
          *
          * Retrieves the block weight metric as a percentage of maximum.
          *
          * @return the current block weight percentage
          *
          */
        double GetBlockWeightPercent() const;
 

        /** GetTrustWeight
          *
          * Retrieves the current internal value for the trust weight metric.
          *
          * @return value of current trust weight
          *
          */
        double GetTrustWeight() const;


        /** GetTrustWeightPercent
          *
          * Retrieves the trust weight metric as a percentage of maximum.
          *
          * @return the current trust weight percentage
          *
          */
        double GetTrustWeightPercent() const;


        /** GetStakeRate
          *
          * Retrieves the current staking reward rate.
          *
          * @return the current stake rate
          *
          */
        double GetStakeRate() const;


        /** GetStakeRatePercent
          *
          * Retrieves the current staking reward rate as an annual percentage.
          *
          * @return the current stake rate percentage
          *
          */
        double GetStakeRatePercent() const;


        /** IsWaitPeriod
          *
          * Checks whether the stake minter is waiting to begin staking.
          *
          * @return true if minter is waiting, false otherwise
          *
          */
        bool IsWaitPeriod() const;


        /** StartStakeMinter
          *
          * Starts the stake minter.
          *
          * Call this method to start the stake minter thread and begin mining Proof of Stake, or
          * to restart it after it was stopped.
          *
          * @return true if the stake minter was started, false if it was already running or not started
          *
          */
        virtual bool StartStakeMinter() = 0;


        /** StopStakeMinter
          *
          * Stops the stake minter.
          *
          * Call this method to signal the stake minter thread stop Proof of Stake mining and end.
          * It can be restarted via a subsequent call to StartStakeMinter().
          *
          * @return true if the stake minter was stopped, false if it was already stopped
          *
          */
        virtual bool StopStakeMinter() = 0;


    protected:
        /** Hash of the best chain when the minter began attempting to mine its current candidate.
         *  If current hashBestChain changes, the minter must start over with a new candidate.
         **/
        uint1024_t hashLastBlock;


        /** Time to sleep between candidate blocks. **/
        uint64_t nSleepTime;


        /** true when the minter is waiting to begin staking **/
        std::atomic<bool> fIsWaitPeriod;


        /** The current internal trust weight value **/
        std::atomic<double> nTrustWeight;


        /** The current internal block weight value **/
        std::atomic<double> nBlockWeight;


        /** The current staking rate for calculating staking rewards **/
        std::atomic<double> nStakeRate;


        /** Default constructor **/
        StakeMinter()
        : hashLastBlock(0)
        , nSleepTime(1000)
        , nTrustWeight(0.0)
        , nBlockWeight(0.0)
        , nStakeRate(0.0)
        {
        }

    };

    }
}

#endif
