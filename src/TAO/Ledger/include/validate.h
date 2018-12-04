/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_VALIDATE_H
#define NEXUS_TAO_LEDGER_INCLUDE_VALIDATE_H

namespace TAO::Ledger
{

    /** Check Block
     *
     *  Checks if a block is valid if not connected to chain.
     *
     *  @param[in] block The block to check.
     *
     *  @return true if the block is valid.
     *
     **/
    template<typename BlockType> bool CheckBlock(BlockType block);

}


#endif
