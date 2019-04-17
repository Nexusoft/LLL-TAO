/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_RETARGET_H
#define NEXUS_TAO_LEDGER_INCLUDE_RETARGET_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        class BlockState;


        /* Target Timespan of 300 Seconds. */
        const uint32_t MINING_TARGET_SPACING = 300;


        /* Target Timespan of 150 seconds. */
        const uint32_t STAKE_TARGET_SPACING = 150;


        /* Target Timespan of 300 Seconds. */
        const uint32_t TESTNET_MINING_TARGET_SPACING = 40;


        /* Target Timespan of 150 seconds. */
        const uint32_t TESTNET_STAKE_TARGET_SPACING = 20;


        /** GetChainTimes
         *
         *  Break the Chain Age in Minutes into Days, Hours, and Minutes.
         *
         *  @param[in] nAge The age in seconds
         *  @param[out] nDays The days from age.
         *  @param[out] nHours The hours from age.
         *  @param[out] nMinutes The minutes from age.
         *
         **/
        void GetChainTimes(uint32_t nAge, uint32_t& nDays, uint32_t& nHours, uint32_t& nMinutes);


        /** GetWeightedTimes
         *
         *  Gets a block time from a weighted average at given depth.
         *
         *  @param[in] state The block state to search from.
         *  @param[in] nDepth The depth to search back.
         *
         *  @return the new weighted time.
         *
         **/
        uint64_t GetWeightedTimes(const BlockState& state, uint32_t nDepth);


        /** GetNextTargetRequired
         *
         *  Switching function for each channel difficulty adjustments.
         *
         *  @param[in] state The block state to retarget from.
         *  @param[in] nChannel The channel to retarget for.
         *
         *  @return the new bits required.
         *
         **/
        uint32_t GetNextTargetRequired(const BlockState& state, int nChannel, bool fDebug = true);


        /** RetargetTrust
         *
         *  Retarget a trust block based on seperate specifications
         *
         *  @param[in] state The block state to retarget from.
         *
         *  @return the new bits required.
         *
         **/
        uint32_t RetargetTrust(const BlockState& state, bool fDebug = true);


        /** RetargetPrime
         *
         *  Scales the maximum increase or decrease by network difficulty.
         *
         *  @param[in] state The block state to retarget from.
         *
         *  @return the new bits required.
         *
         **/
        uint32_t RetargetPrime(const BlockState& state, bool fDebug = true);


        /** RetargetHash
         *
         *  Retarget the hashing channel by seperate specifications.
         *
         *  @param[in] state The block state to retarget from.
         *
         *  @return the new bits required.
         *
         **/
        uint32_t RetargetHash(const BlockState& state, bool fDebug = true);
    }
}

#endif
