/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/base_minter.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

    /* Retrieves the current internal value for the block weight metric. */
    double StakeMinter::GetBlockWeight() const
    {
        return nBlockWeight.load();
    }


    /* Retrieves the current block weight metric as a percentage of maximum. */
    double StakeMinter::GetBlockWeightPercent() const
    {
        return (nBlockWeight.load() * 100.0 / 10.0);
    }


    /* Retrieves the current internal value for the trust weight metric. */
    double StakeMinter::GetTrustWeight() const
    {
        return nTrustWeight.load();
    }


    /* Retrieves the current trust weight metric as a percentage of maximum. */
    double StakeMinter::GetTrustWeightPercent() const
    {
        return (nTrustWeight.load() * 100.0 / 90.0);
    }


    /* Retrieves the current staking reward rate */
    double StakeMinter::GetStakeRate() const
    {
        return nStakeRate.load();
    }


    /* Retrieves the current staking reward rate as an annual percentage */
    double StakeMinter::GetStakeRatePercent() const
    {
        return nStakeRate.load() * 100.0;
    }


    /* Checks whether the stake minter is waiting for average coin
     * age to reach the required minimum before staking Genesis.
     */
    bool StakeMinter::IsWaitPeriod() const
    {
        return fIsWaitPeriod.load();
    }

    } // End Ledger namespace
} // End TAO namespace
