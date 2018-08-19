/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_CHECKPOINTS_H
#define NEXUS_CORE_INCLUDE_CHECKPOINTS_H

#include <map>

class LLC::uint1024;

namespace Core
{
	class CBlockIndex;
	
	
	/* Memory Map to hold all the hashes of the checkpoints decided on by the network. */
	extern std::map<uint32_t, LLC::uint1024> mapCheckpoints;
	
	
	/* Checkpoint Timespan, or the time that triggers a new checkpoint (in Minutes). */
	extern uint32_t CHECKPOINT_TIMESPAN;
	
	
	/* Checkpoint Search. The Maximum amount of checkpoints that can be serached back to find a Descendant. */
	extern uint32_t MAX_CHECKPOINTS_SEARCH;

	
	
	/* __________________________________________________ (Checkpoints Methods) __________________________________________________  */
	
	
	
	
	/* Get the most recent checkpoint that was agreed upon by the timestamp. */
	LLC::uint1024 GetLastCheckpoint();
	
	
	/* Check if the new block triggers a new Checkpoint timespan. */
	bool IsNewTimespan(CBlockIndex* pindex);
	
	
	/* Check that the checkpoint is a Descendant of previous Checkpoint. */
	bool IsDescendant(CBlockIndex* pindex);
	
	
	/* Harden a checkpoint into the checkpoint chain. */
	bool HardenCheckpoint(CBlockIndex* pcheckpoint, bool fInit = false);
	
	
}


#endif
