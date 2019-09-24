/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_CACHE_TEMPLATE_LRU_H
#define NEXUS_LLD_CACHE_TEMPLATE_LRU_H

#include <LLD/hash/xxh3.h>

#include <Util/include/mutex.h>

#include <cstdint>
#include <vector>
#include <map>


//TODO: Abstract base class for all cache systems
namespace LLD
{


    /** TemplateNode
     *
     *  Node to hold the key/values of the double linked list.
     *
     **/
    template<typename KeyType, typename DataType>
    struct TemplateNode
    {
        /** Delete
         *
         *  Template function to delete pointer.
         *  This one is dummy non pointer catch
         *
         **/
        template<typename Type>
        void Delete(Type data)
        { }


        /** Delete
         *
         *  Delete a pointer object. Closes the file.
         *
         **/
        template<typename Type>
        void Delete(Type* data)
        {
            if(data)
                delete data;
        }

        TemplateNode(const KeyType& KeyIn, const DataType& DataIn)
        : pprev(nullptr)
        , pnext(nullptr)
        , Key(KeyIn)
        , Data(DataIn)
        {
        }

        /** Destructor. **/
        ~TemplateNode()
        {
            Delete(Key);
            Delete(Data);
        }

        TemplateNode<KeyType, DataType> *pprev;
        TemplateNode<KeyType, DataType> *pnext;

        KeyType  Key;
        DataType Data;
    };


    /** TemplateLRU
    *
    *   LRU - Least Recently Used.
    *   This class is responsible for holding data that is partially processed.
    *   This class has no types, all objects are in binary forms.
    *
    **/
    template<typename KeyType, typename DataType>
    class TemplateLRU
    {

    protected:

        /* The Maximum Size of the Cache. */
        uint32_t MAX_CACHE_ELEMENTS;


        /* Mutex for thread concurrency. */
        mutable std::mutex MUTEX;


        /* Map of the current holding data. */
        std::map<KeyType, TemplateNode<KeyType, DataType>* > cache;


    public:

        /* Keep track of the first object in linked list. */
        TemplateNode<KeyType, DataType>* pfirst;


        /* Keep track of the last object in linked list. */
        TemplateNode<KeyType, DataType>* plast;


        /** Base Constructor.
         *
         *  MAX_CACHE_ELEMENTS default value is 256 objects
         *  MAX_CACHE_BUCKETS default value is number of elements
         *
         **/
        TemplateLRU()
        : MAX_CACHE_ELEMENTS(256)
        , MUTEX()
        , cache()
        , pfirst(0)
        , plast(0)
        {
            /* Set the start and end pointers. */
            pfirst = nullptr;
            plast  = nullptr;
        }


        /** Total Elements Constructor
         *
         * @param[in] nTotalElementsIn The maximum size of this Cache Pool
         *
         **/
        TemplateLRU(uint32_t nTotalElementsIn)
        : MAX_CACHE_ELEMENTS(nTotalElementsIn)
        , MUTEX()
        , cache()
        , pfirst(0)
        , plast(0)
        {
            /* Set the start and end pointers. */
            pfirst = nullptr;
            plast  = nullptr;
        }


        /** Class Destructor. **/
        ~TemplateLRU()
        {
            /* Loop through the linked list. */
            for(auto & item : cache)
            {
                if(item.second)
                    delete item.second;
            }
        }

        /** Copy assignment operator **/
        TemplateLRU& operator=(TemplateLRU map)
        {
            MAX_CACHE_ELEMENTS = map.MAX_CACHE_ELEMENTS;

            cache              = map.cache;
            pfirst             = map.pfirst;
            plast              = map.plast;

            return *this;
        }


        /** Copy Constructor **/
        TemplateLRU(const TemplateLRU& map)
        {
            MAX_CACHE_ELEMENTS = map.MAX_CACHE_ELEMENTS;

            cache              = map.cache;
            pfirst             = map.pfirst;
            plast              = map.plast;
        }


        /** Has
         *
         *  Check if data exists.
         *
         *  @param[in] Key The binary data of the key.
         *
         *  @return True/False whether pool contains data by index.
         *
         **/
        bool Has(const KeyType& Key) const
        {
            LOCK(MUTEX);

            return cache.count(Key);
        }


        /** RemoveNode
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         **/
        void RemoveNode(TemplateNode<KeyType, DataType>* pthis)
        {
            /* Relink last pointer. */
            if(plast && pthis == plast)
            {
                plast = plast->pprev;

                if(plast)
                    plast->pnext = nullptr;
            }

            /* Relink first pointer. */
            if(pfirst && pthis == pfirst)
            {
                pfirst = pfirst->pnext;

                if(pfirst)
                    pfirst->pprev = nullptr;
            }

            /* Link the next pointer if not null */
            if(pthis->pnext)
                pthis->pnext->pprev = pthis->pprev;

            /* Link the previous pointer if not null. */
            if(pthis->pprev)
                pthis->pprev->pnext = pthis->pnext;
        }


        /** MoveToFront
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


        /** Get
         *
         *  Get the data by index
         *
         *  @param[in] Key The key type.
         *  @param[out] Data The data type.
         *
         *  @return True if object was found, false if none found by index.
         *
         **/
        bool Get(const KeyType& Key, DataType& Data)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(!cache.count(Key))
                return false;

            /* Get the data. */
            TemplateNode<KeyType, DataType> *pthis = cache[Key];

            /* Get the data. */
            Data = pthis->Data;

            /* Move to front of double linked list. */
            MoveToFront(pthis);

            return true;
        }


        /** Put
         *
         *  Add data in the Pool.
         *
         *  @param[in] Key The key type object.
         *  @param[in] Data The data type object.
         *
         **/
        void Put(const KeyType& Key, const DataType& Data)
        {
            LOCK(MUTEX);

            /* Check for bucket collisions. */
            if(cache.count(Key))
            {
                /* Get the node we are working on. */
                TemplateNode<KeyType, DataType>* pthis = cache[Key];

                /* Remove the node from the linked list. */
                RemoveNode(pthis);

                /* Clear the pointer data. */
                cache.erase(Key);

                /* Free the memory. */
                delete pthis;
            }

            /* Create a new cache node. */
            TemplateNode<KeyType, DataType>* pthis = new TemplateNode<KeyType, DataType>(Key, Data);
            cache[Key] = pthis;

            /* Set the new cache node to the front */
            MoveToFront(pthis);

            /* Remove the last node if cache too large. */
            if(cache.size() > MAX_CACHE_ELEMENTS)
            {
                if(plast && plast->pprev)
                {
                    /* Get the last key. */
                    TemplateNode<KeyType, DataType>* pnode = plast;

                    /* Relink in memory. */
                    plast = pnode->pprev;
                    plast->pnext = nullptr;

                    /* Reset hashmap pointer */
                    cache.erase(pnode->Key);
                    pnode->pprev = nullptr;
                    pnode->pnext = nullptr;

                    /* Free memory. */
                    delete pnode;

                    /* Clear the pointers. */
                    pnode = nullptr;
                }
            }
        }


        /** Remove
         *
         *  Force Remove an Object by Index.
         *
         *  @param[in] Key The Binary Data of the Key.
         *
         *  @return True on successful removal, false if it fails.
         *
         **/
        bool Remove(const KeyType& Key)
        {
            LOCK(MUTEX);

            /* Check for key. */
            if(!cache.count(Key))
                return false;

            /* Get the node we are working on. */
            TemplateNode<KeyType, DataType>* pthis = cache[Key];

            /* Remove the node from the linked list. */
            RemoveNode(pthis);

            /* Clear the pointer data. */
            cache.erase(Key);
            pthis->pprev     = nullptr;
            pthis->pnext     = nullptr;

            /* Free the memory. */
            delete pthis;

            return true;
        }
    };
}

#endif
