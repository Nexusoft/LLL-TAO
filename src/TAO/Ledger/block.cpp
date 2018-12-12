/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

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
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/constants.h>

namespace TAO::Ledger
{

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
	uint1024_t Block::GetHash() const
	{
		/* Pre-Version 5 rule of being block hash. */
		if(nVersion < 5)
			return ProofHash();

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
		debug::log(0, "Block(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nChannel = %u, nHeight = %u, nNonce=%" PRIu64 ", vchBlockSig=%s)\n",
				GetHash().ToString().substr(0,20).c_str(),
				nVersion,
				hashPrevBlock.ToString().substr(0,20).c_str(),
				hashMerkleRoot.ToString().substr(0,10).c_str(),
				nTime, nBits, nChannel, nHeight, nNonce,
				HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
	}


	/* Verify the Proof of Work satisfies network requirements. */
	bool Block::VerifyWork() const
    {
		/* Check the Prime Number Proof of Work for the Prime Channel. */
		if(GetChannel() == 1)
		{
			/* Check prime minimum origins. */
			if(nVersion < 5 && ProofHash() < bnPrimeMinOrigins.getuint1024())
				return debug::error(FUNCTION "prime origins below 1016-bits", __PRETTY_FUNCTION__);

			/* Check proof of work limits. */
			uint32_t nPrimeBits = GetPrimeBits(GetPrime());
			if (nPrimeBits < bnProofOfWorkLimit[1])
				return debug::error(FUNCTION "prime-cluster below minimum work", __PRETTY_FUNCTION__);

			/* Check the prime difficulty target. */
			if(nBits > nPrimeBits)
				return debug::error(FUNCTION "prime-cluster below target", __PRETTY_FUNCTION__);

			return true;
		}

		/* Get the hash target. */
		LLC::CBigNum bnTarget;
		bnTarget.SetCompact(nBits);

		/* Check that the hash is within range. */
		if (bnTarget <= 0 || bnTarget > bnProofOfWorkLimit[2])
			return debug::error(FUNCTION "proof-of-work hash not in range", __PRETTY_FUNCTION__);


		/* Check that the that enough work was done on this block. */
		if (ProofHash() > bnTarget.getuint1024())
			return debug::error(FUNCTION "proof-of-work hash below target", __PRETTY_FUNCTION__);

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
	bool Block::VerifySignature(const LLC::ECKey key) const
	{
		return false;
	}
}
