/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/cache/binary_lru.h>
#include <LLD/hash/xxh3.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>

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
        BinaryNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn);

        /** Checksum. **/
        uint64_t Checksum() const
        {
            return XXH64(&vData[0], vData.size(), 0);
        }
    };


    /** Default constructor **/
    BinaryNode::BinaryNode(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vDataIn)
    : pprev(nullptr)
    , pnext(nullptr)
    , hashKey(XXH64(&vKey[0], vKey.size(), 0))
    , vData(vDataIn)
    {
    }


    /** Base Constructor.  **/
    BinaryLRU::BinaryLRU()
    : MAX_CACHE_SIZE(1024 * 1024)
    , MAX_CACHE_BUCKETS(MAX_CACHE_SIZE / 512)
    , nCurrentSize(MAX_CACHE_BUCKETS * 16)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS)
    , checksums(MAX_CACHE_BUCKETS)
    , pfirst(nullptr)
    , plast(nullptr)
    {
    }


    /** Cache Size Constructor **/
    BinaryLRU::BinaryLRU(uint32_t nCacheSizeIn)
    : MAX_CACHE_SIZE(nCacheSizeIn)
    , MAX_CACHE_BUCKETS(nCacheSizeIn / 512)
    , nCurrentSize(MAX_CACHE_BUCKETS * 16)
    , MUTEX()
    , hashmap(MAX_CACHE_BUCKETS)
    , checksums(MAX_CACHE_BUCKETS)
    , pfirst(nullptr)
    , plast(nullptr)
    {
    }


    /** Class Destructor. **/
    BinaryLRU::~BinaryLRU()
    {
        /* Loop through the linked list. */
        for(auto& item : hashmap)
            if(item != nullptr)
                delete item;
    }


    /*  Get the checksum of a data object. */
    uint64_t BinaryLRU::Checksum(const std::vector<uint8_t>& vData) const
    {
        return XXH64(&vData[0], vData.size(), 0);
    }


    /*  Find a bucket for cache key management. */
    uint32_t BinaryLRU::Bucket(const BinaryNode* pnode) const
    {
        /* Get an xxHash. */
        uint64_t nChecksum = pnode->Checksum();
        uint64_t nBucket   = XXH64((uint8_t*)&nChecksum, 8, 0);

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
        /* Get an xxHash. */
        uint64_t nBucket = XXH64((uint8_t*)&nChecksum, 8, 0);

        return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
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
        if(pthis->Checksum() != nChecksum)
            return false;

        /* Check the data is expected. */
        return (pthis->hashKey == XXH64(&vKey[0], vKey.size(), 0));
    }


    /*  Remove a node from the double linked list. */
    void BinaryLRU::RemoveNode(BinaryNode* pthis)
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


    /*  Get the data by index */
    bool BinaryLRU::Get(const std::vector<uint8_t>& vKey, std::vector<uint8_t>& vData)
    {
        LOCK(MUTEX);

        /* Check for data. */
        const uint64_t& nChecksum = checksums[Bucket(vKey)];
        if(nChecksum == 0)
            return false;

        /* Get the binary node. */
        BinaryNode* pthis = hashmap[Bucket(nChecksum)];

        /* Check if the Record Exists. */
        if(pthis == nullptr)
        {
            /* Set checksum to zero if record not found. */
            checksums[Bucket(vKey)] = 0;

            return false;
        }

        /* Check the data is expected. */
        if(pthis->Checksum() != nChecksum)
            return false;

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
    void BinaryLRU::Put(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vData, bool fReserve)
    {
        LOCK(MUTEX);

        /* Check for empty slot. */
        uint64_t nChecksum = checksums[Bucket(vKey)];

        /* Check the checksums. */
        if(nChecksum == Checksum(vData))
            return;

        /* Get the binary node. */
        uint32_t nBucket  = Bucket(nChecksum);
        BinaryNode* pthis = hashmap[nBucket];

        /* Check for dereferencing nullptr. */
        if(pthis != nullptr)
        {
            /* Remove from the linked list. */
            RemoveNode(pthis);

            /* Dereference the pointers. */
            hashmap[nBucket]           = nullptr;
            pthis->pprev               = nullptr;
            pthis->pnext               = nullptr;

            /* Reduce the current size. */
            nCurrentSize -= static_cast<uint32_t>(pthis->vData.size() + 48);

            /* Free the memory. */
            delete pthis;
        }

        /* Create a new cache node. */
        pthis = new BinaryNode(vKey, vData);
        nChecksum = pthis->Checksum();

        /* Add cache node to objects map. */
        hashmap[nBucket]           = pthis;
        checksums[Bucket(vKey)]    = nChecksum;

        /* Set the new cache node to the front */
        MoveToFront(pthis);

        /* Remove the last node if cache too large. */
        if(nCurrentSize > MAX_CACHE_SIZE)
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
            plast->pnext = nullptr;

            /* Clear the pointers. */
            hashmap[Bucket(pnode)]   = nullptr;

            /* Set the checksums. */
            uint32_t nBucket = static_cast<uint32_t>(pnode->hashKey % static_cast<uint64_t>(MAX_CACHE_BUCKETS));
            checksums[nBucket] = 0;

            /* Reset the memory linking. */
            pnode->pprev = nullptr;
            pnode->pnext = nullptr;

            /* Free the memory */
            nCurrentSize -= static_cast<uint32_t>(pnode->vData.size() + 48);

            delete pnode;
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
        const uint64_t& nChecksum = checksums[Bucket(vKey)];

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
            checksums[Bucket(vKey)] = 0;

            return false;
        }

        /* Remove from the linked list. */
        RemoveNode(pthis);

        /* Dereference the pointers. */
        hashmap[Bucket(pthis)]   = nullptr;
        pthis->pprev             = nullptr;
        pthis->pnext             = nullptr;

        /* Free the memory. */
        nCurrentSize -= static_cast<uint32_t>(pthis->vData.size() + 8);
        checksums[Bucket(vKey)] = 0;

        delete pthis;

        return true;
    }
}
