/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_TEMPLATES_SHARD_HASHMAP_H
#define NEXUS_LLD_TEMPLATES_SHARD_HASHMAP_H

#include <LLD/templates/key.h>
#include <LLD/cache/template_lru.h>
#include <LLD/include/enum.h>

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>

//TODO: Abstract base class for all keychains
namespace LLD
{

    /** ShardHashMap
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1).
     *  It uses a linked file list based on index to iterate through files and binary Positions
     *  when there is a collision that is found.
     *
     **/
    class ShardHashMap
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /** The string to hold the database location. **/
        std::string strBaseLocation;


        /** Keychain stream object. **/
        TemplateLRU<std::pair<uint16_t, uint16_t>, std::fstream*>* fileCache;


        /** Disk index shard LRU cache **/
        TemplateLRU<uint16_t, std::vector<uint16_t>*>* diskShards;


        /** Keychain index stream. **/
        TemplateLRU<uint16_t, std::fstream*>* indexCache;


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum shards allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_SHARDS;


        /** The Maximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /** The keychain flags. **/
        uint8_t nFlags;


        /* The key level locking hashmap. */
        mutable std::vector<std::mutex> RECORD_MUTEX;


    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        ShardHashMap(const std::string& strBaseLocationIn, const uint8_t nFlagsIn = FLAGS::APPEND,
            const uint64_t nBucketsIn = 256 * 256 * 64, const uint32_t nShardsIn = 4);


        /** Copy Assignment Operator **/
        ShardHashMap& operator=(const ShardHashMap& map);


        /** Copy Constructor **/
        ShardHashMap(const ShardHashMap& map);


        /** Default Destructor **/
        ~ShardHashMap();


        /** CompressKey
         *
         *  Compresses a given key until it matches size criteria.
         *  This function is one way and efficient for reducing key sizes.
         *
         *  @param[out] vData The binary data of key to compress.
         *  @param[in] nSize The desired size of key after compression.
         *
         **/
        void CompressKey(std::vector<uint8_t>& vData, uint16_t nSize = 32);


        /** GetBucket
         *
         *  Calculates a bucket to be used for the hashmap allocation.
         *
         *  @param[in] vKey The key object to calculate with.
         *  @param[in] nShard The shard assigned to hashmap object.
         *
         *  @return The bucket assigned to the key.
         *
         **/
        uint32_t GetBucket(const std::vector<uint8_t>& vKey, uint32_t& nShard);


        /** LoadShardIndex
         *
         *  Loads a disk index containing shard data into memory.
         *
         *  @param[in] The shard to load index for.
         *
         **/
        void LoadShardIndex(const uint32_t nShard);


        /** Initialize
         *
         *  Initialize the binary hash map keychain.
         *
         **/
        void Initialize();


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *
         *  @param[in] vKey The binary data of key.
         *  @param[out] cKey The key object to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, SectorKey &cKey);


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *  This method iterates all maps to find all keys.
         *
         *  @param[in] vKey The binary data of the key.
         *  @param[out] vKeys The list of keys to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, std::vector<SectorKey>& vKeys);


        /** Put
         *
         *  Write a key to the disk hashmaps.
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        bool Put(const SectorKey& cKey);


        /** Restore
         *
         *  Restore an erased key from keychain.
         *
         *  @param[in] vKey the key to restore.
         *
         *  @return True if the key was restored.
         *
         **/
        bool Restore(const std::vector<uint8_t> &vKey);


        /** Erase
         *
         *  Erase a key from the disk hashmaps.
         *  TODO: This should be optimized further.
         *
         *  @param[in] vKey the key to erase.
         *
         *  @return True if the key was erased, false otherwise.
         *
         **/
        bool Erase(const std::vector<uint8_t> &vKey);
    };
}

#endif
