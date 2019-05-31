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

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Debit Primitive Tests", "[operation]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //check a debit from token
    {
        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(11, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(11);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object token;
                REQUIRE(LLD::regDB->ReadState(hashToken, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 500);
            }
        }
    }


    //check a debit from token
    {
        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(11, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(11);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(1000);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object token;
                REQUIRE(LLD::regDB->ReadState(hashToken, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 0);
            }
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(1);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("account doesn't have sufficient balance") != std::string::npos);
        }
    }


    //check for failed by account balance
    {
        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(11, 100, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(11);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("account doesn't have sufficient balance") != std::string::npos);
        }
    }


    //check for failed by reserved
    {

        //create object
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(11);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << uint256_t(0) << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit from register with reserved address") != std::string::npos);
        }
    }


    //check for failed by reserved
    {

        //create object
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(11);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashAccount << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit to the same address as from") != std::string::npos);
        }
    }


    //check for failed by incorrect base
    {
        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token;

            /* Generate the object register values. */
            token   << std::string("balance")    << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("identifier") << uint8_t(TYPES::UINT256_T) << uint256_t(11)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::RAW) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("failed to parse account object register") != std::string::npos);
        }
    }


    //check for failed by incorrect base
    {
        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAccount  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token;

            /* Generate the object register values. */
            token   << std::string("balance")    << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("identifier") << uint8_t(TYPES::UINT256_T) << uint256_t(11)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("digits")     << uint8_t(TYPES::UINT64_T) << uint64_t(100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(!Execute(tx, FLAGS::WRITE));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit from non-standard object register") != std::string::npos);
        }
    }
}
