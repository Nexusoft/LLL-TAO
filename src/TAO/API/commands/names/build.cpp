/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/names.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Generic handler for creating new indexes for this specific command-set. */
    void Names::BuildIndexes(const TAO::Operation::Contract& rContract)
    {
        /* Get our primitive. */
        rContract.SeekToPrimitive();

        /* Check the contract's primitive. */
        uint8_t nOP = 0;
        rContract >> nOP;

        /* Switch based on operation. */
        switch(nOP)
        {
            /* Handle for CREATE. */
            case TAO::Operation::OP::CREATE:
            case TAO::Operation::OP::CLAIM:
            case TAO::Operation::OP::WRITE:
            {
                
            }
        }


    }
}
