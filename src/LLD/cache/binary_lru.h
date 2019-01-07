/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_CACHE_BINARY_LRU_H
#define NEXUS_LLD_CACHE_BINARY_LRU_H

#include <Util/templates/serialize.h>
#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/hex.h>

#include <openssl/md5.h>


//TODO: Abstract base class for all cache systems
namespace LLD
{

    /** Binary Node
     *
     *  Node to hold the binary data of the double linked list.
     *
     **/
    struct BinaryNode
    {
        BinaryNode* pprev;
        BinaryNode* pnext;

        std::vector<uint8_t> vKey;
        std::vector<uint8_t> vData;

        bool fReserve;
    };


    /** Holding Pool:
    *
    * This class is responsible for holding data that is partially processed.
    * This class has no types, all objects are in binary forms
    *
    */
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
        mutable std::recursive_mutex MUTEX;


        /* Map of the current holding data. */
        std::vector<BinaryNode*> hashmap;


        /* Keep track of the first object in linked list. */
        BinaryNode* pfirst;


        /* Keep track of the last object in linked list. */
        BinaryNode* plast;



    public:

        /** Base Constructor.
         *
         * MAX_CACHE_SIZE default value is 32 MB
         * MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         *
         */
        BinaryLRU() : MAX_CACHE_SIZE(1024 * 1024), MAX_CACHE_BUCKETS(MAX_CACHE_SIZE / 32), nCurrentSize(MAX_CACHE_BUCKETS * 8), pfirst(0), plast(0)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = nullptr;
            plast  = nullptr;
        }


        /** Cache Size Constructor
         *
         * @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         */
        BinaryLRU(uint32_t nCacheSizeIn) : MAX_CACHE_SIZE(nCacheSizeIn), MAX_CACHE_BUCKETS(nCacheSizeIn / 32), nCurrentSize(MAX_CACHE_BUCKETS * 8)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = nullptr;
            plast  = nullptr;
        }


        /* Class Destructor. */
        ~BinaryLRU()
        {
            /* Loop through the linked list. */
            for(auto & item : hashmap)
                if(item)
                    delete item;
        }


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] vKey The key to get bucket for.
         *
         **/
        uint32_t Bucket(const std::vector<uint8_t>& vKey) const
        {
            /* Get an MD5 digest. */
            uint8_t digest[MD5_DIGEST_LENGTH];
            MD5((unsigned char*)&vKey[0], vKey.size(), (unsigned char*)&digest);

            /* Copy bytes into the bucket. */
            uint64_t nBucket;
            std::copy((uint8_t*)&digest[0], (uint8_t*)&digest[0] + 8, (uint8_t*)&nBucket);

            return nBucket % MAX_CACHE_BUCKETS;
        }


        /** Check if data exists
         *
         * @param[in] vKey The binary data of the key
         *
         * @return True/False whether pool contains data by index
         *
         */
        bool Has(std::vector<uint8_t> vKey) const
        {
            LOCK(MUTEX);

            uint32_t nBucket = Bucket(vKey);
            return (hashmap[nBucket] != nullptr && hashmap[nBucket]->vKey == vKey);
        }


        /** Remove Node
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         */
        void RemoveNode(BinaryNode* pthis)
        {
            LOCK(MUTEX);

            /* Link the next pointer if not null */
            if(pthis->pnext)
                pthis->pnext->pprev = pthis->pprev;

            /* Link the previous pointer if not null. */
            if(pthis->pprev)
                pthis->pprev->pnext = pthis->pnext;
        }


        /** Move to Front
         *
         *  Move the node in double linked list to front.
         *
         *  @param[in] pthis The node to move to front.
         *
         **/
        void MoveToFront(BinaryNode* pthis)
        {
            LOCK(MUTEX);

            /* Don't move to front if already in the front. */
            if(pthis == pfirst)
                return;

            /* Move last pointer if moving from back. */
            if(pthis == plast)
            {
                if(plast->pprev)
                    plast = plast->pprev;

                plast->pnext = nullptr;
            }
            else
                RemoveNode(pthis);

            /* Set prev to null to signal front of list */
            pthis->pprev = nullptr;

            /* Set next to the current first */
            pthis->pnext = pfirst;

            /* Update the first reference prev */
            if(pfirst)
            {
                pfirst->pprev = pthis;
                if(!plast)
                {
                    plast = pfirst;
                    plast->pnext = nullptr;
                }
            }

            /* Update the first reference. */
            pfirst        = pthis;
        }


        /** Get the data by index
         *
         * @param[in] vKey The binary data of the key
         * @param[out] vData The binary data of the cached record
         *
         * @return True if object was found, false if none found by index.
         *
         */
        bool Get(std::vector<uint8_t> vKey, std::vector<uint8_t>& vData)
        {
            LOCK(MUTEX);

            /* Get the data. */
            BinaryNode* pthis = hashmap[Bucket(vKey)];

            /* Check if the Record Exists. */
            if (pthis == nullptr || pthis->vKey != vKey)
                return false;

            /* Get the data. */
            vData = pthis->vData;

            /* Move to front of double linked list. */
            if(!pthis->fReserve)
                MoveToFront(pthis);

            return true;
        }


        /** Add data in the Pool
         *
         * @param[in] vKey The key in binary form
         * @param[in] vData The input data in binary form
         *
         */
        void Put(std::vector<uint8_t> vKey, std::vector<uint8_t> vData, bool fReserve = false)
        {
            LOCK(MUTEX);

            /* If has a key, check for bucket collisions. */
            uint32_t nBucket = Bucket(vKey);

            /* Check for bucket collisions. */
            BinaryNode* pthis = nullptr;
            if(hashmap[nBucket] != nullptr)
            {
                /* Update the cache node. */
                pthis = hashmap[nBucket];
                pthis->vData    = vData;
                pthis->vKey     = vKey;
                pthis->fReserve = fReserve;
            }
            else
            {
                /* Create a new cache node. */
                pthis = new BinaryNode();
                pthis->vData    = vData;
                pthis->vKey     = vKey;
                pthis->fReserve = fReserve;

                /* Add cache node to objects map. */
                hashmap[nBucket] = pthis;
            }

            /* Set the new cache node to the front */
            if(!pthis->fReserve)
                MoveToFront(pthis);

            /* Set the new cache size. */
            nCurrentSize += (vData.size() + vKey.size());

            /* Remove the last node if cache too large. */
            while(nCurrentSize > MAX_CACHE_SIZE)
            {
                /* Get the last key. */
                if(plast && plast->pprev)
                {
                    BinaryNode* pnode = plast;

                    /* Relink in memory. */
                    plast = plast->pprev;

                    if(plast && plast->pnext)
                        plast->pnext = nullptr;
                    else
                        printf("plast is nulll\n");

                    /* Reduce the current cache size. */
                    if(pnode)
                    {
                        nCurrentSize -= (pnode->vData.size() - pnode->vKey.size());

                        /* Clear the pointers. */
                        hashmap[Bucket(pnode->vKey)] = nullptr; //TODO: hashmap linked list for collisions

                        /* Reset the memory linking. */
                        pnode->pprev = nullptr;
                        pnode->pnext = nullptr;

                        /* Free the memory */
                        delete pnode;
                    }

                    pnode = nullptr;

                    continue;
                }

                break;
            }
        }


        /** Reserve
         *
         *  Reserve this item in the cache permanently if true, unreserve if false
         *
         *  @param[in] vKey The key to flag as reserved true/false
         *  @param[in] fDisk If this object is to be reserved for disk.
         *
         */
        void Reserve(std::vector<uint8_t> vKey, bool fReserve = true)
        {
            LOCK(MUTEX);

            /* Get the data. */
            BinaryNode* pthis = hashmap[Bucket(vKey)];

            /* Check if the Record Exists. */
            if (pthis == nullptr || pthis->vKey != vKey)
                return;

            /* Set object to reserved. */
            pthis->fReserve = fReserve;

            /* Move back to linked list. */
            if(!fReserve)
            {
                /* Set prev to null to signal front of list */
                pthis->pprev = nullptr;

                /* Set next to the current first */
                pthis->pnext = pfirst;

                /* Set the first pointer. */
                pfirst = pthis;
            }
        }


        /** Force Remove Object by Index
         *
         * @param[in] vKey Binary Data of the Key
         *
         * @return True on successful removal, false if it fails
         *
         */
        bool Remove(std::vector<uint8_t> vKey)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(!Has(vKey))
                return false;

            /* Get the node */
            BinaryNode* pnode = hashmap[Bucket(vKey)];

            /* Reduce the current cache size. */
            nCurrentSize -= (pnode->vData.size() + vKey.size());

            /* Remove from linked list. */
            RemoveNode(pnode);

            /* Remove the object from the map. */
            hashmap[Bucket(vKey)] = nullptr;
            delete pnode;

            return true;
        }
    };
}

#endif
