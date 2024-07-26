/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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
#include <Util/include/allocators.h>

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


    /** encrypted
     *
     *  Abstract base class for encrypting specific data types in a class.
     *
     **/
    class encrypted
    {
    public:

        /** Special method for encrypting specific member variables of class. **/
        virtual void Encrypt() = 0;


        /** Virtual Destructor. **/
        virtual ~encrypted()
        {

        }

    protected:

        /** encrypt memory
         *
         *  Encrypt or Decrypt a pointer.
         *
         **/
        template<class TypeName>
        void encrypt(const TypeName& data)
        {
            static bool fKeySet = false;
            static std::vector<uint8_t> vKey(AES_KEYLEN);
            static std::vector<uint8_t> vIV(AES_BLOCKLEN);

            /* Set the encryption key if not set. */
            if(!fKeySet)
            {
                RAND_bytes((uint8_t*)&vKey[0], AES_KEYLEN);
                RAND_bytes((uint8_t*)&vIV[0], AES_BLOCKLEN);

                fKeySet = true;
            }

            /* Create the AES context. */
            //struct AES_ctx ctx;
            //AES_init_ctx_iv(&ctx, &vKey[0], &vIV[0]);

            /* Encrypt the buffer data. */
            //AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)&data, sizeof(data));
        }


        /** encrypt memory
         *
         *  Encrypt or Decrypt a pointer.
         *
         **/
        template<class TypeName>
        void encrypt(const std::vector<TypeName>& data)
        {
            static bool fKeySet = false;
            static std::vector<uint8_t> vKey(AES_KEYLEN);
            static std::vector<uint8_t> vIV(AES_BLOCKLEN);

            /* Set the encryption key if not set. */
            if(!fKeySet)
            {
                RAND_bytes((uint8_t*)&vKey[0], AES_KEYLEN);
                RAND_bytes((uint8_t*)&vIV[0], AES_BLOCKLEN);

                fKeySet = true;
            }

            /* Create the AES context. */
            //struct AES_ctx ctx;
            //AES_init_ctx_iv(&ctx, &vKey[0], &vIV[0]);

            /* Encrypt the buffer data. */
            //AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)&data[0], data.size() * sizeof(TypeName));
        }


        /** encrypt memory
         *
         *  Encrypt or Decrypt a pointer.
         *
         **/
        void encrypt(const std::string& data)
        {
            static bool fKeySet = false;
            static std::vector<uint8_t> vKey(AES_KEYLEN);
            static std::vector<uint8_t> vIV(AES_BLOCKLEN);

            /* Set the encryption key if not set. */
            if(!fKeySet)
            {
                RAND_bytes((uint8_t*)&vKey[0], AES_KEYLEN);
                RAND_bytes((uint8_t*)&vIV[0], AES_BLOCKLEN);

                fKeySet = true;
            }

            /* Create the AES context. */
            //struct AES_ctx ctx;
            //AES_init_ctx_iv(&ctx, &vKey[0], &vIV[0]);

            /* Encrypt the buffer data. */
            //AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)&data[0], data.size());
        }



        /** encrypt memory
         *
         *  Encrypt or Decrypt a pointer.
         *
         **/
        void encrypt(const SecureString& data)
        {
            static bool fKeySet = false;
            static std::vector<uint8_t> vKey(AES_KEYLEN);
            static std::vector<uint8_t> vIV(AES_BLOCKLEN);

            /* Set the encryption key if not set. */
            if(!fKeySet)
            {
                RAND_bytes((uint8_t*)&vKey[0], AES_KEYLEN);
                RAND_bytes((uint8_t*)&vIV[0], AES_BLOCKLEN);

                fKeySet = true;
            }

            /* Create the AES context. */
            //struct AES_ctx ctx;
            //AES_init_ctx_iv(&ctx, &vKey[0], &vIV[0]);

            /* Encrypt the buffer data. */
            //AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)&data[0], data.size());
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


        /** decrypted_proxy
         *
         *  Temporary class that unlocks a mutex when outside of scope.
         *  Useful for protecting member access to a raw pointer and handling decryption.
         *
         **/
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
                /* Lock the mutex. */
                MUTEX.lock();

                /* Decrypt memory on first proxy. */
                if(nRefs == 0)
                {
                    data->Encrypt();
                }

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
                {
                    data->Encrypt();

                }

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
            RECURSIVE(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                return false;

            /* Decrypt the pointer. */
            data->Encrypt();

            /* Check equivilence. */
            bool fEquals = (*data == dataIn);

            /* Encrypt the poitner. */
            data->Encrypt();

            return fEquals;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        bool operator!=(const TypeName& dataIn) const
        {
            RECURSIVE(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                return false;

            /* Decrypt the pointer. */
            data->Encrypt();

            /* Check equivilence. */
            bool fNotEquals = (*data != dataIn);

            /* Encrypt the poitner. */
            data->Encrypt();

            return fNotEquals;
        }


        /** Not equivilent operator.
         *
         *  @param[in] a The data type to compare to.
         *
         **/
        TypeName operator*() const
        {
            RECURSIVE(MUTEX);

            /* Throw an exception on nullptr. */
            if(data == nullptr)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "member access to nullptr"));

            /* Decrypt the pointer. */
            data->Encrypt();

            /* Check equivilence. */
            const TypeName tRet = *data;

            /* Encrypt the poitner. */
            data->Encrypt();

            return tRet;
        }


        /** Not operator
         *
         *  Check if the pointer is nullptr.
         *
         **/
        bool operator!(void) const
        {
            RECURSIVE(MUTEX);

            return data == nullptr;
        }


        /** Member access overload
         *
         *  Allow encrypted_ptr access like a normal pointer.
         *
         **/
        decrypted_proxy operator->() const
        {
            return decrypted_proxy(data, MUTEX, nRefs);
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
            RECURSIVE(MUTEX);

            return data == nullptr;
       }


       /** SetNull
        *
        *  Sets this pointer to a null state without deleting the data.
        *
        **/
       void SetNull()
       {
           RECURSIVE(MUTEX);
           data = nullptr;
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
            RECURSIVE(MUTEX);

            /* Delete already allocated memory. */
            if(data)
            {
                data->Encrypt();
                delete data;
            }

            /* Shallow copy the pointer. */
            data = pdata;

            /* Encrypt the memory. */
            if(data)
                data->Encrypt();
        }


        /** free
         *
         *  Reset the internal memory of the encrypted pointer.
         *
         **/
        void free()
        {
            RECURSIVE(MUTEX);

            /* Free the memory. */
            if(data)
            {
                data->Encrypt();
                delete data;
            }

            /* Set the pointer to nullptr. */
            data = nullptr;
        }
    };


    /** encrypted_type
     *
     *  Simple template class to allow primitive types to be used with encrypted_ptr
     *  without needing to inherit from memory::encrypted or provide their own Encrypt() implementation
     *
     **/
    template<class TypeName> class encrypted_type : public memory::encrypted
    {
        /* The primitive data being encrypted */
        TypeName DATA;

    public:

        /* Constructor from primitive type data */
        encrypted_type(TypeName tDataIn)
        : DATA  (tDataIn)
        {
        }

        /* Encrypts the internal data */
        void Encrypt()
        {
            encrypt(DATA);
        }


        /** get
         *
         *  Allow encrypted_type access with decryption.
         *
         **/
        TypeName get() const
        {
            /* Decrypt our data. */
            encrypt(DATA);

            //make a copy
            const TypeName RET = DATA;

            /* Encrypt our data. */
            encrypt(DATA);

            return std::move(RET);
        }
    };
}

#endif
