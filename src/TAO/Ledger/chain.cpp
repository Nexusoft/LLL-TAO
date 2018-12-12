/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/chain.h>

namespace TAO::Ledger
{

    /* Break the Chain Age in Minutes into Days, Hours, and Minutes. */
    void GetChainTimes(uint32_t nAge, uint32_t& nDays, uint32_t& nHours, uint32_t& nMinutes)
    {
        nDays = nAge / 1440;
        nHours = (nAge - (nDays * 1440)) / 60;
        nMinutes = nAge % 60;
    }


    /* Get the block state at given hash. */
    bool GetBlockState(uint1024_t hashBlock, BlockState blockState)
    {

    }


    /* Get the previous block state at given hash. */
    bool PrevState(BlockState& blockState)
    {

    }


    /* Get the next block state at given hash. */
    bool NextState(BlockState& blockState)
    {

    }
}
