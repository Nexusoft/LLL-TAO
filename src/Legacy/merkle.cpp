/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/legacy.h>

#include <TAO/Ledger/include/chain.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>

#include <Legacy/types/merkle.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace Legacy
{

    /* Populates the merkle branch for this transaction from its containing block. */
    uint32_t CMerkleTx::SetMerkleBranch(const TAO::Ledger::TritiumBlock* pblock)
    {
        if (config::fClient)
        {
            if (hashBlock == 0)
                return 0;
        }
        else
        {
            TAO::Ledger::TritiumBlock containingBlock;

            if (pblock == nullptr)
            {
                /* Read the transaction from disk -- Do we still need to do this? */
                Legacy::Transaction legacyTx;
                if (!LLD::LegacyDB("r").ReadTx(GetHash(), legacyTx))
                    return 0;

                if (hashBlock > 0)
                {
                    /* Already know the block hash for the block containing this transaction */
                    TAO::Ledger::BlockState containingBlockState;
                    if (!TAO::Ledger::GetState(TAO::Ledger::ChainState::hashBestChain, containingBlockState))
                    {
                        debug::log(0, "Error: CMerkleTx::SetMerkleBranch() containing block for transaction");
                        return 0;
                    }

                    containingBlock = containingBlockState.blockThis;
                }
                else
                {
                    /* Don't know containing block hash */
//TODO - how to find containing block (if any) when only know transaction hash
//                CTxIndex txindex;
//
//                if (!LLD::CIndexDB("r").ReadTxIndex(GetHash(), txindex))
//                    return 0;
//
//                if (!blockTmp.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos))
//                    return 0;
                }

                pblock = &containingBlock;
            }

            /* Update the tx's hashBlock */
            hashBlock = pblock->GetHash();

            /* Locate the transaction */
            for (nIndex = 0; nIndex < (int)pblock->vtx.size(); nIndex++)
            {
//TODO - need to verify correct usage of legacy transaction here...what is in vtx?
//Probably should not compare tx pointer, but rather tx hash unless it must be same exact instance
//                if (pblock->vtx[nIndex] == *(Transaction*)this)
//                    break;
            }

            if (nIndex >= (int)pblock->vtx.size())
            {
                vMerkleBranch.clear();
                nIndex = -1;
                debug::log(0, "ERROR: SetMerkleBranch() : couldn't find tx in block");
                return 0;
            }

            // Fill in merkle branch
//TODO - This will be defined?             
//            vMerkleBranch = pblock->GetMerkleBranch(nIndex);
        }

        // Is the tx in a block that's in the main chain
//TODO - This checks main chain to return depth, supported by BlockState. Need to update support
//        map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
//
//        if (mi == mapBlockIndex.end())
//            return 0;
//
//        CBlockIndex* pindex = (*mi).second;
//
//        if (!pindex || !pindex->IsInMainChain())
//            return 0;
//
//        return pindexBest->nHeight - pindex->nHeight + 1;

        return 0;  //temporary until update this method           
    }


    uint32_t CMerkleTx::GetDepthInMainChain() const
    {
        if (hashBlock == 0 || nIndex == -1)
            return 0;

        // Find the block it claims to be in
        TAO::Ledger::BlockState blockState;
        if (!TAO::Ledger::GetState(hashBlock, blockState))
            return 0;

        if (!blockState.IsInMainChain())
            return 0;

        return TAO::Ledger::ChainState::nBestHeight - blockState.blockThis.nHeight + 1;

    }


    /* Retrieve the number of blocks remaining until transaction outputs are spendable. */
    uint32_t CMerkleTx::GetBlocksToMaturity() const
    {
        if (!(IsCoinBase() || IsCoinStake()))
            return 0;

        uint32_t nMaturity = config::fTestNet ? TAO::Ledger::TESTNET_MATURITY_BLOCKS : TAO::Ledger::NEXUS_MATURITY_BLOCKS;
        return std::max((uint32_t)0, nMaturity - GetDepthInMainChain());
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
