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

    /* Transfers a register between sigchains. */
    bool Transfer(uint256_t hashAddress, uint256_t hashTransfer, uint256_t hashCaller)
    {
        /* Read the register from the database. */
        TAO::Register::State regState = TAO::Register::State();
        if(!LLD::regDB->ReadState(hashAddress, regState))
            return debug::error(FUNCTION "Register %s doesn't exist in register DB", __PRETTY_FUNCTION__, hashAddress.ToString().c_str());

        /* Make sure that you won the rights to register first. */
        if(regState.hashOwner != hashCaller)
            return debug::error(FUNCTION "%s not authorized to transfer register", __PRETTY_FUNCTION__, hashCaller.ToString().c_str());

        /* Set the new owner of the register. */
        regState.hashOwner = hashTransfer;
        if(!LLD::regDB->WriteState(hashAddress, regState))
            return debug::error(FUNCTION "Failed to write new owner for register", __PRETTY_FUNCTION__);

        return true;
    }

}
