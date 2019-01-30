/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_SECTOR_H
#define NEXUS_LLD_TEMPLATES_SECTOR_H

#include <functional>
#include <atomic>

#include <LLD/cache/template_lru.h>
#include <LLD/include/enum.h>
#include <LLD/templates/key.h>
#include <LLD/templates/transaction.h>

#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <condition_variable>

namespace LLD
{


    /* Maximum size a file can be in the keychain. */
    const uint32_t MAX_SECTOR_FILE_SIZE = 1024 * 1024 * 512; //512 MB per File


    /* Maximum cache buckets for sectors. */
    const uint32_t MAX_SECTOR_CACHE_SIZE = 1024 * 1024 * 4; //256 MB Max Cache


    /* The maximum amount of bytes allowed in the memory buffer for disk flushes. **/
    const uint32_t MAX_SECTOR_BUFFER_SIZE = 1024 * 1024 * 4; //32 MB Max Disk Buffer


    /** SectorDatabase
     *
     *  Base Template Class for a Sector Database.
     *  Processes main Lower Level Disk Communications.
     *  A Sector Database Is a Fixed Width Data Storage Medium.
     *
     *  It is ideal for data structures to be stored that do not
     *  change in their size. This allows the greatest efficiency
     *  in fixed data storage (structs, class, etc.).
     *
     *  It is not ideal for data structures that may vary in size
     *  over their lifetimes. The Dynamic Database will allow that.
     *
     *  Key Type can be of any type. Data lengths are attributed to
     *  each key type. Keys are assigned sectors and stored in the
     *  key storage file. Sector files are broken into maximum of 1 GB
     *  for stability on all systems, key files are determined the same.
     *
     *  Multiple Keys can point back to the same sector to allow multiple
     *  access levels of the sector. This specific class handles the lower
     *  level disk communications for the sector database.
     *
     *  If each sector was allowed to be varying sizes it would remove the
     *  ability to use free space that becomes available upon an erase of a
     *  record. Use this Database purely for fixed size structures. Overflow
     *  attempts will trigger an error code.
     *
     **/
    template<typename KeychainType, typename CacheType>
    class SectorDatabase
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;

    protected:
        /* Mutex for Thread Synchronization.
            TODO: Lock Mutex based on Read / Writes on a per Sector Basis.
            Will allow higher efficiency for thread concurrency. */
        std::mutex SECTOR_MUTEX;
        std::mutex BUFFER_MUTEX;
        std::mutex TRANSACTION_MUTEX;


        /* The String to hold the Disk Location of Database File. */
        std::string strBaseLocation;
        std::string strName;


        /* timer for Runtime Calculations. */
        runtime::timer runtime;


        /* Class to handle Transaction Data. */
        SectorTransaction* pTransaction;


        /* Sector Keys Database. */
        KeychainType* pSectorKeys;


        /* Cache Pool */
        CacheType* cachePool;


        /* File stream object. */
        mutable TemplateLRU<uint32_t, std::fstream*>* fileCache;


        /* The current File Position. */
        mutable uint32_t nCurrentFile;
        mutable uint32_t nCurrentFileSize;


        /* Cache Writer Thread. */
        std::thread CacheWriterThread;


        /* The meter thread. */
        std::thread MeterThread;


        /* Disk Buffer Vector. */
        std::vector< std::pair< std::vector<uint8_t>, std::vector<uint8_t> > > vDiskBuffer;


        /* Disk Buffer Memory Size. */
        std::atomic<uint32_t> nBufferBytes;

        /* For the Meter. */
        std::atomic<uint32_t> nBytesRead;
        std::atomic<uint32_t> nBytesWrote;
        std::atomic<uint32_t> nRecordsFlushed;

        /* Destructor Flag. */
        std::atomic<bool> fDestruct;


        /* Initialize Flag. */
        std::atomic<bool> fInitialized;


        /** Database Flags. **/
        uint8_t nFlags;


    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        SectorDatabase(std::string strNameIn, uint8_t nFlagsIn)
        : CONDITION_MUTEX()
        , CONDITION()
        , SECTOR_MUTEX()
        , BUFFER_MUTEX()
        , TRANSACTION_MUTEX()
        , strBaseLocation(config::GetDataDir() + strNameIn + "/datachain/")
        , strName(strNameIn)
        , runtime()
        , pTransaction(nullptr)
        , pSectorKeys(nullptr)
        , cachePool(new CacheType((config::GetArg("-maxcache", 64) * 1024 * 1024) / 4))
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , nCurrentFile(0)
        , nCurrentFileSize(0)
        , CacheWriterThread()
        , MeterThread()
        , vDiskBuffer()
        , nBufferBytes(0)
        , nBytesRead(0)
        , nBytesWrote(0)
        , nRecordsFlushed(0)
        , fDestruct(false)
        , fInitialized(false)
        , nFlags(nFlagsIn)
        {
            /* Set readonly flag if write or append are not specified. */
            if(!(nFlags & FLAGS::FORCE) && !(nFlags & FLAGS::WRITE) && !(nFlags & FLAGS::APPEND))
                nFlags |= FLAGS::READONLY;

            /* Initialize the Database. */
            Initialize();

            if(config::GetBoolArg("-runtime", false))
            {
                debug::log(0, ANSI_COLOR_GREEN FUNCTION, "executed in ",
                    runtime.ElapsedMicroseconds(), " micro-seconds" ANSI_COLOR_RESET);
            }

            CacheWriterThread = std::thread(std::bind(&SectorDatabase::CacheWriter, this));
            MeterThread = std::thread(std::bind(&SectorDatabase::Meter, this));
        }


        /** Default Destructor **/
        ~SectorDatabase()
        {
            fDestruct = true;
            CONDITION.notify_all();

            CacheWriterThread.join();
            MeterThread.join();

            if(pTransaction)
                delete pTransaction;

            delete cachePool;
            delete fileCache;

            if(pSectorKeys)
                delete pSectorKeys;
        }


        /** Initialize
         *
         *  Initialize Sector Database.
         *
         **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(nFlags & FLAGS::CREATE && !filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
                debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

            /* Find the most recent append file. */
            while(true)
            {

                /* TODO: Make a worker or thread to check sizes of files and automatically create new file.
                    Keep independent of reads and writes for efficiency. */
                std::fstream fIncoming(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::in | std::ios::binary);
                if(!fIncoming)
                {

                    /* Assign the Current Size and File. */
                    if(nCurrentFile > 0)
                        --nCurrentFile;
                    else
                    {
                        /* Create a new file if it doesn't exist. */
                        std::ofstream cStream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
                        cStream.close();
                    }

                    break;
                }

                /* Get the Binary Size. */
                fIncoming.seekg(0, std::ios::end);
                nCurrentFileSize = fIncoming.tellg();
                fIncoming.close();

                /* Increment the Current File */
                ++nCurrentFile;
            }

            /* Initialize the Keys Class. */
            pSectorKeys = new KeychainType((config::GetDataDir() + strName + "/keychain/"), nFlags);

            pTransaction = nullptr;
            fInitialized = true;
        }


        /** GetKeys
         *
         *  Get the keys for this sector database from the keychain.
         *
         **/
        std::vector< std::vector<uint8_t> > GetKeys()
        {
            return pSectorKeys->GetKeys();
        }


        /** Exists
         *
         *  Determine if the entry identified by the given key exists.
         *
         *  @param[in] key The key to the database entry.
         *
         *  @return True if the entry exists, false otherwise.
         *
         **/
        template<typename Key>
        bool Exists(const Key& key)
        {
            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Check that the key is not pending in a transaction for Erase. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {
                    if(pTransaction->mapEraseData.count(ssKey.Bytes()))
                        return false;

                    /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
                    if(pTransaction->mapTransactions.count(ssKey.Bytes()))
                        return true;

                    /* Check for keychain commits. */
                    if(pTransaction->mapKeychain.count(ssKey.Bytes()))
                        return true;
                }
            }

            /* Check the cache pool. */
            if(cachePool->Has(ssKey.Bytes()))
                return true;

            /* Return the Key existance in the Keychain Database. */
            SectorKey cKey;
            return pSectorKeys->Get(ssKey.Bytes(), cKey);
        }


        /** Erase
         *
         *  Erase a database entry identified by the given key.
         *
         *  @param[in] key The key to the database entry to erase.
         *
         *  @return True if the entry erased, false otherwise.
         *
         **/
        template<typename Key>
        bool Erase(const Key& key)
        {
            if(nFlags & FLAGS::READONLY)
                assert(!"Erase called on database in read-only mode");

            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Remove the item from the cache pool. */
            cachePool->Remove(ssKey.Bytes());

            /* Add transaction to erase queue. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {

                    pTransaction->EraseTransaction(ssKey.Bytes());

                    return true;
                }
            }

            /* Return the Key existance in the Keychain Database. */
            bool fErased = pSectorKeys->Erase(ssKey.Bytes());

            if(config::GetBoolArg("-runtime", false))
            {
                debug::log(0, ANSI_COLOR_GREEN FUNCTION, "executed in ",
                    runtime.ElapsedMicroseconds(), " micro-seconds" ANSI_COLOR_RESET);
            }

            return fErased;
        }


        /** Read
         *
         *  Read a database entry identified by the given key.
         *
         *  @param[in] key The key to the database entry to read.
         *  @param[out] value The database entry value to read out.
         *
         *  @return True if the entry read, false otherwise.
         *
         **/
        template<typename Key, typename Type>
        bool Read(const Key& key, Type& value)
        {
            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Get the Data from Sector Database. */
            std::vector<uint8_t> vData;

            /* Check that the key is not pending in a transaction for Erase. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {

                    if(pTransaction->mapEraseData.count(ssKey.Bytes()))
                        return false;

                    /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
                    if(pTransaction->mapTransactions.count(ssKey.Bytes()))
                        vData = pTransaction->mapTransactions[ssKey.Bytes()];
                }
            }

            if(vData.empty() && !Get(ssKey.Bytes(), vData))
                return false;

            /* Deserialize Value. */
            DataStream ssValue(vData, SER_LLD, DATABASE_VERSION);
            ssValue >> value;

            return true;
        }


        /** Index
         *
         *  Indexes a key into memory.
         *
         *  @param[in] key The key to index.
         *  @param[in] index The index of the key to write to.
         *
         *  @return True if successful indexing, false otherwise.
         *
         **/
        template<typename Key, typename Type>
        bool Index(const Key& key, const Type& index)
        {
            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Serialize the index into bytes. */
            DataStream ssIndex(SER_LLD, DATABASE_VERSION);
            ssIndex << index;

            /* Check that the key is not pending in a transaction for Erase. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {

                    /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
                    pTransaction->mapIndex[ssKey.Bytes()] = ssIndex.Bytes();

                    return true;
                }
            }

            /* Get the key. */
            SectorKey cKey;
            if(!pSectorKeys->Get(ssIndex.Bytes(), cKey))
                return false;

            /* Write the new sector key. */
            cKey.SetKey(ssKey.Bytes());
            return pSectorKeys->Put(cKey);
        }


        /** Write
         *
         *  Writes a key to the sector database
         *
         *  @param[in] key The key to write.
         *
         *  @return True if successful write, false otherwise.
         *
         **/
        template<typename Key>
        bool Write(const Key& key)
        {
            if(nFlags & FLAGS::READONLY)
                assert(!"Write called on database in read-only mode");

            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Check for transaction. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {
                    /* Check if data is in erase queue, if so remove it. */
                    if(pTransaction->mapEraseData.count(ssKey.Bytes()))
                        pTransaction->mapEraseData.erase(ssKey.Bytes());

                    /* Set the transaction data. */
                    pTransaction->mapKeychain[ssKey.Bytes()] = 0;

                    return true;
                }
            }

            /* Return the Key existance in the Keychain Database. */
            SectorKey cKey(STATE::READY, ssKey.Bytes(), 0, 0, 0);
            return pSectorKeys->Put(cKey);
        }


        /** Write
         *
         *  Writes a key/value pair to the sector database
         *
         *  @param[in] key The key to write.
         *  @param[in] value The value to write.
         *
         *  @return True if successful write, false otherwise.
         *
         **/
        template<typename Key, typename Type>
        bool Write(const Key& key, const Type& value)
        {
            if(nFlags & FLAGS::READONLY)
                assert(!"Write called on database in read-only mode");

            /* Serialize the Key. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Serialize the Value */
            DataStream ssData(SER_LLD, DATABASE_VERSION);
            ssData << value;

            /* Check for transaction. */
            { LOCK(TRANSACTION_MUTEX);
                if(pTransaction)
                {
                    /* Check if data is in erase queue, if so remove it. */
                    if(pTransaction->mapEraseData.count(ssKey.Bytes()))
                        pTransaction->mapEraseData.erase(ssKey.Bytes());

                    /* Set the transaction data. */
                    pTransaction->mapTransactions[ssKey.Bytes()] = ssData.Bytes();

                    return true;
                }
            }

            return Put(ssKey.Bytes(), ssData.Bytes());
        }


        /** Get
         *
         *  Get a record from cache or from disk
         *
         *  @param[in] vKey The binary data of the key to get.
         *  @param[out] vData The binary data of the record to get.
         *
         *  @return True if the record was read successfully.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
        {
            /* Iterate if meters are enabled. */
            nBytesRead += (vKey.size() + vData.size());

            /* Check the cache pool for key first. */
            if(cachePool->Get(vKey, vData))
                return true;

            /* Get the key from the keychain. */
            SectorKey cKey;
            if(pSectorKeys->Get(vKey, cKey))
            {
                { LOCK(SECTOR_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(cKey.nSectorFile, pstream))
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return debug::error(FUNCTION, "couldn't create stream file");
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(cKey.nSectorFile, pstream);
                    }

                    /* Seek to the Sector Position on Disk. */
                    pstream->seekg(cKey.nSectorStart, std::ios::beg);
                    vData.resize(cKey.nSectorSize);

                    /* Read the State and Size of Sector Header. */
                    if(!pstream->read((char*) &vData[0], vData.size()))
                        debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");

                }

                /* Add to cache */
                cachePool->Put(vKey, vData);

                /* Verbose Debug Logging. */
                debug::log(5, FUNCTION, "Current File: ", cKey.nSectorFile,
                    " | Current File Size: ", cKey.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));

                return true;
            }

            return false;
        }


        /** Get
         *
         *  Get a record from from disk if the sector key
         *  is already read from the keychain.
         *
         *  @param[in] cKey The sector key from keychain.
         *  @param[in] vData The binary data of the record to get.WRITE
         *
         *  @return True if the record was read successfully.
         *
         **/
        bool Get(const SectorKey& cKey, std::vector<uint8_t>& vData)
        {
            { LOCK(SECTOR_MUTEX);

                nBytesRead += (cKey.vKey.size() + vData.size());

                /* Find the file stream for LRU cache. */
                std::fstream *pstream;
                if(!fileCache->Get(cKey.nSectorFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return false;
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(cKey.nSectorFile, pstream);
                }

                /* Seek to the Sector Position on Disk. */
                pstream->seekg(cKey.nSectorStart, std::ios::beg);
                vData.resize(cKey.nSectorSize);

                /* Read the State and Size of Sector Header. */
                if(!pstream->read((char*) &vData[0], vData.size()))
                    debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");

                /* Verboe output. */
                debug::log(5, FUNCTION, "Current File: ", cKey.nSectorFile,
                    " | Current File Size: ", cKey.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
            }

            return true;
        }


        /** Update
         *
         *  Update a record on disk.
         *
         *  @param[in] vKey The binary data of the key to flush
         *  @param[in] vData The binary data of the record to flush
         *
         *  @return True if the flush was successful.
         *
         **/
        bool Update(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
        {
            /* Check the keychain for key. */
            SectorKey key;
            if(!pSectorKeys->Get(vKey, key))
                return false;

            /* Check data size constraints. */
            if(vData.size() != key.nSectorSize)
                return debug::error(FUNCTION, "sector size ", key.nSectorSize, " mismatch ", vData.size());

            /* Write the data into the memory cache. */
            cachePool->Put(vKey, vData, false);

            { LOCK(SECTOR_MUTEX);

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(key.nSectorFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return false;
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(key.nSectorFile, pstream);
                }

                /* If it is a New Sector, Assign a Binary Position. */
                pstream->seekp(key.nSectorStart, std::ios::beg);
                pstream->write((char*) &vData[0], vData.size());
                pstream->flush();

                /* Records flushed indicator. */
                ++nRecordsFlushed;
                nBytesWrote += vData.size();

                /* Verboe output. */
                debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                    " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
            }

            return true;
        }


        /** Force
         *
         *  Force a write to disk immediately bypassing write buffers.
         *
         *  @param[in] vKey The binary data of the key to flush
         *  @param[in] vData The binary data of the record to flush
         *
         *  @return True if the flush was successful.
         *
         **/
        bool Force(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
        {
            if(nFlags & FLAGS::APPEND || !Update(vKey, vData))
            {
                /* Create new file if above current file size. */
                if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                {
                    debug::log(4, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                    ++ nCurrentFile;
                    nCurrentFileSize = 0;

                    std::ofstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                    stream.close();
                }

                /* Write the data into the memory cache. */
                cachePool->Put(vKey, vData, false);

                { LOCK(SECTOR_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(nCurrentFile, pstream))
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return false;
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(nCurrentFile, pstream);
                    }

                    /* If it is a New Sector, Assign a Binary Position. */
                    pstream->seekp(nCurrentFileSize, std::ios::beg);
                    pstream->write((char*) &vData[0], vData.size());
                    pstream->flush();

                    /* Create a new Sector Key. */
                    SectorKey key = SectorKey(STATE::READY, vKey, nCurrentFile, nCurrentFileSize, vData.size());

                    /* Increment the current filesize */
                    nCurrentFileSize += vData.size();

                    /* Assign the Key to Keychain. */
                    pSectorKeys->Put(key);

                    /* Verboe output. */
                    debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                        " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
                }
            }

            return true;
        }


        /** Put
         *
         *  Write a record into the cache and disk buffer for flushing to disk.
         *
         *  @param[in] vKey The binary data of the key to put.
         *  @param[in] vData The binary data of the record to put.
         *
         *  @return True if the record was written successfully.
         *
         **/
        bool Put(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
        {
            /* Write the data into the memory cache. */
            cachePool->Put(vKey, vData, !(nFlags & FLAGS::FORCE));

            /* Handle force write mode. */
            if(nFlags & FLAGS::FORCE)
                return Force(vKey, vData);

            /* Wait if the buffer is full. */
            if(nBufferBytes.load() >= MAX_SECTOR_BUFFER_SIZE)
            {
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || nBufferBytes.load() < MAX_SECTOR_BUFFER_SIZE; });
            }

            /* Add to the write buffer thread. */
            { LOCK(BUFFER_MUTEX);
                vDiskBuffer.push_back(std::make_pair(vKey, vData));
                nBufferBytes += (vKey.size() + vData.size());
            }

            /* Notify if buffer was added to. */
            CONDITION.notify_all();

            return true;
        }


        /** CacheWriter
         *
         *  Flushes periodically data from the cache buffer to disk.
         *
         **/
        void CacheWriter()
        {
            /* No cache write on force mode. */
            if(nFlags & FLAGS::FORCE)
            {
                debug::log(0, FUNCTION, strBaseLocation, " in FORCE mode... closing");
                return;
            }

            /* Wait for initialization. */
            while(!fInitialized)
                runtime::sleep(100);

            /* Check if writing is enabled. */
            if(!(nFlags & FLAGS::WRITE) && !(nFlags & FLAGS::APPEND))
                return;

            while(true)
            {
                /* Wait for buffer to empty before shutting down. */
                if((fDestruct.load()) && nBufferBytes.load() == 0)
                    return;

                /* Check for data to be written. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || nBufferBytes.load() > 0; });

                /* Swap the buffer object to get ready for writes. */
                std::vector< std::pair<std::vector<uint8_t>, std::vector<uint8_t>> > vIndexes;
                {
                    LOCK(BUFFER_MUTEX);

                    vIndexes.swap(vDiskBuffer);
                    nBufferBytes = 0;
                }

                /* Create a new file if the sector file size is over file size limits. */
                if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                {
                    debug::log(0, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                    /* Iterate the current file and reset current file sie. */
                    ++nCurrentFile;
                    nCurrentFileSize = 0;

                    /* Create a new file for next writes. */
                    std::fstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::out | std::ios::binary | std::ios::trunc);
                    stream.close();
                }

                /* Iterate through buffer to queue disk writes. */
                for(const auto& vObj : vIndexes)
                {
                    /* Force write data. */
                    Force(vObj.first, vObj.second);

                    /* Set no longer reserved in cache pool. */
                    cachePool->Reserve(vObj.first, false);

                    /* Iterate bytes written for meter. */
                    nBytesWrote += (vObj.first.size() + vObj.second.size());
                }

                /* Notify the condition. */
                CONDITION.notify_all();

                /* Verbose logging. */
                debug::log(3, FUNCTION, "Flushed ", nRecordsFlushed.load(),
                    " Records of ", nBytesWrote.load(), " Bytes");

                /* Reset counters if not in meter mode. */
                if(!config::GetBoolArg("-meters"))
                {
                    nBytesWrote     = 0;
                    nRecordsFlushed = 0;
                }
            }
        }


        /** Meter
         *
         *  LLD Meter Thread. Tracks the Reads/Writes per second.
         *
         **/
        void Meter()
        {
            if(!config::GetBoolArg("-meters", false))
                return;

            runtime::timer TIMER;
            TIMER.Start();

            while(!fDestruct.load())
            {
                nBytesWrote     = 0;
                nBytesRead      = 0;
                nRecordsFlushed = 0;

                runtime::sleep(10000);

                double WPS = nBytesWrote.load() / (TIMER.Elapsed() * 1024.0);
                double RPS = nBytesRead.load() / (TIMER.Elapsed() * 1024.0);

                debug::log(0, FUNCTION, ">>>>> LLD Writing at ", WPS, " Kb/s");
                debug::log(0, FUNCTION, ">>>>> LLD Reading at ", RPS, " Kb/s");
                debug::log(0, FUNCTION, ">>>>> LLD Flushed ", nRecordsFlushed.load(), " Records");

                TIMER.Reset();
            }
        }


        /** TxnBegin
         *
         *  Start a database transaction.
         *
         **/
        void TxnBegin()
        {
            LOCK(TRANSACTION_MUTEX);

            /* Delete a previous database transaction pointer if applicable. */
            if(pTransaction)
                delete pTransaction;

            /* Create the new Database Transaction Object. */
            pTransaction = new SectorTransaction();
        }


        /** TxnAbort
         *
         *  Abort a transaction from happening.
         *
         **/
        void TxnAbort()
        {
            LOCK(TRANSACTION_MUTEX);

            /** Delete the previous transaction pointer if applicable. **/
            if(pTransaction)
                delete pTransaction;

            /** Set the transaction pointer to null also acting like a flag **/
            pTransaction = nullptr;
        }


        /** RollbackTransactions
         *
         *  Rollback the disk to previous state.
         *
         *  @return True if rollback was successful, false otherwise.
         *
         **/
        bool RollbackTransactions()
        {
            /* Commit the sector data. */
            for(auto it = pTransaction->mapOriginalData.begin(); it != pTransaction->mapOriginalData.end(); ++it )
                if(!Force(it->first, it->second))
                    return debug::error(FUNCTION, "failed to rollback transaction");

            return true;
        }


        /** TxnCommit
         *
         *  Commit data from transaction object.
         *
         *  @return True, if commit is successful, false otherwise.
         *
         **/
        bool TxnCommit()
        {
            LOCK(TRANSACTION_MUTEX);

            /* Check that there is a valid transaction to apply to the database. */
            if(!pTransaction)
                return debug::error(FUNCTION, "nothing to commit.");

            //std::ofstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);

            /* Erase data set to be removed. */
            for(auto it = pTransaction->mapEraseData.begin(); it != pTransaction->mapEraseData.end(); ++it )
            {
                /* Erase the transaction data. */
                if(!pSectorKeys->Erase(it->first))
                {
                    delete pTransaction;
                    pTransaction = nullptr;
                    //RollbackTransactions();
                    //TxnAbort();

                    return debug::error(FUNCTION, "failed to erase from keychain");
                }
            }

            /* Commit keychain entries. */
            for(auto it = pTransaction->mapKeychain.begin(); it != pTransaction->mapKeychain.end(); ++it )
            {
                SectorKey cKey(STATE::READY, it->first, 0, 0, 0);
                if(!pSectorKeys->Put(cKey))
                {
                    delete pTransaction;
                    pTransaction = nullptr;
                    //RollbackTransactions();
                    //TxnAbort();
                }
            }

            /* Commit the sector data. */
            for(auto it = pTransaction->mapTransactions.begin(); it != pTransaction->mapTransactions.end(); ++it )
            {
                if(!Force(it->first, it->second))
                {
                    delete pTransaction;
                    pTransaction = nullptr;
                    //RollbackTransactions();
                    //TxnAbort();

                    return debug::error(FUNCTION, "failed to commit sector data");
                }
            }

            /* Commit the index data. */
            std::map<std::vector<uint8_t>, SectorKey> mapIndex;
            for(auto it = pTransaction->mapIndex.begin(); it != pTransaction->mapIndex.end(); ++it )
            {
                /* Get the key. */
                SectorKey cKey;
                if(mapIndex.count(it->second))
                    cKey = mapIndex[it->second];
                else
                {
                    if(!pSectorKeys->Get(it->second, cKey))
                    {
                        delete pTransaction;
                        pTransaction = nullptr;
                        //TxnAbort();

                        return debug::error(FUNCTION, "failed to read indexing entry");
                    }

                    mapIndex[it->second] = cKey;
                }

                /* Write the new sector key. */
                cKey.SetKey(it->first);
                if(!pSectorKeys->Put(cKey))
                {
                    delete pTransaction;
                    pTransaction = nullptr;
                    //TxnAbort();

                    return debug::error(FUNCTION, "failed to write indexing entry");
                }
            }

            /* Cleanup the transaction object. */
            delete pTransaction;
            pTransaction = nullptr;

            return true;
        }
    };
}

#endif
