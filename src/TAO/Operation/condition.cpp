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

        Condition::Condition(const Contract& contractIn, const Contract& callerIn, int32_t nLimitsIn)
        : TAO::Register::BaseVM() //512 bytes of register memory.
        , nLimits(nLimitsIn)
        , contract(contractIn)
        , caller(callerIn)
        {
        }


        /** Copy constructor. **/
        Condition::Condition(const Condition& in)
        : TAO::Register::BaseVM(in)
        , nLimits(in.nLimits)
        , contract(in.contract)
        , caller(in.caller)
        {
        }


        /* Reset the validation script for re-executing. */
        void Condition::Reset()
        {
            contract.Reset();
            nLimits = 2048;

            reset();
        }


        /* Execute the validation script. */
        bool Condition::Execute()
        {
            /* Build the stack for nested grouping. */
            std::stack<std::pair<bool, uint8_t>> vEvaluate;

            /* Push base group, which is what contains final return value. */
            vEvaluate.push(std::make_pair(false, OP::RESERVED));

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


        /* Execute the validation script. */
        bool Condition::Evaluate()
        {
            /* Flag to tell how it evaluated. */
            bool fRet = false;

            /* Grab the first value */
            TAO::Register::Value vLeft;
            if(!GetValue(vLeft))
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get l-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

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
                    if(!GetValue(vRight))
                        throw std::runtime_error(debug::safe_printstr(FUNCTION, "failed to get r-value"));

                    /* Compare both values to one another. */
                    fRet = contains(vLeft, vRight);

                    /* Deallocate the values from the VM. */
                    deallocate(vRight);
                    deallocate(vLeft);

                    break;
                }

                /* For unknown codes, always fail. */
                default:
                    return debug::error(FUNCTION, "malformed conditions");
            }

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
                            throw std::runtime_error(debug::safe_printstr("OP::ADD failed to get r-value"));

                        /* Check computational bounds. */
                        if(vAdd.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::ADD computation greater than 64-bits"));

                        /* Check for overflows. */
                        if(at(vRet) + at(vAdd) < at(vRet))
                            throw std::runtime_error(debug::safe_printstr("OP::ADD 64-bit value overflow"));

                        /* Compute the return value. */
                        at(vRet) += at(vAdd);

                        /* Deallocate r-value from memory. */
                        deallocate(vAdd);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Subtract one number from another. */
                    case OP::SUB:
                    {
                        /* Get the sub from r-value. */
                        TAO::Register::Value vSub;
                        if(!GetValue(vSub))
                            throw std::runtime_error(debug::safe_printstr("OP::SUB failed to get r-value"));

                        /* Check computational bounds. */
                        if(vSub.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::SUB computation greater than 64-bits"));

                        /* Check for overflows. */
                        if(at(vRet) - at(vSub) > at(vRet))
                            throw std::runtime_error(debug::safe_printstr("OP::SUB 64-bit value overflow"));

                        /* Compute the return value. */
                        at(vRet) -= at(vSub);

                        /* Deallocate r-value from memory. */
                        deallocate(vSub);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Increment a number by an order of 1. */
                    case OP::INC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::INC computation greater than 64-bits"));

                        /* Compute the return value. */
                        if(++at(vRet) == 0)
                            throw std::runtime_error(debug::safe_printstr("OP::INC 64-bit value overflow"));

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* De-increment a number by an order of 1. */
                    case OP::DEC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::DEC computation greater than 64-bits"));

                        /* Compute the return value. */
                        if(--at(vRet) == std::numeric_limits<uint64_t>::max())
                            throw std::runtime_error(debug::safe_printstr("OP::DEC 64-bit value overflow"));

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Divide a number by another. */
                    case OP::DIV:
                    {
                        /* Get the divisor from r-value. */
                        TAO::Register::Value vDiv;
                        if(!GetValue(vDiv))
                            throw std::runtime_error(debug::safe_printstr("OP::DIV failed to get r-value"));

                        /* Check computational bounds. */
                        if(vDiv.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::DIV computation greater than 64-bits"));

                        /* Check for exceptions. */
                        if(at(vDiv) == 0)
                            throw std::runtime_error(debug::safe_printstr("OP::DIV cannot divide by zero"));

                        /* Compute the return value. */
                        at(vRet) /= at(vDiv);

                        /* Deallocate r-value from memory. */
                        deallocate(vDiv);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Multiply a number by another. */
                    case OP::MUL:
                    {
                        /* Get the multiplier from r-value. */
                        TAO::Register::Value vMul;
                        if(!GetValue(vMul))
                            throw std::runtime_error(debug::safe_printstr("OP::MUL failed to get r-value"));

                        /* Check computational bounds. */
                        if(vMul.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::MUL computation greater than 64-bits"));

                        /* Check for value overflows. */
                        if (at(vMul) != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / at(vMul))
                            throw std::runtime_error(debug::safe_printstr("OP::MUL 64-bit value overflow"));

                        /* Compute the return value. */
                        at(vRet) *= at(vMul);

                        /* Deallocate r-value from memory. */
                        deallocate(vMul);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Raise a number by the power of another. */
                    case OP::EXP:
                    {
                        /* Get the exponent from r-value. */
                        TAO::Register::Value vExp;
                        if(!GetValue(vExp))
                            throw std::runtime_error(debug::safe_printstr("OP::EXP failed to get r-value"));

                        /* Check computational bounds. */
                        if(vExp.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::EXP computation greater than 64-bits"));

                        /* Catch for a power of 0. */
                        if(at(vExp) == 0)
                            at(vRet) = 1;

                        /* Exponentiate by integers. */
                        uint64_t nBase = at(vRet);
                        for(uint64_t e = 1; e < at(vExp); ++e)
                        {
                            /* Check for value overflows. */
                            if (nBase != 0 && at(vRet) > std::numeric_limits<uint64_t>::max() / nBase)
                                throw std::runtime_error(debug::safe_printstr("OP::EXP 64-bit value overflow"));

                            /* Assign the return value. */
                            at(vRet) *= nBase;
                        }

                        /* Deallocate r-value from memory. */
                        deallocate(vExp);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 256;

                        break;
                    }


                    /* Get the remainder after a division. */
                    case OP::MOD:
                    {
                        /* Get the modulus from r-value. */
                        TAO::Register::Value vMod;
                        if(!GetValue(vMod))
                            throw std::runtime_error(debug::safe_printstr("OP::MOD failed to get r-value"));

                        /* Check computational bounds. */
                        if(vMod.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::MOD computation greater than 64-bits"));

                        /* Check for exceptions. */
                        if(at(vMod) == 0)
                            throw std::runtime_error(debug::safe_printstr("OP::MOD cannot divide by zero"));

                        /* Compute the return value. */
                        at(vRet) %= at(vMod);

                        /* Deallocate r-value from memory. */
                        deallocate(vMod);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

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

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= vData.size();

                        break;
                    }



                    /* Extract an uint8_t from the stream. */
                    case OP::TYPES::UINT8_T:
                    {
                        /* Extract the byte. */
                        uint8_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1;

                        break;
                    }


                    /* Extract an uint16_t from the stream. */
                    case OP::TYPES::UINT16_T:
                    {
                        /* Extract the short. */
                        uint16_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 2;

                        break;
                    }


                    /* Extract an uint32_t from the stream. */
                    case OP::TYPES::UINT32_T:
                    {
                        /* Extract the integer. */
                        uint32_t n = 0;
                        contract >= n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 4;

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

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

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

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 32;

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

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

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

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

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
                            throw std::runtime_error(debug::safe_printstr("OP::TYPES::STRING string is empty"));

                        /* Set the register value. */
                        allocate(str, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= str.size();

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
                            throw std::runtime_error(debug::safe_printstr("OP::TYPES::BYTES vector is empty"));

                        /* Set the register value. */
                        allocate(vData, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= vData.size();

                        break;
                    }


                    /* Get a unified timestamp and push to return value. */
                    case OP::GLOBAL::UNIFIED:
                    {
                        /* Get the current unified timestamp. */
                        uint64_t n = runtime::unifiedtimestamp();

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

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
                                if(!LLD::Register->Read(hashRegister, state))
                                    return false;

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

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.nModified, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 40;

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
                                if(!LLD::Register->Read(hashRegister, state))
                                    return false;

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

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.nCreated, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 40;

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
                                if(!LLD::Register->Read(hashRegister, state))
                                    return false;

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

                                break;
                            }
                        }

                        /* Set the register value. */
                        allocate(state.hashOwner, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

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
                                if(!LLD::Register->Read(hashRegister, state))
                                    return false;

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

                                break;
                            }
                        }

                        /* Push the type onto the return value. */
                        allocate((uint64_t)state.nType, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 33;

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
                                if(!LLD::Register->Read(hashRegister, state))
                                    return false;

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

                                break;
                            }
                        }

                        /* Allocate to the registers. */
                        allocate(state.GetState(), vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

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
                                if(!LLD::Register->Read(hashRegister, object))
                                    return false;

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

                                break;
                            }

                            default:
                                return false;
                        }

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get the genesis id of the transaction caller. */
                    case OP::CALLER::GENESIS:
                    {
                        /* Allocate to the registers. */
                        allocate(caller.Caller(), vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get the timestamp of the transaction caller. */
                    case OP::CALLER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate(caller.Timestamp(), vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1;

                        break;
                    }


                    /* Get the operations of the transaction caller. */
                    case OP::CALLER::OPERATIONS:
                    {
                        /* Get the bytes from caller. */
                        const std::vector<uint8_t>& vBytes = caller.Operations();

                        /* Check for empty operations. */
                        if(vBytes.empty())
                            throw std::runtime_error(debug::safe_printstr("OP::CALLER::OPERATIONS caller has empty operations"));

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
                            throw std::runtime_error(debug::safe_printstr("OP::CALLER::OPERATIONS offset is not within size"));

                        /* Allocate to the registers. */
                        allocate(vBytes, vRet, nOffset);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= vBytes.size();

                        break;
                    }


                    /* Get the current height of the chain. */
                    case OP::LEDGER::HEIGHT:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nHeight, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Get the current supply of the chain. */
                    case OP::LEDGER::SUPPLY:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nMoneySupply, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    case OP::LEDGER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nTime, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Compute an SK256 hash of current return value. */
                    case OP::CRYPTO::SK256:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw std::runtime_error("OP::CRYPTO::SK256: can't hash with no input");

                        /* Compute the return hash. */
                        uint256_t hash = LLC::SK256((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 512;

                        break;
                    }


                    /* Compute an SK512 hash of current return value. */
                    case OP::CRYPTO::SK512:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw std::runtime_error("OP::CRYPTO::SK512: can't hash with no input");

                        /* Compute the return hash. */
                        uint512_t hash = LLC::SK512((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1024;

                        break;
                    }


                    default:
                    {
                        /* If no applicable instruction found, rewind and return. */
                        contract.Rewind(1, Contract::CONDITIONS);
                        if(nLimits < 0)
                            debug::error(FUNCTION, "out of computational limits ", nLimits);

                        return (nLimits >= 0);
                    }
                }
            }

            /* Log if computational limits expired. */
            if(nLimits < 0)
                debug::error(FUNCTION, "out of computational limits ", nLimits);

            return (nLimits >= 0);
        }
    }
}
