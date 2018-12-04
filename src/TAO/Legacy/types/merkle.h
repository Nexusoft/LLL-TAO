/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_MERKLE_H
#define NEXUS_TAO_LEGACY_TYPES_MERKLE_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/types/transaction.h>

#include <Util/templates/serialize.h>

/* forward declarations */
class CBlock;
class CBLockIndex;

namespace Legacy
{

    /** A transaction with a merkle branch linking it to the block chain. */
	class CMerkleTx : public CTransaction
	{
	public:
		uint1024_t hashBlock;
		std::vector<uint512_t> vMerkleBranch;
		int nIndex;

		// memory only
		mutable bool fMerkleVerified;


		CMerkleTx()
		{
			Init();
		}

		CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
		{
			Init();
		}

		void Init()
		{
			hashBlock = 0;
			nIndex = -1;
			fMerkleVerified = false;
		}


		IMPLEMENT_SERIALIZE
		(
			nSerSize += SerReadWrite(s, *(CTransaction*)this, nSerType, nVersion, ser_action);
			nVersion = this->nVersion;
			READWRITE(hashBlock);
			READWRITE(vMerkleBranch);
			READWRITE(nIndex);
		)


		int SetMerkleBranch(const CBlock* pblock=NULL);
		int GetDepthInMainChain(CBlockIndex* &pindexRet) const;
		int GetDepthInMainChain() const { CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
		bool IsInMainChain() const { return GetDepthInMainChain() > 0; }
		int GetBlocksToMaturity() const;
	};

}
