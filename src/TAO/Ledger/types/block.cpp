/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include "include/block.h"

#include "../../../LLC/hash/macro.h"

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
			nVersion = this->nVersion;
			READWRITE(hashPrevBlock);
			READWRITE(hashMerkleRoot);
			READWRITE(nChannel);
			READWRITE(nHeight);
			READWRITE(nBits);
			READWRITE(nNonce);
			READWRITE(nTime);
			READWRITE(vtx);
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

			vtx.clear();
			vchBlockSig.clear();
			vMerkleTree.clear();
		}


		/* Set the channel of the block. */
		void Block::SetChannel(uint32_t nNewChannel)
		{
			nChannel = nNewChannel;
		}


		/* Get the Channel block is produced from. */
		int Block::GetChannel() const
		{
			return nChannel;
		}


		/* Check the NULL state of the block. */
		bool Block::IsNull() const
		{
			return (nBits == 0);
		}


		/* Return the Block's current UNIX Timestamp. */
		int64_t Block::GetBlockTime() const
		{
			return (int64_t)nTime;
		}


		/* Get the prime number of the block. */
		LLC::CBigNum Block::GetPrime() const
		{
			return LLC::CBigNum(GetHash() + nNonce);
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
			return LLC::SK1024(BEGIN(nVersion), END(nTime));
		}


		/* Generate the Signature Hash Required After Block completes Proof of Work / Stake. */
		uint1024_t Block::SignatureHash() const
		{
			if(nVersion < 5)
				return LLC::SK1024(BEGIN(nVersion), END(nTime));
			else
				return LLC::SK1024(BEGIN(nVersion), END(hashPrevChecksum));
		}


		/* Update the nTime of the current block. */
		void Block::UpdateTime()
		{
			nTime = std::max((uint64_t)mapBlockIndex[hashPrevBlock]->GetBlockTime() + 1, UnifiedTimestamp());
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
			for (int nSize = (int)vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
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
			printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nChannel = %u, nHeight = %u, nNonce=%" PRIu64 ", vtx=%d, vchBlockSig=%s)\n",
					GetHash().ToString().substr(0,20).c_str(),
					nVersion,
					hashPrevBlock.ToString().substr(0,20).c_str(),
					hashMerkleRoot.ToString().substr(0,10).c_str(),
					nTime, nBits, nChannel, nHeight, nNonce,
					vtx.size(),
					HexStr(vchBlockSig.begin(), vchBlockSig.end()).c_str());
		}


		/* Verify the Proof of Work satisfies network requirements. */
		bool CBlock::VerifyWork() const
	    {
	        /** Check the Prime Number Proof of Work for the Prime Channel. **/
	        if(GetChannel() == 1)
	        {
	            if(nVersion >= 5 && ProofHash() < bnPrimeMinOrigins.getuint1024())
	                return error("VerifyWork() : prime origins below 1016-bits");

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
	        if ((nVersion < 5 ? GetHash() : ProofHash()) > bnTarget.getuint1024())
	            return error("VerifyWork() : proof-of-work hash below target");

	        return true;
	    }


		/* Verify the Proof of Stake satisfies network requirements. */
		bool Block::VerifyStake() const
	    {

			//TODO: Fill in when trust source file complete

	        return true;
	    }


		/* Sign the block with the key that found the block. */
		bool Block::GenerateSignature(const LLC::CKey& key)
		{
			vector<std::vector<uint8_t> > vSolutions;
			Wallet::TransactionType whichType;
			const CTxOut& txout = vtx[0].vout[0];

			if (!Solver(txout.scriptPubKey, whichType, vSolutions))
				return false;

			if (whichType == Wallet::TX_PUBKEY)
			{
				// Sign
				const std::vector<uint8_t>& vchPubKey = vSolutions[0];
				if (key.GetPubKey() != vchPubKey)
					return false;

				return key.Sign(GetHash(), vchBlockSig, 1024);
			}

			return false;
		}

		/* Check that the block signature is a valid signature. */
		bool Block::VerifySignature() const
		{
			if (GetHash() == hashGenesisBlock)
				return vchBlockSig.empty();

			vector<std::vector<uint8_t> > vSolutions;
			Wallet::TransactionType whichType;
			const CTxOut& txout = vtx[0].vout[0];

			if (!Solver(txout.scriptPubKey, whichType, vSolutions))
				return false;
			if (whichType == Wallet::TX_PUBKEY)
			{
				const std::vector<uint8_t>& vchPubKey = vSolutions[0];
				Wallet::CKey key;
				if (!key.SetPubKey(vchPubKey))
					return false;
				if (vchBlockSig.empty())
					return false;
				return key.Verify(GetHash(), vchBlockSig, 1024);
			}
			return false;
		}
	}
}
