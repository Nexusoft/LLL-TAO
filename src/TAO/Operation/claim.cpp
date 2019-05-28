/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Claims a register from a transfer. */
        bool Claim(TAO::Register::State &state, const uint256_t& hashClaim, const uint64_t nTimestamp)
        {
            /* Make sure the register claim is in SYSTEM pending from a transfer. */
            if(state.hashOwner != 0)
                return debug::error(FUNCTION, "can't claim untransferred register");

            /* Set the new owner of the register. */
            state.hashOwner  = hashTransfer;
            state.SetChecksum();

            /* Check register for validity. */
            state.nModified = nTimestamp;
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.SubString(), " is in invalid state");

            return true;
        }
    }
}
