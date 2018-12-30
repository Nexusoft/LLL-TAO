/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_H

#include <LLC/types/uint1024.h>

#include <Util/include/runtime.h>

//forward declerations for BigNum
namespace LLC
{
	class CBigNum;
	class ECKey;
}

namespace TAO::Ledger
{

	/** Block Class
	 *
	 *  Nodes collect new transactions into a block, hash them into a hash tree,
	 *  and scan through nonce values to make the block's hash satisfy validation
	 *  requirements.
	 *
	 */
	class Block
	{
	public:

		/** The blocks version for. Useful for changing rules. **/
		uint32_t nVersion;


		/** The previous blocks hash. Used to chain blocks together. **/
		uint1024_t hashPrevBlock;


		/** The Merkle Root. A merkle tree of transaction hashes included in header. **/
		uint512_t hashMerkleRoot;


		/** The Block Channel. This number designates what validation algorithm is required. **/
		uint32_t nChannel;


		/** The Block's Height. This number tells what block number this is in the chain. **/
		uint32_t nHeight;


		/** The Block's Bits. This number is a compact representation of the required difficulty. **/
		uint32_t nBits;


		/** The Block's nOnce. This number is used to find the "winning" hash. **/
		uint64_t nNonce;


		/** The Block's timestamp. This number is locked into the signature hash. **/
		uint32_t nTime; //TODO: make this 64 bit


		/** The bytes holding the blocks signature. Signed by the block creator before broadcast. **/
		std::vector<uint8_t> vchBlockSig;



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
		Block(uint32_t nVersionIn, uint1024_t hashPrevBlockIn, uint32_t nChannelIn, uint32_t nHeightIn)
		: nVersion(nVersionIn)
		, hashPrevBlock(hashPrevBlockIn)
		, nChannel(nChannelIn)
		, nHeight(nHeightIn)
		, nBits(0)
		, nNonce(0)
		, nTime(runtime::unifiedtimestamp()) { }


		/** Set the block to Null state. **/
		void SetNull();


		/** SetChannel
		 *
		 *	Sets the channel for the block.
		 *
		 *	@param[in] nNewChannel The channel to set.
		 *
		 **/
		void SetChannel(uint32_t nNewChannel);


		/** GetChannel
		 *
		 *	Gets the channel the block belongs to
		 *
		 *	@return The channel assigned (uint32_t)
		 *
		 */
		uint32_t GetChannel() const;


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
		 *	@return 64-bit uint32_teger of timestamp
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
		LLC::CBigNum GetPrime() const;


		/** Proof Hash
		 *
		 *	Get the Proof Hash of the block. Used to verify work claims.
		 *
		 *	@return 1024-bit proof hash
		 *
		 **/
		uint1024_t ProofHash() const;


		/** Signature Hash
		 *
		 *	Get the hash for signatures.
		 *
		 *	@return 1024-bit proof hash
		 *
		 **/
		uint1024_t SignatureHash() const;


		/** Stake Hash
		 *
		 *	Prove that you staked a number of seconds based on weight
		 *
		 *	@return 1024-bit stake hash
		 *
		 **/
		uint1024_t StakeHash();


		/** Get Hash
		 *
		 *	Get the Hash of the block.
		 *
		 *	@return 1024-bit block hash
		 *
		 **/
		uint1024_t GetHash() const;


		/** UpdateTime
		 *
		 *	Update the blocks timestamp
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
		uint512_t BuildMerkleTree(std::vector<uint512_t> vMerkleTree) const;



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
		bool GenerateSignature(LLC::ECKey key);


		/** VerifySignature
		 *
		 *	Verify that the signature included in block is valid
		 *
		 *	@return True if signature is valid, false otherwise
		 *
		 **/
		bool VerifySignature(LLC::ECKey key) const;


	};
}

#endif
