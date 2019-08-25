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
    , LLD::FLAGS::CREATE | LLD::FLAGS::WRITE
    , 1024 * 1024
    , 1024)
    {
    }

    ~TestDB()
    {

    }

    bool WriteHash(const uint1024_t& hash)
    {
        return Write(std::make_pair(std::string("hash"), hash), hash);
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
    for(uint1024_t i = 0; i < 100000; ++i)
    {
        if(i.Get64() % 100000 == 0)
            debug::log(0, "Written ", i.Get64(), " Total Records...");

        testDB->WriteHash(i);
    }

    delete testDB;



    return 0;
}
