/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/


#include <TAO/Ledger/include/chainstate.h>

namespace TAO::Ledger
{

    /* The best block height in the chain. */
    uint32_t ChainState::nBestHeight = 0;


    /* The best hash in the chain. */
    uint1024_t ChainState::hashBestChain = 0;


    /* The best trust in the chain. */
    uint64_t ChainState::nBestChainTrust = 0;


    /* Flag to tell if initial blocks are downloading. */
    bool ChainState::Synchronizing()
    {
        return false;
    }


    /** The best block in the chain. **/
    BlockState ChainState::stateBest;


    /** The genesis block in the chain. **/
    BlockState ChainState::stateGenesis;

}
