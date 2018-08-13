/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_SUPPLY_H
#define NEXUS_CORE_INCLUDE_SUPPLY_H

#include "../../LLC/types/uint1024.h"

namespace Core
{
	
	class CBlockIndex;
	
	/** These values reflect the Three Decay Equations.
	 * 
	 * 	50 * e ^ (-0.0000011  * nMinutes) + 1.0
	 * 	10 * e ^ (-0.00000055 * nMinutes) + 1.0
	 * 	 1 * e ^ (-0.00000059 * nMinutes) + 0.032 
	 */
	extern double decay[3][3];
	
	
	/* Get the Total Amount to be Released at a given Minute since the NETWORK_TIMELOCK. */
	int64 GetSubsidy(int nMinutes, int nType);
	
	
	/* Calculate the Compounded amount of NXS to be released over the (nInterval) minutes. */
	int64 SubsidyInterval(int nMinutes, int nInterval);
	
	
	/* Calculate the Compounded amount of NXS that should "ideally" have been created to this minute. */
	int64 CompoundSubsidy(int nMinutes, int nTypes = 3);
	
	
	/* Get the total supply of NXS in the chain from the Index Objects that is calculated as chain is built. */
	int64 GetMoneySupply(CBlockIndex* pindex);
	
	
	/* Get the age of the Nexus Blockchain in a figure of Seconds. */
	int64 GetChainAge(int64 nTime);
	
	
	/* Get the Fractional Reward basing the total amount on a number of minutes vs a total reward. */
	int64 GetFractionalSubsidy(int nMinutes, int nType, double nFraction);
	
	
	/* Get the Coinbase Rewards based on the Reserve Balances to keep the Coinbase rewards under the Reserve Production Rates. */
	int64 GetCoinbaseReward(const CBlockIndex* pindex, int nChannel, int nType);
	
	
	/* Release a certain amount of Nexus into the Reserve System at a given Minute of time. */
	int64 ReleaseRewards(int nTimespan, int nStart, int nType);
	
	
	/* Get the total amount released into this given reserve by this point in time in the Block Index Object. */
	int64 GetReleasedReserve(const CBlockIndex* pindex, int nChannel, int nType);
	
	
	/* Check if there is any Nexus that will be released on the Next block in case the reserve values have been severely depleted. */
	bool  ReleaseAvailable(const CBlockIndex* pindex, int nChannel);
	
}

#endif


