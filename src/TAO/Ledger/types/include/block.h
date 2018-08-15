/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_H

#include "transaction.h"
#include "../../../LLC/hash/macro.h"

namespace TAO
{
	namespace Ledger
	{
		/** Nodes collect new transactions into a block, hash them into a hash tree,
		 * and scan through nonce values to make the block's hash satisfy proof-of-work
		 * requirements.  When they solve the proof-of-work, they broadcast the block
		 * to everyone and the block is added to the block chain.  The first transaction
		 * in the block is a special one that creates a new coin owned by the creator
		 * of the block.
		 *
		 * Blocks are appended to blk0001.dat files on disk.  Their location on disk
		 * is indexed by CBlockIndex objects in memory.
		 */
		class CBlock
		{
		public:

			/* Core Block Header. */
			unsigned int nVersion;
			uint1024 hashPrevBlock;
			uint512 hashMerkleRoot;
			unsigned int nChannel;
			unsigned int nHeight;
			unsigned int nBits;
			uint64 nNonce;


			/* The Block Time locked in Block Signature. */
			unsigned int nTime;


			/* Nexus block signature */
			std::vector<unsigned char> vchBlockSig;


			/* Transactions for Current Block. */
			std::vector<CTransaction> vtx;


			/* Memory Only Data. */
			mutable std::vector<uint512> vMerkleTree;

			uint512 hashPrevChecksum; //for new signature hash input. Checksum of previous block signature for block signature chains

			IMPLEMENT_SERIALIZE
			(
				READWRITE(this->nVersion);
				nVersion = this->nVersion;
				READWRITE(hashPrevBlock);
				READWRITE(hashMerkleRoot);
				READWRITE(nChannel);
				READWRITE(nHeight);
				READWRITE(nBits);
				READWRITE(nNonce);
				READWRITE(nTime);

	            //TODO: Serialize vtx on network, but not on disk
	            //Only serialize the hashes of the transactions (as precurser to light blocks)
	            //Then blocks can be relayed with no need for transactional data, which can be represented by L1 locks later down.
	            //Block size will not exist, blocks will only keep record of transactions in merkle done by processing buckets.
	            //Until done in the future with Amine these will be at the descretion of Miners what buckets to include.
	            //possibly assess a penalty if a bucket exists that a miner doesn't include.

				// ConnectBlock depends on vtx following header to generate CDiskTxPos
				if (!(nType & (SER_GETHASH|SER_BLOCKHEADERONLY)))
				{
					READWRITE(vtx);
					READWRITE(vchBlockSig);
				}
				else if (fRead)
				{
					const_cast<CBlock*>(this)->vtx.clear();
					const_cast<CBlock*>(this)->vchBlockSig.clear();
				}
			)

			CBlock()
			{
				SetNull();
			}

			CBlock(unsigned int nVersionIn, uint1024 hashPrevBlockIn, unsigned int nChannelIn, unsigned int nHeightIn)
			{
				SetNull();

				nVersion = nVersionIn;
				hashPrevBlock = hashPrevBlockIn;
				nChannel = nChannelIn;
				nHeight  = nHeightIn;
			}


			/* Set block to a NULL state. */
			void SetNull()
			{
				/* bdg question: should this use the current version instead of hard coded 3? */
				nVersion = 3;
				hashPrevBlock = 0;
				hashMerkleRoot = 0;
				nChannel = 0;
				nHeight = 0;
				nBits = 0;
				nNonce = 0;
				nTime = 0;

				vtx.clear();
				vchBlockSig.clear();
				vMerkleTree.clear();
			}


			/* bdg note: SetChannel is never used */
			/* Set the Channel for block. */
			void SetChannel(unsigned int nNewChannel)
			{
				nChannel = nNewChannel;
			}


			/* Get the Channel block is produced from. */
			int GetChannel() const
			{
				return nChannel;
			}


			/* Check the NULL state of the block. */
			bool IsNull() const
			{
				return (nBits == 0);
			}


			/* Return the Block's current UNIX Timestamp. */
			int64 GetBlockTime() const
			{
				return (int64)nTime;
			}


			/* bdg question: should this check if the block is in the prime channel first? */
			/* Get the prime number of the block. */
			CBigNum GetPrime() const
			{
				return CBigNum(GetHash() + nNonce);
			}


			/* Generate a Hash For the Block from the Header. */
			uint1024 GetHash() const
			{
				/** Hashing template for CPU miners uses nVersion to nBits **/
				if(GetChannel() == 1)
					return LLC::HASH::SK1024(BEGIN(nVersion), END(nBits));

				/** Hashing template for GPU uses nVersion to nNonce **/
				return LLC::HASH::SK1024(BEGIN(nVersion), END(nNonce));
			}


			/* Generate the Signature Hash Required After Block completes Proof of Work / Stake. */
			uint1024 SignatureHash() const
			{
				if(nVersion < 5)
					return LLC::HASH::SK1024(BEGIN(nVersion), END(nTime));
				else
					return LLC::HASH::SK1024(BEGIN(nVersion), END(hashPrevChecksum));
			}


			/* Update the nTime of the current block. */
			void UpdateTime();


			/* Check flags for nPoS block. */
			bool IsProofOfStake() const
			{
				return (nChannel == 0);
			}


			/* Check flags for PoW block. */
			bool IsProofOfWork() const
			{
				return (nChannel == 1 || nChannel == 2);
			}


			/* Generate the Merkle Tree from uint512 hashes. */
			uint512 BuildMerkleTree() const
			{
				vMerkleTree.clear();
				BOOST_FOREACH(const CTransaction& tx, vtx)
					vMerkleTree.push_back(tx.GetHash());
				int j = 0;
				for (int nSize = (int)vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
				{
					for (int i = 0; i < nSize; i += 2)
					{
						int i2 = std::min(i+1, nSize-1);
						vMerkleTree.push_back(LLC::HASH::SK512(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
												    BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
					}
					j += nSize;
				}
				return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
			}


			/* Get the current Branch that is being worked on. */
			std::vector<uint512> GetMerkleBranch(int nIndex) const
			{
				if (vMerkleTree.empty())
					BuildMerkleTree();
				std::vector<uint512> vMerkleBranch;
				int j = 0;
				for (int nSize = (int)vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
				{
					int i = std::min(nIndex^1, nSize-1);
					vMerkleBranch.push_back(vMerkleTree[j+i]);
					nIndex >>= 1;
					j += nSize;
				}
				return vMerkleBranch;
			}


			/* Check that the Merkle branch matches hash tree. */
			static uint512 CheckMerkleBranch(uint512 hash, const std::vector<uint512>& vMerkleBranch, int nIndex)
			{
				if (nIndex == -1)
					return 0;
				for(int i = 0; i < vMerkleBranch.size(); i++)
				{
					if (nIndex & 1)
						hash = LLC::HASH::SK512(BEGIN(vMerkleBranch[i]), END(vMerkleBranch[i]), BEGIN(hash), END(hash));
					else
						hash = LLC::HASH::SK512(BEGIN(hash), END(hash), BEGIN(vMerkleBranch[i]), END(vMerkleBranch[i]));
					nIndex >>= 1;
				}
				return hash;
			}

			/* Write Block to Disk File. */
			bool WriteToDisk(unsigned int& nFileRet, unsigned int& nBlockPosRet);


			/* Read Block from Disk File. */
			bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions=true);


			/* Read Block from Disk File by Index Object. */
			bool ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions=true);


			/* Dump the Block data to Console / Debug.log. */
			void print() const;


			/* Disconnect all associated inputs from a block. */
			bool DisconnectBlock(LLD::CIndexDB& indexdb, CBlockIndex* pindex);


			/* Connect all associated inputs from a block. */
			bool ConnectBlock(LLD::CIndexDB& indexdb, CBlockIndex* pindex);


			/* Verify the Proof of Work satisfies network requirements. */
			bool VerifyWork() const;


			/* Verify the Proof of Stake satisfies network requirements. */
			bool VerifyStake() const;


			/* Sign the block with the key that found the block. */
			bool SignBlock(const Wallet::CKeyStore& keystore);


			/* Check that the block signature is a valid signature. */
			bool CheckBlockSignature() const;

		};


		/** Blockchain Speicific State variables for a block after it is processed.
		 *
		 * This can be broadcast to other nodes as a fully state locked block, with
		 * optional signature of data or validated and generated individually by each
		 * node.
		 *
		 */
		class CBlockState : public CBlock
		{
		public:

			/* The Trust of the Chain to this Block. */
			uint64 nChainTrust;


			/* The Total NXS released to date */
			uint64 nMoneySupply;


			/* The height of this channel. */
			unsigned int nChannelHeight;


			/* The reserves that are released. */
			unsigned int nReleasedReserve[2];


			/* The checkpoint this block was made from. */
			uint1024 hashCheckpoint;


	        /* Used to Iterate forward in the chain */
	        uint1024 hashNextBlock;


	        /* Boolean flag for if this block is connected. */
	        bool fConnected;


	        /* Serialization Macros */
			IMPLEMENT_SERIALIZE
			(
				READWRITE(nChainTrust);
				READWRITE(nMoneySupply);
				READWRITE(nChannelHeight);
				READWRITE(nReleasedReserve[0]);
				READWRITE(nReleasedReserve[1]);
				READWRITE(nReleasedReserve[2]);
				READWRITE(hashCheckpoint);
				READWRITE(hashNextBlock);
			)


			CBlockState() : nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { SetNull(); }

			CBlockState(CBlock blk) : CBlock(blk), nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { }


	        /* Get the block header. */
	        CBlock GetHeader() const
	        {
	            CBlock block;
	            block.nVersion       = nVersion;
	            block.hashPrevBlock  = hashPrevBlock;
	            block.hashMerkleRoot = hashMerkleRoot;
	            block.nChannel       = nChannel;
	            block.nHeight        = nHeight;
	            block.nBits          = nBits;
	            block.nNonce         = nNonce;
	            block.nTime          = nTime;

	            return block;
	        }


	        /* Function to determine if this block has been connected into the main chain. */
	        bool IsInMainChain() const
	        {
	            return fConnected;
	        }


	        /* The hash of this current block state. */
	        uint1024 StateHash() const
	        {
	            return LLC::HASH::SK1024(BEGIN(nVersion), END(hashCheckpoint));
	        }


	        /* Flag to determine if this block is a Proof-of-Work block. */
	        bool IsProofOfWork() const
	        {
	            return (nChannel > 0);
	        }


	        /* Flag to determine if this block is a Proof-of-Stake block. */
	        bool IsProofOfStake() const
	        {
	            return (nChannel == 0);
	        }


	        /* For debuggin Purposes seeing block state data dump */
	        std::string ToString() const;


	        /* For debugging purposes, printing the block to stdout */
	        void print() const;
		};

	}

}




	/** DEPRECATED:
	 *
	 * Describes a place in the block chain to another node such that if the
	 * other node doesn't have the same branch, it can find a recent common trunk.
	 * The further back it is, the further before the fork it may be.
	 */
	class CBlockLocator
	{
	protected:
		std::vector<uint1024> vHave;

	public:

		IMPLEMENT_SERIALIZE
		(
			if (!(nType & SER_GETHASH))
				READWRITE(nVersion);
			READWRITE(vHave);
		)

		CBlockLocator()
		{
		}

		explicit CBlockLocator(const CBlockIndex* pindex)
		{
			Set(pindex);
		}

		explicit CBlockLocator(uint1024 hashBlock)
		{
			std::map<uint1024, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
			if (mi != mapBlockIndex.end())
				Set((*mi).second);
		}


		/* Set by List of Vectors. */
		CBlockLocator(const std::vector<uint1024>& vHaveIn)
		{
			vHave = vHaveIn;
		}


		/* Set the State of Object to NULL. */
		void SetNull()
		{
			vHave.clear();
		}


		/* Check the State of Object as NULL. */
		bool IsNull()
		{
			return vHave.empty();
		}


		/* Set from Block Index Object. */
		void Set(const CBlockIndex* pindex);


		/* Find the total blocks back locator determined. */
		int GetDistanceBack();


		/* Get the Index object stored in Locator. */
		CBlockIndex* GetBlockIndex();


		/* Get the hash of block. */
		uint1024 GetBlockHash();


		/* bdg note: GetHeight is never used. */
		/* Get the Height of the Locator. */
		int GetHeight();

	};


}

#endif
