/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_SUPPLY_H
#define NEXUS_TAO_LEDGER_INCLUDE_SUPPLY_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        class BlockState;

        /** These values reflect the Three Decay Equations.
         *
         *     50 * e ^ (-0.0000011  * nMinutes) + 1.0
         *     10 * e ^ (-0.00000055 * nMinutes) + 1.0
         *      1 * e ^ (-0.00000059 * nMinutes) + 0.032
         *
         */
        const double decay[3][3] =
            {
                {50.0, -0.0000011, 1.0},
                {10.0, -0.00000055, 1.0},
                {1.0, -0.00000059, 0.032}
            };


        /** GetSubsidy
         *
         *  Get the Total Amount to be Released at a given Minute since the NETWORK_TIMELOCK.
         *
         *  @param[in] nMinutes The minutes to calculate for.
         *  @param[in] nType The subsidy to calculate for.
         *
         *  @return The total reward for interval.
         *
         **/
        uint64_t GetSubsidy(const uint32_t nMinutes, const uint32_t nType);


        /** SubsidyInterval
         *
         *  Calculate the Compounded amount of NXS to be released over the (nInterval) minutes.
         *
         *  @param[in] nMinutes The minutes to calculate for.
         *  @param[in] nInterval The interval to calculate in.
         *
         *  @return The subisdy for given interval.
         *
         **/
        uint64_t SubsidyInterval(const uint32_t nMinutes, const uint32_t nInterval);


        /** CompoundSubsidy
         *
         *  Calculate the Compounded amount of NXS that should "ideally" have been created to this minute.
         *
         *  @param[in] nMinutes The minutes to calculate for.
         *  @param[in] nTypes The subsidy to calculate for.
         *
         *  @return The total reward compounded over minutes.
         *
         **/
        uint64_t CompoundSubsidy(const int32_t nMinutes, const uint8_t nTypes = 3);


        /** GetMoneySupply
         *
         *  Get the total supply of NXS in the chain from the state.
         *
         *  @param[in] nMinutes The minutes to calculate for.
         *  @param[in] nType The subsidy to calculate for.
         *
         *  @return The total reward compounded over minutes.
         *
         **/
        uint64_t GetMoneySupply(const BlockState& state);


        /** GetChainAge
         *
         *  Get the age of the Nexus blockchain in seconds.
         *
         *  @param[in] nTime The timestamp to check from
         *
         *  @return The seconds since network time-lock
         *
         **/
        uint32_t GetChainAge(const uint64_t nTime);


        /** GetFractionalSubsidy
         *
         *  Get a fractional reward based on time.
         *
         *  @param[in] nMinutes The minutes to check from
         *  @param[in] nType The coinbase type to get.
         *  @param[in] nFraction The fraction to get subsidy for
         *
         *  @return The subsidy reward at a fractional amount.
         *
         **/
        uint64_t GetFractionalSubsidy(const uint32_t nMinutes, const uint8_t nType, const double nFraction);


        /** GetCoinbaseReward
         *
         *  Get the Coinbase Rewards based on the Reserve Balances to keep the Coinbase
         *  rewards under the Reserve Production Rates.
         *
         *  @param[in] state The block state object to search from.
         *  @param[in] nChannel The channel to get reward for.
         *  @param[in] nType The coinbase output type.
         *
         *  @return The subsidy reward.
         *
         **/
        uint64_t GetCoinbaseReward(const BlockState& state, const uint32_t nChannel, const uint8_t nType);


        /** ReleaseRewards
         *
         *  Release a certain amount of Nexus into the Reserve System at a given Minute of time.
         *
         *  @param[in] nTimespan The timespan to check release from.
         *  @param[in] nStart The starting minutes to start from.
         *  @param[in] nType The coinbase output type.
         *
         *  @return The total released to reserves.
         *
         **/
        uint64_t ReleaseRewards(const uint32_t nTimespan, const uint32_t nStart, const uint8_t nType);


        /** GetReleasedReserve
         *
         *  Get the total amount released into this given reserve by this point in time in the block state
         *
         *  @param[in] state The block state object to search from.
         *  @param[in] nChannel The channel to get reward for.
         *  @param[in] nType The coinbase output type.
         *
         *  @return The released reserves.
         *
         **/
        uint64_t GetReleasedReserve(const BlockState& state, const uint32_t nChannel, const uint8_t nType);
    }
}

#endif
