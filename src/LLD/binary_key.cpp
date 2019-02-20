/*__________________________________________________________________________________________
            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
            (c) Copyright The Nexus Developers 2014 - 2019
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/cache/binary_key.h>
#include <Util/include/mutex.h>
#include <openssl/md5.h>

namespace LLD
{

    /** Default constructor **/
    BinaryKey::BinaryKey(const std::vector<uint8_t>& vKeyIn)
    : pprev(nullptr)
    , pnext(nullptr)
    , vKey(vKeyIn)
    , nTimestamp(runtime::timestamp())
    {
    }


    /** Base Constructor.  **/
    KeyLRU::KeyLRU()
    : MAX_CACHE_SIZE(1024 * 1024)
    , MAX_CACHE_BUCKETS(MAX_CACHE_SIZE / 32)
    , nCurrentSize(MAX_CACHE_BUCKETS * 8)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS)
    , pfirst(nullptr)
    , plast(nullptr)
    {
    }


    /** Cache Size Constructor **/
    KeyLRU::KeyLRU(uint32_t nCacheSizeIn)
    : MAX_CACHE_SIZE(nCacheSizeIn)
    , MAX_CACHE_BUCKETS(nCacheSizeIn / 32)
    , nCurrentSize(MAX_CACHE_BUCKETS * 8)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS)
    , pfirst(nullptr)
    , plast(nullptr)
    {
    }

    /** Copy constructor. **/
    KeyLRU::KeyLRU(const KeyLRU& cache)
    : MAX_CACHE_SIZE(cache.MAX_CACHE_SIZE)
    , MAX_CACHE_BUCKETS(cache.MAX_CACHE_BUCKETS)
    , nCurrentSize(cache.nCurrentSize)
    , MUTEX()
    , hashmap(cache.hashmap)
    , pfirst(cache.pfirst)
    , plast(cache.plast)
    {
    }


    /** Class Destructor. **/
    KeyLRU::~KeyLRU()
    {
        /* Loop through the linked list. */
        for(auto& item : hashmap)
            if(item)
                delete item;
    }


    /*  Find a bucket for cache key management. */
    uint32_t KeyLRU::Bucket(const std::vector<uint8_t>& vKey) const
    {
        /* Get an MD5 digest. */
        uint8_t digest[MD5_DIGEST_LENGTH];
        MD5((uint8_t*)&vKey[0], vKey.size(), (uint8_t*)&digest);

        /* Copy bytes into the bucket. */
        uint64_t nBucket;
        std::copy((uint8_t*)&digest[0], (uint8_t*)&digest[0] + 8, (uint8_t*)&nBucket);

        return nBucket % MAX_CACHE_BUCKETS;
    }


    /*  Check if data exists. */
    bool KeyLRU::Get(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        uint32_t nBucket = Bucket(vKey);

        /* Get the data. */
        BinaryKey* pthis = hashmap[nBucket];
        if(pthis == nullptr)
            return false;

        /* Move to front of LRU. */
        MoveToFront(pthis);

        return (pthis->vKey == vKey);
    }


    /*  Remove a node from the double linked list. */
    void KeyLRU::RemoveNode(BinaryKey* pthis)
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


    /*  Move the node in double linked list to front. */
    void KeyLRU::MoveToFront(BinaryKey* pthis)
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


    /*  Add data in the Pool. */
    void KeyLRU::Put(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* If has a key, check for bucket collisions. */
        uint32_t nBucket = Bucket(vKey);

        /* Check for bucket collisions. */
        if(hashmap[nBucket] != nullptr)
        {
            /* Get the object we are working on. */
            BinaryKey* pthis = hashmap[nBucket];

            /* Remove from the linked list. */
            RemoveNode(pthis);

            /* Dereference the pointers. */
            hashmap[nBucket] = nullptr;
            pthis->pprev     = nullptr;
            pthis->pnext     = nullptr;

            /* Reduce the current size. */
            nCurrentSize -= (pthis->vKey.size());

            /* Free the memory. */
            delete pthis;
        }

        /* Create a new cache node. */
        BinaryKey* pthis = new BinaryKey(vKey);

        /* Add cache node to objects map. */
        hashmap[nBucket] = pthis;

        /* Set the new cache node to the front */
        MoveToFront(pthis);

        /* Set the new cache size. */
        nCurrentSize += (vKey.size());

        /* Remove the last node if cache too large. */
        while(nCurrentSize > MAX_CACHE_SIZE)
        {
            /* Get the last key. */
            if(plast && plast->pprev)
            {
                BinaryKey *pnode = plast;

                /* Relink in memory. */
                plast = plast->pprev;

                if(plast && plast->pnext)
                    plast->pnext = nullptr;
                else
                    printf("plast is null\n");

                /* Reduce the current cache size. */
                if(pnode)
                {
                    nCurrentSize -= (pnode->vKey.size());

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


    /*  Force Remove Object by Index. */
    bool KeyLRU::Remove(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* Get the bucket. */
        uint32_t nBucket = Bucket(vKey);

        /* Get the object we are working on. */
        BinaryKey* pthis = hashmap[nBucket];

        /* Check if the Record Exists. */
        if (pthis == nullptr || pthis->vKey != vKey)
            return false;

        /* Remove from the linked list. */
        RemoveNode(pthis);

        /* Dereference the pointers. */
        hashmap[nBucket] = nullptr;
        pthis->pprev     = nullptr;
        pthis->pnext     = nullptr;

        /* Reduce the current size. */
        nCurrentSize -= (pthis->vKey.size());

        /* Free the memory. */
        delete pthis;

        return true;
    }

}
