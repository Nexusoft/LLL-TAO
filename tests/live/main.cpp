/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <LLD/include/version.h>

#include <TAO/Register/types/stream.h>

#include <Util/templates/datastream.h>

#include <LLD/hash/xxh3.h>

#include <LLC/aes/aes.h>

#include <LLC/include/random.h>

#include <Util/include/memory.h>

#include <Util/include/hex.h>

#include <bitset>

#include <LLD/templates/sector.h>
#include <LLD/keychain/hashtree.h>
#include <LLD/cache/binary_lru.h>

#include <assert.h>



std::vector< std::vector<uint8_t> > vFill(256);




/*  Find a bucket for cache key management. */
uint32_t Bucket(const std::vector<uint8_t>& vKey)
{
    /* Get an xxHash. */
    uint64_t nBucket = XXH64(&vKey[0], vKey.size(), 0);

    return static_cast<uint32_t>(nBucket % static_cast<uint64_t>(256));
}


uint32_t GetLeaf(const std::vector<uint8_t>& vKey)
{
    return 0;
}

/*  Compresses a given key until it matches size criteria.
 *  This function is one way and efficient for reducing key sizes. */
void CompressKey(std::vector<uint8_t>& vKey, uint16_t nSize = 32)
{
    /* Loop until key is of desired size. */
    while(vKey.size() > nSize)
    {
        /* Loop half of the key to XOR elements. */
        for(uint64_t i = 0; i < vKey.size() / 2; ++i)
            if(i * 2 < vKey.size())
                vKey[i] = vKey[i] ^ vKey[i * 2];

        /* Resize the container to half its size. */
        vKey.resize(std::max(uint16_t(vKey.size() / 2), nSize));
    }
}


namespace LLD
{
    class TestDB : public SectorDatabase<BinaryHashTree, BinaryLRU>
    {
    public:
        TestDB()
        : SectorDatabase(std::string("testing"), FLAGS::CREATE | FLAGS::FORCE)
        {
        }

        bool WriteHash(const uint256_t& hash, const uint256_t& hash2)
        {
            return Write(hash, hash2);
        }

        bool ReadHash(const uint256_t& hash, uint256_t& hash2)
        {
            return Read(hash, hash2);
        }

        bool WriteLast(const uint256_t& last)
        {
            return Write(std::string("last"), last);
        }

        bool ReadLast(uint256_t& last)
        {
            return Read(std::string("last"), last);
        }
    };
}

template< typename Iter1, typename Iter2 >
int32_t equal_safe2( Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2 )
{
    while( begin1 != end1 && begin2 != end2 )
    {
        if( *begin1 != *begin2 )
        {
            return *begin1 - *begin2;
        }
        ++begin1;
        ++begin2;
    }

    return 0;
}


int main(int argc, char** argv)
{
    debug::log(0, "Precomputing");

    uint256_t last = 0;
    {
        LLD::TestDB* db = new LLD::TestDB();

        db->ReadLast(last);

        delete db;
    }



    std::vector<uint256_t> vHashes;
    for(uint256_t i = 0; i < 100000; ++i)
        vHashes.push_back(LLC::GetRand256());

    debug::log(0, "Writing");
    {
        LLD::TestDB* db = new LLD::TestDB();

        runtime::timer timer;
        timer.Start();

        uint256_t hash = 0;
        for(int i = 0; i < 10000; ++i)
        {
            if(!db->WriteHash(vHashes[i], hash))
                return debug::error("Failed on ", i);
        }

        db->WriteLast(last + 100000);

        if(!db->Erase(vHashes[9999]))
            return debug::error("Failed to erase");

        uint64_t nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "100,000 entries written in ", nElapsed, " microseconds");

        delete db;
    }


    debug::log(0, "Reading");
    {
        LLD::TestDB* db = new LLD::TestDB();

        runtime::timer timer;
        timer.Start();

        uint256_t hash2;
        for(int i = 0; i < 10000; ++i)
        {
            if(!db->ReadHash(vHashes[i], hash2))
                return debug::error("Failed on ", i);
        }

        uint64_t nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "100,000 entries read in ", nElapsed, " microseconds");

        delete db;
    }

    return 0;

    {
        std::vector< std::vector< std::vector<uint8_t> > > vFiles;
        for(int i = 0; i < 100000; ++i)
        {
            vFiles.push_back(vFill);
        }

        std::vector<uint256_t> hashes;

        runtime::timer timer;
        timer.Start();
        for(int i = 0; i < 10000; ++i)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);

            uint256_t nRand = LLC::GetRand256();
            hashes.push_back(nRand);

            ssKey << nRand;

            std::vector<uint8_t> vKeyCompressed = ssKey.Bytes();

            uint32_t nBucket = Bucket(vKeyCompressed);

            CompressKey(vKeyCompressed);

            //

            uint32_t nFile = 1;
            for(int i = 0; i < vFiles.size(); ++i)
            {
                if(vFiles[nFile - 1][nBucket].empty())
                {
                    vFiles[nFile - 1][nBucket] = vKeyCompressed;

                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = vKeyCompressed;

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    //PrintHex(vKey.begin(), vKey.end());
                    //PrintHex(vKey2.begin(), vKey2.end());

                    bool fRight = (nCompare > 0);

                    //debug::log(0, "Bucket ", nBucket, " Collision ", nCompare, ", iterating... ", nFile);

                    nFile <<= 1;
                    nFile |= (fRight);

                    if(i > 0)
                    {
                        PrintHex(vKeyCompressed.begin(), vKeyCompressed.end());

                        debug::log(0, std::bitset<32>(nFile), " New File ", nFile, " Iterator ", i, " Compare ", nCompare);

                        runtime::sleep(100);
                    }
                }
            }

            //debug::log(0, "---------------------");
        }

        uint64_t nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "1k entries written in ", nElapsed, " microseconds");


        timer.Reset();
        for(const auto& hash : hashes)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << hash;

            std::vector<uint8_t> vKeyCompressed = ssKey.Bytes();

            uint32_t nBucket = Bucket(vKeyCompressed);

            CompressKey(vKeyCompressed);

            uint32_t nFile = 1;
            while(true)
            {
                if(vFiles[nFile - 1][nBucket] == vKeyCompressed)
                {
                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = vKeyCompressed;

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    //PrintHex(vKey.begin(), vKey.end());
                    //PrintHex(vKey2.begin(), vKey2.end());

                    bool fRight = (nCompare > 0);

                    //debug::log(0, "Bucket ", nBucket, " Collision ", nCompare, ", iterating... ", nFile);

                    nFile <<= 1;
                    nFile |= (fRight);

                    debug::log(0, std::bitset<32>(nFile));
                    debug::log(0, "New File ", nFile);

                    runtime::sleep(1000);
                }
            }

            //debug::log(0, "---------------------");
        }

        nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "1k entries read in ", nElapsed, " microseconds");
    }





    {
        std::vector< uint32_t > vIndex(256, 1);

        std::vector< std::vector< std::vector<uint8_t> > > vFiles;
        for(int i = 0; i < 100000; ++i)
        {
            vFiles.push_back(vFill);
        }

        std::vector<uint256_t> hashes;

        runtime::timer timer;
        timer.Start();
        for(int i = 0; i < 10000; ++i)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);

            uint256_t nRand = LLC::GetRand256();
            hashes.push_back(nRand);

            ssKey << nRand;

            std::vector<uint8_t> vKeyCompressed = ssKey.Bytes();

            uint32_t nBucket = Bucket(vKeyCompressed);

            CompressKey(vKeyCompressed);

            uint32_t nFile = vIndex[nBucket];
            while(true)
            {
                if(vFiles[nFile - 1][nBucket].empty())
                {
                    vFiles[nFile - 1][nBucket] = vKeyCompressed;

                    ++vIndex[nBucket];

                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = vKeyCompressed;

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    --nFile;
                }
            }

            //debug::log(0, "---------------------");
        }

        uint64_t nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "1k entries written in ", nElapsed, " microseconds");


        timer.Reset();
        for(const auto& hash : hashes)
        {
            DataStream ssKey(SER_LLD, LLD::DATABASE_VERSION);
            ssKey << hash;

            std::vector<uint8_t> vKeyCompressed = ssKey.Bytes();

            uint32_t nBucket = Bucket(vKeyCompressed);

            CompressKey(vKeyCompressed);

            uint32_t nFile = 1;
            while(true)
            {
                if(vFiles[nFile - 1][nBucket] == vKeyCompressed)
                {
                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = vKeyCompressed;

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    ++nFile;
                }
            }

            //debug::log(0, "---------------------");
        }

        nElapsed = timer.ElapsedMicroseconds();
        debug::log(0, "1k entries read in ", nElapsed, " microseconds");
    }



    return 0;
}
