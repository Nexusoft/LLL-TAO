/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Proof of Stake Tests", "[ledger]")
{
    uint64_t nTrustScore;
    uint64_t nBlockAge;
    uint64_t nStake;
    uint64_t nStakeChange;
    uint32_t nVersion;

    /* Calculate new trust score, normal case */
    {
        nTrustScore = 35000000;
        nBlockAge = 5000;
        nStake = 1000000000000;
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
        nStake = 1000000000000;
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
        nStake = 1000000000000;
        nStakeChange = -100000000000;
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
        nStake = 1000000000000;
        nStakeChange = -100000000000;
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE(nTrustNew == 31500000);
    }

    /* Calculate new trust score, with decay and unstake */
    {
        nTrustScore = 35000000;
        nBlockAge = 100000 + TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET;
        nStake = 1000000000000;
        nStakeChange = -100000000000;
        nVersion = 8;
        uint64_t nTrustNew = TAO::Ledger::GetTrustScore(nTrustScore, nBlockAge, nStake, nStakeChange, nVersion);

        //Verify result
        REQUIRE(nTrustNew == 31230000);
    }
}
