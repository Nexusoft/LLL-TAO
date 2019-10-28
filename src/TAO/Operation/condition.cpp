/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/types/condition.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/exception.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>

#include <cmath>
#include <limits>
#include <stack>

namespace TAO
{

    namespace Operation
    {

        /* Copy constructor. */
        Condition::Condition(const Condition& condition)
        : TAO::Register::BaseVM (condition)
        , contract              (condition.contract)
        , caller                (condition.caller)
        , vEvaluate             (condition.vEvaluate)
        , nCost                 (condition.nCost)
        {
        }


        /* Move constructor. */
        Condition::Condition(Condition&& condition) noexcept
        : TAO::Register::BaseVM (std::move(condition))
        , contract              (std::move(condition.contract))
        , caller                (std::move(condition.caller))
        , vEvaluate             (std::move(condition.vEvaluate))
        , nCost                 (std::move(condition.nCost))
        {
        }


        /* Default Destructor */
        Condition::~Condition()
        {
        }


        Condition::Condition(const Contract& contractIn, const Contract& callerIn, const int64_t nCostIn)
        : TAO::Register::BaseVM ( ) //512 bytes of register memory.
        , contract              (contractIn)
        , caller                (callerIn)
        , vEvaluate             ( )
        , nCost                 (nCostIn)
        {
            /* Push base group, which is what contains final return value. */
            vEvaluate.push(std::make_pair(false, OP::RESERVED));
        }


        /* Reset the validation script for re-executing. */
        void Condition::Reset()
        {
            /* Reset the evaluation stack. */
            vEvaluate.top().first  = false;
            vEvaluate.top().second = OP::RESERVED;

            contract.Reset();
            nCost = 0;

            reset();
        }


        /* Verifies that the contract conditions script is not malformed and can be executed. */
        bool Condition::Verify(const Contract& contract)
        {
            /* Warnings raised */
            std::vector<std::pair<uint16_t, uint64_t>> vWarnings;

            /* Validate the conditions, ignoring any warnings */
            return Verify(contract, vWarnings);
        }


        /* Verifies that the contract conditions script is not malformed and can be executed.  Populates vWarnings with
           warning flags and stream positions, which the caller can use to identify potential problems such as overflows. */
        bool Condition::Verify(const Contract& contract, std::vector<std::pair<uint16_t, uint64_t>> &vWarnings)
        {
            /* Return flag */
            bool fValid = false;

            /* Always return true if there are no conditions to verify! */
            if(contract.Empty(TAO::Operation::Contract::CONDITIONS))
                return true;

            try
            {
                /* Create a dummy caller contract so that we can use it to execute the conditions.  We don't need to give this 
                   dummy caller a valid operations stream as we are only checking for malformed conditions and overflows, so instead
                   we can create the operations stream with 1024 bytes of data set to 0x02 (we use 0x02 so that both ADD and MUL 
                   conditions based on the caller operations data will result in a overflow) */
                TAO::Operation::Contract caller;

                /* Fill contract's operations stream with 1024 bytes all set to 0x02*/
                for(int i=0; i<1024; i++)
                    caller << uint8_t(0x02);

                /* Create a conditions object for the the two contracts */
                Condition condition(contract, caller);

                /* Finally try to execute the contract */
                condition.execute(vWarnings);

                /* If there were any warnings then log them */
                for(const auto& warning : vWarnings)
                    debug::log(2, "Condition warning: ", WarningToString(warning.first), " at position ", warning.second);

                /* If we made it through the execute call without throwing an exception then the condition must be valid */
                fValid = true;

            }
            catch(const TAO::Register::BaseVMException& e)
            {
                /* Set return flag to false as an exception was thrown */
                fValid = false;

                /* Handle intentionally thrown BaseVMExceptions as we need execution to continue after logging the error */
                return debug::error("Condition validation failed: ", e.what(), " at position ", contract.Position());
            }

            return fValid;
        }


        /* Execute the validation script. */
        bool Condition::Execute()
        {
            /* Warnings raised */
            std::vector<std::pair<uint16_t, uint64_t>> vWarnings;

            /* Execute the conditions.  NOTE: we ignore warnings during runtime execution */
            return execute(vWarnings);
        }

        /* Execute the validation script. */
        bool Condition::execute(std::vector<std::pair<uint16_t, uint64_t>> &vWarnings)
        {
            /* Warnings bitmask passed into each evaluate call. */
            uint16_t nWarnings = WARNINGS::NONE;

            /* Ensure the warnings list is cleared */
            vWarnings.clear();

            /* Loop through the operation validation code. */
            contract.Reset(Contract::CONDITIONS);
            while(!contract.End(Contract::CONDITIONS))
            {
                /* Grab the next operation. */
                uint8_t OPERATION = 0;
                contract >= OPERATION;

                /* Switch by operation code. */
                switch(OPERATION)
                {

                    /* Handle for the && operator. */
                    case OP::GROUP:
                    {
                        /* When grouping, add another group layer. */
                        vEvaluate.push(std::make_pair(false, OP::RESERVED));

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw TAO::Register::MalformedException("Malformed condition. OP::GROUP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Handle for the && operator. */
                    case OP::UNGROUP:
                    {
                        /* Check that nothing has been evaluated. */
                        if(vEvaluate.empty())
                            throw TAO::Register::MalformedException("Malformed condition. Missing OP::GROUP");

                        /* Check for evalute state. */
                        bool fEvaluate = vEvaluate.top().first;

                        /* Pop last group from stack. */
                        vEvaluate.pop();
                        switch(vEvaluate.top().second)
                        {
                            /* Handle if this is our first OP. */
                            case OP::RESERVED:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = fEvaluate;

                                break;
                            }

                            /* Handle logical AND operator. */
                            case OP::AND:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (vEvaluate.top().first && fEvaluate);

                                break;
                            }

                            /* Handle logical OR operator. */
                            case OP::OR:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (vEvaluate.top().first || fEvaluate);

                                break;
                            }

                            default:
                                throw TAO::Register::MalformedException("Malformed condition. Conditional type is unsupported");
                        }

                        break;
                    }


                    /* Handle for the && operator. */
                    case OP::AND:
                    {
                        /* Check for an operation. */
                        if(vEvaluate.empty())
                            throw TAO::Register::MalformedException("Malformed condition. OP::AND missing previous OP::UNGROUP");

                        /* Check that evaluate is default value. */
                        if(vEvaluate.top().second == OP::OR)
                            throw TAO::Register::MalformedException("Malformed condition. Cannot evaluate OP::AND with previous OP::OR");

                        /* Set the new evaluate state. */
                        vEvaluate.top().second = OP::AND;

                        break;
                    }


                    /* Handle for the || operator. */
                    case OP::OR:
                    {
                        /* Check for an operation. */
                        if(vEvaluate.empty())
                            throw TAO::Register::MalformedException("Malformed condition. OP::OR missing previous OP::UNGROUP");

                        /* Check that evaluate is default value. */
                        if(vEvaluate.top().second == OP::AND)
                            throw TAO::Register::MalformedException("Malformed condition. Cannot evaluate OP::OR with previous OP::AND");

                        /* Set the new evaluate state. */
                        vEvaluate.top().second = OP::OR;

                        break;
                    }


                    /* For non specific operations, resort to evaluate. */
                    default:
                    {
                        /* If OP is unknown, evaluate. */
                        contract.Rewind(1);

                        /* Check that nothing has been evaluated. */
                        if(vEvaluate.empty())
                            throw TAO::Register::MalformedException("Malformed conditions");

                        /* Check for evalute state. */
                        switch(vEvaluate.top().second)
                        {
                            /* Handle if this is our first OP. */
                            case OP::RESERVED:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = evaluate(nWarnings);

                                break;
                            }

                            /* Handle logical AND operator. */
                            case OP::AND:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (evaluate(nWarnings) && vEvaluate.top().first);

                                break;
                            }

                            /* Handle logical OR operator. */
                            case OP::OR:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (evaluate(nWarnings) || vEvaluate.top().first);

                                break;
                            }

                            default:
                                throw TAO::Register::MalformedException("Malformed condition. Condition type is unsupported");
                        }

                        /* If any warnings were encountered then add them to the list */
                        if(nWarnings != WARNINGS::NONE)
                            vWarnings.push_back(std::make_pair(nWarnings, contract.Position()));

                        break;
                    }
                }
            }

            /* Check the values in groups. */
            if(vEvaluate.size() != 1)
                throw TAO::Register::MalformedException("Malformed condition. Evaluate groups count incomplete");

            /* Return final value. */
            return vEvaluate.top().first;

            
        }


        /* Execute the validation script. */
        bool Condition::evaluate(uint16_t &nWarnings)
        {
            /* Flag to tell how it evaluated. */
            bool fRet = false;

            /* Reset the warnings flag */
            nWarnings = WARNINGS::NONE;

            /* Grab the first value */
            TAO::Register::Value vLeft;
            if(!get_value(vLeft, nWarnings))
                return false;

            /* Ensure there is am operation after the lValue */
            if(contract.End(Contract::CONDITIONS))
                throw TAO::Register::MalformedException("Malformed conditions");

            /* Grab the next operation. */
            uint8_t OPERATION = 0;
            contract >= OPERATION;

            /* Switch by operation code. */
            switch(OPERATION)
            {
                /* Handle for the == operator. */
                case OP::EQUALS:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) == 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle for < operator. */
                case OP::LESSTHAN:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) < 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle for the > operator. */
                case OP::GREATERTHAN:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) > 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle for <= operator. */
                case OP::LESSEQUALS:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) <= 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle for the >= operator. */
                case OP::GREATEREQUALS:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) >= 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle for the != operator. */
                case OP::NOTEQUALS:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = (compare(vLeft, vRight) != 0);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }


                /* Handle to check if a sequence of bytes is inside another. */
                case OP::CONTAINS:
                {
                    /* Grab the second value. */
                    TAO::Register::Value vRight;
                    if(!get_value(vRight, nWarnings))
                        return false;

                    /* Compare both values to one another. */
                    fRet = contains(vLeft, vRight);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }

                /* For unknown codes, always fail. */
                default:
                    throw TAO::Register::MalformedException("Malformed conditions");
            }

            /* Return final response. */
            return fRet;
            
        }


        /** get value
         *
         *  Get a value from the register virtual machine.
         *
         **/
        bool Condition::get_value(TAO::Register::Value& vRet, uint16_t &nWarnings)
        {
            /* Iterate until end of stream. */
            while(!contract.End(Contract::CONDITIONS))
            {
                /* Extract the operation byte. */
                uint8_t OPERATION = 0;
                contract >= OPERATION;

                /* Switch based on the operation. */
                switch(OPERATION)
                {

                    /* Add two 64-bit numbers. */
                    case OP::ADD:
                    {
                        /* Get the add from r-value. */
                        TAO::Register::Value vAdd;
                        if(!get_value(vAdd, nWarnings))
                            return false; //("OP::ADD failed to get r-value");

                        /* Check computational bounds. */
                        if(vAdd.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::ADD computation greater than 64-bits");

                        /* Check for overflows. */
                        if(at(vRet) + at(vAdd) < at(vRet))
                            nWarnings |= WARNINGS::ADD_OVERFLOW; ;

                        /* Compute the return value. */
                        at(vRet) += at(vAdd);

                        /* Deallocate r-value from memory. */
                        deallocate(vAdd);

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw TAO::Register::MalformedException("OP::ADD costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Subtract one number from another. */
                    case OP::SUB:
                    {
                        /* Get the sub from r-value. */
                        TAO::Register::Value vSub;
                        if(!get_value(vSub, nWarnings))
                            return false; //("OP::SUB failed to get r-value");

                        /* Check computational bounds. */
                        if(vSub.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::SUB computation greater than 64-bits");

                        /* Check for overflows. */
                        if(at(vRet) - at(vSub) > at(vRet))
                            nWarnings |= WARNINGS::SUB_OVERFLOW ;

                        /* Compute the return value. */
                        at(vRet) -= at(vSub);

                        /* Deallocate r-value from memory. */
                        deallocate(vSub);

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw TAO::Register::MalformedException("OP::SUB costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Increment a number by an order of 1. */
                    case OP::INC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::INC computation greater than 64-bits");

                        /* Compute the return value. */
                        if(++at(vRet) == 0)
                            nWarnings |= WARNINGS::INC_OVERFLOW;

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw TAO::Register::MalformedException("OP::INC costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* De-increment a number by an order of 1. */
                    case OP::DEC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::DEC computation greater than 64-bits");

                        /* Compute the return value. */
                        if(--at(vRet) == std::numeric_limits<uint64_t>::max())
                            nWarnings |= WARNINGS::DEC_OVERFLOW;

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw TAO::Register::MalformedException("OP::DEC costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Divide a number by another. */
                    case OP::DIV:
                    {
                        /* Get the divisor from r-value. */
                        TAO::Register::Value vDiv;
                        if(!get_value(vDiv, nWarnings))
                            return false; //("OP::DIV failed to get r-value");

                        /* Check computational bounds. */
                        if(vDiv.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::DIV computation greater than 64-bits");

                        /* Check for exceptions. */
                        if(at(vDiv) == 0)
                            nWarnings |= WARNINGS::DIV_BY_ZERO;
                        else
                            /* Compute the return value. */
                            at(vRet) /= at(vDiv);

                        /* Deallocate r-value from memory. */
                        deallocate(vDiv);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw TAO::Register::MalformedException("OP::DIV costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Multiply a number by another. */
                    case OP::MUL:
                    {
                        /* Get the multiplier from r-value. */
                        TAO::Register::Value vMul;
                        if(!get_value(vMul, nWarnings))
                            return false; //("OP::MUL failed to get r-value");

                        /* Check computational bounds. */
                        if(vMul.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::MUL computation greater than 64-bits");

                        /* Check for value overflows. */
                        if(at(vMul) != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / at(vMul))
                            nWarnings |= WARNINGS::MUL_OVERFLOW;

                        /* Compute the return value. */
                        at(vRet) *= at(vMul);

                        /* Deallocate r-value from memory. */
                        deallocate(vMul);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw TAO::Register::MalformedException("OP::MUL costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Raise a number by the power of another. */
                    case OP::EXP:
                    {
                        /* Get the exponent from r-value. */
                        TAO::Register::Value vExp;
                        if(!get_value(vExp, nWarnings))
                            return false; //("OP::EXP failed to get r-value");

                        /* Check computational bounds. */
                        if(vExp.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::EXP computation greater than 64-bits");

                        /* Catch for a power of 0. */
                        if(at(vExp) == 0)
                            at(vRet) = 1;

                        /* Exponentiate by integers. */
                        uint64_t nBase = at(vRet);
                        for(uint64_t e = 1; e < at(vExp); ++e)
                        {
                            /* Check for value overflows. */
                            if(nBase != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / nBase)
                                nWarnings |= WARNINGS::EXP_OVERFLOW;

                            /* Assign the return value. */
                            at(vRet) *= nBase;
                        }

                        /* Deallocate r-value from memory. */
                        deallocate(vExp);

                        /* Check for overflows. */
                        if(nCost + 256 < nCost)
                            throw TAO::Register::MalformedException("OP::EXP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 256;

                        break;
                    }


                    /* Get the remainder after a division. */
                    case OP::MOD:
                    {
                        /* Get the modulus from r-value. */
                        TAO::Register::Value vMod;
                        if(!get_value(vMod, nWarnings))
                            return false; //("OP::MOD failed to get r-value");

                        /* Check computational bounds. */
                        if(vMod.size() > 1 || vRet.size() > 1)
                            throw TAO::Register::MalformedException("OP::MOD computation greater than 64-bits");

                        /* Check for exceptions. */
                        if(at(vMod) == 0)
                            nWarnings |= WARNINGS::MOD_DIV_ZERO;
                        else
                            /* Compute the return value. */
                            at(vRet) %= at(vMod);

                        /* Deallocate r-value from memory. */
                        deallocate(vMod);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw TAO::Register::MalformedException("OP::MOD costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Parse out subdata from bytes. */
                    case OP::SUBDATA:
                    {
                        /* Get the beginning iterator. */
                        uint16_t nBegin = 0;
                        contract >= nBegin;

                        /* Get the size to extract. */
                        uint16_t nSize = 0;
                        contract >= nSize;

                        /* Extract the string. */
                        std::vector<uint8_t> vData(vRet.size() * 8, 0);
                        deallocate(vData, vRet);

                        /* Set the register value. */
                        std::vector<uint8_t> vAlloc(vData.begin() + nBegin, vData.begin() + nBegin + nSize);
                        allocate(vAlloc, vRet);

                        /* Check for overflows. */
                        if(nCost + vData.size() < nCost)
                            throw TAO::Register::MalformedException("OP::SUBDATA costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += vData.size();

                        break;
                    }


                    /* Parse out subdata from bytes. */
                    case OP::CAT:
                    {
                        /* Get the add from r-value. */
                        TAO::Register::Value vCat;
                        if(!get_value(vCat, nWarnings))
                            return false; //("OP::CAT failed to get r-value");

                        /* Extract the string. */
                        std::vector<uint8_t> vAlloc;
                        vAlloc.insert(vAlloc.end(), (uint8_t*)begin(vRet), (uint8_t*)end(vRet));
                        vAlloc.insert(vAlloc.end(), (uint8_t*)begin(vCat), (uint8_t*)end(vCat));

                        /* Deallocate values. */
                        deallocate(vCat);
                        deallocate(vRet);

                        /* Allocate concatenated data. */
                        allocate(vAlloc, vRet);

                        /* Check for overflows. */
                        if(nCost + vCat.size() < nCost)
                            throw TAO::Register::MalformedException("OP::CAT costs value overflow");

                        /* Adjust the costs. */
                        nCost += vCat.size();

                        break;
                    }



                    /* Extract an uint8_t from the stream. */
                    case OP::TYPES::UINT8_T:
                    {
                        /* Extract the byte. */
                        uint8_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 1 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT8_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 1;

                        break;
                    }


                    /* Extract an uint16_t from the stream. */
                    case OP::TYPES::UINT16_T:
                    {
                        /* Extract the short. */
                        uint16_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 2 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT16_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 2;

                        break;
                    }


                    /* Extract an uint32_t from the stream. */
                    case OP::TYPES::UINT32_T:
                    {
                        /* Extract the integer. */
                        uint32_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 4 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT32_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 4;

                        break;
                    }


                    /* Extract an uint64_t from the stream. */
                    case OP::TYPES::UINT64_T:
                    {
                        /* Extract the integer. */
                        uint64_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 8 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT64_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Extract an uint256_t from the stream. */
                    case OP::TYPES::UINT256_T:
                    {
                        /* Extract the integer. */
                        uint256_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 32 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT256_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 32;

                        break;

                    }


                    /* Extract an uint512_t from the stream. */
                    case OP::TYPES::UINT512_T:
                    {
                        /* Extract the integer. */
                        uint512_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT512_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Extract an uint1024_t from the stream. */
                    case OP::TYPES::UINT1024_T:
                    {
                        /* Extract the integer. */
                        uint1024_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::UINT1024_T costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Extract a string from the stream. */
                    case OP::TYPES::STRING:
                    {
                        /* Extract the string. */
                        std::string str;
                        contract >= str;

                        /* Check for empty string. */
                        if(str.empty())
                            throw TAO::Register::MalformedException("OP::TYPES::STRING string is empty");

                        /* Set the register value. */
                        allocate(str, vRet);

                        /* Check for overflows. */
                        uint32_t nSize = str.size();
                        if(nCost + nSize < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::STRING costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += nSize;

                        break;
                    }


                    /* Extract bytes from the stream. */
                    case OP::TYPES::BYTES:
                    {
                        /* Extract the string. */
                        std::vector<uint8_t> vData;
                        contract >= vData;

                        /* Check for empty string. */
                        if(vData.empty())
                            throw TAO::Register::MalformedException("OP::TYPES::BYTES vector is empty");

                        /* Set the register value. */
                        allocate(vData, vRet);

                        /* Check for overflows. */
                        uint32_t nSize = vData.size();
                        if(nCost + nSize < nCost)
                            throw TAO::Register::MalformedException("OP::TYPES::BYTES costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += nSize;

                        break;
                    }


                    /* Get a register's timestamp and push to the return value. */
                    case OP::CALLER::PRESTATE::MODIFIED:
                    case OP::REGISTER::MODIFIED:
                    {
                        /* Register state object. */
                        TAO::Register::State state;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::MODIFIED:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, state))
                                    throw TAO::Register::MalformedException("OP::REGISTER::MODIFIED register not found");

                                /* Check for overflows. */
                                if(nCost + 5004 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::MODIFIED costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 5004;

                                break;
                            }

                            case OP::CALLER::PRESTATE::MODIFIED:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= state;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Check for overflows. */
                                if(nCost + 8 < nCost)
                                    throw TAO::Register::MalformedException("OP::CALLER::PRESTATE::MODIFIED costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 8;

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.nModified, vRet);

                        break;
                    }


                    /* Get a register's timestamp and push to the return value. */
                    case OP::CALLER::PRESTATE::CREATED:
                    case OP::REGISTER::CREATED:
                    {
                        /* Register state object. */
                        TAO::Register::State state;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::CREATED:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, state))
                                    throw TAO::Register::MalformedException("OP::REGISTER::CREATED register not found");

                                /* Check for overflows. */
                                if(nCost + 5004 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::CREATED costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 5004;

                                break;
                            }

                            case OP::CALLER::PRESTATE::CREATED:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= state;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Check for overflows. */
                                if(nCost + 8 < nCost)
                                    throw TAO::Register::MalformedException("OP::CALLER::PRESTATE::CREATED costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 8;

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.nCreated, vRet);

                        break;
                    }


                    /* Get a register's owner and push to the return value. */
                    case OP::CALLER::PRESTATE::OWNER:
                    case OP::REGISTER::OWNER:
                    {
                        /* Register state object. */
                        TAO::Register::State state;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::OWNER:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, state))
                                    throw TAO::Register::MalformedException("OP::REGISTER::OWNER register not found");

                                /* Check for overflows. */
                                if(nCost + 4128 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::OWNER costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 4128;

                                break;
                            }

                            case OP::CALLER::PRESTATE::OWNER:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= state;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Check for overflows. */
                                if(nCost + 32 < nCost)
                                    throw TAO::Register::MalformedException("OP::CALLER::PRESTATE::OWNER costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 32;

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.hashOwner, vRet);

                        break;
                    }


                    /* Get a register's type and push to the return value. */
                    case OP::CALLER::PRESTATE::TYPE:
                    case OP::REGISTER::TYPE:
                    {
                        /* Register state object. */
                        TAO::Register::State state;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::TYPE:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, state))
                                    throw TAO::Register::MalformedException("OP::REGISTER::TYPE register not found");

                                /* Check for overflows. */
                                if(nCost + 4097 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::TYPE costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 4097;

                                break;
                            }

                            case OP::CALLER::PRESTATE::TYPE:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= state;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Check for overflows. */
                                if(nCost + 1 < nCost)
                                    throw TAO::Register::MalformedException("OP::CALLER::PRESTATE::TYPE costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 1;

                                break;
                            }
                        }

                        /* Push the type onto the return value. */
                        allocate(state.nType, vRet);

                        break;
                    }


                    /* Get a register's state and push to the return value. */
                    case OP::CALLER::PRESTATE::STATE:
                    case OP::REGISTER::STATE:
                    {
                        /* Register state object. */
                        TAO::Register::State state;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::STATE:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, state))
                                    throw TAO::Register::MalformedException("OP::REGISTER::STATE register not found");

                                /* Check for overflows. */
                                uint32_t nSize = 4096 + state.GetState().size();
                                if(nCost + nSize < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::STATE costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += nSize;

                                break;
                            }

                            case OP::CALLER::PRESTATE::STATE:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= state;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Check for overflows. */
                                uint32_t nSize = state.GetState().size();
                                if(nCost + nSize < nCost)
                                    throw TAO::Register::MalformedException("OP::CALLER::PRESTATE::STATE costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += nCost;

                                break;
                            }
                        }

                        /* Allocate to the registers. */
                        allocate(state.GetState(), vRet);

                        break;
                    }


                    /* Get an account register's balance and push to the return value. */
                    case OP::CALLER::PRESTATE::VALUE:
                    case OP::REGISTER::VALUE:
                    {
                        /* Register state object. */
                        TAO::Register::Object object;

                        /* Check for register enum. */
                        switch(OPERATION)
                        {
                            case OP::REGISTER::VALUE:
                            {
                                /* Read the register address. */
                                uint256_t hashRegister;
                                deallocate(hashRegister, vRet);

                                /* Read the register states. */
                                if(!LLD::Register->ReadState(hashRegister, object))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE register not found");

                                /* Check for overflows. */
                                if(nCost + 4096 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 4096;

                                break;
                            }

                            case OP::CALLER::PRESTATE::VALUE:
                            {
                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                /* Read the pre-state state. */
                                uint8_t nState = 0;
                                caller >>= nState;

                                /* Get the pre-state. */
                                caller >>= object;

                                /* Reset the contract. */
                                caller.Reset(Contract::REGISTERS);

                                break;
                            }
                        }

                        /* Get the value string. */
                        std::string strValue;
                        contract >= strValue;

                        /* Check for object register type. */
                        if(object.nType != TAO::Register::REGISTER::OBJECT)
                            throw TAO::Register::MalformedException("OP::REGISTER::VALUE register is not an object");

                        /* Parse the object register. */
                        if(!object.Parse())
                            throw TAO::Register::MalformedException("OP::REGISTER::VALUE could not parse object");

                        /* Get the supported type enumeration. */
                        uint8_t nType;
                        if(!object.Type(strValue, nType))
                            throw TAO::Register::MalformedException("OP::REGISTER::VALUE could not get field type");

                        /* Switch supported types. */
                        switch(nType)
                        {
                            /* Standard type for C++ uint8_t. */
                            case TAO::Register::TYPES::UINT8_T:
                            {
                                /* Read the value. */
                                uint8_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT8_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 1 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT8_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 1;

                                break;
                            }


                            /* Standard type for C++ uint16_t. */
                            case TAO::Register::TYPES::UINT16_T:
                            {
                                /* Read the value. */
                                uint16_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT16_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 2 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT16_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 2;

                                break;
                            }


                            /* Standard type for C++ uint32_t. */
                            case TAO::Register::TYPES::UINT32_T:
                            {
                                /* Read the value. */
                                uint32_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT32_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 4 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT32_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 4;

                                break;
                            }


                            /* Standard type for C++ uint64_t. */
                            case TAO::Register::TYPES::UINT64_T:
                            {
                                /* Read the value. */
                                uint64_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT64_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 8 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT64_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 8;

                                break;
                            }


                            /* Standard type for Custom uint256_t */
                            case TAO::Register::TYPES::UINT256_T:
                            {
                                /* Read the value. */
                                uint256_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT256_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 32 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT256_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 32;

                                break;
                            }


                            /* Standard type for Custom uint512_t */
                            case TAO::Register::TYPES::UINT512_T:
                            {
                                /* Read the value. */
                                uint512_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT512_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 64 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT512_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 64;

                                break;
                            }


                            /* Standard type for Custom uint1024_t */
                            case TAO::Register::TYPES::UINT1024_T:
                            {
                                /* Read the value. */
                                uint1024_t nValue;
                                if(!object.Read(strValue, nValue))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT1024_T could not read value");

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 128 < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::UINT1024_T costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += 128;

                                break;
                            }


                            /* Standard type for STL string */
                            case TAO::Register::TYPES::STRING:
                            {
                                /* Read the value. */
                                std::string strData;
                                if(!object.Read(strValue, strData))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::STRING could not read value");

                                /* Allocate the value. */
                                allocate(strData, vRet);

                                /* Check for overflows. */
                                uint32_t nSize = strData.size();
                                if(nCost + nSize < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::STRING costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += nSize;

                                break;
                            }


                            /* Standard type for STL vector with C++ type uint8_t */
                            case TAO::Register::TYPES::BYTES:
                            {
                                /* Read the value. */
                                std::vector<uint8_t> vData;
                                if(!object.Read(strValue, vData))
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::BYTES could not read value");

                                /* Allocate the value. */
                                allocate(vData, vRet);

                                /* Check for overflows. */
                                uint32_t nSize = vData.size();
                                if(nCost + nSize < nCost)
                                    throw TAO::Register::MalformedException("OP::REGISTER::VALUE::BYTES costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += nSize;

                                break;
                            }

                            default:
                                throw TAO::Register::MalformedException("OP::REGISTER::VALUE unsupported type");
                        }

                        break;
                    }


                    /* Get the genesis id of the transaction caller. */
                    case OP::CALLER::GENESIS:
                    {
                        /* Allocate to the registers. */
                        allocate(caller.Caller(), vRet);

                        /* Check for overflows. */
                        if(nCost + 32 < nCost)
                            throw TAO::Register::MalformedException("OP::CALLER::GENESIS costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 32;

                        break;
                    }


                    /* Get the timestamp of the transaction caller. */
                    case OP::CALLER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate(caller.Timestamp(), vRet);

                        /* Check for overflows. */
                        if(nCost + 8 < nCost)
                            throw TAO::Register::MalformedException("OP::CALLER::TIMESTAMP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Get the genesis id of the transaction creator. */
                    case OP::CONTRACT::GENESIS:
                    {
                        /* Allocate to the registers. */
                        allocate(contract.Caller(), vRet);

                        /* Check for overflows. */
                        if(nCost + 32 < nCost)
                            throw TAO::Register::MalformedException("OP::CONTRACT::GENESIS costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 32;

                        break;
                    }


                    /* Get the timestamp of the transaction caller. */
                    case OP::CONTRACT::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate(contract.Timestamp(), vRet);

                        /* Check for overflows. */
                        if(nCost + 8 < nCost)
                            throw TAO::Register::MalformedException("OP::CONTRACT::TIMESTAMP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Get the operations of the transaction caller. */
                    case OP::CONTRACT::OPERATIONS:
                    {
                        /* Get the bytes from caller. */
                        const std::vector<uint8_t>& vBytes = contract.Operations();

                        /* Check for empty operations. */
                        if(vBytes.empty())
                            throw TAO::Register::MalformedException("OP::CONTRACT::OPERATIONS contract has empty operations");

                        /* Check for condition or validate. */
                        uint8_t nOffset = 0;
                        switch(vBytes.at(0))
                        {
                            /* Check for condition at start. */
                            case OP::CONDITION:
                            {
                                /* Condition has offset of one. */
                                nOffset = 1;

                                break;
                            }

                            /* Check for validate at start. */
                            case OP::VALIDATE:
                            {
                                /* Validate has offset of 69. */
                                nOffset = 69;

                                break;
                            }
                        }

                        /* Check that offset is within memory range. */
                        if(vBytes.size() <= nOffset)
                            throw TAO::Register::MalformedException("OP::CONTRACT::OPERATIONS offset is not within size");

                        /* Allocate to the registers. */
                        allocate(vBytes, vRet, nOffset);

                        /* Check for overflows. */
                        uint32_t nSize = vBytes.size();
                        if(nCost + nSize < nCost)
                            throw TAO::Register::MalformedException("OP::CONTRACT::OPERATIONS costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += nCost;

                        break;
                    }


                    /* Get the operations of the transaction caller. */
                    case OP::CALLER::OPERATIONS:
                    {
                        /* Get the bytes from caller. */
                        const std::vector<uint8_t>& vBytes = caller.Operations();

                        /* Check for empty operations. */
                        if(vBytes.empty())
                            throw TAO::Register::MalformedException("OP::CALLER::OPERATIONS caller has empty operations");

                        /* Check for condition or validate. */
                        uint8_t nOffset = 0;
                        switch(vBytes.at(0))
                        {
                            /* Check for condition at start. */
                            case OP::CONDITION:
                            {
                                /* Condition has offset of one. */
                                nOffset = 1;

                                break;
                            }

                            /* Check for validate at start. */
                            case OP::VALIDATE:
                            {
                                /* Validate has offset of 69. */
                                nOffset = 69;

                                break;
                            }
                        }

                        /* Check that offset is within memory range. */
                        if(vBytes.size() <= nOffset)
                            throw TAO::Register::MalformedException("OP::CALLER::OPERATIONS offset is not within size");

                        /* Allocate to the registers. */
                        allocate(vBytes, vRet, nOffset);

                        /* Check for overflows. */
                        uint32_t nSize = vBytes.size();
                        if(nCost + nSize < nCost)
                            throw TAO::Register::MalformedException("OP::CALLER::OPERATIONS costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += nCost;

                        break;
                    }


                    /* Get the current height of the chain. */
                    case OP::LEDGER::HEIGHT:
                    {
                        /* Allocate to the registers. */
                        allocate(TAO::Ledger::ChainState::stateBest.load().nHeight, vRet);

                        /* Check for overflows. */
                        if(nCost + 4 < nCost)
                            throw TAO::Register::MalformedException("OP::LEDGER::HEIGHT costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 4;

                        break;
                    }


                    /* Get the current supply of the chain. */
                    case OP::LEDGER::SUPPLY:
                    {
                        /* Allocate to the registers. */
                        allocate(uint64_t(TAO::Ledger::ChainState::stateBest.load().nMoneySupply), vRet);

                        /* Check for overflows. */
                        if(nCost + 8 < nCost)
                            throw TAO::Register::MalformedException("OP::LEDGER::SUPPLY costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Get the best block timestamp. */
                    case OP::LEDGER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate(uint64_t(TAO::Ledger::ChainState::stateBest.load().nTime), vRet);

                        /* Check for overflows. */
                        if(nCost + 8 < nCost)
                            throw TAO::Register::MalformedException("OP::LEDGER::TIMESTAMP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Compute an SK256 hash of current return value. */
                    case OP::CRYPTO::SK256:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw TAO::Register::MalformedException("OP::CRYPTO::SK256: can't hash with no input");

                        /* Compute the return hash. */
                        uint256_t hash = LLC::SK256((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Check for overflows. */
                        if(nCost + 2048 < nCost)
                            throw TAO::Register::MalformedException("OP::CRYPTO::SK256 costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 2048;

                        break;
                    }


                    /* Compute an SK512 hash of current return value. */
                    case OP::CRYPTO::SK512:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw TAO::Register::MalformedException("OP::CRYPTO::SK512: can't hash with no input");

                        /* Compute the return hash. */
                        uint512_t hash = LLC::SK512((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Check for overflows. */
                        if(nCost + 2048 < nCost)
                            throw TAO::Register::MalformedException("OP::CRYPTO::SK512 costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 2048;

                        break;
                    }

                    case OP::GROUP:
                    case OP::UNGROUP:
                    case OP::RESERVED:
                    case OP::AND:
                    case OP::OR:
                    case OP::EQUALS:
                    case OP::GREATERTHAN:
                    case OP::GREATEREQUALS:
                    case OP::LESSTHAN:
                    case OP::LESSEQUALS:
                    case OP::NOTEQUALS:
                    case OP::CONTAINS:
                    {
                        /* If no applicable instruction found, rewind and return. */
                        
                        contract.Rewind(1, Contract::CONDITIONS);

                        return true;
                    }

                    default:
                    {
                        /* Unknown case so the condition must be malformed */
                        throw TAO::Register::MalformedException("Malformed conditions");
                        break;
                    }
                }
            }

            return true;

        }

        /* Converts a warning flag to a string */
        std::string Condition::WarningToString(uint16_t nWarning)
        {
            switch(nWarning)
            {
                case WARNINGS::NONE :
                    return "";
                
                case WARNINGS::ADD_OVERFLOW :
                    return "OP::ADD 64-bit value overflow";

                case WARNINGS::SUB_OVERFLOW :
                    return "OP::SUB 64-bit value overflow";

                case WARNINGS::INC_OVERFLOW :
                    return "OP::INC 64-bit value overflow";

                case WARNINGS::DEC_OVERFLOW :
                    return "OP::DEC 64-bit value overflow";

                case WARNINGS::MUL_OVERFLOW :
                    return "OP::MUL 64-bit value overflow";
                
                case WARNINGS::EXP_OVERFLOW :
                    return "OP::EXP 64-bit value overflow";

                case WARNINGS::DIV_BY_ZERO :
                    return "OP::DIV cannot divide by zero";
                
                case WARNINGS::MOD_DIV_ZERO :
                    return "OP::MOD cannot divide by zero";
                
                default :
                    return "Unknown warning";

            }
        }
    }
}
