/*__________________________________________________________________________________________
            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
            (c) Copyright The Nexus Developers 2014 - 2021
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_CACHE_KEY_LRU_H
#define NEXUS_LLD_CACHE_KEY_LRU_H

#include <mutex>
#include <cstdint>
#include <vector>

#include <LLD/include/version.h>

#include <Util/templates/datastream.h>


//TODO: Abstract base class for all cache systems
namespace LLD
{

    /** BinaryKey
     *
     *  Node to hold the binary data of the double linked list.
     *
     **/
    struct BinaryKey;


    /** KeyLRU
    *
    *   LRU - Least Recently Used.
    *   This class is responsible for holding data that is partially processed.
    *   This class has no types, all objects are in binary forms.
    *
    **/
    class KeyLRU
    {

    protected:

        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_SIZE;


        /* The total buckets available. */
        uint32_t MAX_CACHE_BUCKETS;


        /* The current size of the pool. */
        uint32_t nCurrentSize;


        /* Mutex for thread concurrency. */
        mutable std::mutex MUTEX;


        /* Map of the current holding data. */
        std::vector<BinaryKey *> hashmap;


        /* Keep track of the first object in linked list. */
        BinaryKey* pfirst;


        /* Keep track of the last object in linked list. */
        BinaryKey* plast;



    public:

        /** Base Constructor.
         *
         *  MAX_CACHE_SIZE default value is 32 MB
         *  MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         *
         **/
        KeyLRU();


		/** Copy Constructor. **/
		KeyLRU(const KeyLRU& cache);


		/** Move Constructor. **/
		KeyLRU(KeyLRU&& cache) noexcept;


		/** Copy assignment. **/
		KeyLRU& operator=(const KeyLRU& cache);


		/** Move assignment. **/
		KeyLRU& operator=(KeyLRU&& cache) noexcept;


        /** Class Destructor. **/
        ~KeyLRU();


        /** Cache Size Constructor
         *
         *  @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         **/
        KeyLRU(uint32_t nCacheSizeIn);


        /** Add
         *
         *  Template for writing a generic key into the key LRU.
         *
         *  @param[in] key The key object to write.
         *
         **/
        template<typename KeyType>
        void Add(const KeyType& key)
        {
            /* Serialize the key object. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Add the key to the cache. */
            Put(ssKey.Bytes());
        }


        /** Ban
         *
         *  Template for banning a generic key.
         *
         *  @param[in] key The key object to write.
         *
         **/
        template<typename KeyType>
        void Ban(const KeyType& key)
        {
            /* Serialize the key object. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Add the key to the cache. */
            Penalize(ssKey.Bytes(), 86400);
        }


        /** Has
         *
         *  Template for checking for a generic key in the key LRU.
         *
         *  @param[in] key The key object to check.
         *
         **/
        template<typename KeyType>
        bool Has(const KeyType& key)
        {
            /* Serialize the key object. */
            DataStream ssKey(SER_LLD, DATABASE_VERSION);
            ssKey << key;

            /* Add the key to the cache. */
            return Get(ssKey.Bytes());
        }


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] vKey The key to get bucket for.
         *
         **/
        uint32_t Bucket(const std::vector<uint8_t>& vKey) const;


        /** Has
         *
         *  Check if data exists.
         *
         *  @param[in] vKey The binary data of the key.
         *
         *  @return True/False whether pool contains data by index.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey);


        /** RemoveNode
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         **/
        void RemoveNode(BinaryKey* pthis);


        /** MoveToFront
         *
         *  Move the node in double linked list to front.
         *
         *  @param[in] pthis The node to move to front.
         *
         **/
        void MoveToFront(BinaryKey* pthis);


        /** Put
         *
         *  Add data in the Pool
         *
         *  @param[in] vKey The key in binary form.
         *  @param[in] vData The input data in binary form.
         *
         **/
        void Put(const std::vector<uint8_t>& vKey);


        /** Penalize
         *
         *  Penalize Object by Index.
         *
         *  @param[in] vKey The key in binary form.
         *  @param[in] nPenalties The time penalties to give key. [DEFAULT: 1 day]
         *
         **/
        void Penalize(const std::vector<uint8_t>& vKey, const uint32_t nPenalties);


        /** Remove
         *
         *  Force Remove Object by Index
         *
         *  @param[in] vKey Binary Data of the Key
         *
         *  @return True on successful removal, false if it fails
         *
         **/
        bool Remove(const std::vector<uint8_t>& vKey);
    };
}

#endif
