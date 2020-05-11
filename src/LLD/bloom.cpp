/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <functional>

#include <LLD/hash/xxh3.h>
#include <LLD/templates/bloom.h>

#include <Util/include/filesystem.h>
#include <Util/include/softfloat.h>
#include <Util/templates/serialize.h>

namespace LLD
{
    /* Check if a particular bit is set in the bloom filter. */
    bool BloomFilter::is_set(const uint64_t nBucket) const
    {
        return bloom[nBucket / 64] & (1 << (63 - (nBucket % 64)));
    }


    /* Set a bit in the bloom filter at given bucket */
    void BloomFilter::set_bit(const uint64_t nBucket)
    {
        bloom[nBucket / 64] |= (1 << (63 - (nBucket % 64)));
    }


    /* Get the bucket for given value k and key. */
    uint64_t BloomFilter::get_bucket(const std::vector<uint8_t>& vKey, const uint32_t nK) const
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), nK);

        return static_cast<uint64_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    /* Thread to flush bloom filter disk states periodically. */
    void BloomFilter::flush_thread()
    {
        /* Loop until shutdown or destruct. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load() && !fDestruct.load())
        {
            /* Check for data to be written. */
            uint64_t nCurrentKeys = nTotalKeys.load();
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);

            /* Wait for signals. */
            runtime::stopwatch swTimer;
            swTimer.start();
            CONDITION.wait(CONDITION_LOCK,
                [&]
                {
                    return
                    (
                        config::fShutdown.load() || fDestruct.load() ||
                        (nCurrentKeys != nTotalKeys.load() && swTimer.ElapsedMilliseconds() > 500) //we don't want to flush every wakeup
                    );
                }
            );

            /* Flush the disk states. */
            Flush();
        }
    }


    /* Copy Constructor. */
    BloomFilter::BloomFilter(const BloomFilter& filter)
    : HASHMAP_TOTAL_BUCKETS (filter.HASHMAP_TOTAL_BUCKETS)
    , bloom                 (filter.bloom)
    , pindex                (filter.pindex)
    , strBaseLocation       (filter.strBaseLocation)
    , MUTEX                 ( )
    , THREAD                ( )
    , CONDITION             ( )
    , fDestruct             (filter.fDestruct.load())
    , MAX_BLOOM_HASHES      (filter.MAX_BLOOM_HASHES)
    , MAX_BLOOM_KEYS        (filter.MAX_BLOOM_KEYS)
    , nTotalKeys            (filter.nTotalKeys.load())
    {
    }


    /* Move Constructor. */
    BloomFilter::BloomFilter(BloomFilter&& filter)
    : HASHMAP_TOTAL_BUCKETS (std::move(filter.HASHMAP_TOTAL_BUCKETS))
    , bloom                 (std::move(filter.bloom))
    , pindex                (std::move(filter.pindex))
    , strBaseLocation       (std::move(filter.strBaseLocation))
    , MUTEX                 ( )
    , THREAD                ( )
    , CONDITION             ( )
    , fDestruct             (filter.fDestruct.load())
    , MAX_BLOOM_HASHES      (std::move(filter.MAX_BLOOM_HASHES))
    , MAX_BLOOM_KEYS        (std::move(filter.MAX_BLOOM_KEYS))
    , nTotalKeys            (filter.nTotalKeys.load())
    {
    }


    /* Copy assignment. */
    BloomFilter& BloomFilter::operator=(const BloomFilter& filter)
    {
        HASHMAP_TOTAL_BUCKETS = filter.HASHMAP_TOTAL_BUCKETS;
        bloom                 = filter.bloom;
        pindex                = filter.pindex;
        strBaseLocation       = filter.strBaseLocation;

        fDestruct             = filter.fDestruct.load();
        MAX_BLOOM_KEYS        = filter.MAX_BLOOM_KEYS;
        nTotalKeys            = filter.nTotalKeys.load();

        return *this;
    }


    /* Move assignment. */
    BloomFilter& BloomFilter::operator=(BloomFilter&& filter)
    {
        HASHMAP_TOTAL_BUCKETS = std::move(filter.HASHMAP_TOTAL_BUCKETS);
        bloom                 = std::move(filter.bloom);
        pindex                = std::move(filter.pindex);
        strBaseLocation       = std::move(filter.strBaseLocation);

        fDestruct             = std::move(filter.fDestruct.load());
        MAX_BLOOM_KEYS        = filter.MAX_BLOOM_KEYS;
        nTotalKeys            = std::move(filter.nTotalKeys.load());

        return *this;
    }


    /* Create bloom filter with given number of buckets. */
    BloomFilter::BloomFilter  (const uint64_t nBuckets, const std::string& strBaseLocationIn, const uint16_t nK)
    : HASHMAP_TOTAL_BUCKETS (nBuckets)
    , bloom                 (HASHMAP_TOTAL_BUCKETS / 64, 0)
    , pindex                (nullptr)
    , strBaseLocation       (strBaseLocationIn)
    , MUTEX                 ( )
    , THREAD                ( )
    , CONDITION             ( )
    , fDestruct             (false)
    , MAX_BLOOM_HASHES      (nK)
    , nTotalKeys            (0)
    {
    }


    /* Default Destructor. */
    BloomFilter::~BloomFilter()
    {
        fDestruct.store(true);

        /* Wait for background thread. */
        CONDITION.notify_all();
        if(THREAD.joinable())
            THREAD.join();

        /* Flush one last time. */
        Flush();
        delete pindex;
    }


    /* Initialize the bit-array from disk. */
    void BloomFilter::Initialize()
    {
        LOCK(MUTEX);

        /* Build the hashmap indexes. */
        std::string strIndex = debug::safe_printstr(strBaseLocation, "_bloom.index");

        /* Create directories if they don't exist yet. */
        if(!filesystem::exists(strBaseLocation) && filesystem::create_directories(strBaseLocation))
            debug::log(0, FUNCTION, "Generated Path ", strBaseLocation);

        /* Check that a new bloom filter disk file needs to be created. */
        if(!filesystem::exists(strIndex))
        {
            /* Write the new disk index .*/
            std::fstream stream(strIndex, std::ios::out | std::ios::binary | std::ios::trunc);
            stream.write((char*)&bloom[0], bloom.size() * 8);

            /* Append the total keys. */
            uint64_t nCurrentKeys = 0;
            stream.write((char*)&nCurrentKeys, 8);
            stream.close();

            /* Debug output showing generation of disk index. */
            debug::log(0, FUNCTION, "Generated Bloom Filter of ", bloom.size() * 8, " bytes");
        }

        /* Create the stream index object. */
        pindex = new std::fstream(strIndex, std::ios::in | std::ios::out | std::ios::binary);
        pindex->seekg(0, std::ios::beg);
        if(!pindex->read((char*)&bloom[0], bloom.size() * 8))
        {
            debug::error(FUNCTION, "only ", pindex->gcount(), "/", bloom.size() * 8, " bytes read");
            return;
        }

        /* Grab the total keys from disk. */
        uint64_t nCurrentKeys = 0;
        if(!pindex->read((char*) &nCurrentKeys, 8))
        {
            debug::error(FUNCTION, "only ", pindex->gcount(), "/8 bytes read");
            return;
        }
        nTotalKeys.store(nCurrentKeys);

        /* Calculate the maximum keys. */
        MAX_BLOOM_KEYS =
            cv::softdouble((bloom.size() * 64) * cv::log(cv::softdouble(2.0)))
                        / cv::softdouble(MAX_BLOOM_HASHES); //(m ln(2) / k = n)

        /* Debug output showing loading of disk index. */
        debug::log(0, FUNCTION, "Loaded Bloom Filter of ", (bloom.size() * 8) + 8, " bytes and (", nTotalKeys.load(), "/", MAX_BLOOM_KEYS, ") keys");

        /* Start up the flush thread. */
        THREAD = std::thread(std::bind(&BloomFilter::flush_thread, this));
    }


    /* Flush the bit-array to the file stream. */
    void BloomFilter::Flush()
    {
        LOCK(MUTEX);

        /* Flush data from array to disk. */
        pindex->seekp(0, std::ios::beg);
        if(!pindex->write((char*) &bloom[0], bloom.size() * 8))
        {
            debug::error(FUNCTION, "only ", pindex->gcount(), "/", bloom.size() * 8, " bytes written");
            return;
        }
        pindex->flush();

        /* Flush the total keys to disk. */
        uint64_t nCurrentKeys = nTotalKeys.load();
        if(!pindex->write((char*) &nCurrentKeys, 8))
        {
            debug::error(FUNCTION, "only ", pindex->gcount(), "/8 bytes written");
            return;
        }
        pindex->flush();

        //debug::log(0, "Flushed to disk with ", nTotalKeys.load(), " keys");
    }


    /* Add a new key to the bloom filter. */
    void BloomFilter::Insert(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* Check for maximum keys. */
        if(nTotalKeys.load() >= MAX_BLOOM_KEYS)
            return;

        /* Run over k hashes. */
        for(uint16_t nK = 0; nK < MAX_BLOOM_HASHES; ++nK)
        {
            uint64_t nBucket = get_bucket(vKey, nK);
            set_bit(nBucket);
        }

        ++nTotalKeys;
        CONDITION.notify_all();
    }


    /* Checks if a key exists in the bloom filter. */
    bool BloomFilter::Has(const std::vector<uint8_t>& vKey) const
    {
        LOCK(MUTEX);

        /* Run over k hashes. */
        for(uint16_t nK = 0; nK < MAX_BLOOM_HASHES; ++nK)
        {
            uint64_t nBucket = get_bucket(vKey, nK);
            if(!is_set(nBucket))
                return false;
        }

        return true;
    }


    /** Full
     *
     *  Determines if the bloom filter is full based on chosen value of k to reduce false positives.
     *
     **/
    bool BloomFilter::Full() const
    {
        return nTotalKeys.load() >= MAX_BLOOM_KEYS;
    }
}
