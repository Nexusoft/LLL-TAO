/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
			
			(c) Copyright The Nexus Developers 2014 - 2018
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_CHECKPOINTS_H
#define NEXUS_CORE_INCLUDE_CHECKPOINTS_H

#include <map>

class uint1024_t;

namespace Core
{
	class CBlockIndex;
	
	
	/* Memory Map to hold all the hashes of the checkpoints decided on by the network. */
	extern std::map<uint32_t, uint1024_t> mapCheckpoints;
	
	
	/* Checkpoint Timespan, or the time that triggers a new checkpoint (in Minutes). */
	extern uint32_t CHECKPOINT_TIMESPAN;
	
	
	/* Checkpoint Search. The Maximum amount of checkpoints that can be serached back to find a Descendant. */
	extern uint32_t MAX_CHECKPOINTS_SEARCH;

	
	
	/* __________________________________________________ (Checkpoints Methods) __________________________________________________  */
	
	
	
	
	/* Get the most recent checkpoint that was agreed upon by the timestamp. */
	uint1024_t GetLastCheckpoint();
	
	
	/* Check if the new block triggers a new Checkpoint timespan. */
	bool IsNewTimespan(CBlockIndex* pindex);
	
	
	/* Check that the checkpoint is a Descendant of previous Checkpoint. */
	bool IsDescendant(CBlockIndex* pindex);
	
	
	/* Harden a checkpoint into the checkpoint chain. */
	bool HardenCheckpoint(CBlockIndex* pcheckpoint, bool fInit = false);
	
	
}


#endif
