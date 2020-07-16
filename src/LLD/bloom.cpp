/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <functional>

#include <LLD/hash/xxhash.h>
#include <LLD/templates/bloom.h>

#include <Util/include/filesystem.h>
#include <Util/templates/serialize.h>

namespace LLD
{
    /** Constant to determine maximum number of k-hashes for the secondary bloom filters. **/
    const uint32_t MAX_BLOOM_HASHES = 1;


    /* Get the bucket for given value k and key. */
    uint64_t BloomFilter::get_bucket(const std::vector<uint8_t>& vKey, const uint32_t nK) const
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH3_64bits_withSeed(&vKey[0], vKey.size(), nK);

        return static_cast<uint64_t>(nBucket % HASHMAP_TOTAL_BUCKETS);
    }


    /* Copy Constructor. */
    BloomFilter::BloomFilter(const BloomFilter& filter)
    : BitArray              (filter)
    , HASHMAP_TOTAL_BUCKETS (filter.HASHMAP_TOTAL_BUCKETS)
    , MUTEX                 ( )
    {
    }


    /* Move Constructor. */
    BloomFilter::BloomFilter(BloomFilter&& filter)
    : BitArray              (std::move(filter))
    , HASHMAP_TOTAL_BUCKETS (std::move(filter.HASHMAP_TOTAL_BUCKETS))
    , MUTEX                 ( )
    {
    }


    /* Copy assignment. */
    BloomFilter& BloomFilter::operator=(const BloomFilter& filter)
    {
        HASHMAP_TOTAL_BUCKETS       = filter.HASHMAP_TOTAL_BUCKETS;
        vRegisters                  = filter.vRegisters;

        return *this;
    }


    /* Move assignment. */
    BloomFilter& BloomFilter::operator=(BloomFilter&& filter)
    {
        HASHMAP_TOTAL_BUCKETS       = std::move(filter.HASHMAP_TOTAL_BUCKETS);
        vRegisters                  = std::move(filter.vRegisters);

        return *this;
    }


    /* Create bloom filter with given number of buckets. */
    BloomFilter::BloomFilter  (const uint64_t nBuckets)
    : BitArray              (nBuckets * 2)
    , HASHMAP_TOTAL_BUCKETS (nBuckets * 2) //we give 2 bits per bucket
    , MUTEX                 ( )
    {
    }


    /* Default Destructor. */
    BloomFilter::~BloomFilter()
    {
    }


    /* Add a new key to the bloom filter. */
    void BloomFilter::Insert(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* Set the internal bit from the bitarray. */
        for(int i = 0; i < MAX_BLOOM_HASHES; ++i)
        {
            uint64_t nBucket = get_bucket(vKey, i);
            set_bit(nBucket);
        }
    }


    /* Checks if a key exists in the bloom filter. */
    bool BloomFilter::Has(const std::vector<uint8_t>& vKey) const
    {
        LOCK(MUTEX);

        /* Set the internal bit from the bitarray. */
        for(int i = 0; i < MAX_BLOOM_HASHES; ++i)
        {
            uint64_t nBucket = get_bucket(vKey, i);
            if(!is_set(nBucket))
                return false;
        }

        return true;
    }
}
