/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Register/include/state.h>

namespace TAO::Operation
{

    /* Creates a new register if it doesn't exist. */
    bool Register(uint256_t hashAddress, uint8_t nType, std::vector<uint8_t> vchData, uint256_t hashCaller)
    {
        /* Check that the register doesn't exist yet. */
        if(LLD::regDB->HasState(hashAddress))
            return debug::error(FUNCTION "cannot allocate register of same memory address %s", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Set the owner of this register. */
        TAO::Register::State state;
        state.nVersion  = 1;
        state.nType     = nType;
        state.hashOwner = hashCaller;
        state << vchData;

        /* Check the state change is correct. */
        if(!state.IsValid())
            return debug::error(FUNCTION "memory address %s is in invalid state", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Write the register to database. */
        if(!LLD::regDB->WriteState(hashAddress, state))
            return debug::error(FUNCTION "failed to write state register %s memory address", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        return true;
    }
}
