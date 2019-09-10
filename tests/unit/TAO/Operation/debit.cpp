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

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/genesis.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Debit Primitive Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //check a debit from token
    {

        //create object
        TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object token;
                REQUIRE(LLD::Register->ReadState(hashToken, token));

                //parse register
                REQUIRE(token.Parse());

                //check balance
                REQUIRE(token.get<uint64_t>("balance") == 500);
            }
        }
    }


    //check a debit from token
    {

        //create object
        TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(1000) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object token;
                REQUIRE(LLD::Register->ReadState(hashToken, token));

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
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(1) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //verify the prestates and poststates
            REQUIRE_FALSE(tx.Verify());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("account doesn't have sufficient balance") != std::string::npos);
        }
    }


    //check for failed by account balance
    TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
    {

        //create object

        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object token = CreateToken(hashToken, 100, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("account doesn't have sufficient balance") != std::string::npos);
        }
    }


    //check for failed by reserved
    {

        //create object
        uint256_t hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << uint256_t(0) << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit with reserved address") != std::string::npos);
        }
    }


    //check for failed by reserved
    {

        //create object
        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashAccount << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit to the same address as from") != std::string::npos);
        }
    }


    //check for failed by incorrect base
    {

        //create object
        TAO::Register::Address hashToken  = TAO::Register::Address(TAO::Register::Address::RAW);
        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

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
                    << std::string("token") << uint8_t(TYPES::UINT256_T) << uint256_t(11)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T) << uint8_t(100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::RAW) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("failed to parse account object register") != std::string::npos);
        }
    }


    //check for failed by incorrect base
    {

        //create object
        TAO::Register::Address hashToken  = TAO::Register::Address(TAO::Register::Address::OBJECT);
        TAO::Register::Address hashAccount  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

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
                    << std::string("token")      << uint8_t(TYPES::UINT256_T) << uint256_t(11)
                    << std::string("supply")     << uint8_t(TYPES::UINT64_T) << uint64_t(1000)
                    << std::string("decimals")     << uint8_t(TYPES::UINT8_T) << uint8_t(100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

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
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("cannot debit from non-standard object register") != std::string::npos);
        }
    }
}
