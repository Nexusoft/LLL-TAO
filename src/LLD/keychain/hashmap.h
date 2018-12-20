/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <LLD/templates/key.h>
#include <LLD/cache/template_lru.h>

#include <iostream>

#include <algorithm>

#include <condition_variable>

#include <atomic>


//TODO: Abstract base class for all keychains
namespace LLD
{


    /** Binary Hash Map
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1)
     *  It uses a linked file list based on index to iterate trhough files and binary Positions
     *  when there is a collision that is found
     *
     **/
    class BinaryHashMap
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::recursive_mutex KEY_MUTEX;


        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;


        /** The string to hold the database location. **/
        std::string strBaseLocation;


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum cache size for allocated keys. **/
        uint32_t HASHMAP_MAX_CACHE_SZIE;


        /** The Maximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /** Initialized flag (used for cache thread) **/
        std::atomic<bool> fCacheActive;


        /** Keychain stream object. **/
        mutable TemplateLRU<uint32_t, std::fstream*>* fileCache;


        /** Total elements in hashmap for quick inserts. **/
        mutable std::vector<uint32_t> hashmap;


        /* The cache writer thread. */
        std::thread CacheThread;


    public:

        BinaryHashMap()
        : HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
        , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 11)
        , fCacheActive(false)
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , CacheThread(std::bind(&BinaryHashMap::CacheWriter, this))
        {
            hashmap.resize(HASHMAP_TOTAL_BUCKETS);
        }

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn)
        : strBaseLocation(strBaseLocationIn)
        , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
        , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 11)
        , fCacheActive(false)
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , CacheThread(std::bind(&BinaryHashMap::CacheWriter, this))
        {
            hashmap.resize(HASHMAP_TOTAL_BUCKETS);

            Initialize();
        }

        BinaryHashMap(std::string strBaseLocationIn, uint32_t nTotalBuckets, uint32_t nMaxCacheSize)
        : strBaseLocation(strBaseLocationIn)
        , HASHMAP_TOTAL_BUCKETS(nTotalBuckets)
        , HASHMAP_MAX_CACHE_SZIE(nMaxCacheSize)
        , HASHMAP_MAX_KEY_SIZE(32)
        , HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 11)
        , fCacheActive(false)
        , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
        , CacheThread(std::bind(&BinaryHashMap::CacheWriter, this))
        {
            hashmap.resize(HASHMAP_TOTAL_BUCKETS);

            Initialize();
        }

        //TODO: cleanup copy constructors
        BinaryHashMap& operator=(BinaryHashMap map)
        {
            strBaseLocation       = map.strBaseLocation;
            fileCache             = map.fileCache;

            return *this;
        }


        BinaryHashMap(const BinaryHashMap& map)
        {
            strBaseLocation    = map.strBaseLocation;
            fileCache          = map.fileCache;
        }


        /** Clean up Memory Usage. **/
        ~BinaryHashMap()
        {
            delete fileCache;
        }


        /** CompressKey
         *
         *  Compresses a given key until it matches size criteria.
         *  This function is one way and efficient for reducing key sizes.
         *
         *  @param[out] vData The binary data of key to compress
         *  @param[in] nSize The desired size of key after compression.
         *
         **/
        void CompressKey(std::vector<uint8_t>& vData, uint16_t nSize = 32)
        {
            /* Loop until key is of desired size. */
            while(vData.size() > nSize)
            {
                /* Loop half of the key to XOR elements. */
                for(int i = 0; i < vData.size() / 2; i ++)
                    if(i * 2 < vData.size())
                        vData[i] ^= vData[i * 2];

                /* Resize the container to half its size. */
                vData.resize(vData.size() / 2);
            }
        }

        /** GetKeys
         *
         *  Placeholder.
         *
         **/
         std::vector< std::vector<uint8_t> > GetKeys()
         {
             
         }


        /** GetBucket
         *
         *  Calculates a bucket to be used for the hashmap allocation
         *
         *  @param[in] vKey The key object to calculate with.
         *
         *  @return The bucket assigned to key
         *
         **/
        uint32_t GetBucket(std::vector<uint8_t> vKey)
        {
            uint32_t nBucket = 0;
            std::copy((uint8_t*)&vKey[0] + (vKey.size() - std::min((uint32_t)4, (uint32_t)vKey.size())), (uint8_t*)&vKey[0] + vKey.size(), (uint8_t*)&nBucket);

            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }


        /** Initialize
         *
         *  Initialize the binary hash map keychain
         *
         **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
                debug::log(0, FUNCTION "Generated Path %s", __PRETTY_FUNCTION__, strBaseLocation.c_str());

            /* Build the hashmap indexes. */
            std::string index = debug::strprintf("%s_hashmap.index", strBaseLocation.c_str());
            if(!filesystem::exists(index))
            {
                /* Generate empty space for new file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * 4, 0);

                /* Write the new disk index .*/
                std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generation of disk index. */
                debug::log(0, FUNCTION "Generated Disk Index of %u bytes", __PRETTY_FUNCTION__, vSpace.size());
            }

            /* Read the hashmap indexes. */
            else
            {
                /* Build a vector to read the disk index. */
                std::vector<uint8_t> vIndex(HASHMAP_TOTAL_BUCKETS * 4, 0);

                /* Read the disk index bytes. */
                std::fstream stream(index, std::ios::in | std::ios::binary);
                stream.read((char*)&vIndex[0], vIndex.size());
                stream.close();

                /* Deserialize the values into memory index. */
                uint32_t nTotalKeys = 0;
                for(int nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; nBucket++)
                {
                    std::copy((uint8_t*)&vIndex[nBucket * 4], (uint8_t*)&vIndex[nBucket * 4] + 4, (uint8_t*)&hashmap[nBucket]);

                    nTotalKeys += hashmap[nBucket];
                }

                /* Debug output showing loading of disk index. */
                debug::log(0, FUNCTION "Loaded Disk Index of %u bytes and %u keys", __PRETTY_FUNCTION__, vIndex.size(), nTotalKeys);
            }

            /* Build the first hashmap index file if it doesn't exist. */
            std::string file = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), 0u).c_str();
            if(!filesystem::exists(file))
            {
                /* Build a vector with empty bytes to flush to disk. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Flush the empty keychain file to disk. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generating of the hashmap file. */
                debug::log(0, FUNCTION "Generated Disk Hash Map %u of %u bytes", __PRETTY_FUNCTION__, 0u, vSpace.size());
            }

            /* Load the stream object into the stream LRU cache. */
            fileCache->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));

            /* Set the initialization flag to complete. */
            fCacheActive = true;

            /* Notify threads it is initialized. */
            CONDITION.notify_all();
        }


        /** Get
         *
         *  Read a key index from the disk hashmaps
         *
         *  @param[in] vKey The binary data of key
         *  @param[out] cKey The key object to return\
         *
         *  @return true if the key was found
         *
         **/
        bool Get(std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Set the cKey return value non compressed. */
            cKey.vKey = vKey;

            /* Compress any keys larger than max size. */
            CompressKey(vKey, HASHMAP_MAX_KEY_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int i = hashmap[nBucket]; i >= 0; i--)
            {
                /* Handle the disk operations. */
                { LOCK(KEY_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(i, pstream))
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i), std::ios::in | std::ios::out | std::ios::binary);

                        /* If file not found add to LRU cache. */
                        fileCache->Put(i, pstream);
                    }

                    /* Seek to the hashmap index in file. */
                    pstream->seekg (nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    pstream->read((char*) &vBucket[0], vBucket.size());
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 11, vBucket.begin() + 11 + vKey.size(), vKey.begin()))
                {
                    /* Deserialie key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;

                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION "State: %s | Length: %u | Bucket %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nBucket, nFilePos, hashmap[nBucket] - 1, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());

                    return true;
                }
            }

            return false;
        }


        /** Get
         *
         *  Read a key index from the disk hashmaps
         *  This method iterates all maps to find all keys.
         *
         *  @param[in] vKey The binary data of key
         *  @param[out] vKeys The list of keys to return
         *
         *  @return true if the key was found
         *
         **/
        bool Get(std::vector<uint8_t> vKey, std::vector<SectorKey>& vKeys)
        {
            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Compress any keys larger than max size. */
            CompressKey(vKey, HASHMAP_MAX_KEY_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            for(int i = hashmap[nBucket]; i >= 0; i--)
            {
                /* Handle the disk operations. */
                { LOCK(KEY_MUTEX);

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream;
                    if(!fileCache->Get(i, pstream))
                    {
                        /* Set the new stream pointer. */
                        pstream = new std::fstream(debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i), std::ios::in | std::ios::out | std::ios::binary);

                        /* If file not found add to LRU cache. */
                        fileCache->Put(i, pstream);
                    }

                    /* Seek to the hashmap index in file. */
                    pstream->seekg (nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    pstream->read((char*) &vBucket[0], vBucket.size());
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 11, vBucket.begin() + 11 + vKey.size(), vKey.begin()))
                {
                    /* Deserialize key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    SectorKey cKey;
                    ssKey >> cKey;

                    /* Add key to return vector. */
                    vKeys.push_back(cKey);

                    /* Debug Output of Sector Key Information. */
                    debug::log(4, FUNCTION "State: %s | Length: %u | Bucket %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nBucket, nFilePos, hashmap[nBucket] - 1, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(vKey.begin(), vKey.end()).c_str());
                }
            }

            return (vKeys.size() > 0);
        }


        /** Put
         *
         *  Write a key to the disk hashmaps
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return true if the key was found
         *
         **/
        bool Put(SectorKey cKey)
        {

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(cKey.vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Compress any keys larger than max size. */
            CompressKey(cKey.vKey, HASHMAP_MAX_KEY_SIZE);

            /* Create a new disk hashmap object in linked list if it doesn't exist. */
            std::string file = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), hashmap[nBucket]);
            if(!filesystem::exists(file))
            {
                /* Blank vector to write empty space in new disk file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Write the blank data to the new file handle. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                if(!stream)
                    return debug::error(FUNCTION "%s", __PRETTY_FUNCTION__, strerror(errno));

                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output for monitoring new disk maps. */
                debug::log(0, FUNCTION "Generated Disk Hash Map %u of %u bytes", __PRETTY_FUNCTION__, hashmap[nBucket], vSpace.size());
            }

            /* Read the State and Size of Sector Header. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << cKey;

            /* Serialize the key into the end of the vector. */
            ssKey.write((char*)&cKey.vKey[0], cKey.vKey.size());

            /* Handle the disk writing operations. */
            { LOCK(KEY_MUTEX);

                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(hashmap[nBucket], pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary);
                    if(!pstream)
                        return debug::error(FUNCTION "Failed to generate file object", __PRETTY_FUNCTION__);

                    /* If not in cache, add to the LRU. */
                    fileCache->Put(hashmap[nBucket], pstream);
                }

                /* Iterate the linked list value in the hashmap. */
                hashmap[nBucket]++;

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);
                pstream->write((char*)&ssKey[0], ssKey.size());
                pstream->flush();

            }

            /* Debug Output of Sector Key Information. */
            debug::log(4, FUNCTION "State: %s | Length: %u | Bucket %u | Location: %u | File: %u | Sector File: %u | Sector Size: %u | Sector Start: %u | Key: %s", __PRETTY_FUNCTION__, cKey.nState == READY ? "Valid" : "Invalid", cKey.nLength, nBucket, nFilePos, hashmap[nBucket] - 1, cKey.nSectorFile, cKey.nSectorSize, cKey.nSectorStart, HexStr(cKey.vKey.begin(), cKey.vKey.end()).c_str());

            /* Signal the cache thread to wake up. */
            fCacheActive = true;
            CONDITION.notify_all();

            return true;
        }


        /* Helper Thread to Batch Write to Disk. */
        void CacheWriter()
        {
            std::mutex CONDITION_MUTEX;
            while(!config::fShutdown)
            {
                /* Wait for Database to Initialize. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait_for(CONDITION_LOCK, std::chrono::milliseconds(1000), [this]{ return fCacheActive.load(); });

                /* Flush the disk hashmap. */
                std::vector<uint8_t> vDisk;
                vDisk.insert(vDisk.end(), (uint8_t*)&hashmap[0], (uint8_t*)&hashmap[0] + (4 * hashmap.size()));

                /* Create the file handler. */
                std::fstream stream(debug::strprintf("%s_hashmap.index", strBaseLocation.c_str()), std::ios::out | std::ios::binary);
                stream.write((char*)&vDisk[0], vDisk.size());
                stream.close();

                fCacheActive = false;
            }
        }


        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            std::unique_lock<std::recursive_mutex>(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);


            //TODO: append an index to the end of keychain saying record was erased.

            return true;
        }
    };
}

#endif
