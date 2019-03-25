/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/basevm.h>

namespace TAO
{

    namespace Register
    {

        /* Get the internal byte level pointers for value. */
        const uint8_t* BaseVM::begin(const Value& value) const
        {
            return (uint8_t*)&vRegister[value.nBegin];
        }


        /* Get the internal byte level pointers for value. */
        const uint8_t* BaseVM::end(const Value& value) const
        {
            return (uint8_t*)&vRegister[value.nEnd];
        }


        /* Allocate a 64 bit integer into the VM register memory. */
        void BaseVM::allocate(const uint64_t& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = ++nPointer;

            /* Check for memory overflows. */
            if(value.nEnd >= vRegister.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

            /* Copy data into register. */
            vRegister[value.nBegin] = data;
        }


        /* Allocate a byte stream into the VM register memory. */
        void BaseVM::allocate(const std::vector<uint8_t>& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = nPointer + (data.size() / 8);

            /* Check for memory overflows. */
            if(value.nEnd >= vRegister.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

            /* Copy data into the registers. */
            std::copy((uint8_t*)&data[0], (uint8_t*)&data[0] + data.size(), (uint8_t*)begin(value));

            /* Iterate the memory pointer. */
            nPointer += value.size();
        }


        /* Allocate a string into the VM register memory. */
        void BaseVM::allocate(const std::string& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = nPointer + (data.size() / 8);

            /* Check for memory overflows. */
            if(value.nEnd >= vRegister.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

            /* Copy data into the registers. */
            std::copy((uint8_t*)&data[0], (uint8_t*)&data[0] + data.size(), (uint8_t*)begin(value));

            /* Iterate the memory pointer. */
            nPointer += value.size();
        }



        /* Deallocate an object from the VM register memory. */
        void BaseVM::deallocate(const Value& value)
        {
            /* Check for that this is last object (TODO: handle defragmenting). */
            if(value.nEnd != nPointer)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, " cannot deallocate when not last"));

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, " invalid memory address ", nPointer - value.size()));

            /* Iterate the memory pointer. */
            nPointer -= value.size();
        }

        /* Get a register value. */
        uint64_t& BaseVM::at(const Value& value)
        {
            return vRegister[value.nBegin];
        }


        /* Compare two memory locations in the register VM space. */
        int64_t BaseVM::compare(const Value& a, const Value& b)
        {
            size_t nSize = std::min(a.size(), b.size()) - 1;
            for(int64_t i = nSize; i >= 0; --i)
            {
                //debug::log(0, "I ", i, " Byte ", std::hex, vRegister[a.nBegin + i], " vs Bytes ", std::hex, vRegister[b.nBegin + i]);
                if(vRegister[a.nBegin + i] != vRegister[b.nBegin + i])
                    return vRegister[a.nBegin + i] - vRegister[b.nBegin + i];

            }
            return 0;
        }


        /* Get the total bytes available in the register VM space. */
        uint32_t BaseVM::available()
        {
            return (vRegister.size() - nPointer) * 8;
        }
    }
}
