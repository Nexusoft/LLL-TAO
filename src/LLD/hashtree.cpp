/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Excellence is not a skill. It is an attitude." - Ralph Marston

____________________________________________________________________________________________*/

#include <LLD/keychain/hashtree.h>
#include <LLD/include/enum.h>
#include <LLD/include/version.h>
#include <LLD/hash/xxh3.h>

#include <Util/templates/datastream.h>
#include <Util/include/filesystem.h>
#include <Util/include/debug.h>
#include <Util/include/memory.h>
#include <Util/include/hex.h>

namespace LLD
{

    /** Default Constructor **/
    BinaryHashTree::BinaryHashTree()
    : KEY_MUTEX()
    , strBaseLocation()
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SIZE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(FLAGS::APPEND)
    {
    }


    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    BinaryHashTree::BinaryHashTree(std::string strBaseLocationIn, uint8_t nFlagsIn)
    : KEY_MUTEX()
    , strBaseLocation(strBaseLocationIn)
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SIZE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(nFlagsIn)
    {
        Initialize();
    }


    /** Default Constructor **/
    BinaryHashTree::BinaryHashTree(std::string strBaseLocationIn, uint32_t nTotalBuckets, uint32_t nMaxCacheSize, uint8_t nFlagsIn)
    : KEY_MUTEX()
    , strBaseLocation(strBaseLocationIn)
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , HASHMAP_TOTAL_BUCKETS(nTotalBuckets)
    , HASHMAP_MAX_CACHE_SIZE(nMaxCacheSize)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(nFlagsIn)
    {
        Initialize();
    }


    /** Copy Assignment Operator **/
    BinaryHashTree& BinaryHashTree::operator=(BinaryHashTree map)
    {
        strBaseLocation       = map.strBaseLocation;
        fileCache             = map.fileCache;

        return *this;
    }


    /** Copy Constructor **/
    BinaryHashTree::BinaryHashTree(const BinaryHashTree& map)
    {
        strBaseLocation    = map.strBaseLocation;
        fileCache          = map.fileCache;
    }


    /** Default Destructor **/
    BinaryHashTree::~BinaryHashTree()
    {
        delete fileCache;
    }


    /*  Compresses a given key until it matches size criteria.
     *  This function is one way and efficient for reducing key sizes. */
    void BinaryHashTree::CompressKey(std::vector<uint8_t>& vKey, uint16_t nSize)
    {
        /* Loop until key is of desired size. */
        while(vKey.size() > nSize)
        {
            /* Loop half of the key to XOR elements. */
            for(uint64_t i = 0; i < vKey.size() / 2; ++i)
                if(i * 2 < vKey.size())
                    vKey[i] = vKey[i] ^ vKey[i * 2];

            /* Resize the container to half its size. */
            vKey.resize(std::max(uint16_t(vKey.size() / 2), nSize));
        }
    }


    /*  Placeholder. */
    std::vector< std::vector<uint8_t> > BinaryHashTree::GetKeys()
    {
        std::vector< std::vector<uint8_t> > vKeys;

        return vKeys;
    }


    /*  Calculates a bucket to be used for the hashmap allocation. */
    uint32_t BinaryHashTree::GetBucket(const std::vector<uint8_t>& vKey)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0);

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(HASHMAP_TOTAL_BUCKETS));
    }


    /*  Read a key index from the disk hashmaps. */
    void BinaryHashTree::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Build the first hashmap index file if it doesn't exist. */
        std::string file = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), 0u);
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
    }


    /*  Read a key index from the disk hashmaps. */
    bool BinaryHashTree::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Iterate hashmap collissions with B-Tree. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        /* Get the file to iterate. */
        uint32_t nFile = 1;

        /* Loop until no file found. */
        while(true)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(nFile - 1, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), nFile - 1);

                /* Check for end. */
                if(!filesystem::exists(filename))
                    return false;

                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(nFile - 1, pstream);
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
                    " | File: ", nFile - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart, "\n",
                    HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }

            /* Handle iterating along the tree. */
            else
            {
                /* Check for greater or less than. */
                int32_t nCompare = memory::compare((uint8_t*)&vBucket[0] + 13, (uint8_t*)&vKeyCompressed[0], vKeyCompressed.size());

                /* Set the next bits for the binary tree. */
                nFile <<= 1;
                nFile |= (nCompare > 0);
            }
        }

        return false;
    }


    /*  Write a key to the disk hashmaps. */
    bool BinaryHashTree::Put(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Iterate hashmap collissions with B-Tree. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        /* Check for empty position. */
        std::vector<uint8_t> vEmpty(vKeyCompressed.size(), 0);

        /* Get the file to iterate. */
        uint32_t nFile = 1;

        /* Loop until no file found. */
        while(true)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(nFile - 1, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), nFile - 1);

                /* Create a new disk hashmap object in linked list if it doesn't exist. */
                if(!filesystem::exists(filename))
                {
                    /* Blank vector to write empty space in new disk file. */
                    std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0);

                    /* Write the blank data to the new file handle. */
                    std::fstream stream(filename, std::ios::out | std::ios::binary | std::ios::trunc);
                    if(!stream)
                        return debug::error(FUNCTION, strerror(errno));

                    stream.write((char*)&vSpace[0], vSpace.size());
                    stream.flush();
                    stream.close();

                    /* Debug output for monitoring new disk maps. */
                    debug::log(0, FUNCTION, "Generated Disk Hash Map ", nFile - 1, " of ", vSpace.size(), " bytes");
                }

                /* Create the stream object. */
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(nFile - 1, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin())
            || std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vEmpty.size(), vEmpty.begin())
            || vBucket[0] == STATE::EMPTY)
            {
                /* Serialize the Sector Key into stream. */
                DataStream ssKey(SER_LLD, DATABASE_VERSION);
                ssKey << cKey;

                /* Serialize the key into the end of the vector. */
                ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);
                pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", nFile - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart,
                    " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }

            /* Handle iterating along the tree. */
            else
            {
                /* Check for greater or less than. */
                int32_t nCompare = memory::compare((uint8_t*)&vBucket[0] + 13, (uint8_t*)&vKeyCompressed[0], vKeyCompressed.size());

                /* Set the next bits for the binary tree. */
                nFile <<= 1;
                nFile |= (nCompare > 0);
            }
        }

        return false;
    }


    /*  Erase a key from the disk hashmaps. */
    bool BinaryHashTree::Erase(const std::vector<uint8_t>& vKey)
    {
        LOCK(KEY_MUTEX);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Iterate hashmap collissions with B-Tree. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        /* Get the file to iterate. */
        uint32_t nFile = 1;

        /* Loop until no file found. */
        while(true)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(nFile - 1, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), nFile - 1);

                /* Create a new disk hashmap object in linked list if it doesn't exist. */
                if(!filesystem::exists(filename))
                    return false;

                /* Create the stream object. */
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    continue;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(nFile - 1, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            {
                /* Deserialize key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                SectorKey cKey;
                ssKey >> cKey;

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);

                /* Create empty vector to overwrite keychain. */
                std::vector<uint8_t> vEmpty = { STATE::EMPTY };
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                debug::log(4, FUNCTION, "Erased State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                    " | Length: ", cKey.nLength,
                    " | Bucket ", nBucket,
                    " | Location: ", nFilePos,
                    " | File: ", nFile - 1,
                    " | Sector File: ", cKey.nSectorFile,
                    " | Sector Size: ", cKey.nSectorSize,
                    " | Sector Start: ", cKey.nSectorStart,
                    " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }

            /* Handle iterating along the tree. */
            else
            {
                /* Check for greater or less than. */
                int32_t nCompare = memory::compare((uint8_t*)&vBucket[0] + 13, (uint8_t*)&vKeyCompressed[0], vKeyCompressed.size());

                /* Set the next bits for the binary tree. */
                nFile <<= 1;
                nFile |= (nCompare > 0);
            }
        }

        return false;
    }
}
