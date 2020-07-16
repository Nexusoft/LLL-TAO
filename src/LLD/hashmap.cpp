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
    const uint32_t MAX_PRIMARY_BLOOM_HASHES = 3;

    const uint32_t MAX_SECONDARY_BLOOM_HASHES = 5;

    const uint32_t MAX_HASHMAP_FILES = 64;

    const uint32_t MAX_MODULUS_FILE_LINKING = 3;

    const uint32_t MAX_BLOOM_BUCKETS = (2 * MAX_HASHMAP_FILES * MAX_PRIMARY_BLOOM_HASHES);

    const uint64_t INDEX_BUCKET_ALLOCATION = (MAX_BLOOM_BUCKETS / 8) + MAX_HASHMAP_FILES;

    const uint64_t HASHMAP_KEY_ALLOCATION = (29 + (MAX_MODULUS_FILE_LINKING * 2));

    const uint64_t HASHMAP_MAX_FILE_SIZE = 1024 * 1024; //50 KB hashmap files

    const uint64_t INDEX_MAX_FILE_SIZE = 1024 * 1024 * 4; //50 KB hashmap files

    const uint64_t MAX_INDEXES_PER_FILE = (INDEX_MAX_FILE_SIZE / INDEX_BUCKET_ALLOCATION);

    const uint64_t MAX_BUCKETS_PER_FILE = (HASHMAP_MAX_FILE_SIZE / HASHMAP_KEY_ALLOCATION);


    /*  Compresses a given key until it matches size criteria. */
    void BinaryHashMap::compress_key(std::vector<uint8_t>& vData, uint16_t nSize)
    {
        /* Loop until key is of desired size. */
        XXH128_hash_t nHash = XXH3_128bits_withSeed(&vData[0], vData.size(), 0);

        /* Restructure the data from a single 128 bit hash. */
        vData.resize(16);
        std::copy((uint8_t*)&nHash, (uint8_t*)&nHash + sizeof(nHash), (uint8_t*)&vData[0]);
    }


    /* Calculates a bucket to be used for the hashmap allocation. */
    uint64_t BinaryHashMap::get_bucket(const std::vector<uint8_t>& vKey)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH3_64bits_withSeed(&vKey[0], vKey.size(), 0);
        return static_cast<uint64_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    uint16_t BinaryHashMap::get_index_file(const uint64_t nBucket)
    {
        return (nBucket / MAX_INDEXES_PER_FILE);
    }


    uint16_t BinaryHashMap::get_hashmap_file(const uint64_t nBucket)
    {
        return (nBucket / MAX_BUCKETS_PER_FILE);
    }


    void BinaryHashMap::set_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBloom, const uint64_t nOffset)
    {
        /* Iterate the number of k-kashes for the bloom filter. */
        for(uint32_t k = 0; k < MAX_PRIMARY_BLOOM_HASHES; ++k)
        {
            uint64_t nBucket = (XXH3_64bits_withSeed(&vKey[0], vKey.size(), k) % MAX_BLOOM_BUCKETS);

            /* Set the according bit from the hash function. */
            uint32_t nIndex = (nBucket / 8) + nOffset;
            uint32_t nBit   = (nBucket % 8);

            vBloom[nIndex] |= uint8_t(1 << nBit);
        }
    }

    bool BinaryHashMap::check_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBloom, const uint64_t nOffset)
    {
        /* Iterate the number of k-kashes for the bloom filter. */
        for(uint32_t k = 0; k < MAX_PRIMARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket inside the bloom filter. */
            uint64_t nBucket = (XXH3_64bits_withSeed(&vKey[0], vKey.size(), k) % MAX_BLOOM_BUCKETS);

            /* Set the according bit from the hash function. */
            uint32_t nIndex = (nBucket / 8) + nOffset;
            uint32_t nBit   = (nBucket % 8);

            /* Check that all bloom filter bits are set. */
            if(!(vBloom[nIndex] & uint8_t(1 << nBit)))
                return false;
        }

        return true;
    }


    void BinaryHashMap::set_bloom(const std::vector<uint8_t>& vKey, uint8_t& nBloom)
    {
        /* Iterate the number of k-kashes for the bloom filter. */
        for(uint32_t k = 0; k < MAX_SECONDARY_BLOOM_HASHES; ++k)
        {
            /* Set the according bit from the hash function. */
            nBloom |= uint8_t(1 << (XXH3_64bits_withSeed(&vKey[0], vKey.size(), k) % 8));
        }
    }


    bool BinaryHashMap::check_bloom(const std::vector<uint8_t>& vKey, const uint8_t nBloom)
    {
        /* Iterate the number of k-kashes for the bloom filter. */
        for(uint32_t k = 0; k < MAX_SECONDARY_BLOOM_HASHES; ++k)
        {
            /* Check that all bloom filter bits are set. */
            if(!(nBloom & uint8_t(1 << (XXH3_64bits_withSeed(&vKey[0], vKey.size(), k) % 8))))
                return false;
        }

        return true;
    }

    /* The Database Constructor. To determine file location and the Bytes per Record. */
    BinaryHashMap::BinaryHashMap(const std::string& strBaseLocationIn, const uint8_t nFlagsIn, const uint64_t nBucketsIn)
    : KEY_MUTEX              ( )
    , strBaseLocation        (strBaseLocationIn)
    , pFileStreams           (new TemplateLRU<uint16_t, std::fstream*>(8))
    , pIndexStreams           (new TemplateLRU<uint16_t, std::fstream*>(8))
    , hashmap                (nBucketsIn)
    , HASHMAP_TOTAL_BUCKETS  (nBucketsIn)
    , nFlags                 (nFlagsIn)
    , RECORD_MUTEX           (1024)
    {
        Initialize();
    }


    /* Copy Constructor */
    BinaryHashMap::BinaryHashMap(const BinaryHashMap& map)
    : KEY_MUTEX              ( )
    , strBaseLocation        (map.strBaseLocation)
    , pFileStreams           (map.pFileStreams)
    , pIndexStreams           (map.pFileStreams)
    , hashmap                (map.hashmap)
    , HASHMAP_TOTAL_BUCKETS  (map.HASHMAP_TOTAL_BUCKETS)
    , nFlags                 (map.nFlags)
    , RECORD_MUTEX           (map.RECORD_MUTEX.size())
    {
        Initialize();
    }


    /* Move Constructor */
    BinaryHashMap::BinaryHashMap(BinaryHashMap&& map)
    : KEY_MUTEX              ( )
    , strBaseLocation        (std::move(map.strBaseLocation))
    , pFileStreams           (std::move(map.pFileStreams))
    , pIndexStreams           (std::move(map.pIndexStreams))
    , hashmap                (std::move(map.hashmap))
    , HASHMAP_TOTAL_BUCKETS  (std::move(map.HASHMAP_TOTAL_BUCKETS))
    , nFlags                 (std::move(map.nFlags))
    , RECORD_MUTEX           (map.RECORD_MUTEX.size())
    {
        Initialize();
    }


    /* Copy Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(const BinaryHashMap& map)
    {
        strBaseLocation        = map.strBaseLocation;
        pFileStreams           = map.pFileStreams;
        pIndexStreams           = map.pIndexStreams;
        hashmap                = map.hashmap;
        HASHMAP_TOTAL_BUCKETS  = map.HASHMAP_TOTAL_BUCKETS;
        nFlags                 = map.nFlags;

        Initialize();

        return *this;
    }


    /* Move Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(BinaryHashMap&& map)
    {
        strBaseLocation        = std::move(map.strBaseLocation);
        pFileStreams           = std::move(map.pFileStreams);
        pIndexStreams           = std::move(map.pIndexStreams);
        hashmap                = std::move(map.hashmap);
        HASHMAP_TOTAL_BUCKETS  = std::move(map.HASHMAP_TOTAL_BUCKETS);
        nFlags                 = std::move(map.nFlags);

        Initialize();

        return *this;
    }


    /* Default Destructor */
    BinaryHashMap::~BinaryHashMap()
    {
        if(pFileStreams)
            delete pFileStreams;

        if(pIndexStreams)
            delete pIndexStreams;
    }


    /* Read a key index from the disk hashmaps. */
    void BinaryHashMap::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Build the hashmap indexes. */
        std::string strIndex = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(5), 0u);
        if(!filesystem::exists(strIndex))
        {
            /* Generate empty space for new file. */
            uint32_t nFiles = (HASHMAP_TOTAL_BUCKETS / MAX_INDEXES_PER_FILE);
            std::vector<uint8_t> vSpace(MAX_INDEXES_PER_FILE * INDEX_BUCKET_ALLOCATION , 0);

            /* Check for overflow values. */
            if(HASHMAP_TOTAL_BUCKETS % MAX_INDEXES_PER_FILE > 0)
            {
                debug::log(0, FUNCTION, "Disk Index overflow by ", (HASHMAP_TOTAL_BUCKETS % MAX_INDEXES_PER_FILE), " Indexes");
                ++nFiles;
            }

            /* Generate the multiple indexing files based on max file size. */
            for(uint16_t nFile = 0; nFile < nFiles; ++nFile)
            {
                std::string strNext = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(5), nFile);

                /* Write the new disk index .*/
                std::fstream stream(strNext, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generation of disk index. */
                debug::log(0, FUNCTION, "Generated Disk Index ", nFile, " of ", INDEX_MAX_FILE_SIZE, " bytes");
            }
        }

        /* Build the first hashmap index file if it doesn't exist. */
        std::string strFile = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), 0u);
        if(!filesystem::exists(strFile))
        {
            /* Build a vector with empty bytes to flush to disk. */
            uint32_t nFiles = (HASHMAP_TOTAL_BUCKETS / MAX_BUCKETS_PER_FILE);
            std::vector<uint8_t> vSpace(MAX_BUCKETS_PER_FILE * HASHMAP_KEY_ALLOCATION, 0);

            /* Check for overflow values. */
            if(HASHMAP_TOTAL_BUCKETS % MAX_BUCKETS_PER_FILE > 0)
            {
                debug::log(0, FUNCTION, "Disk Index overflow by ", (HASHMAP_TOTAL_BUCKETS % MAX_BUCKETS_PER_FILE), " Indexes");
                ++nFiles;
            }

            /* Generate the multiple indexing files based on max file size. */
            for(uint16_t nFile = 0; nFile < nFiles; ++nFile)
            {
                std::string strNext = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), nFile);

                /* Flush the empty keychain file to disk. */
                std::fstream stream(strNext, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generating of the hashmap file. */
                debug::log(0, FUNCTION, "Generated Disk Hash Map ", nFile, " of ", HASHMAP_MAX_FILE_SIZE, " bytes");
            }
        }

        /* Create the stream index object. */
        pIndexStreams->Put(0, new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary));

        /* Load the stream object into the stream LRU cache. */
        pFileStreams->Put(0, new std::fstream(strFile, std::ios::in | std::ios::out | std::ios::binary));
    }


    /* Read a key index from the disk hashmaps. */
    bool BinaryHashMap::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint64_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint16_t nIndexFile = get_index_file(nBucket);
        uint32_t nFileBucket = (nBucket - (nIndexFile * MAX_INDEXES_PER_FILE));

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed, 16);

        /* Find the file stream for LRU cache. */
        std::fstream* pindex;
        if(!pIndexStreams->Get(nIndexFile, pindex))
        {
            /* Set the new stream pointer. */
            std::string strIndex = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(5), nIndexFile);

            pindex = new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary);
            if(!pindex->is_open())
            {
                delete pindex;
                return debug::error(FUNCTION, "failed to access index file ", nIndexFile);
            }

            /* If file not found add to LRU cache. */
            pIndexStreams->Put(nIndexFile, pindex);
        }

        pindex->seekg(uint64_t(nFileBucket * INDEX_BUCKET_ALLOCATION), std::ios::beg);

        /* Get the bucket data. */
        uint64_t nBufferSize = std::min(uint64_t(512), uint64_t((INDEX_MAX_FILE_SIZE) - (nFileBucket * INDEX_BUCKET_ALLOCATION)));
        std::vector<uint8_t> vIndex(nBufferSize, 0);

        //debug::log(0, "File ", nIndexFile, " Bucket Offset ", nFileBucket, " Expected size ", nFileBucket * INDEX_BUCKET_ALLOCATION, " Buffer size ", nBufferSize);
        if(!pindex->read((char*)&vIndex[0], vIndex.size()))
            return debug::error(FUNCTION, "only ", pindex->gcount(), "/", vIndex.size(), " bytes written");

        /* Check the primary bloom filter. */
        if(!check_bloom(vKey, vIndex, MAX_HASHMAP_FILES))
            return false;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = MAX_HASHMAP_FILES; i >= 0; --i)
        {
            /* Check for empty hashmap files. */
            if(vIndex[i] == 0)
                continue;

            /* Check the secondary bloom filter. */
            if(!check_bloom(vKey, vIndex[i]))
                continue;

            /* Calculate the current file-id. */
            uint16_t nHashmapFile  = (get_hashmap_file(nBucket) * (i + 1));
            uint32_t nBucketOffset = (nBucket - (nHashmapFile * MAX_BUCKETS_PER_FILE));
            uint64_t nFilePos      = (nBucketOffset * HASHMAP_KEY_ALLOCATION);

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(nHashmapFile, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i);
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                pFileStreams->Put(nHashmapFile, pstream);
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


    /* Modifies a key in the disk hashmaps. */
    bool BinaryHashMap::Modify(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        compress_key(vKeyCompressed, 16);

        /* Handle if not in append mode which will update the key. */
        if(!(nFlags & FLAGS::APPEND))
        {
            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
            {
                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!pFileStreams->Get(i, pstream))
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
                            " | File: ", hashmap[nBucket] - 1,
                            " | Sector File: ", cKey.nSectorFile,
                            " | Sector Size: ", cKey.nSectorSize,
                            " | Sector Start: ", cKey.nSectorStart, "\n",
                            HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    return true;
                }
            }
        }

        return false;
    }


    /* Write a key to the disk hashmaps. */
    bool BinaryHashMap::Put(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint64_t nBucket = get_bucket(cKey.vKey);

        /* Get the file binary position. */
        uint16_t nIndexFile = get_index_file(nBucket);

        uint32_t nFileBucket = (nBucket - (nIndexFile * MAX_INDEXES_PER_FILE));


        //debug::log(0, "File ", nIndexFile, " Bucket Offset ", nFileBucket, " Expected size ", nFileBucket * INDEX_BUCKET_ALLOCATION);

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        compress_key(vKeyCompressed, 16);

        /* Find the file stream for LRU cache. */
        std::fstream* pindex;
        if(!pIndexStreams->Get(nIndexFile, pindex))
        {
            /* Set the new stream pointer. */
            std::string strIndex = debug::safe_printstr(strBaseLocation, "_index.", std::setfill('0'), std::setw(5), nIndexFile);

            pindex = new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary);
            if(!pindex->is_open())
            {
                delete pindex;
                return debug::error(FUNCTION, "failed to access index file ", nIndexFile);
            }

            /* If file not found add to LRU cache. */
            pIndexStreams->Put(nIndexFile, pindex);
        }

        pindex->seekg(uint64_t(nFileBucket * INDEX_BUCKET_ALLOCATION), std::ios::beg);

        /* Get the bucket data. */
        uint64_t nBufferSize = std::min(uint64_t(512), uint64_t((INDEX_MAX_FILE_SIZE) - (nFileBucket * INDEX_BUCKET_ALLOCATION)));
        std::vector<uint8_t> vIndex(nBufferSize, 0);
        pindex->read((char*)&vIndex[0], vIndex.size());

        /* Find the current hashmap for this bucket. */
        uint32_t nHashmap = 0;
        for(nHashmap = 0; nHashmap < MAX_HASHMAP_FILES; ++nHashmap)
        {
            if(vIndex[nHashmap] == 0)
                break;
        }

        /* Calculate the current file-id. */
        uint16_t nHashmapFile  = (get_hashmap_file(nBucket) * (nHashmap + 1));
        uint32_t nBucketOffset = (nBucket - (nHashmapFile * MAX_BUCKETS_PER_FILE));
        uint64_t nFilePos      = (nBucketOffset * HASHMAP_KEY_ALLOCATION);

        /* Create a new disk hashmap object in linked list if it doesn't exist. */
        std::string strHashmap = debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), nHashmapFile);
        if(!filesystem::exists(strHashmap))
        {
            /* Blank vector to write empty space in new disk file. */
            std::vector<uint8_t> vSpace(HASHMAP_KEY_ALLOCATION, 0);

            /* Write the blank data to the new file handle. */
            std::ofstream stream(strHashmap, std::ios::out | std::ios::binary | std::ios::app);
            if(!stream)
                return debug::error(FUNCTION, strerror(errno));

            for(uint32_t i = 0; i < MAX_BUCKETS_PER_FILE; ++i)
                stream.write((char*)&vSpace[0], vSpace.size());

            stream.close();
        }

        /* Read the State and Size of Sector Header. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << cKey;

        /* Serialize the key into the end of the vector. */
        ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

        /* Find the file stream for LRU cache. */
        std::fstream* pstream;
        if(!pFileStreams->Get(nHashmapFile, pstream))
        {
            /* Set the new stream pointer. */
            pstream = new std::fstream(strHashmap, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
            {
                delete pstream;
                return debug::error(FUNCTION, "Failed to generate file object");
            }

            /* If not in cache, add to the LRU. */
            pFileStreams->Put(nHashmapFile, pstream);
        }

        /* Flush the key file to disk. */
        pstream->seekp (nFilePos, std::ios::beg);
        if(!pstream->write((char*)&ssKey.Bytes()[0], ssKey.size()))
            return debug::error(FUNCTION, "only ", pstream->gcount(), "/", ssKey.size(), " bytes written");

        pstream->flush();

        /* Update the primary and secondary bloom filters. */
        set_bloom(cKey.vKey, vIndex, MAX_HASHMAP_FILES);
        set_bloom(cKey.vKey, vIndex[nHashmap]);

        /* Write the new updated bloom filters to disk. */
        pindex->seekp(uint64_t(nFileBucket * INDEX_BUCKET_ALLOCATION), std::ios::beg);
        pindex->write((char*)&vIndex[0], vIndex.size());
        pindex->flush();

        /* Debug Output of Sector Key Information. */
        if(config::nVerbose >= 4)
            debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                " | Length: ", cKey.nLength,
                " | Bucket ", nBucket,
                " | Location: ", nFilePos,
                " | Bloom: ", uint32_t(vIndex[nHashmap]),
                " | File: ", nHashmap,
                " | Sector File: ", cKey.nSectorFile,
                " | Sector Size: ", cKey.nSectorSize,
                " | Sector Start: ", cKey.nSectorStart,
                " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

        return true;
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool BinaryHashMap::Erase(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed, 16);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i),
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
                std::vector<uint8_t> vEmpty(HASHMAP_KEY_ALLOCATION, 0);
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();

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

        return false;
    }


    /* Restore an index in the hashmap if it is found. */
    bool BinaryHashMap::Restore(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        compress_key(vKeyCompressed, 16);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(strBaseLocation, "_hashmap.", std::setfill('0'), std::setw(5), i),
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
