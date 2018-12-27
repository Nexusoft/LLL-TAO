/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/state.h>

#include <Legacy/include/money.h>

#include <cmath>

namespace TAO::Ledger
{

    /* Get the Total Amount to be Released at a given Minute since the NETWORK_TIMELOCK. */
	uint64_t GetSubsidy(uint32_t nMinutes, uint32_t nType)
    {
        return (((decay[nType][0] * exp(decay[nType][1] * nMinutes))
            + decay[nType][2]) * (Legacy::COIN / 2.0));
    }


    /* Calculate the Compounded amount of NXS to be released over the (nInterval) minutes. */
	uint64_t SubsidyInterval(uint32_t nMinutes, uint32_t nInterval)
	{
		uint64_t nMoneySupply = 0;
		nInterval += nMinutes;

        /* Compound all the minutes of the interval and types. */
		for(uint32_t nMinute = nMinutes; nMinute < nInterval; nMinute++)
        {
			for(uint32_t nType = 0; nType < 3; nType++)
            {
				nMoneySupply += (GetSubsidy(nMinute, nType) * 2);
            }
        }

		return nMoneySupply;
	}


    /* Calculate the Compounded amount of NXS that should "ideally" have been created to this minute. */
	uint64_t CompoundSubsidy(int32_t nMinutes, uint8_t nTypes)
    {
		uint64_t nMoneySupply = 0;
		for(int nMinute = 1; nMinute <= nMinutes; nMinute++)
        {
			for(int nType = (nTypes == 3 ? 0 : nTypes); nType < (nTypes == 3 ? 4 : nTypes + 1); nType++)
            {
				nMoneySupply += (GetSubsidy(nMinute, nType) * 2);
            }
        }

		return nMoneySupply;
	}


    /* Get the total supply of NXS in the chain from the state. */
	uint64_t GetMoneySupply(BlockState state)
	{
        return state.nMoneySupply;
	}


    /* Get the age of the Nexus blockchain in seconds. */
	uint32_t GetChainAge(uint64_t nTime)
    {
        return floor((nTime - (uint32_t)(config::fTestNet ?
            NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK)) / 60.0);
    }


    /* Get a fractional reward based on time. */
	uint64_t GetFractionalSubsidy(uint32_t nMinutes, uint8_t nType, double nFraction)
	{
		int nInterval = floor(nFraction);
		double nRemainder   = nFraction - nInterval;

		uint64_t nSubsidy = 0;
		for(int nMinute = 0; nMinute < nInterval; nMinute++)
			nSubsidy += GetSubsidy(nMinutes + nMinute, nType);

		return nSubsidy + (GetSubsidy(nMinutes + nInterval, nType) * nRemainder);
	}


    /* Get the Coinbase Rewards based on the Reserve Balances to keep the Coinbase rewards under the Reserve Production Rates. */
	uint64_t GetCoinbaseReward(const BlockState state, uint32_t nChannel, uint8_t nType)
    {
        /* Get Last Block Index [1st block back in Channel]. **/
        BlockState first = state;
        if (!GetLastState(first, nChannel))
            return Legacy::COIN;


        /* Get Last Block Index [2nd block back in Channel]. */
        BlockState last = first.Prev();
        if (!GetLastState(last, nChannel))
            return GetSubsidy(1, nType);


        /* Calculate the times between blocks. */
        uint64_t nBlockTime = std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t) 1 );
        uint64_t nMinutes   = ((state.nVersion >= 3) ?
            GetChainAge(first.GetBlockTime()) : std::min(first.nChannelHeight,  GetChainAge(first.GetBlockTime())));


        /* Block Version 3 Coinbase Tx Calculations. */
        if(state.nVersion >= 3)
        {

            /* For Block Version 3: Release 3 Minute Reward decayed at Channel Height when Reserves are above 20 Minute Supply. */
            if(first.nReleasedReserve[nType] > GetFractionalSubsidy(first.nChannelHeight, nType, 20.0))
                return GetFractionalSubsidy(first.nChannelHeight, nType, 3.0);


            /* Otherwise release 2.5 Minute Reward decayed at Chain Age when Reserves are above 4 Minute Supply. */
            else if(first.nReleasedReserve[nType] > GetFractionalSubsidy(nMinutes, nType, 4.0))
                return GetFractionalSubsidy(nMinutes, nType, 2.5);

        }


        /* Block Version 1 Coinbase Tx Calculations: Release 2.5 minute reward if supply is behind 4 minutes */
        else if(first.nReleasedReserve[nType] > GetFractionalSubsidy(nMinutes, nType, 4.0))
            return GetFractionalSubsidy(nMinutes, nType, 2.5);


        /* Calculate the fraction of 2.5 minutes. */
        double nFraction = std::min(nBlockTime / 60.0, 2.5);


        return std::min(GetFractionalSubsidy(nMinutes, nType, nFraction), (uint64_t)first.nReleasedReserve[nType]);
    }


    /* Release a certain amount of Nexus into the Reserve System at a given Minute of time. */
	uint64_t ReleaseRewards(uint32_t nTimespan, uint32_t nStart, uint8_t nType)
    {
        uint64_t nSubsidy = 0;
        for(int nMinutes = nStart; nMinutes < (nStart + nTimespan); nMinutes++)
        {
            nSubsidy += GetSubsidy(nMinutes, nType);
        }

        //debug::log(0, "Reserve %i: %f Nexus | Timespan: %i - %i Minutes", nType, (double)nSubsidy / COIN, nStart, (nStart + nTimespan));
        return nSubsidy;
    }


    /* Get the total amount released into this given reserve by this point in time in the block state */
	uint64_t GetReleasedReserve(const BlockState state, uint32_t nChannel, uint8_t nType)
	{
        /* Get Last Block Index [1st block back in Channel]. **/
        BlockState first = state;
        if (!GetLastState(first, nChannel))
            return Legacy::COIN;

        /* Get Last Block Index [2nd block back in Channel]. */
        int32_t nMinutes = GetChainAge(first.GetBlockTime());
        BlockState last = first.Prev();
        if (!GetLastState(last, nChannel))
            return ReleaseRewards(nMinutes + 5, 1, nType);


		/* Only allow rewards to be released one time per minute */
		int32_t nLastMinutes = GetChainAge(last.GetBlockTime());
		if(nMinutes == nLastMinutes)
			return 0;


		return ReleaseRewards((nMinutes - nLastMinutes), nLastMinutes, nType);
	}


	/* TODO: DEPRECATE THIS METHOD
     * If the Reserves are Depleted, this Tells a miner if there is a new Time Interval with their Previous Block which would signal new release into reserve.
	 * If for some reason this is a false flag, the block will be rejected by the network for attempting to deplete the reserves past 0 */
    bool ReleaseAvailable(const BlockState state, int nChannel)
	{
        /* Get Last Block Index [1st block back in Channel]. **/
        BlockState last = state;
        if (!GetLastState(last, nChannel))
            return true;

		return !(GetChainAge(runtime::UnifiedTimestamp()) == GetChainAge(last.GetBlockTime()));
	}
}
