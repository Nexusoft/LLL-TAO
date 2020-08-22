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
#include <LLD/config/hashmap.h>

class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU, LLD::Config::Hashmap>
{
public:
    TestDB(const LLD::Config::Hashmap& config)
    : SectorDatabase(config)
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

#include <TAO/Ledger/include/genesis_block.h>

const uint256_t hashSeed = 55;

#include <bitset>





/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    config::mapArgs["-datadir"] = "/Users/colincantrell/Library/Application Support/Nexus/LIVE";

    /* Create the ContractDB configuration object. */
    LLD::Config::Hashmap Config =
        LLD::Config::Hashmap("testdb", LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);

    /* Set the ContractDB database internal settings. */
    Config.HASHMAP_TOTAL_BUCKETS   = 256 * 256 * 64;
    Config.MAX_HASHMAP_FILES       = 64;
    Config.MAX_LINEAR_PROBES       = 2;
    Config.SECONDARY_BLOOM_BITS    = 8;
    Config.SECONDARY_BLOOM_HASHES  = 5;
    Config.QUICK_INIT              = false;
    Config.MAX_SECTOR_FILE_STREAMS = 16;
    Config.MAX_SECTOR_BUFFER_SIZE  = 1024 * 1024 * 4; //4 MB write buffer
    Config.MAX_SECTOR_CACHE_SIZE   = 1024 * 1024 * 8; //1 MB of cache available

    TestDB* bloom = new TestDB(Config);

    std::vector<uint1024_t> vKeys;
    for(int i = 0; i < 10000; ++i)
        vKeys.push_back(LLC::GetRand1024());

    runtime::stopwatch swTimer;
    swTimer.start();
    for(const auto& nBucket : vKeys)
    {
        bloom->WriteKey(nBucket, nBucket);
    }
    swTimer.stop();

    uint64_t nElapsed = swTimer.ElapsedMicroseconds();
    debug::log(0, "10k records written in ", nElapsed, " (", 10000000000 / nElapsed, " writes/s)");

    uint1024_t hashKey = 0;

    swTimer.reset();
    swTimer.start();

    uint32_t nTotal = 0;
    for(const auto& nBucket : vKeys)
    {
        ++nTotal;

        if(!bloom->ReadKey(nBucket, hashKey))
            return debug::error("Failed to read ", nBucket.SubString(), " total ", nTotal);
    }
    swTimer.stop();

    nElapsed = swTimer.ElapsedMicroseconds();
    debug::log(0, "10k records read in ", nElapsed, " (", 10000000000 / nElapsed, " read/s)");

    delete bloom;

    return 0;
}
