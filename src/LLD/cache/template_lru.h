/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_CACHE_TEMPLATE_LRU_H
#define NEXUS_LLD_CACHE_TEMPLATE_LRU_H

#include <Util/templates/serialize.h>
#include <Util/include/mutex.h>
#include <Util/include/runtime.h>
#include <Util/include/hex.h>


//TODO: Abstract base class for all cache systems
namespace LLD
{

    /** Template Node
     *
     *  Node to hold the key/values of the double linked list.
     *
     **/
    template<typename KeyType, typename DataType>
    struct TemplateNode
    {
        TemplateNode<KeyType, DataType>* pprev;
        TemplateNode<KeyType, DataType>* pnext;

        KeyType Key;
        DataType Data;
    };


    /** Holding Pool:
    *
    * This class is responsible for holding data that is partially processed.
    * This class has no types, all objects are in binary forms
    *
    */
    template<typename KeyType, typename DataType>
    class TemplateLRU
    {

    protected:

        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_ELEMENTS;


        /* The total buckets available. */
        uint32_t MAX_CACHE_BUCKETS;


        /* The current size of the pool. */
        uint32_t nTotalElements;


        /* Mutex for thread concurrencdy. */
        mutable std::recursive_mutex MUTEX;


        /* Map of the current holding data. */
        std::vector<TemplateNode<KeyType, DataType>*> hashmap;


        /* Keep track of the first object in linked list. */
        TemplateNode<KeyType, DataType>* pfirst;


        /* Keep track of the last object in linked list. */
        TemplateNode<KeyType, DataType>* plast;



    public:

        /** Base Constructor.
         *
         *  MAX_CACHE_ELEMENTS default value is 256 objects
         *  MAX_CACHE_BUCKETS default value is number of elements
         *
         */
        TemplateLRU() : MAX_CACHE_ELEMENTS(256), MAX_CACHE_BUCKETS(MAX_CACHE_ELEMENTS), nTotalElements(0), pfirst(0), plast(0)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = NULL;
            plast  = NULL;
        }


        /** Total Elements Constructor
         *
         * @param[in] nTotalElementsIn The maximum size of this Cache Pool
         *
         */
        TemplateLRU(uint32_t nTotalElementsIn) : MAX_CACHE_ELEMENTS(nTotalElementsIn), MAX_CACHE_BUCKETS(MAX_CACHE_ELEMENTS), nTotalElements(0)
        {
            /* Resize the hashmap vector. */
            hashmap.resize(MAX_CACHE_BUCKETS);

            /* Set the start and end pointers. */
            pfirst = NULL;
            plast  = NULL;
        }


        /* Class Destructor. */
        ~TemplateLRU()
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

        TemplateLRU& operator=(TemplateLRU map)
        {
            MAX_CACHE_ELEMENTS = map.MAX_CACHE_ELEMENTS;
            MAX_CACHE_BUCKETS  = map.MAX_CACHE_BUCKETS;
            nTotalElements     = map.nTotalElements;

            hashmap            = map.hashmap;
            pfirst             = map.pfirst;
            plast              = map.plast;

            return *this;
        }


        TemplateLRU(const TemplateLRU& map)
        {
            MAX_CACHE_ELEMENTS = map.MAX_CACHE_ELEMENTS;
            MAX_CACHE_BUCKETS  = map.MAX_CACHE_BUCKETS;
            nTotalElements     = map.nTotalElements;

            hashmap            = map.hashmap;
            pfirst             = map.pfirst;
            plast              = map.plast;
        }


        /** Bucket
         *
         *  Find a bucket for cache key management.
         *
         *  @param[in] Key The key to get bucket for.
         *
         **/
        uint32_t Bucket(const KeyType& Key) const
        {
            /* Get the bytes from the type object. */
            std::vector<uint8_t> vKey;
            vKey.insert(vKey.end(), (uint8_t*)&Key, (uint8_t*)&Key + sizeof(Key));;

            /* Find the bucket through creating an uint64_t from available bytes. */
            uint64_t nBucket = 0;
            for(int i = 0; i < vKey.size() && i < 8; i++)
                nBucket += vKey[i] << (8 * i);

            /* Round robin to find the bucket. */
            return nBucket % MAX_CACHE_BUCKETS;
        }


        /** Check if data exists
         *
         * @param[in] Key The binary data of the key
         *
         * @return True/False whether pool contains data by index
         *
         */
        bool Has(KeyType Key) const
        {
            LOCK(MUTEX);

            uint32_t nBucket = Bucket(Key);
            return (hashmap[nBucket] != NULL && hashmap[nBucket]->Key == Key);
        }


        /** Remove Node
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         */
        void RemoveNode(TemplateNode<KeyType, DataType>* pthis)
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
        void MoveToFront(TemplateNode<KeyType, DataType>* pthis)
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
         * @param[in] Key The key type
         * @param[out] Data The data type
         *
         * @return True if object was found, false if none found by index.
         *
         */
        bool Get(KeyType Key, DataType& Data)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(!Has(Key))
                return false;

            /* Get the data. */
            TemplateNode<KeyType, DataType>* pthis = hashmap[Bucket(Key)];

            /* Get the data. */
            Data = pthis->Data;

            /* Move to front of double linked list. */
            MoveToFront(pthis);

            return true;
        }


        /** Add data in the Pool
         *
         * @param[in] Key The key type object
         * @param[in] Data The data type object
         *
         */
        void Put(KeyType Key, DataType Data)
        {
            LOCK(MUTEX);

            /* If has a key, check for bucket collisions. */
            uint32_t nBucket = Bucket(Key);

            /* Check for bucket collisions. */
            TemplateNode<KeyType, DataType>* pthis = NULL;
            if(hashmap[nBucket] != NULL)
            {
                /* Update the cache node. */
                pthis = hashmap[nBucket];
                pthis->Data = Data;
                pthis->Key  = Key;
            }
            else
            {
                /* Create a new cache node. */
                pthis = new TemplateNode<KeyType, DataType>();
                pthis->Data = Data;
                pthis->Key  = Key;

                /* Add cache node to objects map. */
                hashmap[nBucket] = pthis;
            }

            /* Set the new cache node to the front */
            MoveToFront(pthis);

            /* Remove the last node if cache too large. */
            if(nTotalElements > MAX_CACHE_ELEMENTS)
            {
                /* Get the last key. */
                if(plast && plast->pprev)
                {
                    TemplateNode<KeyType, DataType>* pnode = plast;

                    /* Relink in memory. */
                    plast = plast->pprev;
                    plast->pnext = NULL;

                    /* Reset hashmap pointer */
                    hashmap[Bucket(pnode->Key)] = NULL; //TODO: hashmap linked list for collisions
                    pnode->pnext = NULL;
                    pnode->pprev = NULL;
                    pnode = NULL;

                    /* Free memory. */
                    Delete(pnode->Key);
                    Delete(pnode->Data);
                    Delete(pnode);
                }
            }
            else
                nTotalElements++;
        }


        /** Delete
         *
         *  Template function to delete pointer.
         *  This one is dummy non pointer catch
         *
         **/
        template<typename Type>
        void Delete(Type data) {}


        /** Delete
         *
         *  Template function to delete pointer.
         *  This one is to aid compile time deletes.
         *
         **/
        template<typename Type>
        void Delete(const Type* data) { delete data; }


        /** Force Remove Object by Index
         *
         * @param[in] Key Binary Data of the Key
         *
         * @return True on successful removal, false if it fails
         *
         */
        bool Remove(KeyType Key)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(!Has(Key))
                return false;

            /* Get the node */
            TemplateNode<KeyType, DataType>* pnode = hashmap[Bucket(Key)];

            /* Reduce the current cache size. */
            nTotalElements --;

            /* Remove from linked list. */
            RemoveNode(pnode);

            /* Remove the object from the map. */
            hashmap[Bucket(Key)] = NULL;

            /* Free memory. */
            Delete(pnode->Key);
            Delete(pnode->Data);
            Delete(pnode);

            return true;
        }
    };
}

#endif
