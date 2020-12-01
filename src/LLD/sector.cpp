/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lfu.h>
#include <LLD/cache/binary_lru.h>

#include <LLD/keychain/filemap.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/keychain/shard_hashmap.h>
#include <LLD/keychain/hashtree.h>

#include <Util/include/filesystem.h>
#include <Util/include/hex.h>
#include <Util/include/runtime.h>

#include <functional>

namespace LLD::Templates
{

    /* The Database Constructor. To determine file location and the Bytes per Record. */
    template<class KeychainType, class CacheType, class ConfigType>
    StaticDatabase<KeychainType, CacheType, ConfigType>::StaticDatabase(const LLD::Config::Sector& sectorIn, const ConfigType& keychainIn)
    : CONDITION_MUTEX   ( )
    , CONDITION         ( )
    , SECTOR_MUTEX      ( )
    , BUFFER_MUTEX      ( )
    , TRANSACTION_MUTEX ( )
    , CONFIG            (sectorIn)
    , runtime           ( )
    , pTransaction      (nullptr)
    , pSectorKeys       (new KeychainType(keychainIn))
    , cachePool         (new CacheType(CONFIG.MAX_SECTOR_CACHE_SIZE))
    , fileCache         (new TemplateLRU<uint32_t, std::fstream*>(CONFIG.MAX_SECTOR_FILE_STREAMS))
    , nCurrentFile      (0)
    , nCurrentFileSize  (0)
    , CacheWriterThread ( )
    , MeterThread       ( )
    , mapDiskBuffer     ( )
    , nBufferBytes      (0)
    , nBytesRead        (0)
    , nBytesWrote       (0)
    , nRecordsFlushed   (0)
    , fDestruct         (false)
    , fInitialized      (false)
    {
        /* Initialize the Database. */
        Initialize();

        if(config::GetBoolArg("-runtime", false))
        {
            debug::log(0, ANSI_COLOR_GREEN FUNCTION, "executed in ",
                runtime.ElapsedMicroseconds(), " micro-seconds" ANSI_COLOR_RESET);
        }

        CacheWriterThread = std::thread(std::bind(&StaticDatabase::CacheWriter, this));
        MeterThread       = std::thread(std::bind(&StaticDatabase::Meter, this));
    }


    /* Default Destructor */
    template<class KeychainType, class CacheType, class ConfigType>
    StaticDatabase<KeychainType, CacheType, ConfigType>::~StaticDatabase()
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
    template<class KeychainType, class CacheType, class ConfigType>
    void StaticDatabase<KeychainType, CacheType, ConfigType>::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(CONFIG.FLAGS & FLAGS::CREATE && !filesystem::exists(CONFIG.DIRECTORY + "datachain/") && filesystem::create_directories(CONFIG.DIRECTORY + "datachain/"))
            debug::log(0, FUNCTION, "Generated Path ", CONFIG.DIRECTORY);

        /* Find the most recent append file. */
        while(true)
        {

            /* Find our current sector file we are on. */
            std::fstream stream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::in | std::ios::binary);
            if(!stream)
            {

                /* Assign the Current Size and File. */
                if(nCurrentFile > 0)
                    --nCurrentFile;
                else
                {
                    /* Create a new file if it doesn't exist. */
                    std::ofstream cStream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::binary | std::ios::out | std::ios::trunc);
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
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
    {
        /* Iterate if meters are enabled. */
        nBytesRead += static_cast<uint32_t>(vKey.size() + vData.size());

        /* Check the cache pool for key first. */
        if(cachePool->Get(vKey, vData))
            return true;

        /* Check disk buffer. */
        if(mapDiskBuffer.count(vKey))
        {
            /* Grab the data off the buffer. */
            vData = mapDiskBuffer[vKey];
            return true;
        }

        /* Get the key from the keychain. */
        SectorKey key;
        if(pSectorKeys->Get(vKey, key))
        {
            {
                LOCK(CONFIG.FILE(key.nSectorFile));

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(key.nSectorFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return debug::error(FUNCTION, "couldn't create stream file");
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(key.nSectorFile, pstream);
                }

                /* Get compact size from record. */
                uint64_t nSize = GetSizeOfCompactSize(key.nSectorSize);

                /* Seek to the Sector Position on Disk. */
                pstream->seekg(key.nSectorStart + nSize, std::ios::beg);

                /* Resize for proper record length. */
                vData.resize(key.nSectorSize - nSize);

                /* Read the State and Size of Sector Header. */
                if(!pstream->read((char*) &vData[0], vData.size()))
                {
                    debug::log(0, "SECTOR STREAM: ", pstream->eof() ? "EOF" : pstream->bad() ? "BAD" : pstream->fail() ? "FAIL" : "UNKNOWN");
                    debug::log(0, "Current File: ", key.nSectorFile, " | Current File Size: ", key.nSectorStart);

                    return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");
                }

            }

            /* Add to cache */
            cachePool->Put(key, vKey, vData);

            /* Verbose Debug Logging. */
            if(config::nVerbose >= 5)
                debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                    " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));

            return true;
        }

        return false;
    }


    /*  Get a record from from disk if the sector key
     *  is already read from the keychain. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Get(const SectorKey& key, std::vector<uint8_t>& vData)
    {
        nBytesRead += static_cast<uint32_t>(key.vKey.size() + vData.size());

        /* Check the cache pool for key first. */
        if(cachePool->Get(key.vKey, vData))
            return true;

        /* Get compact size from record. */
        const uint64_t nSize = GetSizeOfCompactSize(key.nSectorSize);

        /* Resize for proper record length. */
        vData.resize(key.nSectorSize - nSize);

        {
            LOCK(CONFIG.FILE(key.nSectorFile));

            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(key.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(key.nSectorFile, pstream);
            }

            /* Seek to the Sector Position on Disk. */
            pstream->seekg(key.nSectorStart + nSize, std::ios::beg);

            /* Read the State and Size of Sector Header. */
            if(!pstream->read((char*) &vData[0], vData.size()))
            {
                debug::log(0, "SECTOR STREAM: ", pstream->eof() ? "EOF" : pstream->bad() ? "BAD" : pstream->fail() ? "FAIL" : "UNKNOWN");
                return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes read");
            }
        }

        /* Verboe output. */
        if(config::nVerbose >= 5)
            debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));

        return true;
    }


    /*  Update a record on disk. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Update(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
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
        cachePool->Put(key, vKey, vData, false);

        {
            LOCK(CONFIG.FILE(key.nSectorFile));

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(key.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
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

        }

        /* Records flushed indicator. */
        ++nRecordsFlushed;
        nBytesWrote += static_cast<uint32_t>(vData.size());

        /* Verbose output. */
        if(config::nVerbose >= 5)
            debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));

        return true;
    }


    /*  Force a write to disk immediately bypassing write buffers. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Force(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
    {
        if(CONFIG.FLAGS & FLAGS::APPEND || !Update(vKey, vData))
        {
            /* Get current size */
            uint64_t nSize = vData.size() + GetSizeOfCompactSize(vData.size());

            /* Create a new Sector Key. */
            SectorKey key(STATE::READY, vKey, static_cast<uint16_t>(nCurrentFile),
                            nCurrentFileSize, static_cast<uint32_t>(nSize));

            /* Create new file if above current file size. */
            if(nCurrentFileSize > CONFIG.MAX_SECTOR_FILE_SIZE)
            {
                debug::log(4, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                //++nCurrentFile;
                nCurrentFileSize = 0;

                key.nSectorFile  = ++nCurrentFile;
                key.nSectorStart = 0;

                std::ofstream stream
                (
                    debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile),
                    std::ios::out | std::ios::binary | std::ios::trunc
                );
                stream.close();
            }

            {
                LOCK(CONFIG.FILE(nCurrentFile));

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(nCurrentFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
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

            /* Increment the current filesize */
            nCurrentFileSize += static_cast<uint32_t>(nSize);

            /* Records flushed indicator. */
            ++nRecordsFlushed;
            nBytesWrote += static_cast<uint32_t>(nSize);

            /* Assign the Key to Keychain. */
            if(!pSectorKeys->Put(key))
                return debug::error(FUNCTION, "failed to write key to keychain");

            /* Write the data into the memory cache. */
            cachePool->Put(key, vKey, vData, false);

            /* Verboe output. */
            if(config::nVerbose >= 5)
                debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                    " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
        }

        return true;
    }


    /*  Write a record into the cache and disk buffer for flushing to disk. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Put(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
    {
        /* Handle force write mode. */
        if(CONFIG.FLAGS & FLAGS::FORCE)
            return Force(vKey, vData);

        /* Wait if the buffer is full. */
        if(nBufferBytes.load() >= CONFIG.MAX_SECTOR_BUFFER_SIZE)
        {
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || nBufferBytes.load() < CONFIG.MAX_SECTOR_BUFFER_SIZE; });
        }

        /* Add to the write buffer thread. */
        {
            LOCK(BUFFER_MUTEX);

            mapDiskBuffer.emplace(std::make_pair(std::move(vKey), std::move(vData)));
            nBufferBytes += static_cast<uint32_t>(vKey.size() + vData.size());
        }

        /* Notify if buffer was added to. */
        CONDITION.notify_all();

        return true;
    }


    /*  Update a record on disk. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::Delete(const std::vector<uint8_t>& vKey)
    {
        /* Check the keychain for key. */
        SectorKey key;
        if(!pSectorKeys->Get(vKey, key))
            return false;

        /* Return the Key existance in the Keychain Database. */
        if(!pSectorKeys->Erase(vKey))
            return false;

        /* Check that this key isn't a keychain only entry. */
        if(key.nSectorFile ==0 && key.nSectorSize == 0 && key.nSectorStart == 0)
            return true;

        {
            LOCK(CONFIG.FILE(key.nSectorFile));

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(key.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(key.nSectorFile, pstream);
            }

            /* If it is a New Sector, Assign a Binary Position. */
            pstream->seekg(key.nSectorStart, std::ios::beg);

            /* Write the size of record. */
            uint64_t nSize = ReadCompactSize(*pstream);

            /* Seek to write at specific location. */
            pstream->seekp(key.nSectorStart + GetSizeOfCompactSize(nSize), std::ios::beg);

            /* Update the record with blank data. */
            DataStream ssData(SER_LLD, DATABASE_VERSION);
            ssData << std::string("NONE");

            /* Write the data record. */
            if(!pstream->write((char*)ssData.data(), ssData.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), " bytes written");

            /* Flush the rest of the write buffer in stream. */
            pstream->flush();
        }

        return true;
    }


    /*  Flushes periodically data from the cache buffer to disk. */
    template<class KeychainType, class CacheType, class ConfigType>
    void StaticDatabase<KeychainType, CacheType, ConfigType>::CacheWriter()
    {
        /* Wait for initialization. */
        while(!fInitialized)
            runtime::sleep(100);

        /* Check if writing is enabled. */
        if(!(CONFIG.FLAGS & FLAGS::WRITE) && !(CONFIG.FLAGS & FLAGS::APPEND) && !(CONFIG.FLAGS & FLAGS::FORCE))
        {
            debug::log(0, FUNCTION, "Cache Writer is Closing...");
            return;
        }

        /* Loop until shutdown. */
        while(true)
        {
            /* Wait for buffer to empty before shutting down. */
            if((fDestruct.load()) && nBufferBytes.load() == 0)
                return;

            /* Check for data to be written. */
            runtime::timer timer;
            timer.Start();

            /* Wait for thread to wake-up. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait_for(CONDITION_LOCK, std::chrono::milliseconds(100),
                [&]
                {
                    return fDestruct.load() || nBufferBytes.load() > 0 || timer.ElapsedMilliseconds() >= 100;
                }
            );

            /* Run a courtesy flush and sync whether data has been added. */
            pSectorKeys->Flush();

            /* Check for buffered bytes. */
            if(nBufferBytes.load() == 0)
                continue;

            /* Create a new file if the sector file size is over file size limits. */
            if(nCurrentFileSize > CONFIG.MAX_SECTOR_FILE_SIZE)
            {
                debug::log(0, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                /* Iterate the current file and reset current file sie. */
                ++nCurrentFile;
                nCurrentFileSize = 0;

                /* Create a new file for next writes. */
                std::fstream stream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::out | std::ios::binary | std::ios::trunc);
                stream.close();
            }

            uint32_t nRecords = 0;
            do
            {
                /* Iterate through buffer to queue disk writes. */
                BUFFER_MUTEX.lock(); //we can't scope lock because of initializing references here
                const std::vector<uint8_t>& vKey   = mapDiskBuffer.begin()->first;
                const std::vector<uint8_t>& vData  = mapDiskBuffer[vKey];
                BUFFER_MUTEX.unlock();

                /* Get current size */
                uint64_t nSize = vData.size() + GetSizeOfCompactSize(vData.size());

                /* Create a new Sector Key. */
                SectorKey key(STATE::READY, vKey, static_cast<uint16_t>(nCurrentFile),
                                nCurrentFileSize, static_cast<uint32_t>(nSize));

                /* Write the data into the memory cache. */
                cachePool->Put(key, vKey, vData, false);

                {
                    LOCK(CONFIG.FILE(key.nSectorFile));

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(nCurrentFile, pstream) || !pstream)
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::safe_printstr(CONFIG.DIRECTORY, "datachain/_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            continue;
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(nCurrentFile, pstream);
                    }

                    /* If it is a New Sector, Assign a Binary Position. */
                    pstream->seekp(0, std::ios::end);

                    /* Write the size of record. */
                    WriteCompactSize(*pstream, vData.size());

                    /* Write the data record. */
                    if(!pstream->write((char*)&vData[0], vData.size()))
                    {
                        debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes written");
                        continue;
                    }

                    /* Increment the current filesize */
                    nCurrentFileSize = pstream->tellp();

                    /* Records flushed indicator. */
                    ++nRecordsFlushed;
                    nBytesWrote += static_cast<uint32_t>(nSize);

                    /* Verboe output. */
                    if(config::nVerbose >= 5)
                        debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                            " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));

                    pstream->flush();
                }

                /* Assign the Key to Keychain. */
                if(!pSectorKeys->Put(key))
                {
                    debug::error(FUNCTION, "failed to write key to keychain");
                    continue;
                }

                {
                    LOCK(BUFFER_MUTEX);

                    /* Erase the new record and adjust sizes. */
                    mapDiskBuffer.erase(vKey);
                    nRecords = mapDiskBuffer.size();

                    /* Adjust current buffer size. */
                    nBufferBytes -= (vKey.size() + vData.size());
                }
            }
            while(nRecords > 0);

            /* Notify the condition. */
            CONDITION.notify_all();

            /* Verbose logging. */
            debug::log(0, FUNCTION, "Flushed ", nRecordsFlushed.load(),
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
    template<class KeychainType, class CacheType, class ConfigType>
    void StaticDatabase<KeychainType, CacheType, ConfigType>::Meter()
    {
        if(!config::GetBoolArg("-lldmeters", false))
            return;

        runtime::timer TIMER;
        TIMER.Start();

        while(!fDestruct.load())
        {
            runtime::sleep(100);
            if(TIMER.Elapsed() < 30)
                continue;

            /* Write and Read data rates. */
            double WPS = nBytesWrote.load() / (TIMER.Elapsed() * 1024.0);
            double RPS = nBytesRead.load() / (TIMER.Elapsed() * 1024.0);

            /* Check for zero values. */
            if(WPS == 0 && RPS == 0 && nRecordsFlushed.load() == 0)
                continue;

            /* Debug output. */
            debug::log(0,
                ANSI_COLOR_FUNCTION, CONFIG.NAME, " LLD : ", ANSI_COLOR_RESET,
                "Writing ", WPS, " Kb/s | ",
                "Reading ", RPS, " Kb/s | ",
                "Records ", nRecordsFlushed.load());

            TIMER.Reset();
            nBytesWrote.store(0);
            nBytesRead.store(0);
            nRecordsFlushed.store(0);
        }
    }


    /*  Start a database transaction. */
    template<class KeychainType, class CacheType, class ConfigType>
    void StaticDatabase<KeychainType, CacheType, ConfigType>::TxnBegin()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Delete a previous database transaction pointer if applicable. */
        if(pTransaction)
            delete pTransaction;

        /* Create the new Database Transaction Object. */
        pTransaction = new SectorTransaction();
    }


    /*  Write the transaction commitment message. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::TxnCheckpoint()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Check for active transaction. */
        if(!pTransaction)
            return false;

        /* Set commit message into journal. */
        pTransaction->ssJournal << std::string("commit");

        /* Create an append only stream. */
        std::ofstream stream = std::ofstream(debug::safe_printstr(CONFIG.DIRECTORY, "journal.dat"), std::ios::app | std::ios::binary);
        if(!stream.is_open())
            return debug::error(FUNCTION, "failed to open journal file");

        /* Write to the file.  */
        const std::vector<uint8_t>& vBytes = pTransaction->ssJournal.Bytes();
        stream.write((char*)&vBytes[0], vBytes.size());
        stream.close();

        return true;
    }


    /*  Release the transaction checkpoint. */
    template<class KeychainType, class CacheType, class ConfigType>
    void StaticDatabase<KeychainType, CacheType, ConfigType>::TxnRelease()
    {
        LOCK(TRANSACTION_MUTEX);

        /** Delete the previous transaction pointer if applicable. **/
        if(pTransaction)
            delete pTransaction;

        /** Set the transaction pointer to null also acting like a flag **/
        pTransaction = nullptr;

        /* Delete the transaction journal file. */
        std::ofstream stream(debug::safe_printstr(CONFIG.DIRECTORY, "journal.dat"), std::ios::trunc);
        stream.close();
    }


    /*  Commit data from transaction object. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::TxnCommit()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Check that there is a valid transaction to apply to the database. */
        if(!pTransaction)
            return false;

        /* Erase data set to be removed. */
        for(const auto& item : pTransaction->setErasedData)
            if(!pSectorKeys->Erase(item))
                return debug::error(FUNCTION, "failed to erase from keychain");

        /* Commit the sector data. */
        for(const auto& item : pTransaction->mapTransactions)
            if(!Force(item.first, item.second))
                return debug::error(FUNCTION, "failed to commit sector data");

        /* Commit keychain entries. */
        for(const auto& item : pTransaction->setKeychain)
        {
            SectorKey key(STATE::READY, item, 0, 0, 0);
            if(!pSectorKeys->Put(key))
                return debug::error(FUNCTION, "failed to commit to keychain");
        }

        /* Commit the index data. */
        std::map<std::vector<uint8_t>, SectorKey> mapIndex;
        for(const auto& item : pTransaction->mapIndex)
        {
            /* Get the key. */
            SectorKey key;
            if(mapIndex.count(item.second))
                key = mapIndex[item.second];
            else
            {
                if(!pSectorKeys->Get(item.second, key))
                    return debug::error(FUNCTION, "failed to read indexing entry");

                mapIndex[item.second] = key;
            }

            /* Write the new sector key. */
            key.SetKey(item.first);
            if(!pSectorKeys->Put(key))
                return debug::error(FUNCTION, "failed to write indexing entry");
        }

        /* Cleanup the transaction object. */
        delete pTransaction;
        pTransaction = nullptr;

        return true;
    }


    /*  Recover a transaction from the journal. */
    template<class KeychainType, class CacheType, class ConfigType>
    bool StaticDatabase<KeychainType, CacheType, ConfigType>::TxnRecovery()
    {
        /* Create an append only stream. */
        std::ifstream stream(debug::safe_printstr(CONFIG.DIRECTORY, "journal.dat"), std::ios::in | std::ios::out | std::ios::binary);
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

        debug::log(0, FUNCTION, CONFIG.NAME, " transaction journal detected of ", nSize, " bytes");

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
                pTransaction->setKeychain.insert(vKey);

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
                debug::log(0, FUNCTION, CONFIG.NAME, " transaction journal ready to be restored");

                return true;
            }
        }

        return debug::error(FUNCTION, CONFIG.NAME, " transaction journal never reached commit");
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class StaticDatabase<BinaryHashMap, BinaryLRU, Config::Hashmap>;
    //template class StaticDatabase<ShardHashMap,   BinaryLRU>;
    //template class StaticDatabase<BinaryHashMap,  BinaryLFU>;
    //template class StaticDatabase<BinaryHashTree, BinaryLRU>;

}
