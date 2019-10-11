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
        std::vector<BinaryNode*> hashmap;


        /* Map of the current data checksum. */
        std::vector<uint64_t> checksums;


        /* Keep track of the first object in linked list. */
        BinaryNode* pfirst;


        /* Keep track of the last object in linked list. */
        BinaryNode* plast;


        /* Keep track of pool of instantiated nodes that can be reused. */
        BinaryNode* pPool;



    public:

        /** Base Constructor.
         *
         *  MAX_CACHE_SIZE default value is 32 MB
         *  MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         *
         **/
        BinaryLRU();


        /** Cache Size Constructor
         *
         *  @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         **/
        BinaryLRU(uint32_t nCacheSizeIn);


        /** Class Destructor. **/
        ~BinaryLRU();


        /** Bucket
         *
         *  Get the checksum of a data object.
         *
         *  @param[in] vData The data to get checksum of.
         *
         **/
        uint64_t Checksum(const std::vector<uint8_t>& vData) const;


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] key The sector key to find bucket for.
         *
         **/
        uint32_t Bucket(const SectorKey& key) const;


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] vKey The key to get bucket for.
         *
         **/
        uint32_t Bucket(const std::vector<uint8_t>& vKey) const;


        /** Bucket
         *
         *  Find a bucket for checksum key management.
         *
         *  @param[in] nChecksum The checksum to get bucket for.
         *
         **/
        uint32_t Bucket(const uint64_t nChecksum) const;


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


        /** UnlinkNode
         *
         *  Unlinks a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         **/
        void UnlinkNode(BinaryNode* pthis);


        /** MoveToFront
         *
         *  Move the node in double linked list to front.
         *
         *  @param[in] pthis The node to move to front.
         *
         **/
        void MoveToFront(BinaryNode* pthis);


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


        /** AddNode
        *
        *  Creates a new node for the cache.  If there is a node in the pool of previously instantated instances then it will be
        *  used, otherwise a new instance will be instantiated
        *
        *  @param[in] vKey The key in binary form.
        *  @param[in] vData The input data in binary form.
        *
        *  @return The newly added node.
        *
        **/
        BinaryNode* AddNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn);


        /** RemoveNode
        *
        *  Removes a node from the cache and adds it to the node pool for resuse
        *
        *  @param[in] pnode The node to be removed
        *  @param[in] nChecksumBucket Index into the checksum buckets for this node's checksum
        *  @param[in] nHashMapBucket Index into the hashmap bucket for this node
        *
        **/
        void RemoveNode(BinaryNode* pnode, uint32_t nChecksumBucket, uint64_t nHashMapBucket);
    };
}

#endif
