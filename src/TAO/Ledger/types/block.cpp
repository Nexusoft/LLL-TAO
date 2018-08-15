/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#include "../Wallet/wallet.h"

#include "include/block.h"

#include "../include/trust.h"
#include "../include/supply.h"
#include "../include/prime.h"
#include "../include/difficulty.h"
#include "../include/dispatch.h"
#include "../include/checkpoints.h"
#include "../include/manager.h"

#include "../../Util/include/ui_interface.h"

#include "../../LLP/include/mining.h"
#include "../../LLP/include/message.h"
#include "../../LLP/include/legacy.h"

#include "../../LLD/include/index.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

/* Three Dimensional Blockchain
 * 
 * --------- BLK ----------
 * ||||||||||||||||||||||||
 * ||||||||||||||||||||||||
 * ||||||||||||||||||||||||
 * ||||||||||||||||||||||||
 * --------- BLK ----------
 * 
 * Horizonal Locking along with Vertial Block locking
 * LLD Database PArtitioning automatically integrated with the trust
 * 
 * An LLD route from the LLD keychain can be used in order to determine the integrity of data
 * One can double check this with the rest of the ndoes to ensure that they havef a clear idea
 * of what is happening and that everyone has agreed upon the same data reducing the ability for a sybil attack
 * since to sybil attack you would need to hijack the trust routes and the time it took to creat ethem\
 * 
 * Operational time of a route and a data route and nubmer of requests to that data are very importatnt
 * 
 * 
 * State channels or | move
 * 
 * Trust can validate a path of the state channels.
 * 
 * Use multi-level locks
 * 
 * A BLK lock is based on the security of the lock
 * Security of the lock is built up by twin blocks or siblings
 * 
 * These can agree on a set of transaction lock data that as long as
 * it satisfies the transaction lock it contributes to lock at that time
 * 
 * This can minimal all effects of orphans in the cube-chain.
 */

namespace Core
{

	const CBlockIndex* GetLastChannelIndex(const CBlockIndex* pindex, int nChannel)
	{
		while (pindex && pindex->pprev && (pindex->GetChannel() != nChannel))
			pindex = pindex->pprev;
			
		return pindex;
	}
			
			
	/* TODO: Replace this with an LLD instance. */
	bool CBlock::WriteToDisk(unsigned int& nFileRet, unsigned int& nBlockPosRet)
	{
		// Open history file to append
		CAutoFile fileout = CAutoFile(AppendBlockFile(nFileRet), SER_DISK, DATABASE_VERSION);
		if (!fileout)
			return error("CBlock::WriteToDisk() : AppendBlockFile failed");

		// Write index header
		unsigned char pchMessageStart[4];
		memcpy(pchMessageStart, (fTestNet ? LLP::MESSAGE_START_TESTNET : LLP::MESSAGE_START_MAINNET), sizeof(pchMessageStart));
		
		unsigned int nSize = fileout.GetSerializeSize(*this);
		fileout << FLATDATA(pchMessageStart) << nSize;

		// Write block
		long fileOutPos = ftell(fileout);
		if (fileOutPos < 0)
			return error("CBlock::WriteToDisk() : ftell failed");
		nBlockPosRet = fileOutPos;
		fileout << *this;

		// Flush stdio buffers and commit to disk before returning
		fflush(fileout);
		if ((nBestHeight+1) % 500 == 0)
		{
	#ifdef WIN32
			_commit(_fileno(fileout));
	#else
			fsync(fileno(fileout));
	#endif
		}

		return true;
	}

		
	/* TODO: Replace this by an LLD instance. */
	bool CBlock::ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions)
	{
		SetNull();

		// Open history file to read
		CAutoFile filein = CAutoFile(OpenBlockFile(nFile, nBlockPos, "rb"), SER_DISK, DATABASE_VERSION);
		if (!filein)
			return error("CBlock::ReadFromDisk() : OpenBlockFile failed");
		if (!fReadTransactions)
			filein.nType |= SER_BLOCKHEADERONLY;

		// Read block
		try {
			filein >> *this;
		}
		catch (std::exception &e) {
			return error("%s() : deserialize or I/O error", __PRETTY_FUNCTION__);
		}

		return true;
	}
	

	void CBlock::print() const
	{
		printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nChannel = %u, nHeight = %u, nNonce=%" PRIu64 ", vtx=%d, vchBlockSig=%s)\n",
				GetHash().ToString().substr(0,20).c_str(),
				nVersion,
				hashPrevBlock.ToString().substr(0,20).c_str(),
				hashMerkleRoot.ToString().substr(0,10).c_str(),
				nTime, nBits, nChannel, nHeight, nNonce,
				vtx.size(),
				HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
		for (unsigned int i = 0; i < vtx.size(); i++)
		{
			printf("  ");
			vtx[i].print();
		}
		printf("  vMerkleTree: ");
		for (unsigned int i = 0; i < vMerkleTree.size(); i++)
			printf("%s ", vMerkleTree[i].ToString().substr(0,10).c_str());
		printf("\n");
	}


	/* This method can be easily replaced with an LLD method. */
	bool CBlock::ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions)
	{
		if (!fReadTransactions)
		{
			*this = pindex->GetBlockHeader();
			return true;
		}
		
		if (!ReadFromDisk(pindex->nFile, pindex->nBlockPos, fReadTransactions))
			return false;
		if (GetHash() != pindex->GetBlockHash())
			return error("CBlock::ReadFromDisk() : GetHash() doesn't match index");
		return true;
	}


	/* Time update should not be contigent upon the memory map of the previous block.
	 * 
	 * Need this to be connected to the LLD so that these states can be erified either through a blk pool cache of recent blocks
	 * or it can be read from disk from the LLD that will enable its index to be found easily. 
	 */
	void CBlock::UpdateTime() { nTime = std::max((uint64)mapBlockIndex[hashPrevBlock]->GetBlockTime() + 1, UnifiedTimestamp()); }

	
	/* Disconnect block could be seen as the same as following comment on ConnectBlock. */
	bool CBlock::DisconnectBlock(LLD::CIndexDB& indexdb)
	{
		// Disconnect in reverse order
		for (int i = vtx.size() - 1; i >= 0; i--)
			if (!vtx[i].DisconnectInputs(indexdb))
				return false;

		// Nexus: clean up wallet after disconnecting coinstake
		for(auto tx : vtx)
			SyncWithWallets(tx, this, false, false);

		return true;
	}

	
	
	/* Connect block needs some serious work
	 * 
	 * This method only writes the disk index of transaction as tx.ConnectInputs should do
	 * 
	 * But it should be done in a much more efficient way as having a transaction flag from
	 * the manager and blk pool be the ones that determine this.
	 * 
	 * TODO: Depricate this method.
	 */
	bool CBlock::ConnectBlock(LLD::CIndexDB& indexdb)
	{

		// Do not allow blocks that contain transactions which 'overwrite' older transactions,
		// unless those are already completely spent.
		// If such overwrites are allowed, coinbases and transactions depending upon those
		// can be duplicated to remove the ability to spend the first instance -- even after
		// being sent to another address.
        
		// TODO: Check the transactions being spent. don't let a transaction be overwritten.

		//// issue here: it doesn't know the version
		unsigned int nTxPos = pindex->nBlockPos + ::GetSerializeSize(CBlock(), SER_DISK, DATABASE_VERSION) - (2 * GetSizeOfCompactSize(0)) + GetSizeOfCompactSize(vtx.size());

		map<uint512, CTxIndex> mapQueuedChanges;
		int64 nFees = 0;
		int64 nValueIn = 0;
		int64 nValueOut = 0;
		unsigned int nSigOps = 0;
		unsigned int nIterator = 0;
		for(auto tx : vtx)
		{
			nSigOps += tx.GetLegacySigOpCount();
			if (nSigOps > MAX_BLOCK_SIGOPS)
				return error("ConnectBlock() : too many sigops");

			CDiskTxPos posThisTx(pindex->nFile, pindex->nBlockPos, nTxPos);
			nTxPos += ::GetSerializeSize(tx, SER_DISK, DATABASE_VERSION);

			MapPrevTx mapInputs;
			if (tx.IsCoinBase())
				nValueOut += tx.GetValueOut();
			else
			{
				bool fInvalid;
				if (!tx.FetchInputs(indexdb, mapQueuedChanges, true, false, mapInputs, fInvalid))
					return error("ConnectBlock() : Failed to Connect Inputs.");


				// Add in sigops done by pay-to-script-hash inputs;
				// this is to prevent a "rogue miner" from creating
				// an incredibly-expensive-to-validate block.
				nSigOps += tx.TotalSigOps(mapInputs);
				if (nSigOps > MAX_BLOCK_SIGOPS)
					return error("ConnectBlock() : too many sigops");

				int64 nTxValueIn = tx.GetValueIn(mapInputs);
				int64 nTxValueOut = tx.GetValueOut();
				
				nValueIn += nTxValueIn;
				nValueOut += nTxValueOut;
				
				if (!tx.ConnectInputs(indexdb, mapInputs, mapQueuedChanges, posThisTx, pindex, true, false))
					return error("ConnectBlock() : Failed to Connect Inputs...");
			}

			nIterator++;
			mapQueuedChanges[tx.GetHash()] = CTxIndex(posThisTx, tx.vout.size());
		}
		

		// track money supply and mint amount info
		pindex->nMint = nValueOut - nValueIn;
		pindex->nMoneySupply = (pindex->pprev ? pindex->pprev->nMoneySupply : 0) + nValueOut - nValueIn;
		
		if(GetArg("-verbose", 0) >= 2)
			printf("Generated %f Nexus\n", (double) pindex->nMint / COIN);
		
		if (!indexdb.WriteBlockIndex(CDiskBlockIndex(pindex)))
			return error("Connect() : WriteBlockIndex for pindex failed");

		// Write queued txindex changes
		for (map<uint512, CTxIndex>::iterator mi = mapQueuedChanges.begin(); mi != mapQueuedChanges.end(); ++mi)
		{
			if (!indexdb.UpdateTxIndex((*mi).first, (*mi).second))
				return error("ConnectBlock() : UpdateTxIndex failed");
		}

		// Nexus: fees are not collected by miners as in bitcoin
		// Nexus: fees are destroyed to compensate the entire network
		if(GetArg("-verbose", 0) >= 2)
			printf("ConnectBlock() : destroy=%s nFees=%" PRI64d "\n", FormatMoney(nFees).c_str(), nFees);

		// Update block index on disk without changing it in memory.
		// The memory index structure will be changed after the db commits.
		if (pindex->pprev)
		{
			CDiskBlockIndex blockindexPrev(pindex->pprev);
			blockindexPrev.hashNext = pindex->GetBlockHash();
			
			if (!indexdb.WriteBlockIndex(blockindexPrev))
				return error("ConnectBlock() : WriteBlockIndex for blockindexPrev failed");
		}

		// Watch for transactions paying to me
		for(auto tx : vtx)
			SyncWithWallets(tx, this, true);

		return true;
	}
	
	
	/** Verify Work: Verify the Claimed Proof of Work amount for the Two Mining Channels. **/
	bool CBlock::VerifyWork() const
	{
		/** Check the Prime Number Proof of Work for the Prime Channel. **/
		if(GetChannel() == 1)
		{
			unsigned int nPrimeBits = GetPrimeBits(GetPrime());
			if (nPrimeBits < bnProofOfWorkLimit[1])
				return error("VerifyWork() : prime below minimum work");
			
			if(nBits > nPrimeBits)
				return error("VerifyWork() : prime cluster below target");
				
			return true;
		}
		
		CBigNum bnTarget;
		bnTarget.SetCompact(nBits);

		/** Check that the Hash is Within Range. **/
		if (bnTarget <= 0 || bnTarget > bnProofOfWorkLimit[2])
			return error("VerifyWork() : proof-of-work hash not in range");

		
		/** Check that the Hash is within Proof of Work Amount. **/
		if (GetHash() > bnTarget.getuint1024())
			return error("VerifyWork() : proof-of-work hash below target");

		return true;
	}



	
	/* Block Signatures in Tritium need to seal a signature chain of each block.
	 * 
	 * The baseline of this needs to be done with the Manager and LLP integration
	 * Beyond this the sinagure chain needs to create trust as a miner or holder.
	 * 
	 * This also needs to contain a slow decay in reward to incetivze trust.
	 */
	bool CBlock::SignBlock(const Wallet::CKeyStore& keystore)
	{
		vector<std::vector<unsigned char> > vSolutions;
		Wallet::TransactionType whichType;
		const CTxOut& txout = vtx[0].vout[0];

		if (!Solver(txout.scriptPubKey, whichType, vSolutions))
			return false;
		
		if (whichType == Wallet::TX_PUBKEY)
		{
			// Sign
			const std::vector<unsigned char>& vchPubKey = vSolutions[0];
			Wallet::CKey key;
			if (!keystore.GetKey(LLC::HASH::SK256(vchPubKey), key))
				return false;
			if (key.GetPubKey() != vchPubKey)
				return false;
			
			return key.Sign((nVersion >= 4) ? SignatureHash() : GetHash(), vchBlockSig, 1024);
		}
		
		return false;
	}

	
	// Nexus: check block signature
	bool CBlock::CheckBlockSignature() const
	{
		if (GetHash() == hashGenesisBlock)
			return vchBlockSig.empty();

		vector<std::vector<unsigned char> > vSolutions;
		Wallet::TransactionType whichType;
		const CTxOut& txout = vtx[0].vout[0];

		if (!Solver(txout.scriptPubKey, whichType, vSolutions))
			return false;
		if (whichType == Wallet::TX_PUBKEY)
		{
			const std::vector<unsigned char>& vchPubKey = vSolutions[0];
			Wallet::CKey key;
			if (!key.SetPubKey(vchPubKey))
				return false;
			if (vchBlockSig.empty())
				return false;
			return key.Verify((nVersion >= 4) ? SignatureHash() : GetHash(), vchBlockSig, 1024);
		}
		return false;
	}


	bool CheckDiskSpace(uint64 nAdditionalBytes)
	{
		uint64 nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

		// Check for 15MB because database could create another 10MB log file at any time
		if (nFreeBytesAvailable < (uint64)15000000 + nAdditionalBytes)
		{
			fShutdown = true;
			
			return false;
		}
		return true;
	}

	
	FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
	{
		if (nFile == -1)
			return NULL;
		FILE* file = fopen((GetDataDir() / strprintf("blk%04d.dat", nFile)).string().c_str(), pszMode);
		if (!file)
			return NULL;
		if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
		{
			if (fseek(file, nBlockPos, SEEK_SET) != 0)
			{
				fclose(file);
				return NULL;
			}
		}
		return file;
	}

	
	/* TODO: This shouldn't be held by CBlockIndex
	 * It needs to be held in an LLD by the key and then the memory won't be so required.
	 * 
	 * Make sure to have the chain states recorded in the LLD so that we can know exactly what the blockchain state is at.
	 */
	unsigned int nCurrentBlockFile = 1;
	FILE* AppendBlockFile(unsigned int& nFileRet)
	{
		nFileRet = 0;
		loop
		{
			FILE* file = OpenBlockFile(nCurrentBlockFile, 0, "ab");
			if (!file)
				return NULL;
			if (fseek(file, 0, SEEK_END) != 0)
				return NULL;
			
			// FAT32 filesize max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
			if (ftell(file) < 0x7F000000 - MAX_SIZE)
			{
				nFileRet = nCurrentBlockFile;
				return file;
			}
			fclose(file);
			nCurrentBlockFile++;
		}
	}


	/* This function should be changed around to be:
	 * CreateGenesisBlock() that is a default that is checked to the database disks.
	 * 
	 * A block hash is like a work hardened checksum of data to prove data integrity.
	 */
	bool LoadBlockIndex(bool fAllowNew)
	{
		if (fTestNet)
		{
			hashGenesisBlock = hashGenesisBlockTestNet;
			nCoinbaseMaturity = 10;
		}

		if(GetArg("-verbose", 0) >= 0)
			printf("%s Network: genesis=%s nBitsLimit=0x%08x nBitsInitial=0x%08x nCoinbaseMaturity=%d\n",
			   fTestNet? "Test" : "Nexus", hashGenesisBlock.ToString().substr(0, 20).c_str(), bnProofOfWorkLimit[0].GetCompact(), bnProofOfWorkStart[0].GetCompact(), nCoinbaseMaturity);

		if (!fAllowNew)
			return false;


		const char* pszTimestamp = "Silver Doctors [2-19-2014] BANKER CLEAN-UP: WE ARE AT THE PRECIPICE OF SOMETHING BIG";

		CTransaction txNew;
		txNew.nTime = 1409456199;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = Wallet::CScript() << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].SetEmpty();
			
		CBlock block;
		block.vtx.push_back(txNew);
		block.hashPrevBlock = 0;
		block.hashMerkleRoot = block.BuildMerkleTree();
		block.nVersion = 1;
		block.nHeight  = 0;
		block.nChannel = 2;
		block.nTime    = 1409456199;
		block.nBits    = bnProofOfWorkLimit[2].GetCompact();
		block.nNonce   = fTestNet ? 122999499 : 2196828850;

		assert(block.hashMerkleRoot == uint512("0x8a971e1cec5455809241a3f345618a32dc8cb3583e03de27e6fe1bb4dfa210c413b7e6e15f233e938674a309df5a49db362feedbf96f93fb1c6bfeaa93bd1986"));
			
		CBigNum target;
		target.SetCompact(block.nBits);
		block.print();
			
		
		/* Check the Genesis Block Data to the Hard Coded Genesis Hash. */
		if(block.GetHash() != hashGenesisBlock)
			return error("LoadBlockIndex() : genesis hash does not match");

		
		/** Initialize Block Index Database. **/
		LLD::CIndexDB indexdb("cr");
		if (!indexdb.LoadBlockIndex() || mapBlockIndex.empty())
		{
			/** Write the New Genesis to Disk. **/
			unsigned int nFile;
			unsigned int nBlockPos;
			if (!block.WriteToDisk(nFile, nBlockPos))
				return error("LoadBlockIndex() : writing genesis block to disk failed");
			
			pindexBest = new CBlockIndex(nFile, nBlockPos, block);
			if (!pindexBest)
				return error("LoadBlockIndex() : new CBlockIndex failed");
		
			if(!pManager->blkPool.Index(block, pindexBest))
				return error("LoadBlockIndex() : genesis block not accepted");
			
			/* Commit the Genesis to disk. */
			LLD::CIndexDB indexdb("r+");
			indexdb.TxnBegin();
			if(!indexdb.WriteBlockIndex(CDiskBlockIndex(pindexBest)))
				return error("LoadBlockIndex() : failed to write index to disk");
			
			/* Write the Best Chain to the Index Database LLD. */
			if (!indexdb.WriteHashBestChain(pindexBest->GetBlockHash()))
				return error("LoadBlockIndex() : WriteHashBestChain failed");
			
			/* Add the genesis to the block pool. */
			if(!pManager->blkPool.Add(block.GetHash(), block, pManager->blkPool.CONNECTED))
				return error("LoadBlockIndex() : Failed to add to pool");
			
			/* Commit to the LLD. */
			indexdb.TxnCommit();
		}
		else
		{
			
			/* Read the block from the disk. */
			CBlock blk;
			if (!blk.ReadFromDisk(pindexBest))
				return error("LoadBlockIndex() : failed to read best block");
		
			/* Add the object to the block pool. */
			if(!pManager->blkPool.Add(blk.GetHash(), blk, pManager->blkPool.CONNECTED))
				return error("LoadBlockIndex() : Failed to add best block");

		}
		
		
		/* Initialize the Chain Stats. */
		hashBestChain = pindexBest->GetBlockHash();
		nBestHeight = pindexBest->nHeight;
		nBestChainTrust = pindexBest->nChainTrust;
		nTimeBestReceived = UnifiedTimestamp();
		

		return true;
	}
	
	
	/** Constructs a new block **/
	static int nCoinbaseCounter = 0;
	CBlock CreateNewBlock(Wallet::CReserveKey& reservekey, Wallet::CWallet* pwallet, unsigned int nChannel, unsigned int nID, LLP::Coinbase* pCoinbase)
	{
		CBlock cBlock;
        cBlock.SetNull();
		
		/** Create the block from Previous Best Block. **/
		CBlockIndex* pindexPrev = pindexBest;
		
		/** Modulate the Block Versions if they correspond to their proper time stamp **/
		if(UnifiedTimestamp() >= (fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
			cBlock.nVersion = fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION; // --> New Block Versin Activation Switch
		else
			cBlock.nVersion = fTestNet ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1;
		
		/** Create the Coinbase / Coinstake Transaction. **/
		CTransaction txNew;
		txNew.vin.resize(1);
		txNew.vin[0].prevout.SetNull();
		
		/** Set the First output to Reserve Key. **/
		txNew.vout.resize(1);
		

		/** Create the Coinstake Transaction if on Proof of Stake Channel. **/
		if (nChannel == 0)
		{
			/** Mark the Coinstake Transaction with First Input Byte Signature. **/
			txNew.vin[0].scriptSig.resize(8);
			txNew.vin[0].scriptSig[0] = 1;
			txNew.vin[0].scriptSig[1] = 2;
			txNew.vin[0].scriptSig[2] = 3;
			txNew.vin[0].scriptSig[3] = 5;
			txNew.vin[0].scriptSig[4] = 8;
			txNew.vin[0].scriptSig[5] = 13;
			txNew.vin[0].scriptSig[6] = 21;
			txNew.vin[0].scriptSig[7] = 34;
			
			/** Update the Coinstake Timestamp. **/
			txNew.nTime = pindexPrev->GetBlockTime() + 1;
			
			/** Check the Trust Keys. **/
			if(!cTrustPool.HasTrustKey(pindexPrev->GetBlockTime()))
				txNew.vout[0].scriptPubKey << reservekey.GetReservedKey() << Wallet::OP_CHECKSIG;
			else
			{
				uint576 cKey;
				cKey.SetBytes(cTrustPool.vchTrustKey);
				
				txNew.vout[0].scriptPubKey << cTrustPool.vchTrustKey << Wallet::OP_CHECKSIG;
				txNew.vin[0].prevout.n = 0;
				txNew.vin[0].prevout.hash = cTrustPool.Find(cKey).GetHash();
			}
				
		}
		
		/** Create the Coinbase Transaction if the Channel specifies. **/
		else
		{
			/** Set the Coinbase Public Key. **/
			txNew.vout[0].scriptPubKey << reservekey.GetReservedKey() << Wallet::OP_CHECKSIG;

			/** Set the Proof of Work Script Signature. **/
			txNew.vin[0].scriptSig = (Wallet::CScript() << nID * 513513512151);
			
			/** Customized Coinbase Transaction if Submitted. **/
			if(pCoinbase)
			{
				
				/** Dummy Transaction to Allow the Block to be Signed by Pool Wallet. [For Now] **/
				txNew.vout[0].nValue = pCoinbase->nPoolFee;
				
				unsigned int nTx = 1;
				txNew.vout.resize(pCoinbase->vOutputs.size() + 1);
				for(std::map<std::string, uint64>::iterator nIterator = pCoinbase->vOutputs.begin(); nIterator != pCoinbase->vOutputs.end(); nIterator++)
				{
					/** Set the Appropriate Outputs. **/
					txNew.vout[nTx].scriptPubKey.SetNexusAddress(nIterator->first);
					txNew.vout[nTx].nValue = nIterator->second;
					
					nTx++;
				}
				
				int64 nMiningReward = 0;
				for(int nIndex = 0; nIndex < txNew.vout.size(); nIndex++)
					nMiningReward += txNew.vout[nIndex].nValue;
				
			}
			else
				txNew.vout[0].nValue = GetCoinbaseReward(pindexPrev, nChannel, 0);
				
			/** Reset the Coinbase Transaction Counter. **/
			if(nCoinbaseCounter >= 13)
				nCoinbaseCounter = 0;
						
			/** Set the Proper Addresses for the Coinbase Transaction. **/
			txNew.vout.resize(txNew.vout.size() + 2);
			txNew.vout[txNew.vout.size() - 2].scriptPubKey.SetNexusAddress(fTestNet ? TESTNET_DUMMY_ADDRESS : AMBASSADOR_ADDRESSES[nCoinbaseCounter]);
			txNew.vout[txNew.vout.size() - 1].scriptPubKey.SetNexusAddress(fTestNet ? TESTNET_DUMMY_ADDRESS : DEVELOPER_ADDRESSES[nCoinbaseCounter]);
			
			/** Set the Proper Coinbase Output Amounts for Recyclers and Developers. **/
			txNew.vout[txNew.vout.size() - 2].nValue = GetCoinbaseReward(pindexPrev, nChannel, 1);
			txNew.vout[txNew.vout.size() - 1].nValue = GetCoinbaseReward(pindexPrev, nChannel, 2);
					
			nCoinbaseCounter++;
		}
		
		/** Add our Coinbase / Coinstake Transaction. **/
		cBlock.vtx.push_back(txNew);
		
		/** Add in the Transaction from Memory Pool only if it is not a Genesis. **/
		if(nChannel > 0)
			AddTransactions(cBlock.vtx, pindexPrev);
			
		/** Populate the Block Data. **/
		cBlock.hashPrevBlock  = pindexPrev->GetBlockHash();
		cBlock.hashMerkleRoot = cBlock.BuildMerkleTree();
		cBlock.nChannel       = nChannel;
		cBlock.nHeight        = pindexPrev->nHeight + 1;
		cBlock.nBits          = GetNextTargetRequired(pindexPrev, cBlock.GetChannel());
		cBlock.nNonce         = 1;
		
		cBlock.UpdateTime();

		return cBlock;
	}
	
	
	void AddTransactions(std::vector<CTransaction>& vtx, CBlockIndex* pindexPrev)
	{
		/** Collect Memory Pool Transactions into Block. **/
		std::vector<CTransaction> vRemove;
		int64 nFees = 0;
		{
			LLD::CIndexDB indexdb("r");
			

			/* Collect the list of Verified transactions in the txpool. */
			std::vector<CTransaction> vTransactions;
			if(!pManager->txPool.Get(pManager->txPool.ACCEPTED, vTransactions))
				return;
			
			
			/* Loop through the transactions selected from the txpool to establish priority. */
			std::multimap<double, CTransaction> mapPriority;
			for(auto tx : vTransactions)
			{

				/* Double Check the Transaction data like the Memory Pool. */
				if (tx.IsCoinBase() || tx.IsCoinStake() || !tx.IsFinal())
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Transaction Is Coinbase/Coinstake or Not Final %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					continue;
				}

				
				/* Establish Prioity based on previous transaction. */
				double dPriority = 0;
				for(auto txin : tx.vin)
				{
					CTransaction txPrev;
					CTxIndex txindex;
					
					/* If transaction somehow missed orphan checks and doesn't exist on disk, set its status in txpool. */
					if (!txPrev.ReadFromDisk(indexdb, txin.prevout, txindex))
					{
						pManager->txPool.SetState(tx.GetHash(), pManager->txPool.ORPHANED);
						
						continue;
					}
					
					/* Calculate Priority. */
					int64 nValueIn = txPrev.vout[txin.prevout.n].nValue;
					int nConf = txindex.GetDepthInMainChain();
					dPriority += (double) nValueIn * nConf;

					if(GetArg("-verbose", 0) >= 2)
						printf("priority     nValueIn=%-12" PRI64d " nConf=%-5d dPriority=%-20.1f\n", nValueIn, nConf, dPriority);
				}

				
				/* Priority is sum(valuein * age) / txsize */
				dPriority /= ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
				mapPriority.insert(make_pair(-dPriority, tx));

				if(GetArg("-verbose", 0) >= 2)
					printf("priority %-20.1f %s\n%s", dPriority, tx.GetHash().ToString().substr(0,10).c_str(), tx.ToString().c_str());
			}

			
			// Collect transactions into block
			std::map<uint512, CTxIndex> mapTestPool;
			uint64 nBlockSize = 1000;
			uint64 nBlockTx = 0;
			int nBlockSigOps = 100;
			while (!mapPriority.empty())
			{
				
				// Take highest priority transaction off priority queue
				CTransaction tx = (*mapPriority.begin()).second;
				mapPriority.erase(mapPriority.begin());

				
				// Size limits
				unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
				if (nBlockSize + nTxSize >= MAX_BLOCK_SIZE_GEN)
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Block Size Limits Reached on Transaction %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					continue;
				}

				
				/* Make sure the Transaction doesn't exceed the maximum number of operations. */
				unsigned int nTxSigOps = tx.GetLegacySigOpCount();
				if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Too Many Legacy Signature Operations %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					continue;
				}
				

				// Timestamp limit NOTE: This should be checked in the txpool
				if (tx.nTime > UnifiedTimestamp() + MAX_UNIFIED_DRIFT)
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Transaction Time Too Far in Future %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					continue;
				}
				

				/* Prevent Free Transactions for Now NOTE: Change this for time lock activation. */
				int64 nMinFee = tx.GetMinFee(nBlockSize, false, GMF_BLOCK);

				
				// Connecting shouldn't fail due to dependency on other memory pool transactions
				// because we're already processing them in order of dependency
				std::map<uint512, CTxIndex> mapTestPoolTmp(mapTestPool);
				MapPrevTx mapInputs;
				bool fInvalid;
				if (!tx.FetchInputs(indexdb, mapTestPoolTmp, false, true, mapInputs, fInvalid))
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Failed to get Inputs %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					vRemove.push_back(tx);
					continue;
				}

				int64 nTxFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();
				if (nTxFees < nMinFee)
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Not Enough Fees %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					vRemove.push_back(tx);
					continue;
				}
				
				nTxSigOps += tx.TotalSigOps(mapInputs);
				if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Too many P2SH Signature Operations %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					vRemove.push_back(tx);
					continue;
				}

				if (!tx.ConnectInputs(indexdb, mapInputs, mapTestPoolTmp, CDiskTxPos(1,1,1), pindexPrev, false, true))
				{
					if(GetArg("-verbose", 0) >= 2)
						printf("AddTransactions() : Failed to Connect Inputs %s\n", tx.GetHash().ToString().substr(0, 10).c_str());
						
					vRemove.push_back(tx);
					continue;
				}
				
				mapTestPoolTmp[tx.GetHash()] = CTxIndex(CDiskTxPos(1,1,1), tx.vout.size());
				swap(mapTestPool, mapTestPoolTmp);

				
				// Added
				vtx.push_back(tx);
				nBlockSize += nTxSize;
				++nBlockTx;
				nBlockSigOps += nTxSigOps;
				nFees += nTxFees;
			}

			nLastBlockTx = nBlockTx;
			nLastBlockSize = nBlockSize;
			if(GetArg("-verbose", 0) >= 2)
				printf("AddTransactions(): total size %lu\n", nBlockSize);

		}
		
		/* TODO: Remove this, not needed if txpool checks are thorough. */
		for(auto tx : vRemove)
		{
			printf("AddTransactions() : removed invalid tx %s from mempool\n", tx.GetHash().ToString().substr(0, 10).c_str());
			pManager->txPool.Remove(tx.GetHash());
		}
	}


	/** Work Check Before Submit. This checks the work as a miner, a lot more conservatively than the network will check it
		to ensure that you do not submit a bad block. **/
	bool CheckWork(CBlock* pblock, Wallet::CWallet& wallet, Wallet::CReserveKey& reservekey)
	{
		uint1024 hash = pblock->GetHash();
		uint1024 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint1024();
		
		if(pblock->GetChannel() > 0 && !pblock->VerifyWork())
			return error("CheckWork : Work not meeting Target");
		
		
		if(pblock->GetChannel() == 1)
			printf("  prime cluster verified of size %f\n", GetDifficulty(pblock->nBits, 1));
		else
		{
			printf("  hash:   %s  \n", hash.ToString().substr(0, 30).c_str());
			printf("  target: %s\n", hashTarget.ToString().substr(0, 30).c_str());
		}
		
		printf("%s ", DateTimeStrFormat(UnifiedTimestamp()).c_str());
		//printf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());

		
		{
			if (pblock->hashPrevBlock != hashBestChain && mapBlockIndex[pblock->hashPrevBlock]->GetChannel() == pblock->GetChannel())
				return error("CheckWork : Generated work is Stale (Orphaned)");


			/** Process the Block to see if it gets Accepted into Blockchain. **/
			if (!pManager->blkPool.Process(*pblock))
				return error("CheckWork : ProcessBlock, block not accepted\n");
				
			/** Keep the Reserve Key only if it was used in a block. **/
			reservekey.KeepKey();
		}

		return true;
	}
	


	bool CBlockIndex::EraseBlockFromDisk()
	{
		// Open history file
		CAutoFile fileout = CAutoFile(OpenBlockFile(nFile, nBlockPos, "rb+"), SER_DISK, DATABASE_VERSION);
		if (!fileout)
			return false;

		// Overwrite with empty null block
		CBlock block;
		block.SetNull();
		fileout << block;

		return true;
	}


	std::string CBlockIndex::ToString() const
	{
		return strprintf("CBlockIndex(nprev=%08x, pnext=%08x, nFile=%d, nBlockPos=%-6d nHeight=%d, nMint=%s, nMoneySupply=%s, nFlags=(%s), merkle=%s, hashBlock=%s)",
			pprev, pnext, nFile, nBlockPos, nHeight,
			FormatMoney(nMint).c_str(), FormatMoney(nMoneySupply).c_str(),
			IsProofOfStake() ? "PoS" : "PoW",
			hashMerkleRoot.ToString().substr(0,10).c_str(),
			GetBlockHash().ToString().substr(0,20).c_str());
	}

	void CBlockIndex::print() const
	{
		printf("%s\n", ToString().c_str());
	}
	
	
	
	
	
	void CBlockLocator::Set(const CBlockIndex* pindex)
	{
		vHave.clear();
		int nStep = 1;
		while (pindex)
		{
			vHave.push_back(pindex->GetBlockHash());

			// Exponentially larger steps back
			for (int i = 0; pindex && i < nStep; i++)
				pindex = pindex->pprev;
			if (vHave.size() > 10)
				nStep *= 2;
		}
		vHave.push_back(hashGenesisBlock);
	}

		
	int CBlockLocator::GetDistanceBack()
	{
		// Retrace how far back it was in the sender's branch
		int nDistance = 0;
		int nStep = 1;
		for(const uint1024& hash : vHave)
		{
			std::map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
			if (mi != mapBlockIndex.end())
			{
				CBlockIndex* pindex = (*mi).second;
				if (pindex->IsInMainChain())
					return nDistance;
			}
			nDistance += nStep;
			if (nDistance > 10)
				nStep *= 2;
		}
		return nDistance;
	}

	
	CBlockIndex* CBlockLocator::GetBlockIndex()
	{
		// Find the first block the caller has in the main chain
		BOOST_FOREACH(const uint1024& hash, vHave)
		{
			std::map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
			if (mi != mapBlockIndex.end())
			{
				CBlockIndex* pindex = (*mi).second;
				if (pindex->IsInMainChain())
					return pindex;
			}
		}
		return pindexGenesisBlock;
	}

	
	uint1024 CBlockLocator::GetBlockHash()
	{
		// Find the first block the caller has in the main chain
		BOOST_FOREACH(const uint1024& hash, vHave)
		{
			std::map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
			if (mi != mapBlockIndex.end())
			{
				CBlockIndex* pindex = (*mi).second;
				if (pindex->IsInMainChain())
					return hash;
			}
		}
		return hashGenesisBlock;
	}

	
	/* bdg note: GetHeight is never used. */
	int CBlockLocator::GetHeight()
	{
		CBlockIndex* pindex = GetBlockIndex();
		if (!pindex)
			return 0;
		return pindex->nHeight;
	}

}
