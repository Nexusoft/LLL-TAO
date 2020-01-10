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
#include <variant>

#include <Util/include/softfloat.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/locator.h>

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


/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    for(int i = 0; i < 10; ++i)
    {
        uint64_t nTimestamp = runtime::timestamp();
        uint32_t nInterval  = nTimestamp / 2;

        DataStream ssData(SER_LLD, LLD::DATABASE_VERSION);
        ssData << hashSeed << nInterval;

        std::vector<uint8_t> vResult = LLC::SK256(ssData.begin(), ssData.end()).GetBytes();

        //run HMAC generator
        uint32_t nOffset   =  vResult[31] & 0xf ;
        uint32_t nBinary =
             (vResult[nOffset + 0] & 0x7f) << 24
           | (vResult[nOffset + 1] & 0xff) << 16
           | (vResult[nOffset + 2] & 0xff) <<  8
           | (vResult[nOffset + 3] & 0xff) ;

        uint32_t nCode = nBinary % 1000000;

        debug::log(0, "Code is ", nCode);

        runtime::sleep(1000);
    }


    return 0;

    config::mapArgs["-datadir"] = "/public/tests";

    //TestDB* db = new TestDB();

    uint1024_t hashLast = 0;
    //db->ReadLast(hashLast);

    std::fstream stream1(config::GetDataDir() + "/test1.txt", std::ios::in | std::ios::out | std::ios::binary);
    std::fstream stream2(config::GetDataDir() + "/test2.txt", std::ios::in | std::ios::out | std::ios::binary);
    std::fstream stream3(config::GetDataDir() + "/test3.txt", std::ios::in | std::ios::out | std::ios::binary);
    std::fstream stream4(config::GetDataDir() + "/test4.txt", std::ios::in | std::ios::out | std::ios::binary);
    std::fstream stream5(config::GetDataDir() + "/test5.txt", std::ios::in | std::ios::out | std::ios::binary);

    std::vector<uint8_t> vBlank(1024, 0); //1 kb

    stream1.write((char*)&vBlank[0], vBlank.size());
    stream2.write((char*)&vBlank[0], vBlank.size());
    stream3.write((char*)&vBlank[0], vBlank.size());
    stream4.write((char*)&vBlank[0], vBlank.size());
    stream5.write((char*)&vBlank[0], vBlank.size());

    runtime::timer timer;
    timer.Start();
    for(uint64_t n = 0; n < 100000; ++n)
    {
        stream1.seekp(0, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());

        stream1.seekp(8, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());

        stream1.seekp(16, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());

        stream1.seekp(32, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());

        stream1.seekp(64, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());
        stream1.flush();

        //db->WriteKey(n, n);
    }

    debug::log(0, "Wrote 100k records in ", timer.ElapsedMicroseconds(), " micro-seconds");


    timer.Reset();
    for(uint64_t n = 0; n < 100000; ++n)
    {
        stream1.seekp(0, std::ios::beg);
        stream1.write((char*)&vBlank[0], vBlank.size());
        stream1.flush();

        stream2.seekp(8, std::ios::beg);
        stream2.write((char*)&vBlank[0], vBlank.size());
        stream2.flush();

        stream3.seekp(16, std::ios::beg);
        stream3.write((char*)&vBlank[0], vBlank.size());
        stream3.flush();

        stream4.seekp(32, std::ios::beg);
        stream4.write((char*)&vBlank[0], vBlank.size());
        stream4.flush();

        stream5.seekp(64, std::ios::beg);
        stream5.write((char*)&vBlank[0], vBlank.size());
        stream5.flush();
        //db->WriteKey(n, n);
    }
    timer.Stop();

    debug::log(0, "Wrote 100k records in ", timer.ElapsedMicroseconds(), " micro-seconds");

    //db->WriteLast(hashLast + 100000);

    return 0;
}
