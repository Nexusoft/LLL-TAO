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
            std::vector<uint8_t> vSpace(INDEX_FILTER_SIZE, 0);

            /* Write the new disk index .*/
            std::fstream stream(strIndex, std::ios::out | std::ios::binary | std::ios::trunc);

            /* Write the index file bit by bit. */
            for(uint32_t nBucket = 0; nBucket < CONFIG.HASHMAP_TOTAL_BUCKETS; ++nBucket)
                stream.write((char*)&vSpace[0], vSpace.size());

            /* Cleanup. */
            stream.close();

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Disk Index of ", vSpace.size(), " bytes");
        }

        /* Read the hashmap indexes. */
        else if(!CONFIG.QUICK_INIT)
        {
            /* Create a stopwatch to keep track of init time. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Deserialize the values into memory index. */
            uint64_t nTotalKeys = 0;

            /* Generate empty space for new file. */
            std::vector<uint8_t> vBuffer(CONFIG.HASHMAP_TOTAL_BUCKETS * INDEX_FILTER_SIZE, 0);

            /* Write the new disk index .*/
            std::fstream stream(strIndex, std::ios::in | std::ios::out | std::ios::binary);
            stream.read((char*)&vBuffer[0], vBuffer.size());
            stream.close();

            /* Loop through each bucket and account for the number of active keys. */
            for(uint32_t nBucket = 0; nBucket < CONFIG.HASHMAP_TOTAL_BUCKETS; ++nBucket)
            {
                /* Get the binary offset within the current probe. */
                uint64_t nOffset = nBucket * INDEX_FILTER_SIZE;

                /* Grab the current file from the stream. */
                uint16_t nCurrentFile = get_current_file(vBuffer, nOffset);
                for(int16_t nHashmap = 0; nHashmap < CONFIG.MAX_HASHMAP_FILES; ++nHashmap)
                {
                    /* Check for an available hashmap slot. */
                    if(!check_hashmap_available(nHashmap, vBuffer, nOffset + primary_bloom_size() + 2))
                        ++nTotalKeys;
                }

                /* Calculate total keys from hashmaps. */
                nTotalHashmaps = std::max(uint32_t(nCurrentFile), nTotalHashmaps);
            }

            /* Get our total runtime. */
            swTimer.stop();
            uint64_t nElapsed = swTimer.ElapsedMilliseconds();

            /* Debug output showing loading of disk index. */
            const uint64_t nMaxKeys = (CONFIG.HASHMAP_TOTAL_BUCKETS * CONFIG.MAX_HASHMAP_FILES);
            debug::log(0, FUNCTION,
                "Loaded in ",

                 nElapsed, " ms | ",
                vBuffer.size(), " bytes | ",
                nTotalKeys, "/", nMaxKeys, " keys [",
                ANSI_COLOR_CYAN, (nTotalKeys * 100.0) / nMaxKeys, " %", ANSI_COLOR_RESET, "] | ",
                std::min(CONFIG.MAX_HASHMAP_FILES, nTotalHashmaps), " hashmaps"
            );
        }

        /* Create the stream index object. */
        pindex = new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary);
    }


    /* Read a key index from the disk hashmaps. */
    bool BinaryHashMap::Get(const std::vector<uint8_t>& vKey, SectorKey &key)
    {
        LOCK(CONFIG.KEYCHAIN(vKey));

        /* Assign our key to return reference for usage in find_and_read. */
        key.vKey = vKey;

        /* Get the assigned bucket for the hashmap. */
        const uint32_t nBucket = get_bucket(vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBase(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(uint64_t(INDEX_FILTER_SIZE * nBucket), std::ios::beg);
        if(!pindex->read((char*)&vBase[0], vBase.size()))
        {
            return debug::error(FUNCTION, "BASE INDEX: ", pindex->eof() ? "EOF" : pindex->bad() ? "BAD" : pindex->fail() ? "FAIL" : "UNKNOWN",
                " only ", pindex->gcount(), "/", vBase.size(), " bytes read"
            );
        }
        else
        {
            debug::log(0, "read base index at pos ", uint64_t(INDEX_FILTER_SIZE * nBucket), " for bucket=", nBucket);
        }

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBase);

        debug::log(0, FUNCTION, "reading from hashmap ", nHashmap);

        PrintHex(vBase.begin(), vBase.end());
        if(nHashmap >= CONFIG.MAX_HASHMAP_FILES)
        {
            /* Loop all probe expansion cycles to ensure we check all locations a key could have been written. */
            for(uint32_t nCycle = 0; nCycle < (nHashmap - CONFIG.MAX_HASHMAP_FILES); ++nCycle)
            {
                /* Create an adjusted iterator to search through expansion cycles. */
                uint16_t nAdjustedHashmap = (CONFIG.MAX_HASHMAP_FILES + nCycle);

                /* Create our return values from fibanacci_index. */
                uint32_t nAdjustedBucket  = 0;
                uint32_t nTotalBuckets    = 0;

                /* Grab our indexes for current probing cycle. */
                std::vector<uint8_t> vIndex;
                if(!fibanacci_index(nAdjustedHashmap, nBucket, vIndex, nTotalBuckets, nAdjustedBucket))
                    return false;

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nFileRet   = nAdjustedHashmap;
                uint32_t nBucketRet = nAdjustedBucket;

                /* Check our hashmaps for given key. */
                if(find_and_read(key, vIndex, nFileRet, nBucketRet, nTotalBuckets))
                    return true;
            }
        }

        /* We make a copy here to prevent return by reference related bugs */
        uint16_t nFileRet   = nHashmap;
        uint32_t nBucketRet = nBucket;

        /* Check our hashmaps for given key. */
        if(find_and_read(key, vBase, nFileRet, nBucketRet, CONFIG.MIN_LINEAR_PROBES))
            return true;

        return false;//debug::error(FUNCTION, "doesn't exist from hashmap ", nHashmap);
    }


    /* Write a key to the disk hashmaps. */
    bool BinaryHashMap::Put(const SectorKey& key)
    {
        LOCK(CONFIG.KEYCHAIN(key.vKey));

        /* Get the assigned bucket for the hashmap. */
        const uint32_t nBucket       = get_bucket(key.vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBase(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(uint64_t(INDEX_FILTER_SIZE * nBucket), std::ios::beg);
        if(!pindex->read((char*)&vBase[0], vBase.size()))
        {
            return debug::error(FUNCTION, "BASE INDEX: ", pindex->eof() ? "EOF" : pindex->bad() ? "BAD" : pindex->fail() ? "FAIL" : "UNKNOWN",
                " only ", pindex->gcount(), "/", vBase.size(), " bytes read"
            );
        }
        else
        {
            debug::log(0, "read base index at pos ", uint64_t(INDEX_FILTER_SIZE * nBucket), " for bucket=", nBucket);
        }

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBase, 0);

        debug::log(0, FUNCTION, "writing from hashmap ", nHashmap);
        while(nHashmap < CONFIG.MAX_HASHMAP_FILES + CONFIG.MAX_LINEAR_PROBES)
        {
            /* Check if we are in a probe expansion cycle. */
            if(nHashmap >= CONFIG.MAX_HASHMAP_FILES)
            {
                /* Create our return values from fibanacci_index. */
                uint32_t nAdjustedBucket = 0;
                uint32_t nTotalBuckets   = 0;

                /* Grab our indexes for current probing cycle. */
                std::vector<uint8_t> vIndex;
                if(!fibanacci_index(nHashmap, nBucket, vIndex, nTotalBuckets, nAdjustedBucket))
                    return false;

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nFileRet   = nHashmap;
                uint32_t nBucketRet = nAdjustedBucket;

                /* Attempt to find and write key into keychain */
                if(find_and_write(key, vIndex, nFileRet, nBucketRet, nTotalBuckets))
                {
                    /* If we found a slot, signify expansion cycle is complete with a +1 value for searching. */
                    set_current_file(++nHashmap, vBase);

                    /* Flush our base index file to disk (file iterator) */
                    if(!flush_index(vBase, nBucket))
                        return false;

                    /* Flush our probing index file to disk (fibanacci expansion) */
                    if(!flush_index(vIndex, nAdjustedBucket, nBucketRet - nAdjustedBucket))
                        return false;

                    return true;
                }

                /* Check if we are at the end of our probing sequences. */ //TODO: this is buggy, we need to make more solid
                //if(nAdjustedBucket == 0 || nAdjustedBucket + nTotalBuckets == CONFIG.HASHMAP_TOTAL_BUCKETS)
                //    return debug::error(FUNCTION, "probe(s) exhausted: ", nHashmap, " at end of fibanacci search space");
            }
            else
            {
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

                    /* Write the new hashmap in smaller chunks to not overwhelm the buffers. */
                    for(uint32_t nFile = 0; nFile < CONFIG.HASHMAP_TOTAL_BUCKETS; ++nFile)
                        stream.write((char*)&vSpace[0], vSpace.size());
                    stream.close();

                    /* Debug output signifying new hashmap. */
                    debug::log(0, FUNCTION, "Created"
                        " | bucket=", nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                        " | hashmap=", nHashmap,
                        " | size=", CONFIG.HASHMAP_TOTAL_BUCKETS * CONFIG.HASHMAP_KEY_ALLOCATION / 1024.0, " Kb"
                    );
                }

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nFileRet   = nHashmap + 1; //when we get to this point, we have created a new hashmap so require a +1
                uint32_t nBucketRet = nBucket;

                /* Handle our key writing without probing adjacent buckets. */
                if(find_and_write(key, vBase, nFileRet, nBucketRet, CONFIG.MIN_LINEAR_PROBES))
                {
                    /* Check if file needs to be incremented. */
                    if(nFileRet >= nHashmap)
                    {
                        set_current_file(nFileRet + 1, vBase);
                        debug::log(0, FUNCTION, "regular incrementing file handle to ", nHashmap + 1);
                    }
                    else
                    {
                        debug::log(0, FUNCTION, "not incrementing ", nFileRet, " and ", get_current_file(vBase));
                    }


                    /* Flush our index file to disk. */
                    if(!flush_index(vBase, nBucket))
                        return false;

                    return true;
                }
            }

            /* Bump our current file number now. */
            debug::log(0, FUNCTION, "incrementing hashmap loop ", nHashmap + 1);
            set_current_file(++nHashmap, vBase);
        }

        /* Flush our index file to disk to indicate that search space is exhausted. */
        if(!flush_index(vBase, nBucket))
            return debug::error(FUNCTION, "probe(s) exhausted: failed to flush index");

        return debug::error(FUNCTION, "probe(s) exhausted: ", nHashmap, " at end of linear search space");
    }


    /* Flush all buffers to disk if using ACID transaction. */
    void BinaryHashMap::Flush()
    {
    }


    /*  Erase a key from the disk hashmaps.
     *  TODO: This should be optimized further. */
    bool BinaryHashMap::Erase(const std::vector<uint8_t>& vKey)
    {
        //LOCK(KEY_MUTEX);
        LOCK(CONFIG.KEYCHAIN(vKey));

        /* Get the assigned bucket for the hashmap. */
        uint32_t nBucket = get_bucket(vKey);

        /* Get the file binary position. */
        uint64_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = compress_key(vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

        /* Read the index file information. */
        pindex->seekg(INDEX_FILTER_SIZE * nBucket, std::ios::beg);
        if(!pindex->read((char*)&vBuffer[0], vBuffer.size()))
        {
            debug::log(0, "FILE INDEX: ", pindex->eof() ? "EOF" : pindex->bad() ? "BAD" : pindex->fail() ? "FAIL" : "UNKNOWN");
            return debug::error(FUNCTION, "INDEX: only ", pindex->gcount(), "/", vBuffer.size(), " bytes read");
        }

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBuffer, 0);

        /* Check the primary bloom filter. */
        if(!check_primary_bloom(vKey, vBuffer, 2))
            return false;

        /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
        for(int16_t nFile = nHashmap - 1; nFile >= 0; --nFile)
        {
            /* Check the secondary bloom filter. */
            if(!check_secondary_bloom(vKey, vBuffer, nFile, primary_bloom_size() + 2))
                continue;

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(nFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), nFile),
                  std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                pFileStreams->Put(nFile, pstream);
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
                SectorKey key;
                ssKey >> key;

                /* Seek to the hashmap index in file. */
                pstream->seekp (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                std::vector<uint8_t> vEmpty(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
                pstream->write((char*) &vEmpty[0], vEmpty.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Erased State: ", key.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", key.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", nHashmap - 1,
                        " | Sector File: ", key.nSectorFile,
                        " | Sector Size: ", key.nSectorSize,
                        " | Sector Start: ", key.nSectorStart,
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
        uint64_t nFilePos = nBucket * CONFIG.HASHMAP_KEY_ALLOCATION;

        /* Compress any keys larger than max size. */
        std::vector<uint8_t> vKeyCompressed = compress_key(vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBuffer(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

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
        for(int16_t nFile = nHashmap - 1; nFile >= 0; --nFile)
        {
            /* Check the secondary bloom filter. */
            if(!check_secondary_bloom(vKey, vBuffer, nFile, primary_bloom_size() + 2))
                continue;

            /* Find the file stream for LRU cache. */
            std::fstream* pstream;
            if(!pFileStreams->Get(nFile, pstream))
            {
                /* Set the new stream pointer. */
                pstream = new std::fstream(
                  debug::safe_printstr(CONFIG.BASE_DIRECTORY, "/keychain/_hashmap.", std::setfill('0'), std::setw(5), nFile),
                  std::ios::in | std::ios::out | std::ios::binary);

                /* If file not found add to LRU cache. */
                pFileStreams->Put(nFile, pstream);
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
                SectorKey key;
                ssKey >> key;

                /* Skip over keys that are already erased. */
                if(key.Ready())
                    return true;

                /* Seek to the hashmap index in file. */
                pstream->seekp (nFilePos, std::ios::beg);

                /* Read the bucket binary data from file stream */
                std::vector<uint8_t> vReady(STATE::READY);
                pstream->write((char*) &vReady[0], vReady.size());
                pstream->flush();

                /* Debug Output of Sector Key Information. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Restored State: ", key.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", key.nLength,
                        " | Bucket ", nBucket,
                        " | Location: ", nFilePos,
                        " | File: ", nHashmap - 1,
                        " | Sector File: ", key.nSectorFile,
                        " | Sector Size: ", key.nSectorSize,
                        " | Sector Start: ", key.nSectorStart,
                        " | Key: ", HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                return true;
            }
        }

        return false;
    }


    /*  Compresses a given key until it matches size criteria. */
    std::vector<uint8_t> BinaryHashMap::compress_key(const std::vector<uint8_t>& vKey)
    {
        /* Grab a 128-bit xxHash to store as the checksum. */
        XXH128_hash_t hash = XXH3_128bits((uint8_t*)&vKey[0], vKey.size());

        return std::vector<uint8_t>((uint8_t*)&hash, (uint8_t*)&hash + sizeof(hash));
    }


    /* Calculates a bucket to be used for the hashmap allocation. */
    uint32_t BinaryHashMap::get_bucket(const std::vector<uint8_t>& vKey)
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH3_64bits_withSeed(&vKey[0], vKey.size(), 0);
        return static_cast<uint32_t>(nBucket % CONFIG.HASHMAP_TOTAL_BUCKETS);
    }


    /* Find a file stream from the local LRU cache. */
    std::fstream* BinaryHashMap::get_file_stream(const uint32_t nHashmap)
    {
        /* Find the file stream for LRU cache. */
        std::fstream* pstream = nullptr;
        if(pFileStreams->Get(nHashmap, pstream) && !pstream)
        {
            /* Delete stream from the cache. */
            pFileStreams->Remove(nHashmap);
            pstream = nullptr;
        }

        /* Check for null file handle to see if we need to load it again. */
        if(pstream == nullptr)
        {
            /* The absolute path to the file we are opening. */
            std::string strHashmap = debug::safe_printstr(CONFIG.BASE_DIRECTORY, "keychain/_hashmap.", std::setfill('0'), std::setw(5), nHashmap);

            /* Set the new stream pointer. */
            pstream = new std::fstream(strHashmap, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
            {
                delete pstream;
                return nullptr;
            }

            /* If not in cache, add to the LRU. */
            pFileStreams->Put(nHashmap, pstream);
        }

        return pstream;
    }


    /* Flush the current buffer to disk. */
    bool BinaryHashMap::flush_index(const std::vector<uint8_t>& vBuffer, const uint32_t nBucket, const uint32_t nOffset)
    {
        //CRITICAL SECITON
        //LOCK(); TODO: need multiple index file streams allocated like locks are configured
        //get_index_stream(nBucket); //to get stream by bucket allowing us to lock with the same hash

        debug::log(0, FUNCTION, "flushing ", nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS, " | offset=", nOffset);
        PrintHex(vBuffer.begin(), vBuffer.end());

        /* Grab our binary offset. */
        uint32_t nBufferPos = (nOffset * INDEX_FILTER_SIZE);
        if(nBufferPos + INDEX_FILTER_SIZE > vBuffer.size())
            return debug::error(FUNCTION, "buffer overflow protection ", nBufferPos, "/", vBuffer.size()); //TODO" remove on production

        /* Seek to position if we are not already there. */
        //uint64_t nFilePos = (INDEX_FILTER_SIZE * (nBucket + nOffset));
        uint64_t nFilePos = (INDEX_FILTER_SIZE * nBucket);
        if(pindex->tellp() != nFilePos)
        {
            debug::log(0, FUNCTION, "seek to pos ", nFilePos);
            pindex->seekp(nFilePos, std::ios::beg);
        }
        else
            debug::log(0, FUNCTION, "already at pos ", nFilePos);

        /* Check our file number. */
        uint16_t nFile = get_current_file(vBuffer, 0);
        debug::log(0, FUNCTION, "flushing for file ", nFile);

        /* Write updated filters to the index position. */
        if(!pindex->write((char*)&vBuffer[0], vBuffer.size()))
        //if(!pindex->write((char*)&vBuffer[nOffset * INDEX_FILTER_SIZE], INDEX_FILTER_SIZE))
            return debug::error(FUNCTION, "INDEX: only ", pindex->gcount(), "/", vBuffer.size(), " bytes written");

        /* Flush the buffers to disk. */
        pindex->flush();

        return true;
    }


    /* Finds an available hashmap slot within the probing range and writes it to disk. */
    bool BinaryHashMap::find_and_write(const SectorKey& key, std::vector<uint8_t> &vBuffer,
        uint16_t &nHashmap, uint32_t &nBucket, const uint32_t nProbes)
    {
        /* Compress any keys larger than max size. */
        const std::vector<uint8_t> vKeyCompressed = compress_key(key.vKey);

        /* Loop through the adjacent linear hashmap slots in a linear ordering resptive to the buffer object. */
        for(uint32_t nProbe = 0; nProbe < nProbes; ++nProbe)
        {
            /* Check our ranges here and break early if exhausting hashmap buckets. */
            if(nProbe + nBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS) //TODO: remove debug::error when moving to production
                return debug::error(FUNCTION, "out of buckets ", nProbe + nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS);

            /* Get the binary offset within the current probe. */
            const uint64_t nOffset = (nProbe * INDEX_FILTER_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            const uint16_t nBucketIterator = std::min(uint16_t(CONFIG.MAX_HASHMAP_FILES - 1), uint16_t(nHashmap));
            for(int32_t nFile = nBucketIterator; nFile >= 0; --nFile)
            {
                /* Check for an available hashmap slot. */
                if(check_hashmap_available(nFile, vBuffer, nOffset + primary_bloom_size() + 2))
                {
                    /* Serialize the key into the end of the vector. */
                    DataStream ssKey(SER_LLD, DATABASE_VERSION);
                    ssKey << key;
                    ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                    /* Update the primary and secondary bloom filters. */
                    set_primary_bloom  (key.vKey, vBuffer, nOffset + 2);
                    set_secondary_bloom(key.vKey, vBuffer, nFile, nOffset + primary_bloom_size() + 2);

                    /* Write our new hashmap entry into the file's bucket. */
                    const uint64_t nFilePos = ((nBucket + nProbe) * CONFIG.HASHMAP_KEY_ALLOCATION);
                    {
                        LOCK(CONFIG.FILE(nFile));

                        /* Grab the current file stream from LRU cache. */
                        std::fstream* pstream = get_file_stream(nFile);
                        if(!pstream)
                            continue;

                        /* Seek if we are not at position. */
                        if(pstream->tellp() != nFilePos)
                            pstream->seekp(nFilePos, std::ios::beg);

                        /* Flush the key file to disk. */
                        if(!pstream->write((char*)&ssKey.Bytes()[0], ssKey.size()))
                        {
                            //TODO: we should recover or at least reset our streams here
                            return debug::error(FUNCTION, "KEYCHAIN: only ", pstream->gcount(), "/", ssKey.size(), " bytes written");
                        }

                        /* Flush the buffers to disk. */
                        pstream->flush();
                    }

                    /* Debug Output of Sector Key Information. */
                    if(config::nVerbose >= 4)
                        debug::log(4, FUNCTION, ANSI_COLOR_BRIGHT_CYAN, "State: ", key.nState == STATE::READY ? "Valid" : "Invalid",
                            " | Length: ", key.nLength,
                            " | Bucket ", nBucket,
                            " | Pos ", nFilePos,
                            " | Probe ", nProbe,
                            " | Location: ", nFilePos,
                            " | File: ", nFile,
                            " | Sector File: ", key.nSectorFile,
                            " | Sector Size: ", key.nSectorSize,
                            " | Sector Start: ", key.nSectorStart, ANSI_COLOR_RESET,
                            " | Key: ",  HexStr(vKeyCompressed.begin(), vKeyCompressed.end()));

                    /* Set our return values. */
                    nBucket += nProbe;
                    nHashmap = uint16_t(nFile);

                    //std::vector<uint8_t> vBucket = ssKey.Bytes();

                    //debug::log(0, FUNCTION, "Bucket binary data is at ", nFilePos, " | probe=", nProbe, "/", nProbes, " | file=", nFile);
                    //PrintHex(vBucket.begin(), vBucket.end());

                    return true;
                }
            }
        }

        return false;
    }


    /* Finds a key within a hashmap with given probing ranges */
    bool BinaryHashMap::find_and_read(SectorKey &key, const std::vector<uint8_t>& vBuffer,
        uint16_t &nHashmap, uint32_t &nBucket, const uint32_t nProbes)
    {
        /* Compress any keys larger than max size. */
        const std::vector<uint8_t> vKeyCompressed = compress_key(key.vKey);

        /* Check that key was supplied in arguments. */
        if(key.vKey.empty()) //this is mostly superfluous and just to prevent somone from mis-using this method
            return debug::error(FUNCTION, "key binary data missing");

        /* Loop through the adjacent linear hashmap slots. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0); //we can re-use this buffer
        for(uint32_t nProbe = 0; nProbe < nProbes; ++nProbe)
        {
            /* Check our ranges here and break early if exhausting hashmap buckets. */
            if(nProbe + nBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS) //TODO: remove debug::error when moving to production
                return debug::error(FUNCTION, "out of buckets ", nProbe + nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS);

            /* Get the binary offset within the current probe. */
            //const uint64_t nOffset = (nProbe * INDEX_FILTER_SIZE);

            /* Check the primary bloom filter. */
            //if(!check_primary_bloom(key.vKey, vBuffer, nOffset + 2))
            //    continue;

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            const uint16_t nBucketIterator = std::min(uint16_t(CONFIG.MAX_HASHMAP_FILES - 1), uint16_t(nHashmap - 1));
            for(int32_t nFile = nBucketIterator; nFile >= 0; --nFile)
            {
                /* Check the secondary bloom filter. */
                //if(!check_secondary_bloom(key.vKey, vBuffer, nFile, nOffset + primary_bloom_size() + 2))
                //    continue;

                /* Read our bucket level data from the hashmap. */
                const uint64_t nFilePos = ((nBucket + nProbe) * CONFIG.HASHMAP_KEY_ALLOCATION);
                {
                    LOCK(CONFIG.FILE(nFile));

                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream = get_file_stream(nFile);
                    if(!pstream)
                        continue;

                    /* Seek here if our cursor is not at the correct position, but take advantage of linear access when possible */
                    if(pstream->tellg() != nFilePos)
                        pstream->seekg(nFilePos + (nProbe * CONFIG.HASHMAP_KEY_ALLOCATION), std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    if(!pstream->read((char*) &vBucket[0], vBucket.size()))
                    {
                        debug::log(0, "FILE STREAM: ", pstream->eof() ? "EOF" : pstream->bad() ? "BAD" : pstream->fail() ? "FAIL" : "UNKNOWN");
                        return debug::error(FUNCTION, "STREAM: only ", pstream->gcount(), "/", vBucket.size(), " bytes read at cursor ", nFilePos + (nProbe * CONFIG.HASHMAP_KEY_ALLOCATION));
                    }
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Deserialie key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    ssKey >> key;

                    /* Debug Output of Sector Key Information. */
                    if(config::nVerbose >= 4)
                        debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_CYAN, "State: ", key.nState == STATE::READY ? "Valid" : "Invalid",
                        " | Length: ", key.nLength,
                        " | Bucket ", nBucket,
                        " | Pos ", nFilePos,
                        " | Probe ", nProbe,
                        " | Location: ", nFilePos,
                        " | File: ", nHashmap,
                        " | Sector File: ", key.nSectorFile,
                        " | Sector Size: ", key.nSectorSize,
                        " | Sector Start: ", key.nSectorStart, "\n", ANSI_COLOR_RESET,
                        HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    /* Set our return values. */
                    nBucket += nProbe;
                    nHashmap = uint16_t(nFile);

                    return true;
                }
                else
                {
                    //debug::log(0, FUNCTION, "Bucket binary data is at ", nFilePos, " | probe=", nProbe, "/", nProbes, " | file=", nFile);
                    //PrintHex(vBucket.begin(), vBucket.end());
                }

            }
        }

        return false;
    }


    /* Helper method to grab the current file from the input buffer. */
    uint16_t BinaryHashMap::get_current_file(const std::vector<uint8_t>& vBuffer, const uint32_t nOffset)
    {
        /* Check for buffer overflow. */
        if(nOffset + 2 >= vBuffer.size())
            return debug::error(FUNCTION, "buffer overflow protection");

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
        {
            debug::error(FUNCTION, "buffer overflow protection");
            return;
        }

        debug::log(0, FUNCTION, "setting current file to ", nCurrent);

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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return false;
            }

            /* Check that any bits within the range of this hashmap bitset are not set. */
            if(vBuffer[nIndex + nOffset] & (1 << nBit))
                return false;
        }

        return true;
    }


    /* Helper method to check if a file is unoccupied in input buffer. */
    void BinaryHashMap::set_hashmap_available(const uint32_t nHashmap, std::vector<uint8_t> &vBuffer, const uint32_t nOffset)
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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return;
            }

            /* Set the bit within the range to 0. */
            vBuffer[nIndex + nOffset] &= ~(1 << nBit);
        }
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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return true;
            }

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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return;
            }

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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return;
            }

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
            {
                debug::error(FUNCTION, "buffer overflow protection");
                return true;
            }

            /* Check this k-hash value bit is not set. */
            if(!(vBuffer[nIndex + nOffset] & (1 << nBit)))
                return false;
        }

        return true;
    }


    /* Expand a beginning and ending iterator based on fibanacci sequence. */
    void BinaryHashMap::fibanacci_expansion(uint32_t &nBeginProbeExpansion, uint32_t &nEndProbeExpansion, const uint32_t nExpansionCycles)
    {
        /* Calculate our current probing range. */
        for(uint32_t n = 0; n < nExpansionCycles; ++n)
        {
            /* Cache our previous probe begin. */
            uint32_t nLastProbeBegin = nBeginProbeExpansion;

            /* Calculate our fibanacci expansion value. */
            nBeginProbeExpansion = nEndProbeExpansion;
            nEndProbeExpansion  += nLastProbeBegin;
        }
    }

    /* Reads the indexing entries based on fibanacci expansion sequence. */
    bool BinaryHashMap::fibanacci_index(const uint16_t nHashmap, const uint32_t nBucket,
        std::vector<uint8_t> &vIndex, uint32_t &nTotalBuckets, uint32_t &nAdjustedBucket)
    {
        /* Find the total cycles to probe. */
        uint64_t nExpansionCycles = (nHashmap - CONFIG.MAX_HASHMAP_FILES);

        /* Start our probe expansion cycle with default cycles. */
        uint32_t nBeginProbeExpansion = CONFIG.MIN_LINEAR_PROBES;
        uint32_t nEndProbeExpansion   = CONFIG.MIN_LINEAR_PROBES;

        /* Calculate our current probing range. */
        fibanacci_expansion(nBeginProbeExpansion, nEndProbeExpansion, nExpansionCycles);

        /* Cache our adjustment values. */
        uint64_t nIndexPos       = 0;

        /* Check if we are in a forward or reverse probing cycle. */
        int64_t nOverflow = std::min(int64_t(nBucket), int64_t(nBucket + nEndProbeExpansion) - CONFIG.HASHMAP_TOTAL_BUCKETS);
        if(nOverflow > 0)
        {
            /* Calculate our adjusted bucket and index position. */
            nAdjustedBucket = (nBucket - nOverflow);
            nIndexPos       = (INDEX_FILTER_SIZE * nAdjustedBucket);

            /* Debug output . */
            if(config::nVerbose >= 4)
                debug::log(4, FUNCTION, ANSI_COLOR_FUNCTION, "Reverse Expansion", ANSI_COLOR_RESET,
                " | begin=", nBeginProbeExpansion,
                " | end=", nEndProbeExpansion,
                " | overflow=", nOverflow,
                " | bucket=", nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | adjust=", nAdjustedBucket,
                " | pos=", nIndexPos, "/", CONFIG.HASHMAP_TOTAL_BUCKETS * INDEX_FILTER_SIZE,
                " | file=", nHashmap
            );
        }
        else
        {
            /* Check if we need to seek to read in our buffer. */
            nAdjustedBucket = (nBucket + nBeginProbeExpansion);
            nIndexPos       = (INDEX_FILTER_SIZE * nAdjustedBucket);

            /* Debug output . */
            if(config::nVerbose >= 4)
                debug::log(4, FUNCTION, ANSI_COLOR_FUNCTION, "Forward Expansion", ANSI_COLOR_RESET,
                " | begin=", nBeginProbeExpansion,
                " | end=", nEndProbeExpansion,
                " | bucket=", nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | adjust=", nAdjustedBucket,
                " | pos=", nIndexPos, "/", CONFIG.HASHMAP_TOTAL_BUCKETS * INDEX_FILTER_SIZE,
                " | file=", nHashmap
            );
        }

        /* Find our total number of buckets to probe this cycle and check our range. */
        nTotalBuckets = (nEndProbeExpansion - nBeginProbeExpansion);
        if(nTotalBuckets + nAdjustedBucket > CONFIG.HASHMAP_TOTAL_BUCKETS)
            nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS - nAdjustedBucket);

        /* Seek if we aren't at the correct file position. */
        if(pindex->tellg() != nIndexPos)
        {
            pindex->seekg(nIndexPos, std::ios::beg);
            debug::log(0, FUNCTION, "seek to pos ", nIndexPos, " | probes=", nTotalBuckets);
        }
        else
            debug::log(0, FUNCTION, "already at pos ", nIndexPos, " | probes=", nTotalBuckets);

        /* Read data into our buffer object. */
        vIndex.resize(nTotalBuckets * INDEX_FILTER_SIZE);
        if(!pindex->read((char*)&vIndex[0], vIndex.size()))
        {
            return debug::error(FUNCTION,
                "INDEX: ", pindex->eof() ? "EOF" : pindex->bad() ? "BAD" : pindex->fail() ? "FAIL" : "UNKNOWN",
                " only ", pindex->gcount(), "/", vIndex.size(), " bytes read"
            );
        }
        else
        {
            PrintHex(vIndex.begin(), vIndex.end());
        }

        return true;
    }
}
