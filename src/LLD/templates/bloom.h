/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_TEMPLATES_BLOOM_H
#define NEXUS_LLD_TEMPLATES_BLOOM_H

#include <LLD/include/version.h>

#include <Util/templates/serialize.h>
#include <Util/templates/datastream.h>

#include <cstdint>
#include <vector>
#include <atomic>
#include <fstream>
#include <condition_variable>

#include <Util/include/mutex.h>


namespace LLD
{
    /** Bloom Filter Class
     *
     *  Class to act as a container for keys.
     *  This class operateds in O(1) for insert and search
     *
     *  It has an internal file handler that commits to disk when called.
     *
     **/
    class BloomFilter
    {

        /** is_set
         *
         *  Check if a particular bit is set in the bloom filter.
         *
         *  @param[in] nBucket The bucket to check bit for.
         *
         *  @return true if the bit is set.
         *
         **/
        bool is_set(const uint64_t nBucket) const;


        /** set_bit
         *
         *  Set a bit in the bloom filter at given bucket
         *
         *  @param[in] nBucket The bucket to set bit for.
         *
         **/
        void set_bit(const uint64_t nBucket);


        /** get_bucket
         *
         *  Get the bucket for given value k and key.
         *
         *  @param[in] vKey The binary data of database key.
         *  @param[in] nK The k-value for hash function.
         *
         *  @return the bucket for given value k.
         *
         **/
        uint64_t get_bucket(const std::vector<uint8_t>& vKey, const uint32_t nK) const;


        /** The total number of buckets for this bloom filter. **/
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The bloom filter bitarray using 64 bit registers. **/
        std::vector<uint64_t> bloom;


        /** Mutex to protect internal states. **/
        mutable std::mutex MUTEX;

    public:


        /** Default Constructor. **/
        BloomFilter()                                    = delete;


        /** Copy Constructor. **/
        BloomFilter(const BloomFilter& filter);


        /** Move Constructor. **/
        BloomFilter(BloomFilter&& filter);


        /** Copy assignment. **/
        BloomFilter& operator=(const BloomFilter& filter);


        /** Move assignment. **/
        BloomFilter& operator=(BloomFilter&& filter);


        /** Default Destructor. **/
        ~BloomFilter();


        /** Create bloom filter with given number of buckets. **/
        BloomFilter  (const uint64_t nBuckets);



        /** Bytes
         *
         *  Get the beginning memory location of the bloom filter.
         *
         **/
        uint8_t* Bytes() const;


        /** Size
         *
         *  Get the size (in bytes) of the bloom filter.
         *
         **/
         uint64_t Size() const;


        /** Insert
         *
         *  Add a new key to the bloom filter.
         *
         *  @param[in] vKey The binary data of key to insert.
         *
         **/
        void Insert(const std::vector<uint8_t>& vKey);


        /** Has
         *
         *  Checks if a key exists in the bloom filter.
         *
         *  @param[in] vKey The binary data of key to check.
         *
         *  @return true if the key was found.
         *
         **/
        bool Has(const std::vector<uint8_t>& vKey) const;





        /** Insert
         *
         *  Add a new key to the bloom filter.
         *
         *  @param[in] key The binary data of key to insert.
         *
         **/
        template<typename KeyType>
        void Insert(const KeyType& key)
        {
            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            Insert(ssKey.Bytes());
        }



        /** Has
         *
         *  Checks if a key exists in the bloom filter.
         *
         *  @param[in] key The key to check for
         *
         *  @return true if the key was found.
         *
         **/
        template<typename KeyType>
        bool Has(const KeyType& key) const
        {
            /* Serialize Key into Bytes. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            return Has(ssKey.Bytes());
        }
    };
}

#endif
