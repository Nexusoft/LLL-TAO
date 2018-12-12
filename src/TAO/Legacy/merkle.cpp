/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/ledger.h>

#include <TAO/Ledger/types/block.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Legacy/types/merkle.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace Legacy
{

    /* Populates the merkle branch for this transaction from its containing block. */
    int CMerkleTx::SetMerkleBranch(const TAO::Ledger::Block* pblock)
    {
        if (config::fClient)
        {
            if (hashBlock == 0)
                return 0;
        }
        else
        {
            TAO::Ledger::Block blockTmp;
            if (pblock == nullptr)
            {
                /* Read the transaction from disk -- Do we still need to do this? */
                TAO::Ledger::Transaction ledgerTx;
                if (!LLD::LedgerDB("r").ReadTx(GetHash(), ledgerTx))
                    return 0;

// TODO - this is old code, above update retrieves ledger transaction, need to get block for it for this process
//                CTxIndex txindex;
//
//                if (!LLD::CIndexDB("r").ReadTxIndex(GetHash(), txindex))
//                    return 0;
//
//                if (!blockTmp.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos))
//                    return 0;
//
//                pblock = &blockTmp;
            }

            // Update the tx's hashBlock
//TODO - This sectino needs to find the transaction within the block to extract its merkle branch (no GetMerkleBranch method now, how is this done?)
//            hashBlock = pblock->GetHash();
//
//            // Locate the transaction
//            for (nIndex = 0; nIndex < (int)pblock->vtx.size(); nIndex++)
//                if (pblock->vtx[nIndex] == *(CTransaction*)this)
//                    break;
//
//            if (nIndex == (int)pblock->vtx.size())
//            {
//                vMerkleBranch.clear();
//                nIndex = -1;
//                debug::log(0, "ERROR: SetMerkleBranch() : couldn't find tx in block\n");
//                return 0;
//            }
//
//            // Fill in merkle branch
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


//TODO - This entire method needs to be rewritten to replace block index, or removed if we are not going to support this method
//    int CMerkleTx::GetDepthInMainChain(CBlockIndex* &pindexRet) const
//    {
//        if (hashBlock == 0 || nIndex == -1)
//            return 0;
//
//        // Find the block it claims to be in
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
//        // Make sure the merkle branch connects to this block
//        if (!fMerkleVerified)
//        {
//            if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
//                return 0;
//
//            fMerkleVerified = true;
//        }
//
//        pindexRet = pindex;
//        return pindexBest->nHeight - pindex->nHeight + 1;
//
//    }


    /* Retrieve the depth in chain of block containing this transaction */
    int CMerkleTx::GetDepthInMainChain() const
    {
//TODO - replace use of CBlockIndex and block.ReadFromDisk
// This method was added to replace old version that took CBlockIndex, see commented code above
//        // Read block header
//        CBlock block;
//        if (!block.ReadFromDisk(pos.nFile, pos.nBlockPos, false))
//            return 0;
//
//        // Find the block in the index
//        map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.GetHash());
//
//        if (mi == mapBlockIndex.end())
//            return 0;
//
//        CBlockIndex* pindex = (*mi).second;
//
//        if (!pindex || !pindex->IsInMainChain())
//            return 0;
//
//        return 1 + nBestHeight - pindex->nHeight;

        return 0; //placeholder
    }


    /* Retrieve the number of blocks remaining until transaction outputs are spendable. */
    int CMerkleTx::GetBlocksToMaturity() const
    {
        if (!(IsCoinBase() || IsCoinStake()))
            return 0;

//TODO Use a setting in place of hard-coded value
        return std::max(0, (100 + (config::fTestNet ? 1 : 20)) - GetDepthInMainChain());
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
