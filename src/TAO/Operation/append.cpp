/*_________________________________________________________________________________________________

               (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

               (c) Copyright The Nexus Developers 2014 - 2019

               Distributed under the MIT software license, see the accompanying
               file COPYING or http://www.opensource.org/licenses/mit-license.php.

               "ad vocem populi" - To the Voice of the People

_________________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Writes data to a register. */
        bool Append(TAO::Register::State &state, const std::vector<uint8_t>& vchData)
        {
            /* Check that append is allowed. */
            if(state.nType != TAO::Register::APPEND)
                return debug::error(FUNCTION, "cannot call on non append register");

            /* Append the state data. */
            state.vchState.insert(state.vchState.end(), vchData.begin(), vchData.end());

            /* Update the state register checksum. */
            state.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }
    }
}
