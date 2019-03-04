/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/keychain/hashtree.h>
#include <LLD/templates/key.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>

#include <Util/templates/datastream.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/runtime.h>

#include <functional>

namespace LLD
{

    /** Default Constructor **/
    BinaryHashTree::BinaryHashTree()
    : HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(128)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 11))
    , fInitialized(false)
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>())
    , CacheThread(std::bind(&BinaryHashTree::CacheWriter, this))
    {
        hashmap.resize(HASHMAP_TOTAL_BUCKETS);
    }


    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    BinaryHashTree::BinaryHashTree(std::string strBaseLocationIn)
    : strBaseLocation(strBaseLocationIn)
    , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SZIE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(128)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 11))
    , fInitialized(false)
    , fileCache(new TemplateLRU<uint32_t, std::fstream *>())
    , CacheThread(std::bind(&BinaryHashTree::CacheWriter, this))
    {
        hashmap.resize(HASHMAP_TOTAL_BUCKETS);

        Initialize();
    }


    /** Default Destructor **/
    BinaryHashTree::~BinaryHashTree()
    {
        if(fileCache)
            delete fileCache;
    }


    /*  Handle the Assigning of a Map Bucket. */
    uint32_t BinaryHashTree::GetBucket(const std::vector<uint8_t> &vKey) const
    {
        uint64_t nBucket = 0;
        for(uint32_t i = 0; i < vKey.size() && i < 8; ++i)
            nBucket += vKey[i] << (8 * i);

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(HASHMAP_TOTAL_BUCKETS));
    }


    /*  Read the Database Keys and File Positions. */
    void BinaryHashTree::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Build the hashmap indexes. */
        std::string index = debug::safe_printstr(strBaseLocation, "_hashmap.index");
        if(!filesystem::exists(index))
        {
            /* Generate empty space for new file. */
            std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * 4, 0);

            /* Write the new disk index .*/
            std::fstream stream(index, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Disk Index of ", vSpace.size(), " bytes");
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
            debug::log(0, FUNCTION, "Loaded Disk Index of ", vIndex.size(), " bytes and ", nTotalKeys, " keys");
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
            debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", vSpace.size(), " bytes");
        }

        /* Load the stream object into the stream LRU cache. */
        fileCache->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));

        /* Set the initialization flag to complete. */
        fInitialized = true;
    }


    /*  Read a key index from the disk hashmaps. */
    bool BinaryHashTree::Get(const std::vector<uint8_t> vKey, SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        for(int32_t i = hashmap[nBucket]; i >= 0; --i)
        {
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
            std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 11, vBucket.begin() + 11 + vKey.size(), vKey.begin()))
            {
                /* Deserialie key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;

                return true;
            }
        }

        return false;
    }


    /*  Write a key to the disk hashmaps. */
    bool BinaryHashTree::Put(const SectorKey &cKey) const
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Create a new disk hashmap object in linked list if it doesn't exist. */
        std::string file = debug::strprintf("%s_hashmap.%05u", strBaseLocation.c_str(), hashmap[nBucket]);
        if(!filesystem::exists(file))
        {
            /* Blank vector to write empty space in new disk file. */
            std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

            /* Write the blank data to the new file handle. */
            std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output for monitoring new disk maps. */
            debug::log(0, FUNCTION, "Generated Disk Hash Map ", hashmap[nBucket], " of ", vSpace.size(), " bytes");
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
        ++hashmap[nBucket];

        /* Read the State and Size of Sector Header. */
        DataStream ssKey(SER_LLD, DATABASE_VERSION);
        ssKey << cKey;

        /* Serialize the key into the end of the vector. */
        ssKey.write((char*)&cKey.vKey[0], cKey.vKey.size());

        /* Flush the key file to disk. */
        pstream->seekp (nFilePos, std::ios::beg);
        pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());

        return true;
    }


    /*  Helper Thread to Batch Write to Disk. */
    void BinaryHashTree::CacheWriter()
    {
        while(!config::fShutdown.load())
        {
            /* Wait for Database to Initialize. */
            if(!fInitialized)
            {
                runtime::sleep(10);

                continue;
            }

            /* Flush the disk hashmap. */
            std::vector<uint8_t> vDisk;
            for(auto bucket : hashmap)
                vDisk.insert(vDisk.end(), (uint8_t*)&bucket, (uint8_t*)&bucket + 4);

            /* Create the file handler. */
            std::string file = debug::strprintf("%s_hashmap.index", strBaseLocation.c_str());
            std::fstream stream(file, std::ios::out | std::ios::binary);
            stream.write((char*)&vDisk[0], vDisk.size());
            stream.close();

            //debug::log(0, FUNCTION, "Flushed ", vDisk.size(), " Index Bytes to Disk");

            runtime::sleep(1000);
        }
    }


    /*  Simple Erase for now, not efficient in Data Usage of HD but quick to
     *  get erase function working. */
    bool BinaryHashTree::Erase(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        /* Check for the Key. */
        //uint32_t nBucket = GetBucket(vKey);

        return true;
    }


}
