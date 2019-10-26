/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/types/state.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>

#include <cmath>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Checkpoint timespan. **/
        uint32_t CHECKPOINT_TIMESPAN = 90;


        /* Check if the new block triggers a new Checkpoint timespan.*/
        bool IsNewTimespan(const BlockState& state)
        {
            /* Catch if checkpoint is not established. */
            if(ChainState::hashCheckpoint == 0)
                return true;

            /* Get previous block state. */
            BlockState statePrev;
            if(!LLD::legDB->ReadBlock(state.hashPrevBlock, statePrev))
                return true;

            /* Get checkpoint state. */
            BlockState stateCheck;
            if(!LLD::legDB->ReadBlock(state.hashCheckpoint, stateCheck))
                return debug::error(FUNCTION, "failed to read checkpoint");

            /* Calculate the time differences. */
            uint32_t nFirstMinutes = static_cast<uint32_t>(floor(static_cast<double>(state.GetBlockTime() - stateCheck.GetBlockTime()) / 60.0));
            uint32_t nLastMinutes =  static_cast<uint32_t>(floor(static_cast<double>(statePrev.GetBlockTime() - stateCheck.GetBlockTime()) / 60.0));

            return (nFirstMinutes != nLastMinutes && nFirstMinutes >= CHECKPOINT_TIMESPAN);
        }


        /* Check that the checkpoint is a Descendant of previous Checkpoint.*/
        bool IsDescendant(const BlockState& state)
        {
            /* If no checkpoint defined, return true. */
            if(ChainState::hashCheckpoint == 0)
                return true;

            /* Check hard coded checkpoints when syncing. */
            if(ChainState::Synchronizing())
            {
                /* Check that height isn't exceeded. */
                if(config::fTestNet || state.nHeight > CHECKPOINT_HEIGHT)
                    return true;

                /* Check map checkpoints. */
                auto it = mapCheckpoints.find(state.nHeight);
                if(it == mapCheckpoints.end())
                    return true;

                /* Verbose logging for hardcoded checkpoints. */
                debug::log(0, "===== HARDCODED Checkpoint ", it->first, " Hash ", it->second.ToString().substr(0, 20));

                /* Block must match checkpoints map. */
                return it->second == state.hashCheckpoint;
            }

            /* Check The Block Hash */
            BlockState check = state;
            while(!check.IsNull())
            {
                /* Check that checkpoint exists in the map. */
                if(ChainState::hashCheckpoint.load() == check.hashCheckpoint)
                    return true;

                /* Break when new height is found. */
                if(state.nHeight < ChainState::nCheckpointHeight.load())
                    return false;

                /* Iterate backwards. */
                check = check.Prev();
            }

            return false;
        }


        /*Harden a checkpoint into the checkpoint chain.*/
        bool HardenCheckpoint(const BlockState& state)
        {
            /* Only Harden New Checkpoint if it Fits new timestamp. */
            if(!IsNewTimespan(state))
                return false;

            /* Update the Checkpoints into Memory. */
            ChainState::hashCheckpoint    = state.hashCheckpoint;

            /* Get checkpoint state. */
            BlockState stateCheckpoint;
            if(!LLD::legDB->ReadBlock(state.hashCheckpoint, stateCheckpoint))
                return debug::error(FUNCTION, "failed to read checkpoint");

            /* Set the correct height for the checkpoint. */
            ChainState::nCheckpointHeight = stateCheckpoint.nHeight;

            /* Dump the Checkpoint if not Initializing. */
            debug::log(ChainState::Synchronizing() ? 1 : 0, "===== Hardened Checkpoint ", ChainState::hashCheckpoint.load().ToString().substr(0, 20), " Height ", ChainState::nCheckpointHeight.load());

            return true;
        }
    }
}
