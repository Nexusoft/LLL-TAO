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


        /* Execute the validation script. */
        bool Condition::Execute()
        {
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
                            throw debug::exception("OP::GROUP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Handle for the && operator. */
                    case OP::UNGROUP:
                    {
                        /* Check that nothing has been evaluated. */
                        if(vEvaluate.empty())
                            return debug::error(FUNCTION, "malformed conditional statement");

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
                                return debug::error(FUNCTION, "conditional type is unsupported");
                        }

                        break;
                    }


                    /* Handle for the && operator. */
                    case OP::AND:
                    {
                        /* Check for an operation. */
                        if(vEvaluate.empty())
                            return debug::error(FUNCTION, "evaluate ungrouping count incomplete");

                        /* Check that evaluate is default value. */
                        if(vEvaluate.top().second == OP::OR)
                            return debug::error(FUNCTION, "cannot evaluate OP::AND with previous OP::OR");

                        /* Set the new evaluate state. */
                        vEvaluate.top().second = OP::AND;

                        break;
                    }


                    /* Handle for the || operator. */
                    case OP::OR:
                    {
                        /* Check for an operation. */
                        if(vEvaluate.empty())
                            return debug::error(FUNCTION, "evaluate ungrouping count incomplete");

                        /* Check that evaluate is default value. */
                        if(vEvaluate.top().second == OP::AND)
                            return debug::error(FUNCTION, "cannot evaluate OP::OR with previous OP::AND");

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
                            return debug::error(FUNCTION, "malformed conditional statement");

                        /* Check for evalute state. */
                        switch(vEvaluate.top().second)
                        {
                            /* Handle if this is our first OP. */
                            case OP::RESERVED:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = Evaluate();

                                break;
                            }

                            /* Handle logical AND operator. */
                            case OP::AND:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (Evaluate() && vEvaluate.top().first);

                                break;
                            }

                            /* Handle logical OR operator. */
                            case OP::OR:
                            {
                                /* Check that statement evaluates. */
                                vEvaluate.top().first = (Evaluate() || vEvaluate.top().first);

                                break;
                            }

                            default:
                                return debug::error(FUNCTION, "conditional type is unsupported");
                        }

                        break;
                    }
                }
            }

            /* Check the values in groups. */
            if(vEvaluate.size() != 1)
                return debug::error(FUNCTION, "evaluate groups count incomplete");

            /* Return final value. */
            return vEvaluate.top().first;
        }


        /* Evaluate the condition. */
        bool Condition::Evaluate()
        {
            /* Flag to tell how it evaluated. */
            bool fRet = false;

            /* Flags to indicate if left and right values were obtained correctly */
            bool fLeft = false;
            bool fRight = false;

            /* The left and right values for the evaluation */
            TAO::Register::Value vLeft;
            TAO::Register::Value vRight;

            /* Grab the first value */
            fLeft = GetValue(vLeft);

            /* Ensure there is more conditions stream data */
            if(contract.End(Contract::CONDITIONS))
                return debug::error(FUNCTION, "malformed conditions");

            /* Grab the next operation. */
            uint8_t OPERATION = 0;
            contract >= OPERATION;

            /* Validate the op code */
            switch(OPERATION)
            {
                case OP::EQUALS:
                case OP::LESSTHAN:
                case OP::GREATERTHAN:
                case OP::LESSEQUALS:
                case OP::GREATEREQUALS:
                case OP::NOTEQUALS:
                case OP::CONTAINS:
                {
                    /* Grab the second value. */
                    fRight = GetValue(vRight);
                    
                    break;
                }
                /* For unknown codes, always fail. */
                default:
                    return debug::error(FUNCTION, "malformed conditions");
            }

            /* If we successfully obtained both left and right values along with the op code then perform the operation */
            if(fLeft && fRight)
            {
                /* Switch by operation code. */
                switch(OPERATION)
                {
                    /* Handle for the == operator. */
                    case OP::EQUALS:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) == 0);

                        break;
                    }

                    /* Handle for < operator. */
                    case OP::LESSTHAN:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) < 0);

                        break;
                    }

                    /* Handle for the > operator. */
                    case OP::GREATERTHAN:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) > 0);

                        break;
                    }

                    /* Handle for <= operator. */
                    case OP::LESSEQUALS:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) <= 0);

                        break;
                    }

                    /* Handle for the >= operator. */
                    case OP::GREATEREQUALS:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) >= 0);

                        break;
                    }

                    /* Handle for the != operator. */
                    case OP::NOTEQUALS:
                    {
                        /* Compare both values to one another. */
                        fRet = (compare(vLeft, vRight) != 0);

                        break;
                    }

                    /* Handle to check if a sequence of bytes is inside another. */
                    case OP::CONTAINS:
                    {
                        /* Compare both values to one another. */
                        fRet = contains(vLeft, vRight);

                        break;
                    } 
                }
            }
            else
            {
                /* If we didn't obtain both values then it must evaluate to false */
                fRet = false;
            }

            /* Deallocate the values that we obtained and pushed to the stack */
            if(fRight)
                deallocate(vRight);

            if(fLeft)
                deallocate(vLeft);
            
            
            
            /* Return final response. */
            return fRet;
        }


        /** get value
         *
         *  Get a value from the register virtual machine.
         *
         **/
        bool Condition::GetValue(TAO::Register::Value& vRet)
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
                        if(!GetValue(vAdd))
                            return debug::error("OP::ADD failed to get r-value");

                        /* Check computational bounds. */
                        if(vAdd.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::ADD computation greater than 64-bits");

                        /* Check for overflows. */
                        if(at(vRet) + at(vAdd) < at(vRet))
                            throw debug::exception("OP::ADD 64-bit value overflow");

                        /* Compute the return value. */
                        at(vRet) += at(vAdd);

                        /* Deallocate r-value from memory. */
                        deallocate(vAdd);

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw debug::exception("OP::ADD costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Subtract one number from another. */
                    case OP::SUB:
                    {
                        /* Get the sub from r-value. */
                        TAO::Register::Value vSub;
                        if(!GetValue(vSub))
                            return debug::error("OP::SUB failed to get r-value");

                        /* Check computational bounds. */
                        if(vSub.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::SUB computation greater than 64-bits");

                        /* Check for overflows. */
                        if(at(vRet) - at(vSub) > at(vRet))
                            throw debug::exception("OP::SUB 64-bit value overflow");

                        /* Compute the return value. */
                        at(vRet) -= at(vSub);

                        /* Deallocate r-value from memory. */
                        deallocate(vSub);

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw debug::exception("OP::SUB costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Increment a number by an order of 1. */
                    case OP::INC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw debug::exception("OP::INC computation greater than 64-bits");

                        /* Compute the return value. */
                        if(++at(vRet) == 0)
                            throw debug::exception("OP::INC 64-bit value overflow");

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw debug::exception("OP::INC costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* De-increment a number by an order of 1. */
                    case OP::DEC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw debug::exception("OP::DEC computation greater than 64-bits");

                        /* Compute the return value. */
                        if(--at(vRet) == std::numeric_limits<uint64_t>::max())
                            throw debug::exception("OP::DEC 64-bit value overflow");

                        /* Check for overflows. */
                        if(nCost + 64 < nCost)
                            throw debug::exception("OP::DEC costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 64;

                        break;
                    }


                    /* Divide a number by another. */
                    case OP::DIV:
                    {
                        /* Get the divisor from r-value. */
                        TAO::Register::Value vDiv;
                        if(!GetValue(vDiv))
                            return debug::error("OP::DIV failed to get r-value");

                        /* Check computational bounds. */
                        if(vDiv.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::DIV computation greater than 64-bits");

                        /* Check for exceptions. */
                        if(at(vDiv) == 0)
                            throw debug::exception("OP::DIV cannot divide by zero");

                        /* Compute the return value. */
                        at(vRet) /= at(vDiv);

                        /* Deallocate r-value from memory. */
                        deallocate(vDiv);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw debug::exception("OP::DIV costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Multiply a number by another. */
                    case OP::MUL:
                    {
                        /* Get the multiplier from r-value. */
                        TAO::Register::Value vMul;
                        if(!GetValue(vMul))
                            return debug::error("OP::MUL failed to get r-value");

                        /* Check computational bounds. */
                        if(vMul.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::MUL computation greater than 64-bits");

                        /* Check for value overflows. */
                        if(at(vMul) != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / at(vMul))
                            throw debug::exception("OP::MUL 64-bit value overflow");

                        /* Compute the return value. */
                        at(vRet) *= at(vMul);

                        /* Deallocate r-value from memory. */
                        deallocate(vMul);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw debug::exception("OP::MUL costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 128;

                        break;
                    }


                    /* Raise a number by the power of another. */
                    case OP::EXP:
                    {
                        /* Get the exponent from r-value. */
                        TAO::Register::Value vExp;
                        if(!GetValue(vExp))
                            return debug::error("OP::EXP failed to get r-value");

                        /* Check computational bounds. */
                        if(vExp.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::EXP computation greater than 64-bits");

                        /* Catch for a power of 0. */
                        if(at(vExp) == 0)
                            at(vRet) = 1;

                        /* Exponentiate by integers. */
                        uint64_t nBase = at(vRet);
                        for(uint64_t e = 1; e < at(vExp); ++e)
                        {
                            /* Check for value overflows. */
                            if(nBase != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / nBase)
                                throw debug::exception("OP::EXP 64-bit value overflow");

                            /* Assign the return value. */
                            at(vRet) *= nBase;
                        }

                        /* Deallocate r-value from memory. */
                        deallocate(vExp);

                        /* Check for overflows. */
                        if(nCost + 256 < nCost)
                            throw debug::exception("OP::EXP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 256;

                        break;
                    }


                    /* Get the remainder after a division. */
                    case OP::MOD:
                    {
                        /* Get the modulus from r-value. */
                        TAO::Register::Value vMod;
                        if(!GetValue(vMod))
                            return debug::error("OP::MOD failed to get r-value");

                        /* Check computational bounds. */
                        if(vMod.size() > 1 || vRet.size() > 1)
                            throw debug::exception("OP::MOD computation greater than 64-bits");

                        /* Check for exceptions. */
                        if(at(vMod) == 0)
                            throw debug::exception("OP::MOD cannot divide by zero");

                        /* Compute the return value. */
                        at(vRet) %= at(vMod);

                        /* Deallocate r-value from memory. */
                        deallocate(vMod);

                        /* Check for overflows. */
                        if(nCost + 128 < nCost)
                            throw debug::exception("OP::MOD costs value overflow");

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
                            throw debug::exception("OP::SUBDATA costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += vData.size();

                        break;
                    }


                    /* Parse out subdata from bytes. */
                    case OP::CAT:
                    {
                        /* Get the add from r-value. */
                        TAO::Register::Value vCat;
                        if(!GetValue(vCat))
                            return debug::error("OP::CAT failed to get r-value");

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
                            throw debug::exception("OP::CAT costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT8_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT16_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT32_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT64_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT256_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT512_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::UINT1024_T costs value overflow");

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
                            throw debug::exception("OP::TYPES::STRING string is empty");

                        /* Set the register value. */
                        allocate(str, vRet);

                        /* Check for overflows. */
                        uint32_t nSize = str.size();
                        if(nCost + nSize < nCost)
                            throw debug::exception("OP::TYPES::STRING costs value overflow");

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
                            throw debug::exception("OP::TYPES::BYTES vector is empty");

                        /* Set the register value. */
                        allocate(vData, vRet);

                        /* Check for overflows. */
                        uint32_t nSize = vData.size();
                        if(nCost + nSize < nCost)
                            throw debug::exception("OP::TYPES::BYTES costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                if(nCost + 5004 < nCost)
                                    throw debug::exception("OP::REGISTER::MODIFIED costs value overflow");

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
                                    throw debug::exception("OP::CALLER::PRESTATE::MODIFIED costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                if(nCost + 5004 < nCost)
                                    throw debug::exception("OP::REGISTER::CREATED costs value overflow");

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
                                    throw debug::exception("OP::CALLER::PRESTATE::CREATED costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                if(nCost + 4128 < nCost)
                                    throw debug::exception("OP::REGISTER::OWNER costs value overflow");

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
                                    throw debug::exception("OP::CALLER::PRESTATE::OWNER costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                if(nCost + 4097 < nCost)
                                    throw debug::exception("OP::REGISTER::TYPE costs value overflow");

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
                                    throw debug::exception("OP::CALLER::PRESTATE::TYPE costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                uint32_t nSize = 4096 + state.GetState().size();
                                if(nCost + nSize < nCost)
                                    throw debug::exception("OP::REGISTER::STATE costs value overflow");

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
                                    throw debug::exception("OP::CALLER::PRESTATE::STATE costs value overflow");

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
                                    return false;

                                /* Check for overflows. */
                                if(nCost + 4096 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE costs value overflow");

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
                            return false;

                        /* Parse the object register. */
                        if(!object.Parse())
                            return false;

                        /* Get the supported type enumeration. */
                        uint8_t nType;
                        if(!object.Type(strValue, nType))
                            return false;

                        /* Switch supported types. */
                        switch(nType)
                        {
                            /* Standard type for C++ uint8_t. */
                            case TAO::Register::TYPES::UINT8_T:
                            {
                                /* Read the value. */
                                uint8_t nValue;
                                if(!object.Read(strValue, nValue))
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 1 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT8_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 2 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT16_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 4 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT32_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 8 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT64_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 32 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT256_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 64 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT512_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(nValue, vRet);

                                /* Check for overflows. */
                                if(nCost + 128 < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::UINT1024_T costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(strData, vRet);

                                /* Check for overflows. */
                                uint32_t nSize = strData.size();
                                if(nCost + nSize < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::STRING costs value overflow");

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
                                    return false;

                                /* Allocate the value. */
                                allocate(vData, vRet);

                                /* Check for overflows. */
                                uint32_t nSize = vData.size();
                                if(nCost + nSize < nCost)
                                    throw debug::exception("OP::REGISTER::VALUE::BYTES costs value overflow");

                                /* Reduce the costs to prevent operation exhuastive attacks. */
                                nCost += nSize;

                                break;
                            }

                            default:
                                return false;
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
                            throw debug::exception("OP::CALLER::GENESIS costs value overflow");

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
                            throw debug::exception("OP::CALLER::TIMESTAMP costs value overflow");

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
                            throw debug::exception("OP::CONTRACT::GENESIS costs value overflow");

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
                            throw debug::exception("OP::CONTRACT::TIMESTAMP costs value overflow");

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
                            throw debug::exception("OP::CONTRACT::OPERATIONS contract has empty operations");

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
                            return debug::error("OP::CONTRACT::OPERATIONS offset is not within size");

                        /* Allocate to the registers. */
                        allocate(vBytes, vRet, nOffset);

                        /* Check for overflows. */
                        uint32_t nSize = vBytes.size();
                        if(nCost + nSize < nCost)
                            throw debug::exception("OP::CONTRACT::OPERATIONS costs value overflow");

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
                            return debug::error("OP::CALLER::OPERATIONS caller has empty operations");

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
                            return debug::error("OP::CALLER::OPERATIONS offset is not within size");

                        /* Allocate to the registers. */
                        allocate(vBytes, vRet, nOffset);

                        /* Check for overflows. */
                        uint32_t nSize = vBytes.size();
                        if(nCost + nSize < nCost)
                            throw debug::exception("OP::CALLER::OPERATIONS costs value overflow");

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
                            throw debug::exception("OP::LEDGER::HEIGHT costs value overflow");

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
                            throw debug::exception("OP::LEDGER::SUPPLY costs value overflow");

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
                            throw debug::exception("OP::LEDGER::TIMESTAMP costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 8;

                        break;
                    }


                    /* Compute an SK256 hash of current return value. */
                    case OP::CRYPTO::SK256:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw debug::exception("OP::CRYPTO::SK256: can't hash with no input");

                        /* Compute the return hash. */
                        uint256_t hash = LLC::SK256((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Check for overflows. */
                        if(nCost + 2048 < nCost)
                            throw debug::exception("OP::CRYPTO::SK256 costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 2048;

                        break;
                    }


                    /* Compute an SK512 hash of current return value. */
                    case OP::CRYPTO::SK512:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw debug::exception("OP::CRYPTO::SK512: can't hash with no input");

                        /* Compute the return hash. */
                        uint512_t hash = LLC::SK512((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Check for overflows. */
                        if(nCost + 2048 < nCost)
                            throw debug::exception("OP::CRYPTO::SK512 costs value overflow");

                        /* Reduce the costs to prevent operation exhuastive attacks. */
                        nCost += 2048;

                        break;
                    }


                    default:
                    {
                        /* If no applicable instruction found, rewind and return. */
                        contract.Rewind(1, Contract::CONDITIONS);

                        return true;
                    }
                }
            }

            return true;
        }
    }
}
