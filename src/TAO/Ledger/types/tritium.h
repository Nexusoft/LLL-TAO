/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_TRITIUM_H
#define NEXUS_TAO_LEDGER_TYPES_TRITIUM_H

#include <TAO/Ledger/types/block.h>

#include <Util/templates/serialize.h>

namespace TAO::Ledger
{

	/** Tritium Block
	 *
     *  A tritium block contains referecnes to the transactions in blocks.
     *  These are used to build the merkle tree for checking.
     *  Transactions are processed before block is recieved, and commit
     *  When a block is recieved to break up processing requirements.
     *
     **/
	class TritiumBlock : public Block
	{
	public:

		/** The transaction history. **/
        std::vector<uint512_t> vtx;


		IMPLEMENT_SERIALIZE
		(
            READWRITE(nVersion);
            READWRITE(hashPrevBlock);
            READWRITE(hashMerkleRoot);
            READWRITE(nChannel);
            READWRITE(nHeight);
            READWRITE(nBits);
            READWRITE(nNonce);
            READWRITE(nTime);
            READWRITE(vchBlockSig);

			READWRITE(vtx);
		)


        /** The default constructor. **/
		TritiumBlock()  { SetNull(); }


		/** ToString
         *
         *  For debugging Purposes seeing block state data dump
         *
         **/
		std::string ToString() const;


		/** print
         *
         *  For debugging purposes, printing the block to stdout
         *
         **/
		void print() const;
	};

}

#endif
