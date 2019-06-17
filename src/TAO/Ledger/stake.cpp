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
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/transaction.h>

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
            if(config::fTestNet.load())
                return TAO::Ledger::TESTNET_MINIMUM_INTERVAL;

            else if(TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION < 7)
                return TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY;

            else if(runtime::timestamp() > TAO::Ledger::NETWORK_VERSION_TIMELOCK[5])  //timelock activation for Tritium interval
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
        uint64_t GetTrustScore(const uint64_t nTrustPrev, const uint64_t nStake, const uint64_t nBlockAge)
        {
            uint64_t nTrust = 0;
            uint64_t nTrustMax = MaxTrustScore();
            uint64_t nBlockAgeMax = MaxBlockAge();

            /* Block age less than maximum awards trust score increase equal to the current block age. */
            if(nBlockAge <= nBlockAgeMax)
            {
                nTrust = std::min((nTrustPrev + nBlockAge), nTrustMax);
            }
            else
            {
                /* Block age more than maximum allowed is penalized 3 times the time it has exceeded the maximum. */

                /* Calculate the penalty for score (3x the time). */
                uint64_t nPenalty = (nBlockAge - nBlockAgeMax) * (uint64_t)3;

                /* Trust back to zero if penalty more than previous score. */
                if(nPenalty < nTrustPrev)
                    nTrust = nTrustPrev - nPenalty;
                else
                    nTrust = 0;
            }

            /* Double check that the trust score cannot exceed the maximum */
            if(nTrust > nTrustMax)
                nTrust = nTrustMax;

            return nTrust;
        }


        /* Calculate trust score penalty that results from unstaking a portion of stake balance. */
        uint64_t GetUnstakePenalty(const uint64_t nTrustPrev, const uint64_t nStakePrev,
                                   const uint64_t nStakeNew, const uint256_t& hashGenesis)
        {
            /* Unstake penalty only applies if stake balance is reduced */
            if(nStakeNew >= nStakePrev)
                return 0;

            /* Cutoff time of grace period. Stake added within grace period can be removed without penalty */
            uint64_t nCutoff = runtime::unifiedtimestamp() - (uint64_t)(config::fTestNet.load() ? STAKE_GRACE_PERIOD_TESTNET : STAKE_GRACE_PERIOD);

            /* Look through tx history to calculate any amount of stake added within the grace period */
            uint512_t hashLast = 0;
            int64_t nStakeAdded = 0;

            /* Get the most recent tx hash for the user account. */
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Loop through all transactions within grace period and accumulate net amount of stake added. */
                while(hashLast != 0)
                {
                    /* Get the transaction for the current hashLast. */
                    TAO::Ledger::Transaction txCheck;
                    if(!LLD::Ledger->ReadTx(hashLast, txCheck, TAO::Ledger::FLAGS::MEMPOOL))
                        break;

                    /* Transaction must be after cutoff to be within the grace period */
                    if(txCheck.nTimestamp < nCutoff)
                        break;

                    /* Test whether the transaction contains a staking operation */
                    for(uint32_t i = 0; i < txCheck.Size(); ++i)
                    {
                        if(TAO::Register::Unpack(txCheck[i], TAO::Operation::OP::STAKE))
                        {
                            /* Unpack the amount added to stake by stake operation */
                            uint64_t nAmount;
                            TAO::Register::Unpack(txCheck[i], nAmount);

                            nStakeAdded += nAmount;
                        }

                        /* Also have to account for stake removed during grace period as this negates the amount added */
                        if(TAO::Register::Unpack(txCheck[i], TAO::Operation::OP::UNSTAKE))
                        {
                            /* Unpack the amount added to stake by stake operation */
                            uint64_t nAmount;
                            TAO::Register::Unpack(txCheck[i], nAmount);

                            nStakeAdded -= nAmount;
                        }
                    }

                    /* Iterate to next previous user tx */
                    hashLast = txCheck.hashPrevTx;
                }
            }

            /* If net change during grace period is unstake, then amount added is 0 */
            if(nStakeAdded < 0)
                nStakeAdded = 0;

            /* Unstake penalty only applies if stake balance reduced by more than amount added during grace period */
            if((nStakeNew + nStakeAdded) >= nStakePrev)
                return 0;

            /* When unstake, new trust score is fraction of old trust score equal to fraction of balance remaining.
             * (nStakeNew / nStakePrev) is fraction of balance remaining, multiply by old trust score to get new one.
             * In other words, (nStakeNew / nStakePrev) * old trust score = new trust score
             *
             * In implementation, multiplication is done first to allow this to use integer math.
             *
             * If have stake added during grace period, this amount is added to nStakeNew, reducing the trust penalty.
             *
             * Example 1: have 100 stake and remove 30, new stake is 70 and new trust score is (70 / 100) * old trust score
             * New score is 70% old score, a 30% penalty.
             *
             * Example 2: have 100 stake and add 100 more (200 stake), then remove 150 within grace period, new stake is 50.
             * If penalty applied to full amount removed, new score = (50 / 200) * old trust score, a 75% penalty
             * but because it was removed during grace period, new score = ((50 + 100) / 200) * old trust score, a 25% penalty
             *
             * Note that, in example 2, if only remove 50, then (nStakeNew + nStakeAdded) = (150 + 100) > current 200 stake
             * so the if-check above is true and penalty is 0 because have only removed a portion of the stake added
             * during the grace period.
             */
            uint64_t nTrustNew = ((nStakeNew + (uint64_t)nStakeAdded) * nTrustPrev) / nStakePrev;

            /* Penalty is amount of trust reduction */
            return (nTrustPrev - nTrustNew);
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
        double GetCurrentThreshold(const uint64_t nBlockTime, const uint64_t nNonce)
        {
            return (nBlockTime * 100.0) / nNonce;
        }


        /* Calculate the minimum Required Energy Efficiency Threshold. */
        double GetRequiredThreshold(double nTrustWeight, double nBlockWeight, uint64_t nStake)
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
            if(isGenesis)
                return 0.005;

            /* Stake rate starts at 0.005 (0.5%) and grows to 0.03 (3%) when trust score reaches maximum */
            double nTrustScoreRatio = (double)nTrust / (double)MaxTrustScore();

            return std::min(0.03, (0.025 * log((9.0 * nTrustScoreRatio) + 1.0) / LOG10) + 0.005);
        }


        /* Calculate the coinstake reward for a given stake. */
        uint64_t GetCoinstakeReward(const uint64_t nStake, const uint64_t nStakeTime, const uint64_t nTrust, const bool isGenesis)
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
