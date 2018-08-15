/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_BLOCK_H
#define NEXUS_CORE_INCLUDE_BLOCK_H

namespace TAO
{

    namespace Ledger
    {

        /* Search back for an index given PoW / PoS parameters. */
        const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);


        /* Search back for an index of given Mining Channel. */
        const CBlockIndex* GetLastChannelIndex(const CBlockIndex* pindex, int nChannel);



        /* Check the disk space for the current partition database is stored in. */
        bool CheckDiskSpace(uint64 nAdditionalBytes = 0);


        /* DEPRECATED: Find the block from file and binary position (blk0001.dat), */
        FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode);


        /* DEPRECATED: Add a block to the block file and binary position (blk0001.dat). */
        FILE* AppendBlockFile(unsigned int& nFileRet);


        /* DEPRECATED: Load the Genesis and other blocks from the BDB/LLD Indexes. */
        bool LoadBlockIndex(bool fAllowNew = true);


        /* Create a new block with given parameters and optional coinbase transaction. */
        CBlock CreateNewBlock(Wallet::CReserveKey& reservekey, Wallet::CWallet* pwallet, unsigned int nChannel, unsigned int nID = 0, LLP::Coinbase* pCoinbase = NULL);


        /* Add the Transactions to a Block from the Memroy Pool (TODO: Decide whether to put this in transactions.h/transactions.cpp or leave it here). */
        void AddTransactions(std::vector<CTransaction>& vtx, CBlockIndex* pindexPrev);


        /* Check that the Proof of work is valid for the given Mined block before sending it into the processing queue. */
        bool CheckWork(CBlock* pblock, Wallet::CWallet& wallet, Wallet::CReserveKey& reservekey);


        /* TODO: Remove this where not needed. */
        std::string GetChannelName(int nChannel);

    }
}
