/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/names.h>
#include <TAO/API/include/execute.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>
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
            case TAO::Operation::OP::WRITE:
            {
                /* Grab our register address. */
                TAO::Register::Address hashName;
                if(!TAO::Register::Unpack(rContract, hashName))
                    return;

                /* Execute our contract. */
                TAO::Register::Object tObject =
                    ExecuteContract(rContract);

                /* Parse our object if we need it. */
                if(tObject.nType == TAO::Register::REGISTER::OBJECT)
                    tObject.Parse();

                /* Check for wanted standards. */
                const uint8_t nStandard = tObject.Standard();
                if(nStandard != TAO::Register::OBJECTS::NAME)
                    return; //we don't need to work on any other object

                /* Grab our record now. */
                const TAO::Register::Address hashRecord =
                    tObject.get<uint256_t>("address");

                /* Special pre-state logic. */
                if(nOP == TAO::Operation::OP::WRITE)
                {
                    /* Grab our pre-state. */
                    TAO::Register::Object tPreState =
                        rContract.PreState();

                    /* Parse our object if we need it. */
                    if(tPreState.nType == TAO::Register::REGISTER::OBJECT)
                        tPreState.Parse();

                    /* Get our initial ptr record. */
                    const TAO::Register::Address hashPreState =
                        tPreState.get<uint256_t>("address");

                    /* Check if our ptr record has been updated. */
                    if(hashPreState != hashRecord)
                    {
                        /* Erase our database entry for old record. */
                        if(LLD::Logical->ErasePTR(hashPreState))
                            debug::log(3, "PTR record updated for: ", hashName.ToString());
                    }
                }

                /* Add our PTR record now. */
                if(LLD::Logical->WritePTR(hashRecord, hashName))
                    debug::log(3, "PTR record created for: ", hashRecord.ToString(), " => ", hashName.ToString());

                break;
            }
        }
    }
}
