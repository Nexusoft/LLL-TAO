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

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/shard_hashmap.h>
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

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <list>
#include <variant>


class TestDB : public LLD::SectorDatabase<LLD::ShardHashMap, LLD::BinaryLRU>
{
public:
    TestDB()
    : SectorDatabase("testdb"
    , LLD::FLAGS::CREATE | LLD::FLAGS::FORCE
    , 256 * 256 * 64
    , 1024 * 1024 * 4)
    {
    }

    ~TestDB()
    {

    }

    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }

    bool ReadLast(uint1024_t& last)
    {
        return Read(std::string("last"), last);
    }


    bool WriteHash(const uint1024_t& hash)
    {
        return Write(std::make_pair(std::string("hash"), hash), hash, "hash");
    }

    bool ReadHash(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash"), hash), hash2);
    }


    bool EraseHash(const uint1024_t& hash)
    {
        return Erase(std::make_pair(std::string("hash"), hash));
    }
};


/*
Hash Tables:

Set max tables per timestamp.

Keep disk index of all timestamps in memory.

Keep caches of Disk Index files (LRU) for low memory footprint

Check the timestamp range of bucket whether to iterate forwards or backwards


_hashmap.000.0000
_name.shard.file
  t0 t1 t2
  |  |  |

  timestamp each hashmap file if specified
  keep indexes in TemplateLRU

  search from nTimestamp < timestamp[nShard][nHashmap]



*/


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    //uint1024_t hash = ;

    TestDB* testDB = new TestDB();

    std::vector<uint1024_t> vRecords;
    if(!testDB->BatchRead("hash", vRecords, 10))
        return debug::error("failed to batch read");

    for(const auto& a : vRecords)
    {
        debug::log(0, "Record ", a.Get64());
        if(!testDB->Erase(std::make_pair(std::string("hash"), a)))
            return debug::error("failed to erase");
    }

    return 0;

    for(int t = 0; t < 1000; ++t)
    {
        uint1024_t last = 0;
        testDB->ReadLast(last);

        debug::log(0, "Last is ", last.Get64());

        runtime::timer timer;
        timer.Start();
        for(uint1024_t i = 1 ; i < last; ++i)
        {
            if(i.Get64() % 10000 == 0)
            {
                uint64_t nElapsed = timer.ElapsedMilliseconds();
                debug::log(0, "Read ", i.Get64(), " Total Records in ", nElapsed, " ms (", i.Get64() * 1000 / nElapsed, " per / s)");
            }

            uint1024_t hash;
            testDB->ReadHash(i, hash);
        }


        timer.Reset();
        uint32_t nTotal = 0;
        for(uint1024_t i = last + 1 ; i < last + 100000; ++i)
        {
            if(++nTotal % 10000 == 0)
            {
                uint64_t nElapsed = timer.ElapsedMilliseconds();
                debug::log(0, "Wrote ", nTotal, " Total Records in ", nElapsed, " ms (", nTotal * 1000 / nElapsed, " per / s)");
            }

            testDB->WriteHash(i);
        }

        testDB->WriteLast(last + 100000);

    }

    delete testDB;



    return 0;
}
