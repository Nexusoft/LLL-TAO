/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/cache/binary_lru.h>
#include <LLD/templates/key.h>
#include <LLD/hash/xxh3.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

namespace LLD
{
    /*  Node to hold the binary data of the double linked list. */
    class BinaryNode
    {
    public:

        /** Linked list pointers. **/
        BinaryNode* pprev;
        BinaryNode* pnext;

        /** Store the key as 64-bit hash, since we have checksum to verify against too. **/
        uint64_t hashKey;

        /** The data in the binary node. **/
        std::vector<uint8_t> vData;

        /** Default constructor **/
        BinaryNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn)
        : pprev   (nullptr)
        , pnext   (nullptr)
        , hashKey (XXH64(&vKey[0], vKey.size(), 0))
        , vData   (vDataIn)
        {
        }

        /** bucket Node is Assigned to. **/
        uint32_t Bucket(const uint64_t nBuckets)
        {
            return static_cast<uint32_t>(hashKey % static_cast<uint64_t>(nBuckets));
        }


        /** Erase the data. **/
        void Erase()
        {
            std::vector<uint8_t>().swap(vData);
        }


        /** Check if node is in null state. **/
        bool IsNull() const
        {
            return hashKey == 0;
        }


        /** Set node into null state. **/
        void SetNull()
        {
            pprev   = nullptr;
            pnext   = nullptr;
            hashKey = 0;

            Erase();
        }
    };


    /** Cache Size Constructor **/
    BinaryLRU::BinaryLRU(const uint32_t nCacheSizeIn)
    : MAX_CACHE_SIZE    (nCacheSizeIn)
    , MAX_CACHE_BUCKETS (nCacheSizeIn / 128)
    , nCurrentSize      (MAX_CACHE_BUCKETS * 16)
    , MUTEX             ( )
    , hashmap           (MAX_CACHE_BUCKETS, nullptr)
    , indexes           (MAX_CACHE_BUCKETS, 0)
    , pfirst            (nullptr)
    , plast             (nullptr)
    {
    }


    /** Class Destructor. **/
    BinaryLRU::~BinaryLRU()
    {
        LOCK(MUTEX);

        /* Loop through the linked list. */
        for(auto& item : hashmap)
            if(item != nullptr)
                delete item;
    }


    /*  Check if data exists. */
    bool BinaryLRU::Has(const std::vector<uint8_t>& vKey) const
    {
        LOCK(MUTEX);

        /* Get the data. */
        const uint64_t& nIndex  = indexes[bucket(vKey)];
        if(nIndex == 0)
            return false;

        /* Check if the Record Exists. */
        const BinaryNode* pthis = hashmap[slot(nIndex)];
        if(pthis == nullptr)
            return false;

        /* Check for null state. */
        if(pthis->IsNull())
            return false;

        /* Check the data is expected. */
        return (pthis->hashKey == XXH64(&vKey[0], vKey.size(), 0));
    }


    /*  Find a bucket for cache key management. */
    uint32_t BinaryLRU::bucket(const std::vector<uint8_t>& vKey) const
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0);

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
    }


    /*  Get the data by index */
    bool BinaryLRU::Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
    {
        LOCK(MUTEX);

        /* Check for data. */
        uint64_t& nIndex  = indexes[bucket(vKey)];
        if(nIndex == 0)
            return false;

        /* Get the binary node. */
        BinaryNode* pthis = hashmap[slot(nIndex)];
        if(pthis == nullptr)
        {
            nIndex = 0; //reset by reference
            return false;
        }

        /* Check for null state. */
        if(pthis->IsNull())
        {
            nIndex = 0; //reset by reference
            return false;
        }

        /* Check the keys are correct. */
        if(pthis->hashKey != XXH64(&vKey[0], vKey.size(), 0))
            return false;

        /* Get the data. */
        vData = pthis->vData;

        /* Move to front of double linked list. */
        move_to_front(pthis);

        return true;
    }


    /*  Add data in the Pool. */
    void BinaryLRU::Put(const SectorKey& key, const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData, bool fReserve)
    {
        LOCK(MUTEX);

        /* Get the binary node. */
        uint32_t nIndexBucket = bucket(vKey);
        uint64_t& nIndex = indexes[nIndexBucket];
        if(nIndex != 0)
        {
            /* Get the node from checksum. */
            uint32_t nSlot    = slot(nIndex);
            BinaryNode* pthis = hashmap[nSlot];

            /* Check for dereferencing nullptr. */
            if(pthis != nullptr && !pthis->IsNull())
            {
                /* Reduce the current size. */
                nCurrentSize -= static_cast<uint32_t>(pthis->vData.size());

                /* Free the memory. */
                remove_node(pthis);
                pthis->SetNull();
            }
        }

        /* Set the new index. NOTE nSectorFile is 0-based so we add 1 to it to ensure we always have an index*/
        nIndex = ((key.nSectorFile + 1) * key.nSectorStart);

        /* Cleanup if colliding with another bucket. */
        uint32_t nSlot = slot(nIndex);
        if(hashmap[nSlot] != nullptr)
        {
            /* Claiming an empty hashmap slot. */
            if(!hashmap[nSlot]->IsNull())
            {
                /* Erase data on collision. */
                nCurrentSize -= static_cast<uint32_t>(hashmap[nSlot]->vData.size());
                hashmap[nSlot]->Erase();
            }

            /* Set new values. */
            hashmap[nSlot]->hashKey = XXH64(&vKey[0], vKey.size(), 0);
            hashmap[nSlot]->vData   = vData;

            /* Move to front of list. */
            move_to_front(hashmap[nSlot]);
        }
        else
        {
            /* Add cache node to objects map. */
            hashmap[nSlot] = new BinaryNode(vKey, vData);;
            move_to_front(hashmap[nSlot]);

            /* Account for the node's memory size. */
            nCurrentSize += sizeof(*hashmap[nSlot]);
        }

        /* Update the index */
        indexes[nIndexBucket] = nIndex;

        /* Remove the last node if cache too large. */
        while(nCurrentSize > MAX_CACHE_SIZE)
        {
            /* Get last pointer. */
            BinaryNode* pnode = plast;
            if(!pnode)
                return;

            /* Set the new links. */
            plast = plast->pprev;
            if (plast)
                plast->pnext = nullptr;

            /* Reset the memory linking. */
            pnode->pprev = nullptr;
            pnode->pnext = nullptr;

            /* Reduce memory size. */
            nCurrentSize -= static_cast<uint32_t>(pnode->vData.size());

            /* Calculate the buckets for the node being deleted */
            uint32_t  nBucket = pnode->Bucket(MAX_CACHE_BUCKETS);
            uint64_t& nRemove = indexes[nBucket];

            /* Reset the slot. */
            hashmap[slot(nRemove)]->SetNull();
            nRemove                = 0;
        }

        nCurrentSize += static_cast<uint32_t>(vData.size());
    }


    /*  Reserve this item in the cache permanently if true, unreserve if false. */
    void BinaryLRU::Reserve(const std::vector<uint8_t>& vKey, bool fReserve)
    {
    }


    /*  Force Remove Object by Index. */
    bool BinaryLRU::Remove(const std::vector<uint8_t>& vKey)
    {
        LOCK(MUTEX);

        /* Get the data. */
        uint64_t& nIndex = indexes[bucket(vKey)];
        if(nIndex == 0)
            return false;

        /* Get the binary node. */
        uint32_t nSlot = slot(nIndex);
        if(hashmap[nSlot] == nullptr)
        {
            nIndex = 0; //reset by reference
            return false;
        }

        /* Remove from the linked list. */
        BinaryNode* pthis = hashmap[nSlot];
        remove_node(pthis);

        /* Set to null state. */
        hashmap[nSlot]->SetNull();

        /* Free the memory. */
        nCurrentSize  -= static_cast<uint32_t>(pthis->vData.size() + 48);
        nIndex         = 0; //reset by reference

        return true;
    }


    /*  Find a bucket for checksum key management. */
    uint32_t BinaryLRU::slot(const uint64_t nIndex) const
    {
        return static_cast<uint32_t>(nIndex % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
    }


    /*  Remove a node from the double linked list. */
    void BinaryLRU::remove_node(BinaryNode* pthis)
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


    /*  Move the node in double linked list to front. */
    void BinaryLRU::move_to_front(BinaryNode* pthis)
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
            /* Set last for second iteration. */
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
}
