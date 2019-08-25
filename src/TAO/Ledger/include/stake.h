/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STAKE_H
#define NEXUS_TAO_LEDGER_INCLUDE_STAKE_H

#include <LLC/types/uint1024.h>


/**
 *  The functions defined here provide a single source for settings and calculations related to Nexus Proof of Stake.
 *
 *  Settings-related functions replace direct references to defined constants in the code, allowing for
 *  activation-triggered changes to be coded in one place rather than in multiple places throughout the code.
 *  Any other changes, such as alternative forms of definition, would also be encapsulated within
 *  these functions and not require any change elsewhere in the code.
 *
 *  Similarly, functions that perform calculations isolate the definition of these calculations into a
 *  single location, supporting use in multiple places while only having the calculation itself
 *  coded once.
 *
 **/

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** MaxBlockAge
         *
         *  Retrieve the setting for maximum block age (time since last stake block mined)
         *  before trust decay begins.
         *
         *  @return the current system setting for maximum block age
         *
         **/
        uint64_t MaxBlockAge();


        /** MinCoinAge
         *
         *  Retrieve the setting for minimum coin age required to begin staking Genesis.
         *
         *  @return the current system setting for minimum coin age
         *
         **/
        uint64_t MinCoinAge();


        /** MinStakeInterval
         *
         *  Retrieve the minimum number of blocks required between an account's stake transactions.
         *
         *  @return the current system setting for minimum stake interval
         *
         **/
        uint32_t MinStakeInterval();


        /** TrustWeightBase
         *
         *  Retrieve the base time value for calculating trust weight.
         *
         *  @return the current system setting for trust weight base
         *
         **/
        uint64_t TrustWeightBase();


        /** GetTrustScore
         *
         *  Calculate new trust score from parameters.
         *
         *  @param[in] nTrustPrev - previous trust score of trust account
         *  @param[in] nStake - current stake balance
         *  @param[in] nBlockAge - current block age (time since last stake block for trust account)
         *
         *  @return new value for trust score
         *
         **/
        uint64_t GetTrustScore(const uint64_t nTrustPrev, const uint64_t nStake, const uint64_t nBlockAge);


        /** GetUnstakePenalty
         *
         *  Calculate amount of trust score reduction that results from unstaking a portion of stake balance.
         *
         *  @param[in] nTrustPrev - previous trust score of trust account
         *  @param[in] nStakePrev - previous stake amount for trust account
         *  @param[in] nStakeNew - new stake amount for trust account
         *  @param[in] hashGenesis - user genesis of trust account owner
         *
         *  @return value trust score penalty
         *
         **/
        uint64_t GetUnstakePenalty(const uint64_t nTrustPrev, const uint64_t nStakePrev,
                                   const uint64_t nStakeNew, const uint256_t& hashGenesis);


        /** BlockWeight
         *
         *  Calculate the proof of stake block weight for a given block age.
         *
         *  @param[in] nBlockAge
         *
         *  @return value for block weight
         *
         **/
        double BlockWeight(const uint64_t nBlockAge);


        /** GenesisWeight
         *
         *  Calculate the equivalent proof of stake trust weight for staking Genesis with a given coin age.
         *
         *  @param[in] nCoinAge
         *
         *  @return value for trust weight
         *
         **/
        double GenesisWeight(const uint64_t nCoinAge);


        /** TrustWeight
         *
         *  Calculate the proof of stake trust weight for a given trust score.
         *
         *  @param[in] nTrust - Trust score
         *
         *  @return value for trust weight
         *
         **/
        double TrustWeight(const uint64_t nTrust);


        /** GetCurrentThreshold
         *
         *  Calculate the current threshold value for Proof of Stake.
         *  This value must exceed required threshold for staking to proceed.
         *
         *  @param[in] nBlockTime - Amount of time since last block generated on the network
         *  @param[in] nNonce - Nonce value for stake iteration
         *
         *  @return value for current threshold
         *
         **/
        double GetCurrentThreshold(const uint64_t nBlockTime, const uint64_t nNonce);


        /** GetRequiredThreshold
         *
         *  Calculate the minimum Required Energy Efficiency Threshold.
         *  Can only mine Proof of Stake when current threshold exceeds this value.
         *
         *  @param[in] nTrustWeight - Current trust weight
         *  @param[in] nBlockWeight - Current block weight
         *  @param[in] nStake - Current stake balance
         *
         *  @return value for minimum required threshold
         *
         **/
        double GetRequiredThreshold(const double nTrustWeight, const double nBlockWeight, const uint64_t nStake);


        /** StakeRate
         *
         *  Calculate the stake rate corresponding to a given trust score.
         *
         *  Returned value is absolute rate. To display a percent, multiply by 100.
         *
         *  @param[in] nTrust - Current trust score (ignored when fGenesis = true)
         *  @param[in] isGenesis - Set true if staking for Genesis transaction
         *
         *  @return value for stake rate
         *
         **/
        double StakeRate(const uint64_t nTrust, const bool isGenesis = false);


        /** GetCoinstakeReward
         *
         *  Calculate the coinstake reward for a given stake.
         *
         *  @param[in] nStake - Stake balance on which reward is to be paid
         *  @param[in] nStakeTime - Amount of time reward is for
         *  @param[in] nTrust - Current trust score (ignored when fGenesis = true)
         *  @param[in] isGenesis - Set true if staking for Genesis transaction
         *
         *  @return amount of coinstake reward
         *
         **/
        uint64_t GetCoinstakeReward(const uint64_t nStake, const uint64_t nStakeTime,
                                    const uint64_t nTrust, const bool isGenesis = false);

    }
}

#endif
