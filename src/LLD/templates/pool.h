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

namespace LLD
{

    struct CacheNode
    {
        CacheNode* pprev;
        CacheNode* pnext;

        std::vector<uint8_t> vKey;
        std::vector<uint8_t> vData;
    };


    /** Holding Pool:
    *
    * This class is responsible for holding data that is partially processed.
    * It is also uselef for data that needs to be relayed from cache once recieved.
    *
    */
    class MemCachePool
    {

    protected:

        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_SIZE;


        /* The current size of the pool. */
        uint32_t nCurrentSize;


        /* Mutex for thread concurrencdy. */
        Mutex_t MUTEX;


        /* Map of the current holding data. */
        std::map<std::vector<uint8_t>, CacheNode*> mapObjects;


        /* Keep track of the first object in linked list. */
        CacheNode* pfirst;


        /* Keep track of the last object in linked list. */
        CacheNode* plast;



    public:

        /** Base Constructor.
         *
         * MAX_CACHE_SIZE default value is 32 MB
         * MAX_CACHE_BUCKETS default value is 65,539 (2 bytes)
         *
         */
        MemCachePool() : MAX_CACHE_SIZE(1024 * 1024), nCurrentSize(0), pfirst(0), plast(0)
        {
            pfirst = NULL;
            plast  = NULL;
        }


        /** Cache Size Constructor
         *
         * @param[in] nCacheSizeIn The maximum size of this Cache Pool
         *
         */
        MemCachePool(uint32_t nCacheSizeIn) : MAX_CACHE_SIZE(nCacheSizeIn), nCurrentSize(0)
        {
            pfirst = NULL;
            plast  = NULL;
        }


        /* Class Destructor. */
        ~MemCachePool()
        {
            //TODO: delete the cache nodes
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
            return (mapObjects.count(vKey) > 0);
        }


        /** Remove Node
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         */
        void RemoveNode(CacheNode* pthis)
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
        void MoveToFront(CacheNode* pthis)
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
                pfirst->pprev = pthis;

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

            /* Check if the Record Exists. */
            if(!Has(vKey))
                return false;

            /* Get the data. */
            vData = mapObjects[vKey]->vData;

            /* Move to front of double linked list. */
            MoveToFront(mapObjects[vKey]);

            return true;
        }


        /** Add data in the Pool
         *
         * @param[in] vKey The key in binary form
         * @param[in] vData The input data in binary form
         *
         */
        void Put(std::vector<uint8_t> vKey, std::vector<uint8_t> vData)
        {
            LOCK(MUTEX);

            /* If has a key, update that in map. */
            if(Has(vKey))
                mapObjects[vKey]->vData = vData;
            else
            {
                /* Create a new cache node. */
                CacheNode* pthis = new CacheNode();
                pthis->vData = vData;
                pthis->vKey  = vKey;

                /* Add cache node to objects map. */
                mapObjects[vKey] = pthis;

                /* Check for last. */
                if(!plast)
                    plast = pthis;

                if(!pfirst)
                    pfirst = pthis;
                else
                {
                    pfirst->pprev = pthis;
                    pthis->pnext  = pfirst;
                    pfirst        = pthis;
                }

            }


            /* Remove the last node if cache too large. */
            if(nCurrentSize > MAX_CACHE_SIZE)
            {
                /* Get the last key. */
                CacheNode* pnode = plast;

                /* Relink in memory. */
                plast = plast->pprev;
                plast->pnext = NULL;

                /* Reduce the current cache size. */
                nCurrentSize += (vData.size() - pnode->vData.size());

                /* Clear the pointers. */
                mapObjects.erase(vKey);
                delete pnode;
            }
            else
                nCurrentSize += vData.size();
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

            /* Reduce the current cache size. */
            nCurrentSize -= (mapObjects[vKey]->vData.size() + vKey.size());

            /* Get the cache node. */
            CacheNode* pnode = mapObjects[vKey];

            /* Remove from linked list. */
            RemoveNode(pnode);

            /* Remove the object from the map. */
            mapObjects.erase(vKey);

            /* Erase the pointer. */
            delete pnode;

            return true;
        }
    };
}

#endif
