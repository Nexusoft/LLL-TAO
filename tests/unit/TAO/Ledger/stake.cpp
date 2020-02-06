/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <Util/include/softfloat.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Trust Score Tests", "[ledger]")
{
    uint64_t nTrustScore;
    uint64_t nBlockAge;
    uint64_t nStake = 1000000000000; //1000000 NXS;
    uint64_t nStakeChange;
    uint32_t nVersion;

    /* Calculate new trust score, normal case */
    {
        nTrustScore = 35000000;
        nBlockAge = 5000;
        nStakeChange = 0;
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE(nTrustNew == 35005000);
    }

    /* Calculate new trust score, with decay */
    {
        nTrustScore = 35000000;
        nBlockAge = 100000 + TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET;
        nStakeChange = 0;
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE(nTrustNew == 34700000);
    }

    /* Test overflow for unstake of v7 blocks, no decay */
    {
        nTrustScore = 34995000;
        nBlockAge = 5000;
        nStakeChange = 0;
        nStakeChange = -100000000000; //unstake 100000 NXS (10% trust cost)
        nVersion = 7;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE_FALSE(nTrustNew == 31500000);
    }

    /* Test overflow for unstake of v8 blocks, no decay */
    {
        //Calculate trust score using v8 method
        nTrustScore = 34995000;
        nBlockAge = 5000;
        nStakeChange = -100000000000; //unstake 100000 NXS (10% trust cost)
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE(nTrustNew == 31500000);
    }

    /* Calculate new trust score, with decay and unstake */
    {
        nTrustScore = 35000000;
        nBlockAge = 100000 + TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET; //100000 seconds after timespan = 300000 trust score decay
        nStakeChange = -100000000000; //unstake 100000 NXS
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result (decay 300000, then 10% trust cost)
        REQUIRE(nTrustNew == 31230000);
    }
}


TEST_CASE( "Stake Metrics Tests", "[ledger]")
{
    uint64_t nStake = 1000000000000; //1000000 NXS

    /* Test max block weight */
    {
        cv::softdouble nBlockWeight = TAO::Ledger::BlockWeight(TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET);

        //Verify result
        REQUIRE(nBlockWeight == 10.0);
    }

    /* Test max genesis trust weight */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::GenesisWeight(TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET);

        //Verify result
        REQUIRE(nTrustWeight == 10.0);
    }

    /* Test trust weight base */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::TrustWeight(TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET);

        //Verify result
        REQUIRE(nTrustWeight == 45.0);
    }

    /* Test capped trust weight */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::TrustWeight(TAO::Ledger::ONE_YEAR);

        //Verify result
        REQUIRE(nTrustWeight == 90.0);
    }

    /* Stake Rate Genesis */
    {
        cv::softdouble nStakeRate = TAO::Ledger::StakeRate(0, true);

        //Verify result
        REQUIRE(nStakeRate == 0.005);

        //Calculate net stake reward for one year at this rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR, 0, true);

        REQUIRE(nCoinstakeReward == 5000000000); //5000 NXS
    }

    /* Stake Rate Max */
    {
        cv::softdouble nStakeRate = TAO::Ledger::StakeRate(TAO::Ledger::TRUST_SCORE_MAX_TESTNET, false);

        //Verify result
        REQUIRE(nStakeRate == 0.03);

        //Calculate net stake reward for one year at this rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR,
                                                                    TAO::Ledger::TRUST_SCORE_MAX_TESTNET, false);

        REQUIRE(nCoinstakeReward == 30000000000); //30000 NXS
    }

    /* Stake Rate - 50% trust weight */
    {
        /* Calculate the trust score in case ratio of TRUST_WEIGHT_BASE_TESTNET / TRUST_SCORE_MAX_TESTNET changes for testing.
           This hard codes the ratio of 84/364 as the base to max ratio. */
        uint64_t nTestTrust = (84 * TAO::Ledger::TRUST_SCORE_MAX_TESTNET) / 364;
        cv::softdouble nStakeRate = TAO::Ledger::StakeRate(nTestTrust, false);

        //Verify result (to 7 significant digits)
        nStakeRate = nStakeRate * cv::softdouble(10000000);
        nStakeRate = cv::softdouble(cvRound64(nStakeRate)) / cv::softdouble(10000000);
        REQUIRE(nStakeRate == 0.0172029);

        //Calculate net stake reward for one year at this rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR, nTestTrust, false);

        REQUIRE(nCoinstakeReward == 17202915975);
    }
}


TEST_CASE( "Proof of Stake Tests", "[ledger]")
{
    uint64_t nStake = 1000000000000; //1000000 NXS

    /* Stake Reward Genesis */
    {
        //Calculate net stake reward for one year at genesis rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR, 0, true);

        REQUIRE(nCoinstakeReward == 5000000000); //5000 NXS
    }

    /* Stake Reward Max */
    {
        //Calculate net stake reward for one year at max rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR,
                                                                    TAO::Ledger::TRUST_SCORE_MAX_TESTNET, false);

        REQUIRE(nCoinstakeReward == 30000000000); //30000 NXS
    }

    /* Stake Reward - 50% trust weight */
    {
        /* Calculate the trust score in case ratio of TRUST_WEIGHT_BASE_TESTNET / TRUST_SCORE_MAX_TESTNET changes for testing.
           This hard codes the ratio of 84/364 as the base to max ratio. */
        uint64_t nTestTrust = (84 * TAO::Ledger::TRUST_SCORE_MAX_TESTNET) / 364;

        //Calculate net stake reward for one year test trust rate
        uint64_t nCoinstakeReward = TAO::Ledger::GetCoinstakeReward(nStake, TAO::Ledger::ONE_YEAR, nTestTrust, false);

        REQUIRE(nCoinstakeReward == 17202915975);
    }

    /* Threshold */
    {
        cv::softdouble nThreshold = TAO::Ledger::GetCurrentThreshold(1000, 1000);

        //Verify result
        REQUIRE(nThreshold == 100.0);
    }


    nStake = 1000000000; //1000 NXS
    cv::softdouble nBlockWeight;

    /* Required Threshold Genesis with one timespan block weight */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::GenesisWeight(TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET);

        nBlockWeight = TAO::Ledger::BlockWeight(TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET);

        REQUIRE(nTrustWeight == 10.0);
        REQUIRE(nBlockWeight == 10.0);

        cv::softdouble nRequired = TAO::Ledger::GetRequiredThreshold(nTrustWeight, nBlockWeight, nStake);

        //Verify result
        REQUIRE(nRequired == 88.0);
    }

    /* Required Threshold Trust Weight Base */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::TrustWeight(TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET);

        REQUIRE(nTrustWeight == 45.0);

        cv::softdouble nRequired = TAO::Ledger::GetRequiredThreshold(nTrustWeight, nBlockWeight, nStake);

        //Verify result
        REQUIRE(nRequired == 53.0);
    }

    /* Required Threshold Trust Weight Capped */
    {
        cv::softdouble nTrustWeight = TAO::Ledger::TrustWeight(TAO::Ledger::ONE_YEAR);

        REQUIRE(nTrustWeight == 90.0);

        cv::softdouble nRequired = TAO::Ledger::GetRequiredThreshold(nTrustWeight, nBlockWeight, nStake);

        //Verify result
        REQUIRE(nRequired == 8.0);
    }
}
