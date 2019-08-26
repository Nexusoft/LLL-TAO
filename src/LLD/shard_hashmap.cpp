/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/keychain/shard_hashmap.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>
#include <LLD/hash/xxh3.h>

#include <Util/templates/datastream.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <iomanip>

namespace LLD
{

    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    ShardHashMap::ShardHashMap(const std::string& strBaseLocationIn, const uint8_t nFlagsIn,
        const uint64_t nBucketsIn, const uint32_t nShardsIn)
    : KEY_MUTEX()
    , strBaseLocation(strBaseLocationIn)
    , fileCache(new TemplateLRU<std::pair<uint16_t, uint16_t>, std::fstream*>(8))
    , diskShards(new TemplateLRU<uint16_t, std::vector<uint16_t>*>(nShardsIn))
    , indexCache(new TemplateLRU<uint16_t, std::fstream*>(nShardsIn))
    , HASHMAP_TOTAL_BUCKETS(nBucketsIn)
    , HASHMAP_TOTAL_SHARDS(nShardsIn)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(nFlagsIn)
    , RECORD_MUTEX(1024)
    {
        Initialize();
    }


    /* Copy Constructor */
    ShardHashMap::ShardHashMap(const ShardHashMap& map)
    : KEY_MUTEX()
    , strBaseLocation(map.strBaseLocation)
    , fileCache(map.fileCache)
    , diskShards(map.diskShards)
    , indexCache(map.indexCache)
    , HASHMAP_TOTAL_BUCKETS(map.HASHMAP_TOTAL_BUCKETS)
    , HASHMAP_TOTAL_SHARDS(map.HASHMAP_TOTAL_SHARDS)
    , HASHMAP_MAX_KEY_SIZE(map.HASHMAP_MAX_KEY_SIZE)
    , HASHMAP_KEY_ALLOCATION(map.HASHMAP_KEY_ALLOCATION)
    , nFlags(map.nFlags)
    , RECORD_MUTEX(map.RECORD_MUTEX.size())
    {
        Initialize();
    }


    /* Copy Assignment Operator */
    ShardHashMap& ShardHashMap::operator=(const ShardHashMap& map)
    {
        strBaseLocation        = map.strBaseLocation;
        fileCache              = map.fileCache;
        diskShards             = map.diskShards;
        indexCache             = map.indexCache;
        HASHMAP_TOTAL_BUCKETS  = map.HASHMAP_TOTAL_BUCKETS;
        HASHMAP_TOTAL_SHARDS   = map.HASHMAP_TOTAL_SHARDS;
        HASHMAP_MAX_KEY_SIZE   = map.HASHMAP_MAX_KEY_SIZE;
        HASHMAP_KEY_ALLOCATION = map.HASHMAP_KEY_ALLOCATION;
        nFlags                 = map.nFlags;

        Initialize();

        return *this;
    }


    /** Default Destructor **/
    ShardHashMap::~ShardHashMap()
    {
        delete fileCache;
        delete diskShards;
        delete indexCache;
    }


    /*  Compresses a given key until it matches size criteria.
     *  This function is one way and efficient for reducing key sizes. */
    void ShardHashMap::CompressKey(std::vector<uint8_t>& vData, uint16_t nSize)
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


    /*  Calculates a bucket to be used for the hashmap allocation. */
    uint32_t ShardHashMap::GetBucket(const std::vector<uint8_t>& vKey, uint32_t& nShard)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0) / 7;
        nShard = static_cast<uint32_t>(nBucket % HASHMAP_TOTAL_SHARDS);

        /* Get the hashmap bucket. */
        return static_cast<uint32_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    /* Loads a disk index containing shard data into memory.*/
    static uint64_t TOTAL_KEYS = 0;
    void ShardHashMap::LoadShardIndex(const uint32_t nShard)
    {
        /* Check that disk index isn't already loaded. */
        if(diskShards->Has(nShard))
            return;

        /* Build the hashmap indexes. */
        std::string index = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(3), nShard);
        if(!filesystem::exists(index))
        {
            /* Generate empty space for new file. */
            const static std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * 2, 0);

            /* Write the new disk index .*/
            std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Disk Index ", nShard, " of ", vSpace.size(), " bytes");

            /* Set disk index cache to blank values. */
            std::vector<uint16_t>* hashmap = new std::vector<uint16_t>(HASHMAP_TOTAL_BUCKETS, 0);

            /* Add to the LRU cache. */
            diskShards->Put(nShard, hashmap);
        }

        /* Read the hashmap indexes. */
        else
        {
            /* Create the stream index object. */
            std::fstream* pstream;
            if(!indexCache->Get(nShard, pstream))
            {
                pstream = new std::fstream(index, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    throw debug::exception(FUNCTION, "index cache file could not be loaded");
                }

                indexCache->Put(nShard, pstream);
            }

            /* Build a vector to read the disk index. */
            std::vector<uint8_t> vIndex(HASHMAP_TOTAL_BUCKETS * 2, 0);

            /* Read the entire index shard. */
            pstream->read((char*)&vIndex[0], vIndex.size());

            /* Deserialize the values into memory index. */
            uint32_t nTotalKeys = 0;

            /* Create the hashmap vector. */
            std::vector<uint16_t>* hashmap = new std::vector<uint16_t>(HASHMAP_TOTAL_BUCKETS, 0);
            for(uint32_t nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; ++nBucket)
            {
                std::copy((uint8_t *)&vIndex[nBucket * 2], (uint8_t *)&vIndex[nBucket * 2] + 2, (uint8_t *)&hashmap->at(nBucket));

                nTotalKeys += hashmap->at(nBucket);

                TOTAL_KEYS += hashmap->at(nBucket);
            }

            /* Debug output showing loading of disk index. */
            //debug::log(0, FUNCTION, "Loaded Shard ", nShard, " Index of ", vIndex.size(), " bytes and ", nTotalKeys, " keys");

            /* Add to the LRU cache. */
            diskShards->Put(nShard, hashmap);
        }
    }


    /*  Read a key index from the disk hashmaps. */
    void ShardHashMap::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Loop through the available shards. */
        for(uint32_t nShard = 0; nShard < HASHMAP_TOTAL_SHARDS; ++nShard)
        {
            /* Load the hashmap shards. */
            LoadShardIndex(nShard);

            /* Build the first hashmap index file if it doesn't exist. */
            std::string file = debug::safe_printstr(strBaseLocation, "_hashmap.",
                std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), 0u);

            /* Create hashmap for current shard. */
            if(!filesystem::exists(file))
            {
                /* Build a vector with empty bytes to flush to disk. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Flush the empty keychain file to disk. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generating of the hashmap file. */
                debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", vSpace.size(), " bytes");
            }

            /* Load the stream object into the stream LRU cache. */
            std::fstream* pstream = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
                throw debug::exception(FUNCTION, "failed to open ", file);

            fileCache->Put(std::make_pair(nShard, 0), pstream);
        }

        debug::log(0, FUNCTION, "Shard Hashmap Initialized with ", TOTAL_KEYS, " total keys");
    }


    /*  Read a key index from the disk hashmaps. */
    bool ShardHashMap::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nShard  = 0;
        uint32_t nBucket = GetBucket(vKey, nShard);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Get the disk index. */
        std::vector<uint16_t>* hashmap;
        if(!diskShards->Get(nShard, hashmap))
        {
            LoadShardIndex(nShard);
            if(!diskShards->Get(nShard, hashmap))
                return debug::error(FUNCTION, "couldn't get shard index");
        }

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap->at(nBucket) - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(std::make_pair(nShard, i), pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                    std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), i);

                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(std::make_pair(nShard, i), pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

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
                debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", hashmap->at(nBucket) - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart, "\n",
                    HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }
        }

        return false;
    }


    /*  Read a key index from the disk hashmaps.
     *  This method iterates all maps to find all keys. */
    bool ShardHashMap::Get(const std::vector<uint8_t>& vKey, std::vector<SectorKey>& vKeys)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nShard  = 0;
        uint32_t nBucket = GetBucket(vKey, nShard);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Get the disk index. */
        std::vector<uint16_t>* hashmap;
        if(!diskShards->Get(nShard, hashmap))
        {
            LoadShardIndex(nShard);
            if(!diskShards->Get(nShard, hashmap))
                return debug::error(FUNCTION, "couldn't get shard index");
        }

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap->at(nBucket) - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(std::make_pair(nShard, i), pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                    std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), i);

                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(std::make_pair(nShard, i), pstream);
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

                /* Check if the key is in ready state. */
                if(!cKey.Ready())
                    continue;

                /* Assign the binary key. */
                cKey.vKey = vKey;

                /* Add key to return vector. */
                vKeys.push_back(cKey);

                /* Debug Output of Sector Key Information. */
                debug::log(4, FUNCTION, "Found State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", hashmap->at(nBucket) - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart, "\n",
                    HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));
            }
        }

        return (vKeys.size() > 0);
    }


    /*  Write a key to the disk hashmaps. */
    bool ShardHashMap::Put(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nShard  = 0;
        uint32_t nBucket = GetBucket(cKey.vKey, nShard);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Get the disk index. */
        std::vector<uint16_t>* hashmap;
        if(!diskShards->Get(nShard, hashmap))
        {
            LoadShardIndex(nShard);
            if(!diskShards->Get(nShard, hashmap))
                return debug::error(FUNCTION, "couldn't get shard index");
        }

        /* Handle if not in append mode which will update the key. */
        if(!(nFlags & FLAGS::APPEND))
        {
            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int16_t i = hashmap->at(nBucket) - 1; i >= 0; --i)
            {
                /* Find the file stream for LRU cache. */
                std::fstream *pstream;
                if(!fileCache->Get(std::make_pair(nShard, i), pstream))
                {
                    /* Set the new stream pointer. */
                    std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                        std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), i);

                    pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream->is_open())
                    {
                        delete pstream;
                        continue;
                    }

                    /* If file not found add to LRU cache. */
                    fileCache->Put(std::make_pair(nShard, i), pstream);
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

                    /* Handle the disk writing operations. */
                    pstream->seekp (nFilePos, std::ios::beg);
                    pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                    pstream->flush();

                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", hashmap->at(nBucket) - 1,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    return true;
                }
            }
        }

        /* Create a new disk hashmap object in linked list if it doesn't exist. */
        std::string file = debug::safe_printstr(strBaseLocation, "_hashmap.",
            std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), hashmap->at(nBucket));

        if(!filesystem::exists(file))
        {
            /* Blank vector to write empty space in new disk file. */
            std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

            /* Write the blank data to the new file handle. */
            std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            if(!stream)
                return debug::error(FUNCTION, strerror(errno));

            stream.write((char*)&vSpace[0], vSpace.size());
            stream.flush();
            stream.close();

            /* Debug output for monitoring new disk maps. */
            debug::log(0, FUNCTION, "Generated Disk Hash Map ", hashmap->at(nBucket), " in Shard ", nShard, " of ", vSpace.size(), " bytes");

            //debug::log(0, FUNCTION, "Total Space Efficiency is ", (nTotalKeys * 100.0) / (hashmap.size() * nTotalHashmaps),
            //                        " With ", nTotalKeys, " Keys");
        }

        /* Read the State and Size of Sector Header. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << cKey;

        /* Serialize the key into the end of the vector. */
        ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

        /* Find the file stream for LRU cache. */
        std::fstream* pstream;
        if(!fileCache->Get(std::make_pair(nShard, hashmap->at(nBucket)), pstream))
        {
            /* Set the new stream pointer. */
            std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), hashmap->at(nBucket));

            /* Set the new stream pointer. */
            pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream || !pstream->is_open())
            {
                delete pstream;
                return debug::error(FUNCTION, "Failed to generate file object");
            }

            /* If not in cache, add to the LRU. */
            fileCache->Put(std::make_pair(nShard, hashmap->at(nBucket)), pstream);
        }

        /* Flush the key file to disk. */
        pstream->seekp (nFilePos, std::ios::beg);
        pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
        pstream->flush();

        /* Seek to the index position. */
        std::fstream* pindex;
        if(!indexCache->Get(nShard, pindex))
        {
            /* Build the hashmap indexes. */
            std::string index = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(3), nShard);

            /* Create new index file. */
            pindex = new std::fstream(index, std::ios::in | std::ios::out | std::ios::binary);
            if(!pindex->is_open())
            {
                delete pindex;
                return debug::error(FUNCTION, "Failed to generate file object");
            }

            /* If not in cache, add to the LRU. */
            indexCache->Put(nShard, pindex);
        }

        /* Seek to binary position in index file. */
        pindex->seekp((nBucket * 2), std::ios::beg);

        /* Write the index to disk. */
        uint16_t nIndex = ++hashmap->at(nBucket);

        /* Get the bucket data. */
        std::vector<uint8_t> vBucket((uint8_t*)&nIndex, (uint8_t*)&nIndex + 2);

        /* Write the index into hashmap. */
        pindex->write((char*)&vBucket[0], vBucket.size());
        pindex->flush();

        /* Debug Output of Sector Key Information. */
        debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
            " | Length: ", cKey.nLength,
            " | Bucket ", nBucket,
            " | Location: ", nFilePos,
            " | File: ", hashmap->at(nBucket) - 1,
            " | Sector File: ", cKey.nSectorFile,
            " | Sector Size: ", cKey.nSectorSize,
            " | Sector Start: ", cKey.nSectorStart,
            " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

        return true;
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool ShardHashMap::Erase(const std::vector<uint8_t>& vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nShard  = 0;
        uint32_t nBucket = GetBucket(vKey, nShard);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Get the disk index. */
        std::vector<uint16_t>* hashmap;
        if(!diskShards->Get(nShard, hashmap))
        {
            LoadShardIndex(nShard);
            if(!diskShards->Get(nShard, hashmap))
                return debug::error(FUNCTION, "couldn't get shard index");
        }

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap->at(nBucket) - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(std::make_pair(nShard, i), pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                    std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), i);

                /* Set the new stream pointer. */
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return debug::error(FUNCTION, "Failed to generate file object");
                }

                /* If not in cache, add to the LRU. */
                fileCache->Put(std::make_pair(nShard, i), pstream);
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
                std::vector<uint8_t> vEmpty(HASHMAP_KEY_ALLOCATION, 0);
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                debug::log(4, FUNCTION, "Erased State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", hashmap->at(nBucket) - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart,
                    " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        return false;
    }


    /*  Restore an index in the hashmap if it is found. */
    bool ShardHashMap::Restore(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nShard  = 0;
        uint32_t nBucket = GetBucket(vKey, nShard);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Get the disk index. */
        std::vector<uint16_t>* hashmap;
        if(!diskShards->Get(nShard, hashmap))
        {
            LoadShardIndex(nShard);
            if(!diskShards->Get(nShard, hashmap))
                return debug::error(FUNCTION, "couldn't get shard index");
        }

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap->at(nBucket) - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(std::make_pair(nShard, i), pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.",
                    std::setfill('0'), std::setw(3), nShard, ".", std::setfill('0'), std::setw(5), hashmap->at(nBucket));

                /* Set the new stream pointer. */
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                fileCache->Put(std::make_pair(nShard, i), pstream);
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
                debug::log(4, FUNCTION, "Restored State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", hashmap->at(nBucket) - 1,
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
