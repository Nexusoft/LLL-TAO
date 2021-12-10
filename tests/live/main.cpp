/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/static.h>

#include <Util/system/include/debug.h>
#include <Util/encoding/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/encoding/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <system/include/runtime.h>

#include <list>
#include <functional>
#include <variant>

#include <Util/system/types/softfloat.h>
#include <system/include/filesystem.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/locator.h>

#include <LLD/config/hashmap.h>
#include <LLD/config/static.h>

class TestDB : public LLD::Templates::StaticDatabase<LLD::BinaryHashMap, LLD::BinaryLRU, LLD::Config::Hashmap>
{
public:
    TestDB(const LLD::Config::Static& sector, const LLD::Config::Hashmap& keychain)
    : StaticDatabase(sector, keychain)
    {
    }

    ~TestDB()
    {

    }

    bool WriteKey(const uint1024_t& key, const uint1024_t& value)
    {
        return Write(std::make_pair(std::string("key"), key), value);
    }


    bool ReadKey(const uint1024_t& key, uint1024_t &value)
    {
        return Read(std::make_pair(std::string("key"), key), value);
    }


    bool HasKey(const uint1024_t& key)
    {
        return Exists(std::make_pair(std::string("key"), key));
    }


    bool EraseKey(const uint1024_t& key)
    {
        return Erase(std::make_pair(std::string("key"), key));
    }


    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }

    bool ReadLast(uint1024_t &last)
    {
        return Read(std::string("last"), last);
    }

};

#include <TAO/Ledger/include/genesis_block.h>

const uint256_t hashSeed = 55;

#include <bitset>

#include <LLP/include/global.h>

#include <LLD/cache/template_lru.h>

#include <leveldb/c.h>

TestDB* bloom;

std::atomic<uint32_t> nTotalWritten;
std::atomic<uint32_t> nTotalRead;

void BatchWrite(runtime::stopwatch &swElapsed, runtime::stopwatch &swReaders)
{
    for(int n = 0; n < config::GetArg("-tests", 1); ++n)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "Generating Keys for test: ", n, "/", config::GetArg("-tests", 1), ANSI_COLOR_RESET);

        std::vector<uint1024_t> vKeys;
        for(int i = 0; i < config::GetArg("-total", 1); ++i)
            vKeys.push_back(LLC::GetRand1024());


        runtime::stopwatch swTimer;
        swTimer.start();
        swElapsed.start();

        uint32_t nTotal = 0;
        for(const auto& nBucket : vKeys)
        {
            ++nTotalWritten;

            ++nTotal;
            if(!bloom->WriteKey(nBucket, nBucket))
            {
                debug::error(FUNCTION, "failed to write ", VARIABLE(nTotal));
                return;
            }
        }


        swElapsed.stop();
        swTimer.stop();

        //runtime::sleep(10);
        //runtime::sleep(100 + LLC::GetRandInt(500));

        uint64_t nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0,  "[LLD] ", vKeys.size() / 1000, "k records written in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

        uint1024_t hashKey = 0;

        swTimer.reset();
        swTimer.start();

        swReaders.start();

        nTotal = 0;
        for(const auto& nBucket : vKeys)
        {
            ++nTotal;
            ++nTotalRead;

            if(!bloom->ReadKey(nBucket, hashKey))
            {
                debug::error("Failed to read ", nBucket.SubString(), " total ", nTotal);
                return;
            }
                //return
        }
        swTimer.stop();
        swReaders.stop();

        //runtime::sleep(10);

        nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0, "[LLD] ", vKeys.size() / 1000, "k records read in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vKeys.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);

        //runtime::sleep(100 + LLC::GetRandInt(500));

        //if(!bloom->EraseKey(vKeys[0]))
        //    return debug::error("failed to erase ", vKeys[0].SubString());

        //if(bloom->ReadKey(vKeys[0], hashKey))
        //    return debug::error("Failed to erase ", vKeys[0].SubString());
    }
}


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
    mutable std::shared_mutex MUTEX;

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


    const uint32_t LOCK_SIZE;


    /** Vector of locks for locking pages. **/
    mutable std::vector<std::shared_mutex> LOCKS;


    std::shared_mutex& PAGE(const uint64_t nPos) const
    {
        /* Find our page based on binary position. */
        const uint32_t nLock = (nPos / LOCK_SIZE); //TODO: we need to account for crossing page boundaries with nSize and lock

        /* Check our ranges. */ //TODO: remove for production
        if(nLock >= LOCKS.size())
            throw std::domain_error(debug::safe_printstr(FUNCTION, "lock out of bounds ", nLock, "/", LOCKS.size()));

        return LOCKS[nLock];
    }


public:

    /** Don't allow empty constructors. **/
    mstream() = delete;


    /** Construct with correct flags and path to file. **/
    mstream(const std::string& strPath, const uint8_t nFlags, const uint32_t nPagesPerLock = 1)
    : MUTEX          ( )
    , CTX            ( )
    , GET            (0)
    , PUT            (0)
    , STATUS         (0)
    , FLAGS          (nFlags)
    , LOCK_SIZE      (filesystem::page_size() * nPagesPerLock)
    , LOCKS          ( )
    {
        open(strPath, nFlags);

        /* Create the locks vector based on requested lock granularity. */
        const uint32_t nLocks =  (CTX.nMappedSize / LOCK_SIZE) + 1; //+1 to account for remainders
        LOCKS = std::vector<std::shared_mutex>(nLocks);
    }


    /** Copy Constructor deleted because mmap_sink is move-only. **/
    mstream(const mstream& stream) = delete;


    /** Move Constructor. **/
    mstream(mstream&& stream)
    : MUTEX          ( )
    , CTX            (std::move(stream.CTX))
    , GET            (std::move(stream.GET))
    , PUT            (std::move(stream.PUT))
    , STATUS         (stream.STATUS.load())
    , FLAGS          (std::move(stream.FLAGS))
    , LOCK_SIZE      (stream.LOCK_SIZE)
    , LOCKS          ( )
    {
        /* Create the locks vector based on requested lock granularity. */
        const uint32_t nLocks =  (CTX.nMappedSize / LOCK_SIZE) + 1; //+1 to account for remainders
        LOCKS = std::vector<std::shared_mutex>(nLocks);
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

        /* Create the locks vector based on requested lock granularity. */
        const uint32_t nLocks =  (CTX.nMappedSize / LOCK_SIZE) + 1; //+1 to account for remainders
        LOCKS = std::vector<std::shared_mutex>(nLocks);

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


        debug::log(0, "Created mmap of ", nFileSize / filesystem::page_size(), " blocks (", nFileSize, " bytes)");
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


    /** ! operator
     *
     *  Check that the current mmap is in good condition.
     *
     **/
    bool operator!(void) const
    {
        return !good();
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

        uint64_t nRemaining = nSize;
        uint64_t nWritePos  = nPos;
        uint64_t nIterator  = 0;
        do
        {
            WRITER_LOCK(PAGE(nWritePos));

            const uint32_t nLock = (nWritePos / LOCK_SIZE);
            const uint64_t nBoundary  = ((nLock + 1) * LOCK_SIZE);

            const uint64_t nMaxBytes = std::min(nRemaining, std::max(uint64_t(1), (nBoundary - nWritePos))); //need 1u otherwise we could loop indefinately

            //debug::log(0, "LOCK: ", VARIABLE(nWritePos), " | ", VARIABLE(nRemaining), " | ", VARIABLE(nMaxBytes), " | ", VARIABLE(nBoundary), " | ", VARIABLE(nLock));

            /* Copy the memory mapped buffer into return buffer. */
            std::copy((uint8_t*)pBuffer + nIterator, (uint8_t*)pBuffer + nIterator + nMaxBytes, (uint8_t*)CTX.begin() + nWritePos);

            nRemaining -= nMaxBytes;
            nIterator  += nMaxBytes;
            nWritePos  += nMaxBytes;
        }
        while(nRemaining > 0);

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
    bool read(char* pBuffer, const uint64_t nSize, const uint64_t nPos) const
    {

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


        uint64_t nRemaining = nSize;
        uint64_t nReadPos   = nPos;
        do
        {
            READER_LOCK(PAGE(nReadPos));

            const uint32_t nLock = (nReadPos / LOCK_SIZE);
            const uint64_t nBoundary  = ((nLock + 1) * LOCK_SIZE);

            const uint64_t nMaxBytes = std::min(nRemaining, std::max(uint64_t(1), (nBoundary - nReadPos))); //need 1u otherwise we could loop indefinately

            //debug::log(0, "LOCK: ", VARIABLE(nReadPos), " | ", VARIABLE(nRemaining), " | ", VARIABLE(nMaxBytes), " | ", VARIABLE(nBoundary), " | ", VARIABLE(nLock));

            /* Copy the memory mapped buffer into return buffer. */
            std::copy((uint8_t*)CTX.begin() + nReadPos, (uint8_t*)CTX.begin() + nReadPos + nMaxBytes, (uint8_t*)pBuffer);

            nRemaining -= nMaxBytes;
            nReadPos   += nMaxBytes;
        }
        while(nRemaining > 0);

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



#include <stdio.h>

class pfile
{
public:

    FILE* hFile;

    std::mutex MUTEX;

    std::thread::id THREAD;

    pfile(const std::string& strPath)
    : hFile  (nullptr)
    , MUTEX  ( )
    , THREAD (0)
    {
        hFile = fopen(strPath.c_str(), "rw");
    }

    bool IsNull() const
    {
        return hFile == nullptr;
    }

    void Bind() const
    {
    }

    bool Available() const
    {
        return true;
        //return (THREAD == 0)
    }

    ~pfile()
    {
        fclose(hFile);
    }
};



#include <atomic>

template<class T>
struct node
{
    T data;
    node* next;
    node(const T& data) : data(data), next(nullptr) {}
};

template<class T>
class stack
{
    std::atomic<node<T>*> head;
 public:
    void push(const T& data)
    {
        node<T>* new_node = new node<T>(data);

        // put the current value of head into new_node->next
        new_node->next = head.load(std::memory_order_seq_cst);

        // now make new_node the new head, but if the head
        // is no longer what's stored in new_node->next
        // (some other thread must have inserted a node just now)
        // then put that new head into new_node->next and try again
        while(!std::atomic_compare_exchange_weak_explicit(
                                &head,
                                &new_node->next,
                                new_node,
                                std::memory_order_release,
                                std::memory_order_seq_cst))
                ; // the body of the loop is empty
// note: the above loop is not thread-safe in at least
// GCC prior to 4.8.3 (bug 60272), clang prior to 2014-05-05 (bug 18899)
// MSVC prior to 2014-03-17 (bug 819819). See member function version for workaround
    }
};


struct Test
{
    uint64_t a;
    uint64_t b;
    uint64_t c;
};




#include <atomic/types/shared_ptr.h>


void PushThread(util::atomic::shared_ptr<Test>& s)
{
    for(int i = 0; i < 4000; ++i)
    {
        util::atomic::shared_ptr<Test> pNew;
        std::shared_ptr<Test> pExpected = s.load();
        do
        {
            pNew.store(*pExpected);
            pNew.load()->a++;
        }
        while(!s.compare_exchange_weak(pExpected, pNew.load()));
    }

    for(int i = 0; i < 4000; ++i)
    {
        util::atomic::shared_ptr<Test> pNew;
        std::shared_ptr<Test> pExpected = s.load();
        do
        {
            pNew.store(*pExpected);
            pNew.load()->b++;
        }
        while(!s.compare_exchange_weak(pExpected, pNew.load()));
    }
}


namespace util::system
{
    uint64_t nTesting;

    void log(uint32_t nLevel, std::string strOutput)
    {
        std::cout << strOutput << std::endl;
    }
}


#include <atomic/types/lock-shared-ptr.h>


namespace live::atomic
{
    template<typename Type> class dequeue
    {
        struct Anchor;

        struct Node
        {
            Type tData;
            std::atomic<Node*> pNext;
            std::atomic<Node*> pPrev;

            Node(const Type& rData)
            : tData (rData)
            , pNext (nullptr)
            , pPrev (nullptr)
            {
            }

            Node()
            : tData ()
            , pNext (nullptr)
            , pPrev (nullptr)
            {
            }

            Node* Prev() const
            {
                return reinterpret_cast<Node*>(uint64_t(pPrev.load()) & (1 << 2));
            }

            Node* Next() const
            {
                return reinterpret_cast<Node*>(uint64_t(pNext.load()) & (1 << 2));
            }


            void Prev(const Node* ptr)
            {
                pPrev.store(reinterpret_cast<Node*>(uint64_t(ptr) | (1 << 2)));
            }
        };

        struct Anchor
        {
            Node* pNext;
            Node* pPrev;
        };

        std::atomic<Node*> aHead;
        std::atomic<Node*> aTail;

        std::atomic<size_t> nSize;

        std::atomic<uint64_t> nForward;

    public:

        dequeue()
        : aHead (nullptr)
        , aTail (nullptr)
        , nSize (0)
        , nForward (0)
        {
        }


        void push_back(const Type& rData)
        {
            /* Create our new node to add to the queue. */
    		Node* pNode = new Node(rData);

            /* Check for our first insert. */
            if(aHead.load() == nullptr)
            {
                /* Set our initial head and tail values to new element. */
                aHead.store(pNode);
                aTail.store(pNode);

                /* Incrememnt our size atomically. */
                ++nSize;

                return;
            }

            /* Adjust our node's previous link now. */
            //Node* pTemp = aTail.load();
            //Node* pTail = aTail.load();
            //while(!pNode->pPrev.compare_exchange_weak(pTemp, pTail));

            /* We need this to hold our tail value for CAS for our tail pointer. */
            Node* pTail = nullptr;
            while(true)
            {
                /* Load our current tail pointer. */
                pTail = aTail.load();

                if(aTail.load() != pTail)
                {
                    debug::warning(FUNCTION, "Out of sync 1 ");
                    continue;
                }

                /* Check that tail doesn't have a next node. */
                Node* pTailNext = pTail->pNext.load();
                if(pTailNext)
                {
                    ++nForward;

                    /* Bring tail forward to next node. */
                    while(!aTail.compare_exchange_weak(pTail, pTailNext));
                    continue;
                }

                if(aTail.load() != pTail)
                {
                    debug::warning(FUNCTION, "Out of sync 2");
                    continue;
                }

                //pNode->Prev(pTail);

                /* Swing our tail next pointer forward to our new node. */
                if(pTail->pNext.compare_exchange_weak(pTailNext, pNode))
                {
                    pNode->pPrev.store(pTail);
                    break;
                }
            }

            /* Swing our new tail pointer to our new node. */
            while(!aTail.compare_exchange_weak(pTail, pNode));

            //while(!pNode->pPrev.compare_exchange_weak(pTail, pTail));
            //pNode->pPrev.store(pTail);
            //pNode->pPrev.store(pTail);

            /* Incrememnt our size atomically. */
            ++nSize;
        }

        void pop_back()
        {
            /* Grab our current head. */
            Node* pTail = nullptr;

            /* Loop to handle our CAS since we are using weak which can spuriously fail. */
            while(true)
            {
                /* Load our current tail pointer. */
                pTail = aTail.load();

                if(aTail.load() != pTail)
                {
                    debug::warning(FUNCTION, "Out of sync 1");
                    continue;
                }

                /* Load our next head pointer. */
                Node* pTailPrev = pTail->pPrev.load();
                if(!pTailPrev)
                {
                    debug::warning(FUNCTION, "no previous pointer for tail... empty?");
                    return;
                }

                Node* pTailCheck = pTailPrev->pNext.load();
                if(pTailCheck != pTail)
                {
                    debug::warning(FUNCTION, "incoherent pointer for tail, moving tail back...");

                    /* Bring tail backward to prev node. */
                    while(!pTailPrev->pNext.compare_exchange_weak(pTailCheck, pTail));
                    continue;
                }

                pTailPrev->pNext.store(nullptr);

                /* Swing our new atomic head to our next pointer. */
                if(aTail.compare_exchange_weak(pTail, pTailPrev))
                    break;
            }

            /* Decrement our size atomically. */
            --nSize;
        }


        void pop_front()
        {
            /* Grab our current head. */
            Node* pHead = aHead.load();

            /* Loop to handle our CAS since we are using weak which can spuriously fail. */
            while(true)
            {
                /* Load our next head pointer. */
                Node* pHeadNext = pHead->pNext.load();
                pHeadNext->pPrev.store(nullptr);

                /* Swing our new atomic head to our next pointer. */
                if(aHead.compare_exchange_weak(pHead, pHeadNext))
                    break;
            }

            /* Decrement our size atomically. */
            --nSize;
        }


        const Type& front() const
        {
            return aHead.load()->tData;
        }

        const Type& back() const
        {
            return aTail.load()->tData;
        }

        size_t size() const
        {
            return nSize.load();
        }

        bool empty() const
        {
            return (nSize.load() == 0);
        }

        uint32_t count() const
        {
            uint32_t nCount = 0;
            Node* pStart = aHead.load();

            while(pStart != nullptr)
            {
                pStart = pStart->pNext;

                ++nCount;
            }

            return nCount;
        }

        void print_next() const
        {
            uint32_t nCount = 0;
            Node* pStart = aHead.load();

            while(pStart != nullptr)
            {
                debug::log(0, "[", nCount++, "] ", pStart->tData);
                pStart = pStart->pNext.load();
            }
        }


        void print_prev() const
        {
            uint32_t nCount = 0;
            Node* pStart = aTail.load();

            while(pStart != nullptr)
            {
                debug::log(0, "[", nCount++, "] ", pStart->tData);
                pStart = pStart->pPrev.load();
            }
        }


        void check_coherent() const
        {
            std::vector<Type> vTypes;
            {
                Node* pStart = aHead.load();
                while(pStart != nullptr)
                {
                    vTypes.push_back(pStart->tData);
                    pStart = pStart->pNext.load();
                }
            }

            {
                Node* pStart = aTail.load();

                uint32_t nCount = 0;

                for(int32_t nIndex = vTypes.size() - 1; nIndex >= 0; --nIndex)
                {
                    if(pStart)
                    {
                        if(vTypes[nIndex] == pStart->tData)
                            debug::log(0, "[", ++nCount, "] ", ANSI_COLOR_BRIGHT_GREEN, pStart->tData, ANSI_COLOR_RESET , " | ", vTypes[nIndex]);
                        else
                            debug::log(0, "[", ++nCount, "] ", ANSI_COLOR_BRIGHT_RED, pStart->tData, ANSI_COLOR_RESET, " | ", vTypes[nIndex]);

                        pStart = pStart->pPrev.load();
                    }
                    else
                        debug::log(0, "[", ++nCount, "] ", ANSI_COLOR_BRIGHT_RED, vTypes[nIndex], ANSI_COLOR_RESET);
                }
            }

            if(nForward.load() > 0)
                debug::warning(FUNCTION, "moved tail forward ", nForward.load(), " times");
        }
    };
}


void TestThread(util::atomic::lock_shared_ptr<Test>& ptr)
{
    util::atomic::lock_shared_ptr<Test> ptrNew = ptr;

    for(int i = 0; i < 10000; ++i)
        ptr->c++;
}

void ListThread2(util::atomic::lock_shared_ptr<std::queue<uint32_t>>& ptr, runtime::stopwatch& swTimer, std::atomic<uint64_t>& raCount)
{
    for(int n = 0; n < 100000; ++n)
    {
        for(int i = 0; i < 4; ++i)
        {
            raCount++;

            swTimer.start();
            ptr->push(i);
            swTimer.stop();
        }

        //debug::log(0, "[PUSH]", VARIABLE(std::this_thread::get_id()), " | ", VARIABLE(ptr->front()), " | ", VARIABLE(ptr->back()), " | ", VARIABLE(ptr->size()));

        for(uint32_t i = 0; i < 3; ++i)
        {
            raCount++;

            swTimer.start();
            ptr->front();
            ptr->pop();
            swTimer.stop();
        }
    }


    //debug::log(0, "[POP]", VARIABLE(std::this_thread::get_id()), " | ", VARIABLE(ptr.front()), " | ", VARIABLE(ptr.back()), " | ", VARIABLE(ptr.size()));
}


void ListThread(live::atomic::dequeue<uint32_t>& ptr, runtime::stopwatch& swTimer, std::atomic<uint64_t>& raCount)
{
    for(int n = 0; n < 2; ++n)
    {
        for(int i = 0; i < 40; ++i)
        {
            raCount++;

            swTimer.start();
            ptr.push_back(LLC::GetRandInt(144));
            swTimer.stop();
        }

        //debug::log(0, "[PUSH]", VARIABLE(std::this_thread::get_id()), " | ", VARIABLE(ptr.front()), " | ", VARIABLE(ptr.back()), " | ", VARIABLE(ptr.size()));

        for(uint32_t i = 0; i < 39; ++i)
        {
            raCount++;

            swTimer.start();
            ptr.front();
            ptr.pop_back();
            swTimer.stop();
        }

    }


    //debug::log(0, "[POP]", VARIABLE(std::this_thread::get_id()), " | ", VARIABLE(ptr.front()), " | ", VARIABLE(ptr.back()), " | ", VARIABLE(ptr.size()));
}

#include <atomic/types/queue.h>

namespace proto::atomic
{
    template<typename Type>
    class dequeue
    {
        static const uint64_t nMask = (~uint64_t(0) >> 1);


        class Node
        {
        public:

            /** Pointer to next node in linked list. **/
            std::atomic<Node*> pPrev;


            /** Pointer to prev node in linked list. **/
            std::atomic<Node*> pNext;


            /** Data item that is being stored in node. **/
            Type tData;


            /** Default Constructor. **/
            Node() noexcept
            : pPrev (nullptr)
            , pNext (nullptr)
            , tData ( )
            {
            }


            /** Constructor taking data reference.
             *
             *  @param[in] tDataIn The data to store in this node.
             *
             **/
            Node(const Type& tDataIn) noexcept
            : pPrev (nullptr)
            , pNext (nullptr)
            , tData   (tDataIn)
            {
            }

        };


        class AnchorValue
        {
            /** Anchor pointer to next value in linked list. */
            Node* pTail;


            /** Anchor pointer to prev value in linked list. */
            Node* pHead;


        public:

            enum : uint8_t
            {
                STABLE = 0,
                RPUSH  = 1,
                LPUSH  = 2,
                ERROR  = 3,
            };

            AnchorValue() noexcept
            : pTail (nullptr)
            , pHead (nullptr)
            {
            }

            void SetTail(const Node* pNode)
            {
                pTail = reinterpret_cast<Node*>(uint64_t(pNode) & nMask);

                debug::log(0, FUNCTION, std::bitset<64>(pTail));
            }

            void SetHead(const Node* pNode)
            {
                pHead = reinterpret_cast<Node*>(uint64_t(pNode) & nMask);

                debug::log(0, FUNCTION, std::bitset<64>(pHead));
            }

            Node* GetTail() const
            {
                const auto pNode = reinterpret_cast<Node*>(uint64_t(pTail) & nMask);

                debug::log(0, FUNCTION, std::bitset<64>(pNode));
                return pNode;
            }

            Node* GetHead() const
            {
                const auto pNode = reinterpret_cast<Node*>(uint64_t(pHead) & nMask);

                debug::log(0, FUNCTION, std::bitset<64>(pNode));
                return pNode;
            }

            uint8_t GetStatus() const
            {
                const bool fTail = (uint64_t(pTail) & (1 << 1));
                const bool fHead = (uint64_t(pHead) & (1 << 1));

                /* If both pointers have bitset of 0, anchor is stable. */
                if(!fTail && !fHead)
                    return STABLE;

                /* If next pointer has bitset of 1, anchor is right-incoherent. */
                if(fTail && !fHead)
                    return RPUSH;

                /* If prev pointer has biset of 1, anchor is left-incorherent. */
                if(!fTail && fHead)
                    return LPUSH;

                return ERROR;
            }

            void SetStatus(const uint8_t nCode)
            {
                /* Switch based on our status code. */
                switch(nCode)
                {
                    /* Stable is indicated as both prev and next being set to 0. */
                    case STABLE:
                    {
                        pTail = reinterpret_cast<Node*>(uint64_t(pTail) & nMask);
                        pHead = reinterpret_cast<Node*>(uint64_t(pHead) & nMask);

                        return;
                    }

                    /* Right Push status is marked by a 0 on prev and a 1 on next. */
                    case RPUSH:
                    {
                        pTail = reinterpret_cast<Node*>(uint64_t(pTail) | (1 << 1)); //we add a bit to pNext for rPUSH
                        pHead = reinterpret_cast<Node*>(uint64_t(pHead) & nMask);

                        return;
                    }

                    /* Left Push status is marked by a 0 on next and a 1 on prev. */
                    case LPUSH:
                    {
                        pTail = reinterpret_cast<Node*>(uint64_t(pTail) & nMask);
                        pHead = reinterpret_cast<Node*>(uint64_t(pHead) | (1 << 1)); //we add a bit to pPrev for LPUSH

                        return;
                    }
                }

                /* If we fall through here from invalid code, case will indicate an error by setting both bits to 1. */
                pTail = reinterpret_cast<Node*>(uint64_t(pTail) | (1 << 1));
                pHead = reinterpret_cast<Node*>(uint64_t(pHead) | (1 << 1));
            }
        };


        class AnchorType
        {
        public:

            std::atomic<AnchorValue> tValue;


            void copy(AnchorType &tAnchor) __attribute__((noinline))
            {
                while(true) //loop CAS style
                {
                    AnchorValue tCopy = tValue.load();
                    if(tCopy != tValue.load())
                        continue;

                    tAnchor.store(tCopy);
                }
            }


            bool compare_exchange_weak(AnchorType* pExpectedIn, AnchorType* pNew)
            {
                AnchorValue tExpected = pExpectedIn->tValue.load();
                if(tValue.compare_exchange_weak(tExpected, pNew->tValue.load()))
                    return true;

                return false;
            }
        };

    public:


        AnchorType tAnchor;

        dequeue()
        : tAnchor ( )
        {
        }
    };





}

int main()
{
    const uint64_t nThreads = 8;

    util::system::nTesting = 0;

    util::system::log(0, "Testing");

    proto::atomic::dequeue<std::string> dequeue;

    {

        std::atomic<uint64_t> raCount(0);

        live::atomic::dequeue<uint32_t> listTest;

        std::vector<std::thread> vThreads(nThreads);
        std::vector<runtime::stopwatch> vTimers(nThreads);
        for(uint32_t n = 0; n < vThreads.size(); ++n)
        {
            vTimers[n]  = runtime::stopwatch();
            vThreads[n] = std::thread(ListThread, std::ref(listTest), std::ref(vTimers[n]), std::ref(raCount));
        }

        for(uint32_t n = 0; n < vThreads.size(); ++n)
            vThreads[n].join();

        uint64_t nElapsed = 0;
        for(uint32_t n = 0; n < vTimers.size(); ++n)
            nElapsed += vTimers[n].ElapsedMilliseconds();

        nElapsed /= vThreads.size();
        //nElapsed = raCount.load() / (nElapsed / 1000);

        double dRate = raCount.load() / (nElapsed / 1000.0);
        debug::log(0, "[DONE] ", VARIABLE(listTest.size()), " | ",
                                 VARIABLE(listTest.count()), " | in ", nElapsed, " | ", std::fixed, dRate / 1000000.0, " MM/s");

        debug::warning("Completed ", raCount.load(), " ops");

        listTest.check_coherent();

        return 0;
    }


    {
        std::atomic<uint64_t> raCount(0);

        util::atomic::lock_shared_ptr<std::queue<uint32_t>> listTest =
            util::atomic::lock_shared_ptr<std::queue<uint32_t>>(new std::queue<uint32_t>());



        std::vector<std::thread> vThreads(nThreads);
        std::vector<runtime::stopwatch> vTimers(nThreads);
        for(uint32_t n = 0; n < vThreads.size(); ++n)
        {
            vTimers[n]  = runtime::stopwatch();
            vThreads[n] = std::thread(ListThread2, std::ref(listTest), std::ref(vTimers[n]), std::ref(raCount));
        }


        for(uint32_t n = 0; n < vThreads.size(); ++n)
            vThreads[n].join();

        uint64_t nElapsed = 0;
        for(uint32_t n = 0; n < vTimers.size(); ++n)
            nElapsed += vTimers[n].ElapsedMilliseconds();

        nElapsed /= vThreads.size();
        //nElapsed = raCount.load() / (nElapsed / 1000);

        double dRate = raCount.load() / (nElapsed / 1000.0);
        debug::warning("Completed ", raCount.load(), " ops");
        debug::log(0, "[DONE] ", VARIABLE(listTest->size()), " | in ", nElapsed, " | ", std::fixed, dRate / 1000000.0, " MM/s");
        //listTest.print();
    }



    return 0;


    //std::atomic<std::string> ptr;


    util::atomic::lock_shared_ptr<Test> ptrTest = util::atomic::lock_shared_ptr<Test>(new Test());
    {
        util::atomic::lock_shared_ptr<Test> ptrTest1 = ptrTest;
        util::atomic::lock_shared_ptr<Test> ptrTest2 = ptrTest1;


        ptrTest->a = 55;
        ptrTest2->b = 88;
    }

    {

            std::thread t1(TestThread, std::ref(ptrTest));
            std::thread t2(TestThread, std::ref(ptrTest));
            std::thread t3(TestThread, std::ref(ptrTest));

            t1.join();
            t2.join();
            t3.join();
    }


    debug::log(0, VARIABLE(ptrTest->a), " | ", VARIABLE(ptrTest->b), " | ", VARIABLE(ptrTest->c));

    return 0;

    //atomic<Test> ptr;
    //ptr.store(Test());

    //Test n;
    //n.a = 55;

    //ptr.store(n);

    //s.store(new Test());

    {
        std::atomic<Test> term;

        util::atomic::shared_ptr<Test> ptr(new Test());

        //ptr->a = 5;

        //debug::log(0, VARIABLE(ptr->a), " | ", VARIABLE(ptr->b));

        std::thread t1(PushThread, std::ref(ptr));
        std::thread t2(PushThread, std::ref(ptr));
        std::thread t3(PushThread, std::ref(ptr));
        std::thread t4(PushThread, std::ref(ptr));
        //std::thread t5(PushThread, std::ref(ptr));

        debug::log(0, "Generated threads...");

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        //t5.join();

        debug::log(0, VARIABLE(ptr.load()->a), " | ", VARIABLE(ptr.load()->b), " | ", VARIABLE(ptr.load()->c));

        return 0;
    }


}



#include <atomic/include/typedef.h>

util::atomic::uint32_t nStreamReads;

void BatchRead(mstream &stream, runtime::stopwatch &timer)
{
    for(int n = 0; n < config::GetArg("-total", 0); ++n)
    {
        try
        {
            uint64_t nIndex = LLC::GetRandInt(config::GetArg("-total", 0) - 2);
            std::vector<uint8_t> vPage(filesystem::page_size() + 55, 0);

            timer.start();
            stream.read((char*)&vPage[0], vPage.size(), filesystem::page_size() * nIndex);
            timer.stop();

            stream.write((char*)&vPage[0], vPage.size(), filesystem::page_size() * nIndex);

            ++nStreamReads;
        }
        catch(const std::exception& e)
        {
            debug::warning(FUNCTION, e.what());
        }
    }

    return;
}


int main3(int argc, char** argv)
{
    config::ParseParameters(argc, argv);

    LLP::Initialize();

    const std::string strFile = "/database/test.dat";
    if(!filesystem::exists(strFile) || config::GetBoolArg("-restart"))
    {
        std::vector<uint8_t> vData(filesystem::page_size() * config::GetArg("-total", 0), 0);

        std::ofstream out(strFile, std::ios::in | std::ios::out | std::ios::trunc);
        out.write((char*)&vData[0], vData.size());
        out.close();
    }

    mstream stream(strFile, std::ios::in | std::ios::out, 1);

    std::vector<uint8_t> vData2((filesystem::page_size() * 2), 0xaa);
    stream.write((char*)&vData2[0], vData2.size(), filesystem::page_size() * 333);


    std::vector<uint8_t> vData((filesystem::page_size() * 2), 0x00);
    stream.read((char*)&vData[0], vData.size(), filesystem::page_size() * 333);

    PrintHex(vData.begin(), vData.end());

    return 0;
}







int main4(int argc, char** argv)
{
    config::ParseParameters(argc, argv);

    LLP::Initialize();

    const std::string strFile = "/database/test.dat";
    if(!filesystem::exists(strFile) || config::GetBoolArg("-restart"))
    {
        std::vector<uint8_t> vData(filesystem::page_size() * config::GetArg("-total", 0), 0);

        std::ofstream out(strFile, std::ios::in | std::ios::out | std::ios::trunc);
        out.write((char*)&vData[0], vData.size());
        out.close();
    }

    mstream stream(strFile, std::ios::in | std::ios::out, 1);


    std::vector<runtime::stopwatch> vTimers;
    for(int i = 0; i < config::GetArg("-threads", 4); ++i)
    {
        vTimers.push_back(runtime::stopwatch());
    }

    std::vector<std::thread> vThreads;

    for(int i = 0; i < config::GetArg("-threads", 4); ++i)
    {
        vThreads.push_back(std::thread(BatchRead, std::ref(stream), std::ref(vTimers[i])));
    }

    for(int i = 0; i < vThreads.size(); ++i)
        vThreads[i].join();

    uint64_t nElapsed = 0;
    for(int i = 0; i < vTimers.size(); ++i)
        nElapsed += vTimers[i].ElapsedMicroseconds();

    nElapsed /= vTimers.size();

    debug::log(0, "Reading from mmap at ", std::fixed, 1000000.0 * nStreamReads.load() / nElapsed, " reads/s");


    return 0;
}


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main2(int argc, char** argv)
{
    rlimit rlim;

    int nRet = getrlimit(RLIMIT_AS, &rlim);
    debug::log(0, VARIABLE(nRet), " | ", VARIABLE(rlim.rlim_cur), " | ", VARIABLE(rlim.rlim_max));


    nRet = getrlimit(RLIMIT_MEMLOCK, &rlim);
    debug::log(0, VARIABLE(nRet), " | ", VARIABLE(rlim.rlim_cur), " | ", VARIABLE(rlim.rlim_max));

    config::ParseParameters(argc, argv);

    LLP::Initialize();

    //config::nVerbose.store(4);
    config::mapArgs["-datadir"] = "/database";

    const std::string strDB = "LLD";

    std::string strIndex = config::mapArgs["-datadir"] + "/" + strDB + "/";

    if(config::GetBoolArg("-reset", false))
    {
        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "Deleting data directory ", strIndex, ANSI_COLOR_RESET);
        filesystem::remove_directories(strIndex);
    }

    std::vector<uint1024_t> vFirst;


    bool fNew = false;

    std::string strKeys = config::mapArgs["-datadir"] + "/keys";
    if(!filesystem::exists(strKeys))
    {
        /* Create 10k new keys. */
        std::vector<uint1024_t> vKeys;
        for(int i = 0; i < 10000; ++i)
            vKeys.push_back(LLC::GetRand1024());

        /* Create a new file for next writes. */
        std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::trunc);
        stream.write((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
        stream.close();

        debug::log(0, "Created new keys file.");
        vFirst = vKeys;

        fNew = true;
    }
    else
    {
        /* Create 10k new keys. */
        std::vector<uint1024_t> vKeys(10000, 0);

        /* Create a new file for next writes. */
        //std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::in);
        //stream.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
        //stream.close();

        runtime::stopwatch swMap;
        if(config::GetBoolArg("-fstream"))
        {
            swMap.start();
            std::fstream stream(strKeys, std::ios::out | std::ios::binary | std::ios::in);
            stream.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0]));
            stream.close();
            swMap.stop();

            debug::log(0, "fstream completed in ", swMap.ElapsedMicroseconds());
        }
        else
        {
            swMap.start();

            //file_t hFile = open_file(strKeys, )
            memory::mstream stream2(strKeys, std::ios::in | std::ios::out);
            if(!stream2.read((char*)&vKeys[0], vKeys.size() * sizeof(vKeys[0])))
                return debug::error("Failed to read data for mmap");

            swMap.stop();

            stream2.seekp((vKeys.size() - 1) * sizeof(vKeys[0]));
            if(!stream2.write((char*)&vKeys[2], sizeof(vKeys[2])))
                return debug::error("Failed to write a new mapping");

            stream2.flush();

            stream2.seekg(0, std::ios::beg);

            std::vector<uint8_t> vData(vKeys.size() * sizeof(vKeys[0]), 0);
            if(!stream2.read((char*)&vData[0], vData.size()))
                return debug::error("Failed to read data for mmap");

            stream2.close();

            debug::log(0, "MMap completed in ", swMap.ElapsedMicroseconds());
        }

        vFirst = vKeys;

        debug::log(0, "[A] ", vFirst.begin()->SubString());
        debug::log(0, "[B] ", vFirst.back().SubString());
    }


    #if 0
    leveldb_t *db;
    leveldb_options_t *options;
    leveldb_readoptions_t *roptions;
    leveldb_writeoptions_t *woptions;
    char *err = NULL;

    /******************************************/
    /* OPEN */

    options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);

    db = leveldb_open(options, "/database/leveldb", &err);

    if (err != NULL) {
        return debug::error("Failed to open LEVELDB instance");
    }

    /* reset error var */
    leveldb_free(err); err = NULL;
    #endif



    //build our base configuration
    LLD::Config::Base BASE =
        LLD::Config::Base(strDB, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);

    //build our sector configuration
    LLD::Config::Static SECTOR      = LLD::Config::Static(BASE);
    SECTOR.MAX_SECTOR_FILE_STREAMS  = 16;
    SECTOR.MAX_SECTOR_BUFFER_SIZE   = 1024 * 1024 * 4; //4 MB write buffer
    SECTOR.MAX_SECTOR_CACHE_SIZE    = 256; //4 MB of cache available

    //build our hashmap configuration
    LLD::Config::Hashmap CONFIG     = LLD::Config::Hashmap(BASE);
    CONFIG.HASHMAP_TOTAL_BUCKETS    = 20000000;
    CONFIG.MAX_FILES_PER_HASHMAP    = 4;
    CONFIG.MAX_FILES_PER_INDEX      = 10;
    CONFIG.MAX_HASHMAPS             = 50;
    CONFIG.MIN_LINEAR_PROBES        = 1;
    CONFIG.MAX_LINEAR_PROBES        = 1024;
    CONFIG.MAX_HASHMAP_FILE_STREAMS = 100;
    CONFIG.MAX_INDEX_FILE_STREAMS   = 10;
    CONFIG.PRIMARY_BLOOM_HASHES     = 7;
    CONFIG.PRIMARY_BLOOM_ACCURACY   = 300;
    CONFIG.SECONDARY_BLOOM_BITS     = 13;
    CONFIG.SECONDARY_BLOOM_HASHES   = 7;
    CONFIG.QUICK_INIT               = !config::GetBoolArg("-audit", false);

    bloom = new TestDB(SECTOR, CONFIG);


    runtime::stopwatch swElapsed, swReaders;
    {
        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "[LLD] BASE LEVEL TEST:", ANSI_COLOR_RESET);

        runtime::stopwatch swTimer;
        swTimer.start();
        swElapsed.start();

        uint32_t nTotal = 0;
        if(fNew)
        {
            for(const auto& nBucket : vFirst)
            {
                ++nTotalWritten;

                ++nTotal;
                if(!bloom->WriteKey(nBucket, nBucket))
                    return debug::error(FUNCTION, "failed to write ", VARIABLE(nTotal));
            }


            swElapsed.stop();
            swTimer.stop();

            uint64_t nElapsed = swTimer.ElapsedMicroseconds();
            debug::log(0,  "[LLD] ", vFirst.size() / 1000, "k records written in ", nElapsed,
                ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " writes/s)", ANSI_COLOR_RESET);

        }


        uint1024_t hashKey = 0;

        swTimer.reset();
        swTimer.start();

        //swReaders.start();

        nTotal = 0;
        for(const auto& nBucket : vFirst)
        {
            ++nTotal;
            //++nTotalRead;

            if(!bloom->ReadKey(nBucket, hashKey))
                return debug::error("Failed to read ", nBucket.SubString(), " total ", nTotal);
        }
        swTimer.stop();
        //swReaders.stop();
        nTotal = 0;

        uint64_t nElapsed = swTimer.ElapsedMicroseconds();
        debug::log(0, "[LLD] ", vFirst.size() / 1000, "k records read in ", nElapsed,
            ANSI_COLOR_BRIGHT_YELLOW, " (", std::fixed, (1000000.0 * vFirst.size()) / nElapsed, " read/s)", ANSI_COLOR_RESET);
    }



    runtime::stopwatch t1Elap;
    runtime::stopwatch t1Read;
    std::thread t1(BatchWrite, std::ref(t1Elap), std::ref(t1Read));

    runtime::stopwatch t2Elap;
    runtime::stopwatch t2Read;
    std::thread t2(BatchWrite, std::ref(t2Elap), std::ref(t2Read));

    runtime::stopwatch t3Elap;
    runtime::stopwatch t3Read;
    std::thread t3(BatchWrite, std::ref(t3Elap), std::ref(t3Read));


    runtime::stopwatch t4Elap;
    runtime::stopwatch t4Read;
    std::thread t4(BatchWrite, std::ref(t4Elap), std::ref(t4Read));


    runtime::stopwatch t5Elap;
    runtime::stopwatch t5Read;
    std::thread t5(BatchWrite, std::ref(t5Elap), std::ref(t5Read));


    runtime::stopwatch t6Elap;
    runtime::stopwatch t6Read;
    std::thread t6(BatchWrite, std::ref(t6Elap), std::ref(t6Read));


    runtime::stopwatch t7Elap;
    runtime::stopwatch t7Read;
    std::thread t7(BatchWrite, std::ref(t7Elap), std::ref(t7Read));


    runtime::stopwatch t8Elap;
    runtime::stopwatch t8Read;
    std::thread t8(BatchWrite, std::ref(t8Elap), std::ref(t8Read));


    runtime::stopwatch t9Elap;
    runtime::stopwatch t9Read;
    std::thread t9(BatchWrite, std::ref(t9Elap), std::ref(t9Read));


    debug::log(0, "Waiting for threads");
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();

    {
        /******************************************/
        /* CLOSE */

        runtime::stopwatch swClose;
        swClose.start();
        //swElapsed.start();
        delete bloom;
        swClose.stop();
        //swElapsed.stop();

        debug::log(0, "[LLD] Closed in ", swClose.ElapsedMilliseconds(), " ms");
    }


    {
        uint64_t nElapsed =
            (t1Elap.ElapsedMicroseconds() +
            t2Elap.ElapsedMicroseconds() +
            t3Elap.ElapsedMicroseconds() +
            t4Elap.ElapsedMicroseconds() +
            t5Elap.ElapsedMicroseconds() +
            t6Elap.ElapsedMicroseconds() +
            t7Elap.ElapsedMicroseconds() +
            t8Elap.ElapsedMicroseconds() +
            t9Elap.ElapsedMicroseconds()) / 9;

        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "[LLD] Completed ", nTotalWritten, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalWritten) / nElapsed, " writes/s)");
    }

    {
        uint64_t nElapsed =
            (t1Read.ElapsedMicroseconds() +
            t2Read.ElapsedMicroseconds() +
            t3Read.ElapsedMicroseconds() +
            t4Read.ElapsedMicroseconds() +
            t5Read.ElapsedMicroseconds() +
            t6Read.ElapsedMicroseconds() +
            t7Read.ElapsedMicroseconds() +
            t8Read.ElapsedMicroseconds() +
            t9Read.ElapsedMicroseconds() ) / 9;

        uint64_t nMinutes = nElapsed / 60000000;
        uint64_t nSeconds = (nElapsed / 1000000) - (nMinutes * 60);

        debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "[LLD] Completed ", nTotalRead, " Keys in ",
            nMinutes, " minutes ", nSeconds, " seconds (", std::fixed, (1000000.0 * nTotalRead) / nElapsed, " reads/s)");
    }



    return 0;
}
