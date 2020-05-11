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


    /* Copy Constructor. */
    BloomFilter::BloomFilter(const BloomFilter& filter)
    : HASHMAP_TOTAL_BUCKETS (filter.HASHMAP_TOTAL_BUCKETS)
    , bloom                 (filter.bloom)
    , MUTEX                 ( )
    , MAX_BLOOM_HASHES      (filter.MAX_BLOOM_HASHES)
    {
    }


    /* Move Constructor. */
    BloomFilter::BloomFilter(BloomFilter&& filter)
    : HASHMAP_TOTAL_BUCKETS (std::move(filter.HASHMAP_TOTAL_BUCKETS))
    , bloom                 (std::move(filter.bloom))
    , MUTEX                 ( )
    , MAX_BLOOM_HASHES      (std::move(filter.MAX_BLOOM_HASHES))
    {
    }


    /* Copy assignment. */
    BloomFilter& BloomFilter::operator=(const BloomFilter& filter)
    {
        HASHMAP_TOTAL_BUCKETS = filter.HASHMAP_TOTAL_BUCKETS;
        bloom                 = filter.bloom;

        return *this;
    }


    /* Move assignment. */
    BloomFilter& BloomFilter::operator=(BloomFilter&& filter)
    {
        HASHMAP_TOTAL_BUCKETS = std::move(filter.HASHMAP_TOTAL_BUCKETS);
        bloom                 = std::move(filter.bloom);

        return *this;
    }


    /* Create bloom filter with given number of buckets. */
    BloomFilter::BloomFilter(const uint64_t nBuckets, const uint16_t nTotalHashes)
    : HASHMAP_TOTAL_BUCKETS (nBuckets)
    , bloom                 (HASHMAP_TOTAL_BUCKETS / 64, 0)
    , MUTEX                 ( )
    , MAX_BLOOM_HASHES      (nTotalHashes)
    , nTotalKeys            (0)
    {
    }


    /* Default Destructor. */
    BloomFilter::~BloomFilter()
    {
    }


    /* Get the beginning memory location of the bloom filter. */
    uint8_t* BloomFilter::Bytes() const
    {
        return (uint8_t*)&bloom[0];
    }


    /* Get the size (in bytes) of the bloom filter. */
    uint64_t BloomFilter::Size() const
    {
        return bloom.size() * 8;
    }


    /* Add a new key to the bloom filter. */
    void BloomFilter::Insert(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* Run over k hashes. */
        for(uint16_t nK = 0; nK < MAX_BLOOM_HASHES; ++nK)
        {
            uint64_t nBucket = get_bucket(vKey, nK);
            set_bit(nBucket);
        }

        ++nTotalKeys;
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
}
