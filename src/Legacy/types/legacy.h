/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_BASE_BLOCK_H
#define NEXUS_TAO_LEDGER_TYPES_BASE_BLOCK_H

#include <LLC/types/uint1024.h>
#include <Util/macro/header.h>

//forward declerations for BigNum
namespace LLC
{
	class CBigNum;
	class ECKey;
}

namespace Legacy

	/** Block Class
	 *
	 *  Nodes collect new transactions into a block, hash them into a hash tree,
	 *  and scan through nonce values to make the block's hash satisfy validation
	 *  requirements.
	 *
	 */
	class Block
	{
	public:

		/** The blocks version for. Useful for changing rules. **/
		std::vector<LegacyTransaction> vtx;


		//standard serialization methods
		SERIALIZE_HEADER


		/** The default constructor. Sets block state to Null. **/
		Block() { SetNull(); }


		/** A base constructor.
		 *
		 *	@param[in] nVersionIn The version to set block to
		 *	@param[in] hashPrevBlockIn The previous block being linked to
		 *	@param[in] nChannelIn The channel this block is being created for
		 *	@param[in] nHeightIn The height this block is being created at.
		 *
		**/
		Block(uint32_t nVersionIn, uint1024_t hashPrevBlockIn, uint32_t nChannelIn, uint32_t nHeightIn) : nVersion(nVersionIn), hashPrevBlock(hashPrevBlockIn), nChannel(nChannelIn), nHeight(nHeightIn), nBits(0), nNonce(0), nTime(0) { }


		/** Stake Hash
		 *
		 *	Prove that you staked a number of seconds based on weight
		 *
		 *	@return 1024-bit stake hash
		 *
		 **/
		uint1024_t StakeHash();


	};

}

#endif
