/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
			
			(c) Copyright The Nexus Developers 2014 - 2018
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#include "include/supply.h"
#include "include/unifiedtime.h"

#include "network/include/blkpool.h"

using namespace std;
namespace Core
{
    
	/* Use the decay equations. */
	double decay[3][3] = { {50.0, -0.0000011, 1.0}, {10.0, -0.00000055, 1.0}, {1.0, -0.00000059, 0.032} };

	
	/* Returns the value of a full minutes reward per channel */
	int64_t GetSubsidy(int nMinutes, int nType) { return (((decay[nType][0] * exp(decay[nType][1] * nMinutes)) + decay[nType][2]) * (COIN / 2.0)); }
	

	/* Compound the subsidy from a start point to an interval point. */
	int64_t SubsidyInterval(int nMinutes, int nInterval)
	{
		int64_t nMoneySupply = 0;
		nInterval += nMinutes;
		
		for(int nMinute = nMinutes; nMinute < nInterval; nMinute++)
			for(int nType = 0; nType < 3; nType++)
				nMoneySupply += (GetSubsidy(nMinute, nType) * 2);
				
		return nMoneySupply;
	}
	

	/** Returns the Calculated Money Supply after nMinutes compounded from the Decay Equations. **/
	int64_t CompoundSubsidy(int nMinutes, int nTypes)
    {
		int64_t nMoneySupply = 0;
		for(int nMinute = 1; nMinute <= nMinutes; nMinute++)
			for(int nType = (nTypes == 3 ? 0 : nTypes); nType < (nTypes == 3 ? 4 : nTypes + 1); nType++) //nTypes == 3 designates all channels, nTypes == 0, 1, 2 equals miner, ambassador, and developers
				nMoneySupply += (GetSubsidy(nMinute, nType) * 2);
				
		return nMoneySupply;
	}
	
	
	/* Calculates the Actual Money Supply from nReleasedReserve and nMoneySupply. */
    uint64_t GetMoneySupply(const CBlockState blk)
	{
        return blk.nMoneySupply;
	}
	
	
	/* Get the Age of the Block Chain in Minutes. */
	int64_t GetChainAge(int64_t nTime) { return floor((nTime - (int64_t)(fTestNet ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK)) / 60.0); }ß
	
	
	/* Returns the Time Based value of the Block which is a Fraction of Time's worth of Full Minutes Subsidy. */
    int64_t GetFractionalSubsidy(int nMinutes, int nType, double nFraction)
	{
		int nInterval = floor(nFraction);
		double nRemainder   = nFraction - nInterval;
		
		int64_t nSubsidy = 0;
		for(int nMinute = 0; nMinute < nInterval; nMinute++)
			nSubsidy += GetSubsidy(nMinutes + nMinute, nType);

		return nSubsidy + (GetSubsidy(nMinutes + nInterval, nType) * nRemainder);
	}


	/* Releases Nexus into Blockchain for Miners to Create. */
	int64_t ReleaseRewards(int nTimespan, int nStart, int nType)
	{
		int64_t nSubsidy = 0;
		for(int nMinutes = nStart; nMinutes < (nStart + nTimespan); nMinutes++)
			nSubsidy += GetSubsidy(nMinutes, nType);
		
		//printf("Reserve %i: %f Nexus | Timespan: %i - %i Minutes\n", nType, (double)nSubsidy / COIN, nStart, (nStart + nTimespan));
		return nSubsidy;
	}
	
	
	/* Releases Nexus into Blockchain [2% of Money Supply] for POS Minting. */
	int64_t ReleaseInflation(int nTimespan, int nStart)
	{
		int64_t nSubsidy = GetInflation(nTimespan, CompoundSubsidy(nStart));
		
		printf("Inflation: %f Nexus | Timespan %i - %i Minutes\n", (double)nSubsidy / COIN, nStart, (nStart + nTimespan));
		return nSubsidy;
	}
    
    
    /* Returns the Coinbase Reward Generated for Block Timespan */
    int64_t CBlkPool::GetCoinbaseReward(const CBlock blk, int nChannel, int nType)
    {
        const CBlockState blkFirst = GetLastBlock(blk.GetHash(), nChannel);
        if(!blkFirst.hashPrevBlock == 0)
            return COIN;
        
        const CBlockState blkLast = GetLastBlock(blkFirst.GetHash(), nChannel);
        if(!blkLast.hashPrevBlock == 0)
            return GetSubsidy(1, nType);
        
        
        int64_t nBlockTime = max(blkFirst.GetBlockTime() - blkLast.GetBlockTime(), (int64_t) 1 );
        int64_t nMinutes   = ((blk.nVersion >= 3) ? GetChainAge(blkFirst.GetBlockTime()) : min(blkFirst.nChannelHeight,  GetChainAge(blkFirst.GetBlockTime())));
        
        
        /* Block Version 3 Coinbase Tx Calculations. */
        if(blk.nVersion >= 3)
        {
            
            /* For Block Version 3: Release 3 Minute Reward decayed at Channel Height when Reserves are above 20 Minute Supply. */
            if(blkFirst.nReleasedReserve[nType] > GetFractionalSubsidy(blkFirst.nChannelHeight, nType, 20.0))
                return GetFractionalSubsidy(blkFirst.nChannelHeight, nType, 3.0);
            
            
            /* Otherwise release 2.5 Minute Reward decayed at Chain Age when Reserves are above 4 Minute Supply. */
            else if(blkFirst.nReleasedReserve[nType] > GetFractionalSubsidy(nMinutes, nType, 4.0))
                return GetFractionalSubsidy(nMinutes, nType, 2.5);
            
        }
        
        /* Block Version 1 Coinbase Tx Calculations: Release 2.5 minute reward if supply is behind 4 minutes */
        else if(blkFirst.nReleasedReserve[nType] > GetFractionalSubsidy(nMinutes, nType, 4.0))
            return GetFractionalSubsidy(nMinutes, nType, 2.5);
        
        
        double nFraction = min(nBlockTime / 60.0, 2.5);
        
        //TODO: DEPRECATE AND TEST
        if(blkFirst.nReleasedReserve[nType] == 0 && ReleaseAvailable(blk, nChannel))
            return GetFractionalSubsidy(nMinutes, nType, nFraction);
        
        return min(GetFractionalSubsidy(nMinutes, nType, nFraction), blkFirst.nReleasedReserve[nType]);
    }
	

	/* Calculates the release of new rewards based on the Network Time */
    int64_t CBlkPool::GetReleasedReserve(const CBlockState blk, int nChannel, int nType)
	{
        const CBlockState blkFirst = GetLastBlock(blk.GetHash(), nChannel);
        if(!blkFirst.hashPrevBlock == 0)
            return COIN;
        
        const CBlockState blkLast = GetLastBlock(blkFirst.GetHash(), nChannel);
        if(!blkLast.hashPrevBlock == 0)
            return ReleaseRewards(nMinutes + 5, 1, nType);
			
		/* Only allow rewards to be released one time per minute */
		int nLastMinutes = GetChainAge(blkLast.GetBlockTime());
		if(nMinutes == nLastMinutes)
			return 0;
		
		return ReleaseRewards((nMinutes - nLastMinutes), nLastMinutes, nType);
	}
	
	
	/* TODO: DEPRECATE THIS METHOD
     * If the Reserves are Depleted, this Tells a miner if there is a new Time Interval with their Previous Block which would signal new release into reserve.
	 * If for some reason this is a false flag, the block will be rejected by the network for attempting to deplete the reserves past 0 */
    bool CBlkPool::ReleaseAvailable(const CBlockState blk, int nChannel)
	{
		const CBlockState blkLast = GetLastBlock(blk.GetHash(), nChannel);
		if(blkLast.hashPrevBlock == 0)
			return true;
			
		return !(GetChainAge(UnifiedTimestamp()) == GetChainAge(blkLast.GetBlockTime()));
	}
}
