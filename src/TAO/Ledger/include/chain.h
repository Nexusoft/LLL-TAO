/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_CHAIN_H
#define NEXUS_TAO_LEDGER_INCLUDE_CHAIN_H

#include <LLC/types/bignum.h>

namespace TAO::Ledger
{

    /** Get Chain Times
     *
     *  Break the Chain Age in Minutes into Days, Hours, and Minutes.
     *
     *  @param[in] nAge The age in total seconds.
     *  @param[out] nDays The number of days return.
     *  @param[out] nHours The number of hours past on day mark.
     *  @param[out] nMinutes The number of minutes past hour mark.
     *
     **/
    void GetChainTimes(uint32_t nAge, uint32_t& nDays, uint32_t& nHours, uint32_t& nMinutes);



    /** Get Block State
     *
     *  Get the block state at given hash.
     *
     *  @param[in] hashBlock The hash of block to lookup.
     *  @param[out] blockState The block state object.
     *
     *  @return true if the block state was found.
     *
     **/
     bool GetBlockState(uint1024_t hashBlock, BlockState blockState);
}

#endif
