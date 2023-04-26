/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_CACHE_BINARY_LRU_H
#define NEXUS_LLD_CACHE_BINARY_LRU_H

#include <mutex>
#include <cstdint>
#include <vector>


//TODO: Abstract base class for all cache systems
namespace LLD
{
    class SectorKey;


    /** BinaryNode
     *
     *  Node to hold the binary data of the double linked list.
     *
     **/
    struct BinaryNode;


    /** BinaryLRU
    *
    *   LRU - Least Recently Used.
    *   This class is responsible for holding data that is partially processed.
    *   This class has no types, all objects are in binary forms.
    *
    **/
    class BinaryLRU
    {
        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_SIZE;


        /* The total buckets available. */
        uint32_t MAX_CACHE_BUCKETS;


        /* The current size of the pool. */
        uint32_t nCurrentSize;


        /* Mutex for thread concurrency. */
        mutable std::mutex MUTEX;


        /* Map of the current holding data. */
        std::vector<BinaryNode*> hashmap;


        /* Map of the current data indexes. */
        std::vector<uint64_t> indexes;


        /* Keep track of the first object in linked list. */
        BinaryNode* pfirst;


        /* Keep track of the last object in linked list. */
        BinaryNode* plast;


    public:


        /** Default Constructor. **/
        BinaryLRU()                                  = delete;


		/** Copy Constructor. **/
		BinaryLRU(const BinaryLRU& cache)            = delete;


		/** Move Constructor. **/
		BinaryLRU(BinaryLRU&& cache)                 = delete;


		/** Copy assignment. **/
		BinaryLRU& operator=(const BinaryLRU& cache) = delete;


		/** Move assignment. **/
		BinaryLRU& operator=(BinaryLRU&& cache)      = delete;


        /** Class Destructor. **/
        ~BinaryLRU();


        /** Cache Size Constructor
         *
         *  @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         **/
        BinaryLRU(const uint32_t nCacheSizeIn);


        /** Has
         *
         *  Check if data exists.
         *
         *  @param[in] vKey The binary data of the key.
         *
         *  @return True/False whether pool contains data by index.
         *
         **/
        bool Has(const std::vector<uint8_t>& vKey) const;


        /** Get
         *
         *  Get the data by index
         *
         *  @param[in] vKey The binary data of the key.
         *  @param[out] vData The binary data of the cached record.
         *
         *  @return True if object was found, false if none found by index.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData);


        /** Put
         *
         *  Add data in the Pool
         *
         *  @param[in] vKey The key in binary form.
         *  @param[in] vData The input data in binary form.
         *  @param[in] fReserve Flag for if item should be saved from cache eviction.
         *
         **/
        void Put(const SectorKey& key, const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData, bool fReserve = false);


        /** Reserve
         *
         *  Reserve this item in the cache permanently if true, unreserve if false
         *
         *  @param[in] vKey The key to flag as reserved true/false
         *  @param[in] fReserve If this object is to be reserved for disk.
         *
         **/
        void Reserve(const std::vector<uint8_t>& vKey, bool fReserve = true);


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


    private:

        /** RemoveNode
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         **/
        void remove_node(BinaryNode* pthis);


        /** MoveToFront
         *
         *  Move the node in double linked list to front.
         *
         *  @param[in] pthis The node to move to front.
         *
         **/
        void move_to_front(BinaryNode* pthis);


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] vKey The key to get bucket for.
         *
         **/
        uint32_t bucket(const std::vector<uint8_t>& vKey) const;


        /** Bucket
         *
         *  Find a bucket for checksum key management.
         *
         *  @param[in] nIndex The checksum to get bucket for.
         *
         **/
        uint32_t slot(const uint64_t nIndex) const;
    };
}

#endif
