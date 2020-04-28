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

    /* Default Constructor. */
    MerkleTx::MerkleTx()
    : Transaction   ( )
    , hashBlock     (0)
    , vMerkleBranch ( )
    , nIndex        (-1)
    {
    }

    /* Copy Constructor. */
    MerkleTx::MerkleTx(const MerkleTx& tx)
    : Transaction   (tx)
    , hashBlock     (tx.hashBlock)
    , vMerkleBranch (tx.vMerkleBranch)
    , nIndex        (tx.nIndex)
    {
    }


    /* Move Constructor. */
    MerkleTx::MerkleTx(MerkleTx&& tx) noexcept
    : Transaction   (std::move(tx))
    , hashBlock     (std::move(tx.hashBlock))
    , vMerkleBranch (std::move(tx.vMerkleBranch))
    , nIndex        (std::move(tx.nIndex))
    {
    }


    /* Copy assignment. */
    MerkleTx& MerkleTx::operator=(const MerkleTx& tx)
    {
        nVersion      = tx.nVersion;
        nTime         = tx.nTime;
        vin           = tx.vin;
        vout          = tx.vout;
        nLockTime     = tx.nLockTime;
        hashBlock     = tx.hashBlock;
        vMerkleBranch = tx.vMerkleBranch;
        nIndex        = tx.nIndex;

        return *this;
    }


    /* Move assignment. */
    MerkleTx& MerkleTx::operator=(MerkleTx&& tx) noexcept
    {
        nVersion      = std::move(tx.nVersion);
        nTime         = std::move(tx.nTime);
        vin           = std::move(tx.vin);
        vout          = std::move(tx.vout);
        nLockTime     = std::move(tx.nLockTime);
        hashBlock     = std::move(tx.hashBlock);
        vMerkleBranch = std::move(tx.vMerkleBranch);
        nIndex        = std::move(tx.nIndex);

        return *this;
    }


    /* Destructor. */
    MerkleTx::~MerkleTx()
    {
    }


    /* Constructor */
    MerkleTx::MerkleTx(const Transaction& txIn)
    : Transaction   (txIn)
    , hashBlock     (0)
    , vMerkleBranch ( )
    , nIndex        (-1)
    {
    }


    uint32_t MerkleTx::GetDepthInMainChain() const
    {
        if(hashBlock == 0)
            return 0;

        /* Find the block it claims to be in */
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


        TAO::Ledger::BlockState state;

        /* If this transaction has been included in a block then find the block so we can base the maturity calculation on it */
        if(hashBlock != 0)
            LLD::Ledger->ReadBlock(hashBlock, state);

        if(IsCoinBase())
            nMaturity = TAO::Ledger::MaturityCoinBase(state);
        else
            nMaturity = TAO::Ledger::MaturityCoinStake(state);

        /* Legacy mainnet maturity blocks need +20 added so that wallet considers immature for 120 blocks.
         * The NEXUS_MATURITY_LEGACY setting value of 100 was kept for backwards compatability within other parts of code.
         */
        if(nMaturity == TAO::Ledger::NEXUS_MATURITY_LEGACY && !config::fTestNet.load())
            nMaturity += 20;

        if(nDepth >= nMaturity)
            return 0;
        else
            return nMaturity - nDepth;
    }


    /* Checks if this transaction has a valid merkle path.*/
    bool MerkleTx::CheckMerkleBranch(const uint512_t& hashMerkleRoot) const
    {
        /* Generate merkle root from merkle branch. */
        uint512_t hashMerkleCheck = TAO::Ledger::Block::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex);

        return hashMerkleRoot == hashMerkleCheck;
    }


    /* Builds a merkle branch from block state. */
    bool MerkleTx::BuildMerkleBranch(const TAO::Ledger::BlockState& state)
    {
        /* Cache this txid. */
        uint512_t hash = GetHash();

        /* Find the index of this transaction. */
        for(nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
            if(state.vtx[nIndex].second == hash)
                break;

        /* Check for valid index. */
        if(nIndex == state.vtx.size())
            return debug::error(FUNCTION, "transaction not found");

        /* Build merkle branch. */
        vMerkleBranch = state.GetMerkleBranch(state.vtx, nIndex);

        /* NOTE: extra expensive check for testing, consider removing in production */
        uint512_t hashCheck = TAO::Ledger::Block::CheckMerkleBranch(hash, vMerkleBranch, nIndex);
        if(state.hashMerkleRoot != hashCheck)
            return debug::error(FUNCTION, "merkle root mismatch ", hashCheck.SubString());

        return true;
    }


    /* Builds a merkle branch from block state. */
    bool MerkleTx::BuildMerkleBranch(const uint1024_t& hashConfirmed)
    {
        /* Get the confirming block. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashConfirmed, state))
            return debug::error(FUNCTION, "no valid block to generate merkle path");

        /* Set his block's hash. */
        hashBlock     = hashConfirmed;

        return BuildMerkleBranch(state);;
    }


    /* Builds a merkle branch without any block data. */
    bool MerkleTx::BuildMerkleBranch()
    {
        /* Cache this txid. */
        uint512_t hash = GetHash();

        /* Get the confirming block. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hash, state))
            return debug::error(FUNCTION, "no valid block to generate merkle path");

        /* Set his block's hash. */
        hashBlock = state.GetHash();

        return BuildMerkleBranch(state);
    }
}
