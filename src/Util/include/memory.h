/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_MEMORY_H
#define NEXUS_UTIL_INCLUDE_MEMORY_H

#include <cstdint>
#include <Util/include/mutex.h>
#include <Util/include/debug.h>

namespace memory
{

    /** compare
     *
     *  Compares two byte arrays and determines their signed equivalence byte for
     *   byte.
     *
     *  @param[in] a The first byte array to compare.
     *  @param[in] b The second byte array to compare.
     *  @param[in] size The number of bytes to compare.
     *
     *  @return Returns the signed difference of the first different byte value.
     *
     **/
    int32_t compare(const uint8_t *a, const uint8_t *b, const uint64_t size);


    /** atomic
     *
     *  Protects an object inside with a mutex.
     *
     **/
    template<class TypeName>
    class atomic
    {
        /* The mutex to protect the memory. */
        mutable std::mutex MUTEX;


        /* The internal data. */
        TypeName data;


    public:

        /** Default Constructor. **/
        atomic()
        : MUTEX()
        , data()
        {
        }


        /** Constructor for storing. **/
        atomic(const TypeName& dataIn)
        : MUTEX()
        , data()
        {
            store(dataIn);
        }


        /** Assignment operator.
         *
         *  @param[in] a The atomic to assign from.
         *
         **/
        atomic& operator=(const atomic& a)
        {
            LOCK(MUTEX);

            data = a.data;
            return (*this);
        }


        /** Assignment operator.
         *
         *  @param[in] dataIn The atomic to assign from.
         *
         **/
        atomic& operator=(const TypeName& dataIn)
        {
            LOCK(MUTEX);

            data = dataIn;
            return (*this);
        }


        /** Equivilent operator.
         *
         *  @param[in] a The atomic to compare to.
         *
         **/
        bool operator==(const atomic& a) const
        {
            LOCK(MUTEX);

            return data == a.data;
        }


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName& dataIn) const
        {
            LOCK(MUTEX);

            return data == dataIn;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The atomic to compare to.
         *
         **/
        bool operator!=(const atomic& a) const
        {
            LOCK(MUTEX);

            return data != a.data;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName& dataIn) const
        {
            LOCK(MUTEX);

            return data != dataIn;
        }


        /** load
         *
         *  Load the object from memory.
         *
         **/
        const TypeName load() const
        {
            LOCK(MUTEX);

            return data;
        }


        /** store
         *
         *  Stores an object into memory.
         *
         *  @param[in] dataIn The data to into protected memory.
         *
         **/
        void store(const TypeName& dataIn)
        {
            LOCK(MUTEX);

            data = dataIn;
        }
    };


    /** lock_proxy
     *
     *  Temporary class that unlocks a mutex when outside of scope.
     *  Useful for protecting member access to a raw pointer.
     *
     **/
    template <class TypeName>
    class lock_proxy
    {
        /** Reference of the mutex. **/
        std::mutex& MUTEX;


        /** The pointer being locked. **/
        TypeName* data;


    public:

        /** Basic constructor
         *
         *  Assign the pointer and reference to the mutex.
         *
         *  @param[in] pData The pointer to shallow copy
         *  @param[in] MUTEX_IN The mutex reference
         *
         **/
        lock_proxy(TypeName* pData, std::mutex& MUTEX_IN)
        : data(pData)
        , MUTEX(MUTEX_IN)
        {
        }


        /** Destructor
        *
        *  Unlock the mutex.
        *
        **/
        ~lock_proxy()
        {
           MUTEX.unlock();
        }


        /** Member Access Operator.
        *
        *  Access the memory of the raw pointer.
        *
        **/
        TypeName* operator->() const
        {
            if(data == nullptr)
            {
                MUTEX.unlock();
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));
            }

           return data;
        }
    };


    /** atomic_ptr
     *
     *  Protects a pointer with a mutex.
     *
     **/
    template<class TypeName>
    class atomic_ptr
    {
        /** The internal locking mutex. **/
        mutable std::mutex MUTEX;


        /** The internal raw poitner. **/
        TypeName* data;


    public:

        /** Default Constructor. **/
        atomic_ptr()
        : MUTEX()
        , data(nullptr)
        {
        }


        /** Constructor for storing. **/
        atomic_ptr(TypeName* dataIn)
        : MUTEX()
        , data(dataIn)
        {
        }


        /** Copy Constructor. **/
        atomic_ptr(const atomic_ptr<TypeName>& pointer) = delete;


        /** Move Constructor. **/
        atomic_ptr(const atomic_ptr<TypeName>&& pointer)
        : data(pointer.data)
        {
        }


        /** Destructor. **/
        ~atomic_ptr()
        {
        }


        /** Assignment operator.
         *
         *  @param[in] dataIn The atomic to assign from.
         *
         **/
        atomic_ptr& operator=(const atomic_ptr<TypeName>& dataIn)
        {
            data = dataIn.data;

            return (*this);
        }


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName& dataIn) const
        {
            LOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                return false;

            return *data == dataIn;
        }


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName* ptr) const
        {
            LOCK(MUTEX);

            return data == ptr;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName* ptr) const
        {
            LOCK(MUTEX);

            return data != ptr;
        }

        /** Not operator
         *
         *  Check if the pointer is nullptr.
         *
         **/
        bool operator!(void)
        {
            LOCK(MUTEX);

            return data == nullptr;
        }


        /** Member access overload
         *
         *  Allow atomic_ptr access like a normal pointer.
         *
         **/
        TypeName* operator->()
        {
            LOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access on a nullptr"));

            return data;
            //return lock_proxy<TypeName>(data, MUTEX);
        }


        /** dereference operator overload
         *
         *  Load the object from memory.
         *
         **/
        TypeName operator*() const
        {
            LOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "dereferencing a nullptr"));

            return *data;
        }


        TypeName* load()
        {
            LOCK(MUTEX);

            return data;
        }


        /** store
         *
         *  Stores an object into memory.
         *
         *  @param[in] dataIn The data into protected memory.
         *
         **/
        void store(TypeName* dataIn)
        {
            LOCK(MUTEX);

            if(data)
                delete data;

            data = dataIn;
        }


        /** reset
         *
         *  Reset the internal memory of the atomic pointer.
         *
         **/
        void free()
        {
            LOCK(MUTEX);

            if(data)
                delete data;

            data = nullptr;
        }


        void print()
        {
            LOCK(MUTEX);

            printf("pointer %llu mutex %llu\n", (uint64_t)data, (uint64_t)&MUTEX);
        }
    };
}

#endif
