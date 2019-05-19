/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/stake.h>

#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/unpack.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Constants for use in staking calculations (move to TAO/Ledger/include/constants.h ?) */
        const double LOG3 = log(3); 
        const double LOG10 = log(10); 


        /* Retrieve the setting for maximum block age (time since last stake before trust decay begins. */
        uint64_t MaxBlockAge()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN);
        }


        /* Retrieve the setting for maximum trust score value allowed. */
        uint64_t MaxTrustScore()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::TRUST_SCORE_MAX_TESTNET : TAO::Ledger::TRUST_SCORE_MAX);
        }


        /* Retrieve the setting for minimum coin age required to begin staking Genesis.*/
        uint64_t MinCoinAge()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::MINIMUM_GENESIS_COIN_AGE_TESTNET : TAO::Ledger::MINIMUM_GENESIS_COIN_AGE);
        }


        /* Retrieve the minimum number of blocks required between an account's stake transactions.*/
        uint32_t MinStakeInterval()
        {
            if (config::fTestNet.load())
                return TAO::Ledger::TESTNET_MINIMUM_INTERVAL;

            else if (TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION < 7)
                return TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY;

            else if (runtime::timestamp() > TAO::Ledger::NETWORK_VERSION_TIMELOCK[5])  //timelock activation for Tritium interval
                return TAO::Ledger::MAINNET_MINIMUM_INTERVAL;

            else
                return TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY;
        }


        /* Retrieve the base trust score setting for performing trust weight calculations. */
        uint64_t TrustWeightBase()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET : TAO::Ledger::TRUST_WEIGHT_BASE);
        }


        /* Calculate new trust score from parameters. */
        uint64_t TrustScore(const uint64_t nTrustPrev, const uint64_t nStake, const uint64_t nBlockAge)
        {
            uint64_t nTrust = 0;
            uint64_t nTrustMax = MaxTrustScore();
            uint64_t nBlockAgeMax = MaxBlockAge();

            /* Block age less than maximum awards trust score increase equal to the current block age. */
            if (nBlockAge <= nBlockAgeMax)
            {
                nTrust = std::min((nTrustPrev + nBlockAge), nTrustMax);
            }
            else
            {
                /* Block age more than maximum allowed is penalized 3 times the time it has exceeded the maximum. */

                /* Calculate the penalty for score (3x the time). */
                uint64_t nPenalty = (nBlockAge - nBlockAgeMax) * (uint64_t)3;

                /* Trust back to zero if penalty more than previous score. */
                if (nPenalty < nTrustPrev)
                    nTrust = nTrustPrev - nPenalty;
                else
                    nTrust = 0;
            }

            /* Double check that the trust score cannot exceed the maximum */
            if (nTrust > nTrustMax)
                nTrust = nTrustMax;

            return nTrust;
        }


        /* Calculate the proof of stake block weight for a given block age. */
        double BlockWeight(const uint64_t nBlockAge)
        {

            /* Block Weight reaches maximum of 10.0 when Block Age equals the max block age */
            double nBlockAgeRatio = (double)nBlockAge / (double)MaxBlockAge();

            return std::min(10.0, (9.0 * log((2.0 * nBlockAgeRatio) + 1.0) / LOG3) + 1.0);
        }


        /* Calculate the equivalent proof of stake trust weight for staking Genesis with a given coin age. */
        double GenesisWeight(const uint64_t nCoinAge)
        {
            /* Trust Weight For Genesis is based on Coin Age. Genesis trust weight is less than normal trust weight,
             * reaching a maximum of 10.0 after average Coin Age reaches trust weight base.
             */
            double nGenesisTrustRatio = (double)nCoinAge / (double)TrustWeightBase();

            return std::min(10.0, (9.0 * log((2.0 * nGenesisTrustRatio) + 1.0) / LOG3) + 1.0);
        }


        /* Calculate the proof of stake trust weight for a given trust score. */
        double TrustWeight(const uint64_t nTrust)
        {
            /* Trust Weight base is time for 50% score. Weight continues to grow with Trust Score until it reaches max of 90.0
             * This formula will reach 45.0 (50%) after accumulating 84 days worth of Trust Score (Mainnet base), 
             * while requiring close to a year to reach maximum.
             */
            double nTrustWeightRatio = (double)nTrust / (double)TrustWeightBase();

            return std::min(90.0, (44.0 * log((2.0 * nTrustWeightRatio) + 1.0) / LOG3) + 1.0);
        }


        /* Calculate the current threshold value for Proof of Stake. */
        double CurrentThreshold(const uint64_t nBlockTime, const uint64_t nNonce)
        {
            return (nBlockTime * 100.0) / nNonce;
        }


        /* Calculate the minimum Required Energy Efficiency Threshold. */
        double RequiredThreshold(double nTrustWeight, double nBlockWeight, uint64_t nStake)
        {
            /*  Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
             *  Weight from staking balance reduces the required threshold by increasing the denominator.
             */
            return ((108.0 - nTrustWeight - nBlockWeight) * TAO::Ledger::MAX_STAKE_WEIGHT) / nStake;
        }


        /* Calculate the stake rate corresponding to a given trust score. */
        double StakeRate(const uint64_t nTrust, const bool isGenesis)
        {
            /* Stake rate fixed at 0.005 (0.5%) when staking Genesis */
            if (isGenesis)
                return 0.005;

            /* Stake rate starts at 0.005 (0.5%) and grows to 0.03 (3%) when trust score reaches maximum */
            double nTrustScoreRatio = (double)nTrust / (double)MaxTrustScore();

            return std::min(0.03, (0.025 * log((9.0 * nTrustScoreRatio) + 1.0) / LOG10) + 0.005);
        }


        /* Calculate the coinstake reward for a given stake. */
        uint64_t CoinstakeReward(const uint64_t nStake, const uint64_t nStakeTime, const uint64_t nTrust, const bool isGenesis)
        {

            double nStakeRate = StakeRate(nTrust, isGenesis);

            /* Reward rate for time period is annual rate * (time period / annual time) or nStakeRate * (nStakeTime / MaxTrustScore)
             * Then, overall nStakeReward = nStake * reward rate
             *
             * Thus, the appropriate way to write this (for clarity) would be: nStakeReward = nStake * nStakeRate * (nStakeTime / MaxTrustScore)
             * However, with integer arithmetic (nStakeTime / MaxTrustScore) would evaluate to 0 or 1, etc. and the overall nStakeReward would be erroneous
             *
             * Therefore, we apply parentheses around the full multiplication portion before applying the division to get appropriate reward. 
             */
            uint64_t nStakeReward = (nStake * nStakeRate * nStakeTime) / MaxTrustScore();

            return nStakeReward;
        }

    }
}
