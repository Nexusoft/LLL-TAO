/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/types/state.h>

#include <TAO/Ledger/include/checkpoints.h>

#include <cmath>

namespace TAO::Ledger
{

    /* Memory Map to hold all the hashes of the checkpoints decided on by the network. */
    std::map<uint1024_t, uint32_t> mapCheckpoints;


    /* Checkpoint Timespan, or the time that triggers a new checkpoint (in Minutes). */
    const uint32_t CHECKPOINT_TIMESPAN = 15;


    /*Check if the new block triggers a new Checkpoint timespan.*/
    bool IsNewTimespan(const BlockState state)
    {
        /* Always return true if no checkpoints. */
        if(mapCheckpoints.empty())
            return true;

        /* Get previous block state. */
        BlockState statePrev;
        if(!LLD::legDB->ReadBlock(state.hashPrevBlock, statePrev))
            return true;

        /* Get checkpoint state. */
        BlockState stateCheck;
        if(!LLD::legDB->ReadBlock(state.hashCheckpoint, stateCheck))
            return debug::error(FUNCTION "failed to read checkpoint", __PRETTY_FUNCTION__);

        /* Calculate the time differences. */
        uint32_t nFirstMinutes = floor((state.GetBlockTime() - stateCheck.GetBlockTime()) / 60.0);
        uint32_t nLastMinutes =  floor((statePrev.GetBlockTime() - stateCheck.GetBlockTime()) / 60.0);

        return (nFirstMinutes != nLastMinutes && nFirstMinutes >= CHECKPOINT_TIMESPAN);
    }


    /* Check that the checkpoint is a Descendant of previous Checkpoint.*/
    bool IsDescendant(const BlockState state)
    {
        if(mapCheckpoints.empty() || state.nHeight <= 1)
            return true;

        /* Check that checkpoint exists in the map. */
        if(mapCheckpoints.count(state.hashCheckpoint))
            return true;

        return false;
    }


    /*Harden a checkpoint into the checkpoint chain.*/
    bool HardenCheckpoint(const BlockState state)
    {

        /* Only Harden New Checkpoint if it Fits new timestamp. */
        if(!IsNewTimespan(state.Prev()))
            return false;

        /* Only Harden a New Checkpoint if it isn't already hardened. */
        if(mapCheckpoints.count(state.hashCheckpoint))
            return true;

        /* Update the Checkpoints into Memory. */
        mapCheckpoints[state.hashCheckpoint] = 0;


        /* Dump the Checkpoint if not Initializing. */
        debug::log(0, "===== Hardened Checkpoint %s", state.hashCheckpoint.ToString().substr(0, 20).c_str());

        return true;
    }
}
