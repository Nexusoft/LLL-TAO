/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <string>

#include <TAO/Ledger/include/state.h>
#include <TAO/Ledger/types/state.h>

namespace TAO::Ledger
{

    /*-----Definitions for variables declared in include/state.h -----*/
    /* The best block height in the chain. */
    uint32_t nBestHeight;


    /* The best hash in the chain. */
    uint1024_t hashBestChain;


    /* The best trust in the chain. */
    uint64_t nBestChainTrust;


    /* For debugging Purposes seeing block state data dump */
    std::string BlockState::ToString() const
    {
        return std::string(""); //TODO - replace placeholder
    }


    /* For debugging purposes, printing the block to stdout */
    void BlockState::print() const
    {

    }

}
