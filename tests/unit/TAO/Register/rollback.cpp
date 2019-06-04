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

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/enum.h>

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

        //create object
        uint256_t hashRegister = LLC::GetRand256();
        Object account = CreateToken(hashRegister, 1000, 100);

        //payload
        tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << account.GetState();

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check that register exists
        REQUIRE(LLD::Register->HasState(hashRegister));

        //rollback the transaction
        REQUIRE(Rollback(tx[0]));

        //make sure object is deleted
        REQUIRE(!LLD::Register->HasState(hashRegister));
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
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::RAW) << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

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
            tx[0] << uint8_t(OP::WRITE) << hashRegister << std::vector<uint8_t>(10, 0x1f);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(10, 0x1f));

            //rollback the transaction
            REQUIRE(Rollback(tx[0]));

            //grab the new state
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

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
            tx[0] << uint8_t(OP::APPEND) << hashRegister << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

            //check data
            REQUIRE(state.vchState == std::vector<uint8_t>(20, 0xff));

            //rollback the transaction
            REQUIRE(Rollback(tx[0]));

            //grab the new state
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

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
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::RAW) << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

            //check owner
            REQUIRE(state.hashOwner == hashGenesis);

        }

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();
        tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

        //payload
        tx[0] << uint8_t(OP::TRANSFER) << hashRegister << uint256_t(0xffff);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //write transaction
        REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check that register exists
        State state;
        REQUIRE(LLD::Register->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == 0);

        //rollback the transaction
        REQUIRE(Rollback(tx[0]));

        //grab the new state
        REQUIRE(LLD::Register->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == hashGenesis);
    }



    //rollback a claim
    {
        //create object
        uint256_t hashRegister = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();
        uint256_t hashGenesis2 = LLC::GetRand256();

        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::RAW) << std::vector<uint8_t>(10, 0xff);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

            //check owner
            REQUIRE(state.hashOwner == hashGenesis);

        }

        uint512_t hashTx;
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(OP::TRANSFER) << hashRegister << hashGenesis2;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //get claim hash
            hashTx = tx.GetHash();

            //check that register exists
            State state;
            REQUIRE(LLD::Register->ReadState(hashRegister, state));

            //check owner
            REQUIRE(state.hashOwner == 0);

        }


        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CLAIM) << hashTx << uint32_t(0) << hashRegister;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            //check proof is active
            REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));

            //rollback the transaction
            REQUIRE(Rollback(tx[0]));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == 0);

                //check for the proof
                REQUIRE(!LLD::Ledger->HasProof(hashRegister, hashTx, 0));
            }

            {
                //check for event
                uint32_t nSequence;
                REQUIRE(LLD::Ledger->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::Ledger->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

                //check the hashes match
                REQUIRE(txEvent.GetHash() == hashTx);
            }
        }


        //claim back to self and rollback
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CLAIM) << hashTx << uint32_t(0) << hashRegister;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis);
            }

            //check proof is active
            REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));

            /*
            {
                //check event is discarded
                uint32_t nSequence;
                REQUIRE(LLD::Ledger->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(!LLD::Ledger->ReadEvent(hashGenesis2, nSequence - 1, txEvent));
            }
            */

            //rollback the transaction
            REQUIRE(Rollback(tx[0]));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == 0);

                //check for the proof
                REQUIRE(!LLD::Ledger->HasProof(hashRegister, hashTx, 0));
            }

            {
                //check event is back
                uint32_t nSequence;
                REQUIRE(LLD::Ledger->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::Ledger->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

                //check the hashes match
                REQUIRE(txEvent.GetHash() == hashTx);
            }
        }


        //simulate original user finally getting their claim
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(OP::CLAIM) << hashTx << uint32_t(0) << hashRegister;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            //check proof is active
            REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::Register->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            {
                //check for event
                uint32_t nSequence;
                REQUIRE(LLD::Ledger->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::Ledger->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

                //check the hashes match
                REQUIRE(txEvent.GetHash() == hashTx);
            }
        }
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
            Object token = CreateToken(hashRegister, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(hashRegister);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashRegister << hashAccount << uint64_t(500);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object token;
                REQUIRE(LLD::Register->ReadState(hashRegister, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 500);
            }

            //rollback
            REQUIRE(Rollback(tx[0]));

            //check register values
            {
                Object token;
                REQUIRE(LLD::Register->ReadState(hashRegister, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 1000);
            }
        }


        //rollback a credit from token
        {
            //create object
            uint256_t hashRegister = LLC::GetRand256();
            uint256_t hashAccount  = LLC::GetRand256();
            uint256_t hashGenesis  = LLC::GetRand256();
            uint256_t hashGenesis2 = LLC::GetRand256();

            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 0;
                tx.nTimestamp  = runtime::timestamp();

                //create object
                Object token = CreateToken(hashRegister, 1000, 100);

                //payload
                tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << token.GetState();

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            }


            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 0;
                tx.nTimestamp  = runtime::timestamp();

                //create object
                Object account = CreateAccount(hashRegister);

                //payload
                tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            }


            uint512_t hashTx;
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 1;
                tx.nTimestamp  = runtime::timestamp();
                tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

                //payload
                tx[0] << uint8_t(OP::DEBIT) << hashRegister << hashAccount << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //write transaction
                REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

                //set hash
                hashTx = tx.GetHash();

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);
                }
            }


            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 2;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashAccount, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);

                    //check that proofs are established
                    REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));
                }

                //rollback
                REQUIRE(Rollback(tx[0]));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashAccount, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 0);

                    //check that proofs are removed
                    REQUIRE(!LLD::Ledger->HasProof(hashRegister, hashTx, 0));
                }
            }


            //attempt to to send back to self
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashRegister << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 1000);

                    //check that proofs are removed
                    REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));
                }

                //rollback
                REQUIRE(Rollback(tx[0]));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);

                    //check that proofs are removed
                    REQUIRE(!LLD::Ledger->HasProof(hashRegister, hashTx, 0));

                    {
                        //check event is discarded
                        uint32_t nSequence;
                        REQUIRE(LLD::Ledger->ReadSequence(hashGenesis2, nSequence));

                        //check for tx
                        TAO::Ledger::Transaction txEvent;
                        REQUIRE(LLD::Ledger->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

                        //make sure hashes match
                        REQUIRE(txEvent.GetHash() == hashTx);
                    }
                }
            }


            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //commit to disk
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::Register->ReadState(hashAccount, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);

                    //check that proofs are established
                    REQUIRE(LLD::Ledger->HasProof(hashRegister, hashTx, 0));
                }
            }


            //attempt to double spend
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //make sure it fails
                REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
            }


            //attempt to double spend back to self
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashRegister << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(tx.Build());

                //make sure it fails
                REQUIRE(!Execute(tx[0], TAO::Ledger::FLAGS::MEMPOOL));
            }
        }
    }


    //create a trust register from inputs spent on coinbase
    {
        uint256_t hashTrust    = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        uint512_t hashCoinbaseTx;
        uint512_t hashLastTrust;
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis << uint64_t(5000);

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //set the hash
            hashCoinbaseTx = tx.GetHash();
        }


        //Create a trust register
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object trustRegister = CreateTrust();

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << trustRegister.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check trust account initial values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadState(hashTrust, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 0);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
                REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
            }
        }


        //Add balance to trust account by crediting from Coinbase tx
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashCoinbaseTx << uint32_t(0) << hashTrust << hashGenesis << uint64_t(5000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadState(hashTrust, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            }
        }


        //OP::GENESIS rollback
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.nTimestamp  = runtime::timestamp();

            //payload with coinstake reward
            tx[0] << uint8_t(OP::GENESIS) << hashTrust << uint64_t(5);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                //check added to trust index
                REQUIRE(LLD::Register->HasTrust(hashGenesis));

                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 5);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }

            //rollback the genesis
            REQUIRE(Rollback(tx[0]));

            //check register values
            {
                //check removed from trust index
                REQUIRE(!LLD::Register->HasTrust(hashGenesis));

                Object trustAccount;
                REQUIRE(LLD::Register->ReadState(hashTrust, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            }
        }


        //handle an OP::GENESIS
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::GENESIS) << hashTrust << uint64_t(6);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //set trust hash
            hashLastTrust = tx.GetHash();

            //check register values
            {
                //check added to trust index
                REQUIRE(LLD::Register->HasTrust(hashGenesis));

                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 6);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }
        }


        //OP::TRUST rollback
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(555) << uint64_t(7);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 13);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 555);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }

            //rollback
            Rollback(tx[0]);

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 6);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }
        }


        //handle an OP::TRUST
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(2000) << uint64_t(10);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            hashLastTrust = tx.GetHash();

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 16);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }
        }


        //OP::STAKE rollback
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.nTimestamp  = runtime::timestamp();

            //payload with coinstake reward
            tx[0] << uint8_t(OP::STAKE) << uint64_t(15);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 1);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5015);
            }

            //rollback the stake
            REQUIRE(Rollback(tx[0]));

            //verify rollback
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 16);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }
        }


        //OP::UNSTAKE rollback
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.nTimestamp  = runtime::timestamp();

            //payload with removed stake amount and trust penalty
            tx[0] << uint8_t(OP::UNSTAKE) << uint64_t(2000) << uint64_t(800);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 2016);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 1200);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 3000);
            }

            //rollback
            Rollback(tx[0]);

            //check register values
            {
                Object trustAccount;
                REQUIRE(LLD::Register->ReadTrust(hashGenesis, trustAccount));

                //parse register
                REQUIRE(trustAccount.Parse());

                //check register
                REQUIRE(trustAccount.get<uint64_t>("balance") == 16);
                REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
                REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            }
        }
    }
}
