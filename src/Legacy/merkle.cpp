/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/merkle.h>
#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace Legacy
{

    uint32_t MerkleTx::GetDepthInMainChain() const
    {
        if(hashBlock == 0)
            return 0;

        // Find the block it claims to be in
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashBlock, state))
            return 0;

        if(!state.IsInMainChain())
            return 0;

        return TAO::Ledger::ChainState::nBestHeight.load() - state.nHeight + 1;

    }


    /* Retrieve the number of blocks remaining until transaction outputs are spendable. */
    uint32_t MerkleTx::GetBlocksToMaturity() const
    {
        if(!(IsCoinBase() || IsCoinStake()))
            return 0;

        uint32_t nMaturity;
        uint32_t nDepth = GetDepthInMainChain();

        if(IsCoinBase())
            nMaturity = TAO::Ledger::MaturityCoinBase();
        else
            nMaturity = TAO::Ledger::MaturityCoinStake();

        /* Legacy mainnet maturity blocks needs +20 added so that wallet considers immature for 120 blocks.
         * The NEXUS_MATURITY_LEGACY setting value of 100 was kept for backwards compatability within other parts of code.
         */
        if(nMaturity == TAO::Ledger::NEXUS_MATURITY_LEGACY && !config::fTestNet)
            nMaturity += 20;

        if(nDepth >= nMaturity)
            return 0;
        else
            return nMaturity - nDepth;
    }
}
