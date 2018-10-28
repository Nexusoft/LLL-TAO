/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_HASHMAP_H

#include <LLD/templates/key.h>
#include <LLD/cache/template_lru.h>

#include <algorithm>


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
        mutable Mutex_t KEY_MUTEX;


        /** The String to hold the Disk Location of Database File.
            Each Database File Acts as a New Table as in Conventional Design.
            Key can be any Type, which is how the Database Records are Accessed. **/
        std::string strBaseLocation;


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum cache size for allocated keys. **/
        uint32_t HASHMAP_MAX_CACHE_SZIE;


        /** The MAximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /* Initialized flag (used for cache thread) */
        bool fInitialized;


        /* Keychain stream object. */
        mutable TemplateLRU<uint32_t, std::fstream*>* fileCache;


        /* Total elements in hashmap for quick inserts. */
        mutable std::vector<uint32_t> hashmap;


        /* The cache for recent files. */
        mutable TemplateLRU<uint32_t, std::vector<uint8_t>>* diskCache;


        /* The cache writer thread. */
        Thread_t CacheThread;


    public:

        BinaryHashMap() : HASHMAP_TOTAL_BUCKETS(256 * 256 * 32), HASHMAP_MAX_CACHE_SZIE(10 * 1024), HASHMAP_MAX_KEY_SIZE(128), HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 15), fInitialized(false), fileCache(new TemplateLRU<uint32_t, std::fstream*>()), diskCache(new TemplateLRU<uint32_t, std::vector<uint8_t>>(32)), CacheThread(std::bind(&BinaryHashMap::CacheWriter, this))
        {
            hashmap.resize(HASHMAP_TOTAL_BUCKETS);
        }

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(std::string strBaseLocationIn) : strBaseLocation(strBaseLocationIn), HASHMAP_TOTAL_BUCKETS(256 * 256 * 32), HASHMAP_MAX_CACHE_SZIE(10 * 1024), HASHMAP_MAX_KEY_SIZE(128), HASHMAP_KEY_ALLOCATION(HASHMAP_MAX_KEY_SIZE + 15), fInitialized(false), fileCache(new TemplateLRU<uint32_t, std::fstream*>()), diskCache(new TemplateLRU<uint32_t, std::vector<uint8_t>>(32)), CacheThread(std::bind(&BinaryHashMap::CacheWriter, this))
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
        }


        /** Handle the Assigning of a Map Bucket. **/
        uint32_t GetBucket(const std::vector<uint8_t> vKey) const
        {
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

            return nBucket % HASHMAP_TOTAL_BUCKETS;
        }


        /** Read the Database Keys and File Positions. **/
        void Initialize()
        {
            /* Create directories if they don't exist yet. */
            if(boost::filesystem::create_directories(strBaseLocation))
                printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, strBaseLocation.c_str());

            /* Build the hashmap indexes. */
            std::string index = strprintf("%s_hashmap.index", strBaseLocation.c_str());
            if(!boost::filesystem::exists(index))
            {
                /* Generate empty space for new file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * 4, 0);

                /* Write the new disk index .*/
                std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generation of disk index. */
                printf(FUNCTION "Generated Disk Index of %u bytes\n", __PRETTY_FUNCTION__, vSpace.size());
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
                for(int nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; nBucket++)
                    std::copy((uint8_t*)&vIndex[nBucket * 4], (uint8_t*)&vIndex[nBucket * 4] + 4, (uint8_t*)&hashmap[nBucket]);

                /* Debug output showing loading of disk index. */
                printf(FUNCTION "Loaded Disk Index of %u bytes\n", __PRETTY_FUNCTION__, vIndex.size());
            }

            /* Build the first hashmap index file if it doesn't exist. */
            std::string file = strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), 0u).c_str();
            if(!boost::filesystem::exists(file))
            {
                /* Build a vector with empty bytes to flush to disk. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Flush the empty keychain file to disk. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output showing generating of the hashmap file. */
                printf(FUNCTION "Generated Disk Hash Map %u of %u bytes\n", __PRETTY_FUNCTION__, 0u, vSpace.size());
            }

            /* Load the stream object into the stream LRU cache. */
            fileCache->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));

            /* Set the initialization flag to complete. */
            fInitialized = true;
        }


        /** Get a Record from the Database with Given Key. **/
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            for(int i = hashmap[nBucket]; i >= 0; i--)
            {
                /* Find the file stream for LRU cache. */
                std::fstream* pstream;
                if(!fileCache->Get(i, pstream))
                {
                    /* Set the new stream pointer. */
                    pstream = new std::fstream(strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), i), std::ios::in | std::ios::out | std::ios::binary);

                    /* If file not found add to LRU cache. */
                    fileCache->Put(i, pstream);
                }

                /* Seek to the hashmap index in file. */
                pstream->seekg (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
                pstream->read((char*) &vBucket[0], vBucket.size());

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 15, vBucket.begin() + 15 + vKey.size(), vKey.begin()))
                {
                    /* Deserialie key and return if found. */
                    CDataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    ssKey >> cKey;

                    return true;
                }
            }

            return false;
        }


        /** Add / Update A Record in the Database **/
        bool Put(SectorKey cKey) const
        {
            LOCK(KEY_MUTEX);

            /* Get the assigned bucket for the hashmap. */
            uint32_t nBucket = GetBucket(cKey.vKey);

            /* Get the file binary position. */
            uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

            /* Create a new disk hashmap object in linked list if it doesn't exist. */
            std::string file = strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), hashmap[nBucket]);
            if(!boost::filesystem::exists(file))
            {
                /* Blank vector to write empty space in new disk file. */
                std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                /* Write the blank data to the new file handle. */
                std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output for monitoring new disk maps. */
                printf(FUNCTION "Generated Disk Hash Map %u of %u bytes\n", __PRETTY_FUNCTION__, hashmap[nBucket], vSpace.size());
            }

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(hashmap[nBucket], pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary);

                /* If not in cache, add to the LRU. */
                fileCache->Put(hashmap[nBucket], pstream);
            }

            /* Iterate the linked list value in the hashmap. */
            hashmap[nBucket]++;

            /* Read the State and Size of Sector Header. */
            CDataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << cKey;

            //TODO: This serialization wastes time in copying memory. Write better vector based stream
            std::vector<uint8_t> vData(ssKey.begin(), ssKey.end());
            vData.insert(vData.end(), cKey.vKey.begin(), cKey.vKey.end());

            /* Flush the key file to disk. */
            pstream->seekp (nFilePos, std::ios::beg);
            pstream->write((char*)&vData[0], vData.size());

            return true;
        }


        /* Helper Thread to Batch Write to Disk. */
        void CacheWriter()
        {
            while(!fShutdown)
            {
                /* Wait for Database to Initialize. */
                if(!fInitialized)
                {
                    Sleep(10);

                    continue;
                }

                /* Flush the disk hashmap. */
                std::vector<uint8_t> vDisk;
                for(auto bucket : hashmap)
                    vDisk.insert(vDisk.end(), (uint8_t*)&bucket, (uint8_t*)&bucket + 4);

                /* Create the file handler. */
                std::string file = strprintf("%s_hashmap.index", strBaseLocation.c_str());
                std::fstream stream(file, std::ios::out | std::ios::binary);
                stream.write((char*)&vDisk[0], vDisk.size());
                stream.close();

                //printf(FUNCTION " Flushed %u Index Bytes to Disk\n", __PRETTY_FUNCTION__, vDisk.size());

                Sleep(1000);
            }
        }

        /** Simple Erase for now, not efficient in Data Usage of HD but quick to get erase function working. **/
        bool Erase(const std::vector<uint8_t> vKey)
        {
            LOCK(KEY_MUTEX);

            /* Check for the Key. */
            uint32_t nBucket = GetBucket(vKey);

            return true;
        }
    };
}

#endif
