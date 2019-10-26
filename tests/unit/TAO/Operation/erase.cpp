/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>

#include <unit/catch2/catch.hpp>

#include <algorithm>

TEST_CASE( "FLAGS::ERASE Tests", "[erase]")
{
    using namespace TAO::Ledger;
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //main token for split dividends
    uint256_t hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);

    //tokenized asset
    uint256_t hashAsset     = TAO::Register::Address(TAO::Register::Address::OBJECT);

    //token accounts for split claims
    uint256_t hashAccount[2] =
    {
        TAO::Register::Address(TAO::Register::Address::ACCOUNT),
        TAO::Register::Address(TAO::Register::Address::ACCOUNT)
    };

    //nexus accounts for split claims
    uint256_t hashNexus[2] =
    {
        TAO::Register::Address(TAO::Register::Address::ACCOUNT),
        TAO::Register::Address(TAO::Register::Address::ACCOUNT)
    };


    //two users
    uint256_t hashGenesis1[2] =
    {
        TAO::Ledger::SignatureChain::Genesis("testuser"),
        TAO::Ledger::SignatureChain::Genesis("testuser1")
    };

    uint512_t hashPrivKey1[2] =
    {
        LLC::GetRand512(),
        LLC::GetRand512()
    };

    uint512_t hashPrivKey2[2] =
    {
        LLC::GetRand512(),
        LLC::GetRand512()
    };

    uint512_t hashPrevTx[2]
    {
        0,
        0
    };

    //USER A
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object token = CreateToken(hashToken, 1000, 0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashToken, object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashToken, memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //set previous
            hashPrevTx[0] = hash;
        }

        //set address
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount[0] << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount[0], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAccount[0], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint256_t>("token") == hashToken);

            //set previous
            hashPrevTx[0] = hash;
        }


        //set address
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            uint256_t hash256   = TAO::Register::Address(TAO::Register::Address::OBJECT);
            uint512_t hash512   = LLC::GetRand512();
            uint1024_t hash1024 = LLC::GetRand1024();

            std::vector<uint8_t> vBytes(15);
            RAND_bytes((uint8_t*)&vBytes[0], vBytes.size());

            //create object
            Object asset;
            asset << std::string("uint8_t")    << uint8_t(TYPES::UINT8_T)    << uint8_t(55)
                   << std::string("uint16_t")   << uint8_t(TYPES::UINT16_T)   << uint16_t(9383)
                   << std::string("uint32_t")   << uint8_t(TYPES::UINT32_T)   << uint32_t(82384293823)
                   << std::string("uint64_t")   << uint8_t(TYPES::UINT64_T)   << uint64_t(239482349023843984)
                   << std::string("uint256_t")  << uint8_t(TYPES::UINT256_T)  << hash256
                   << std::string("uint512_t")  << uint8_t(TYPES::UINT512_T)  << hash512
                   << std::string("uint1024_t") << uint8_t(TYPES::UINT1024_T) << hash1024
                   << std::string("string")     << uint8_t(TYPES::STRING)     << std::string("this is a string to test long forms")
                   << std::string("bytes")      << uint8_t(TYPES::BYTES)      << vBytes;

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAsset << uint8_t(REGISTER::OBJECT) << asset.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAsset, object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAsset, memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //set previous
            hashPrevTx[0] = hash;
        }


        //set address
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::TRANSFER) << hashAsset << hashToken << uint8_t(TRANSFER::FORCE);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAsset, object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAsset, memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //set previous
            hashPrevTx[0] = hash;
        }



        //create nexus address
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNexus[0] << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[0], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[0], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint256_t>("token") == 0);

            //set previous
            hashPrevTx[0] = hash;
        }
    }


    //USER A
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount[1] << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAccount[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint256_t>("token") == hashToken);

            //set previous
            hashPrevTx[1] = tx.GetHash();
        }


        //create nexus address
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashNexus[1] << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint256_t>("token") == 0);

            //set previous
            hashPrevTx[1] = hash;
        }


        //create some nexus
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis1[1] << uint64_t(1000000000) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //set previous
            hashPrevTx[1] = hash;
        }


        //index to new best block
        BlockState stateBest;
        stateBest.nHeight = 500;

        //write to disk
        uint1024_t hashBest = stateBest.GetHash();
        REQUIRE(LLD::Ledger->WriteBlock(hashBest, stateBest));

        //credit the coinbase
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashPrevTx[1] << uint32_t(0) << hashNexus[1] << hashGenesis1[1] << uint64_t(1000000000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK, &stateBest));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, hashBest));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 1000000000);

            //set previous
            hashPrevTx[1] = hash;
        }
    }


    //distribute the tokens
    {
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount[0] << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashToken, object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashToken, memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 500);

            //set previous
            hashPrevTx[0] = hash;
        }


        //credit account
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashPrevTx[0] << uint32_t(0) << hashAccount[0] << hashToken << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount[0], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAccount[0], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 500);

            //set previous
            hashPrevTx[0] = hash;
        }


        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount[1] << uint64_t(400) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashToken, object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashToken, memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 100);

            //set previous
            hashPrevTx[0] = hash;
        }
    }


    //split dividend payout transaction
    uint512_t hashSplitTx;

    //credit tokens USER 0
    {
        //credit account
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashPrevTx[0] << uint32_t(0) << hashAccount[1] << hashToken << uint64_t(400);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashAccount[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashAccount[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 400);

            //set previous
            hashPrevTx[1] = hash;
        }

        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashNexus[1] << hashAsset << uint64_t(1 * NXS_COIN) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == 999 * NXS_COIN);

            //set previous
            hashPrevTx[1] = hash;

            //set split transaction
            hashSplitTx   = hash;
        }
    }


    {
        //first split dividend payment
        {
            //set private keys
            hashPrivKey1[0] = hashPrivKey2[0];
            hashPrivKey2[0] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[0];
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx[0];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[0], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashSplitTx << uint32_t(0) << hashNexus[0] << hashAccount[0] << uint64_t(0.5 * NXS_COIN);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[0]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //check claimed memory state
            uint64_t nClaimed = 0;

            //should fail disk but pass memory
            REQUIRE_FALSE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nClaimed, FLAGS::BLOCK));
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nClaimed, FLAGS::MEMPOOL));

            //check value
            REQUIRE(nClaimed == uint64_t(0.5 * NXS_COIN));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[0], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[0], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(0.5 * NXS_COIN));

            //disk should pass now
            uint64_t nDiskClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nDiskClaimed, FLAGS::BLOCK));

            //check memory claimed
            uint64_t nMemoryClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nMemoryClaimed, FLAGS::MEMPOOL));

            //check memory and disk
            REQUIRE(nDiskClaimed == nMemoryClaimed);

            //set previous
            hashPrevTx[0] = hash;
        }
    }


    //claim other split dividend payment
    {
        //first split dividend payment
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashSplitTx << uint32_t(0) << hashNexus[1] << hashAccount[1] << uint64_t(0.4 * NXS_COIN);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));

            //check claimed memory state
            uint64_t nClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nClaimed, FLAGS::MEMPOOL));

            //check memory value
            REQUIRE(nClaimed == uint64_t(0.9 * NXS_COIN));

            //check claimed disk state
            uint64_t nDiskClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nDiskClaimed, FLAGS::BLOCK));
            REQUIRE(nDiskClaimed == uint64_t(0.5 * NXS_COIN));

            //write to disk
            REQUIRE(LLD::Ledger->WriteTx(hash, tx));

            //verify prestates and postates (disk)
            REQUIRE(tx.Verify(FLAGS::BLOCK));

            //connect on disk
            REQUIRE(tx.Connect(FLAGS::BLOCK));

            //index to genesis
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));

            //check for account
            Object object;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], object));

            //check memory vs disk
            Object memory;
            REQUIRE(LLD::Register->ReadState(hashNexus[1], memory, FLAGS::MEMPOOL));
            REQUIRE(memory == object);

            //check values
            REQUIRE(object.Parse());
            REQUIRE(object.get<uint64_t>("balance") == uint64_t(999.4 * NXS_COIN));

            //disk should pass now
            nDiskClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nDiskClaimed, FLAGS::BLOCK));

            //check memory claimed
            uint64_t nMemoryClaimed = 0;
            REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nMemoryClaimed, FLAGS::MEMPOOL));

            //check memory and disk
            REQUIRE(nDiskClaimed == nMemoryClaimed);

            //set previous
            hashPrevTx[1] = hash;
        }


        //reverse to self with ERASE
        {
            //set private keys
            hashPrivKey1[1] = hashPrivKey2[1];
            hashPrivKey2[1] = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis1[1];
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx[1];
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2[1], TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashSplitTx << uint32_t(0) << hashNexus[1] << hashNexus[1] << uint64_t(0.1 * NXS_COIN);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign transaction
            REQUIRE(tx.Sign(hashPrivKey1[1]));

            //cache the hash
            uint512_t hash = tx.GetHash();

            //verify prestates and poststates
            REQUIRE(tx.Verify(FLAGS::MEMPOOL));

            //connect in memory
            REQUIRE(tx.Connect(FLAGS::MEMPOOL));


            {
                //check claimed memory state
                uint64_t nClaimed = 0;
                REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nClaimed, FLAGS::MEMPOOL));

                //check memory value
                REQUIRE(nClaimed == uint64_t(1 * NXS_COIN));

                //check claimed disk state
                uint64_t nDiskClaimed = 0;
                REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nDiskClaimed, FLAGS::BLOCK));
                REQUIRE(nDiskClaimed == uint64_t(0.9 * NXS_COIN));
            }



            {
                //check for account
                Object object;
                REQUIRE(LLD::Register->ReadState(hashNexus[1], object, FLAGS::MEMPOOL));

                //check values
                REQUIRE(object.Parse());
                REQUIRE(object.get<uint64_t>("balance") == uint64_t(999.5 * NXS_COIN));
            }

            //test value erase
            REQUIRE(tx.Disconnect(FLAGS::ERASE));

            {
                //check for account
                Object object;
                REQUIRE(LLD::Register->ReadState(hashNexus[1], object));

                //check memory vs disk
                Object memory;
                REQUIRE(LLD::Register->ReadState(hashNexus[1], memory, FLAGS::MEMPOOL));
                REQUIRE(memory == object);

                //check values
                REQUIRE(object.Parse());
                REQUIRE(object.get<uint64_t>("balance") == uint64_t(999.4 * NXS_COIN));
            }


            {
                //check claimed memory state
                uint64_t nClaimed = 0;
                REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nClaimed, FLAGS::MEMPOOL));

                //check memory value
                REQUIRE(nClaimed == uint64_t(0.9 * NXS_COIN));

                //check claimed disk state
                uint64_t nDiskClaimed = 0;
                REQUIRE(LLD::Ledger->ReadClaimed(hashSplitTx, 0, nDiskClaimed, FLAGS::BLOCK));
                REQUIRE(nDiskClaimed == uint64_t(0.9 * NXS_COIN));
            }

        }
    }




    //NOW THAT WE ARE ALL SETUP, LET'S SETUP FOR A SPLIT DIVIDEND PAYMENT






}
