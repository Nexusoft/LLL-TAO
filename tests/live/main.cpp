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
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

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
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Initialize LLD. */
    LLD::Initialize();


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);



    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();



    config::fTestNet.store(true);
    config::fPrivate.store(true);

    //create object
    uint256_t hashGenesis   = TAO::Ledger::SignatureChain::Genesis("testuser");
    uint512_t hashPrivKey1  = LLC::GetRand512();
    uint512_t hashPrivKey2  = LLC::GetRand512();

    uint512_t hashPrevTx;

    TAO::Register::Address hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create object
        Object token = CreateToken(hashToken, 1000000000, 0);

        //payload
        tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

        //generate the prestates and poststates
        tx.Build();

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        TAO::Ledger::mempool.Accept(tx);

        //set previous
        hashPrevTx = tx.GetHash();

        //commit to disk
        LLD::Ledger->WriteTx(hashPrevTx, tx);
        tx.Connect(TAO::Ledger::FLAGS::BLOCK);

        LLD::Ledger->IndexBlock(hashPrevTx, TAO::Ledger::ChainState::Genesis());
        TAO::Ledger::mempool.Remove(hashPrevTx);
    }


    TAO::Register::Address hashAccount     = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
    {
        //set private keys
        hashPrivKey1 = hashPrivKey2;
        hashPrivKey2 = LLC::GetRand512();

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.hashPrevTx  = hashPrevTx;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //create object
        Object account = CreateAccount(hashToken);

        //payload
        tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

        //generate the prestates and poststates
        tx.Build();

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        TAO::Ledger::mempool.Accept(tx);

        //set previous
        hashPrevTx = tx.GetHash();

        //commit to disk
        LLD::Ledger->WriteTx(hashPrevTx, tx);
        tx.Connect(TAO::Ledger::FLAGS::BLOCK);

        LLD::Ledger->IndexBlock(hashPrevTx, TAO::Ledger::ChainState::Genesis());
        TAO::Ledger::mempool.Remove(hashPrevTx);
    }


    {
        //set private keys
        hashPrivKey1 = hashPrivKey2;
        hashPrivKey2 = LLC::GetRand512();

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.hashPrevTx  = hashPrevTx;
        tx.nSequence   = 2;
        tx.nTimestamp  = runtime::timestamp();
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

        //payload
        for(int i = 0; i < 10; ++i)
            tx[i] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(1) << uint64_t(0);

        //generate the prestates and poststates
        tx.Build();

        //sign
        tx.Sign(hashPrivKey1);

        //commit to disk
        TAO::Ledger::mempool.Accept(tx);

        //set previous
        hashPrevTx = tx.GetHash();

        //commit to disk
        LLD::Ledger->WriteTx(hashPrevTx, tx);
        tx.Connect(TAO::Ledger::FLAGS::BLOCK);

        LLD::Ledger->IndexBlock(hashPrevTx, TAO::Ledger::ChainState::Genesis());
        TAO::Ledger::mempool.Remove(hashPrevTx);
    }


    return 0;



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

    LLP::TritiumNode node;
    node.Connect(LLP::BaseAddress("104.192.170.154", 9888));

    node.PushMessage(LLP::ACTION::VERSION, LLP::PROTOCOL_VERSION, uint64_t(3838238), std::string("Test Client"));

    node.Subscribe(LLP::SUBSCRIPTION::LASTINDEX | LLP::SUBSCRIPTION::BESTCHAIN | LLP::SUBSCRIPTION::BESTHEIGHT);

    /* Ask for list of blocks if this is current sync node. */
    for(int i = 0; i < 36000; ++i)
    {
        debug::log(0, "Sending List");
        node.PushMessage(LLP::ACTION::LIST,
            uint8_t(LLP::SPECIFIER::SYNC),
            uint8_t(LLP::TYPES::BLOCK),
            uint8_t(LLP::TYPES::UINT1024_T),
            TAO::Ledger::ChainState::Genesis(),
            uint1024_t(0)
        );

        node.ReadPacket();
        if(node.PacketComplete())
            node.ResetPacket();

        runtime::sleep(100);
    }


    runtime::sleep(10000);

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
