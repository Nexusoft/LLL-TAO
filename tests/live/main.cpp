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


class Test
{
public:

    Test()
    {

    }

    /** Subscribed
     *
     *  Determine if a node is subscribed to receive relay message.
     *
     **/
    template<typename MessageType>
    bool Subscribed(const MessageType& message)
    {
        return debug::error("lower order function ", message);
    }
};


class Test2 : public Test
{
public:

    Test2()
    {

    }

    /** Subscribed
     *
     *  Determine if a node is subscribed to receive relay message.
     *
     **/
    bool Subscribed(const uint16_t& message)
    {
        return debug::error("Higher order function ", message);
    }
};

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/types/condition.h>

#include <Util/include/softfloat.h>
#include <TAO/Ledger/include/prime.h>

#include <Util/include/condition_variable.h>


condition_variable condition;

uint32_t nValue = 0;

void Thread()
{
    while(true)
    {
        debug::log(0, "Waiting for work!");
        condition.wait([]{ return nValue > 0; });

        debug::log(0, "Doing Work! ", nValue);
    }
}

void Thread2()
{
    while(true)
    {
        runtime::sleep(1);
        condition.notify_one();
    }
}

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    uint1024_t hashOrigin = uint1024_t("0x01001a8938c1b4620c106b072a02c8799da027dabfd864e9130d4ee32e8fbf67280d93e21424ee57d2045a0a1e88a4af6a74f2b1eaddede19fb3a7cea2eef8dcfe4c7f0099c9324eb49f7f06eb15e77a121a4919a27d2fc67ba0de6a68a78ad53bd77fd8f6adad237747c08582634526737b544b073464addf3c7df6f67eb162");
    uint64_t nNonce = 2705923963233114459;


    std::vector<uint8_t> vOffsets;

    runtime::timer benchmark;
    benchmark.Start();

    double nDifficulty = TAO::Ledger::GetPrimeDifficulty(hashOrigin + nNonce, vOffsets);

    debug::log(0, "Difficulty ", nDifficulty, " in ", benchmark.ElapsedMilliseconds(), "ms");

    TAO::Ledger::GetOffsets(hashOrigin + nNonce, vOffsets);

    benchmark.Reset();
    double nDifficulty2 = TAO::Ledger::GetPrimeDifficulty(hashOrigin + nNonce, vOffsets, true);

    debug::log(0, "Difficulty ", nDifficulty2, " in ", benchmark.ElapsedMilliseconds(), "ms");

    return 0;


    TestDB* testDB = new TestDB();

        contract <= uint8_t(OP::CONTRACT::OPERATIONS) <= uint8_t(OP::SUBDATA) <= uint16_t(1) <= uint16_t(32); //hashFrom
        contract <= uint8_t(OP::NOTEQUALS); //if the proof is not the hashFrom we can assume it is a split dividend payment
        contract <= uint8_t(0xd3)   <= uint8_t(OP::SUBDATA) <= uint16_t(101) <= uint16_t(32);  //hashProof

        if(!TAO::Operation::Condition::Validate(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");
    }

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);

    debug::log(0, "Write Hash");
    debug::log(0, "Hash ", hash.Get64());
    testDB->WriteHash(hash, hash);

        if(!TAO::Operation::Condition::Validate(contract, vWarnings))
            debug::error("Validate Error");
        else
            debug::error("Validate Success");
    }

    //testDB->TxnBegin();

        contract <= uint8_t(OP::AND);

        contract <= (uint8_t)OP::TYPES::UINT64_T <= std::numeric_limits<uint64_t>::max() <= (uint8_t) OP::ADD <= (uint8_t)OP::TYPES::UINT64_T <= uint64_t(100) <= (uint8_t)OP::EQUALS <= (uint8_t)OP::TYPES::UINT32_T <= 222u;
        
        contract <= uint8_t(OP::OR);

        /* 5 + 5 = 10 */
        contract <= uint8_t(OP::GROUP);
        contract <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::NOTEQUALS);
        contract <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::UNGROUP);

    //testDB->TxnCheckpoint();
    //testDB->TxnCommit();


    testDB->WriteHash(hash, hashTest3 + 3);

    uint1024_t hashN = 0;
    testDB->ReadHash(hash, hashN);

    testDB->WriteHash2(hash, hashTest3 + 10);

    testDB->ReadHash2(hash, hashN);



    {
        debug::log(0, "Read Hash");
        uint1024_t hashTest4;
        testDB->ReadHash(hash, hashTest4);

        /* Create 1024 bytes if data */
        std::vector<uint8_t> vData(1024);

    {
        debug::log(0, "Read Hash");
        uint1024_t hashTest4;
        testDB->ReadHash2(hash, hashTest4);

        Condition condition(contract, caller);
        if(!condition.Execute())
            debug::error("Execute Error");
        else
            debug::error("Execute Success");
    }

    {
        TAO::Operation::Contract contract;
        contract << uint8_t(OP::TRANSFER) << hashAsset << hashGenesis << uint8_t(TAO::Operation::TRANSFER::CLAIM);
    
        contract <= uint8_t(OP::CONTRACT::TIMESTAMP) <= uint8_t(OP::ADD) <= uint8_t(OP::TYPES::UINT64_T) <= uint64_t(5);
        contract <= uint8_t(OP::GREATERTHAN) <= uint8_t(OP::CALLER::TIMESTAMP);

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

            testDB->WriteHash(i, i);
        }

        testDB->WriteLast(last + 100000);

    }




    return 0;
}
