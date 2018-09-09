/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

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
		class BlockState : public Block
		{
		public:

			/* The Trust of the Chain to this Block. */
			uint64_t nChainTrust;


			/* The Total NXS released to date */
			uint64_t nMoneySupply;


			/* The height of this channel. */
			uint32_t nChannelHeight;


			/* The reserves that are released. */
			uint32_t nReleasedReserve[2];


			/* The checkpoint this block was made from. */
			uint1024_t hashCheckpoint;


			/* Used to Iterate forward in the chain */
			uint1024_t hashNextBlock;


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


			BlockState() : nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { SetNull(); }

			BlockState(CBlock blk) : CBlock(blk), nChainTrust(0), nMoneySupply(0), nChannelHeight(0), nReleasedReserve(0, 0, 0), hashCheckpoint(0), fConnected(false) { }


			/* Function to determine if this block has been connected into the main chain. */
			bool IsInMainChain() const
			{
				return fConnected;
			}


			/* The hash of this current block state. */
			uint1024_t StateHash() const
			{
				return LLC::SK1024(BEGIN(nVersion), END(hashCheckpoint));
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
