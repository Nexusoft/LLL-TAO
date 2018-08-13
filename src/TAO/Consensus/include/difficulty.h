/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_DIFFICULTY_H
#define NEXUS_CORE_INCLUDE_DIFFICULTY_H

namespace Core
{
	class CBlockIndex;
	
	
	/* Target Timespan of 300 Seconds. */
	const unsigned int nTargetTimespan = 300;
	
	
	/* Determines the Decimal of nBits per Channel for a decent "Frame of Reference". Has no functionality in Network Operation. */
	double GetDifficulty(unsigned int nBits, int nChannel);
	
	
	/* Break the Chain Age in Minutes into Days, Hours, and Minutes. */
	void GetChainTimes(unsigned int nAge, unsigned int& nDays, unsigned int& nHours, unsigned int& nMinutes);
	
	
	/* Get Weighted Times functions to weight the average on an iterator */
	int64 GetWeightedTimes(const CBlockIndex* pindex, unsigned int nDepth);

	
	/* Switching function for each difficulty re-target [each channel uses their own version] */
	unsigned int GetNextTargetRequired(const CBlockIndex* pindex, int nChannel);
	
	
	/* Trust Channel Retargeting: Modulate Difficulty based on production rate. */
	unsigned int RetargetTrust(const CBlockIndex* pindex);
	
	
	/* Prime Channel Retargeting. Very different than GPU or POS retargeting. Scales the Maximum
		Increase / Decrease by Network Difficulty. This helps to keep increases more time based than
		mathematically based. This means that as the difficulty rises, the maximum up/down in difficulty
		will decrease keeping the time difference in difficulty jumps the same from diff 1 - 100. */
	unsigned int RetargetPrime(const CBlockIndex* pindex);

	
	/* Hash Channel Retargeting: Modulate Difficulty based on production rate. */
	unsigned int RetargetHash(const CBlockIndex* pindex);
}

#endif
