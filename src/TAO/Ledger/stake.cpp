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
#include <TAO/Ledger/include/chainstate.h>

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

        /* Retrieve the setting for maximum block age (time since last stake before trust decay begins. */
        uint64_t MaxBlockAge()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN);
        }


        /* Retrieve the setting for minimum coin age required to begin staking Genesis.*/
        uint64_t MinCoinAge()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::MINIMUM_GENESIS_COIN_AGE_TESTNET : TAO::Ledger::MINIMUM_GENESIS_COIN_AGE);
        }


        /* Retrieve the minimum number of blocks required between an account's stake transactions.*/
        uint32_t MinStakeInterval(const Block& block)
        {
            if(config::fTestNet)
                return TESTNET_MINIMUM_INTERVAL;

            /* Apply legacy interval for all versions prior to version 7 */
            if(block.nVersion < 7)
                return MAINNET_MINIMUM_INTERVAL_LEGACY;

            return MAINNET_MINIMUM_INTERVAL;
        }


        /* Retrieve the base trust score setting for performing trust weight calculations. */
        uint64_t TrustWeightBase()
        {
            return (uint64_t)(config::fTestNet.load() ? TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET : TAO::Ledger::TRUST_WEIGHT_BASE);
        }


        /* Calculate new trust score from parameters. */
        uint64_t GetTrustScore(const uint64_t nScorePrev, const uint64_t nBlockAge,
                               const uint64_t nStake, const int64_t nStakeChange, const uint32_t nVersion)
        {
            /* Note that trust score can be affected by both trust decay and unstake penalty simultaneously.
             * Trust decay should always be applied first, as it is a linear decay from the previous trust score.
             * Any unstake penalty is only applied to what is left after trust decay is included.
             *
             * If unstake penalty were applied first, trust would fully decay in less time than it should.
             */
            uint64_t nScore = 0;
            const uint64_t nBlockAgeMax = MaxBlockAge();

            /* Block age less than maximum awards trust score increase equal to the current block age. */
            if(nBlockAge <= nBlockAgeMax)
            {
                /* Trust score grows indefinitely as long as block age less than minimum.
                 * Trust weight and stake rate cap when they reach their max values even though score keeps growing.
                 * The impact of unlimited trust score is that you can unstake a portion (with resulting penalty below) and
                 * potentially still keep trust weight and stake rate at their max values.
                 */
                nScore = nScorePrev + nBlockAge;
            }
            else
            {
                /* Block age more than maximum allowed, trust is penalized 3 times the time it has exceeded the maximum. */

                /* Calculate the penalty for score (3x the time). */
                uint64_t nPenalty = (nBlockAge - nBlockAgeMax) * (uint64_t)3;

                /* Trust back to zero if penalty more than previous score. */
                if(nPenalty < nScorePrev)
                    nScore = nScorePrev - nPenalty;
                else
                    nScore = 0;
            }

            if(nStakeChange < 0)
            {
                /* Adjust trust based on penalty for removing stake
                 *
                 * When unstake, resulting trust score is fraction of trust score equal to fraction of stake remaining.
                 * (nStakeNew / nStake) is fraction of stake remaining, multiply by trust score to get new one.
                 *
                 * In other words, (nStakeNew / nStake) * trust score = new trust score
                 *
                 * In implementation, multiplication is done first to allow this to use integer math.
                 */
                uint64_t nStakeNew = 0;
                uint64_t nUnstake = 0 - nStakeChange;

                /* Unstake amount should never be more than stake but check as a precaution */
                if(nUnstake < nStake)
                    nStakeNew = nStake - nUnstake;

                /* Check for version 8. */
                if(nVersion > 7)
                {
                    /* Build new score with 128-bit arithmatic to prevent overflows. */
                    uint128_t nScoreNew = (nStakeNew * uint128_t(nScore)) / nStake;
                    nScore = nScoreNew.Get64();
                }
                else //old way using 64-bits
                    nScore = (nStakeNew * nScore) / nStake;
            }

            return nScore;
        }


        /* Calculate the proof of stake block weight for a given block age. */
        cv::softdouble BlockWeight(const uint64_t nBlockAge)
        {
            /* Block Weight reaches maximum of 10.0 when Block Age equals the max block age */
            cv::softdouble nBlockRatio = cv::softdouble(nBlockAge) / cv::softdouble(MaxBlockAge());

            return std::min(cv::softdouble(10.0), (cv::softdouble(9.0) * cv::log((cv::softdouble(2.0) * nBlockRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
        }


        /* Calculate the equivalent proof of stake trust weight for staking Genesis with a given coin age. */
        cv::softdouble GenesisWeight(const uint64_t nCoinAge)
        {
            /* Trust Weight For Genesis is based on Coin Age. Genesis trust weight is less than normal trust weight,
             * reaching a maximum of 10.0 after average Coin Age reaches trust weight base.
             */
            cv::softdouble nWeightRatio = cv::softdouble(nCoinAge) / cv::softdouble(TrustWeightBase());

            return std::min(cv::softdouble(10.0), (cv::softdouble(9.0) * cv::log((cv::softdouble(2.0) * nWeightRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
        }


        /* Calculate the proof of stake trust weight for a given trust score. */
        cv::softdouble TrustWeight(const uint64_t nTrust)
        {
            /* Trust Weight base is time for 50% score. Weight continues to grow with Trust Score until it reaches max of 90.0
             * This formula will reach 45.0 (50%) after accumulating 84 days worth of Trust Score (Mainnet base),
             * while requiring close to a year to reach maximum.
             */
            cv::softdouble nWeightRatio = cv::softdouble(nTrust) / cv::softdouble(TrustWeightBase());

            return std::min(cv::softdouble(90.0), (cv::softdouble(44.0) * cv::log((cv::softdouble(2.0) * nWeightRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(3))) + cv::softdouble(1.0));
        }


        /* Calculate the current threshold value for Proof of Stake. */
        cv::softdouble GetCurrentThreshold(const uint64_t nBlockTime, const uint64_t nNonce)
        {
            return (cv::softdouble(nBlockTime) * cv::softdouble(100.0)) / cv::softdouble(nNonce);
        }


        /* Calculate the minimum Required Energy Efficiency Threshold. */
        cv::softdouble GetRequiredThreshold(cv::softdouble nTrustWeight, cv::softdouble nBlockWeight, const uint64_t nStake)
        {
            /*  Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
             *  Weight from staking balance reduces the required threshold by increasing the denominator.
             */
            return ((cv::softdouble(108.0) - cv::softdouble(nTrustWeight) - cv::softdouble(nBlockWeight)) * cv::softdouble(TAO::Ledger::MAX_STAKE_WEIGHT)) / cv::softdouble(nStake);
        }


        /* Calculate the stake rate corresponding to a given trust score. */
        cv::softdouble StakeRate(const uint64_t nTrust, const bool fGenesis)
        {
            /* Stake rate fixed at 0.005 (0.5%) when staking Genesis */
            if(fGenesis)
                return cv::softdouble(0.005);

            /* No trust score cap in Tritium staking, but use testnet max for testnet stake rate (so it grows faster) */
            static const uint32_t nRateBase = config::fTestNet ? TRUST_SCORE_MAX_TESTNET : ONE_YEAR;

            /* Stake rate starts at 0.005 (0.5%) and grows to 0.03 (3%) when trust score reaches or exceeds one year */
            cv::softdouble nTrustRatio = cv::softdouble(nTrust) / cv::softdouble(nRateBase);

            return std::min(cv::softdouble(0.03), (cv::softdouble(0.025) * cv::log((cv::softdouble(9.0) * nTrustRatio) + cv::softdouble(1.0)) / cv::log(cv::softdouble(10))) + cv::softdouble(0.005));
        }


        /* Calculate the coinstake reward for a given stake. */
        uint64_t GetCoinstakeReward(const uint64_t nStake, const uint64_t nStakeTime, const uint64_t nTrust, const bool fGenesis)
        {

            cv::softdouble nStakeRate = StakeRate(nTrust, fGenesis);

            /* Reward rate for time period is annual rate * (time period / annual time) or nStakeRate * (nStakeTime / ONE_YEAR)
             * Then, overall nStakeReward = nStake * reward rate
             *
             * Thus, the appropriate way to write this (for clarity) would be:
             *      StakeReward = nStake * nStakeRate * (nStakeTime / ONE_YEAR)
             *
             * However, with integer arithmetic (nStakeTime / ONE_YEAR) would evaluate to 0 or 1, etc. and the nStakeReward
             * would be erroneous.
             *
             * Therefore, it performs the full multiplication portion first.
             */
            uint64_t nStakeReward = (nStake * nStakeRate * nStakeTime) / ONE_YEAR;

            return nStakeReward;
        }


        /* Check the overall consistency of trust score calculations. */
        bool CheckConsistency(const uint512_t& hashLastTrust, uint64_t& nTrustRet)
        {
            /* Give a little bit of info that we are running the checks. */
            debug::log(0, FUNCTION, "Checking trust key consistency from ", hashLastTrust.SubString());

            /* Start searching from last stake block. */
            uint512_t hashLast = hashLastTrust;
            std::vector<int64_t> vChanges;

            /* Keep persistent version for calculating consistent score. */
            uint64_t nTrustPrev = 0;

            /* Loop until end of trust transactions. */
            bool fConsistent = true;
            while(true)
            {
                /* Read current transaction. */
                Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx))
                    break;

                /* Check for coinstake. */
                if(tx.IsCoinStake())
                {
                    /* Read the current block. */
                    TAO::Ledger::BlockState block;
                    if(!LLD::Ledger->ReadBlock(hashLast, block))
                        break;

                    /* Reset the coinstake contract streams. */
                    tx[0].Reset();

                    /* Get the trust object register. */
                    TAO::Register::Object account;

                    /* Deserialize from the stream. */
                    uint8_t nState = 0;
                    tx[0] >>= nState;
                    tx[0] >>= account;

                    /* Parse the object. */
                    if(!account.Parse())
                        return debug::error(FUNCTION, "failed to parse object register from pre-state");

                    /* Validate that it is a trust account. */
                    if(account.Standard() != TAO::Register::OBJECTS::TRUST)
                        return debug::error(FUNCTION, "stake producer account is not a trust account");

                    /* Values for trust calculations. */
                    uint64_t nBlockAge    = 0;
                    uint64_t nStake       = 0;
                    int64_t  nStakeChange = 0;

                    /* Check for trust calculations. */
                    if(tx.IsTrust())
                    {
                        /* Extract values from producer operation */
                        tx[0].Seek(1, TAO::Operation::Contract::OPERATIONS);

                        /* Get last trust hash. */
                        uint512_t hashLastTrust = 0;
                        tx[0] >> hashLastTrust;

                        /* Get claimed trust score. */
                        uint64_t nClaimedTrust = 0;
                        tx[0] >> nClaimedTrust;
                        tx[0] >> nStakeChange;

                        /* Get previous block. Block time used for block age/coin age calculation */
                        TAO::Ledger::BlockState statePrev;
                        if(!LLD::Ledger->ReadBlock(block.hashPrevBlock, statePrev))
                            return debug::error(FUNCTION, "prev block not in database");

                        /* Get the last stake block. */
                        TAO::Ledger::BlockState stateLast;
                        if(!LLD::Ledger->ReadBlock(hashLastTrust, stateLast))
                            return debug::error(FUNCTION, "last block not in database");

                        /* Calculate Block Age (time from last stake block until previous block) */
                        nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

                        /* Get pre-state trust account values */
                        nTrustPrev = account.get<uint64_t>("trust");
                        nStake     = account.get<uint64_t>("stake");

                        /* Check both values against eachother. */
                        uint64_t nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, 7);
                        uint64_t nCheck = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, 8);

                        /* Validate the trust score calculation */
                        if(nTrust != nCheck)
                        {
                            /* Set return value. */
                            fConsistent = false;

                            /* Push the stake change to value. */
                            vChanges.push_back(nCheck - nTrustPrev);
                            debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_GREEN, "Inconsistent ", nTrust, " value ", nCheck, " for user ", hashGenesis.SubString(), "... FIXED", ANSI_COLOR_RESET);
                        }
                        else
                            vChanges.push_back(nTrust - nTrustPrev);

                        /* Iterate backwards by last trust block. */
                        hashLast = hashLastTrust;
                    }
                    else
                        break;
                }
            }

            /* Break early if all checks passed. */
            if(fConsistent)
                return true;

            /* Reverse entries. */
            nTrustRet = nTrustPrev;
            for(auto it = vChanges.rbegin(); it != vChanges.rend(); ++it)
                nTrustRet += (*it);

            return false;
        }


        /** Retrieves the most recent stake transaction for a user account. */
        bool FindLastStake(const uint256_t& hashGenesis, Transaction& tx)
        {
            /* Start with most recent signature chain transaction. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast))
                return false;

            /* Loop until find stake transaction or reach first transaction on user acount (hashLast == 0). */
            while(hashLast != 0)
            {
                /* Get the transaction for the current hashLast. */
                Transaction txCheck;
                if(!LLD::Ledger->ReadTx(hashLast, txCheck))
                    return false;

                /* Test whether the transaction contains a staking operation */
                if(txCheck.IsCoinStake())
                {
                    /* Found last stake transaction. */
                    tx = txCheck;

                    return true;
                }

                /* Stake tx not found, yet, iterate to next previous user tx */
                hashLast = txCheck.hashPrevTx;
            }

            return false;
        }

    }
}
