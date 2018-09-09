/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/include/key.h>
#include <LLC/types/bignum.h>

#include <Util/templates/serialize.h>
#include <Util/include/hex.h>
#include <Util/include/runtime.h>

#include <TAO/Ledger/types/block.h>

namespace TAO
{

	namespace Ledger
	{

		//serialization functions
		//TODO: Serialize vtx on network, but not on disk
		//Only serialize the hashes of the transactions (as precurser to light blocks)
		//Then blocks can be relayed with no need for transactional data, which can be represented by L1 locks later down.
		//Block size will not exist, blocks will only keep record of transactions in merkle done by processing buckets.
		//Until done in the future with Amine these will be at the descretion of Miners what buckets to include.
		//possibly assess a penalty if a bucket exists that a miner doesn't include.
		SERIALIZE_SOURCE
		(
			Block,

			READWRITE(this->nVersion);
			READWRITE(hashPrevBlock);
			READWRITE(hashMerkleRoot);
			READWRITE(nChannel);
			READWRITE(nHeight);
			READWRITE(nBits);
			READWRITE(nNonce);
			READWRITE(nTime);
			READWRITE(vchBlockSig);
		)


		/* Set the block state to null. */
		void Block::SetNull()
		{
			nVersion = 3; //TODO: Use current block version
			hashPrevBlock = 0;
			hashMerkleRoot = 0;
			nChannel = 0;
			nHeight = 0;
			nBits = 0;
			nNonce = 0;
			nTime = 0;
			vchBlockSig.clear();
		}


		/* Set the channel of the block. */
		void Block::SetChannel(uint32_t nNewChannel)
		{
			nChannel = nNewChannel;
		}


		/* Get the Channel block is produced from. */
		uint32_t Block::GetChannel() const
		{
			return nChannel;
		}


		/* Check the NULL state of the block. */
		bool Block::IsNull() const
		{
			return (nBits == 0);
		}


		/* Return the Block's current UNIX Timestamp. */
		uint64_t Block::GetBlockTime() const
		{
			return (uint64_t)nTime;
		}


		/* Get the prime number of the block. */
		LLC::CBigNum Block::GetPrime() const
		{
			return LLC::CBigNum(ProofHash() + nNonce);
		}


		/* GGet the Proof Hash of the block. Used to verify work claims. */
		uint1024_t Block::ProofHash() const
		{
			/** Hashing template for CPU miners uses nVersion to nBits **/
			if(GetChannel() == 1)
				return LLC::SK1024(BEGIN(nVersion), END(nBits));

			/** Hashing template for GPU uses nVersion to nNonce **/
			return LLC::SK1024(BEGIN(nVersion), END(nNonce));
		}


		uint1024_t Block::StakeHash()
		{

		}


		/* Generate a Hash For the Block from the Header. */
		uint1024_t Block::BlockHash() const
		{
			return LLC::SK1024(BEGIN(nVersion), END(nTime));
		}


		/* Update the nTime of the current block. */
		void Block::UpdateTime()
		{
			nTime = UnifiedTimestamp();
		}


		/* Check flags for nPoS block. */
		bool Block::IsProofOfStake() const
		{
			return (nChannel == 0);
		}


		/* Check flags for PoW block. */
		bool Block::IsProofOfWork() const
		{
			return (nChannel == 1 || nChannel == 2);
		}


		/* Generate the Merkle Tree from uint512_t hashes. */
		uint512_t Block::BuildMerkleTree(std::vector<uint512_t> vMerkleTree) const
		{
			int j = 0;
			for (int nSize = (int)vMerkleTree.size(); nSize > 1; nSize = (nSize + 1) / 2)
			{
				for (int i = 0; i < nSize; i += 2)
				{
					int i2 = std::min(i+1, nSize-1);
					vMerkleTree.push_back(LLC::SK512(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
												BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
				}
				j += nSize;
			}
			return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
		}


		/* Dump the Block data to Console / Debug.log. */
		void Block::print() const
		{
			printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nChannel = %u, nHeight = %u, nNonce=%" PRIu64 ", vchBlockSig=%s)\n",
					BlockHash().ToString().substr(0,20).c_str(),
					nVersion,
					hashPrevBlock.ToString().substr(0,20).c_str(),
					hashMerkleRoot.ToString().substr(0,10).c_str(),
					nTime, nBits, nChannel, nHeight, nNonce,
					HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
		}


		/* Verify the Proof of Work satisfies network requirements. */
		bool Block::VerifyWork() const
	    {

			//fill in proof of work here when linked together

	        return true;
	    }


		/* Verify the Proof of Stake satisfies network requirements. */
		bool Block::VerifyStake() const
	    {

			//TODO: Fill in when trust source file complete

	        return true;
	    }


		/* Sign the block with the key that found the block. */
		bool Block::GenerateSignature(const LLC::ECKey key)
		{
			return false;
		}

		/* Check that the block signature is a valid signature. */
		bool Block::VerifySignature() const
		{
			return false;
		}
	}
}
