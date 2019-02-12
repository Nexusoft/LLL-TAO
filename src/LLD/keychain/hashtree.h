/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_HASHTREE_H
#define NEXUS_LLD_TEMPLATES_HASHTREE_H

#include <LLD/cache/template_lru.h>

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <thread>


//TODO: Abstract base class for all keychains
namespace LLD
{

    /** Forward declarations **/
    class SectorKey;


    /** BinaryHashTree
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Tree with a search complexity of O(log n)
     *  It uses files as leafs or nodes when collisions are found in buckets
     *  making it a hybrid hashmap and binary search tree
     *
     **/
    class BinaryHashTree
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /** The string to hold the database location. **/
        std::string strBaseLocation;


        /** The Maximum buckets allowed in the hashmap. */
        uint32_t HASHMAP_TOTAL_BUCKETS;


        /** The Maximum cache size for allocated keys. **/
        uint32_t HASHMAP_MAX_CACHE_SZIE;


        /** The Maximum key size for static key sectors. **/
        uint16_t HASHMAP_MAX_KEY_SIZE;


        /** The total space that a key consumes. */
        uint16_t HASHMAP_KEY_ALLOCATION;


        /** Initialized flag (used for cache thread) **/
        bool fInitialized;


        /** Keychain stream object. **/
        mutable TemplateLRU<uint32_t, std::fstream *> *fileCache;


        /** Total elements in hashmap for quick inserts. **/
        mutable std::vector<uint32_t> hashmap;


        /* The cache writer thread. */
        std::thread CacheThread;


    public:

        /** Default Constructor **/
        BinaryHashTree();


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashTree(std::string strBaseLocationIn);


        /** Default Destructor **/
        ~BinaryHashTree();


        /** GetBucket
         *
         *  Handle the Assigning of a Map Bucket.
         *
         *  @param[in] vKey The key to obtain a bucket id from.
         *
         *  @return Returns the bucket id.
         *
         **/
        uint32_t GetBucket(const std::vector<uint8_t> &vKey) const;


        /** Initialize
         *
         *  Read the Database Keys and File Positions.
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
        bool Get(const std::vector<uint8_t> vKey, SectorKey& cKey);


        /** Put
         *
         *  Write a key to the disk hashmaps.
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        bool Put(const SectorKey &cKey) const;


        /** CacheWriter
         *
         *  Helper Thread to Batch Write to Disk.
         *
         **/
        void CacheWriter();


        /** Erase
         *
         *  Simple Erase for now, not efficient in Data Usage of HD but quick to
         *  get erase function working.
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
