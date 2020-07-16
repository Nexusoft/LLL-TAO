/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/sector.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <Util/include/runtime.h>

#include <list>
#include <functional>
#include <variant>

#include <Util/include/softfloat.h>
#include <Util/include/filesystem.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/locator.h>

#include <LLD/templates/bloom.h>

class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
{
public:
    TestDB()
    : SectorDatabase("testdb"
    , LLD::FLAGS::CREATE | LLD::FLAGS::FORCE
    , 256 * 256
    , 256)
    {
    }

    ~TestDB()
    {

    }

    bool WriteKey(const uint1024_t& key, const uint1024_t& value)
    {
        return Write(std::make_pair(std::string("key"), key), value);
    }


    bool ReadKey(const uint1024_t& key, uint1024_t &value)
    {
        return Read(std::make_pair(std::string("key"), key), value);
    }

    bool HasKey(const uint1024_t& key)
    {
        return Exists(std::make_pair(std::string("key"), key));
    }


    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }

    bool ReadLast(uint1024_t &last)
    {
        return Read(std::string("last"), last);
    }

};

#include <TAO/Ledger/include/genesis_block.h>

const uint256_t hashSeed = 55;

#include <bitset>


const uint32_t MAX_PRIMARY_K_HASHES = 7;
const uint32_t MAX_BITS_PER_BLOOM   = 12;

bool check_hashmap_available(const uint32_t nHashmap, const std::vector<uint8_t>& vBuffer)
{
    uint32_t nBeginIndex  = (nHashmap * MAX_BITS_PER_BLOOM) / 8;
    uint32_t nBeginOffset = (nHashmap * MAX_BITS_PER_BLOOM) % 8;

    for(uint32_t n = 0; n < MAX_BITS_PER_BLOOM; ++n)
    {
        uint32_t nIndex  = nBeginIndex + ((nBeginOffset + n) / 8);
        uint32_t nOffset = (nBeginOffset + n) % 8;

        if(vBuffer[nIndex] & (1 << nOffset))
            return false;
    }

    return true;
}

bool check_secondary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nHashmap = 0)
{
    uint32_t nBeginIndex  = (nHashmap * MAX_BITS_PER_BLOOM) / 8;
    uint32_t nBeginOffset = (nHashmap * MAX_BITS_PER_BLOOM) % 8;

    for(uint32_t k = 0; k < MAX_PRIMARY_K_HASHES; ++k)
    {
        uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % MAX_BITS_PER_BLOOM;

        uint32_t nIndex  = nBeginIndex + (nBeginOffset + nBucket) / 8;
        uint32_t nBit    = (nBeginOffset + nBucket) % 8;

        if(!(vBuffer[nIndex] & (1 << nBit)))
            return false;
    }

    return true;
}

void set_secondary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nHashmap = 0)
{
    uint32_t nBeginIndex  = (nHashmap * MAX_BITS_PER_BLOOM) / 8;
    uint32_t nBeginOffset = (nHashmap * MAX_BITS_PER_BLOOM) % 8;

    for(uint32_t k = 0; k < MAX_PRIMARY_K_HASHES; ++k)
    {
        uint64_t nBucket = XXH3_64bits_withSeed((uint8_t*)&vKey[0], vKey.size(), k) % MAX_BITS_PER_BLOOM;

        uint32_t nIndex  = nBeginIndex + (nBeginOffset + nBucket) / 8;
        uint32_t nBit    = (nBeginOffset + nBucket) % 8;

        vBuffer[nIndex] |= (1 << nBit);
    }
}

#include <LLD/hash/xxhash.h>


class LinkedNode
{
public:

    uint64_t nHash;

    std::vector<LinkedNode*> vChild;

    LinkedNode() = delete;

    LinkedNode(const uint16_t nNodes)
    : nHash (0)
    , vChild(nNodes, nullptr)
    {
    }
};


class ModulusLinkedList
{
    uint64_t get_hash(const std::vector<uint8_t>& vKey)
    {
        return XXH3_64bits_withSeed(&vKey[0], vKey.size(), 0);
    }

public:

    std::vector<LinkedNode*> vLast;

    uint16_t nModulus;

    ModulusLinkedList() = delete;

    ModulusLinkedList(const uint16_t nModulusIn)
    : vLast    (nModulusIn, nullptr)
    , nModulus (nModulusIn)
    {
    }

    template<typename KeyType>
    bool Has(const KeyType& key)
    {
        DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
        ssKey << key;

        uint64_t nHash = get_hash(ssKey.Bytes());
        uint32_t nBranch = (nHash % nModulus);

        uint32_t nTotal = 1;

        /* Check the last indexes. */
        LinkedNode* pnode = vLast[nBranch];
        while(pnode)
        {
            ++nTotal;

            if(pnode->nHash == nHash)
            {
                debug::log(0, "Key found after ", nTotal, " Iterations ", nHash);
                return true;
            }

            pnode = pnode->vChild[nBranch];
        }

        return false;
    }


    template<typename KeyType>
    void Insert(const KeyType& key)
    {
        DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
        ssKey << key;

        uint64_t nHash = get_hash(ssKey.Bytes());
        uint32_t nBranch = (nHash % nModulus);

        /* Check the last indexes. */
        LinkedNode* pnode = new LinkedNode(nModulus);
        pnode->vChild = vLast;
        pnode->nHash = nHash;

        /* Set this as the new last pointer. */
        vLast[nBranch] = pnode;
    }
};


class TrieNode
{
public:

    uint64_t nHash;

    //std::vector<LinkedNode*> vChild;
    TrieNode* pleft;
    TrieNode* pright;

    TrieNode(const uint64_t nHashIn)
    : nHash  (nHashIn)
    , pleft  (nullptr)
    , pright (nullptr)
    {
    }
};

class ModulusSearchTree
{
    uint64_t get_hash(const std::vector<uint8_t>& vKey)
    {
        return XXH3_64bits_withSeed(&vKey[0], vKey.size(), 0);
    }

public:

    TrieNode* proot;

    ModulusSearchTree()
    : proot    (nullptr)
    {
    }

    template<typename KeyType>
    bool Has(const KeyType& key)
    {
        DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
        ssKey << key;

        uint64_t nHash = get_hash(ssKey.Bytes());
        //uint32_t nBranch = (nHash % 2);

        uint32_t nTotal = 1;

        /* Check for root node. */
        if(!proot)
            return false;

        TrieNode* pnode = proot;
        while(pnode)
        {
            ++nTotal;

            if(pnode->nHash == nHash)
            {
                debug::log(0, "Modulus Tree index found after ", nTotal, " iterations");
                return true;
            }

            bool fRight = pnode->nHash > nHash;//((nHash % 2) != (pnode->nHash % 2));

            //if(pnode->nHash > nHash)
            //if(nBranch == 1)
            if(fRight)
                pnode = pnode->pright;

            //if(pnode->nHash < nHash)
            //if(nBranch == 0)
            else
                pnode = pnode->pleft;


        }

        return false;
    }


    template<typename KeyType>
    void Insert(const KeyType& key)
    {
        DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
        ssKey << key;

        uint64_t nHash = get_hash(ssKey.Bytes());
        //uint32_t nBranch = (nHash % 2);

        /* Check for root. */
        if(!proot)
        {
            proot = new TrieNode(nHash);

            return;
        }

        uint32_t nTotal = 1;

        TrieNode* pnode = proot;
        while(true)
        {
            ++nTotal;

            if(pnode->nHash == nHash)
                break;

            bool fRight = pnode->nHash > nHash;//((nHash % 2) != (pnode->nHash % 2));
            //if(pnode->nHash > nHash)
            //if(nBranch == 1)

            if(fRight)
            {
                if(pnode->pright)
                    pnode = pnode->pright;
                else
                {
                    debug::log(0, "Modulus RH Tree index created after ", nTotal, " iterations");
                    pnode->pright = new TrieNode(nHash);
                    break;
                }
            }


            //if(pnode->nHash < nHash)
            //if(nBranch == 0)
            else
            {
                if(pnode->pleft)
                    pnode = pnode->pleft;
                else
                {
                    debug::log(0, "Modulus LH Tree index created after ", nTotal, " iterations");
                    pnode->pleft = new TrieNode(nHash);
                    break;
                }
            }



        }
    }
};


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::mapArgs["-datadir"] = "/database/LIVE";

    TestDB* bloom = new TestDB();

    uint1024_t hashData = 0;
    if(!bloom->HasKey(hashBlock))
        bloom->WriteKey(hashBlock, hashBlock);

    runtime::stopwatch swReader;
    swReader.start();


    for(int i = 0; i < nTotalTests; ++i)
        if(!bloom->ReadKey(hashBlock, hashData))
            debug::error("Failed to read ", hashBlock.SubString());

    swReader.stop();

    debug::log(0, "Oldest record read in ", swReader.ElapsedMicroseconds(), " (", std::fixed, (1000000.0 * nTotalTests) / swReader.ElapsedMicroseconds(), " op/s)");

    std::vector<uint1024_t> vKeys;
    for(int i = 0; i < 1000000; ++i)
        vKeys.push_back(LLC::GetRand1024());

    uint32_t nDuplicates = 0;


    std::vector<uint8_t> vBuffer((vKeys.size() * MAX_BITS_PER_BLOOM) / 8, 0);

    debug::log(0, "Allocating a buffer of size ", vBuffer.size(), " for ", vKeys.size(), " files");

    DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
    ssKey << vKeys[0];

    std::vector<uint8_t> vKey = ssKey.Bytes();
    set_secondary_bloom(vKey, vBuffer, 0);

    for(uint32_t n = 1; n < vKeys.size(); ++n)
    {
        DataStream ssData(SER_LLD, LLD::DATABASE_VERSION);
        ssData << vKeys[n];

        if(!check_hashmap_available(n, vBuffer))
            return debug::error("FILE ", n, " IS NOT EMPTY");

        set_secondary_bloom(ssData.Bytes(), vBuffer, n);
        if(check_secondary_bloom(ssData.Bytes(), vBuffer, 0))
            ++nDuplicates;

        if(check_hashmap_available(n, vBuffer))
            return debug::error("FILE ", n, " EMPTY");
    }

    debug::log(0, "Created ", vKeys.size(), " Bloom filters with ",
        nDuplicates, " duplicates [", std::fixed, (nDuplicates * 100.0) / vKeys.size(), " %]");

    return 0;

    runtime::stopwatch swTimer;
    swTimer.start();

    uint32_t nCount = 0;
    for(const auto& nBucket : vKeys)
    {
        if(!bloom->WriteKey(nBucket, nBucket))
            return debug::error("Failed on ", ++nCount, " to write ", nBucket.SubString());
    }
    swTimer.stop();

    debug::log(0, nTotalTests, " records written in ", swTimer.ElapsedMicroseconds(), " (", std::fixed, (1000000.0 * nTotalTests) / swTimer.ElapsedMicroseconds(), " op/s)");

    uint1024_t hashKey = 0;

    swTimer.reset();
    swTimer.start();
    for(const auto& nBucket : vKeys)
    {
        if(!bloom->ReadKey(nBucket, hashKey))
            return debug::error("Failed to read ", nBucket.SubString());
    }
    swTimer.stop();

    debug::log(0, nTotalTests, " records read in ", swTimer.ElapsedMicroseconds(), " (", std::fixed, (1000000.0 * nTotalTests) / swTimer.ElapsedMicroseconds(), " op/s)");


    std::vector<uint1024_t> vKeys2;
    for(int i = 0; i < nTotalTests; ++i)
        vKeys2.push_back(LLC::GetRand1024());

    swTimer.reset();
    swTimer.start();
    for(const auto& nBucket : vKeys2)
    {
        if(bloom->HasKey(nBucket))
            return debug::error("KEY EXISTS!!! ", nBucket.ToString());
    }
    swTimer.stop();

    debug::log(0, nTotalTests, " records checked in ", swTimer.ElapsedMicroseconds(), " (", std::fixed, (1000000.0 * nTotalTests) / swTimer.ElapsedMicroseconds(), " op/s)");

    delete bloom;

    return 0;
}
