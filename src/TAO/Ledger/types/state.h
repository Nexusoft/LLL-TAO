/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_STATE_H
#define NEXUS_TAO_LEDGER_TYPES_STATE_H


namespace TAO::Ledger
{

	/** Block State Class
	 *
	 *  This class is responsible for storing state variables
	 *  that are chain specific for a specified block. These
	 *  values are not recognized until this block is linked
	 *  in the chain with all previous states before it.
	 *
	 **/
	class BlockState
	{
	public:

		/** The tritum block this is holding state for. **/
		TritiumBlock blockThis;


		/** The Trust of the Chain to this Block. */
		uint64_t nChainTrust;


		/** The Total NXS released to date */
		uint64_t nMoneySupply;


		/** The height of this channel. */
		uint32_t nChannelHeight;


		/** The reserves that are released. */
		uint32_t nReleasedReserve[2];


		/** Used to Iterate forward in the chain */
		uint1024_t hashNextBlock;


		/** Used to store the checkpoint. **/
		uint1024_t hashCheckpoint;


		/* Serialization Macros */
		IMPLEMENT_SERIALIZE
		(
			READWRITE(blockThis);
			READWRITE(nChainTrust);
			READWRITE(nMoneySupply);
			READWRITE(nChannelHeight);
			READWRITE(nReleasedReserve[0]);
			READWRITE(nReleasedReserve[1]);
			READWRITE(nReleasedReserve[2]);

			//for disk operations only
			READWRITE(hashNextBlock);
			READWRITE(hashCheckpoint);
		)


		BlockState() :
		nChainTrust(0),
		nMoneySupply(0),
		nChannelHeight(0),
		nReleasedReserve(0, 0, 0)
		{
			SetNull();
		}


		BlockState(TritiumBlock blockIn) :
		blockThis(blockIn),
		nChainTrust(0),
		nMoneySupply(0),
		nChannelHeight(0),
		nReleasedReserve(0, 0, 0)
		{
		}


		BlockState(LegacyBlock blockIn) :
		blockThis(blockIn),
		nChainTrust(0),
		nMoneySupply(0),
		nChannelHeight(0),
		nReleasedReserve(0, 0, 0)
		{

		}


		/* Function to determine if this block has been connected into the main chain. */
		bool IsInMainChain() const
		{
			return (hashNextBlock != 0 || GetHash() == hashBestChain);
		}


		/* For debugging Purposes seeing block state data dump */
		std::string ToString() const;


		/* For debugging purposes, printing the block to stdout */
		void print() const;
	};

}

#endif
