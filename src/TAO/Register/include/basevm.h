/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_BASEVM_H
#define NEXUS_TAO_REGISTER_INCLUDE_BASEVM_H

#include <TAO/Register/include/value.h>

#include <Util/include/debug.h>

namespace TAO
{

    namespace Register
    {

        /** BaseVM
         *
         *  A virtual machine that manages processing memory in 64-bit registers.
         *
         **/
        class BaseVM
        {
        protected:

            /** The internal register memory. **/
            std::vector<uint64_t> vRegister;


            /** The internal memory pointer. */
            uint32_t nPointer;


        public:

            /** Default constructor. **/
            BaseVM(uint32_t nSize = 64)
            : vRegister(nSize, 0)
            , nPointer(0)
            {
            }


            /** begin
             *
             *  Get the internal byte level pointers for value.
             *
             *  @param[in] value The value object to get pointer for.
             *
             *  @return the memory location of value.
             *
             **/
            const uint8_t* begin(const Value& value) const;


            /** end
             *
             *  Get the internal byte level pointers for value.
             *
             *  @param[in] value The value object to get pointer for.
             *
             *  @return the ending memory location of value.
             *
             **/
            const uint8_t* end(const Value& value) const;


            /** reset
             *
             *  Reset the internal memory pointer.
             *
             **/
            void reset();


            /** allocate
             *
             *  Allocate a new object into the VM register memory.
             *
             *  @param[in] data The data to allocate.
             *  @param[out] value The value object containing memory locations.
             *
             **/
            template<typename Data>
            void allocate(const Data& data, Value& value)
            {
                /* Get the size. */
                uint32_t nSize = (sizeof(data) / 8) + (sizeof(data) % 8 == 0 ? 0 : 1);

                /* Set the value pointers. */
                value.nBegin = nPointer;
                value.nEnd   = nPointer + nSize;
                value.nBytes = sizeof(data);

                /* Check for memory overflows. */
                if(value.nEnd >= vRegister.size())
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, " out of register memory"));

                /* Copy data into the registers. */
                std::copy((uint8_t*)&data, (uint8_t*)&data + value.nBytes, (uint8_t*)begin(value));

                /* Iterate the memory pointer. */
                nPointer += nSize;
            }


            /** allocate
             *
             *  Allocate a byte stream into the VM register memory.
             *
             *  @param[in] data The data to allocate.
             *  @param[out] value The value object containing memory locations.
             *
             **/
            void allocate(const std::vector<uint8_t>& data, Value& value);


            /** allocate
             *
             *  Allocate a string into the VM register memory.
             *
             *  @param[in] data The data to allocate.
             *  @param[out] value The value object containing memory locations.
             *
             **/
            void allocate(const std::string& data, Value& value);


            /** deallocate
             *
             *  Deallocate an object from the VM register memory and return a copy.
             *
             *  @param[out] data The data to return.
             *  @param[in] value The value object containing memory locations.
             *
             **/
            template<typename Data>
            void deallocate(Data& data, const Value& value)
            {
                /* Check for that this is last object (TODO: handle defragmenting). */
                if(value.nEnd != nPointer)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, " cannot deallocate when not last"));

                /* Check for memory overflows. */
                if((int32_t)(nPointer - value.size()) < 0)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, " invalid memory address ", nPointer - value.size()));

                /* Check for value size overflows. */
                if(end(value) - begin(value) != sizeof(data))
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, " deallocate size mismatch ", end(value) - begin(value), " to ", sizeof(data)));

                /* Copy data from the registers. */
                std::copy((uint8_t*)begin(value), (uint8_t*)end(value), (uint8_t*)&data);

                /* Zero out the memory. */
                for(uint32_t i = 0; i < value.size(); ++i)
                    vRegister[value.nBegin + i] = 0;

                /* Iterate the memory pointer. */
                nPointer -= value.size();
            }


            /** deallocate
             *
             *  Deallocate an vector from the VM register memory and return a copy.
             *
             *  @param[out] data The data to return.
             *  @param[in] value The value object containing memory locations.
             *
             **/
            void deallocate(std::vector<uint8_t>& data, const Value& value);


            /** deallocate
             *
             *  Deallocate an string from the VM register memory and return a copy.
             *
             *  @param[out] data The data to return.
             *  @param[in] value The value object containing memory locations.
             *
             **/
            void deallocate(std::string& data, const Value& value);


            /** deallocate
             *
             *  Deallocate an object from the VM register memory.
             *
             *  @param[in] value The value object containing memory locations.
             *
             **/
            void deallocate(const Value& value);


            /** at
             *
             *  Get a register value.
             *
             *  @param[in] value The value object containing memory locations.
             *
             *  @return The value contained in the register.
             *
             **/
            uint64_t& at(const Value& value);


            /** compare
             *
             *  Compare two memory locations in the register VM space.
             *
             *  @param[in] a The value object containing first memory locations.
             *  @param[in] b The value object containing second memory locations.
             *
             *  @return The difference in (little-endian) byte ordering.
             *
             **/
            int64_t compare(const Value& a, const Value& b);


            /** contains
             *
             *  Compare two memory locations in the register VM space.
             *
             *  @param[in] a The value object containing first memory locations.
             *  @param[in] b The value object containing second memory locations.
             *
             *  @return True if b is contained in a
             *
             **/
            bool contains(const Value& a, const Value& b);


            /** available
             *
             *  Get the total bytes available in the register VM space.
             *
             *  @return The total bytes available for allocation.
             *
             **/
            uint32_t available();
        };
    }
}

#endif
