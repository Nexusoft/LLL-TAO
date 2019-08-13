/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
        struct ChainState
        {
            /** The best block height in the chain. **/
            static std::atomic<uint32_t> nBestHeight;


            /** The best trust in the chain. **/
            static std::atomic<uint64_t> nBestChainTrust;


            /** The current checkpoint height. **/
            static std::atomic<uint64_t> nCheckpointHeight;


            /** The best hash in the chain. */
            static memory::atomic<uint1024_t> hashBestChain;


            /** Hardened Checkpoint. **/
            static memory::atomic<uint1024_t> hashCheckpoint;


            /** Synchronizing
             *
             *  Flag to tell if initial blocks are downloading.
             *
             **/
            static bool Synchronizing();


            /** PercentSynchronized
             *
             *  Real value of the total synchronzation percent completion.
             *
             **/
            static double PercentSynchronized();


            /** Initialize
             *
             *  Initialize the Chain State.
             *
             **/
            static bool Initialize();


            /** Genesis
             *
             *  Get the hash of the genesis block.
             *
             **/
            static uint1024_t Genesis();


            /** The best block in the chain. **/
            static memory::atomic<BlockState> stateBest;


            /** The best block in the chain. **/
            static BlockState stateGenesis;

        };
    }
}


#endif
