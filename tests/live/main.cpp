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

    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }


    bool ReadLast(uint1024_t& last)
    {
        return Read(std::string("last"), last);
    }


    bool WriteHash(const uint1024_t& hash, const uint1024_t& hash2)
    {
        return Write(std::make_pair(std::string("hash"), hash), hash2, "hash");
    }

    bool WriteHash2(const uint1024_t& hash, const uint1024_t& hash2)
    {
        return Write(std::make_pair(std::string("hash2"), hash), hash2, "hash");
    }

    bool IndexHash(const uint1024_t& hash)
    {
        return Index(std::make_pair(std::string("hash2"), hash), std::make_pair(std::string("hash"), hash));
    }


    bool ReadHash(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash"), hash), hash2);
    }


    bool ReadHash2(const uint1024_t& hash, uint1024_t& hash2)
    {
        return Read(std::make_pair(std::string("hash2"), hash), hash2);
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

    return 0;
}
