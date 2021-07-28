/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/names.h>

#include <TAO/Operation/types/contract.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic handler for creating new indexes for this specific command-set. */
    void Names::BuildIndexes(const TAO::Operation::Contract& rContract)
    {
        /* Check the contract's primitive. */
        const uint8_t nOP =
            rContract.Operations()[0];

        debug::log(0, "Handling Contract for OP ", uint32_t(nOP));
    }
}
