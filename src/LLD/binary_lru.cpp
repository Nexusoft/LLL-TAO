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
    struct BinaryNode
    {
        /** The prev pointer. **/
        BinaryNode* pprev;

        /** The next pointer. **/
        BinaryNode* pnext;

        /** Store the key as 64-bit hash, since we have checksum to verify against too. **/
        uint64_t hashKey;

        /** The data in the binary node. **/
        std::vector<uint8_t> vData;

        /** Default constructor **/
        BinaryNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn)
        : pprev(nullptr)
        , pnext(nullptr)
        , hashKey(XXH64(&vKey[0], vKey.size(), 0))
        , vData(vDataIn)
        {
        }

        /** Method to update this node with a new key and data **/
        void Update(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn)
        {
            /* Swap to clear vector data (this will swap vector with temporary value to be destroyed). */
            std::vector<uint8_t>().swap(vData);

            /* Reset linking pointers. */
            pprev = nullptr;
            pnext = nullptr;

            /* Set new data. */
            hashKey = XXH64(&vKey[0], vKey.size(), 0);
            vData   = vDataIn;
        }
    };



    /** Base Constructor.  **/
    BinaryLRU::BinaryLRU()
    : MAX_CACHE_SIZE(1024 * 1024)
    , MAX_CACHE_BUCKETS(MAX_CACHE_SIZE / 128)
    , nCurrentSize(MAX_CACHE_BUCKETS * 16)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS, nullptr)
    , checksums(MAX_CACHE_BUCKETS, 0)
    , pfirst(nullptr)
    , plast(nullptr)
    , pPool(nullptr)
    {
    }


    /** Cache Size Constructor **/
    BinaryLRU::BinaryLRU(uint32_t nCacheSizeIn)
    : MAX_CACHE_SIZE(nCacheSizeIn)
    , MAX_CACHE_BUCKETS(nCacheSizeIn / 128)
    , nCurrentSize(MAX_CACHE_BUCKETS * 16)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS, nullptr)
    , checksums(MAX_CACHE_BUCKETS, 0)
    , pfirst(nullptr)
    , plast(nullptr)
    , pPool(nullptr)
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

        /* Clean up those in the pool */
        while(pPool != nullptr)
        {
            /* Get pointer to node to delete */
            BinaryNode* pDelete = pPool;

            /* Update the pool pointer */
            pPool = pDelete->pnext;

            /* delete the node */
            delete pDelete;
        }
    }


    /*  Get the checksum of a data object. */
    uint64_t BinaryLRU::Checksum(const std::vector<uint8_t>& vData) const
    {
        return XXH64(&vData[0], vData.size(), 0);
    }


    /* Find a bucket for cache key management. */
    uint32_t BinaryLRU::Bucket(const SectorKey& key) const
    {
        /* Get the bucket. */
        uint64_t nBucket = key.nSectorFile * key.nSectorStart;

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
    }


    /*  Find a bucket for cache key management. */
    uint32_t BinaryLRU::Bucket(const std::vector<uint8_t>& vKey) const
    {
        /* Get an xxHash. */
        uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0);

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
    }


    /*  Find a bucket for checksum key management. */
    uint32_t BinaryLRU::Bucket(const uint64_t nChecksum) const
    {
        return static_cast<uint32_t>(nChecksum % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
    }


    /*  Check if data exists. */
    bool BinaryLRU::Has(const std::vector<uint8_t>& vKey) const
    {
        LOCK(MUTEX);

        /* Get the data. */
        const uint64_t& nChecksum = checksums[Bucket(vKey)];
        if(nChecksum == 0)
            return false;

        /* Check if the Record Exists. */
        const BinaryNode* pthis = hashmap[Bucket(nChecksum)];
        if(pthis == nullptr)
            return false;

        /* Check the data is expected. */
        return (pthis->hashKey == XXH64(&vKey[0], vKey.size(), 0));
    }


    /*  Unlinks a node from the double linked list. */
    void BinaryLRU::UnlinkNode(BinaryNode* pthis)
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
    void BinaryLRU::MoveToFront(BinaryNode* pthis)
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
            UnlinkNode(pthis);

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


    /*  Get the data by index */
    bool BinaryLRU::Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
    {
        LOCK(MUTEX);

        /* Get the checksum bucket. */
        uint32_t nChecksumBucket = Bucket(vKey);

        /* Check for data. */
        const uint64_t& nChecksum = checksums[nChecksumBucket];
        if(nChecksum == 0)
            return false;

        /* Get the binary node. */
        BinaryNode* pthis = hashmap[Bucket(nChecksum)];

        /* Check if the Record Exists. */
        if(pthis == nullptr)
        {
            /* Set checksum to zero if record not found. */
            checksums[nChecksumBucket] = 0;

            return false;
        }

        /* Check the keys are correct. */
        if(pthis->hashKey != XXH64(&vKey[0], vKey.size(), 0))
            return false;

        /* Get the data. */
        vData = pthis->vData;

        /* Move to front of double linked list. */
        MoveToFront(pthis);

        return true;
    }


    /*  Add data in the Pool. */
    void BinaryLRU::Put(const SectorKey& key, const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData, bool fReserve)
    {
        LOCK(MUTEX);

        /* Check for empty slot. */
        uint32_t nChecksumBucket = Bucket(vKey);
        uint64_t nChecksum       = checksums[nChecksumBucket];

        /* Get the binary node. */
        if(nChecksum != 0)
        {
            uint32_t nBucket  = Bucket(nChecksum);
            BinaryNode* pthis = hashmap[nBucket];

            /* Check for dereferencing nullptr. */
            if(pthis != nullptr)
            {
                /* Remove from the linked list. */
                UnlinkNode(pthis);

                /* Remove from the cache */
                RemoveNode(pthis, nChecksumBucket, nBucket);
            }

            checksums[nChecksumBucket] = 0;
        }

        /* Create a new cache node. */
        BinaryNode* pnew = AddNode(vKey, vData);
        nChecksum = (key.nSectorFile * key.nSectorStart);

        /* Get the bucket. */
        uint32_t nBucket    = Bucket(nChecksum);

        /* Cleanup if colliding with another bucket. */
        if(hashmap[nBucket] != nullptr)
        {
            /* Get copy of pointer. */
            BinaryNode* pthis = hashmap[nBucket];

            /* Remove from the linked list. */
            UnlinkNode(pthis);

            /* Remove from the cache */
            RemoveNode(pthis, nChecksumBucket, nBucket);

        }

        /* Add cache node to objects map. */
        hashmap[nBucket]            = pnew;
        checksums[nChecksumBucket]  = nChecksum;

        /* Set the new cache node to the front */
        MoveToFront(pnew);

        /* Remove the last node if cache too large. */
        while(nCurrentSize > MAX_CACHE_SIZE)
        {
            /* Get last pointer. */
            BinaryNode* pnode = plast;
            if(!pnode)
                return;

            /* Set the next link. */
            if(!plast->pprev)
                return;

            /* Set the new links. */
            plast = plast->pprev;

            /* Check for nullptr. */
            if(plast)
                plast->pnext = nullptr;

            /* Calculate the buckets for the node being deleted */
            uint32_t nChecksumBucket = static_cast<uint32_t>(pnode->hashKey % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
            uint64_t nHashMapBucket = Bucket(checksums[nBucket]);

            /* Remove from the cache */
            RemoveNode(pnode, nChecksumBucket, nHashMapBucket);
        }

        /* Set the new cache size. */
        nCurrentSize += static_cast<uint32_t>(vData.size() + 48);
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
        uint64_t nChecksumBucket = Bucket(vKey);
        const uint64_t& nChecksum = checksums[nChecksumBucket];

        /* Check for data. */
        if(nChecksum == 0)
            return false;

        /* Get the binary node. */
        uint32_t nBucket  = Bucket(nChecksum);
        BinaryNode* pthis = hashmap[nBucket];

        /* Check if the Record Exists. */
        if(pthis == nullptr)
        {
            /* Reset the checksum. */
            checksums[nChecksumBucket] = 0;

            return false;
        }

        /* Remove from the linked list. */
        UnlinkNode(pthis);

        /* Remove from the cache */
        RemoveNode(pthis, nChecksumBucket, nBucket);

        return true;
    }

    /* Creates a new node for the cache.  If there is a node in the pool of previously instantated instances then it will be
    *  used, otherwise a new instance will be instantiated  */
    BinaryNode* BinaryLRU::AddNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn)
    {
        BinaryNode* pAdd = nullptr;

        /* If we already have a node in the pool then reuse it */
        if(pPool)
        {
            /* Get pointer to node from pool */
            pAdd = pPool;

            /* Update the pool pointer */
            pPool = pPool->pnext;

            /* Update the node with the incoming data */
            pAdd->Update(vKey, vDataIn);
        }
        else
        {
            /* Instantiate a new instance */
            pAdd = new BinaryNode(vKey, vDataIn);
        }

        /* return the newly added node */
        return pAdd;

    }

    /* Removes a node from the cache and adds it to the node pool for resuse */
    void BinaryLRU::RemoveNode(BinaryNode* pnode, uint32_t nChecksumBucket, uint64_t nHashMapBucket)
    {
        /* Dereference the pointers. */
        pnode->pprev             = nullptr;
        pnode->pnext             = nullptr;

        /* Free the memory. */
        nCurrentSize -= static_cast<uint32_t>(pnode->vData.size() + 48);

        /* Clear the buckets */
        checksums[nChecksumBucket] = 0;
        hashmap[nHashMapBucket] = nullptr;

        /* Add the node into the pool */
        if(pPool)
            pnode->pnext = pPool;

        /* Update pool pointer to node being put back into the pool */
        pPool = pnode;
    }
}
/* Update pool pointer to node being put back into the pool */
        pPool = pnode;
    }
}
