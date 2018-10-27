/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_MEMCACHEPOOL_H
#define NEXUS_LLD_TEMPLATES_MEMCACHEPOOL_H

#include <Util/templates/serialize.h>
#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/hex.h>


//TODO: Abstract base class for all cache systems
namespace LLD
{

    template<typename DataType> struct CacheNode
    {
        CacheNode<DataType>* pprev;
        CacheNode<DataType>* pnext;

        std::vector<uint8_t> vKey;

        DataType vData;
    };


    /** Holding Pool:
    *
    * This class is responsible for holding data that is partially processed.
    * It is also uselef for data that needs to be relayed from cache once recieved.
    *
    */
    template<typename DataType>
    class MemCachePool
    {

    protected:

        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_SIZE;


        /* The total buckets available. */
        uint32_t MAX_CACHE_BUCKETS;


        /* The current size of the pool. */
        uint32_t nCurrentSize;


        /* Mutex for thread concurrencdy. */
        mutable Mutex_t MUTEX;


        /* Map of the current holding data. */
        std::vector<CacheNode<DataType>*> hashmap;


        /* Keep track of the first object in linked list. */
        CacheNode<DataType>* pfirst;


        /* Keep track of the last object in linked list. */
        CacheNode<DataType>* plast;



    public:

        /** Base Constructor.
         *
         * MAX_CACHE_SIZE default value is 32 MB
         * MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         *
         */
        MemCachePool() : MAX_CACHE_SIZE(1024 * 1024), MAX_CACHE_BUCKETS(MAX_CACHE_SIZE / 64), nCurrentSize(MAX_CACHE_BUCKETS * 24), pfirst(0), plast(0)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = NULL;
            plast  = NULL;
        }


        /** Cache Size Constructor
         *
         * @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         */
        MemCachePool(uint32_t nCacheSizeIn) : MAX_CACHE_SIZE(nCacheSizeIn), MAX_CACHE_BUCKETS(nCacheSizeIn / 64), nCurrentSize(MAX_CACHE_BUCKETS * 24)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = NULL;
            plast  = NULL;
        }


        /* Class Destructor. */
        ~MemCachePool()
        {
            /* Loop through the linked list. */
            while(pfirst)
            {
                /* Free memory of previous entry. */
                if(pfirst->pprev)
                    delete pfirst->pprev;

                /* Iterate forward */
                pfirst = pfirst->pnext;
            }
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
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

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
            return (hashmap[nBucket] != NULL && hashmap[nBucket]->vKey == vKey);
        }


        /** Remove Node
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         */
        void RemoveNode(CacheNode<DataType>* pthis)
        {
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
        void MoveToFront(CacheNode<DataType>* pthis)
        {
            /* Don't move to front if already in the front. */
            if(pthis == pfirst)
                return;

            /* Remove the node from linked list. */
            RemoveNode(pthis);

            /* Set prev to null to signal front of list */
            pthis->pprev = NULL;

            /* Set next to the current first */
            pthis->pnext = pfirst;

            /* Update the first reference prev */
            if(pfirst)
            {
                pfirst->pprev = pthis;
                if(!plast)
                    plast = pfirst;
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
        bool Get(std::vector<uint8_t> vKey, DataType& vData)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(!Has(vKey))
                return false;

            /* Get the data. */
            CacheNode<DataType>* pthis = hashmap[Bucket(vKey)];

            /* Get the data. */
            vData = pthis->vData;

            /* Move to front of double linked list. */
            MoveToFront(pthis);

            return true;
        }


        /** Add data in the Pool
         *
         * @param[in] vKey The key in binary form
         * @param[in] vData The input data in binary form
         *
         */
        void Put(std::vector<uint8_t> vKey, DataType vData)
        {
            LOCK(MUTEX);

            /* If has a key, check for bucket collisions. */
            uint32_t nBucket = Bucket(vKey);

            /* Check for bucket collisions. */
            CacheNode<DataType>* pthis = NULL;
            if(Has(vKey))
            {
                /* Update the cache node. */
                pthis = hashmap[nBucket];
                pthis->vData = vData;
                pthis->vKey  = vKey;
            }
            else
            {
                /* Create a new cache node. */
                pthis = new CacheNode<DataType>();
                pthis->vData = vData;
                pthis->vKey  = vKey;

                /* Add cache node to objects map. */
                hashmap[nBucket] = pthis;
            }

            /* Set the new cache node to the front */
            MoveToFront(pthis);

            /* Remove the last node if cache too large. */
            if(nCurrentSize > MAX_CACHE_SIZE)
            {
                /* Get the last key. */
                if(plast->pprev)
                {
                    CacheNode<DataType>* pnode = plast;

                    /* Relink in memory. */
                    plast = plast->pprev;
                    plast->pnext = NULL;

                    /* Reduce the current cache size. */
                    if (std::is_same<DataType, std::vector<uint8_t>>::value)
                        nCurrentSize += (vKey.size() + vData.size() - pnode->vData.size() - pnode->vKey.size());
                    else
                        nCurrentSize += (vKey.size() + sizeof(vData) - pnode->vKey.size() - sizeof(pnode->vData));


                    /* Clear the pointers. */
                    hashmap[Bucket(pnode->vKey)] = NULL; //TODO: hashmap linked list for collisions
                    delete pnode;
                }
            }
            else if (std::is_same<DataType, std::vector<uint8_t>>::value)
                nCurrentSize += (vData.size() + vKey.size());
            else
                nCurrentSize += (sizeof(vData) + vKey.size());
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
            CacheNode<DataType>* pnode = hashmap[Bucket(vKey)];

            /* Reduce the current cache size. */
            nCurrentSize -= (pnode->vData.size() + vKey.size());

            /* Remove from linked list. */
            RemoveNode(pnode);

            /* Remove the object from the map. */
            hashmap[Bucket(vKey)] = NULL;
            delete pnode;

            return true;
        }
    };
}

#endif
