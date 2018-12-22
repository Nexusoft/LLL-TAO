/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_SECTOR_H
#define NEXUS_LLD_TEMPLATES_SECTOR_H

#include <functional>
#include <atomic>

#include <LLD/templates/key.h>
#include <LLD/templates/transaction.h>

#include <LLD/cache/template_lru.h>

#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <condition_variable>

namespace LLD
{

    /* Maximum size a file can be in the keychain. */
    const uint32_t MAX_SECTOR_FILE_SIZE = 1024 * 1024 * 128; //128 MB per File


    /* Maximum cache buckets for sectors. */
    const uint32_t MAX_SECTOR_CACHE_SIZE = 1024 * 1024 * 64; //512 MB Max Cache


    /* The maximum amount of bytes allowed in the memory buffer for disk flushes. **/
    const uint32_t MAX_SECTOR_BUFFER_SIZE = 1024 * 1024 * 64; //512 MB Max Disk Buffer


    /** Base Template Class for a Sector Database.
        Processes main Lower Level Disk Communications.
        A Sector Database Is a Fixed Width Data Storage Medium.

        It is ideal for data structures to be stored that do not
        change in their size. This allows the greatest efficiency
        in fixed data storage (structs, class, etc.).

        It is not ideal for data structures that may vary in size
        over their lifetimes. The Dynamic Database will allow that.

        Key Type can be of any type. Data lengths are attributed to
        each key type. Keys are assigned sectors and stored in the
        key storage file. Sector files are broken into maximum of 1 GB
        for stability on all systems, key files are determined the same.

        Multiple Keys can point back to the same sector to allow multiple
        access levels of the sector. This specific class handles the lower
        level disk communications for the sector database.

        If each sector was allowed to be varying sizes it would remove the
        ability to use free space that becomes available upon an erase of a
        record. Use this Database purely for fixed size structures. Overflow
        attempts will trigger an error code.
    **/
    template<typename KeychainType, typename CacheType>
    class SectorDatabase
    {
    protected:
        /* Mutex for Thread Synchronization.
            TODO: Lock Mutex based on Read / Writes on a per Sector Basis.
            Will allow higher efficiency for thread concurrency. */
        std::recursive_mutex SECTOR_MUTEX;
        std::recursive_mutex BUFFER_MUTEX;


        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;


        /* The String to hold the Disk Location of Database File. */
        std::string strBaseLocation, strName;


        /* Read only Flag for Sectors. */
        bool fReadOnly = false;


        /* Destructor Flag. */
        bool fDestruct = false;


        /* Initialize Flag. */
        bool fInitialized = false;


        /* timer for Runtime Calculations. */
        runtime::timer runtime;


        /* Class to handle Transaction Data. */
        SectorTransaction* pTransaction;


        /* Sector Keys Database. */
        KeychainType* pSectorKeys;


        /* For the Meter. */
        uint32_t nBytesRead;
        uint32_t nBytesWrote;
        uint32_t nRecordsFlushed;


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

    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        SectorDatabase(std::string strNameIn, const char* pszMode="r+")
        : strName(strNameIn)
        , strBaseLocation(config::GetDataDir() + strNameIn + "/datachain/")
        , cachePool(new CacheType(MAX_SECTOR_CACHE_SIZE))
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , nBytesRead(0)
        , nBytesWrote(0)
        , nCurrentFile(0)
        , nCurrentFileSize(0)
        , CacheWriterThread(std::bind(&SectorDatabase::CacheWriter, this))
        , MeterThread(std::bind(&SectorDatabase::Meter, this))
        , nBufferBytes(0)
        {
            if(config::GetBoolArg("-runtime", false))
                runtime.Start();

            /* Read only flag when instantiating new database. */
            fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));

            /* Initialize the Database. */
            Initialize();

            if(config::GetBoolArg("-runtime", false))
                debug::log(0, ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());
        }

        ~SectorDatabase()
        {
            fDestruct = true;

            CacheWriterThread.join();

            delete pTransaction;
            delete cachePool;
            delete pSectorKeys;
        }


        /** Initialize Sector Database. **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
                debug::log(0, FUNCTION "Generated Path %s", __PRETTY_FUNCTION__, strBaseLocation.c_str());

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
            pSectorKeys = new KeychainType((config::GetDataDir() + strName + "/keychain/"));

            pTransaction = nullptr;
            fInitialized = true;
        }


        /* Get the keys for this sector database from the keychain.  */
        std::vector< std::vector<uint8_t> > GetKeys()
        {
            return pSectorKeys->GetKeys();
        }


        template<typename Key>
        bool Exists(const Key& key)
        {
            /** Serialize Key into Bytes. **/
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Return the Key existance in the Keychain Database. */
            SectorKey cKey;
            return pSectorKeys->Get(static_cast<std::vector<uint8_t>>(ssKey), cKey);
        }

        template<typename Key>
        bool Erase(const Key& key)
        {
            if(config::GetBoolArg("-runtime", false))
                runtime.Start();

            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            if(pTransaction)
            {
                pTransaction->EraseTransaction(static_cast<std::vector<uint8_t>>(ssKey));

                return true;
            }

            /* Return the Key existance in the Keychain Database. */
            bool fErased = pSectorKeys->Erase(static_cast<std::vector<uint8_t>>(ssKey));

            if(config::GetBoolArg("-runtime", false))
                debug::log(0, ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

            return fErased;
        }

        template<typename Key, typename Type>
        bool Read(const Key& key, Type& value)
        {
            /** Serialize Key into Bytes. **/
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /** Get the Data from Sector Database. **/
            std::vector<uint8_t> vData;
            if(!Get(static_cast<std::vector<uint8_t>>(ssKey), vData))
                return false;

            /** Deserialize Value. **/
            DataStream ssValue(vData, SER_LLD, DATABASE_VERSION);
            ssValue >> value;

            return true;
        }

        template<typename Key, typename Type>
        bool Write(const Key& key, const Type& value)
        {
            if (fReadOnly)
                assert(!"Write called on database in read-only mode");

            /* Serialize the Key. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Serialize the Value */
            DataStream ssData(SER_LLD, DATABASE_VERSION);
            ssData << value;

            return Put(ssKey, ssData);
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
        bool Get(std::vector<uint8_t> vKey, std::vector<uint8_t>& vData)
        {
            /* Iterate if meters are enabled. */
            nBytesRead += (vKey.size() + vData.size());

            /* Check the cache pool for key first. */
            if(cachePool->Get(vKey, vData))
                return true;

            /* Check that the key is not pending in a transaction for Erase. */
            if(pTransaction && pTransaction->mapEraseData.count(vKey))
                return false;

            /* Check if the new data is set in a transaction to ensure that the database knows what is in volatile memory. */
            if(pTransaction && pTransaction->mapTransactions.count(vKey))
            {
                vData = pTransaction->mapTransactions[vKey];

                debug::log(4, FUNCTION "%s", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str());

                return true;
            }

            /* Get the key from the keychain. */
            SectorKey cKey;
            if(pSectorKeys->Get(vKey, cKey))
            {
                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(cKey.nSectorFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream)
                        return false;

                    /* If file not found add to LRU cache. */
                    fileCache->Put(cKey.nSectorFile, pstream);
                }

                /* Read the record from disk. */
                {
                    LOCK(SECTOR_MUTEX);

                    /* Seek to the Sector Position on Disk. */
                    pstream->seekg(cKey.nSectorStart, std::ios::beg);
                    vData.resize(cKey.nSectorSize);

                    /* Read the State and Size of Sector Header. */
                    pstream->read((char*) &vData[0], vData.size());
                }

                /* Add to cache */
                cachePool->Put(vKey, vData);

                /* Verbose Debug Logging. */
                debug::log(4, FUNCTION "%s", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str());

                return true;
            }
            else
                return debug::error(FUNCTION "KEY NOT FOUND", __PRETTY_FUNCTION__);

            return false;
        }


        /** Get
         *
         *  Get a record from from disk if the sector key
         *  is already read from the keychain.
         *
         *  @param[in] cKey The sector key from keychain.
         *  @param[in] vData The binary data of the record to get.
         *
         *  @return True if the record was read successfully.
         *
         **/
        bool Get(SectorKey cKey, std::vector<uint8_t>& vData)
        {
            nBytesRead += (cKey.vKey.size() + vData.size());

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(cKey.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream)
                    return false;

                /* If file not found add to LRU cache. */
                fileCache->Put(cKey.nSectorFile, pstream);
            }

            /* Read the record from the disk. */
            {
                LOCK(SECTOR_MUTEX);

                /* Seek to the Sector Position on Disk. */
                pstream->seekg(cKey.nSectorStart, std::ios::beg);
                vData.resize(cKey.nSectorSize);

                /* Read the State and Size of Sector Header. */
                pstream->read((char*) &vData[0], vData.size());
            }

            /* Verbose Debug Logging. */
            debug::log(4, FUNCTION "%s", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str());

            return true;
        }


        /** Flush
         *
         *  Flush a write to disk immediately bypassing write buffers.
         *
         *  @param[in] vKey The binary data of the key to flush
         *  @param[in] vData The binary data of the record to flush
         *
         *  @return True if the flush was successful.
         *
         **/
        bool Flush(std::vector<uint8_t> vKey, std::vector<uint8_t> vData)
        {
            if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
            {
                debug::log(4, FUNCTION "Current File too Large, allocating new File %u", __PRETTY_FUNCTION__, nCurrentFileSize, nCurrentFile + 1);

                ++ nCurrentFile;
                nCurrentFileSize = 0;

                std::ofstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                stream.close();
            }

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(nCurrentFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream)
                    return false;

                /* If file not found add to LRU cache. */
                fileCache->Put(nCurrentFile, pstream);
            }

            /* Write the data to disk. */
            {
                LOCK(SECTOR_MUTEX);

                /* If it is a New Sector, Assign a Binary Position. */
                pstream->seekp(nCurrentFileSize, std::ios::beg);
                pstream->write((char*) &vData[0], vData.size());
                pstream->flush();
            }

            /* Create a new Sector Key. */
            SectorKey cKey = SectorKey(READY, vKey, nCurrentFile, nCurrentFileSize, vData.size());

            /* Increment the current filesize */
            nCurrentFileSize += vData.size();

            /* Assign the Key to Keychain. */
            pSectorKeys->Put(cKey);

            /* Verboe output. */
            debug::log(4, FUNCTION "%s | Current File: %u | Current File Size: %u", __PRETTY_FUNCTION__, HexStr(vData.begin(), vData.end()).c_str(), nCurrentFile, nCurrentFileSize);

            if(config::GetBoolArg("-runtime", false))
                debug::log(0, ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

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
        bool Put(std::vector<uint8_t> vKey, std::vector<uint8_t> vData)
        {
            /* Write the data into the memory cache. */
            cachePool->Put(vKey, vData, true);

            while(!config::fShutdown && nBufferBytes > MAX_SECTOR_BUFFER_SIZE)
                runtime::sleep(100);

            /* Add to the write buffer thread. */
            { LOCK(BUFFER_MUTEX);
                vDiskBuffer.push_back(std::make_pair(vKey, vData));
                nBufferBytes += (vKey.size() + vData.size());

                CONDITION.notify_all();
            }

            return true;
        }


        /** Cache Writer
         *
         *  Flushes periodically data from the cache buffer to disk.
         *
         **/
        void CacheWriter()
        {
            /* The mutex for the condition. */
            std::mutex CONDITION_MUTEX;

            /* Wait for initialization. */
            while(!fInitialized)
                runtime::sleep(100);

            while(true)
            {
                /* Wait for buffer to empty before shutting down. */
                if(config::fShutdown && nBufferBytes == 0)
                    return;

                /* Check for data to be written. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait(CONDITION_LOCK, [this]{ return nBufferBytes > 0; });

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
                    debug::log(0, FUNCTION "Generated Sector File %u", __PRETTY_FUNCTION__, nCurrentFile + 1);

                    /* Iterate the current file and reset current file sie. */
                    ++nCurrentFile;
                    nCurrentFileSize = 0;

                    /* Create a new file for next writes. */
                    std::fstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::out | std::ios::binary | std::ios::trunc);
                    stream.close();
                }

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                { LOCK(SECTOR_MUTEX);

                    if(!fileCache->Get(nCurrentFile, pstream))
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream)
                            continue;

                        /* If file not found add to LRU cache. */
                        fileCache->Put(nCurrentFile, pstream);
                    }

                    /* Seek to the end of the file */
                    pstream->seekp(nCurrentFileSize, std::ios::beg);
                }

                /* Iterate through buffer to queue disk writes. */
                std::vector<uint8_t> vWrite;
                for(auto & vObj : vIndexes)
                {
                    /* Assign the Key to Keychain. */
                    pSectorKeys->Put(SectorKey(READY, vObj.first, nCurrentFile, nCurrentFileSize, vObj.second.size()));

                    /* Increment the current filesize */
                    nCurrentFileSize += vObj.second.size();

                    /* Add data to the write buffer */
                    vWrite.insert(vWrite.end(), vObj.second.begin(), vObj.second.end());

                    /* Add the file size to the written bytes. */
                    nBytesWrote += vObj.first.size();

                    /* Flush to disk on periodic intervals (1 MB). */
                    if(vWrite.size() > 1024 * 1024)
                    {
                        LOCK(SECTOR_MUTEX);

                        nBytesWrote += (vWrite.size());

                        pstream->write((char*)&vWrite[0], vWrite.size());
                        pstream->flush();

                        vWrite.clear();
                    }

                    /* Set cache back to not reserved. */
                    cachePool->Reserve(vObj.first, false);

                    ++nRecordsFlushed;
                }

                nBytesWrote += (vWrite.size());

                /* Flush remaining to disk. */
                {
                    LOCK(SECTOR_MUTEX);
                    pstream->write((char*)&vWrite[0], vWrite.size());
                    pstream->flush();
                }

                /* Verbose logging. */
                debug::log(4, FUNCTION "Flushed %u Records of %u Bytes", __PRETTY_FUNCTION__, nRecordsFlushed, nBytesWrote);

                /* Reset counters if not in meter mode. */
                if(!config::GetBoolArg("-meters"))
                {
                    nBytesWrote     = 0;
                    nRecordsFlushed = 0;
                }
            }
        }

        /* LLP Meter Thread. Tracks the Requests / Second. */
        void Meter()
        {
            if(!config::GetBoolArg("-meters", false))
                return;

            runtime::timer TIMER;
            TIMER.Start();

            while(!config::fShutdown)
            {
                nBytesWrote     = 0;
                nBytesRead      = 0;
                nRecordsFlushed = 0;

                runtime::sleep(10000);

                double WPS = nBytesWrote / (TIMER.Elapsed() * 1024.0);
                double RPS = nBytesRead / (TIMER.Elapsed() * 1024.0);

                debug::log(0, FUNCTION ">>>>> LLD Writing at %f Kb/s", __PRETTY_FUNCTION__, WPS);
                debug::log(0, FUNCTION ">>>>> LLD Reading at %f Kb/s", __PRETTY_FUNCTION__, RPS);
                debug::log(0, FUNCTION ">>>>> LLD Flushed %u Records", __PRETTY_FUNCTION__, nRecordsFlushed);

                TIMER.Reset();
            }
        }

        /** Start a New Database Transaction.
            This will put all the database changes into pending state.
            If any of the database updates fail in procewss it will roll the database back to its previous state. **/
        void TxnBegin()
        {
            /* Delete a previous database transaction pointer if applicable. */
            if(pTransaction)
                delete pTransaction;

            /* Create the new Database Transaction Object. */
            pTransaction = new SectorTransaction();

            /* Debug Output. */
            debug::log(4, FUNCTION "New Sector Transaction Started.", __PRETTY_FUNCTION__);
        }

        /** Abort the current transaction that is pending in the transaction chain. **/
        void TxnAbort()
        {
            /** Delete the previous transaction pointer if applicable. **/
            if(pTransaction)
                delete pTransaction;

            /** Set the transaction pointer to null also acting like a flag **/
            pTransaction = nullptr;
        }

        /** Return the database state back to its original state before transactions are commited. **/
        bool RollbackTransactions()
        {
                /** Iterate the original data memory map to reset the database to its previous state. **/
            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapOriginalData.begin(); nIterator != pTransaction->mapOriginalData.end(); nIterator++ )
                if(!Put(nIterator->first, nIterator->second))
                    return false;

            return true;
        }

        /** Commit the Data in the Transaction Object to the Database Disk.
            TODO: Handle the Transaction Rollbacks with a new Transaction Keychain and Sector Database.
            Make it temporary and named after the unique identity of the sector database.
            Fingerprint is SK64 hash of unified time and the sector database name along with some other data
            To be determined...

            1. TxnStart()
                + Create a new Transaction Record (TODO: Find how this will be indentified. Maybe Unique Tx Hash and Registry in Journal of Active Transaction?)
                + Create a new Transaction Memory Object

            2. Put()
                + Add new data to the Transaction Record
                + Add new data to the Transaction Memory
                + Keep states of keys in a valid object for recover of corrupted keychain.

            3. Get()  NOTE: Read from the Transaction object rather than the database
                + Read the data from the Transaction Memory

            4. Commit()
                +

            **/
        bool TxnCommit()
        {
            std::unique_lock<std::recursive_mutex> lk(SECTOR_MUTEX);

            if(config::GetBoolArg("-runtime", false))
                runtime.Start();

            debug::log(4, FUNCTION "Commiting Transactin to Datachain.", __PRETTY_FUNCTION__);

            /** Check that there is a valid transaction to apply to the database. **/
            if(!pTransaction)
                return error(FUNCTION "No Transaction data to Commit.", __PRETTY_FUNCTION__);

            /** Habdle setting the sector key flags so the database knows if the transaction was completed properly. **/
            debug::log(4, FUNCTION "Commiting Keys to Keychain.", __PRETTY_FUNCTION__);

            /** Set the Sector Keys to an Invalid State to know if there are interuptions the sector was not finished successfully. **/
            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                SectorKey cKey;
                if(pSectorKeys->HasKey(nIterator->first))
                {
                    if(!pSectorKeys->Get(nIterator->first, cKey))
                        return error(FUNCTION "Couldn't get the Active Sector Key.", __PRETTY_FUNCTION__);

                    cKey.nState = TRANSACTION;
                    pSectorKeys->Put(cKey);
                }
            }

            /** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
            debug::log(4, FUNCTION "Erasing Sector Keys Flagged for Deletion.", __PRETTY_FUNCTION__);

            /** Erase all the Transactions that are set to be erased. That way if they are assigned a TRANSACTION flag we know to roll back their key to orginal data. **/
            for(typename std::map< std::vector<uint8_t>, uint32_t >::iterator nIterator = pTransaction->mapEraseData.begin(); nIterator != pTransaction->mapEraseData.end(); nIterator++ )
            {
                if(!pSectorKeys->Erase(nIterator->first))
                    return error(FUNCTION "Couldn't get the Active Sector Key for Delete.", __PRETTY_FUNCTION__);
            }

            /** Commit the Sector Data to the Database. **/
            debug::log(4, FUNCTION "Commit Data to Datachain Sector Database.", __PRETTY_FUNCTION__);

            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                /** Declare the Key and Data for easier reference. **/
                std::vector<uint8_t> vKey  = nIterator->first;
                std::vector<uint8_t> vData = nIterator->second;

                /* Write Header if First Update. */
                if(!pSectorKeys->HasKey(vKey))
                {
                    if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                    {
                        debug::log(4, FUNCTION "Current File too Large, allocating new File %u", __PRETTY_FUNCTION__, nCurrentFileSize, nCurrentFile + 1);

                        ++nCurrentFile;
                        nCurrentFileSize = 0;

                        std::ofstream fStream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::out | std::ios::binary);
                        fStream.close();
                    }

                    /* Open the Stream to Read the data from Sector on File. */
                    std::string strFilename = debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile);
                    std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);

                    /* If it is a New Sector, Assign a Binary Position.
                        TODO: Track Sector Database File Sizes. */
                    fStream.seekp(nCurrentFileSize, std::ios::beg);

                    fStream.write((char*) &vData[0], vData.size());
                    fStream.close();

                    /* Create a new Sector Key. */
                    SectorKey cKey(READY, vKey, nCurrentFile, nCurrentFileSize, vData.size());

                    /* Increment the current filesize */
                    nCurrentFileSize += vData.size();

                    /* Assign the Key to Keychain. */
                    pSectorKeys->Put(cKey);
                }
                else
                {
                    /* Get the Sector Key from the Keychain. */
                    SectorKey cKey;
                    if(!pSectorKeys->Get(vKey, cKey))
                    {
                        pSectorKeys->Erase(vKey);

                        return false;
                    }

                    /* Open the Stream to Read the data from Sector on File. */
                    std::string strFilename = debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), cKey.nSectorFile);
                    std::fstream fStream(strFilename.c_str(), std::ios::in | std::ios::out | std::ios::binary);

                    /* Locate the Sector Data from Sector Key.
                        TODO: Make Paging more Efficient in Keys by breaking data into different locations in Database. */
                    fStream.seekp(cKey.nSectorStart, std::ios::beg);
                    if(vData.size() > cKey.nSectorSize)
                    {
                        fStream.close();
                        debug::log(0, FUNCTION "PUT (TOO LARGE) NO TRUNCATING ALLOWED (Old %u :: New %u):%s", __PRETTY_FUNCTION__, cKey.nSectorSize, vData.size(), HexStr(vData.begin(), vData.end()).c_str());

                        return false;
                    }

                    /* Assign the Writing State for Sector. */
                    //TODO: use memory maps

                    fStream.write((char*) &vData[0], vData.size());
                    fStream.close();

                    cKey.nState    = READY;

                    pSectorKeys->Put(cKey);
                }
            }

            /** Update the Keychain with Checksums and READY Flag letting sectors know they were written successfully. **/
            debug::log(4, FUNCTION "Commiting Key Valid States to Keychain.", __PRETTY_FUNCTION__);

            for(typename std::map< std::vector<uint8_t>, std::vector<uint8_t> >::iterator nIterator = pTransaction->mapTransactions.begin(); nIterator != pTransaction->mapTransactions.end(); nIterator++ )
            {
                /** Assign the Writing State for Sector. **/
                SectorKey cKey;
                if(!pSectorKeys->Get(nIterator->first, cKey))
                    return error(FUNCTION "Failed to Get Key from Keychain.", __PRETTY_FUNCTION__);

                /** Set the Sector states back to Active. **/
                cKey.nState    = READY;

                /** Commit the Keys to Keychain Database. **/
                if(!pSectorKeys->Put(cKey))
                    return error(FUNCTION "Failed to Commit Key to Keychain.", __PRETTY_FUNCTION__);
            }

            /** Clean up the Sector Transaction Key.
                TODO: Delete the Sector and Keychain for Current Transaction Commit ID. **/
            delete pTransaction;
            pTransaction = nullptr;

            if(config::GetBoolArg("-runtime", false))
                debug::log(0, ANSI_COLOR_GREEN FUNCTION "executed in %u micro-seconds" ANSI_COLOR_RESET, __PRETTY_FUNCTION__, runtime.ElapsedMicroseconds());

            return true;
        }
    };
}

#endif
