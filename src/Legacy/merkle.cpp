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

        /* Mainnet maturity blocks needs +20 added. The NEXUS_MATURITY_BLOCKS setting of 100 was kept for backwards compatability within other parts of code. */
        int32_t nCoinbaseMaturity = config::fTestNet ? TAO::Ledger::TESTNET_MATURITY_BLOCKS : (TAO::Ledger::NEXUS_MATURITY_BLOCKS + 20);

        /* Need to add an extra 1 to nCoinbaseMaturity because maturity requires that many blocks AFTER the coinbase/coinstake
         * which counts as 1 itself for value returned by GetDepthInMainChain(), where top of chain is depth 1.
         *
         * So, for mainnet, 120 blocks for maturity is reached when depth of block containing this transaction is 121
         * Thus, blocks remaining to maturity is 121 - (depth of block containing this transactions).
         * For example, immediately after being added to chain, this transaction is at top of chain with depth 1 and (121 - 1) = 120 blocks to maturity
         */
        return std::max((int32_t)0, (int32_t)(nCoinbaseMaturity + 1 - GetDepthInMainChain()));
    }
}
