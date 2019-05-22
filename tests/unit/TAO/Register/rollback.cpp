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

#include <unit/catch2/catch.hpp>

TEST_CASE( "Register Rollback Tests", "[register]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //create dummy block
    uint1024_t hashBlock = 0;
    {
        TAO::Ledger::BlockState state;
        state.nVersion       = 7;
        state.hashPrevBlock  = 0;
        state.nChannel       = 1;
        state.nHeight        = 5;
        state.hashMerkleRoot = 555;
        state.nBits          = 333;
        state.nNonce         = 222;
        state.nTime          = 999;

        //set hash
        hashBlock = state.GetHash();

        //write to disk
        LLD::legDB->WriteBlock(hashBlock, state);
    }


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

        //write index
        REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

        //commit to disk
        REQUIRE(Execute(tx, FLAGS::WRITE));

        //check that register exists
        State state;
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

        //check owner
        REQUIRE(state.hashOwner == 0);

        //rollback the transaction
        REQUIRE(Rollback(tx));

        //grab the new state
        REQUIRE(LLD::regDB->ReadState(hashRegister, state));

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

        uint512_t hashTx;
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::TRANSFER) << hashRegister << hashGenesis2;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //get claim hash
            hashTx = tx.GetHash();

            //check that register exists
            State state;
            REQUIRE(LLD::regDB->ReadState(hashRegister, state));

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
            tx << uint8_t(OP::CLAIM) << hashTx;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            //check proof is active
            REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));

            //rollback the transaction
            REQUIRE(Rollback(tx));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == 0);

                //check for the proof
                REQUIRE(!LLD::legDB->HasProof(hashRegister, hashTx));
            }

            {
                //check for event
                uint32_t nSequence;
                REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

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
            tx << uint8_t(OP::CLAIM) << hashTx;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis);
            }

            //check proof is active
            REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));

            /*
            {
                //check event is discarded
                uint32_t nSequence;
                REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(!LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));
            }
            */

            //rollback the transaction
            REQUIRE(Rollback(tx));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == 0);

                //check for the proof
                REQUIRE(!LLD::legDB->HasProof(hashRegister, hashTx));
            }

            {
                //check event is back
                uint32_t nSequence;
                REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

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

            //payload
            tx << uint8_t(OP::CLAIM) << hashTx;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check that register exists
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            //check proof is active
            REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));

            //grab the new state
            {
                State state;
                REQUIRE(LLD::regDB->ReadState(hashRegister, state));

                //check owner
                REQUIRE(state.hashOwner == hashGenesis2);
            }

            {
                //check for event
                uint32_t nSequence;
                REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                //check for tx
                TAO::Ledger::Transaction txEvent;
                REQUIRE(LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

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

            //write index
            REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

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
            uint256_t hashGenesis2 = LLC::GetRand256();

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
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 0;
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
                tx.nSequence   = 1;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx << uint8_t(OP::DEBIT) << hashRegister << hashAccount << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

                //write transaction
                REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

                //write index
                REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

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
                tx.hashGenesis = hashGenesis2;
                tx.nSequence   = 2;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx << uint8_t(OP::CREDIT) << hashTx << hashRegister << hashAccount << uint64_t(500);

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

                    //check that proofs are established
                    REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));
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

                    //check that proofs are removed
                    REQUIRE(!LLD::legDB->HasProof(hashRegister, hashTx));
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
                tx << uint8_t(OP::CREDIT) << hashTx << hashRegister << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

                //commit to disk
                REQUIRE(Execute(tx, FLAGS::WRITE));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::regDB->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 1000);

                    //check that proofs are removed
                    REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));

                    /*
                    {
                        //check event is discarded
                        uint32_t nSequence;
                        REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                        //check for tx
                        TAO::Ledger::Transaction txEvent;
                        REQUIRE(!LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));
                    }
                    */
                }

                //rollback
                REQUIRE(Rollback(tx));

                //check register values
                {
                    Object account;
                    REQUIRE(LLD::regDB->ReadState(hashRegister, account));

                    //parse register
                    REQUIRE(account.Parse());

                    //check balance
                    REQUIRE(account.get<uint64_t>("balance") == 500);

                    //check that proofs are removed
                    REQUIRE(!LLD::legDB->HasProof(hashRegister, hashTx));

                    {
                        //check event is discarded
                        uint32_t nSequence;
                        REQUIRE(LLD::legDB->ReadSequence(hashGenesis2, nSequence));

                        //check for tx
                        TAO::Ledger::Transaction txEvent;
                        REQUIRE(LLD::legDB->ReadEvent(hashGenesis2, nSequence - 1, txEvent));

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
                tx << uint8_t(OP::CREDIT) << hashTx << hashRegister << hashAccount << uint64_t(500);

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

                    //check that proofs are established
                    REQUIRE(LLD::legDB->HasProof(hashRegister, hashTx));
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
                tx << uint8_t(OP::CREDIT) << hashTx << hashRegister << hashAccount << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));
            }


            //attempt to double spend back to self
            {
                //create the transaction object
                TAO::Ledger::Transaction tx;
                tx.hashGenesis = hashGenesis;
                tx.nSequence   = 3;
                tx.nTimestamp  = runtime::timestamp();

                //payload
                tx << uint8_t(OP::CREDIT) << hashTx << hashRegister << hashRegister << uint64_t(500);

                //generate the prestates and poststates
                REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));
            }
        }
    }


    //create a trust register from inputs spent on coinbase
    {
        //create object
        //uint256_t hashRegister = LLC::GetRand256();

        uint256_t hashTrust    = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        uint512_t hashTx;
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::COINBASE) << uint64_t(5000);

            //write transaction
            REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::legDB->IndexBlock(tx.GetHash(), hashBlock));

            //set the hash
            hashTx = tx.GetHash();
        }


        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object trust = CreateTrust();

            //payload
            tx << uint8_t(OP::REGISTER) << hashTrust << uint8_t(REGISTER::OBJECT) << trust.GetState();

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
            tx << uint8_t(OP::CREDIT) << hashTx << hashGenesis << hashTrust << uint64_t(5000);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 5000);
            }
        }


        //handle an OP::GENESIS
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::GENESIS) << hashTrust;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 0);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 5000);

                //check for trust index
                REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trust));
            }


            //rollback the genesis
            REQUIRE(Rollback(tx));


            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 5000);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 0);

                //check for trust index
                REQUIRE(!LLD::regDB->HasTrust(hashGenesis));
            }
        }


        //handle an OP::TRUST NOTE: intended failure
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::TRUST) << hashTx << uint64_t(555);

            //generate the prestates and poststates
            REQUIRE(!Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 5000);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 0);

                //check for trust index
                REQUIRE(!LLD::regDB->HasTrust(hashGenesis));
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
            tx << uint8_t(OP::GENESIS) << hashTrust;

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //set trust hash
            hashTx = tx.GetHash();

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadState(hashTrust, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 0);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 5000);

                //check for trust index
                REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trust));
            }
        }


        //handle an OP::TRUST
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx << uint8_t(OP::TRUST) << hashTx << uint64_t(555);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 0);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 5000);

                //check trust
                REQUIRE(trust.get<uint64_t>("trust") == 555);
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
            tx << uint8_t(OP::TRUST) << hashTx << uint64_t(333);

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 0);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 5000);

                //check trust
                REQUIRE(trust.get<uint64_t>("trust") == 333);
            }

            //rollback
            Rollback(tx);

            //check register values
            {
                Object trust;
                REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance
                REQUIRE(trust.get<uint64_t>("balance") == 0);

                //check balance
                REQUIRE(trust.get<uint64_t>("stake") == 5000);

                //check trust
                REQUIRE(trust.get<uint64_t>("trust") == 555);
            }
        }
    }
}
