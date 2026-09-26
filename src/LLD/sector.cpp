/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lfu.h>
#include <LLD/cache/binary_lru.h>

#include <LLD/keychain/filemap.h>
#include <LLD/keychain/hashmap.h>

#include <TAO/Ledger/include/sync_profile.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/filesystem.h>
#include <Util/include/hex.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <set>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace LLD
{
    namespace
    {
        bool SyncFileHandle(FILE* stream)
        {
            if(!stream)
                return false;

            #ifdef WIN32
            return (_commit(_fileno(stream)) == 0);
            #else
            /* Prefer fdatasync on Linux: content durability without a full
             * metadata flush on every call. */
            #if defined(__linux__)
            if(fdatasync(fileno(stream)) == 0)
                return true;
            #endif
            return (fsync(fileno(stream)) == 0);
            #endif
        }


        bool SyncFile(const std::string& strPath)
        {
            FILE* stream = std::fopen(strPath.c_str(), "rb+");
            if(!stream)
                return false;

            const bool fSynced = SyncFileHandle(stream);
            return (std::fclose(stream) == 0 && fSynced);
        }


        /* Creation-dirty paths must also persist newly created database parents.
         * Content-only commits never enter this directory-chain sync. */
        bool SyncParentDirectory(const std::string& strPath)
        {
            std::filesystem::path path = std::filesystem::path(strPath).lexically_normal();
            if(path.has_relative_path() && path.filename().empty())
                path = path.parent_path();
            else if(!path.empty() && !std::filesystem::is_directory(path))
                path = path.parent_path();

            if(path.empty())
                return false;

            return config::SyncDataDirectoryChain(path.string());
        }

    }



    /* The Database Constructor. To determine file location and the Bytes per Record. */
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
    , fTxnReleaseRequired(false)
    , fJournalDirectoryDirty(true)
    , setPendingSectorFiles( )
    , fSectorDirectoryDirty(true)
    , pSectorKeys(new KeychainType((config::GetDataDir() + strName + "/keychain/"),
          nFlagsIn | ((nFlagsIn & (FLAGS::FORCE | FLAGS::WRITE | FLAGS::APPEND)) ? 0 : FLAGS::READONLY),
          nBucketsIn))
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


    /* Default Destructor */
    template<class KeychainType, class CacheType>
    SectorDatabase<KeychainType, CacheType>::~SectorDatabase()
    {
        fDestruct = true;
        CONDITION.notify_all();

        if(CacheWriterThread.joinable())
            CacheWriterThread.join();

        if(MeterThread.joinable())
            MeterThread.join();

        /* Best-effort durable flush of any pending data files. */
        if(!(nFlags & FLAGS::READONLY) && pSectorKeys)
        {
            for(const uint16_t nSectorFile : setPendingSectorFiles)
            {
                const std::string strPath = debug::safe_printstr(
                    strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nSectorFile);
                SyncFile(strPath);
            }

            if(fSectorDirectoryDirty)
                SyncParentDirectory(strBaseLocation);

            pSectorKeys->SyncTouchedFiles();
            setPendingSectorFiles.clear();
            fSectorDirectoryDirty = false;
        }

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
        if(!(nFlags & FLAGS::READONLY) && nFlags & FLAGS::CREATE
        && !filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Find the most recent append file. */
        while(true)
        {
            /* Get our current file's path. */
            const std::string& strPath =
                debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nCurrentFile);

            /* Build a stream object now. */
            int64_t nSize = -1;
            if(nFlags & FLAGS::READONLY)
            {
                std::error_code error;
                const auto nFileSize = std::filesystem::file_size(strPath, error);
                if(error && error != std::errc::no_such_file_or_directory)
                    throw debug::exception(FUNCTION, "cannot stat sector ", strPath, ": ", error.message());
                if(!error)
                    nSize = static_cast<int64_t>(nFileSize);
            }
            else
                nSize = filesystem::size(strPath);
            if(nSize == -1)
            {
                /* Assign the Current Size and File. */
                if(nCurrentFile > 0)
                    --nCurrentFile;
                else if(!(nFlags & FLAGS::READONLY))
                {
                    /* Create a new file if it doesn't exist. */
                    std::ofstream cStream(strPath, std::ios::binary | std::ios::out | std::ios::trunc);
                    cStream.close();
                    fSectorDirectoryDirty = true;
                }

                break;
            }

            /* Get the Binary Size. */
            nCurrentFileSize = nSize;

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
                SHARED_LOCK(SECTOR_MUTEX);

                /* Use a private read-only stream so that multiple readers can
                 * proceed concurrently under the shared_lock without racing on
                 * the shared fileCache's fstream seek/read state. */
                std::ifstream stream(
                    debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), cKey.nSectorFile),
                    std::ios::in | std::ios::binary);

                if(!stream.is_open())
                    return debug::error(FUNCTION, "couldn't open stream file for read");

                /* Get compact size from record. */
                uint64_t nSize = GetSizeOfCompactSize(cKey.nSectorSize);

                /* Seek to the Sector Position on Disk. */
                stream.seekg(cKey.nSectorStart + nSize, std::ios::beg);

                /* Resize for proper record length. */
                vData.resize(cKey.nSectorSize - nSize);

                /* Read the State and Size of Sector Header. */
                if(!stream.read((char*) &vData[0], vData.size()))
                    return debug::error(FUNCTION, "only ", stream.gcount(), "/", vData.size(), " bytes read");
            }

            /* Add to cache */
            cachePool->Put(cKey, vKey, vData);

            /* Verbose Debug Logging. */
            if(config::nVerbose >= 5)
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
            SHARED_LOCK(SECTOR_MUTEX);

            nBytesRead += static_cast<uint32_t>(cKey.vKey.size() + vData.size());

            /* Check the cache pool for key first. */
            if(cachePool->Get(cKey.vKey, vData))
                return true;

            /* Use a private read-only stream so that multiple readers can
             * proceed concurrently under the shared_lock without racing on
             * the shared fileCache's fstream seek/read state. */
            std::ifstream stream(
                debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), cKey.nSectorFile),
                std::ios::in | std::ios::binary);

            if(!stream.is_open())
                return false;

            /* Get compact size from record. */
            uint64_t nSize = GetSizeOfCompactSize(cKey.nSectorSize);

            /* Seek to the Sector Position on Disk. */
            stream.seekg(cKey.nSectorStart + nSize, std::ios::beg);

            /* Resize for proper record length. */
            vData.resize(cKey.nSectorSize - nSize);

            /* Read the State and Size of Sector Header. */
            if(!stream.read((char*) &vData[0], vData.size()))
                return debug::error(FUNCTION, "only ", stream.gcount(), "/", vData.size(), " bytes read");

            /* Verbose output. */
            if(config::nVerbose >= 5)
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
        cachePool->Put(key, vKey, vData, false);

        {
            WRITE_LOCK(SECTOR_MUTEX);

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(key.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(key.nSectorFile, pstream);
            }

            /* Check stream file is still open. */
            if(!pstream->is_open())
                pstream->open(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);

            /* If it is a New Sector, Assign a Binary Position. */
            setPendingSectorFiles.insert(key.nSectorFile);
            pstream->seekp(key.nSectorStart, std::ios::beg);

            /* Write the size of record. */
            WriteCompactSize(*pstream, vData.size());

            /* Write the data record. */
            if(!pstream->write((char*) &vData[0], vData.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes written");

            pstream->flush();
            if(!*pstream)
                return debug::error(FUNCTION, "failed to flush sector file");

            /* Records flushed indicator. */
            ++nRecordsFlushed;
            nBytesWrote += static_cast<uint32_t>(vData.size());

            /* Verbose output. */
            if(config::nVerbose >= 5)
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
            /* Sector key built inside the lock so that nCurrentFile and
             * nCurrentFileSize are read and updated atomically. */
            SectorKey key;

            {
                WRITE_LOCK(SECTOR_MUTEX);

                /* Create new file if above current file size. */
                if(nCurrentFileSize > MAX_SECTOR_FILE_SIZE)
                {
                    debug::log(4, FUNCTION, "allocating new sector file ", nCurrentFile + 1);

                    ++nCurrentFile;
                    nCurrentFileSize = 0;

                    std::ofstream stream
                    (
                        debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nCurrentFile),
                        std::ios::out | std::ios::binary | std::ios::trunc
                    );
                    stream.close();
                    fSectorDirectoryDirty = true;
                }

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(nCurrentFile, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return false;
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(nCurrentFile, pstream);
                }

                /* Check stream file is still open. */
                if(!pstream->is_open())
                    pstream->open(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nCurrentFile), std::ios::in | std::ios::out | std::ios::binary);

                /* If it is a New Sector, Assign a Binary Position. */
                setPendingSectorFiles.insert(static_cast<uint16_t>(nCurrentFile));
                pstream->seekp(nCurrentFileSize, std::ios::beg);

                /* Write the size of record. */
                WriteCompactSize(*pstream, vData.size());

                /* Write the data record. */
                if(!pstream->write((char*) &vData[0], vData.size()))
                    return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vData.size(), " bytes written");

                pstream->flush();
                if(!*pstream)
                    return debug::error(FUNCTION, "failed to flush sector file");

                /* Get current size */
                const uint64_t nSize =
                    (vData.size() + GetSizeOfCompactSize(vData.size()));

                /* Create the Sector Key while still holding the lock so that
                 * nCurrentFile and nCurrentFileSize are consistent. */
                key = SectorKey(STATE::READY, vKey, static_cast<uint16_t>(nCurrentFile),
                                nCurrentFileSize, static_cast<uint32_t>(nSize));

                /* Increment the current filesize (now inside lock). */
                nCurrentFileSize += static_cast<uint32_t>(nSize);

                /* Records flushed indicator. */
                ++nRecordsFlushed;
                nBytesWrote += static_cast<uint32_t>(nSize);
            }

            /* Assign the Key to Keychain (has its own internal synchronization). */
            if(!pSectorKeys->Put(key))
                return debug::error(FUNCTION, "failed to write key to keychain");

            /* Write the data into the memory cache. */
            cachePool->Put(key, vKey, vData, false);

            /* Verbose output. */
            if(config::nVerbose >= 5)
                debug::log(5, FUNCTION, "Current File: ", key.nSectorFile,
                    " | Current File Size: ", key.nSectorStart, "\n", HexStr(vData.begin(), vData.end(), true));
        }

        return true;
    }


    /*  Write a record into the cache and disk buffer for flushing to disk. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Put(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData)
    {
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


    /*  Update a record on disk. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::Delete(const std::vector<uint8_t>& vKey)
    {
        /* Check the keychain for key. */
        SectorKey key;
        if(!pSectorKeys->Get(vKey, key))
            return false;

        /* Return the Key existance in the Keychain Database. */
        if(!pSectorKeys->Erase(vKey))
            return false;

        /* Check that this key isn't a keychain only entry. */
        if(key.nSectorFile == 0 && key.nSectorSize == 0 && key.nSectorStart == 0)
            return true;

        {
            WRITE_LOCK(SECTOR_MUTEX);

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(key.nSectorFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(key.nSectorFile, pstream);
            }

            /* Check stream file is still open. */
            if(!pstream->is_open())
                pstream->open(debug::safe_printstr(strBaseLocation, "_block.", std::setfill('0'), std::setw(5), key.nSectorFile), std::ios::in | std::ios::out | std::ios::binary);

            /* Seek to write at specific location. */
            setPendingSectorFiles.insert(key.nSectorFile);
            pstream->seekp(key.nSectorStart + GetSizeOfCompactSize(key.nSectorSize), std::ios::beg);

            /* Update the record with blank data. */
            DataStream ssData(SER_LLD, DATABASE_VERSION);
            ssData << std::string("NONE");
            ssData.resize(key.nSectorSize - GetSizeOfCompactSize(key.nSectorSize));

            /* Write the data record. */
            if(!pstream->write((char*)ssData.data(), ssData.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), " bytes written");

            /* Flush the rest of the write buffer in stream. */
            pstream->flush();
            if(!*pstream)
                return debug::error(FUNCTION, "failed to flush deleted sector data");

            /* Verboe output. */
            if(config::nVerbose >= 4)
                debug::log(4, FUNCTION, "DELETE: Current File: ", key.nSectorFile,
                    " | Current File Size: ", key.nSectorStart, "\n");
        }

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

        /* Loop until shutdown. */
        while(!config::fShutdown.load())
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

            /* Iterate through buffer to queue disk writes. */
            for(const auto& vObj : vIndexes)
            {
                /* Force write data. */
                Force(vObj.first, vObj.second);

                /* Set no longer reserved in cache pool. */
                cachePool->Reserve(vObj.first, false);
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
        if(!config::GetBoolArg("-lldmeters", false))
            return;

        runtime::timer TIMER;
        TIMER.Start();

        /* Loop until shutdown. */
        while(!config::fShutdown.load() && !fDestruct.load())
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
                ANSI_COLOR_FUNCTION, strName, " LLD : ", ANSI_COLOR_RESET,
                "Writing ", WPS, " Kb/s | ",
                "Reading ", RPS, " Kb/s | ",
                "Records ", nRecordsFlushed.load());

            TIMER.Reset();
            nBytesWrote.store(0);
            nBytesRead.store(0);
            nRecordsFlushed.store(0);
        }
    }


    /*  Check if a physical database transaction is currently open. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::HasTransaction()
    {
        LOCK(TRANSACTION_MUTEX);

        return (pTransaction != nullptr);
    }


    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::HasPendingTransactionWork()
    {
        LOCK(TRANSACTION_MUTEX);

        return (pTransaction
            && (!pTransaction->mapTransactions.empty()
             || !pTransaction->setKeychain.empty()
             || !pTransaction->mapIndex.empty()
             || !pTransaction->setErasedData.empty()));
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
        fTxnReleaseRequired = false;
    }


    /*  Write the transaction commitment message. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnCheckpoint()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Check for active transaction. */
        if(!pTransaction)
            return false;

        runtime::timer timerCheckpoint;
        runtime::timer timerFsync;
        const bool fProfile = TAO::Ledger::SyncProfile::Enabled();
        if(fProfile)
            timerCheckpoint.Start();

        /* Recovery requires a commit marker from every participant, including empty ones.
         *
         * Journal fsync is the write-ahead commit point. Keychain/sector data
         * files are synced during apply, before any participant's journal
         * can be released. */
        pTransaction->ssJournal << std::string("commit");

        const std::string strJournal =
            debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat");
        if(!filesystem::exists(strJournal))
            fJournalDirectoryDirty = true;

        /* Create an append only stream. */
        FILE* stream = std::fopen(strJournal.c_str(), "ab");
        if(!stream)
            return debug::error(FUNCTION, "failed to open journal file");

        /* Even a failed write or sync may leave journal bytes that abort must discard. */
        fTxnReleaseRequired = true;

        if(fProfile)
            timerFsync.Start();

        /* Write to the file.  */
        const std::vector<uint8_t>& vBytes = pTransaction->ssJournal.Bytes();
        if(std::fwrite(vBytes.data(), 1, vBytes.size(), stream) != vBytes.size()
        || std::fflush(stream) != 0)
        {
            std::fclose(stream);
            return debug::error(FUNCTION, "failed to flush journal file");
        }

        if(!SyncFileHandle(stream))
        {
            std::fclose(stream);
            return debug::error(FUNCTION, "failed to sync journal file");
        }

        if(std::fclose(stream) != 0)
            return debug::error(FUNCTION, "failed to close journal file");

        /* Keep new journal entries dirty until their directory sync succeeds. */
        if(fJournalDirectoryDirty && !SyncParentDirectory(strJournal))
            return debug::error(FUNCTION, "failed to sync journal directory");

        fJournalDirectoryDirty = false;

        if(fProfile)
        {
            timerFsync.Stop();
            timerCheckpoint.Stop();
            TAO::Ledger::SyncProfile::RecordTxnCheckpoint(1, timerCheckpoint.ElapsedMicroseconds(), timerFsync.ElapsedMicroseconds());
        }

        return true;
    }


    /*  Release the transaction checkpoint. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnRelease()
    {
        LOCK(TRANSACTION_MUTEX);

        /** Delete the previous transaction pointer if applicable. **/
        if(pTransaction)
            delete pTransaction;

        /** Set the transaction pointer to null also acting like a flag **/
        pTransaction = nullptr;

        if(!fTxnReleaseRequired)
            return true;

        runtime::timer timerRelease;
        runtime::timer timerFsync;
        const bool fProfile = TAO::Ledger::SyncProfile::Enabled();
        if(fProfile)
        {
            timerRelease.Start();
            timerFsync.Start();
        }

        /* Durably truncate the transaction journal. The file already exists, so
         * only the truncated contents need an fsync — not a full directory chain. */
        const std::string strJournal =
            debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat");
        FILE* stream = std::fopen(strJournal.c_str(), "wb");
        if(!stream)
            return debug::error(FUNCTION, "failed to truncate journal file");

        const bool fSynced = SyncFileHandle(stream);
        if(std::fclose(stream) != 0 || !fSynced)
            return debug::error(FUNCTION, "failed to sync truncated journal file");

        fTxnReleaseRequired = false;
        if(fProfile)
        {
            timerFsync.Stop();
            timerRelease.Stop();
            TAO::Ledger::SyncProfile::RecordTxnRelease(1, timerRelease.ElapsedMicroseconds(), timerFsync.ElapsedMicroseconds());
        }

        return true;
    }


    /*  Commit data from transaction object. */
    template<class KeychainType, class CacheType>
    bool SectorDatabase<KeychainType, CacheType>::TxnCommit()
    {
        LOCK(TRANSACTION_MUTEX);

        /* Check that there is a valid transaction to apply to the database. */
        if(!pTransaction)
            return false;

        const bool fEmpty =
            pTransaction->mapTransactions.empty()
            && pTransaction->setKeychain.empty()
            && pTransaction->mapIndex.empty()
            && pTransaction->setErasedData.empty();

        runtime::timer timerApply;
        runtime::timer timerFsync;
        const bool fProfile = TAO::Ledger::SyncProfile::Enabled();
        if(fProfile && !fEmpty)
            timerApply.Start();

        if(!fEmpty)
        {
            pSectorKeys->BeginDurabilityTracking();

            /* Erase data set to be removed. Erase is idempotent for missing keys
             * and still propagates keychain I/O failures. */
            for(const auto& item : pTransaction->setErasedData)
            {
                if(!pSectorKeys->Erase(item))
                    return debug::error(FUNCTION, "failed to erase from keychain");
            }

            /* Commit the sector data. */
            for(const auto& item : pTransaction->mapTransactions)
            {
                if(!Force(item.first, item.second))
                    return debug::error(FUNCTION, "failed to commit sector data");
            }

            /* Commit keychain entries. */
            for(const auto& item : pTransaction->setKeychain)
            {
                SectorKey cKey(STATE::READY, item, 0, 0, 0);
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
                    /* Check for the new indexing entry. */
                    if(!pSectorKeys->Get(item.second, cKey))
                        return debug::error(FUNCTION, "failed to read indexing entry");

                    mapIndex[item.second] = cKey;
                }

                /* Write the new sector key. */
                cKey.SetKey(item.first);
                if(!pSectorKeys->Put(cKey))
                    return debug::error(FUNCTION, "failed to write indexing entry");
            }
        }

        /* Every participant must make its data durable before the coordinator
         * releases any journals. A prior participant's flush cannot cover this
         * database, even when it was empty. */
        if(fProfile)
            timerFsync.Start();
        {
            WRITE_LOCK(SECTOR_MUTEX);
            for(const uint16_t nSectorFile : setPendingSectorFiles)
            {
                const std::string strPath = debug::safe_printstr(
                    strBaseLocation, "_block.", std::setfill('0'), std::setw(5), nSectorFile);
                if(!SyncFile(strPath))
                    return debug::error(FUNCTION, "failed to sync sector file");
            }

            if(fSectorDirectoryDirty && !SyncParentDirectory(strBaseLocation))
                return debug::error(FUNCTION, "failed to sync sector directory");

            if(!pSectorKeys->SyncTouchedFiles())
                return debug::error(FUNCTION, "failed to sync keychain files");

            setPendingSectorFiles.clear();
            fSectorDirectoryDirty = false;
        }
        if(fProfile)
            timerFsync.Stop();

        /* Cleanup the transaction object. */
        delete pTransaction;
        pTransaction = nullptr;

        if(fProfile && !fEmpty)
        {
            timerApply.Stop();
            TAO::Ledger::SyncProfile::RecordTxnApply(1, timerApply.ElapsedMicroseconds(), timerFsync.ElapsedMicroseconds());
        }

        return true;
    }


    /*  Recover a transaction from the journal. */
    template<class KeychainType, class CacheType>
    RECOVERY SectorDatabase<KeychainType, CacheType>::TxnRecovery()
    {
        const std::string strJournal =
            debug::safe_printstr(config::GetDataDir(), strName, "/journal.dat");

        std::error_code ec;
        const std::filesystem::file_status nStatus = std::filesystem::status(strJournal, ec);
        if(ec)
        {
            if(ec == std::errc::no_such_file_or_directory)
                return RECOVERY::INCOMPLETE;

            return debug::error(FUNCTION, "failed to inspect ", strName, " transaction journal"),
                   RECOVERY::FAILED;
        }

        if(!std::filesystem::exists(nStatus))
            return RECOVERY::INCOMPLETE;

        if(!std::filesystem::is_regular_file(nStatus))
        {
            return debug::error(FUNCTION, strName, " transaction journal is not a regular file"),
                   RECOVERY::FAILED;
        }

        /* Open the journal for reading. */
        std::ifstream stream(strJournal, std::ios::in | std::ios::binary | std::ios::ate);
        if(!stream.is_open())
            return debug::error(FUNCTION, "failed to open ", strName, " transaction journal"),
                   RECOVERY::FAILED;

        /* Get the Binary Size. */
        const std::streampos nStreamSize = stream.tellg();
        if(nStreamSize < 0
        || static_cast<uint64_t>(nStreamSize) > std::numeric_limits<uint32_t>::max())
            return debug::error(FUNCTION, "failed to read ", strName, " transaction journal size"),
                   RECOVERY::FAILED;

        /* Get the data buffer. */
        const uint32_t nSize = static_cast<uint32_t>(nStreamSize);

        /* Check journal size for 0. */
        if(nSize == 0)
            return RECOVERY::INCOMPLETE;

        /* Create buffer to read into. */
        std::vector<uint8_t> vBuffer(nSize, 0);

        /* Read the journal file. */
        stream.seekg (0, std::ios::beg);
        stream.read((char*) &vBuffer[0], vBuffer.size());
        if(!stream)
            return debug::error(FUNCTION, "failed to read ", strName, " transaction journal"),
                   RECOVERY::FAILED;

        stream.close();

        debug::log(0, FUNCTION, strName, " transaction journal detected of ", nSize, " bytes");

        /* Parse into a temporary transaction so failures retain the existing state. */
        std::unique_ptr<SectorTransaction> pRecovery(new SectorTransaction());

        try
        {
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
                    pRecovery->EraseTransaction(vKey);

                    /* Debug output. */
                    debug::log(0, FUNCTION, "erasing key ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
                }
                else if(strType == "key")
                {
                    /* Get the key to write. */
                    std::vector<uint8_t> vKey;
                    ssJournal >> vKey;

                    /* Write the key. */
                    pRecovery->setKeychain.insert(vKey);

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
                    pRecovery->mapTransactions[vKey] = vData;

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
                    pRecovery->mapIndex[vKey] = vIndex;

                    /* Debug output. */
                    debug::log(0, FUNCTION, "indexing key ", HexStr(vKey.begin(), vKey.end()).substr(0, 20));
                }
                else if(strType == "commit")
                {
                    LOCK(TRANSACTION_MUTEX);
                    /* A process restart does not prove directory entries survived
                     * a previous failed sync. Persist ancestry before recovery apply. */
                    if(fJournalDirectoryDirty && !SyncParentDirectory(strJournal))
                        return debug::error(FUNCTION, "failed to sync recovered journal directory"),
                               RECOVERY::FAILED;

                    fJournalDirectoryDirty = false;
                    delete pTransaction;
                    pTransaction = pRecovery.release();
                    fTxnReleaseRequired = true;

                    debug::log(0, FUNCTION, strName, " transaction journal ready to be restored");
                    return RECOVERY::COMPLETE;
                }
                else
                    return debug::error(FUNCTION, strName, " transaction journal contains invalid entry"),
                           RECOVERY::FAILED;
            }
        }
        catch(const std::exception& e)
        {
            return debug::error(FUNCTION, strName, " transaction journal parse failed: ", e.what()),
                   RECOVERY::FAILED;
        }

        /* A valid but uncommitted journal must be discarded before the next transaction. */
        {
            LOCK(TRANSACTION_MUTEX);
            fTxnReleaseRequired = true;
        }

        debug::log(0, FUNCTION, strName, " transaction journal never reached commit");
        return RECOVERY::INCOMPLETE;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class SectorDatabase<BinaryHashMap,  BinaryLRU>;

}
