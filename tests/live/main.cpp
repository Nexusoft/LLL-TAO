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
    };
}


int main(int argc, char** argv)
{

    uint256_t hash = LLC::GetRand256();
    {
        LLD::TestDB* db = new LLD::TestDB();

        runtime::timer timer;
        timer.Start();
        for(int i = 0; i < 100000; ++i)
        {
            db->WriteHash(hash + i, hash);
        }

        uint64_t nElapsed = timer.ElapsedMilliseconds();
        debug::log(0, "100,000 entries written in ", nElapsed, " milliseconds");

        delete db;
    }


    {
        LLD::TestDB* db = new LLD::TestDB();

        runtime::timer timer;
        timer.Start();

        uint256_t hash2;
        for(int i = 0; i < 100000; ++i)
        {
            db->ReadHash(hash + i, hash2);
        }

        uint64_t nElapsed = timer.ElapsedMilliseconds();
        debug::log(0, "100,000 entries read in ", nElapsed, " milliseconds");

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

            uint32_t nBucket = Bucket(ssKey.Bytes());

            uint32_t nFile = 1;
            while(true)
            {
                if(vFiles[nFile - 1][nBucket].empty())
                {
                    vFiles[nFile - 1][nBucket] = ssKey.Bytes();

                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = ssKey.Bytes();

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    //PrintHex(vKey.begin(), vKey.end());
                    //PrintHex(vKey2.begin(), vKey2.end());

                    bool fRight = (nCompare > 0);

                    //debug::log(0, "Bucket ", nBucket, " Collision ", nCompare, ", iterating... ", nFile);

                    nFile <<= 1;
                    nFile |= (fRight);

                    //debug::log(0, std::bitset<32>(nFile));
                    //debug::log(0, "New File ", nFile);
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

            uint32_t nBucket = Bucket(ssKey.Bytes());

            uint32_t nFile = 1;
            while(true)
            {
                if(vFiles[nFile - 1][nBucket] == ssKey.Bytes())
                {
                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = ssKey.Bytes();

                    int32_t nCompare = memory::compare((uint8_t*)&vKey[0], (uint8_t*)&vKey2[0], std::min(vKey.size(), vKey2.size()));

                    //PrintHex(vKey.begin(), vKey.end());
                    //PrintHex(vKey2.begin(), vKey2.end());

                    bool fRight = (nCompare > 0);

                    //debug::log(0, "Bucket ", nBucket, " Collision ", nCompare, ", iterating... ", nFile);

                    nFile <<= 1;
                    nFile |= (fRight);

                    //debug::log(0, std::bitset<32>(nFile));
                    //debug::log(0, "New File ", nFile);
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

            uint32_t nBucket = Bucket(ssKey.Bytes());

            uint32_t nFile = vIndex[nBucket];
            while(true)
            {
                if(vFiles[nFile - 1][nBucket].empty())
                {
                    vFiles[nFile - 1][nBucket] = ssKey.Bytes();

                    ++vIndex[nBucket];

                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = ssKey.Bytes();

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

            uint32_t nBucket = Bucket(ssKey.Bytes());

            uint32_t nFile = 1;
            while(true)
            {
                if(vFiles[nFile - 1][nBucket] == ssKey.Bytes())
                {
                    break;
                }
                else
                {
                    std::vector<uint8_t> vKey2 = vFiles[nFile - 1][nBucket];
                    std::vector<uint8_t> vKey  = ssKey.Bytes();

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
