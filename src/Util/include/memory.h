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
#include <Util/include/filesystem.h>
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
    template<class TypeName> class encrypted_type : memory::encrypted
    {
    public:
        /* Constructor from primitive type data */
        encrypted_type(TypeName data){DATA = data;}

        /* Encrypts the internal data */
        void Encrypt(){ encrypt(DATA);}

        /* The primitive data being encrypted */
        TypeName DATA;
    };


    /** mmap_context
     *
     *  Context to hold information pertaining to an active memory mapping.
     *
     **/
    struct mmap_context
    {

        /** Beginning of the mapped address space. **/
        char* data;


        /** Length of the requested file mapping. **/
        uint64_t nLength;


        /** Length of the actual mapping based on page boundaries. **/
        uint64_t nMappedSize;


        /** The file handle to reference memory mapping file descriptor. **/
        file_t hFile;


        /** The file mapping handle to reference memory mapping file descriptor. **/
        file_t hMapping;


        /** Default Constructor. **/
        mmap_context()
        : data        (nullptr)
        , nLength     (0)
        , nMappedSize (0)
        , hFile       (INVALID_HANDLE_VALUE)
        {
        }

        /** IsNull
         *
         *  Checks if the context is in a null state.
         *
         **/
        bool IsNull() const
        {
            return data == nullptr && nLength == 0;
        }


        /** size
         *
         *  Get the current size of the context.
         *
         **/
        size_t size() const
        {
            return nLength;
        }


        /** mapped_length
         *
         *  Get the current size of the mapped memory which is adjusted for page boundaries.
         *
         **/
        size_t mapped_length() const
        {
            return nMappedSize;
        }


        /** offset
         *
         *  Find offset relative to file's start, where the mapping was requested.
         *
         **/
        size_t offset() const
        {
            return nMappedSize - nLength;
        }


        /** begin
         *
         *  The memory address of the start of this mmap.
         *
         **/
        char* begin()
        {
            return data;
        }


        /** begin
         *
         *  The memory address of the start of this mmap.
         *
         **/
        const char* begin() const
        {
            return data;
        }


        /** get_mapping_start
         *
         *  Get the mapping beginning address location.
         *
         **/
        char* get_mapping_start()
        {
            return data ? (data - offset()) : nullptr;
        }


        /** get_mapping_start
         *
         *  Get the mapping beginning address location.
         *
         **/
        const char* get_mapping_start() const
        {
            return data ? (data - offset()) : nullptr;
        }
    };


    /** @Class mstream
     *
     *  RAII wrapper around mio::mmap library.
     *
     *  Allows reading and writing on memory mapped files with an interface that closely resembles std::fstream.
     *  Keeps track of reading and writing iterators and seeks over the memory map rather than through a buffer
     *  and a disk seek. Allows interchangable swap ins and outs for regular buffered io or mmapped io.
     *
     **/
    class mstream
    {
        std::mutex MUTEX;

        /** Internal enumeration flags for mimicking fstream flags. **/
        enum STATE
        {
            BAD  = (1 << 1),
            FAIL = (1 << 2),
            END  = (1 << 3),
            FULL = (1 << 4),
        };


        /** MMAP object from mio wrapper. **/
        mmap_context CTX;


        /** Binary position of reading pointer. **/
        uint64_t GET;


        /** Binary position of writing pointer. **/
        uint64_t PUT;


        /** Total bytes recently read. **/
        uint64_t COUNT;


        /** Current status of stream object. **/
        mutable std::atomic<uint8_t> STATUS;


        /** Current input flags to set behavior. */
        uint8_t FLAGS;

    public:

        /** Don't allow empty constructors. **/
        mstream() = delete;


        /** Construct with correct flags and path to file. **/
        mstream(const std::string& strPath, const uint8_t nFlags)
        : MUTEX  ( )
        , CTX    ( )
        , GET    (0)
        , PUT    (0)
        , STATUS (0)
        , FLAGS  (nFlags)
        {
            open(strPath, nFlags);
        }


        /** Copy Constructor deleted because mmap_sink is move-only. **/
        mstream(const mstream& stream) = delete;


        /** Move Constructor. **/
        mstream(mstream&& stream)
        : MUTEX  ( )
        , CTX    (std::move(stream.CTX))
        , GET    (std::move(stream.GET))
        , PUT    (std::move(stream.PUT))
        , STATUS (stream.STATUS.load())
        , FLAGS  (std::move(stream.FLAGS))
        {
        }


        /** Copy Assignment deleted because mmap_sink is move-only. **/
        mstream& operator=(const mstream& stream) = delete;


        /** Move Assignment. **/
        mstream& operator=(mstream&& stream)
        {
            CTX    = std::move(stream.CTX);
            GET    = std::move(stream.GET);
            PUT    = std::move(stream.PUT);
            STATUS = stream.STATUS.load();
            FLAGS  = std::move(stream.FLAGS);

            return *this;
        }


        /** Default destructor. **/
        ~mstream()
        {
            /* Close on destruct. */
            close();
        }


        void open(const std::string& strPath, const uint8_t nFlags)
        {
            LOCK(MUTEX);

            /* Set the appropriate flags. */
            FLAGS = nFlags;

            /* Attempt to open the file. */
            file_t hFile = filesystem::open_file(strPath);
            if(hFile == INVALID_HANDLE_VALUE)
            {
                STATUS |= STATE::BAD;
                return;
            }

            /* Get the filesize for mapping. */
            int64_t nFileSize = filesystem::query_file_size(hFile);
            if(nFileSize == INVALID_HANDLE_VALUE)
            {
                STATUS |= STATE::FAIL;
                return;
            }


            /* Attempt to map the file. */
            if(!memory_map(hFile, 0, nFileSize))
            {
                STATUS |= STATE::FAIL;
                return;
            }
        }


        /** is_open
         *
         *  Checks if the mmap is currently open for reading.
         *
         **/
        bool is_open() const
        {
            return CTX.hFile != INVALID_HANDLE_VALUE;
        }


        /** good
         *
         *  Checks if the mmap is in an operable state.
         *
         **/
        bool good() const
        {
            /* Check our status bits first. */
            if(STATUS & STATE::BAD || STATUS & STATE::FAIL || STATUS & STATE::END)
                return false;

            return is_open();
        }


        /** bad
         *
         *  Checks if the mmap has failed or reached end of file.
         *
         **/
        bool bad() const
        {
            /* Check our failbits first. */
            if(STATUS & STATE::BAD || STATUS & STATE::FAIL)
                return true;

            /* Check the reverse of good(). */
            return false;
        }


        /** fail
         *
         *  Checks if the mmap has failed at any point.
         *
         **/
        bool fail() const
        {
            /* Check our failbits first. */
            if(STATUS & STATE::FAIL)
                return true;

            return false;
        }


        /** eof
         *
         *  Checks if the end of file has been found.
         *
         **/
        bool eof() const
        {
            /* Check if our end of file flag has been set. */
            if(STATUS & STATE::END)
                return true;

            return false;
        }


        /** ! operator
         *
         *  Check that the current mmap is in good condition.
         *
         **/
        bool operator!(void) const
        {
            return !good();
        }


        /** tellg
         *
         *  Gives current reading position for the memory map
         *
         **/
        uint64_t tellg() const
        {
            return GET;
        }


        /** tellp
         *
         *  Gives current writing position for the memory map
         *
         **/
        uint64_t tellp() const
        {
            return PUT;
        }


        /** gcount
         *
         *  Tell us how many bytes we have just read.
         *
         **/
        uint64_t gcount() const
        {
            return COUNT;
        }


        /** seekg
         *
         *  Sets the current reading position for memory map.
         *
         *  @param[in] nPos The binary position in bytes to seek to
         *
         *  @return reference of stream
         *
         **/
        mstream& seekg(const uint64_t nPos)
        {
            //LOCK(MUTEX);

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::in))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Unset the eof bit. */
            STATUS &= ~(STATE::END);
            GET = nPos;

            /* Check we aren't out of bounds. */
            if(GET > CTX.size())
                STATUS |= (STATE::END);

            return *this;
        }


        /** seekg
         *
         *  Sets the current reading position for memory map.
         *
         *  @param[in] nPos The binary position in bytes to seek to
         *  @param[in] nFlag The flag to adjust seeking behavior.
         *
         *  @return reference of stream
         *
         **/
        mstream& seekg(const uint64_t nPos, const uint8_t nFlag)
        {
            //LOCK(MUTEX);

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::in))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Unset the eof bit. */
            STATUS &= ~(STATE::END);

            /* Handle a seek from different flags. */
            switch(nFlag)
            {
                case std::ios::beg:
                {
                    GET = nPos;
                    break;
                }

                case std::ios::end:
                {
                    GET = (CTX.size() - nPos);
                    break;
                }

                case std::ios::cur:
                {
                    GET += nPos;
                    break;
                }
            }

            /* Check we aren't out of bounds. */
            if(GET > CTX.size())
                STATUS |= (STATE::END);

            return *this;
        }


        /** seekp
         *
         *  Sets the current writing position for memory map.
         *
         *  @param[in] nPos The binary position in bytes to seek to
         *
         *  @return reference of stream
         *
         **/
        mstream& seekp(const uint64_t nPos)
        {
            LOCK(MUTEX);

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::out))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Unset the eof bit. */
            STATUS &= ~(STATE::END);
            PUT = nPos;

            /* Check we aren't out of bounds. */
            if(PUT > CTX.size())
                STATUS |= (STATE::END);

            return *this;
        }


        /** seekp
         *
         *  Sets the current writing position for memory map.
         *
         *  @param[in] nPos The binary position in bytes to seek to
         *  @param[in] nFlag The flag to adjust seeking behavior.
         *
         *  @return reference of stream
         *
         **/
        mstream& seekp(const uint64_t nPos, const uint8_t nFlag)
        {
            LOCK(MUTEX);

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::out))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Unset the eof bit. */
            STATUS &= ~(STATE::END);

            /* Handle a seek from different flags. */
            switch(nFlag)
            {
                case std::ios::beg:
                {
                    PUT = nPos;
                    break;
                }

                case std::ios::end:
                {
                    PUT = (CTX.size() - nPos);
                    break;
                }

                case std::ios::cur:
                {
                    PUT += nPos;
                    break;
                }
            }

            /* Check we aren't out of bounds. */
            if(PUT > CTX.size())
            {
                STATUS |= (STATE::END);
            }

            return *this;
        }




        /** flush
         *
         *  Flushes buffers to disk from memorymap.
         *
         *  @return reference of stream
         *
         **/
        mstream& flush()
        {
            /* Check that we are properly mapped. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return *this;
            }

            /* Sync to filesystem. */
            if(FLAGS & std::ios::out && STATUS & STATE::FULL)
                sync();

            /* Let the object know we completed flush. */
            STATUS &= ~STATE::FULL;

            return *this;
        }


        /** write
         *
         *  Writes a series of bytes into memory mapped file
         *
         *  @param[in] pBuffer The starting address of buffer to write.
         *  @param[in] nSize The size to write to memory map.
         *
         *  @return reference of stream
         *
         **/
        mstream& write(const char* pBuffer, const uint64_t nSize)
        {
            LOCK(MUTEX);

            /* Check that we are properly mapped. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return *this;
            }

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::out))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Check if we will write to the end of file. */
            if(PUT + nSize > CTX.size())
            {
                STATUS |= (STATE::END);
                return *this;
            }

            /* Copy the input buffer into memory map. */
            std::copy((uint8_t*)pBuffer, (uint8_t*)pBuffer + nSize, (uint8_t*)CTX.begin() + PUT);

            /* Let the object know we are ready to flush. */
            STATUS |= STATE::FULL;

            /* Increment our write position. */
            PUT += nSize;

            return *this;
        }


        /** write
         *
         *  Writes a series of bytes into memory mapped file
         *
         *  @param[in] pBuffer The starting address of buffer to write.
         *  @param[in] nSize The size to write to memory map.
         *
         *  @return reference of stream
         *
         **/
        bool write(const char* pBuffer, const uint64_t nSize, const uint64_t nPos) const
        {
            /* Check that we are properly mapped. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return debug::error("stream is not opened");
            }

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::out))
            {
                STATUS |= (STATE::BAD);
                return debug::error("stream not open for writing");
            }

            /* Check if we will write to the end of file. */
            if(nPos + nSize > CTX.size())
            {
                STATUS |= (STATE::END);
                return debug::error("reached end of stream");
            }

            /* Copy the input buffer into memory map. */
            std::copy((uint8_t*)pBuffer, (uint8_t*)pBuffer + nSize, (uint8_t*)CTX.begin() + nPos);

            /* Let the object know we are ready to flush. */
            STATUS |= STATE::FULL;

            return true;
        }


        /** read
         *
         *  Reads a series of bytes from memory mapped file
         *
         *  @param[in] pBuffer The starting address of buffer to read.
         *  @param[in] nSize The size to read from memory map.
         *
         *  @return reference of stream
         *
         **/
        mstream& read(char* pBuffer, const uint64_t nSize)
        {
            //LOCK(MUTEX);

            /* Check that we are properly mapped. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return *this;
            }

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::in))
            {
                STATUS |= (STATE::BAD);
                return *this;
            }

            /* Check if we will write to the end of file. */
            if(GET + nSize > CTX.size())
            {
                STATUS |= (STATE::END);
                return *this;
            }

            /* Copy the memory mapped buffer into return buffer. */
            std::copy((uint8_t*)CTX.begin() + GET, (uint8_t*)CTX.begin() + GET + nSize, (uint8_t*)pBuffer);

            /* Increment our write position. */
            GET  += nSize;
            COUNT = nSize;

            return *this;
        }


        /** read
         *
         *  Reads a series of bytes from memory mapped file
         *
         *  @param[in] pBuffer The starting address of buffer to read.
         *  @param[in] nSize The size to read from memory map.
         *
         *  @return reference of stream
         *
         **/
        bool read(char* pBuffer, const uint64_t nSize, const uint64_t nPos) const
        {
            //LOCK(MUTEX);

            /* Check that we are properly mapped. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return false;
            }

            /* Check our stream flags. */
            if(!(FLAGS & std::ios::in))
            {
                STATUS |= (STATE::BAD);
                return false;
            }

            /* Check if we will write to the end of file. */
            if(nPos + nSize > CTX.size())
            {
                STATUS |= (STATE::END);
                return false;
            }

            /* Copy the memory mapped buffer into return buffer. */
            std::copy((uint8_t*)CTX.begin() + nPos, (uint8_t*)CTX.begin() + nPos + nSize, (uint8_t*)pBuffer);

            return true;
        }


        /** close
         *
         *  Closes the mstream mmap handle and flushes to disk.
         *
         **/
        void close()
        {
            /* Sync to filesystem. */
            if(FLAGS & std::ios::out && (STATUS & STATE::FULL))
                sync();

            /* Let the object know we completed flush. */
            STATUS &= ~STATE::FULL;

            /* Unmap the file now. */
            unmap();
        }


    private:


        /** memory_map
         *
         *  Map a given file to a virtual memory location for use in reading and writing.
         *
         **/
        bool memory_map(const file_t hFile, const uint64_t nOffset, const uint64_t nLength)
        {
            /* Find our page alignment boundaries. */
            const int64_t nAlignedOffset = filesystem::make_offset_page_aligned(nOffset);
            const int64_t nMappingSize   = nOffset - nAlignedOffset + nLength;

        #ifdef _WIN32

            /* Create the file mapping. */
            const int64_t nMaxFilesize = nOffset + nLength;
            const file_t hFile = ::CreateFileMapping(
                    hFile,
                    0,
                    PAGE_READWRITE,
                    int64_high(nMaxFilesize),
                    int64_low(nMaxFilesize),
                    0);

            /* Check for file handle failures. */
            if(hFile == INVALID_HANDLE_VALUE)
                return debug::error(FUNCTION, "failed to establish memory mapping");

            /* Get the mapping information. */
            char* pBegin = static_cast<char*>(::MapViewOfFile(
                    hFile,
                    FILE_MAP_WRITE,
                    int64_high(nAlignedOffset),
                    int64_low(nAlignedOffset),
                    nMappingSize));

            /* Check for mapping failures. */
            if(pBegin == nullptr)
                return debug::error(FUNCTION, "failed to establish page view");
        #else // POSIX

            /* Establish a new mapping. */
            char* pBegin = static_cast<char*>(::mmap(
                    0, // Don't give hint as to where to map.
                    nMappingSize,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    hFile,
                    nAlignedOffset));

            /* Let the OS know we will be accessing in random ordering. */
            posix_madvise(pBegin + nOffset - nAlignedOffset, nMappingSize, POSIX_MADV_RANDOM);

            /* Check for mapping failures. */
            if(pBegin == MAP_FAILED)
                return debug::error(FUNCTION, "failed to create memory mapping: ", std::strerror(errno));
        #endif

            /* Populate the context to return. */
            CTX.data        = pBegin + nOffset - nAlignedOffset;
            CTX.nLength     = nLength;
            CTX.nMappedSize = nMappingSize;
            CTX.hFile       = hFile;

            return true;
        }


        void unmap()
        {
            if(!is_open()) { return; }
            // TODO do we care about errors here?
        #ifdef _WIN32
            //if(is_mapped()) TODO: this needs to check the mapping handle fd
            {
                ::UnmapViewOfFile(CTX.get_mapping_start());
                //::CloseHandle(CTX.hFile); TODO: we need to capture separate windoze related handle to close mmap
            }
        #else // POSIX
            if(CTX.data) { ::munmap(const_cast<char*>(CTX.get_mapping_start()), CTX.mapped_length()); }
        #endif

            /* Close our file handles. */
        #ifdef _WIN32
                ::CloseHandle(CTX.hFile);
        #else // POSIX
                ::close(CTX.hFile);
        #endif

            // Reset fields to their default values.
            CTX.data        = nullptr;
            CTX.nMappedSize = 0;
            CTX.nLength     = 0;
            CTX.hFile       = INVALID_HANDLE_VALUE;
            //TODO: windoze extra handle
        }


        /** sync
         *
         *  Tell the OS to update the modified pages to disk.
         *
         **/
        void sync()
        {
            /* Check for invalid file handles. */
            if(!is_open())
            {
                STATUS |= STATE::FAIL;
                return;
            }

            /* Check that there is data to write. */
            if(!(STATUS & STATE::FULL))
                return;

            /* Only sync if not there's data mapped. */
            if(CTX.data)
            {
        #ifdef _WIN32
                if(::FlushViewOfFile(CTX.get_mapping_start(), CTX.mapped_length()) == 0
                   || ::FlushFileBuffers(CTX.hFile) == 0)
        #else // POSIX
                if(::msync(CTX.get_mapping_start(), CTX.mapped_length(), MS_ASYNC) != 0)
        #endif
                {
                    STATUS |= STATE::FAIL;
                    return;
                }
            }
        #ifdef _WIN32
            if(::FlushFileBuffers(CTX.hFile) == 0)
            {
                STATUS |= STATE::FAIL;
                return;
            }
        #endif
        }
    };
}

#endif
