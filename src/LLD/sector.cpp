/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/sector.h>
#include <Util/include/filesystem.h>
#include <Util/include/hex.h>

#include <functional>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    template<class KeychainType, class CacheType>
    SectorDatabase<KeychainType, CacheType>::SectorDatabase(const std::string& strNameIn,
                                                            const uint8_t nFlagsIn, const uint64_t nBucketsIn,
                                                            const uint32_t nCacheIn)
    : CONDITION_MUTEX()
    , CONDITION()
    , SECTOR_MUTEX()
    , BUFFER_MUTEX()
    , TRANSACTION_MUTEX()
    , strBaseLocation(config::GetDataDir() + strNameIn + "/datachain/")
    , strName(strNameIn)
    , runtime()
    , pTransaction(nullptr)
    , pSectorKeys(new KeychainType((config::GetDataDir() + strName + "/keychain/"), nFlagsIn, nBucketsIn))
    , cachePool(new CacheType(nCacheIn))
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
    template<class KeychainType, class CacheType>
    SectorDatabase<KeychainType, CacheType>::~SectorDatabase()
    {
        fDestruct = true;
        CONDITION.notify_all();

        if(CacheWriterThread.joinable())
            CacheWriterThread.join();

        if(MeterThread.joinable())
            MeterThread.join();

        if(pTransaction)
            delete pTransaction;

        if(cachePool)
            delete cachePool;

        if(fileCache)
            delete fileCache;

        if(pSectorKeys)
            delete pSectorKeys;
    }


    /*  Initialize Sector Database. */
    template<class KeychainType, class CacheType>
    void SectorDatabase<KeychainType, CacheType>::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(nFlags & FLAGS::CREATE && !filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Find the most recent append file. */
        while(true)
        {

            /* TODO: Make a worker or thread to check sizes of files and automatically create new file.
                Keep independent of reads and writes for efficiency. */
            std::fstream stream(debug::strprintf("%s_block.%05u", strBaseLocation.c_str(), nCurrentFile).c_str(), std::ios::in | std::ios::binary);
            if(!stream)
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
            stream.seekg(0, std::ios::end);
            nCurrentFileSize = static_cast<uint32_t>(stream.tellg());
            stream.close();

            /* Increment the Current File */
            ++nCurrentFile;
        }

        pTransaction = nullptr;
        fInitialized = true;
    }


    /*  Get a record from cache or from disk */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
    {
        /* Iterate if meters are enabled. */
        nBytesRead += static_cast<uint32_t>(vKey.size() + vData.size());

        /* Check the cache pool for key first. */
        if(cachePool->Get(vKey, vData))
            return true;

        /* Get the key from the keychain. */
        SectorKey cKey;
        if(pSectorKeys->Get(vKey, cKey))
        {
            {
                LOCK(SECTOR_MUTEX);

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

                /* Get compact size from record. */
                uint64_t nSize = GetSizeOfCompactSize(cKey.nSectorSize);

                /* Seek to the Sector Position on Disk. */
                pstream->seekg(cKey.nSectorStart + nSize, std::ios::beg);

                /* Resize for proper record length. */
                vData.resize(cKey.nSectorSize - nSize);

                /* Read the State and Size of Sector Header. */
                if(!pstream->read((char*) &vData[0], vData.size()))
                    return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");

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


    /*  Get a record from from disk if the sector key
     *  is already read from the keychain. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Get(const SectorKey& cKey, std::vector<uint8_t>& vData)
    {
        {
            LOCK(SECTOR_MUTEX);

            nBytesRead += static_cast<uint32_t>(cKey.vKey.size() + vData.size());

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

            /* Get compact size from record. */
            uint64_t nSize = GetSizeOfCompactSize(cKey.nSectorSize);

            /* Seek to the Sector Position on Disk. */
            pstream->seekg(cKey.nSectorStart + nSize, std::ios::beg);

            /* Resize for proper record length. */
            vData.resize(cKey.nSectorSize - nSize);

            /* Read the State and Size of Sector Header. */
            if(!pstream->read((char*) &vData[0], vData.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");

            /* Verboe output. */
            debug::log(5, FUNCTION, "Current File: ", cKey.nSectorFile,
                " | Current File Size: ", cKey.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
        }

        return true;
    }


    /*  Update a record on disk. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Update(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
    {
        /* Check the keychain for key. */
        SectorKey key;
        if(!pSectorKeys->Get(vKey, key))
            return false;

        /* Get current size */
        uint64_t nSize = vData.size() + GetSizeOfCompactSize(vData.size());

        /* Check data size constraints. */
        if(nSize != key.nSectorSize)
            return false;

        /* Write the data into the memory cache. */
        cachePool->Put(vKey, vData, false);

        {
            LOCK(SECTOR_MUTEX);

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

            /* Write the size of record. */
            WriteCompactSize(*pstream, vData.size());

            /* Write the data record. */
            if(!pstream->write((char*) &vData[0], vData.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes written");

            pstream->flush();

            /* Records flushed indicator. */
            ++nRecordsFlushed;
            nBytesWrote += static_cast<uint32_t>(vData.size());

            /* Verboe output. */
            debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
        }

        return true;
    }


    /*  Force a write to disk immediately bypassing write buffers. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Force(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
    {
        if(nFlags & FLAGS::APPEND || !Update(vKey, vData))
        {
            /* Write the data into the memory cache. */
            cachePool->Put(vKey, vData, false);

            {
                LOCK(SECTOR_MUTEX);

                /* Create new file if above current file size. */
                if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                {
                    debug::log(4, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                    ++nCurrentFile;
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

                /* Write the size of record. */
                WriteCompactSize(*pstream, vData.size());

                /* Write the data record. */
                if(!pstream->write((char*) &vData[0], vData.size()))
                    return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes written");

                pstream->flush();

            }

            /* Get current size */
            uint64_t nSize = vData.size() + GetSizeOfCompactSize(vData.size());

            /* Create a new Sector Key. */
            SectorKey key(STATE::READY, vKey, static_cast<uint16_t>(nCurrentFile),
                            nCurrentFileSize, static_cast<uint32_t>(nSize));

            /* Increment the current filesize */
            nCurrentFileSize += static_cast<uint32_t>(nSize);

            /* Assign the Key to Keychain. */
            if(!pSectorKeys->Put(key))
                return debug::error(FUNCTION, "failed to write key to keychain");

            /* Verboe output. */
            debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
        }

        return true;
    }


    /*  Write a record into the cache and disk buffer for flushing to disk. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Put(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
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
        {
            LOCK(BUFFER_MUTEX);

            vDiskBuffer.push_back(std::make_pair(vKey, vData));
            nBufferBytes += static_cast<uint32_t>(vKey.size() + vData.size());
        }

        /* Notify if buffer was added to. */
        CONDITION.notify_all();

        return true;
    }


    /*  Flushes periodically data from the cache buffer to disk. */
    template<class KeychainType, class CacheType>
    void SectorDatabase<KeychainType, CacheType>::CacheWriter()
    {
        /* Wait for initialization. */
        while(!fInitialized)
            runtime::sleep(100);

        /* Check if writing is enabled. */
        if(!(nFlags & FLAGS::WRITE) && !(nFlags & FLAGS::APPEND))
            return;

        /* No cache write on force mode. */
        if(nFlags & FLAGS::FORCE)
        {
            debug::log(0, FUNCTION, strBaseLocation, " in FORCE mode... closing");
            return;
        }


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
                nBytesWrote += static_cast<uint32_t>(vObj.first.size() + vObj.second.size());
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


    /*  LLD Meter Thread. Tracks the Reads/Writes per second. */
    template<class KeychainType, class CacheType>
    void SectorDatabase<KeychainType, CacheType>::Meter()
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


    /*  Start a database transaction. */
    template<class KeychainType, class CacheType>
    void SectorDatabase<KeychainType, CacheType>::TxnBegin()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Delete a previous database transaction pointer if applicable. */
        if(pTransaction)
            delete pTransaction;

        /* Create the new Database Transaction Object. */
        pTransaction = new SectorTransaction();

        /* Create an append only stream. */
        STREAM = std::ofstream(debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat"), std::ios::app | std::ios::binary);
        if(!STREAM.is_open())
            throw std::runtime_error("Failed to open Journal File");
    }

    /* Rollback the transaction to previous state. */
     template<class KeychainType, class CacheType>
     void SectorDatabase<KeychainType, CacheType>::TxnRollback()
     {
         LOCK(TRANSACTION_MUTEX);

         /* Check that there is a valid transaction to apply to the database. */
         assert(pTransaction);

         /* Restore erase data. */
         for(const auto& item : pTransaction->mapEraseData)
             if(!pSectorKeys->Restore(item.first))
                 assert(debug::error(FUNCTION, "failed to rollback erase"));

        /* Erase all the transactions. */
        for(const auto& item : pTransaction->mapTransactions)
            if(!pSectorKeys->Erase(item.first))
                assert(debug::error(FUNCTION, "failed to rollback transactions"));

         /* Commit the sector data. */
         for(const auto& item : pTransaction->mapOriginalData)
             if(!Force(item.first, item.second))
                assert(debug::error(FUNCTION, "failed to rollback sector data"));

         /* Commit keychain entries. */
         for(const auto& item : pTransaction->mapKeychain)
             if(!pSectorKeys->Erase(item.first))
                assert(debug::error(FUNCTION, "failed to commit to keychain"));

         /* Commit the index data. */
         for(const auto& item : pTransaction->mapIndex)
            if(!pSectorKeys->Erase(item.first))
                assert(debug::error(FUNCTION, "failed to erase indexes"));

         /* Cleanup the transaction object. */
         delete pTransaction;
         pTransaction = nullptr;
     }


    /*  Write the transaction commitment message. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnCheckpoint()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Serialize the key. */
        DataStream ssJournal(SER_LLD, DATABASE_VERSION);
        ssJournal << std::string("commit");

        /* Write to the file.  */
        const std::vector<uint8_t>& vBytes = ssJournal.Bytes();
        STREAM.write((char*)&vBytes[0], vBytes.size());
        STREAM.close();

        return true;
    }


    /*  Release the transaction checkpoint. */
    template<class KeychainType, class CacheType>
    void SectorDatabase<KeychainType, CacheType>::TxnRelease()
    {
        LOCK(TRANSACTION_MUTEX);

        /** Delete the previous transaction pointer if applicable. **/
        if(pTransaction)
            delete pTransaction;

        /** Set the transaction pointer to null also acting like a flag **/
        pTransaction = nullptr;

        /* Delete the transaction journal file. */
        std::ofstream stream(debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat"), std::ios::trunc);
        stream.close();
    }


    /*  Commit data from transaction object. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnCommit()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Check that there is a valid transaction to apply to the database. */
        if(!pTransaction)
            return false;

        /* Erase data set to be removed. */
        for(const auto& item : pTransaction->mapEraseData)
            if(!pSectorKeys->Erase(item.first))
                return debug::error(FUNCTION, "failed to erase from keychain");

        /* Commit the sector data. */
        for(const auto& item : pTransaction->mapTransactions)
            if(!Force(item.first, item.second))
                return debug::error(FUNCTION, "failed to commit sector data");

        /* Commit keychain entries. */
        for(const auto& item : pTransaction->mapKeychain)
        {
            SectorKey cKey(STATE::READY, item.first, 0, 0, 0);
            if(!pSectorKeys->Put(cKey))
                return debug::error(FUNCTION, "failed to commit to keychain");
        }

        /* Commit the index data. */
        std::map<std::vector<uint8_t>, SectorKey> mapIndex;
        for(const auto& item : pTransaction->mapIndex)
        {
            /* Get the key. */
            SectorKey cKey;
            if(mapIndex.count(item.second))
                cKey = mapIndex[item.second];
            else
            {
                if(!pSectorKeys->Get(item.second, cKey))
                    return debug::error(FUNCTION, "failed to read indexing entry");

                mapIndex[item.second] = cKey;
            }

            /* Write the new sector key. */
            cKey.SetKey(item.first);
            if(!pSectorKeys->Put(cKey))
                return debug::error(FUNCTION, "failed to write indexing entry");
        }

        /* Cleanup the transaction object. */
        delete pTransaction;
        pTransaction = nullptr;

        return true;
    }


    /*  Recover a transaction from the journal. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnRecovery()
    {
        /* Create an append only stream. */
        std::ifstream stream(debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat"), std::ios::in | std::ios::out | std::ios::binary);
        if(!stream.is_open())
            return false;

        /* Get the Binary Size. */
        stream.ignore(std::numeric_limits<std::streamsize>::max());

        /* Get the data buffer. */
        uint32_t nSize = static_cast<uint32_t>(stream.gcount());

        /* Check journal size for 0. */
        if(nSize == 0)
            return false;

        /* Create buffer to read into. */
        std::vector<uint8_t> vBuffer(nSize, 0);

        /* Read the keychain file. */
        stream.seekg (0, std::ios::beg);
        stream.read((char*) &vBuffer[0], vBuffer.size());
        stream.close();

        debug::log(0, FUNCTION, strName, " transaction journal detected of ", nSize, " bytes");

        /* Create the transaction object. */
        TxnBegin();

        /* Serialize the key. */
        const DataStream ssJournal(vBuffer, SER_LLD, DATABASE_VERSION);
        while(!ssJournal.End())
        {
            /* Read the data entry type. */
            std::string strType;
            ssJournal >> strType;

            /* Check for Erase. */
            if(strType == "erase")
            {
                /* Get the key to erase. */
                std::vector<uint8_t> vKey;
                ssJournal >> vKey;

                /* Erase the key. */
                pTransaction->EraseTransaction(vKey);

                /* Debug output. */
                debug::log(0, FUNCTION, "erasing key ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
            }
            else if(strType == "key")
            {
                /* Get the key to write. */
                std::vector<uint8_t> vKey;
                ssJournal >> vKey;

                /* Write the key. */
                pTransaction->mapKeychain[vKey] = 0;

                /* Debug output. */
                debug::log(0, FUNCTION, "writing keychain ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
            }
            else if(strType == "write")
            {
                /* Get the key to write. */
                std::vector<uint8_t> vKey;
                ssJournal >> vKey;

                /* Get the data to write. */
                std::vector<uint8_t> vData;
                ssJournal >> vData;

                /* Write the sector data. */
                pTransaction->mapTransactions[vKey] = vData;

                /* Debug output. */
                debug::log(0, FUNCTION, "writing data ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
            }
            else if(strType == "index")
            {
                /* Get the key to index. */
                std::vector<uint8_t> vKey;
                ssJournal >> vKey;

                /* Get the data to index to. */
                std::vector<uint8_t> vIndex;
                ssJournal >> vIndex;

                /* Set the indexing key. */
                pTransaction->mapIndex[vKey] = vIndex;

                /* Debug output. */
                debug::log(0, FUNCTION, "indexing key ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
            }
            if(strType == "commit")
            {
                debug::log(0, FUNCTION, strName, " transaction journal ready to be restored");

                return true;
            }
        }

        return debug::error(FUNCTION, strName, " transaction journal never reached commit");
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class SectorDatabase<BinaryHashMap, BinaryLRU>;

}
