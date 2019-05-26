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
#include <TAO/Register/include/system.h>
#include <TAO/Register/include/verify.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/enum.h>

#include <unit/catch2/catch.hpp>

#include <algorithm>

TEST_CASE( "Mempool and memory sequencing tests", "[ledger]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //create a list of transactions
    {

        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashGenesis  = LLC::GetRand256();
        uint512_t hashPrivKey1  = LLC::GetRand512();
        uint512_t hashPrivKey2  = LLC::GetRand512();

        uint512_t hashPrevTx;

        uint256_t hashToken     = LLC::GetRand256();
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
            Object token = CreateToken(11, 1000, 100);

            //payload
            tx << uint8_t(OP::REGISTER) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        uint256_t hashAccount = LLC::GetRand256();
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(11);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        uint256_t hashAddress = LLC::GetRand256();
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object2, FLAGS::MEMPOOL));

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("this string"));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("test") << uint8_t(OP::TYPES::STRING) << std::string("stRInGISNew");

            //payload
            tx << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("byte") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(13);

            //payload
            tx << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(13));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashToken, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 900);

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashToken, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 400);

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashToken, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 100);

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        //test a failure
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));
        }



        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //payload
            tx << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::regDB->ReadState(hashToken, object2, FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 0);

            //make sure not on disk
            REQUIRE(!LLD::regDB->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
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

                LLD::legDB->WriteTx(hash, tx);
                REQUIRE(Verify(tx, FLAGS::WRITE));
                REQUIRE(Execute(tx, FLAGS::WRITE));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }

            //check token balance with mempool flag on
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::regDB->ReadState(hashToken, object2, FLAGS::MEMPOOL));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 0);
            }


            //check token balance from disk only
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::regDB->ReadState(hashToken, object2));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 0);
            }
        }
    }








    //handle out of order transactions
    {

        //cleanup
        LLD::regDB->EraseIdentifier(22);

        //vector to shuffle
        std::vector<TAO::Ledger::Transaction> vTX;

        //create object
        uint256_t hashGenesis  = LLC::GetRand256();
        uint512_t hashPrivKey1  = LLC::GetRand512();
        uint512_t hashPrivKey2  = LLC::GetRand512();

        uint512_t hashPrevTx;

        uint256_t hashToken     = LLC::GetRand256();
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
            Object token = CreateToken(22, 1000, 100);

            //payload
            tx << uint8_t(OP::REGISTER) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }

        //set address
        uint256_t hashAccount = LLC::GetRand256();
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object account = CreateAccount(11);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }

        //set address
        uint256_t hashAddress = LLC::GetRand256();
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }



        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = LLC::GetRand256();
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 9;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2, TAO::Ledger::SIGNATURE::BRAINPOOL);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("identifier") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx << uint8_t(OP::REGISTER) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        {
            //random shuffle the list for sequencing
            std::random_shuffle(vTX.begin(), vTX.end());

            //accept all transactions in random ordering
            for(auto& tx : vTX)
            {
                REQUIRE(TAO::Ledger::mempool.Accept(tx));
            }

            //check mempool list sequencing
            std::vector<uint512_t> vHashes;
            REQUIRE(TAO::Ledger::mempool.List(vHashes));

            //commit memory pool transactions to disk
            for(auto& hash : vHashes)
            {
                TAO::Ledger::Transaction tx;
                REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

                LLD::legDB->WriteTx(hash, tx);
                REQUIRE(Verify(tx, FLAGS::WRITE));
                REQUIRE(Execute(tx, FLAGS::WRITE));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }
        }
    }
}
