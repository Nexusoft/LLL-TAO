/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Transfers a register between sigchains. */
        bool Transfer(TAO::Register::State &state, const uint256_t& hashTransfer)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTransfer))
                return debug::error(FUNCTION, "cannot transfer register to reserved address");

            /* Check that you aren't sending to yourself. */
            if(state.hashOwner == hashTransfer)
                return debug::error(FUNCTION, state.hashOwner.SubString(), " transfer to self");

            /* Set the new register's owner. */
            state.hashOwner  = 0; //register custody is in SYSTEM ownership until claimed

            /* Set the register checksum. */
            state.SetChecksum();

            /* Check register for validity. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory is in invalid state");

            return true;
        }
    }
}
