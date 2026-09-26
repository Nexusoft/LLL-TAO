/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/keychain/hashmap.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>
#include <LLD/hash/xxh3.h>

#include <Util/templates/datastream.h>
#include <Util/include/config.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <cstdio>
#include <filesystem>
#include <iomanip>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace LLD
{

    /* The Database Constructor. To determine file location and the Bytes per Record. */
    BinaryHashMap::BinaryHashMap(const std::string& strBaseLocationIn, const uint8_t nFlagsIn, const uint64_t nBucketsIn)
    : KEY_MUTEX              ( )
    , strBaseLocation        (strBaseLocationIn)
    , fileCache              (new TemplateLRU<uint16_t, std::fstream*>(8))
    , pindex                 (nullptr)
    , hashmap                (nBucketsIn)
    , HASHMAP_TOTAL_BUCKETS  (nBucketsIn)
    , HASHMAP_MAX_KEY_SIZE   (32)
    , HASHMAP_KEY_ALLOCATION (static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags                 (nFlagsIn)
    , setDirtyFiles          ( )
    , fDirectoryDirty        (false)
    , RECORD_MUTEX           (1024)
    {
        Initialize();
    }


    /* Copy Constructor */
    BinaryHashMap::BinaryHashMap(const BinaryHashMap& map)
    : KEY_MUTEX              ( )
    , strBaseLocation        (map.strBaseLocation)
    , fileCache              (map.fileCache)
    , pindex                 (map.pindex)
    , hashmap                (map.hashmap)
    , HASHMAP_TOTAL_BUCKETS  (map.HASHMAP_TOTAL_BUCKETS)
    , HASHMAP_MAX_KEY_SIZE   (map.HASHMAP_MAX_KEY_SIZE)
    , HASHMAP_KEY_ALLOCATION (map.HASHMAP_KEY_ALLOCATION)
    , nFlags                 (map.nFlags)
    , setDirtyFiles          ( )
    , fDirectoryDirty        (false)
    , RECORD_MUTEX           (map.RECORD_MUTEX.size())
    {
        Initialize();
    }


    /* Move Constructor */
    BinaryHashMap::BinaryHashMap(BinaryHashMap&& map)
    : KEY_MUTEX              ( )
    , strBaseLocation        (std::move(map.strBaseLocation))
    , fileCache              (std::move(map.fileCache))
    , pindex                 (std::move(map.pindex))
    , hashmap                (std::move(map.hashmap))
    , HASHMAP_TOTAL_BUCKETS  (std::move(map.HASHMAP_TOTAL_BUCKETS))
    , HASHMAP_MAX_KEY_SIZE   (std::move(map.HASHMAP_MAX_KEY_SIZE))
    , HASHMAP_KEY_ALLOCATION (std::move(map.HASHMAP_KEY_ALLOCATION))
    , nFlags                 (std::move(map.nFlags))
    , setDirtyFiles          (std::move(map.setDirtyFiles))
    , fDirectoryDirty        (map.fDirectoryDirty)
    , RECORD_MUTEX           (map.RECORD_MUTEX.size())
    {
        Initialize();
    }


    /* Copy Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(const BinaryHashMap& map)
    {
        strBaseLocation        = map.strBaseLocation;
        fileCache              = map.fileCache;
        pindex                 = map.pindex;
        hashmap                = map.hashmap;
        HASHMAP_TOTAL_BUCKETS  = map.HASHMAP_TOTAL_BUCKETS;
        HASHMAP_MAX_KEY_SIZE   = map.HASHMAP_MAX_KEY_SIZE;
        HASHMAP_KEY_ALLOCATION = map.HASHMAP_KEY_ALLOCATION;
        nFlags                 = map.nFlags;
        setDirtyFiles.clear();
        fDirectoryDirty        = false;

        Initialize();

        return *this;
    }


    /* Move Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(BinaryHashMap&& map)
    {
        strBaseLocation        = std::move(map.strBaseLocation);
        fileCache              = std::move(map.fileCache);
        pindex                 = std::move(map.pindex);
        hashmap                = std::move(map.hashmap);
        HASHMAP_TOTAL_BUCKETS  = std::move(map.HASHMAP_TOTAL_BUCKETS);
        HASHMAP_MAX_KEY_SIZE   = std::move(map.HASHMAP_MAX_KEY_SIZE);
        HASHMAP_KEY_ALLOCATION = std::move(map.HASHMAP_KEY_ALLOCATION);
        nFlags                 = std::move(map.nFlags);
        setDirtyFiles          = std::move(map.setDirtyFiles);
        fDirectoryDirty        = map.fDirectoryDirty;

        Initialize();

        return *this;
    }


    /* Default Destructor */
    BinaryHashMap::~BinaryHashMap()
    {
        if(fileCache)
            delete fileCache;

        if(pindex)
            delete pindex;
    }


    /*  Compresses a given key until it matches size criteria. */
    void BinaryHashMap::CompressKey(std::vector<uint8_t>& vData, uint16_t nSize)
    {
        /* Loop until key is of desired size. */
        while(vData.size() > nSize)
        {
            /* Loop half of the key to XOR elements. */
            uint64_t nSize2 = (vData.size() >> 1);
            for(uint64_t i = 0; i < nSize2; ++i)
            {
                uint64_t i2 = (i << 1);
                if(i2 < (nSize2 << 1))
                    vData[i] = vData[i] ^ vData[i2];
            }

            /* Resize the container to half its size. */
            vData.resize(std::max(uint16_t(nSize2), nSize));
        }
    }


    /* Calculates a bucket to be used for the hashmap allocation. */
    uint32_t BinaryHashMap::GetBucket(const std::vector<uint8_t>& vKey)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0) / 7;

        return static_cast<uint32_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    /* Read a key index from the disk hashmaps. */
    void BinaryHashMap::Initialize()
    {
        const bool fReadOnly = (nFlags & FLAGS::READONLY);
        if(!fReadOnly)
            fDirectoryDirty = true;
        const auto nOpenMode = std::ios::in | std::ios::binary
            | (fReadOnly ? std::ios::openmode(0) : std::ios::out);

        /* Create directories if they don't exist yet. */
        if(!fReadOnly && !filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Build the hashmap indexes. */
        std::string index = debug::safe_printstr(strBaseLocation, "_hashmap.index");
        if(!fReadOnly && !filesystem::exists(index))
        {
            /* Preserve the on-disk layout without physically writing every zero. */
            const uint64_t nSize = uint64_t(HASHMAP_TOTAL_BUCKETS) * 4;

            /* Write the new disk index .*/
            std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
            if(!stream)
                throw debug::exception(FUNCTION, "failed to create disk index");

            stream.seekp(nSize - 1);
            stream.put('\0');
            stream.flush();
            if(!stream)
            {
                stream.close();
                filesystem::remove(index);
                throw debug::exception(FUNCTION, "failed to initialize disk index");
            }

            stream.close();
            if(!stream)
            {
                filesystem::remove(index);
                throw debug::exception(FUNCTION, "failed to close disk index");
            }

            setDirtyFiles.insert(index);
            fDirectoryDirty = true;

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Disk Index of ", nSize, " bytes");
        }

        /* Read the hashmap indexes. */
        else if(std::filesystem::exists(index))
        {
            /* Build a vector to read the disk index. */
            std::vector<uint8_t> vIndex(HASHMAP_TOTAL_BUCKETS * 2, 0);

            /* Read the disk index bytes. */
            std::fstream stream(index, std::ios::in | std::ios::binary);
            stream.read((char*)&vIndex[0], vIndex.size());
            if(fReadOnly && !stream)
                throw debug::exception(FUNCTION, "cannot read disk index ", index);
            stream.close();

            /* Deserialize the values into memory index. */
            uint32_t nTotalKeys = 0;
            for(uint32_t nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; ++nBucket)
            {
                std::copy((uint8_t *)&vIndex[nBucket * 2], (uint8_t *)&vIndex[nBucket * 2] + 2, (uint8_t *)&hashmap[nBucket]);

                nTotalKeys += hashmap[nBucket];
            }

            /* Debug output showing loading of disk index. */
            debug::log(0, FUNCTION, "Loaded Disk Index of ", vIndex.size(), " bytes and ", nTotalKeys, " keys");
        }

        /* Build the first hashmap index file if it doesn't exist. */
        std::string file = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), 0u);
        if(!fReadOnly && !filesystem::exists(file))
        {
            /* Sparse holes read as zero-filled buckets on supported filesystems. */
            const uint64_t nSize = uint64_t(HASHMAP_TOTAL_BUCKETS) * HASHMAP_KEY_ALLOCATION;

            /* Flush the empty keychain file to disk. */
            std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            if(!stream)
                throw debug::exception(FUNCTION, "failed to create disk hashmap");

            stream.seekp(nSize - 1);
            stream.put('\0');
            stream.flush();
            if(!stream)
            {
                stream.close();
                filesystem::remove(file);
                throw debug::exception(FUNCTION, "failed to initialize disk hashmap");
            }

            stream.close();
            if(!stream)
            {
                filesystem::remove(file);
                throw debug::exception(FUNCTION, "failed to close disk hashmap");
            }

            setDirtyFiles.insert(file);
            fDirectoryDirty = true;

            /* Debug output showing generating of the hashmap file. */
            debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", nSize, " bytes");
        }

        /* Create the stream index object. */
        pindex = new std::fstream(index, nOpenMode);

        /* Load the stream object into the stream LRU cache. */
        fileCache->Put(0, new std::fstream(file, nOpenMode));
    }


    /* Read a key index from the disk hashmaps. */
    bool BinaryHashMap::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        const bool fReadOnly = (nFlags & FLAGS::READONLY);
        const auto nOpenMode = std::ios::in | std::ios::binary
            | (fReadOnly ? std::ios::openmode(0) : std::ios::out);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);

                pstream = new std::fstream(filename, nOpenMode);
                if(!pstream->is_open())
                {
                    delete pstream;
                    if(fReadOnly && std::filesystem::exists(filename))
                        throw debug::exception(FUNCTION, "cannot read disk hashmap ", filename);
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Check that file is open. */
            if(!pstream->is_open())
            {
                const std::string filename =
                    debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);
                pstream->open(filename, nOpenMode);
                if(!pstream->is_open())
                {
                    if(fReadOnly && std::filesystem::exists(filename))
                        throw debug::exception(FUNCTION, "cannot read disk hashmap ", filename);
                    continue;
                }
            }

            /* Seek to the hashmap index in file. */
            pstream->clear();
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());
            if(!*pstream)
            {
                if(fReadOnly)
                    throw debug::exception(FUNCTION, "cannot read disk hashmap bucket");
                continue;
            }

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            {
                /* Deserialie key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;

                /* Check if the key is ready. */
                if(!cKey.Ready())
                    continue;

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }
        }

        return false;
    }


    /* Write a key to the disk hashmaps. */
    bool BinaryHashMap::Put(const SectorKey& cKey)
    {
        if(nFlags & FLAGS::READONLY)
            return false;

        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Handle if not in append mode which will update the key. */
        if(!(nFlags & FLAGS::APPEND))
        {
            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
            {
                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(i, pstream))
                {
                    std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);

                    /* Set the new stream pointer. */
                    pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                            filename, " (", strerror(errno), ")");
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(i, pstream);
                }

                /* Check that file is open. */
                if(!pstream->is_open())
                    pstream->open(debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i), std::ios::in | std::ios::out | std::ios::binary);

                /* Seek to the hashmap index in file. */
                pstream->seekg (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                pstream->read((char*) &vBucket[0], vBucket.size());

                /* Check if this bucket has the key or is in an empty state. */
                if(vBucket[0] == STATE::EMPTY || std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Serialize the key and return if found. */
                    DataStream ssKey(SER_LLD, DATABASE_VERSION);
                    ssKey << cKey;

                    /* Serialize the key into the end of the vector. */
                    ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(i, pstream))
                    {
                        std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);

                        /* Set the new stream pointer. */
                        pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                filename, " (", strerror(errno), ")");
                        }

                        /* If file not found add to LRU cache. */
                        fileCache->Put(i, pstream);
                    }

                    /* Check that file is open. */
                    if(!pstream->is_open())
                        pstream->open(debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i), std::ios::in | std::ios::out | std::ios::binary);


                    /* Handle the disk writing operations. */
                    pstream->seekp (nFilePos, std::ios::beg);
                    pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                    pstream->flush();
                    if(!*pstream)
                        return debug::error(FUNCTION, "failed to flush hashmap file");

                    setDirtyFiles.insert(debug::safe_printstr(
                        strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i));

                    /* Debug Output of Sector Key Information. */
                    if(config::nVerbose >= 4)
                        debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                            " | Length: ", cKey.nLength,
                            " | Bucket ", nBucket,
                            " | Location: ", nFilePos,
                            " | File: ", hashmap[nBucket] - 1,
                            " | Sector File: ", cKey.nSectorFile,
                            " | Sector Size: ", cKey.nSectorSize,
                            " | Sector Start: ", cKey.nSectorStart, "\n",
                            HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    return true;
                }
            }
        }

        /* Create a new disk hashmap object in linked list if it doesn't exist. */
        std::string file = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), hashmap[nBucket]);
        const int64_t nExpectedSize =
            static_cast<int64_t>(HASHMAP_TOTAL_BUCKETS) * HASHMAP_KEY_ALLOCATION;
        if(filesystem::exists(file) && filesystem::size(file) != nExpectedSize
        && !filesystem::remove(file))
            return debug::error(FUNCTION, "failed to remove partial hashmap file");

        if(!filesystem::exists(file))
        {
            /* Extend the new collision file without a full zero-fill write. */
            std::ofstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            if(!stream)
                return debug::error(FUNCTION, strerror(errno));

            stream.seekp(nExpectedSize - 1);
            stream.put('\0');

            stream.flush();
            if(!stream)
            {
                stream.close();
                filesystem::remove(file);
                return debug::error(FUNCTION, "failed to initialize hashmap file");
            }

            stream.close();
            if(!stream)
            {
                filesystem::remove(file);
                return debug::error(FUNCTION, "failed to close hashmap file");
            }

            fDirectoryDirty = true;
        }

        /* Read the State and Size of Sector Header. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << cKey;

        /* Serialize the key into the end of the vector. */
        ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

        /* Find the file stream for LRU cache. */
        std::fstream* pstream;
        if(!fileCache->Get(hashmap[nBucket], pstream))
        {
            /* Set the new stream pointer. */
            pstream = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
            {
                delete pstream;
                return debug::error(FUNCTION, "Failed to generate file object");
            }

            /* If not in cache, add to the LRU. */
            fileCache->Put(hashmap[nBucket], pstream);
        }

        /* Check that file is open. */
        if(!pstream->is_open())
            pstream->open(file, std::ios::in | std::ios::out | std::ios::binary);

        /* Flush the key file to disk. */
        pstream->seekp (nFilePos, std::ios::beg);
        pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
        pstream->flush();
        if(!*pstream)
            return debug::error(FUNCTION, "failed to flush hashmap file");

        setDirtyFiles.insert(file);

        /* Check index file handle is open. */
        if(!pindex->is_open())
            pindex->open(debug::safe_printstr(strBaseLocation, "_hashmap.index"), std::ios::in | std::ios::out | std::ios::binary);

        /* Seek to the index position. */
        pindex->seekp((nBucket * 2), std::ios::beg);

        /* Write the index to disk. */
        uint16_t nIndex = ++hashmap[nBucket];

        /* Get the bucket data. */
        std::vector<uint8_t> vBucket((uint8_t*)&nIndex, (uint8_t*)&nIndex + 2);

        /* Write the index into hashmap. */
        pindex->write((char*)&vBucket[0], vBucket.size());
        pindex->flush();
        if(!*pindex)
            return debug::error(FUNCTION, "failed to flush hashmap index");

        setDirtyFiles.insert(debug::safe_printstr(strBaseLocation, "_hashmap.index"));

        /* Debug Output of Sector Key Information. */
        if(config::nVerbose >= 4)
            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                " | Length: ", cKey.nLength,
                " | Bucket ", nBucket,
                " | Hashmap ", hashmap[nBucket],
                " | Location: ", nFilePos,
                " | File: ", hashmap[nBucket] - 1,
                " | Sector File: ", cKey.nSectorFile,
                " | Sector Size: ", cKey.nSectorSize,
                " | Sector Start: ", cKey.nSectorStart,
                " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

        return true;
    }


    /* Flush all buffers to disk if using ACID transaction. */
    void BinaryHashMap::Flush()
    {
        if(nFlags & FLAGS::READONLY)
            return;

        /* Check index file handle is open. */
        if(!pindex->is_open())
            pindex->open(debug::safe_printstr(strBaseLocation, "_hashmap.index"), std::ios::in | std::ios::out | std::ios::binary);

        /* Flush the index files. */
        pindex->flush();

        /* Iterate the linked list until end. */
        TemplateNode<uint16_t, std::fstream*>* pnode = fileCache->pfirst;
        while(pnode && pnode->pnext)
        {
            /* Flush to disk. */
            pnode->Data->flush();

            /* Set to next. */
            pnode = pnode->pnext;
        }
    }


    /* Preserve unsynced files and directory metadata until a successful sync. */
    void BinaryHashMap::BeginDurabilityTracking()
    {
        LOCK(KEY_MUTEX);
    }


    /* Sync only files touched while applying the current transaction. */
    bool BinaryHashMap::SyncTouchedFiles()
    {
        LOCK(KEY_MUTEX);

        /* Transaction commits also hit empty participants; avoid walking the
         * stream cache when nothing is waiting for a durable sync. */
        if(setDirtyFiles.empty() && !fDirectoryDirty)
            return true;

        /* Push any user-space stream buffers before durable file sync. */
        Flush();

        for(const auto& strPath : setDirtyFiles)
        {
            FILE* stream = std::fopen(strPath.c_str(), "rb+");
            if(!stream)
                return false;

            #ifdef WIN32
            const bool fSynced = (_commit(_fileno(stream)) == 0);
            #else
            bool fSynced = false;
            #if defined(__linux__)
            fSynced = (fdatasync(fileno(stream)) == 0);
            #endif
            if(!fSynced)
                fSynced = (fsync(fileno(stream)) == 0);
            #endif

            if(std::fclose(stream) != 0 || !fSynced)
                return false;
        }

        /* Directory entries only change when a new hashmap file is created.
         * Content updates of existing files do not need a directory chain walk
         * on every commit (the NODE vs RC-25 disk stall regression). */
        if(fDirectoryDirty)
        {
            std::filesystem::path path = std::filesystem::path(strBaseLocation).lexically_normal();
            if(path.has_relative_path() && path.filename().empty())
                path = path.parent_path();

            if(!config::SyncDataDirectoryChain(path.string()))
                return false;
        }

        setDirtyFiles.clear();
        fDirectoryDirty = false;
        return true;
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool BinaryHashMap::Erase(const std::vector<uint8_t> &vKey)
    {
        if(nFlags & FLAGS::READONLY)
            return false;

        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(i, pstream))
            {
                const std::string filename = debug::safe_printstr(
                    strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);

                /* Set the new stream pointer. */
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return debug::error(FUNCTION, "couldn't open hashmap object at: ",
                        filename, " (", strerror(errno), ")");
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Check that file is open. */
            if(!pstream->is_open())
                pstream->open(debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i),
                    std::ios::in | std::ios::out | std::ios::binary);

            if(!pstream->is_open())
                return debug::error(FUNCTION, "couldn't open hashmap object at bucket file ", i);

            /* Seek to the hashmap index in file. */
            pstream->clear();
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());
            if(!*pstream)
                return debug::error(FUNCTION, "failed to read hashmap bucket");

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            {
                /* Deserialize key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                SectorKey cKey;
                ssKey >> cKey;

                /* Seek to the hashmap index in file. */
                pstream->clear();
                pstream->seekp(nFilePos, std::ios::beg);

                /* Write an empty bucket over the erased key. */
                std::vector<uint8_t> vEmpty(HASHMAP_KEY_ALLOCATION, 0);
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();
                if(!*pstream)
                    return debug::error(FUNCTION, "failed to flush hashmap erase");

                setDirtyFiles.insert(debug::safe_printstr(
                    strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i));

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Erased State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        /* Missing keys are already erased; treat that as successful idempotent apply. */
        return true;
    }


    /* Restore an index in the hashmap if it is found. */
    bool BinaryHashMap::Restore(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i),
                  std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg (nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            {
                /* Deserialize key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                SectorKey cKey;
                ssKey >> cKey;

                /* Skip over keys that are already erased. */
                if(cKey.Ready())
                    return true;

                /* Seek to the hashmap index in file. */
                pstream->seekp (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                std::vector<uint8_t> vReady(STATE::READY);
                pstream->write((char*) &vReady[0], vReady.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Restored State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap[nBucket] - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        return false;
    }
}
