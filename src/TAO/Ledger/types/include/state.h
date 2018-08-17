/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BLOCK_STATE_H
#define NEXUS_TAO_LEDGER_TYPES_BLOCK_STATE_H


namespace TAO
{
	namespace Ledger
	{

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

				//for disk operations only
				//READWRITE(hashNextBlock);
			)


			CBlockState() : nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { SetNull(); }

			CBlockState(CBlock blk) : CBlock(blk), nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { }


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

#endif
