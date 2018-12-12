/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_MERKLE_H
#define NEXUS_TAO_LEGACY_TYPES_MERKLE_H

#include <TAO/Legacy/types/transaction.h>

#include <Util/templates/serialize.h>


namespace TAO
{
	namespace Ledger
	{
		class Block;
	}
}

namespace Legacy
{
	class CBlockIndex;

    /** A transaction with a merkle branch linking it to the block chain. */
	class CMerkleTx : public Transaction
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

		CMerkleTx(const Transaction& txIn) : Transaction(txIn)
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
			nSerSize += SerReadWrite(s, *(Transaction*)this, nSerType, nVersion, ser_action);
			nSerVersion = this->nVersion;
			READWRITE(hashBlock);
			READWRITE(vMerkleBranch);
			READWRITE(nIndex);
		)


		int SetMerkleBranch(const TAO::Ledger::Block* pblock=NULL) {}
		//int GetDepthInMainChain(CBlockIndex* &pindexRet) const;
		int GetDepthInMainChain() const { return 0; }
		bool IsInMainChain() const { return GetDepthInMainChain() > 0; }
		int GetBlocksToMaturity() const { return 0; }
	};
}

#endif
