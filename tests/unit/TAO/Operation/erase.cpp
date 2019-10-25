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

#include <unit/catch2/catch.hpp>

#include <algorithm>

TEST_CASE( "FLAGS::ERASE Tests", "[erase]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    TAO::Register::Address hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);
    TAO::Register::Address hashAsset     = TAO::Register::Address(TAO::Register::Address::OBJECT);


    //create object
    uint256_t hashGenesis1[2] = { TAO::Ledger::SignatureChain::Genesis("testuser"), TAO::Ledger::SignatureChain::Genesis("testuser1") };
    uint512_t hashPrivKey1[2] = { LLC::GetRand512(), LLC::GetRand512() };
    uint512_t hashPrivKey2[2] = { LLC::GetRand512(), LLC::GetRand512() };

    uint512_t hashPrevTx[2];

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
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1[0]);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx[0] = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
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
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1[0]);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx[0] = tx.GetHash();
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
            Object object;
            object << std::string("uint8_t")    << uint8_t(TYPES::UINT8_T)    << uint8_t(55)
                   << std::string("uint16_t")   << uint8_t(TYPES::UINT16_T)   << uint16_t(9383)
                   << std::string("uint32_t")   << uint8_t(TYPES::UINT32_T)   << uint32_t(82384293823)
                   << std::string("uint64_t")   << uint8_t(TYPES::UINT64_T)   << uint64_t(239482349023843984)
                   << std::string("uint256_t")  << uint8_t(TYPES::UINT256_T)  << hash256
                   << std::string("uint512_t")  << uint8_t(TYPES::UINT512_T)  << hash512
                   << std::string("uint1024_t") << uint8_t(TYPES::UINT1024_T) << hash1024
                   << std::string("string")     << uint8_t(TYPES::STRING)     << std::string("this is a string to test long forms")
                   << std::string("bytes")      << uint8_t(TYPES::BYTES)      << vBytes;

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAsset << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1[0]);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx[0] = tx.GetHash();
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

            //sign
            tx.Sign(hashPrivKey1[0]);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx[0] = tx.GetHash();
        }
    }


    //USER A
    {
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
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
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1[1]);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx[1] = tx.GetHash();
        }
    }


    {
        //check mempool list sequencing
        std::vector<uint512_t> vHashes;
        REQUIRE(TAO::Ledger::mempool.List(vHashes));

        //commit memory pool transactions to disk
        for(auto& hash : vHashes)
        {
            TAO::Ledger::Transaction tx;
            REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

            LLD::Ledger->WriteTx(hash, tx);
            REQUIRE(tx.Verify());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            REQUIRE(LLD::Ledger->IndexBlock(hash, TAO::Ledger::ChainState::Genesis()));
            REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
        }
    }



    //NOW THAT WE ARE ALL SETUP, LET'S SETUP FOR A SPLIT DIVIDEND PAYMENT






}
