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
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <LLD/templates/bloom.h>

class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
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

    bool WriteKey(const uint1024_t& key, const uint1024_t& value)
    {
        return Write(std::make_pair(std::string("key"), key), value);
    }


    bool ReadKey(const uint1024_t& key, uint1024_t &value)
    {
        return Read(std::make_pair(std::string("key"), key), value);
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


const uint256_t hashSeed = 55;


#include <bitset>


static std::atomic<uint32_t> nThread;
void Testing()
{
    ++nThread;
    std::fstream pstream = std::fstream(config::GetDataDir() + "/TEST/files/" + debug::safe_printstr("_file.", nThread.load()), std::ios::in | std::ios::out | std::ios::binary);

    runtime::timer swFiles;
    swFiles.Start();

    std::vector<uint8_t> vData(1024, 0);
    pstream.write((char*)&vData[0], vData.size());
    pstream.flush();

    debug::log(0, "Completed in ", swFiles.ElapsedNanoseconds(), " microseconds");
}

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    uint512_t hash = 293548230430984;
    LLD::BloomFilter* bloom = new LLD::BloomFilter(256 * 256 * 64 * 4.4, config::GetDataDir() + "/TEST/bloom/", 3);
    bloom->Initialize();
    //bloom->Insert(hash + 3);

    for(; hash < 293548230430984 + 10; ++hash)
        debug::log(0, "Has (", hash.SubString(), ") Item ", bloom->Has(hash) ? "TRUE" : "FALSE");

    hash = 823828;
    bloom->Insert(hash);

    debug::log(0, "------------ NEXT SET -------------");
    for(; hash < 823828 + 10; ++hash)
        debug::log(0, "Has (", hash.SubString(), ") Item ", bloom->Has(hash) ? "TRUE" : "FALSE");

    //bloom.Flush();
    delete bloom;



    return 0;

    TestDB* db = new TestDB();

    uint1024_t hashLast = 0;
    db->ReadLast(hashLast);

    runtime::timer timer;
    timer.Start();
    for(uint1024_t n = hashLast; n < hashLast + 100000; ++n)
    {
        db->WriteKey(n, n);
    }

    debug::log(0, "Wrote 100k records in ", timer.ElapsedMicroseconds(), " micro-seconds");

    db->WriteLast(hashLast + 100000);

    return 0;
}
