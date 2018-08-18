/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_H

#include "../../../../Util/templates/serialize.h"

//forward declerations for BigNum
namespace LLC
{
	namespace TYPES
	{
		class CBigNum;
	}
}

namespace Legacy
{
	class Transaction;
}

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
		class Block
		{
		public:

			/** The blocks version for. Useful for changing rules. **/
			unsigned int nVersion;


			/** The previous blocks hash. Used to chain blocks together. **/
			uint1024 hashPrevBlock;


			/** The Merkle Root. A merkle tree of transaction hashes included in header. **/
			uint512 hashMerkleRoot;


			/** The Block Channel. This number designates what validation algorithm is required. **/
			unsigned int nChannel;


			/** The Block's Height. This number tells what block number this is in the chain. **/
			unsigned int nHeight;


			/** The Block's Bits. This number is a compact representation of the required difficulty. **/
			unsigned int nBits;


			/** The Block's nOnce. This number is used to find the "winning" hash. **/
			uint64 nNonce;


			/** The Block's timestamp. This number is locked into the signature hash. **/
			unsigned int nTime;


			/** The bytes holding the blocks signature. Signed by the block creator before broadcast. **/
			std::vector<unsigned char> vchBlockSig;


			/** The transactions included in this block. Used to build the merkle tree. **/
			std::vector<Legacy::Transaction> vtx;


			/** Memory only structure to hold merkle tree data. **/
			mutable std::vector<uint512> vMerkleTree;


			//standard serialization methods
			SERIALIZE_HEADER


			/** The default constructor. Sets block state to Null. **/
			Block() { SetNull(); }


			/** A base constructor.
			 *
			 *	@param[in] nVersionIn The version to set block to
			 *	@param[in] hashPrevBlockIn The previous block being linked to
			 *	@param[in] nChannelIn The channel this block is being created for
			 *	@param[in] nHeightIn The height this block is being created at.
			 *
			**/
			Block(unsigned int nVersionIn, uint1024 hashPrevBlockIn, unsigned int nChannelIn, unsigned int nHeightIn) : nVersion(nVersionIn), hashPrevBlock(hashPrevBlockIn), nChannel(nChannelIn), nHeight(nHeightIn), nBits(0), nNonce(0), nTime(0) { }


			/** Set the block to Null state. **/
			void SetNull()
			{
				nVersion = 3; //TODO: Use current block version
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


			/** SetChannel
			 *
			 *	Sets the channel for the block.
			 *
			 *	@param[in] nNewChannel The channel to set.
			 *
			 **/
			void SetChannel(unsigned int nNewChannel);


			/** GetChannel
			 *
			 *	Gets the channel the block belongs to
			 *
			 *	@return The channel assigned (unsigned int)
			 *
			 */
			unsigned int GetChannel() const;


			/** IsNull
			 *
			 *	Checks the Null state of the block
			 *
			 *	@return true if null, false otherwise
			 *
			 **/
			bool IsNull() const;


			/** GetBlockTime
			 *
			 *	Returns the current UNIX timestamp of the block.
			 *
			 *	@return 64-bit unsigned integer of timestamp
			 *
			 **/
			uint64_t GetBlockTime() const;


			/** GetPrime
			 *
			 *	Get the Prime number for the block (hash + nNonce)
			 *
			 *	@return Prime number stored as a CBigNum (wrapper for BIGNUM in OpenSSL)
			 *
			 **/
			LLC::TYPES::CBigNum GetPrime() const;


			/** GetHash
			 *
			 *	Get the Hash of the block.
			 *
			 *	@return 1024-bit block hash
			 *
			 **/
			uint1024 GetHash() const;


			/** SignatureHash
			 *
			 *	Get the signature hash of block. This is signed by block finder.
			 *
			 *	@return 1024-bit hash for signature
			 *
			 **/
			uint1024 SignatureHash() const;


			/** UpdateTime
			 *
			 *	Update the blocks Timestamp
			 *
			 **/
			void UpdateTime();


			/** IsProofOfStake
			 *
			 *	@return true if the block is proof of stake.
			 *
			 **/
			bool IsProofOfStake() const;


			/** IsProofOfWork
			 *
			 *	@return true if the block is proof of work.
			 *
			 **/
			bool IsProofOfWork() const;


			/** BuildMerkleTree
			 *
			 *	Build the merkle tree from the transaction list.
			 *
			 *	@return The 512-bit merkle root
			 *
			 **/
			uint512 BuildMerkleTree() const;


			/** GetMerkleBranch
			 *
			 *	Find the branch in the merkle tree that the given index belongs to.
			 *
			 *	@param[in] nIndex The index to search for
			 *
			 *	@return A vector containing the hashes of the transaction's branch
			 *
			 **/
			std::vector<uint512> GetMerkleBranch(int nIndex) const;


			/** CheckMerkleBranch
			 *
			 *	Check that the transaction exists in the merkle branch.
			 *
			 **/
			uint512 CheckMerkleBranch(uint512 hash, const std::vector<uint512>& vMerkleBranch, int nIndex);


			/** print
			 *
			 *	Dump to the log file the raw block data
			 *
			 **/
			void print() const;


			/** VerifyWork
			 *
			 *	Verify the work was completed by miners as advertised
			 *
			 *	@return true if work is valid, false otherwise
			 *
			 **/
			bool VerifyWork() const;


			/** VerifyStake
			 *
			 *	Verify the stake work was completed
			 *
			 *	@return true if stake is valid, false otherwise
			 *
			 **/
			bool VerifyStake() const;


			/** GenerateSignature
			 *
			 *	Generate the signature as the block finder
			 *
			 *	@param[in] key The key object containing private key to make Signature
			 *
			 *	@return True if the signature was made successfully, false otherwise
			 *
			 **/
			bool GenerateSignature(const LLC::CKey& key);


			/** VerifySignature
			 *
			 *	Verify that the signature included in block is valid
			 *
			 *	@return True if signature is valid, false otherwise
			 *
			 **/
			bool VerifySignature() const;


		};

	}

}

#endif
