/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/merkle.h>
#include <LLD/include/legacy.h>
#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace Legacy
{

    uint32_t CMerkleTx::GetDepthInMainChain() const
    {
        if (hashBlock == 0)
            return 0;

        // Find the block it claims to be in
        TAO::Ledger::BlockState blockState;
        if (!LLD::legDB->ReadBlock(hashBlock, blockState))
            return 0;

        if (!blockState.IsInMainChain())
            return 0;

        return TAO::Ledger::ChainState::nBestHeight - blockState.nHeight + 1;

    }


    /* Retrieve the number of blocks remaining until transaction outputs are spendable. */
    uint32_t CMerkleTx::GetBlocksToMaturity() const
    {
        if (!(IsCoinBase() || IsCoinStake()))
            return 0;

        int32_t nCoinbaseMaturity = config::fTestNet ? TAO::Ledger::TESTNET_MATURITY_BLOCKS : TAO::Ledger::NEXUS_MATURITY_BLOCKS;
        return std::max((int32_t)0, (int32_t)(nCoinbaseMaturity + (config::fTestNet ? 1 : 20) - GetDepthInMainChain()));
    }


//TODO - These were taken out, added old code back in as reference, need to figure out replacement for usage in wallet.cpp
//    bool CMerkleTx::AcceptToMemoryPool(LLD::CIndexDB& indexdb, bool fCheckInputs)
//    {
//        if (config::fClient)
//        {
//            if (!IsInMainChain() && !ClientConnectInputs())
//                return false;
//
//            return CTransaction::AcceptToMemoryPool(indexdb, false);
//        }
//        else
//        {
//            return CTransaction::AcceptToMemoryPool(indexdb, fCheckInputs);
//        }
//    }
//
//    bool CMerkleTx::AcceptToMemoryPool()
//    {
//        LLD::CIndexDB indexdb("r");
//
//        return AcceptToMemoryPool(indexdb);
//    }

}
