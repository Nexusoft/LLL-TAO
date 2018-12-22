/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_TYPES_MERKLE_H
#define NEXUS_TAO_LEGACY_TYPES_MERKLE_H

#include <TAO/Ledger/types/tritium.h>
#include <Legacy/types/transaction.h>

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

    /** @class CMerkleTx
     *
     * A transaction with a merkle branch linking it to the block chain.
     *
     **/
	class CMerkleTx : public Transaction
	{
	private:
        /** Init
         *
         *  Initializes an empty merkle transaction
         *
         **/
		void Init()
		{
			hashBlock = 0;
			nIndex = -1;
			fMerkleVerified = false;
		}


	public:
		/** The block hash of the block containing this transaction **/
		uint1024_t hashBlock;

		/** The merkle branch for this transaction **/
		std::vector<uint512_t> vMerkleBranch;

		/** Index of transaction within containing block **/
		int32_t nIndex;

		/** Memory only, true if merkle branch confirmed as connecting to containing block **/
		mutable bool fMerkleVerified;


        /** Constructor
         *
         *  Initializes an empty merkle transaction
         *
         **/
		CMerkleTx()
		{
			Init();
		}


        /** Constructor
         *
         *  Initializes a merkle transaction with data copied from a Transaction. 
         *
         *  @param[in] pwalletIn The wallet for this wallet transaction
         *
         *  @param[in] txIn Transaction data to copy into this merkle transaction
         *
         **/
		CMerkleTx(const Transaction& txIn) : Transaction(txIn)
		{
			Init();
		}


		/* Implement serialization/deserializaiton for CMerkleTx, first by serializing/deserializing 
		 * base class data then processing local data 
		 */
		IMPLEMENT_SERIALIZE
		(
			nSerSize += SerReadWrite(s, *(Transaction*)this, nSerType, nSerVersion, ser_action);
			nSerVersion = this->nVersion;
			READWRITE(hashBlock);
			READWRITE(vMerkleBranch);
			READWRITE(nIndex);
		)


        /** SetMerkleBranch
         *
         *  Populates the merkle branch for this transaction from its containing block. 
         *
         *  @param[in] pblock The containing block, if nullptr method will look it up
         *
         *  @return Depth in chain of block containing this transaction, 0 if not in main chain or merkle branch not set
         *
         **/
		uint32_t SetMerkleBranch(const TAO::Ledger::TritiumBlock* pblock = nullptr);


        /** GetDepthInMainChain
         *
         *  Retrieve the depth in chain of block containing this transaction
         *
         *  @return Depth in chain, 0 if not in main chain
         *
         **/
		uint32_t GetDepthInMainChain() const;


        /** IsInMainChain
         *
         *  Tests whether the block containing this transaction part of the main chain.
         *
         *  @return true if block containing this transaction is in main chain
         *
         **/
		inline bool IsInMainChain() const { return GetDepthInMainChain() > 0; }


        /** GetBlocksToMaturity
         *
         *  Retrieve the number of blocks remaining until transaction outputs are spendable.
         *
         *  @return Blocks remaining until transaction is mature, 
         *          0 if not Coinbase or Coinstake transaction (spendable immediately upon confirm)
         *
         **/
		uint32_t GetBlocksToMaturity() const;


//TODO - Do these require implementation or can they be removed?
		//bool AcceptToMemoryPool(LLD::CIndexDB& indexdb, bool fCheckInputs=true);

		//bool AcceptToMemoryPool();

	};
}

#endif
