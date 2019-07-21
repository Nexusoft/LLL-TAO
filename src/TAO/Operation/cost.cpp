/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Man often becomes what he believes himself to be." - Mahatma Gandhi

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/types/script.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/cost.h>

#include <TAO/Operation/include/constants.h>
#include <TAO/Operation/types/contract.h>
#include <TAO/Operation/types/condition.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/constants.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

namespace TAO
{

    namespace Operation
    {

        /* Executes a given operation byte sequence. */
        void Cost(const Contract& contract, uint64_t &nCost)
        {
            /* Reset the contract streams. */
            contract.Reset();

            /* Get the contract OP. */
            uint8_t nOP = 0;
            contract >> nOP;

            /* Check the current opcode. */
            switch(nOP)
            {
                /* Condition that allows a validation to occur. */
                case OP::CONDITION:
                {
                    /* Condition has no parameters. */
                    contract >> nOP;

                    /* Check for valid primitives that can have a condition. */
                    switch(nOP)
                    {
                        /* Transfer and debit are the only permitted. */
                        case OP::TRANSFER:
                        case OP::DEBIT:
                        {
                            //transfer and debit are permitted
                            break;
                        }
                    }

                    break;
                }


                /* Validate a previous contract's conditions */
                case OP::VALIDATE:
                {
                    /* Extract the transaction from contract. */
                    uint512_t hashTx = 0;
                    contract >> hashTx;

                    /* Extract the contract-id. */
                    uint32_t nContract = 0;
                    contract >> nContract;

                    /* Verify the operation rules. */
                    const Contract condition = LLD::Ledger->ReadContract(hashTx, nContract);

                    /* Build the validation script for execution. */
                    Condition conditions = Condition(condition, contract);
                    conditions.Execute();

                    /* Assess the fees for the computation limits. */
                    if(conditions.nCost > CONDITION_LIMIT_FREE)
                        nCost += (conditions.nCost - CONDITION_LIMIT_FREE);

                    /* Get next OP. */
                    contract >> nOP;

                    break;
                }
            }


            /* Check the current opcode. */
            switch(nOP)
            {

                case OP::CREATE:
                {
                    /* Get the Address of the Register. */
                    TAO::Register::Address hashAddress;
                    contract >> hashAddress;

                    /* Get the Register Type. */
                    uint8_t nType = 0;
                    contract >> nType;

                    /* Check for object type. */
                    if(nType != TAO::Register::REGISTER::OBJECT)
                        return;

                    /* Get the register data. */
                    std::vector<uint8_t> vchData;
                    contract >> vchData;

                    /* Create the register object. */
                    TAO::Register::Object object;
                    object.nVersion   = 1;
                    object.nType      = nType;
                    object.hashOwner  = contract.Caller();
                    object.SetState(vchData);

                    /* Parse the object. */
                    if(!object.Parse())
                        throw debug::exception(FUNCTION, "malformed object register");

                    /* Set the fee cost to the contract. */
                    nCost += object.Cost();

                    break;
                }


                /* Transfer ownership of a register to another signature chain. */
                case OP::CLAIM:
                {
                    /* Extract the transaction from contract. */
                    uint512_t hashTx = 0;
                    contract >> hashTx;

                    /* Extract the contract-id. */
                    uint32_t nContract = 0;
                    contract >> nContract;

                    /* Extract the address from the contract. */
                    uint256_t hashAddress = 0;
                    contract >> hashAddress;

                    /* Verify the operation rules. */
                    const Contract transfer = LLD::Ledger->ReadContract(hashTx, nContract);
                    if(!transfer.Empty(Contract::CONDITIONS))
                    {
                        /* Check for condition. */
                        Condition conditions = Condition(transfer, contract);
                        conditions.Execute();

                        /* Assess the fees for the computation limits. */
                        if(conditions.nCost > CONDITION_LIMIT_FREE)
                            nCost += (conditions.nCost - CONDITION_LIMIT_FREE);
                    }

                    break;
                }


                /* Credit tokens to an account you own. */
                case OP::CREDIT:
                {
                    /* Extract the transaction from contract. */
                    uint512_t hashTx = 0;
                    contract >> hashTx;

                    /* Extract the contract-id. */
                    uint32_t nContract = 0;
                    contract >> nContract;

                    /* Verify the operation rules. */
                    const Contract debit = LLD::Ledger->ReadContract(hashTx, nContract);
                    if(!debit.Empty(Contract::CONDITIONS))
                    {
                        /* Check for condition. */
                        Condition conditions = Condition(debit, contract);
                        conditions.Execute();

                        /* Assess the fees for the computation limits. */
                        if(conditions.nCost > CONDITION_LIMIT_FREE)
                            nCost += (conditions.nCost - CONDITION_LIMIT_FREE);
                    }
                }
            }
        }
    }
}
