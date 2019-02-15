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
        : data()
        {
        }


        /** Constructor for storing. **/
        atomic(const TypeName& dataIn)
        : data()
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


    /** atomic_ptr
     *
     *  Protects a pointer with a mutex.
     *
     **/
    template<class TypeName>
    class atomic_ptr
    {
        /* The mutex to protect the memory. */
        mutable std::mutex MUTEX;


        /* The internal data. */
        TypeName* data;


    public:

        /** Default Constructor. **/
        atomic_ptr()
        : data(nullptr)
        {
        }


        /** Constructor for storing. **/
        atomic_ptr(const TypeName& dataIn)
        : data(nullptr)
        {
            store(dataIn);
        }


        /** Destructor. **/
        ~atomic_ptr()
        {
            if(data)
                delete data;
        }


        /** Assignment operator.
         *
         *  @param[in] dataIn The atomic to assign from.
         *
         **/
        atomic_ptr& operator=(const TypeName& dataIn)
        {
            store(dataIn);

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


        /** Member access overload
         *
         *  Allow atomic_ptr access like a normal pointer.
         *
         **/
        TypeName* operator->()
        {
            LOCK(MUTEX);

            return data;
        }


        /** load
         *
         *  Load the object from memory.
         *
         **/
        TypeName* load() const
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
        void store(const TypeName& dataIn)
        {
            LOCK(MUTEX);

            if(data)
                delete data;

            data = new TypeName(dataIn);
        }


        /** store
         *
         *  Stores an object into memory.
         *
         *  @param[in] dataIn The data into protected memory.
         *
         **/
        void store(const TypeName* dataIn)
        {
            LOCK(MUTEX);

            if(data)
                delete data;

            if(dataIn)
                data = new TypeName(dataIn);
            else
                data = nullptr;
        }


        /** free
         *
         *  Free the internal memory of the atomic pointer.
         *
         **/
        void free()
        {
            LOCK(MUTEX);

            if(data)
                delete data;

            data = nullptr;
        }
    };
}

#endif
