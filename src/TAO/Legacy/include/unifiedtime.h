/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_UNIFIEDTIME_H
#define NEXUS_CORE_INCLUDE_UNIFIEDTIME_H

#include <vector>

#include "../../LLC/types/uint1024.h"

namespace Core
{
	
	/* Unified Time Flags */
	extern bool fTimeUnified;


	/* Offset calculated from average Unified Offset collected over time. */
	extern int UNIFIED_AVERAGE_OFFSET;


	/* Vector to Contain list of Unified Time Offset from Time Seeds, Seed Nodes, and Peers. */
	extern std::vector<int> UNIFIED_TIME_DATA;


	/** Initializes the Unified Time System.
		A] Checks Database for Offset Average List
		B] Gets Periodic Average of 10 Seeds if first Unified Time **/
	void InitializeUnifiedTime();


	/** Gets the Current Unified Time Average. The More Data in Time Average, the less
		a pesky node with a manipulated time seed can influence. Keep in mind too that the
		Unified Time will only be updated upon your outgoing connection... otherwise anyone flooding
		network with false time seed will just be ignored. The average is a moving one, so that iter_swap
		will allow clocks that operate under different intervals to always stay synchronized with the network. **/
	int GetUnifiedAverage();


	/* Gets the UNIX Timestamp from the Nexus Network */
	uint64 UnifiedTimestamp(bool fMilliseconds = false);
	
	
	/* The unified time samples thread.
	 * TODO: Remove post Tritium
	 */
	void ThreadUnifiedSamples(void* parg);
	
}


#endif
