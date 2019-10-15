/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "If only you knew the magnificence of the 3, 6, and 9, then you
             would have a key to the universe" - Nikola Tesla

____________________________________________________________________________________________*/

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/basevm.h>
#include <TAO/Register/types/exception.h>

namespace TAO
{

    namespace Register
    {


        /* Default constructor. */
        BaseVM::BaseVM(const uint32_t nSize)
        : vRegister (nSize, 0)
        , nPointer  (0)
        {
        }


        /* Copy constructor. */
        BaseVM::BaseVM(const BaseVM& vm)
        : vRegister (vm.vRegister)
        , nPointer  (vm.nPointer)
        {
        }


        /* Move constructor. */
        BaseVM::BaseVM(BaseVM&& vm) noexcept
        : vRegister (std::move(vm.vRegister))
        , nPointer  (std::move(vm.nPointer))
        {
        }


        /* Copy assignment. */
        BaseVM& BaseVM::operator=(const BaseVM& vm)
        {
            vRegister = vm.vRegister;
            nPointer  = vm.nPointer;

            return *this;
        }


        /* Move assignment. */
        BaseVM& BaseVM::operator=(BaseVM&& vm) noexcept
        {
            vRegister = std::move(vm.vRegister);
            nPointer  = std::move(vm.nPointer);

            return *this;
        }


        /* Default Destructor */
        BaseVM::~BaseVM()
        {
        }


        /* Get the internal byte level pointers for value. */
        const uint8_t* BaseVM::begin(const Value& value) const
        {
            return (uint8_t*)&vRegister[value.nBegin];
        }


        /* Get the internal byte level pointers for value. */
        const uint8_t* BaseVM::end(const Value& value) const
        {
            return (uint8_t*)&vRegister[value.nEnd] - ((value.size() * 8) - value.nBytes);
        }


        /* Reset the internal memory pointer. */
        void BaseVM::reset()
        {
            nPointer = 0;
        }


        /* Allocate a 8 bit integer into the VM register memory. */
        void BaseVM::allocate(const uint8_t& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = ++nPointer;
            value.nBytes = 1;

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into register. */
            vRegister[value.nBegin] = data;
        }


        /* Allocate a 16 bit integer into the VM register memory. */
        void BaseVM::allocate(const uint16_t& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = ++nPointer;
            value.nBytes = 2;

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into register. */
            vRegister[value.nBegin] = data;
        }


        /* Allocate a 32 bit integer into the VM register memory. */
        void BaseVM::allocate(const uint32_t& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = ++nPointer;
            value.nBytes = 4;

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into register. */
            vRegister[value.nBegin] = data;
        }


        /* Allocate a 64 bit integer into the VM register memory. */
        void BaseVM::allocate(const uint64_t& data, Value& value)
        {
            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = ++nPointer;
            value.nBytes = 8;

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into register. */
            vRegister[value.nBegin] = data;
        }


        /* Allocate a byte stream into the VM register memory. */
        void BaseVM::allocate(const std::vector<uint8_t>& data, Value& value, const uint8_t nOffset)
        {
            /* Get the size. */
            uint32_t nSize = ((data.size() - nOffset) / 8) + ((data.size() - nOffset) % 8 == 0 ? 0 : 1);

            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = nPointer + nSize;
            value.nBytes = (data.size() - nOffset);

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into the registers. */
            std::copy((uint8_t*)&data[nOffset], (uint8_t*)&data[nOffset] + value.nBytes, (uint8_t*)begin(value));

            /* Iterate the memory pointer. */
            nPointer += nSize;
        }


        /* Allocate a string into the VM register memory. */
        void BaseVM::allocate(const std::string& data, Value& value)
        {
            /* Get the size. */
            uint32_t nSize = (data.size() / 8) + (data.size() % 8 == 0 ? 0 : 1);

            /* Set the value pointers. */
            value.nBegin = nPointer;
            value.nEnd   = nPointer + nSize;
            value.nBytes = data.size();

            /* Check for memory overflows. */
            if(value.nEnd > vRegister.size())
                throw BaseVMException("Out of register memory");

            /* Copy data into the registers. */
            std::copy((uint8_t*)&data[0], (uint8_t*)&data[0] + value.nBytes, (uint8_t*)begin(value));

            /* Iterate the memory pointer. */
            nPointer += nSize;
        }


        /* Deallocate a 8 bit integer from the VM register memory. */
        void BaseVM::deallocate(uint8_t& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
                
            /* Check for that this is last object */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Check for value size overflows. */
            if(end(value) - begin(value) != sizeof(data))
                throw BaseVMException("Deallocate size mismatch ", end(value) - begin(value), " to ", sizeof(data));

            /* Copy data from the registers. */
            data = vRegister[value.nBegin];

            /* Zero out the memory. */
            vRegister[value.nBegin] = 0;

            /* Iterate the memory pointer. */
            --nPointer;
        }


        /* Deallocate a 16 bit integer from the VM register memory. */
        void BaseVM::deallocate(uint16_t& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Check for value size overflows. */
            if(end(value) - begin(value) != sizeof(data))
                throw BaseVMException("Deallocate size mismatch ", end(value) - begin(value), " to ", sizeof(data));

            /* Copy data from the registers. */
            data = vRegister[value.nBegin];

            /* Zero out the memory. */
            vRegister[value.nBegin] = 0;

            /* Iterate the memory pointer. */
            --nPointer;
        }


        /* Deallocate a 32 bit integer from the VM register memory. */
        void BaseVM::deallocate(uint32_t& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Check for value size overflows. */
            if(end(value) - begin(value) != sizeof(data))
                throw BaseVMException("Deallocate size mismatch ", end(value) - begin(value), " to ", sizeof(data));

            /* Copy data from the registers. */
            data = vRegister[value.nBegin];

            /* Zero out the memory. */
            vRegister[value.nBegin] = 0;

            /* Iterate the memory pointer. */
            --nPointer;
        }


        /* Deallocate a 64 bit integer from the VM register memory. */
        void BaseVM::deallocate(uint64_t& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Check for value size overflows. */
            if(end(value) - begin(value) != sizeof(data))
                throw BaseVMException("Deallocate size mismatch ", end(value) - begin(value), " to ", sizeof(data));

            /* Copy data from the registers. */
            data = vRegister[value.nBegin];

            /* Zero out the memory. */
            vRegister[value.nBegin] = 0;

            /* Iterate the memory pointer. */
            --nPointer;
        }


        /* Deallocate a vector from the VM register memory and return a copy. */
        void BaseVM::deallocate(std::vector<uint8_t>& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object (TODO: handle defragmenting). */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Set the data to expected size. */
            data.resize(value.nBytes);

            /* Check for value size overflows. */
            if(end(value) - begin(value) != data.size())
                throw BaseVMException("Deallocate size mismatch");

            /* Copy data from the registers. */
            std::copy((uint8_t*)begin(value), (uint8_t*)end(value), (uint8_t*)&data[0]);

            /* Zero out the memory. */
            for(uint32_t i = 0; i < value.size(); ++i)
                vRegister[value.nBegin + i] = 0;

            /* Iterate the memory pointer. */
            nPointer -= value.size();
        }


        /* Deallocate a string from the VM register memory and return a copy. */
        void BaseVM::deallocate(std::string& data, const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object (TODO: handle defragmenting). */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Set the data to expected size. */
            data.resize(value.nBytes);

            /* Check for value size overflows. */
            if(end(value) - begin(value) != data.size())
                throw BaseVMException("Deallocate size mismatch ", value.size() * 8, " > ", data.size());

            /* Copy data from the registers. */
            std::copy((uint8_t*)begin(value), (uint8_t*)end(value), (uint8_t*)&data[0]);

            /* Zero out the memory. */
            for(uint32_t i = 0; i < value.size(); ++i)
                vRegister[value.nBegin + i] = 0;

            /* Iterate the memory pointer. */
            nPointer -= value.size();
        }


        /* Deallocate an object from the VM register memory. */
        void BaseVM::deallocate(const Value& value)
        {
            /* Check that there is something in the register to deallocate */
            if(nPointer == 0 || value.nEnd == 0)
                throw BaseVMException("Missing register value");
            
            /* Check for that this is last object (TODO: handle defragmenting). */
            if(value.nEnd != nPointer)
                throw BaseVMException("Cannot deallocate when not last");

            /* Check for memory overflows. */
            if((int32_t)(nPointer - value.size()) < 0)
                throw BaseVMException("Invalid memory address ", nPointer - value.size());

            /* Zero out the memory. */
            for(uint32_t i = 0; i < value.size(); ++i)
                vRegister[value.nBegin + i] = 0;

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
            /* Catch sizes of zero. */
            if(a.size() == 0 || b.size() == 0)
                return false;

            /* Compare memory locations. */
            size_t nSize = std::min(a.size(), b.size()) - 1;
            for(int64_t i = nSize; i >= 0; --i)
            {
                if(vRegister[a.nBegin + i] != vRegister[b.nBegin + i])
                {
                    //debug::log(0, "I ", i, " Byte ", std::hex, vRegister[a.nBegin + i], " vs Bytes ", std::hex, vRegister[b.nBegin + i]);
                    return vRegister[a.nBegin + i] - vRegister[b.nBegin + i];
                }
            }
            return 0;
        }


        /* Compare two memory locations in the register VM space. */
        bool BaseVM::contains(const Value& a, const Value& b)
        {
            /* Value b cannot be contained in value a if it is larger. */
            if(b.size() > a.size() || a.size() == 0 || b.size() == 0)
                return false;

            /* Copy bytes from registers to compare byte for byte. */
            std::vector<uint8_t> vA(begin(a), end(a));
            std::vector<uint8_t> vB(begin(b), end(b));

            /* Loop through the values in registers. */
            for(uint64_t i = 0; i < vA.size(); ++i)
            {
                //debug::log(0, "I ", i, " Byte ", std::hex, vA[i], " vs Bytes ", std::hex, vB[0]);

                /* Check for the start of b. */
                if(vA[i] == vB[0])
                {
                    //debug::log(0, "I ", i, " Byte ", vA[i], " vs Bytes ", vB[0]);

                    /* Loop through bytes in b and check to a. */
                    for(uint64_t n = 1; n < vB.size(); ++n)
                    {
                        //debug::log(0, "I ", i, " Byte ", vA[i + n], " vs Bytes ", vB[n]);

                        /* Break if bytes don't match and search isn't a wildcard byte. */
                        if(vA[i + n] != vB[n] && vB[n] != TAO::Operation::OP::WILDCARD)
                            break;

                        /* Return true if all bytes match. */
                        if(n + 1 == vB.size())
                            return true;
                    }
                }
            }

            return false;
        }


        /* Get the total bytes available in the register VM space. */
        uint32_t BaseVM::available()
        {
            return (vRegister.size() - nPointer) * 8;
        }
    }
}
