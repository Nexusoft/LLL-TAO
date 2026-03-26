/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_CHAINSTATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CHAINSTATE_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/state.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** ChainState
         *
         *
         *
         **/
        namespace ChainState
        {

            /** The best block height in the chain. **/
            extern std::atomic<uint32_t> nBestHeight;


            /** The best trust in the chain. **/
            extern std::atomic<uint64_t> nBestChainTrust;


            /** The current checkpoint height. **/
            extern std::atomic<uint64_t> nCheckpointHeight;


            /** Track if the chain is currently undergoing a re-org. */
            extern std::atomic<bool> fChainReorg;


            /** The best hash in the chain. */
            extern memory::atomic<uint1024_t> hashBestChain;


            /** Hardened Checkpoint. **/
            extern memory::atomic<uint1024_t> hashCheckpoint;


            /** Synchronizing
             *
             *  Flag to tell if initial blocks are downloading.
             *
             **/
            bool Synchronizing();


            /** PercentSynchronized
             *
             *  Real value of the total synchronization percent completion.
             *
             **/
            double PercentSynchronized();


            /** SyncProgress
             *
             *  Percentage of blocks synchronized since the node started.
             *
             **/
            double SyncProgress();


            /** Initialize
             *
             *  Initialize the Chain State.
             *
             **/
            bool Initialize();


            /** Genesis
             *
             *  Get the hash of the genesis block.
             *
             **/
            uint1024_t Genesis();


            /** The best block in the chain. **/
            extern memory::atomic<BlockState> tStateBest;


            /** The best block in the chain. **/
            extern BlockState tStateGenesis;


            /** RepairCheckpointIfStale
             *
             *  Attempt to repair the in-memory checkpoint state when it has drifted from
             *  the on-disk best-chain state.  The in-memory gate (tStateBest.hashCheckpoint
             *  vs hashCheckpoint) is checked first to avoid disk I/O when no repair is
             *  needed; only a confirmed mismatch triggers the ReadBlock() disk read.
             *
             *  On success the in-memory hashCheckpoint and nCheckpointHeight are updated
             *  to match the on-disk best state, and the caller should retry IsDescendant().
             *
             *  @return true  if the checkpoint was successfully repaired
             *  @return false if the in-memory state was already consistent (no repair
             *                needed), or if the repair could not be completed
             *
             **/
            bool RepairCheckpointIfStale();

        }
    }
}


#endif
