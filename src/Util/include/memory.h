/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_MEMORY_H
#define NEXUS_UTIL_INCLUDE_MEMORY_H

#include <cstdint>

#include <LLC/aes/aes.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>

#include <openssl/rand.h>

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


    /** copy
     *
     *  Copies from two sets of iteratos and checks the sizes for potential buffer
     *  overflows.
     *
     *  @param[in] src_begin The source beginning iterator
     *  @param[in] src_end The source ending iterator
     *  @param[in] dst_begin The destination beginning iterator
     *  @param[in] dst_end The destination ending iterator
     *
     **/
    template<typename Type>
    void copy(const Type* src_begin, const Type* src_end , const Type* dst_begin, const Type* dst_end)
    {
        /* Check the source iterators. */
        if(src_end < src_begin)
            throw std::domain_error("src end iterator less than src begin");

        /* Check the destination iterators. */
        if(dst_end < dst_begin)
            throw std::domain_error("dst end iterator less than dst begin");

        /* Check the sizes. */
        if(src_end - src_begin != dst_end - dst_begin)
            throw std::domain_error("src size mismatch with dst size");

        /* Copy the data. */
        std::copy((Type*)src_begin, (Type*)src_end, (Type*)dst_begin);
    }


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
        std::recursive_mutex& MUTEX;


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
        lock_proxy(TypeName* pData, std::recursive_mutex& MUTEX_IN)
        : MUTEX(MUTEX_IN)
        , data(pData)
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
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

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
        mutable std::recursive_mutex MUTEX;


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


        /** Assignment operator. **/
        atomic_ptr& operator=(const atomic_ptr<TypeName>& dataIn) = delete;


        /** Assignment operator. **/
        atomic_ptr& operator=(TypeName* dataIn) = delete;


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName& dataIn) const
        {
            RLOCK(MUTEX);

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
            RLOCK(MUTEX);

            return data == ptr;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName* ptr) const
        {
            RLOCK(MUTEX);

            return data != ptr;
        }

        /** Not operator
         *
         *  Check if the pointer is nullptr.
         *
         **/
        bool operator!(void)
        {
            RLOCK(MUTEX);

            return data == nullptr;
        }


        /** Member access overload
         *
         *  Allow atomic_ptr access like a normal pointer.
         *
         **/
        lock_proxy<TypeName> operator->()
        {
            MUTEX.lock();

            return lock_proxy<TypeName>(data, MUTEX);
        }


        /** dereference operator overload
         *
         *  Load the object from memory.
         *
         **/
        TypeName operator*() const
        {
            RLOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "dereferencing a nullptr"));

            return *data;
        }

        TypeName* load()
        {
            RLOCK(MUTEX);

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
            RLOCK(MUTEX);

            if(data)
                delete data;

            data = dataIn;
        }


        /** free
         *
         *  Free the internal memory of the encrypted pointer.
         *
         **/
        void free()
        {
            RLOCK(MUTEX);

            if(data)
                delete data;

            data = nullptr;
        }
    };



    /** The encrypted and decryption key TODO: make this randomized across the program. **/
    static std::vector<uint8_t> vKey(16);
    static bool fKeySet = false;


    /** encrypt memory
     *
     *  Encrypt or Decrypt a pointer.
     *
     **/
    template<class TypeName>
    void encrypt(TypeName* data, bool fEncrypt)
    {
        /* Set the encryption key if not set. */
        if(!fKeySet)
        {
            RAND_bytes((uint8_t*)&vKey[0], 16);

            fKeySet = true;
        }

        /* Get the size of memory. */
        size_t nSize = sizeof(*data);

        /* Copy memory into vector. */
        std::vector<uint8_t> vData(nSize + (nSize % 16));
        std::copy((uint8_t*)data, (uint8_t*)data + nSize, (uint8_t*)&vData[0]);

        /* Create the AES context. */
        struct AES_ctx ctx;
        AES_init_ctx(&ctx, &vKey[0]);

        /* Encrypt in block sizes of 16. */
        for(uint32_t i = 0; i < vData.size(); i += 16)
        {
            if(fEncrypt)
                AES_ECB_encrypt(&ctx, &vData[0] + i);
            else
                AES_ECB_decrypt(&ctx, &vData[0] + i);
        }

        /* Copy crypted data back into memory. */
        std::copy((uint8_t*)&vData[0], (uint8_t*)&vData[0] + nSize, (uint8_t*)data);
    }


    /** decrypted_proxy
     *
     *  Temporary class that unlocks a mutex when outside of scope.
     *  Useful for protecting member access to a raw pointer and handling decryption.
     *
     **/
    template <class TypeName>
    class decrypted_proxy
    {
        /** Reference of the mutex. **/
        std::recursive_mutex& MUTEX;

        /** The pointer being locked. **/
        TypeName* data;

        /** The reference count to ensure it knows when to re-encrypt the memory. */
        std::atomic<uint32_t>& nRefs;

    public:

        /** Basic constructor
         *
         *  Assign the pointer and reference to the mutex.
         *
         *  @param[in] pData The pointer to shallow copy
         *  @param[in] MUTEX_IN The mutex reference
         *
         **/
        decrypted_proxy(TypeName* pdata, std::recursive_mutex& MUTEX_IN, std::atomic<uint32_t>& nRefsIn)
        : MUTEX(MUTEX_IN)
        , data(pdata)
        , nRefs(nRefsIn)
        {
            /* Decrypt memory on first proxy. */
            if(nRefs == 0)
                encrypt(data, false);

            /* Increment the reference count. */
            ++nRefs;
        }


        /** Destructor
        *
        *  Unlock the mutex and encrypt memory.
        *
        **/
        ~decrypted_proxy()
        {
            /* Decrement the ref count. */
            --nRefs;

            /* Encrypt memory again when ref count is 0. */
            if(nRefs == 0)
                encrypt(data, true);

            /* Unlock the mutex. */
            MUTEX.unlock();
        }


        /** Member Access Operator.
        *
        *  Access the memory of the raw pointer.
        *
        **/
        TypeName* operator->() const
        {
            /* Stop member access if poitner is null. */
            if(data == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

            return data;
        }
    };


    /** encrypted_ptr
     *
     *  Protects a pointer with a mutex.
     *  Encrypts / Decrypts the memory allocated to pointer.
     *
     **/

    template<class TypeName>
    class encrypted_ptr
    {
        /** The internal locking mutex. **/
        mutable std::recursive_mutex MUTEX;

        /** The internal raw poitner. **/
        TypeName* data;

        /** Reference count for decrypted_proxy. **/
        mutable std::atomic<uint32_t> nRefs;

    public:

        /** Default Constructor. **/
        encrypted_ptr()
        : MUTEX()
        , data(nullptr)
        , nRefs(0)
        {
        }


        /** Create pointer by raw pointer data. **/
        encrypted_ptr(TypeName* pdata)
        : MUTEX()
        , data(nullptr)
        , nRefs(0)
        {
            store(pdata);
        }


        /** Create pointer from object. **/
        encrypted_ptr(const TypeName& type)
        : MUTEX()
        , data(nullptr)
        , nRefs(0)
        {
            store(new TypeName(type));
        }


        /** Copy Constructor. **/
        encrypted_ptr(const encrypted_ptr<TypeName>& pointer) = delete;


        /** Move Constructor. **/
        encrypted_ptr(const encrypted_ptr<TypeName>&& pointer)
        : data(pointer.data)
        , nRefs(pointer.nRefs.load())
        {
        }


        /** Destructor. **/
        ~encrypted_ptr()
        {
        }


        /** Copy Assignment operator. **/
        encrypted_ptr& operator=(const encrypted_ptr<TypeName>& pdata) = delete;


        /** Move Assignment operator. **/
        encrypted_ptr& operator=(const encrypted_ptr<TypeName>&& pdata)
        {
            data  = pdata.data;
            nRefs = pdata.nRefs.load();

            return *this;
        }


        /** Assignment operator. **/
        encrypted_ptr& operator=(TypeName* dataIn)
        {
            store(dataIn);

            return *this;
        }


        /** Equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator==(const TypeName& dataIn) const
        {
            RLOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                return false;

            /* Decrypt the pointer. */
            encrypt(data, false);

            /* Check equivilence. */
            bool fEquals = (*data == dataIn);

            /* Encrypt the poitner. */
            encrypt(data, true);

            return fEquals;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName& dataIn) const
        {
            RLOCK(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                return false;

            /* Decrypt the pointer. */
            encrypt(data, false);

            /* Check equivilence. */
            bool fNotEquals = (*data != dataIn);

            /* Encrypt the poitner. */
            encrypt(data, true);

            return fNotEquals;
        }


        /** Not operator
         *
         *  Check if the pointer is nullptr.
         *
         **/
        bool operator!(void) const
        {
            RLOCK(MUTEX);

            return data == nullptr;
        }


        /** Member access overload
         *
         *  Allow encrypted_ptr access like a normal pointer.
         *
         **/
        decrypted_proxy<TypeName> operator->() const
        {
            MUTEX.lock();

            return decrypted_proxy<TypeName>(data, MUTEX, nRefs);
        }


        /** IsNull
        *
        *  Determines if the internal data for this encrypted pointer is nullptr
        *
        *  @return True, if the internal data for this encrypted pointer is nullptr.
        *
        **/
       bool IsNull() const
       {
            RLOCK(MUTEX);

            return data == nullptr;
       }


        /** store
         *
         *  Stores an object into memory.
         *
         *  @param[in] dataIn The data into protected memory.
         *
         **/
        void store(TypeName* pdata)
        {
            RLOCK(MUTEX);

            /* Delete already allocated memory. */
            if(data)
                delete data;

            /* Shallow copy the pointer. */
            data = pdata;

            /* Encrypt the memory. */
            encrypt(data, true);
        }


        /** free
         *
         *  Reset the internal memory of the encrypted pointer.
         *
         **/
        void free()
        {
            RLOCK(MUTEX);

            /* Free the memory. */
            if(data)
            {
                encrypt(data, false);
                delete data;
            }

            /* Set the pointer to nullptr. */
            data = nullptr;
        }
    };
}

#endif
