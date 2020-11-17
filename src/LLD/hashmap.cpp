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
#include <bitset>


namespace LLD
{
    /* The Database Constructor. To determine file location and the Bytes per Record. */
    BinaryHashMap::BinaryHashMap(const Config::Hashmap& configIn)
    : CONFIG                (configIn)
    , INDEX_FILTER_SIZE     (primary_bloom_size() + secondary_bloom_size() + 2)
    , pFileStreams          (new TemplateLRU<std::pair<uint16_t, uint16_t>, std::fstream*>(CONFIG.MAX_HASHMAP_FILE_STREAMS))
    , pIndexStreams         (new TemplateLRU<uint16_t, std::fstream*>(CONFIG.MAX_INDEX_FILE_STREAMS))
    {
        Initialize();
    }


    /* Copy Constructor */
    BinaryHashMap::BinaryHashMap(const BinaryHashMap& map)
    : CONFIG                 (map.CONFIG)
    , INDEX_FILTER_SIZE      (map.INDEX_FILTER_SIZE)
    , pFileStreams           (map.pFileStreams)
    , pIndexStreams          (map.pIndexStreams)
    {
        Initialize();
    }


    /* Move Constructor */
    BinaryHashMap::BinaryHashMap(BinaryHashMap&& map)
    : CONFIG                 (std::move(map.CONFIG))
    , INDEX_FILTER_SIZE      (std::move(map.INDEX_FILTER_SIZE))
    , pFileStreams           (std::move(map.pFileStreams))
    , pIndexStreams          (std::move(map.pIndexStreams))
    {
        Initialize();
    }


    /* Copy Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(const BinaryHashMap& map)
    {
        pFileStreams   = map.pFileStreams;
        pIndexStreams  = map.pIndexStreams;

        Initialize();

        return *this;
    }


    /* Move Assignment Operator */
    BinaryHashMap& BinaryHashMap::operator=(BinaryHashMap&& map)
    {
        pFileStreams   = std::move(map.pFileStreams);
        pIndexStreams  = std::move(map.pIndexStreams);

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
        /* Keep track of total hashmaps. */
        uint32_t nTotalHashmaps = 0;

        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(CONFIG.DIRECTORY + "keychain/") && filesystem::create_directories(CONFIG.DIRECTORY + "keychain/"))
            debug::log(0, FUNCTION, "Generated Path ", CONFIG.DIRECTORY);

        /* Read the hashmap indexes and calculate our total keys in this database instance. */
        if(!CONFIG.QUICK_INIT)
        {
            /* Create a stopwatch to keep track of init time. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Deserialize the values into memory index. */
            uint64_t nTotalKeys = 0;

            /* Calculate the number of bukcets per index file. */
            const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_INDEX) + 1;
            std::vector<uint8_t> vBuffer(nTotalBuckets * INDEX_FILTER_SIZE, 0);

            /* Loop through each bucket and account for the number of active keys. */
            for(uint32_t nIndexIterator = 0; nIndexIterator < CONFIG.MAX_FILES_PER_INDEX; ++nIndexIterator)
            {
                /* Read the new disk index .*/
                std::fstream* pindex = get_index_stream(nIndexIterator);
                if(!pindex)
                    continue;

                /* Read our index data into our buffer. */
                if(!pindex->read((char*)&vBuffer[0], vBuffer.size()))
                    continue;

                /* Check these slots for available keys. */
                for(uint32_t nBucket = 0; nBucket < nTotalBuckets; ++nBucket)
                {
                    /* Get the binary offset within the current probe. */
                    uint64_t nOffset = nBucket * INDEX_FILTER_SIZE;

                    /* Grab the current file from the stream. */
                    uint16_t nCurrentFile = get_current_file(vBuffer, nOffset);
                    for(int16_t nHashmap = 0; nHashmap < CONFIG.MAX_HASHMAPS; ++nHashmap)
                    {
                        /* Check for an available hashmap slot. */
                        if(!check_hashmap_available(nHashmap, vBuffer, nOffset + primary_bloom_size() + 2))
                            ++nTotalKeys;
                    }

                    /* Calculate total keys from hashmaps. */
                    nTotalHashmaps = std::max(uint32_t(nCurrentFile), nTotalHashmaps);
                }
            }

            /* Get our total runtime. */
            swTimer.stop();
            uint64_t nElapsed = swTimer.ElapsedMilliseconds();

            /* Debug output showing loading of disk index. */
            const uint64_t nMaxKeys = (CONFIG.HASHMAP_TOTAL_BUCKETS * CONFIG.MAX_HASHMAPS);
            debug::log(0, FUNCTION,
                "Loaded in ",

                 nElapsed, " ms | ",
                vBuffer.size(), " bytes | ",
                nTotalKeys, "/", nMaxKeys, " keys [",
                ANSI_COLOR_CYAN, (nTotalKeys * 100.0) / nMaxKeys, " %", ANSI_COLOR_RESET, "] | ",
                std::min(CONFIG.MAX_HASHMAPS, nTotalHashmaps), " hashmaps"
            );
        }
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

        /* Read our index information. */
        if(!read_index(vBase, nBucket, CONFIG.MIN_LINEAR_PROBES))
            return false;

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBase);
        if(nHashmap >= CONFIG.MAX_HASHMAPS)
        {
            /* Create our cached values to detect when no more useful work is being completed. */
            uint32_t nBucketCache  = 0, nTotalCache = 0;

            /* Loop all probe expansion cycles to ensure we check all locations a key could have been written. */
            for(uint32_t nCycle = 0; nCycle < (nHashmap - CONFIG.MAX_HASHMAPS); ++nCycle)
            {
                /* Create an adjusted iterator to search through expansion cycles. */
                uint16_t nAdjustedHashmap = (CONFIG.MAX_HASHMAPS + nCycle + 1); //we need +1 here to derive correct fibanacci zone

                /* Create our return values from fibanacci_index. */
                uint32_t nAdjustedBucket  = 0;
                uint32_t nTotalBuckets    = 0;

                /* Grab our indexes for current probing cycle. */
                std::vector<uint8_t> vIndex;
                if(!fibanacci_index(nAdjustedHashmap, nBucket, vIndex, nTotalBuckets, nAdjustedBucket))
                    return false;

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nHashmapRet   = nAdjustedHashmap;
                uint32_t nBucketRet    = nAdjustedBucket;

                /* Check our hashmaps for given key. */
                if(find_and_read(key, vIndex, nHashmapRet, nBucketRet, nTotalBuckets))
                    return true;

                /* Check that our cached bucket and total are different (signifying useful work was completed). */
                if(nBucketCache == nAdjustedBucket && nTotalCache == nTotalBuckets)
                    return debug::error(FUNCTION, "probe(s) exhausted: ", VARIABLE(nBucketCache), " | ", VARIABLE(nTotalCache), " at end of fibanacci search space");

                /* Update our cached value if it passed. */
                nBucketCache = nAdjustedBucket;
                nTotalCache  = nTotalBuckets;
            }
        }

        //TODO: we need to adjust for MIN_LINEAR_PROBES reverse probing and check the root bucket before probing other buckets

        /* We make a copy here to prevent return by reference related bugs */
        uint16_t nHashmapRet   = nHashmap;
        uint32_t nBucketRet    = nBucket;

        /* Check our hashmaps for given key. */
        if(find_and_read(key, vBase, nHashmapRet, nBucketRet, CONFIG.MIN_LINEAR_PROBES))
            return true;

        return false;
    }


    /* Write a key to the disk hashmaps. */
    bool BinaryHashMap::Put(const SectorKey& key)
    {
        LOCK(CONFIG.KEYCHAIN(key.vKey));

        /* Get the assigned bucket for the hashmap. */
        const uint32_t nBucket       = get_bucket(key.vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBase(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

        /* Read our index information. */
        if(!read_index(vBase, nBucket, CONFIG.MIN_LINEAR_PROBES))
            return false;

        /* Create our cached values to detect when no more useful work is being completed. */
        uint32_t nBucketCache  = 0, nTotalCache = 0;
        uint16_t nHashmap = get_current_file(vBase, 0);

        /* Loop through our potential linear probe cycles. */
        while(nHashmap < CONFIG.MAX_HASHMAPS + CONFIG.MAX_LINEAR_PROBE_CYCLES)
        {
            /* Check if we are in a probe expansion cycle. */
            if(nHashmap >= CONFIG.MAX_HASHMAPS)
            {
                /* Create our return values from fibanacci_index. */
                uint32_t nAdjustedBucket = 0;
                uint32_t nTotalBuckets   = 0;

                /* Grab our indexes for current probing cycle. */
                std::vector<uint8_t> vIndex;
                if(!fibanacci_index(nHashmap, nBucket, vIndex, nTotalBuckets, nAdjustedBucket))
                    return false;

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nHashmapRet   = nHashmap;
                uint32_t nBucketRet    = nAdjustedBucket;

                /* Attempt to find and write key into keychain */
                if(find_and_write(key, vIndex, nHashmapRet, nBucketRet, nTotalBuckets))
                {
                    /* If we found a slot, signify expansion cycle is complete with a +1 value for searching. */
                    set_current_file(nHashmap, vBase); //we set to current fibanacci zone so we can keep writing to it for next key

                    /* Flush our base index file to disk (file iterator) */
                    if(!flush_index(vBase, nBucket))
                        return false;

                    /* Flush our probing index file to disk (fibanacci expansion) */
                    if(!flush_index(vIndex, nAdjustedBucket, nBucketRet - nAdjustedBucket))
                        return false;

                    return true;
                }

                /* Check that our cached bucket is different (signifying useful work was completed). */
                if(nBucketCache == nAdjustedBucket && nTotalCache == nTotalBuckets)
                    return debug::error(FUNCTION, "probe(s) exhausted: ", VARIABLE(nBucketCache), " | ", VARIABLE(nTotalCache), " at end of fibanacci search space");

                /* Update our cached values if it passed. */
                nBucketCache = nAdjustedBucket;
                nTotalCache  = nTotalBuckets;
            }
            else
            {
                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nHashmapRet   = nHashmap;
                uint32_t nBucketRet    = nBucket;

                /* Handle our key writing without probing adjacent buckets. */
                if(find_and_write(key, vBase, nHashmapRet, nBucketRet, CONFIG.MIN_LINEAR_PROBES))
                {
                    /* Check if file needs to be incremented. */
                    if(nHashmapRet >= nHashmap)
                        set_current_file(nHashmapRet + 1, vBase);

                    /* Flush our index file to disk. */
                    if(!flush_index(vBase, nBucket))
                        return false;

                    return true;
                }
            }

            /* Bump our current file number now. */
            set_current_file(++nHashmap, vBase);
        }

        /* Flush our index file to disk to indicate that search space is exhausted. */
        if(!flush_index(vBase, nBucket))
            return false;

        return debug::error(FUNCTION, "probe(s) exhausted: ", VARIABLE(nHashmap), " at end of linear search space ", VARIABLE(CONFIG.MAX_LINEAR_PROBE_CYCLES));
    }


    /*  Erase a key from the disk hashmaps. */
    bool BinaryHashMap::Erase(const std::vector<uint8_t>& vKey)
    {
        LOCK(CONFIG.KEYCHAIN(vKey));

        /* Get the assigned bucket for the hashmap. */
        const uint32_t nBucket = get_bucket(vKey);

        /* Build our buffer based on total linear probes. */
        std::vector<uint8_t> vBase(INDEX_FILTER_SIZE * CONFIG.MIN_LINEAR_PROBES, 0);

        /* Read our index information. */
        if(!read_index(vBase, nBucket, CONFIG.MIN_LINEAR_PROBES))
            return false;

        /* Grab the current hashmap file from the buffer. */
        uint16_t nHashmap = get_current_file(vBase);
        if(nHashmap >= CONFIG.MAX_HASHMAPS)
        {
            /* Loop all probe expansion cycles to ensure we check all locations a key could have been written. */
            for(uint32_t nCycle = 0; nCycle < (nHashmap - CONFIG.MAX_HASHMAPS); ++nCycle)
            {
                /* Create an adjusted iterator to search through expansion cycles. */
                const uint16_t nAdjustedHashmap = (CONFIG.MAX_HASHMAPS + nCycle + 1); //we need +1 here to derive correct fibanacci zone

                /* Create our return values from fibanacci_index. */
                uint32_t nAdjustedBucket  = 0;
                uint32_t nTotalBuckets    = 0;

                /* Grab our indexes for current probing cycle. */
                std::vector<uint8_t> vIndex;
                if(!fibanacci_index(nAdjustedHashmap, nBucket, vIndex, nTotalBuckets, nAdjustedBucket))
                    return false;

                /* We make a copy here to prevent return by reference related bugs */
                uint16_t nHashmapRet   = nAdjustedHashmap;
                uint32_t nBucketRet    = nAdjustedBucket;

                /* Check our hashmaps for given key. */
                if(find_and_erase(vKey, vIndex, nHashmapRet, nBucketRet, nTotalBuckets))
                {
                    /* Remove this key from indexing file. */
                    const uint32_t nOffset = (INDEX_FILTER_SIZE * (nBucketRet - nAdjustedBucket));
                    set_hashmap_available(nHashmapRet, vIndex, nOffset + primary_bloom_size() + 2);

                    /* Flush our probing index file to disk (fibanacci expansion) */
                    if(!flush_index(vIndex, nAdjustedBucket, nBucketRet - nAdjustedBucket))
                        return false;

                    return true;
                }
            }
        }

        /* We make a copy here to prevent return by reference related bugs */
        uint16_t nHashmapRet   = nHashmap;
        uint32_t nBucketRet    = nBucket;

        /* Check our hashmaps for given key. */
        if(find_and_erase(vKey, vBase, nHashmapRet, nBucketRet, CONFIG.MIN_LINEAR_PROBES))
        {
            /* Remove this key from indexing file. */
            set_hashmap_available(nHashmapRet, vBase, primary_bloom_size() + 2);

            /* Flush our probing index file to disk (fibanacci expansion) */
            if(!flush_index(vBase, nBucket))
                return false;

            return true;
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
    std::fstream* BinaryHashMap::get_file_stream(const uint32_t nHashmap, const uint32_t nBucket)
    {
        /* Find the index of our stream based on configuration. */
        const uint32_t nFile = (nBucket * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;

        /* Cache our index for re-use in this method. */
        const std::pair<uint16_t, uint16_t> pairIndex = std::make_pair(nFile, nHashmap);

        /* Find the file stream for LRU cache. */
        std::fstream* pstream = nullptr;
        if(pFileStreams->Get(pairIndex, pstream) && !pstream) //TODO: !pstream->good() for production
        {
            debug::log(0, "Removing Stream ", VARIABLE(nFile), " | ", VARIABLE(nHashmap));

            /* Delete stream from the cache. */
            pFileStreams->Remove(pairIndex);
            pstream = nullptr;
        }

        /* Check for null file handle to see if we need to load it again. */
        if(pstream == nullptr)
        {
            /* The absolute path to the file we are opening. (ex. _hashmap.001.00001) */
            const std::string strHashmap = debug::safe_printstr(CONFIG.DIRECTORY, "keychain/_hashmap.",
                std::setfill('0'), std::setw(3), pairIndex.first, ".", std::setfill('0'), std::setw(5), pairIndex.second);

            /* Create a new file if it doesn't exist yet. */
            if(!filesystem::exists(strHashmap))
            {
                /* Calculate the number of bukcets per index file. */
                const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_HASHMAP) + 1;

                /* Blank vector to write empty space in new disk file. */
                std::vector<uint8_t> vSpace(CONFIG.HASHMAP_KEY_ALLOCATION, 0);

                /* Write the blank data to the new file handle. */
                std::ofstream stream(strHashmap, std::ios::out | std::ios::binary | std::ios::app);
                if(!stream)
                    return nullptr;

                /* Write the new hashmap in smaller chunks to not overwhelm the buffers. */
                for(uint32_t n = 0; n < nTotalBuckets; ++n)
                    stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output signifying new hashmap. */
                debug::log(0, FUNCTION, "Created Hashmap",
                    " | file=",   nFile + 1, "/", CONFIG.MAX_FILES_PER_HASHMAP,
                    " | hashmap=", nHashmap + 1, "/", CONFIG.MAX_HASHMAPS,
                    " | size=", (nTotalBuckets * CONFIG.HASHMAP_KEY_ALLOCATION) / 1024.0, " Kb"
                );
            }

            /* Set the new stream pointer. */
            pstream = new std::fstream(strHashmap, std::ios::in | std::ios::out | std::ios::binary);
            if(!pstream->is_open())
            {
                delete pstream;
                return nullptr;
            }

            /* If not in cache, add to the LRU. */
            pFileStreams->Put(pairIndex, pstream);
        }

        return pstream;
    }



    /* Get a corresponding index stream from the LRU, if it doesn't exist create the file with std::ofstream. */
    std::fstream* BinaryHashMap::get_index_stream(const uint32_t nFile)
    {
        /* Find the file stream for LRU cache. */
        std::fstream* pindex = nullptr;
        if(pIndexStreams->Get(nFile, pindex) && !pindex) //TODO: !pindex->good() for production
        {
            /* Delete stream from the cache. */
            pIndexStreams->Remove(nFile);
            pindex = nullptr;
        }

        /* Check for null file handle to see if we need to load it again. */
        if(pindex == nullptr)
        {
            /* Build the filesystem path */
            std::string strHashmap = debug::safe_printstr(CONFIG.DIRECTORY, "keychain/_index.", std::setfill('0'), std::setw(5), nFile);

            /* Create a new file if it doesn't exist yet. */
            if(!filesystem::exists(strHashmap))
            {
                /* Calculate the number of bukcets per index file. */
                const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_INDEX) + 1;

                /* Blank vector to write empty space in new disk file. */
                std::vector<uint8_t> vSpace(INDEX_FILTER_SIZE * nTotalBuckets, 0);

                /* Write the blank data to the new file handle. */
                std::ofstream stream(strHashmap, std::ios::out | std::ios::binary | std::ios::app);
                if(!stream)
                    return nullptr;

                /* Write the new index file to disk. */
                stream.write((char*)&vSpace[0], vSpace.size());
                stream.close();

                /* Debug output signifying new hashmap. */
                debug::log(0, FUNCTION, "Created Index"
                    " | file=", nFile + 1, "/", CONFIG.MAX_FILES_PER_INDEX,
                    " | size=", (INDEX_FILTER_SIZE * nTotalBuckets) / 1024.0, " Kb"
                );
            }

            /* Set the new stream pointer. */
            pindex = new std::fstream(strHashmap, std::ios::in | std::ios::out | std::ios::binary);
            if(!pindex->is_open())
            {
                delete pindex;
                return nullptr;
            }

            /* If not in cache, add to the LRU. */
            pIndexStreams->Put(nFile, pindex);
        }

        return pindex;
    }


    /* Flush the current buffer to disk. */
    bool BinaryHashMap::flush_index(const std::vector<uint8_t>& vBuffer, const uint32_t nBucket, const uint32_t nOffset)
    {
        /* Calculate our adjusted bucket. */
        const uint32_t nAdjustedBucket = (nBucket + nOffset);

        /* Check we are flushing within range of the hashmap indexes. */
        if(nBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS)
            return debug::error(FUNCTION, "out of range ", VARIABLE(nBucket));

        /* Calculate the current file designated by the bucket. */
        const uint32_t nFile = (nAdjustedBucket * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;

        /* Used to check projected file based on boundary iterator to determine if the count needs a -1 offset. */
        const uint32_t nBegin = (nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX;
        const uint32_t nCheck = (nBegin * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;

        /* Grab our binary offset. */
        const uint64_t nBufferPos = (nOffset * INDEX_FILTER_SIZE);
        if(nBufferPos + INDEX_FILTER_SIZE > vBuffer.size())
            return debug::error(FUNCTION, "buffer overflow protection ", nBufferPos, "/", vBuffer.size()); //TODO" remove on production

        /* Find our new file position from current bucket and offset. */
        const uint64_t nFilePos = (INDEX_FILTER_SIZE * (nAdjustedBucket - ((nCheck != nFile) ? 1 : 0) -
            ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX)));

        /* Grab our file stream. */
        std::fstream* pindex = get_index_stream(nFile);
        if(!pindex)
            return debug::error(FUNCTION, "INDEX: failed to open stream: ", VARIABLE(nFile));

        /* Seek to position if we are not already there. */
        if(pindex->tellp() != nFilePos)
            pindex->seekp(nFilePos, std::ios::beg);

        /* Write updated filters to the index position. */
        if(!pindex->write((char*)&vBuffer[nBufferPos], INDEX_FILTER_SIZE))
            return debug::error(FUNCTION, "INDEX: only ", pindex->gcount(), "/", vBuffer.size(), " bytes written");

        /* Flush the buffers to disk. */
        pindex->flush();

        return true;
    }


    /* Read an index entry at given bucket crossing file boundaries. */
    bool BinaryHashMap::read_index(std::vector<uint8_t> &vBuffer, const uint32_t nBucket, const uint32_t nTotal)
    {
        /* Check we are flushing within range of the hashmap indexes. */
        if(nBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS)
            return debug::error(FUNCTION, "out of range ", VARIABLE(nBucket));

        /* Keep track of how many buckets and bytes we have remaining in this read cycle. */
        uint32_t nRemaining = nTotal;
        uint32_t nIterator  = nBucket;
        uint64_t nBufferPos = 0;

        /* Adjust our buffer size to fit the total buckets. */
        vBuffer.resize(INDEX_FILTER_SIZE * nTotal);
        do
        {
            /* Calculate the file and boundaries we are on with current bucket. */
            const uint32_t nFile = (nIterator * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;
            const uint32_t nBoundary  = (((nFile + 1) * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX);

            /* Used to check projected file based on boundary iterator to determine if the count needs a -1 offset. */
            const uint32_t nBegin = (nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX;
            const uint32_t nCheck = (nBegin * CONFIG.MAX_FILES_PER_INDEX) / CONFIG.HASHMAP_TOTAL_BUCKETS;

            /* Find our new file position from current bucket and offset. */
            const uint64_t nFilePos      = (INDEX_FILTER_SIZE * (nIterator - ((nCheck != nFile) ? 1 : 0) -
                ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_INDEX)));

            /* Grab our file stream. */
            std::fstream* pindex = get_index_stream(nFile);
            if(!pindex)
                return debug::error(FUNCTION, "INDEX: failed to open index stream");

            /* Seek to position if we are not already there. */
            if(pindex->tellg() != nFilePos)
                pindex->seekg(nFilePos, std::ios::beg);

            /* Find the range (in bytes) we want to read for this index range. */
            const uint32_t nMaxBuckets = std::min(nRemaining, std::max(1u, (nBoundary - nIterator))); //need 1u otherwise we could loop indefinately
            const uint64_t nReadSize   = nMaxBuckets * INDEX_FILTER_SIZE;

            /* Read our index data into the buffer. */
            if(!pindex->read((char*)&vBuffer[nBufferPos], nReadSize))
            {
                debug::warning(FUNCTION, VARIABLE(nFilePos), " | ", VARIABLE(nFile), " | ", VARIABLE(nMaxBuckets), " | ", VARIABLE(nBufferPos), " | ", VARIABLE(nReadSize));
                return debug::error(FUNCTION, "INDEX: ", pindex->eof() ? "EOF" : pindex->bad() ? "BAD" : pindex->fail() ? "FAIL" : "UNKNOWN",
                    " only ", pindex->gcount(), "/", vBuffer.size(), " bytes read"
                );
            }

            /* Track our current binary position, remaining buckets to read, and current bucket iterator. */
            nRemaining -= nMaxBuckets;
            nBufferPos += nReadSize;
            nIterator  += nMaxBuckets;
        }
        while(nRemaining > 0); //continue reading into the buffer, with one loop per file adjusting to each boundary

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
            /* Calculate our adjusted bucket. */
            const uint32_t nAdjustedBucket = (nBucket + nProbe);

            /* Check our ranges here and break early if exhausting hashmap buckets. */
            if(nAdjustedBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS) //TODO: remove debug::error when moving to production
                return debug::error(FUNCTION, "out of buckets ", nProbe + nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS);

            /* Get the binary offset within the current probe. */
            const uint64_t nOffset = (nProbe * INDEX_FILTER_SIZE);

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            const uint16_t nBeginHashmap = std::min(uint16_t(CONFIG.MAX_HASHMAPS - 1), uint16_t(nHashmap));
            for(int32_t nHashmapIterator = nBeginHashmap; nHashmapIterator >= 0; --nHashmapIterator)
            {
                /* Check for an available hashmap slot. */
                if(check_hashmap_available(nHashmapIterator, vBuffer, nOffset + primary_bloom_size() + 2))
                {
                    /* Serialize the key into the end of the vector. */
                    DataStream ssKey(SER_LLD, DATABASE_VERSION);
                    ssKey << key;
                    ssKey.write((char*)&vKeyCompressed[0], vKeyCompressed.size());

                    /* Update the primary and secondary bloom filters. */
                    set_primary_bloom  (key.vKey, vBuffer, nOffset + 2);
                    set_secondary_bloom(key.vKey, vBuffer, nHashmapIterator, nOffset + primary_bloom_size() + 2);

                    /* Calculate the file and boundaries we are on with current bucket. */
                    const uint32_t nFile = (nAdjustedBucket * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;
                    const uint32_t nBoundary  = ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_HASHMAP);

                    /* Used to check projected file based on boundary iterator to determine if the count needs a -1 offset. */
                    const uint32_t nCheck = (nBoundary * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;

                    /* Write our new hashmap entry into the file's bucket. */
                    const uint64_t nFilePos = (CONFIG.HASHMAP_KEY_ALLOCATION * (nAdjustedBucket - nBoundary - ((nCheck != nFile) ? 1 : 0)));
                    {
                        /* Grab the current file stream from LRU cache. */
                        std::fstream* pstream = get_file_stream(nHashmapIterator, nAdjustedBucket);
                        if(!pstream)
                            continue;

                        /* Seek if we are not at position. */
                        if(pstream->tellp() != nFilePos)
                            pstream->seekp(nFilePos, std::ios::beg);

                        /* Flush the key file to disk. */
                        if(!pstream->write((char*)&ssKey.Bytes()[0], ssKey.size()))
                            return debug::error(FUNCTION, "KEYCHAIN: only ", pstream->gcount(), "/", ssKey.size(), " bytes written");

                        /* Flush the buffers to disk. */
                        pstream->flush();
                    }

                    /* Set our return values. */
                    nBucket  = nAdjustedBucket;
                    nHashmap = uint16_t(nHashmapIterator);

                    /* Debug Output of Sector Key Information. */
                    if(config::nVerbose >= 4)
                        debug::log(4, FUNCTION, ANSI_COLOR_BRIGHT_CYAN, "State: ", key.nState == STATE::READY ? "Valid" : "Invalid",
                            " | Length: ", key.nLength,
                            " | Bucket ", nBucket,
                            " | Pos ", nFilePos,
                            " | Probe ", nProbe,
                            " | Location: ", nFilePos,
                            " | File: ", nHashmapIterator,
                            " | Sector File: ", key.nSectorFile,
                            " | Sector Size: ", key.nSectorSize,
                            " | Sector Start: ", key.nSectorStart, "\n", ANSI_COLOR_RESET,
                            HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                    return true;
                }
            }
        }

        return false;
    }


    /* Finds an available hashmap slot within the probing range and writes it to disk. */
    bool BinaryHashMap::find_and_erase(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer,
        uint16_t &nHashmap, uint32_t &nBucket, const uint32_t nProbes)
    {
        /* Compress any keys larger than max size. */
        const std::vector<uint8_t> vKeyCompressed = compress_key(vKey);

        /* Loop through the adjacent linear hashmap slots. */
        std::vector<uint8_t> vBucket(CONFIG.HASHMAP_KEY_ALLOCATION, 0); //we can re-use this buffer
        for(uint32_t nProbe = 0; nProbe < nProbes; ++nProbe)
        {
            /* Calculate our adjusted bucket. */
            const uint32_t nAdjustedBucket = (nBucket + nProbe);

            /* Check our ranges here and break early if exhausting hashmap buckets. */
            if(nAdjustedBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS) //TODO: remove debug::error when moving to production
                return debug::error(FUNCTION, "out of buckets ", nProbe + nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS);

            /* Get the binary offset within the current probe. */
            const uint64_t nOffset = (nProbe * INDEX_FILTER_SIZE);

            /* Check the primary bloom filter. */
            if(!check_primary_bloom(vKey, vBuffer, nOffset + 2))
                continue;

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            const uint16_t nBeginHashmap = std::min(uint16_t(CONFIG.MAX_HASHMAPS - 1), uint16_t(nHashmap - 1));
            for(int32_t nHashmapIterator = nBeginHashmap; nHashmapIterator >= 0; --nHashmapIterator)
            {
                /* Check the secondary bloom filter. */
                if(!check_secondary_bloom(vKey, vBuffer, nHashmapIterator, nOffset + primary_bloom_size() + 2))
                    continue;

                /* Calculate the file and boundaries we are on with current bucket. */
                const uint32_t nFile = (nAdjustedBucket * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;
                const uint32_t nBoundary  = ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_HASHMAP);

                /* Used to check projected file based on boundary iterator to determine if the count needs a -1 offset. */
                const uint32_t nCheck = (nBoundary * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;

                /* Write our new hashmap entry into the file's bucket. */
                const uint64_t nFilePos = (CONFIG.HASHMAP_KEY_ALLOCATION * (nAdjustedBucket - nBoundary - ((nCheck != nFile) ? 1 : 0)));
                {
                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream = get_file_stream(nHashmapIterator, nAdjustedBucket);
                    if(!pstream)
                        continue;

                    /* Seek here if our cursor is not at the correct position, but take advantage of linear access when possible */
                    if(pstream->tellg() != nFilePos)
                        pstream->seekg(nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    if(!pstream->read((char*) &vBucket[0], vBucket.size()))
                    {
                        /* Calculate the number of bukcets per index file. */
                        const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_HASHMAP) + 1;
                        debug::log(0, VARIABLE(nHashmapIterator), " | ", VARIABLE(nBucket), " | ", VARIABLE(nTotalBuckets), " | ", VARIABLE(nFilePos));
                        debug::log(0, "FILE STREAM: ", pstream->eof() ? "EOF" : pstream->bad() ? "BAD" : pstream->fail() ? "FAIL" : "UNKNOWN");
                        return debug::error(FUNCTION, "STREAM: only ", pstream->gcount(), "/", vBucket.size(),
                            " bytes read at cursor ", nFilePos, "/", nTotalBuckets * CONFIG.HASHMAP_KEY_ALLOCATION);

                    }

                    /* Check if this bucket has the key */
                    if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                    {
                        /* Seek if we are not at position. */
                        if(pstream->tellp() != nFilePos)
                            pstream->seekp(nFilePos, std::ios::beg);

                        /* Flush the key file to disk. */
                        const std::vector<uint8_t> vBlank(CONFIG.HASHMAP_KEY_ALLOCATION, 0);
                        if(!pstream->write((char*)&vBlank[0], vBlank.size()))
                            return debug::error(FUNCTION, "KEYCHAIN: only ", pstream->gcount(), "/", vBlank.size(), " bytes written");

                        /* Set our return values. */
                        nBucket  = nAdjustedBucket;
                        nHashmap = uint16_t(nHashmapIterator);

                        /* Debug Output of Sector Key Information. */
                        if(config::nVerbose >= 4)
                            debug::log(4, FUNCTION, ANSI_COLOR_BRIGHT_CYAN, "State: Empty",
                                " | Bucket ", nBucket,
                                " | Pos ", nFilePos,
                                " | Probe ", nProbe,
                                " | Location: ", nFilePos,
                                " | File: ", nHashmapIterator,
                                ANSI_COLOR_RESET, "\n",
                                HexStr(vKeyCompressed.begin(), vKeyCompressed.end(), true));

                        /* Flush the buffers to disk. */
                        pstream->flush();

                        return true;
                    }
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
            /* Calculate our adjusted bucket. */
            const uint32_t nAdjustedBucket = (nBucket + nProbe);

            /* Check our ranges here and break early if exhausting hashmap buckets. */
            if(nAdjustedBucket >= CONFIG.HASHMAP_TOTAL_BUCKETS) //TODO: remove debug::error when moving to production
                return debug::error(FUNCTION, "out of buckets ", nProbe + nBucket, "/", CONFIG.HASHMAP_TOTAL_BUCKETS);

            /* Get the binary offset within the current probe. */
            const uint64_t nOffset = (nProbe * INDEX_FILTER_SIZE);

            /* Check the primary bloom filter. */
            if(!check_primary_bloom(key.vKey, vBuffer, nOffset + 2))
                continue;

            /* Reverse iterate the linked file list from hashmap to get most recent keys first. */
            const uint16_t nBeginHashmap = std::min(uint16_t(CONFIG.MAX_HASHMAPS - 1), uint16_t(nHashmap - 1));
            for(int32_t nHashmapIterator = nBeginHashmap; nHashmapIterator >= 0; --nHashmapIterator)
            {
                /* Check the secondary bloom filter. */
                if(!check_secondary_bloom(key.vKey, vBuffer, nHashmapIterator, nOffset + primary_bloom_size() + 2))
                    continue;

                /* Calculate the file and boundaries we are on with current bucket. */
                const uint32_t nFile = (nAdjustedBucket * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;
                const uint32_t nBoundary  = ((nFile * CONFIG.HASHMAP_TOTAL_BUCKETS) / CONFIG.MAX_FILES_PER_HASHMAP);

                /* Used to check projected file based on boundary iterator to determine if the count needs a -1 offset. */
                const uint32_t nCheck = (nBoundary * CONFIG.MAX_FILES_PER_HASHMAP) / CONFIG.HASHMAP_TOTAL_BUCKETS;

                /* Read our bucket level data from the hashmap. */
                const uint64_t nFilePos = (CONFIG.HASHMAP_KEY_ALLOCATION * (nAdjustedBucket - nBoundary - ((nCheck != nFile) ? 1 : 0)));
                {
                    /* Find the file stream for LRU cache. */
                    std::fstream* pstream = get_file_stream(nHashmapIterator, nAdjustedBucket);
                    if(!pstream)
                        continue;

                    /* Seek here if our cursor is not at the correct position, but take advantage of linear access when possible */
                    if(pstream->tellg() != nFilePos)
                        pstream->seekg(nFilePos, std::ios::beg);

                    /* Read the bucket binary data from file stream */
                    if(!pstream->read((char*) &vBucket[0], vBucket.size()))
                    {
                        /* Calculate the number of bukcets per index file. */
                        const uint32_t nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS / CONFIG.MAX_FILES_PER_HASHMAP) + 1;
                        debug::log(0, VARIABLE(nHashmapIterator), " | ", VARIABLE(nBucket), " | ", VARIABLE(nTotalBuckets), " | ", VARIABLE(nFilePos));
                        debug::log(0, "FILE STREAM: ", pstream->eof() ? "EOF" : pstream->bad() ? "BAD" : pstream->fail() ? "FAIL" : "UNKNOWN");
                        return debug::error(FUNCTION, "STREAM: only ", pstream->gcount(), "/", vBucket.size(),
                            " bytes read at cursor ", nFilePos, "/", nTotalBuckets * CONFIG.HASHMAP_KEY_ALLOCATION);

                    }
                }

                /* Check if this bucket has the key */
                if(std::equal(vBucket.begin() + 13, vBucket.begin() + 13 + vKeyCompressed.size(), vKeyCompressed.begin()))
                {
                    /* Deserialie key and return if found. */
                    DataStream ssKey(vBucket, SER_LLD, DATABASE_VERSION);
                    ssKey >> key;

                    /* Set our return values. */
                    nBucket  = nAdjustedBucket;
                    nHashmap = uint16_t(nHashmapIterator);

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

                    return true;
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
        uint64_t nBits = CONFIG.MAX_HASHMAPS * CONFIG.SECONDARY_BLOOM_BITS;
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
                debug::error(FUNCTION, "buffer overflow protection ", nIndex + nOffset, " | ", vBuffer.size());
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
        uint32_t nBeginBit = (nHashmap * CONFIG.SECONDARY_BLOOM_BITS) % 8;

        /* Loop through the total k-hashes for this bloom filter. */
        for(uint32_t k = 0; k < CONFIG.SECONDARY_BLOOM_HASHES; ++k)
        {
            /* Find the bucket that correlates to this k-hash. */
            uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % CONFIG.SECONDARY_BLOOM_BITS;

            /* Calculate our local bit and index in the buffer. */
            uint32_t nIndex  = nBeginIndex + (nBeginBit + nBucket) / 8;
            uint32_t nBit    = (nBeginBit + nBucket) % 8;

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
        uint32_t nExpansionCycles = std::min(nHashmap - CONFIG.MAX_HASHMAPS, CONFIG.MAX_LINEAR_PROBE_CYCLES);

        /* Start our probe expansion cycle with default values (our fibinacci expansion will use base MIN_LINEAR_PROBES). */
        uint32_t nBeginProbeExpansion = CONFIG.MIN_LINEAR_PROBES;
        uint32_t nEndProbeExpansion   = CONFIG.MIN_LINEAR_PROBES;

        /* Calculate our current probing range. */
        fibanacci_expansion(nBeginProbeExpansion, nEndProbeExpansion, nExpansionCycles);

        /* Check if we are in a forward or reverse probing cycle. */
        int64_t nOverflow = std::min(int64_t(nBucket), int64_t(nBucket + nEndProbeExpansion) - int64_t(CONFIG.HASHMAP_TOTAL_BUCKETS));
        if(nOverflow > 0)
        {
            /* Calculate our adjusted bucket and index position. */
            nAdjustedBucket = (nBucket - nOverflow);

            /* Debug output . */
            if(config::nVerbose >= 4)
                debug::log(4, FUNCTION, ANSI_COLOR_FUNCTION, "Reverse Expansion", ANSI_COLOR_RESET,
                " | begin=", nBeginProbeExpansion,
                " | end=", nEndProbeExpansion,
                " | overflow=", nOverflow,
                " | bucket=", nBucket + 1, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | adjusted=", nAdjustedBucket + 1, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | iterator=", nHashmap
            );
        }
        else
        {
            /* Check if we need to seek to read in our buffer. */
            nAdjustedBucket = std::min(nBucket + nBeginProbeExpansion, uint32_t(CONFIG.HASHMAP_TOTAL_BUCKETS - 1));

            /* Debug output . */
            if(config::nVerbose >= 4)
                debug::log(4, FUNCTION, ANSI_COLOR_FUNCTION, "Forward Expansion", ANSI_COLOR_RESET,
                " | begin=", nBeginProbeExpansion,
                " | end=", nEndProbeExpansion,
                " | bucket=", nBucket + 1, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | adjusted=", nAdjustedBucket + 1, "/", CONFIG.HASHMAP_TOTAL_BUCKETS,
                " | iterator=", nHashmap
            );
        }

        /* Check our probing ranges and adjust where necessary. */
        if(nEndProbeExpansion > CONFIG.MAX_LINEAR_PROBES)
        {
            /* Make sure we haven't expanded to a range beyond our probing configuration. */
            if(nBeginProbeExpansion > CONFIG.MAX_LINEAR_PROBES)
            {
                debug::warning(FUNCTION, VARIABLE(nBeginProbeExpansion), " adjusted");

                nBeginProbeExpansion = CONFIG.MAX_LINEAR_PROBES;
            }


            /* Adjust the end of the probing range to set as our maximum value probe from the origin (eg. nBucket). */
            nEndProbeExpansion = CONFIG.MAX_LINEAR_PROBES;

            debug::warning(FUNCTION, VARIABLE(nEndProbeExpansion), " adjusted");
        } //this will default to a total of 0 buckets if outside of range, and then be cleaned up with the cache check in Put()


        /* Find our total number of buckets to probe this cycle and check our range. */
        nTotalBuckets = (nEndProbeExpansion - nBeginProbeExpansion);
        if(nAdjustedBucket + nTotalBuckets > CONFIG.HASHMAP_TOTAL_BUCKETS)
        {
            nTotalBuckets = (CONFIG.HASHMAP_TOTAL_BUCKETS - std::min(uint32_t(CONFIG.HASHMAP_TOTAL_BUCKETS), nAdjustedBucket));
            debug::warning(FUNCTION, VARIABLE(nTotalBuckets), " adjusted");
        }


        /* Read our index information. */
        if(!read_index(vIndex, nAdjustedBucket, nTotalBuckets))
            return false;

        return true;
    }
}
