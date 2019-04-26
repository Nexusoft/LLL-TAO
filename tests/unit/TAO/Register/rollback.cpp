/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Register Rollback Tests", "[register]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //rollback a token object register
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = LLC::GetRand256();
        tx.nSequence   = 0;
        tx.nTimestamp  = runtime::timestamp();

        //cleanup from last time
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashRegister = LLC::GetRand256();
        Object account = CreateToken(11, 1000, 100);

        //payload
        tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::OBJECT) << account.GetState();

        //generate the prestates and poststates
        REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

        //commit to disk
        REQUIRE(Execute(tx, FLAGS::WRITE));

        //check for reserved identifier
        REQUIRE(LLD::regDB->HasIdentifier(11));

        //check that register exists
        REQUIRE(LLD::regDB->HasState(hashRegister));

        //rollback the transaction
        REQUIRE(Rollback(tx));

        //check reserved identifier
        REQUIRE(!LLD::regDB->HasIdentifier(11));

        //make sure object is deleted
        REQUIRE(!LLD::regDB->HasState(hashRegister));
    }


    //rollback a state write
    {
        //create object
        uint256_t hashRegister = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::APPEND) << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(10, 0xff));

        }

        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::WRITE) << hashRegister << std::vector<uint8_t>(10, 0x1f);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(10, 0x1f));

            //rollback the transaction
            REQUIRE(Rollback(tx));

            //grab the new state
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(10, 0xff));
        }


        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::APPEND) << hashRegister << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(20, 0xff));

            //rollback the transaction
            REQUIRE(Rollback(tx));

            //grab the new state
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(10, 0xff));
        }
    }



    //rollback a transfer
    {
        //create object
        uint256_t hashRegister = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::RAW) << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check owner
            REQUIRE(state.hashOwner == hashGenesis);

        }

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx << uint8_t(OP::TRANSFER) << hashRegister << uint256_t(0xffff);

        //generate the prestates and poststates
        REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

        //write transaction
        REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

        //commit to disk
        REQUIRE(Execute(tx, FLAGS::WRITE));

        //check that register exists
        State state;
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == 0xffff);

        //rollback the transaction
        REQUIRE(Rollback(tx));

        //grab the new state
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == hashGenesis);
    }



    {
        //create object
        uint256_t hashRegister = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::RAW) << std::vector<uint8_t>(0xff, 0);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

            //check owner
            REQUIRE(state.hashOwner == hashGenesis);

        }

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx << uint8_t(OP::TRANSFER) << hashRegister << uint256_t(0xffff);

        //generate the prestates and poststates
        REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

        //write transaction
        REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

        //commit to disk
        REQUIRE(Execute(tx, FLAGS::WRITE));

        //check that register exists
        State state;
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == 0xffff);

        //rollback the transaction
        REQUIRE(Rollback(tx));

        //grab the new state
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == hashGenesis);
    }


    //rollback a debit from token
    {
        //create object
        uint256_t hashRegister = LLC::GetRand256();
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
            tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::OBJECT) << token.GetState();

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
            tx << uint8_t(OP::REGISTER) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

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
            tx << uint8_t(OP::DEBIT) << hashRegister << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object token;
                REQUIRE(LLD::regDB->ReadState(hashRegister, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 500);
            }

            //rollback
            REQUIRE(Rollback(tx));

            //check register values
            {
                Object token;
                REQUIRE(LLD::regDB->ReadState(hashRegister, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 1000);
            }
        }


        //cleanup from last time
        LLD::regDB->EraseIdentifier(11);

        //rollback a credit from token
        {
            //create object
            uint256_t hashRegister = LLC::GetRand256();
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
                tx << uint8_t(OP::REGISTER) << hashRegister << uint8_t(REGISTER::OBJECT) << token.GetState();

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
                tx << uint8_t(OP::REGISTER) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

                //generate the prestates and poststates
                REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

                //commit to disk
                REQUIRE(Execute(tx, FLAGS::WRITE));
            }


            uint512_t hashTx;
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 2;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx << uint8_t(OP::DEBIT) << hashRegister << hashAccount << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

                //write transaction
                REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

                //commit to disk
                REQUIRE(Execute(tx, FLAGS::WRITE));

                //set hash
                hashTx = tx.GetHash();

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::regDB->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);
                }
            }


            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx << uint8_t(OP::CREDIT) << hashTx << hashGenesis << hashAccount << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

                //commit to disk
                REQUIRE(Execute(tx, FLAGS::WRITE));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::regDB->ReadState(hashAccount, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);
                }

                //rollback
                REQUIRE(Rollback(tx));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::regDB->ReadState(hashAccount, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 0);
                }
            }
        }
    }
}
