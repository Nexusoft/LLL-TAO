/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_CHAINSTATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_CHAINSTATE_H

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        struct ChainState
        {
            /** The best block height in the chain. **/
            static uint32_t nBestHeight;


            /** The best hash in the chain. */
            static uint1024_t hashBestChain;


            /** The best trust in the chain. **/
            static uint64_t nBestChainTrust;


            /** Hardened Checkpoint. **/
            static uint1024_t hashCheckpoint;


            /** Flag to tell if initial blocks are downloading. **/
            static bool Synchronizing();


            /** Initialize the Chain State. */
            static bool Initialize();


            /** The best block in the chain. **/
            static BlockState stateBest;


            /** The best block in the chain. **/
            static BlockState stateGenesis;

        };
    }
}


#endif
