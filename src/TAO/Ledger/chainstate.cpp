/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>

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

    /* Initialize the Chain State. */
    bool ChainState::Initialize()
    {

        /* Initialize the Genesis. */
        if(!CreateGenesis())
            return debug::error(FUNCTION "failed to create genesis", __PRETTY_FUNCTION__);

        /* Read the best chain. */
        if(!LLD::legDB->ReadBestChain(hashBestChain))
            return debug::error(FUNCTION "failed to read best chain", __PRETTY_FUNCTION__);

        /* Get the best chain stats. */
        if(!LLD::legDB->ReadBlock(hashBestChain, stateBest))
            return debug::error(FUNCTION "failed to read best block", __PRETTY_FUNCTION__);

        if(stateBest.IsNull())
        {
            debug::error(FUNCTION "null best state...", __PRETTY_FUNCTION__);

            /* Get the best chain stats. */
            if(!LLD::legDB->ReadBlock(hashGenesis, stateBest))
                return debug::error(FUNCTION "failed to read best block", __PRETTY_FUNCTION__);

            hashBestChain = hashGenesis;
        }

        /* Fill out the best chain stats. */
        nBestHeight     = stateBest.nHeight;
        nBestChainTrust = stateBest.nChainTrust;
        stateBest.print();

        /* Debug logging. */
        debug::log2(0, TESTING, config::fTestNet? "Test" : "Nexus", " Network: genesis=", hashGenesis.ToString().substr(0, 20),
        " nBitsStart=0x", std::hex, bnProofOfWorkStart[0].GetCompact(), " best=", stateBest.GetHash().ToString().substr(0, 20),
        " height=", stateBest.nHeight);

        return true;
    }


    /** The best block in the chain. **/
    BlockState ChainState::stateBest;


    /** The genesis block in the chain. **/
    BlockState ChainState::stateGenesis;

}
