/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

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
#include <Util/include/hex.h>

namespace LLD
{

    /** Default Constructor **/
    BinaryHashTree::BinaryHashTree()
    : KEY_MUTEX()
    , strBaseLocation()
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , pindex(nullptr)
    , hashmap(256 * 256 * 24)
    , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SIZE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(FLAGS::APPEND)
    {
    }


    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    BinaryHashTree::BinaryHashTree(const std::string& strBaseLocationIn, uint8_t nFlagsIn)
    : KEY_MUTEX()
    , strBaseLocation(strBaseLocationIn)
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , pindex(nullptr)
    , hashmap(256 * 256 * 24)
    , HASHMAP_TOTAL_BUCKETS(256 * 256 * 24)
    , HASHMAP_MAX_CACHE_SIZE(10 * 1024)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(nFlagsIn)
    {
        Initialize();
    }


    /** Default Constructor **/
    BinaryHashTree::BinaryHashTree(const std::string& strBaseLocationIn, const uint32_t nTotalBuckets, const uint32_t nMaxCacheSize,  const uint8_t nFlagsIn)
    : KEY_MUTEX()
    , strBaseLocation(strBaseLocationIn)
    , fileCache(new TemplateLRU<uint32_t, std::fstream*>(8))
    , pindex(nullptr)
    , hashmap(nTotalBuckets)
    , HASHMAP_TOTAL_BUCKETS(nTotalBuckets)
    , HASHMAP_MAX_CACHE_SIZE(nMaxCacheSize)
    , HASHMAP_MAX_KEY_SIZE(32)
    , HASHMAP_KEY_ALLOCATION(static_cast<uint16_t>(HASHMAP_MAX_KEY_SIZE + 13))
    , nFlags(nFlagsIn)
    {
        Initialize();
    }


    /** Copy Assignment Operator **/
    BinaryHashTree& BinaryHashTree::operator=(const BinaryHashTree& map)
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
        delete pindex;
    }


    /*  Compresses a given key until it matches size criteria.
     *  This function is one way and efficient for reducing key sizes. */
    void BinaryHashTree::CompressKey(std::vector<uint8_t>& vData, uint16_t nSize)
    {
        /* Loop until key is of desired size. */
        while(vData.size() > nSize)
        {
            /* Loop half of the key to XOR elements. */
            for(uint64_t i = 0; i < vData.size() / 2; ++i)
                if(i * 2 < vData.size())
                    vData[i] = vData[i] ^ vData[i * 2];

            /* Resize the container to half its size. */
            vData.resize(std::max(uint16_t(vData.size() / 2), nSize));
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

        return static_cast<uint32_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    /*  Read a key index from the disk hashmaps. */
    void BinaryHashTree::Initialize()
    {
        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Build the hashmap indexes. */
        std::string index = debug::safe_printstr(strBaseLocation, "_hashtree.index");
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
            for(uint32_t nBucket = 0; nBucket < HASHMAP_TOTAL_BUCKETS; ++nBucket)
            {
                std::copy((uint8_t *)&vIndex[nBucket * 4], (uint8_t *)&vIndex[nBucket * 4] + 4, (uint8_t *)&hashmap[nBucket]);

                nTotalKeys += hashmap[nBucket];
            }

            /* Debug output showing loading of disk index. */
            debug::log(0, FUNCTION, "Loaded Disk Index of ", vIndex.size(), " bytes and ", nTotalKeys, " keys");
        }

        /* Build the first hashmap index file if it doesn't exist. */
        std::string file = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), 1u);
        if(!filesystem::exists(file))
        {
            /* Build a vector with empty bytes to flush to disk. */
            std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0xff);

            /* Flush the empty keychain file to disk. */
            std::fstream stream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&vSpace[0], vSpace.size());
            stream.close();

            /* Debug output showing generating of the hashmap file. */
            debug::log(0, FUNCTION, "Generated Disk Hash Map 0 of ", vSpace.size(), " bytes");
        }

        /* Create the stream index object. */
        pindex = new std::fstream(index, std::ios::in | std::ios::out | std::ios::binary);

        /* Load the stream object into the stream LRU cache. */
        fileCache->Put(0, new std::fstream(file, std::ios::in | std::ios::out | std::ios::binary));
    }


    template< typename Iter1, typename Iter2 >
    int32_t compare(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2)
    {
        while(begin1 != end1 && begin2 != end2)
        {
            if(*begin1 != *begin2)
            {
                return *begin1 - *begin2;
            }
            ++begin1;
            ++begin2;
        }

        return 0;
    }


    /*  Read a key index from the disk hashmaps. */
    bool BinaryHashTree::Get(const std::vector<uint8_t>& vKey, SectorKey &cKey)
    {
        LOCK(KEY_MUTEX);

        static const std::vector<uint8_t> vBlank(HASHMAP_KEY_ALLOCATION, 0xff);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Set the cKey return value non compressed. */
        cKey.vKey = vKey;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        //PrintHex(vKey.begin(), vKey.end());

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        uint32_t i = 1;
        while(true)
        {
            /* Set the new stream pointer. */
            std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), i);

            /* Check for file. */
            if(!filesystem::exists(filename))
                return false;

            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(i, pstream))
            {
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            //if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            //if(compare(vBucket, vKeyCompressed) == 0)
            int32_t nCompare = compare(vBucket.begin() + 13, vBucket.end(), vKeyCompressed.begin(), vKeyCompressed.end());
            if(nCompare == 0)
            {
                /* Deserialize key and return if found. */
                DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                ssKey >> cKey;

                /* Check if the key is ready. */
                if(!cKey.Ready())
                    return false;

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
            else if(compare(vBucket.begin(), vBucket.end(), vBlank.begin(), vBlank.end()) == 0)
                return false;

            //++i;
            //i += 2;
            i <<= 1;
            if(nCompare > 0)
                i |= 1;
            //i  |= (nCompare < 0);
        }

        return false;
    }


    /*  Write a key to the disk hashmaps. */
    bool BinaryHashTree::Put(const SectorKey& cKey)
    {
        LOCK(KEY_MUTEX);

        static const std::vector<uint8_t> vBlank(HASHMAP_KEY_ALLOCATION, 0xff);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(cKey.vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = cKey.vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        uint32_t i = 1;
        while(true)
        {
            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), i);

                /* Check for file. */
                if(!filesystem::exists(filename))
                {
                    /* Blank vector to write empty space in new disk file. */
                    static const std::vector<uint8_t> vSpace(HASHMAP_TOTAL_BUCKETS * HASHMAP_KEY_ALLOCATION, 0xff);

                    /* Write the blank data to the new file handle. */
                    std::fstream stream(filename, std::ios::out | std::ios::binary | std::ios::trunc);
                    if(!stream)
                        return debug::error(FUNCTION, strerror(errno));

                    stream.write((char*)&vSpace[0], vSpace.size());
                    stream.flush();
                    stream.close();

                    /* Debug output for monitoring new disk maps. */
                    debug::log(0, FUNCTION, "Generated Disk Hash Map ", i, " of ", vSpace.size(), " bytes");
                }

                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());

            /* Check if this bucket has the key */
            //if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            //if(compare(vBucket, vKeyCompressed) == 0)
            int32_t nCompare = compare(vBucket.begin() + 13, vBucket.end(), vKeyCompressed.begin(), vKeyCompressed.end());
            if(nCompare == 0 || compare(vBucket.begin(), vBucket.end(), vBlank.begin(), vBlank.end()) == 0)
            {
                /* Read the State and Size of Sector Header. */
                DataStream ssKey(SER_LLD, DATABASE_VERSION);
                ssKey << cKey;

                /* Serialize the key into the end of the vector. */
                ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);
                pstream->write((char*)&ssKey.Bytes()[0], ssKey.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "State: ", cKey.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", cKey.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", i,
                        " | Sector File: ", cKey.nSectorFile,
                        " | Sector Size: ", cKey.nSectorSize,
                        " | Sector Start: ", cKey.nSectorStart, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }

            //++i;
            //i += 2;
            //i <<= 1;
            //i  |= (nCompare < 0);

            i <<= 1;
            if(nCompare > 0)
                i |= 1;
        }

        return true;
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool BinaryHashTree::Erase(const std::vector<uint8_t> &vKey)
    {
        LOCK(KEY_MUTEX);

        static const std::vector<uint8_t> vBlank(HASHMAP_KEY_ALLOCATION, 0xff);

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = GetBucket(vKey);

        /* Get the file binary position. */
        uint32_t nFilePos = nBucket * HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = vKey;
        CompressKey(vKeyCompressed, HASHMAP_MAX_KEY_SIZE);

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(HASHMAP_KEY_ALLOCATION, 0);

        uint32_t i = 1;
        while(true)
        {
            /* Set the new stream pointer. */
            std::string filename = debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), i);

            /* Check for file. */
            if(!filesystem::exists(filename))
                return debug::error(filename, " not found");

            /* Find the file stream for LRU cache. */
            std::fstream *pstream;
            if(!fileCache->Get(i, pstream))
            {
                pstream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary);
                if(!pstream->is_open())
                {
                    delete pstream;
                    return false;
                }

                /* If file not found add to LRU cache. */
                fileCache->Put(i, pstream);
            }

            /* Seek to the hashmap index in file. */
            pstream->seekg(nFilePos, std::ios::beg);

            /* Read the bucket binary data from file stream */
            pstream->read((char*) &vBucket[0], vBucket.size());


            /* Check if this bucket has the key */
            //if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
            //if(compare(vBucket, vKeyCompressed) == 0)
            int32_t nCompare = compare(vBucket.begin() + 13, vBucket.end(), vKeyCompressed.begin(), vKeyCompressed.end());
            if(nCompare == 0)
            {
                /* Set the key state to  empty. */
                vBucket[0] = STATE::EMPTY;

                /* Flush the key file to disk. */
                pstream->seekp (nFilePos, std::ios::beg);
                pstream->write((char*)&vBucket[0], vBucket.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", i, "\n",
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                return true;
            }
            else if(compare(vBucket.begin(), vBucket.end(), vBlank.begin(), vBlank.end()) == 0)
                return false;

            //++i;
            //i += 2;
            i <<= 1;
            i  |= (nCompare < 0);
        }

        return false;
    }


    /*  Restore an index in the hashmap if it is found. */
    bool BinaryHashTree::Restore(const std::vector<uint8_t> &vKey)
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
        for(int i = hashmap[nBucket] - 1; i >= 0; --i)
        {
            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!fileCache->Get(i, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(strBaseLocation, "_hashtree.", std::setfill('0'), std::setw(5), i),
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
