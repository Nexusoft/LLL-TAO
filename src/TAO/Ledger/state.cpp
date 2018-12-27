/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <string>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>



namespace TAO::Ledger
{

    /* Get the block state object. */
    BlockState GetLastState(uint1024_t hashBlock, uint32_t nChannel)
    {

    }


    /* Get the previous block state in chain. */
    bool BlockState::Prev() const
    {

    }


    /* Get the next block state in chain. */
    bool BlockState::Next() const
    {

    }

    /** Function to determine if this block has been connected into the main chain. **/
    bool BlockState::IsInMainChain() const
    {
        return (hashNextBlock != 0 || blockThis.GetHash() == ChainState::hashBestChain);
    }


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
