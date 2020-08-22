/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/keychain/hashmap.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>
#include <LLD/hash/xxhash.h>
#include <LLD/templates/bloom.h>

#include <Util/templates/datastream.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <iomanip>

namespace LLD
{
    /* The Database Constructor. To determine file location and the Bytes per Record. */
    BinaryHashMap::BinaryHashMap(const Config::Hashmap& config)
    : KEY_MUTEX             ( )
    , CONFIG                (config)
    , INDEX_FILTER_SIZE     (primary_bloom_size() + secondary_bloom_size() + 2)
    , pFileStreams          (new TemplateLRU<uint16_t, std::fstream*>(CONFIG.MAX_HASHMAP_FILE_STREAMS))
    , pindex                (nullptr)
    {
        Initialize();
    }


    /* Copy Constructor */
    BinaryHashMap::BinaryHashMap(const BinaryHashMap& map)
    : KEY_MUTEX              ( )
    , CONFIG                 (map.CONFIG)
    , INDEX_FILTER_SIZE      (map.INDEX_FILTER_SIZE)
    , pFileStreams           (map.pFileStreams)
    , pindex                 (map.pindex)
    {
        Initialize();
    }


    /* Move Constructor */
    BinaryHashMap::BinaryHashMap(BinaryHashMap&& map)
    : KEY_MUTEX              ( )
    , CONFIG                 (std::move(map.CONFIG))
    , INDEX_FILTER_SIZE      (std::move(map.INDEX_FILTER_SIZE))
    , pFileStreams           (std::move(map.pFileStreams))
    , pindex                 (std::move(map.pindex))
    {
        Initialize();
    }


    /* Copy Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(const BinaryHashMap& map)
    {
        pFileStreams   = map.pFileStreams;
        pindex         = map.pindex;

        Initialize();

        return *this;
    }


    /* Move Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(BinaryHashMap&& map)
    {
        pFileStreams   = std::move(map.pFileStreams);
        pindex         = std::move(map.pindex);

        Initialize();

        return *this;
    }


    /* Default Destructor */
    BinaryHashMap::~BinaryHashMap()
    {
        if(pFileStreams)
            delete pFileStreams;

        if(pindex)
            delete pindex;
    }


    /* Read a key index from the disk hashmaps. */
    void BinaryHashMap::Initialize()
    {
        /* Keep track of total hashmaps. */
        uint32_t nTotalHashmaps = 0;

        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(CONFIG.BASE_DIRECTORY + "keychain/") && filesystem::create_directories(CONFIG.BASE_DIRECTORY + "keychain/"))
            debug::log(0, FUNCTION, "Generated Path ", CONFIG.BASE_DIRECTORY);

        /* Build the hashmap indexes. */
        std::string strIndex = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.index");
        if(!filesystem::exists(strIndex))
        {
            /* Generate empty space for new file. */
            std::vector<uint8_t> vSpace(CONFIG.HASHMAP_TOTAL_BUCKETS * INDEX_FILTER_SIZE, 0);

            /* Write the new disk index .*/
            std::fstream stream(strIndex, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Disk Index of ", vSpace.size(), " bytes");
        }

        /* Read the hashmap indexes. */
        else if(!CONFIG.QUICK_INIT)
        {
            /* Deserialize the values into memory index. */
            uint32_t nTotalKeys = 0;

            //TODO: finish this to calculate total number of keys
            std::vector<uint8_t> vIndex(512, 0);

            /* Debug output showing loading of disk index. */
            debug::log(0, FUNCTION, "Loaded Disk Index | ", vIndex.size(), " bytes | ", nTotalKeys, " keys | ", nTotalHashmaps, " hashmaps");
        }

        /* Build the first hashmap index file if it doesn't exist. */
        std::string file = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), 0u);
        if(!filesystem::exists(file))
        {
            /* Build a vector with empty bytes to flush to disk. */
            std::vector<uint8_t> vSpace(CONFIG.HASHMAP_TOTAL_BUCKETS * CONFIG.HASHMAP_KEY_ALLOCATION, 0);

            /* Flush the empty keychain file to disk. */
            std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output showing generating of the hashmap file. */
            debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", vSpace.size(), " bytes");
        }

        /* Create the stream index object. */
        pindex = new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary);

        /* Load the stream object into the stream LRU cache. */
        pFileStreams->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));
    }


    /* Read a key index from the disk hashmaps. */
    bool BinaryHashMap::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed);

        /* Build our buffer based on total linear probes. */
        const uint32_t MAX_LINEAR_PROBES = std::min(CONFIG.HASHMAP_TOTAL_BUCKETS - nBucket, CONFIG.MAX_LINEAR_PROBES);
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * MAX_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        pindex->read((char*)&vBuffer[0], vBuffer.size());

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBuffer, 0);
        //if(!check_hashmap_available(nHashmap, vBuffer, 0))
        //    return debug::error("Hashmap ", nHashmap, " is available, should be blank");

        /* Check the primary bloom filter. */
        if(!check_primary_bloom(vKey, vBuffer, 2))
            return false;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = nHashmap - 1; i >= 0; --i)
        {
            /* Check the secondary bloom filter. */
            if(!check_secondary_bloom(vKey, vBuffer, nHashmap, primary_bloom_size() + 2))
                continue;

            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!pFileStreams->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), i);
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                pFileStreams->Put(i, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            if(!pstream->read((char*) &vBucket[0], vBucket.size()))
                return debug::error(FUNCTION, "only ", pstream->gcount(), "/", vBucket.size(), " bytes read");

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
                    debug::log(0, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", nHashmap - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }
        }

        return debug::error(FUNCTION, "doesn't exist from hashmap ", nHashmap);
    }


    /* Write a key to the disk hashmaps. */
    bool BinaryHashMap::Put(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        compress_key(vKeyCompressed);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * CONFIG.MAX_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        pindex->read((char*)&vBuffer[0], vBuffer.size());

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBuffer, 0);

        /* Handle if not in append mode which will update the key. */
        if(!(CONFIG.FLAGS & FLAGS::APPEND))
        {
            /* Check the primary bloom filter. */
            if(check_primary_bloom(cKey.vKey, vBuffer, 2))
            {
                /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
                std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
                for(int16_t i = nHashmap - 1; i >= 0; --i)
                {
                    /* Check the secondary bloom filter. */
                    if(!check_secondary_bloom(cKey.vKey, vBuffer, nHashmap, primary_bloom_size() + 2))
                        continue;

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!pFileStreams->Get(i, pstream))
                    {
                        std::string filename = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), i);

                        /* Set the new stream pointer. */
                        pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                        if(!pstream->is_open())
                        {
                            delete pstream;
                            return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                filename, " (", strerror(errno), ")");
                        }

                        /* If file not found add to LRU cache. */
                        pFileStreams->Put(i, pstream);
                    }

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
                        if(!pFileStreams->Get(i, pstream))
                        {
                            std::string filename = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), i);

                            /* Set the new stream pointer. */
                            pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                            if(!pstream->is_open())
                            {
                                delete pstream;
                                return debug::error(FUNCTION, "couldn't create hashmap object at: ",
                                    filename, " (", strerror(errno), ")");
                            }

                            /* If file not found add to LRU cache. */
                            pFileStreams->Put(i, pstream);
                        }

                        /* Handle the disk writing operations. */
                        pstream->seekp (nFilePos, std::ios::beg);
                        pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                        pstream->flush();

                        /* Debug Output of Sector Key Information. */
                        if(config::nVerbose >= 4)
                            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                                " | Length: ", cKey.nLength,
                                " | Bucket ", nBucket,
                                " | Location: ", nFilePos,
                                " | File: ", nHashmap - 1,
                                " | Sector File: ", cKey.nSectorFile,
                                " | Sector Size: ", cKey.nSectorSize,
                                " | Sector Start: ", cKey.nSectorStart, "\n",
                                HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                        return true;
                    }
                }
            }
        }

        /* Create a new disk hashmap object in linked list if it doesn't exist. */
        std::string strHashmap = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), nHashmap);
        if(!filesystem::exists(strHashmap))
        {
            /* Blank vector to write empty space in new disk file. */
            std::vector<uint8_t> vSpace(CONFIG.HASHMAP_KEY_ALLOCATION, 0);

            /* Write the blank data to the new file handle. */
            std::ofstream stream(strHashmap, std::ios::out | std::ios::binary | std::ios::app);
            if(!stream)
                return debug::error(FUNCTION, strerror(errno));

            for(uint32_t i = 0; i < CONFIG.HASHMAP_TOTAL_BUCKETS; ++i)
                stream.write((char*)&vSpace[0], vSpace.size());

            //stream.flush();
            stream.close();
        }

        /* Find the file stream for LRU cache. */
        std::fstream* pstream;
        if(!pFileStreams->Get(nHashmap, pstream))
        {
            /* Set the new stream pointer. */
            pstream = new std::fstream(strHashmap, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
            {
                delete pstream;
                return debug::error(FUNCTION, "Failed to generate file object");
            }

            /* If not in cache, add to the LRU. */
            pFileStreams->Put(nHashmap, pstream);
        }

        /* Serialize the key into the end of the vector. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << cKey;
        ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

        /* Update the total files iterator. */
        ++nHashmap;
        set_current_file(nHashmap, vBuffer, 0);

        /* Flush the key file to disk. */
        pstream->seekp (nFilePos, std::ios::beg);
        if(!pstream->write((char*)&ssKey.Bytes()[0], ssKey.size()))
            return debug::error(FUNCTION, "KEYCHAIN: only ", pstream->gcount(), "/", ssKey.size(), " bytes written");

        /* Update the primary and secondary bloom filters. */
        set_primary_bloom(cKey.vKey, vBuffer, 2);
        set_secondary_bloom(cKey.vKey, vBuffer, nHashmap, primary_bloom_size() + 2);

        /* Write updated filters to the index position. */
        pindex->seekp(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        if(!pindex->write((char*)&vBuffer[0], vBuffer.size()))
            return debug::error(FUNCTION, "INDEX: only ", pindex->gcount(), "/", vBuffer.size(), " bytes written");

        /* Flush the buffers to disk. */
        pindex->flush();
        pstream->flush();

        /* Debug Output of Sector Key Information. */
        if(config::nVerbose >= 4)
            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                " | Length: ", cKey.nLength,
                " | Bucket ", nBucket,
                " | Location: ", nFilePos,
                " | File: ", nHashmap - 1,
                " | Sector File: ", cKey.nSectorFile,
                " | Sector Size: ", cKey.nSectorSize,
                " | Sector Start: ", cKey.nSectorStart,
                " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

        return true;
    }


    /* Flush all buffers to disk if using ACID transaction. */
    void BinaryHashMap::Flush()
    {
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool BinaryHashMap::Erase(const std::vector<uint8_t>& vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * CONFIG.MAX_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        pindex->read((char*)&vBuffer[0], vBuffer.size());

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBuffer, 0);

        /* Check the primary bloom filter. */
        if(!check_primary_bloom(vKey, vBuffer, 2))
            return false;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = nHashmap - 1; i >= 0; --i)
        {
            /* Check the secondary bloom filter. */
            if(!check_secondary_bloom(vKey, vBuffer, nHashmap, primary_bloom_size() + 2))
                continue;

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), i),
                  std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                pFileStreams->Put(i, pstream);
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

                /* Seek to the hashmap index in file. */
                pstream->seekp (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                std::vector<uint8_t> vEmpty(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Erased State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", nHashmap - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        return false;
    }


    /* Restore an index in the hashmap if it is found. */
    bool BinaryHashMap::Restore(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * CONFIG.MAX_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        pindex->read((char*)&vBuffer[0], vBuffer.size());

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBuffer, 0);

        /* Check the primary bloom filter. */
        if(!check_primary_bloom(vKey, vBuffer, 2))
            return false;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = nHashmap - 1; i >= 0; --i)
        {
            /* Check the secondary bloom filter. */
            if(!check_secondary_bloom(vKey, vBuffer, nHashmap, primary_bloom_size() + 2))
                continue;

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(CONFIG.BASE_DIRECTORY, "/keychain/_hashmap.", std::setfill('0'), std::setw(5), i),
                  std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                pFileStreams->Put(i, pstream);
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
                        " | File: ", nHashmap - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        return false;
    }


    /*  Compresses a given key until it matches size criteria. */
    void BinaryHashMap::compress_key(std::vector<uint8_t>& vData, uint16_t nSize)
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
    uint32_t BinaryHashMap::get_bucket(const std::vector<uint8_t>& vKey)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH3_64bits_withSeed(&vKey[0], vKey.size(), 0);
        return static_cast<uint32_t>(nBucket % CONFIG.HASHMAP_TOTAL_BUCKETS);
    }


    /* Helper method to grab the current file from the input buffer. */
    uint16_t BinaryHashMap::get_current_file(const std::vector<uint8_t>& vBuffer, const uint32_t nOffset)
    {
        /* Check for buffer overflow. */
        if(nOffset + 2 >= vBuffer.size())
            return 0;

        /* Copy from the buffer at current offset. */
        uint16_t nRet = 0;
        std::copy((uint8_t*)&vBuffer[nOffset], (uint8_t*)&vBuffer[nOffset] + 2, (uint8_t*)&nRet);

        return nRet;
    }


    /* Helper method to set the current file in the input buffer. */
    void BinaryHashMap::set_current_file(const uint16_t nCurrent, std::vector<uint8_t> &vBuffer, const uint32_t nOffset)
    {
        /* Check for buffer overflow. */
        if(nOffset + 2 >= vBuffer.size())
            return;

        /* Copy to the buffer at current offset. */
        std::copy((uint8_t*)&nCurrent, (uint8_t*)&nCurrent + 2, (uint8_t*)&vBuffer[nOffset]);
    }


    /* Helper method to check if a file is unoccupied in input buffer. */
    bool BinaryHashMap::check_hashmap_available(const uint32_t nHashmap, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset)
    {
        /* Calculate initial hashmap index and bit offset. */
        uint32_t nBeginIndex  = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) / 8;
        uint32_t nBeginBit    = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) % 8;

        /* Loop through all bits in a secondary bloom filter. */
        for(uint32_t n = 0; n < CONFIG.SECONDARY_BLOOM_BITS; ++n)
        {
            /* Calculate our offsets based on current bit iterated. */
            uint32_t nIndex  = nBeginIndex + ((nBeginBit + n) / 8);
            uint32_t nBit    = (nBeginBit + n) % 8;

            /* Check for buffer overflow. */
            if(nIndex + nOffset >= vBuffer.size())
                return true;

            /* Check that any bits within the range of this hashmap bitset are not set. */
            if(vBuffer[nIndex + nOffset] & (1 << nBit))
                return false;
        }

        return true;
    }


    /* Helper method to get the total bytes in the secondary bloom filter. */
    uint32_t BinaryHashMap::secondary_bloom_size()
    {
        /* Cache the total bits to calculate size. */
        uint64_t nBits = CONFIG.MAX_HASHMAP_FILES * CONFIG.SECONDARY_BLOOM_BITS;
        return (nBits / 8) + ((nBits % 8 == 0) ? 0 : 1); //we want 1 byte padding here for bits that overflow
    }


    /* Check if a given key is within a secondary bloom filter. */
    bool BinaryHashMap::check_secondary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer,
        const uint32_t nHashmap, const uint32_t nOffset)
    {
        /* Find the starting index and bit offset. */
        uint32_t nBeginIndex  = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) / 8;
        uint32_t nBeginBit    = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) % 8;

        /* Loop through the total k-hashes for this bloom filter. */
        for(uint32_t k = 0; k < CONFIG.SECONDARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket that correlates to this k-hash. */
            uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % CONFIG.SECONDARY_BLOOM_BITS;

            /* Calculate our local bit and index in the buffer. */
            uint32_t nIndex  = nBeginIndex + (nBeginBit + nBucket) / 8;
            uint32_t nBit    = (nBeginBit + nBucket) % 8;

            /* Check for buffer overflow, if triggered flag as false positive so we don't create more cascading errors
             * due to a false negative when bloom filters need to guarantee no false negatives.
             */
            if(nIndex + nOffset >= vBuffer.size())
                return true;

            /* Check this k-hash value bit is not set. */
            if(!(vBuffer[nIndex + nOffset] & (1 << nBit)))
                return false;
        }

        return true;
    }


    /* Set a given key is within a secondary bloom filter. */
    void BinaryHashMap::set_secondary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer,
        const uint32_t nHashmap, const uint32_t nOffset)
    {
        /* Find the starting index and bit offset. */
        uint32_t nBeginIndex  = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) / 8;
        uint32_t nBeginOffset = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) % 8;

        /* Loop through the total k-hashes for this bloom filter. */
        for(uint32_t k = 0; k < CONFIG.SECONDARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket that correlates to this k-hash. */
            uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % CONFIG.SECONDARY_BLOOM_BITS;

            /* Calculate our local bit and index in the buffer. */
            uint32_t nIndex  = nBeginIndex + (nBeginOffset + nBucket) / 8;
            uint32_t nBit    = (nBeginOffset + nBucket) % 8;

            /* Check for buffer overflow. */
            if(nIndex + nOffset >= vBuffer.size())
                return;

            /* Set the bit for this k-hash value. */
            vBuffer[nIndex + nOffset] |= (1 << nBit);
        }
    }


    /* Helper method to get the total bytes in the primary bloom filter. */
    uint32_t BinaryHashMap::primary_bloom_size()
    {
        return (CONFIG.PRIMARY_BLOOM_BITS / 8) + ((CONFIG.PRIMARY_BLOOM_BITS % 8 == 0) ? 0 : 1); //we want 1 byte padding here for bits that overflow
    }


    /* Set a given key is within a primary bloom filter. */
    void BinaryHashMap::set_primary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nOffset)
    {
        /* Loop through the total k-hashes for this bloom filter. */
        for(uint32_t k = 0; k < CONFIG.PRIMARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket that correlates to this k-hash. */
            uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % CONFIG.PRIMARY_BLOOM_BITS;

            /* Calculate our local bit and index in the buffer. */
            uint32_t nIndex  = (nBucket / 8);
            uint32_t nBit    = (nBucket % 8);

            /* Check for buffer overflow. */
            if(nIndex + nOffset >= vBuffer.size())
                return;

            /* Set the bit for this k-hash value. */
            vBuffer[nIndex + nOffset] |= (1 << nBit);
        }
    }


    /* Check if a given key is within a primary bloom filter. */
    bool BinaryHashMap::check_primary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset)
    {
        /* Loop through the total k-hashes for this bloom filter. */
        for(uint32_t k = 0; k < CONFIG.PRIMARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket that correlates to this k-hash. */
            uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % CONFIG.PRIMARY_BLOOM_BITS;

            /* Calculate our local bit and index in the buffer. */
            uint32_t nIndex  = (nBucket / 8);
            uint32_t nBit    = (nBucket % 8);

            /* Check for buffer overflow, if triggered flag as false positive so we don't create more cascading errors
             * due to a false negative when bloom filters need to guarantee no false negatives.
             */
            if(nIndex + nOffset >= vBuffer.size())
                return true;

            /* Check this k-hash value bit is not set. */
            if(!(vBuffer[nIndex + nOffset] & (1 << nBit)))
                return false;
        }

        return true;
    }
}
