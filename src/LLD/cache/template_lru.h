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
#include <Util/include/debug.h>

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
    class TemplateNode
    {
        /** Delete
         *
         *  Template function to delete pointer.
         *  This one is dummy non pointer catch
         *
         **/
        template<typename Type>
        void _delete(Type data)
        { }


        /** Delete
         *
         *  Delete a pointer object. Closes the file.
         *
         **/
        template<typename Type>
        void _delete(Type* data)
        {
            if(data)
                delete data;
        }

    public:

        TemplateNode(const KeyType& KeyIn, const DataType& DataIn)
        : pprev (nullptr)
        , pnext (nullptr)
        , Key   (KeyIn)
        , Data  (DataIn)
        {
        }

        /** Destructor. **/
        ~TemplateNode()
        {
            _delete(Key);
            _delete(Data);
        }

        TemplateNode<KeyType, DataType>* pprev;
        TemplateNode<KeyType, DataType>* pnext;

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


        /** Default Constructor. **/
        TemplateLRU()                                    = delete;


        /** Copy Constructor. **/
        TemplateLRU(const TemplateLRU& cache)            = delete;


        /** Move Constructor. **/
        TemplateLRU(TemplateLRU&& cache)                 = delete;


        /** Copy assignment. **/
        TemplateLRU& operator=(const TemplateLRU& cache) = delete;


        /** Move assignment. **/
        TemplateLRU& operator=(TemplateLRU&& cache)      = delete;


        /** Class Destructor. **/
        ~TemplateLRU()
        {
            LOCK(MUTEX);

            /* Loop through the linked list. */
            for(auto & item : cache)
            {
                if(item.second)
                    delete item.second;
            }
        }


        /** Total Elements Constructor
         *
         * @param[in] nElements The maximum size of this Cache Pool
         *
         **/
        TemplateLRU(const uint32_t nElements)
        : MAX_CACHE_ELEMENTS (nElements)
        , MUTEX              ( )
        , cache              ( )
        , pfirst             (nullptr)
        , plast              (nullptr)
        {
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

            /* Check if the Record Exists. */
            if(cache.find(Key) == cache.end())
                return false;

            /* Get the data. */
            TemplateNode<KeyType, DataType> *pthis = cache.at(Key);

            /* Check for correct key. */
            if(pthis->Key != Key)
                return debug::error(FUNCTION, "Key Mismatch");

            return true;
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
        bool Get(const KeyType& Key, DataType &Data)
        {
            LOCK(MUTEX);

            /* Check if the Record Exists. */
            if(cache.find(Key) == cache.end())
                return false;

            /* Get the data. */
            TemplateNode<KeyType, DataType> *pthis = cache[Key];

            /* Check for correct key. */
            if(pthis->Key != Key)
                return debug::error(FUNCTION, "Key Mismatch");

            /* Set the data. */
            Data = pthis->Data;

            /* Move to front of double linked list. */
            move_to_front(pthis);

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
            if(cache.find(Key) != cache.end())
            {
                /* Get the node we are working on. */
                TemplateNode<KeyType, DataType>* pthis = cache[Key];

                /* Remove the node from the linked list. */
                remove_node(pthis);

                /* Clear the pointer data. */
                cache.erase(Key);

                /* Free the memory. */
                delete pthis;
            }

            /* Create a new cache node. */
            cache[Key] = new TemplateNode<KeyType, DataType>(Key, Data);
            move_to_front(cache[Key]);

            /* Remove the last node if cache too large. */
            if(cache.size() > MAX_CACHE_ELEMENTS)
            {
                /* Get last pointer. */
                TemplateNode<KeyType, DataType>* pnode = plast;
                if(!pnode)
                    return;

                /* Set the new links. */
                plast = plast->pprev;

                /* Check for nullptr. */
                if (plast)
                    plast->pnext = nullptr;

                /* Reset hashmap pointer */
                cache.erase(pnode->Key);
                pnode->pprev = nullptr;
                pnode->pnext = nullptr;

                /* Free memory. */
                delete pnode;
                pnode = nullptr;
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
            if(cache.find(Key) == cache.end())
                return false;

            /* Get the node we are working on. */
            TemplateNode<KeyType, DataType>* pthis = cache[Key];

            /* Remove the node from the linked list. */
            remove_node(pthis);

            /* Clear the pointer data. */
            cache.erase(Key);
            pthis->pprev     = nullptr;
            pthis->pnext     = nullptr;

            /* Free the memory. */
            delete pthis;

            return true;
        }


    private:

        /** remove_node
         *
         *  Remove a node from the double linked list.
         *
         *  @param[in] pthis The node to remove from list.
         *
         **/
        void remove_node(TemplateNode<KeyType, DataType>* pthis)
        {
            /* Relink last pointer. */
            if(pthis == plast)
            {
                plast = plast->pprev;
                if(plast)
                    plast->pnext = nullptr;
            }

            /* Relink first pointer. */
            if(pthis == pfirst)
            {
                pfirst = pfirst->pnext;
                if(pfirst)
                    pfirst->pprev = nullptr;
            }

            /* Link the next pointer if not null */
            if (pthis->pnext)
                pthis->pnext->pprev = pthis->pprev;

            /* Link the previous pointer if not null. */
            if (pthis->pprev)
                pthis->pprev->pnext = pthis->pnext;

            /* Unlink next and prev. */
            pthis->pnext = nullptr;
            pthis->pprev = nullptr;
        }


        /** move_to_front
         *
         *  Move the node in double linked list to front.
         *
         *  @param[in] pthis The node to move to front.
         *
         **/
        void move_to_front(TemplateNode<KeyType, DataType>* pthis)
        {
            /* Don't move to front if already in the front. */
            if(pthis == pfirst)
                return;

            /* Move last pointer if moving from back. */
            remove_node(pthis);

            /* Link to front. */
            pthis->pprev = nullptr;
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
    };
}

#endif
