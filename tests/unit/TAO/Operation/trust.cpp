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

TEST_CASE( "Trust Operation Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    uint256_t hashTrust    = LLC::GetRand256();
    uint256_t hashGenesis  = LLC::GetRand256();

    uint512_t hashLastTrust;

    //create a trust account register
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create trust register
            Object trustRegister = CreateTrust();

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << trustRegister.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        {
            //check the trust register
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadState(hashTrust, trustAccount));

            //parse
            REQUIRE(trustAccount.Parse());

            //check standards
            REQUIRE(trustAccount.Standard() == OBJECTS::TRUST);
            REQUIRE(trustAccount.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 0);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);

            //add balance to seed remaining tests
            trustAccount.Write("balance", (uint64_t)5000);
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            trustAccount.SetChecksum();
            REQUIRE(LLD::regDB->WriteState(hashTrust, trustAccount));
        }

        {
            //verify update
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadState(hashTrust, trustAccount));

            //parse
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Trust w/o Genesis NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        hashLastTrust = LLC::GetRand512();

        //payload
        tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(555) << uint64_t(6);

        //generate the prestates and poststates (trust w/o genesis should fail)
        REQUIRE(!tx.Build());

        //check register values
        {
            //check trust not indexed
            REQUIRE(!LLD::regDB->HasTrust(hashGenesis));

            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadState(hashTrust, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Stake w/o Genesis NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        hashLastTrust = LLC::GetRand512();

        //payload
        tx[0] << uint8_t(OP::STAKE) << uint64_t(1000);

        //generate the prestates and poststates (trust w/o genesis should fail)
        REQUIRE(!tx.Build());

        //check register values
        {
            //check trust not indexed
            REQUIRE(!LLD::regDB->HasTrust(hashGenesis));

            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadState(hashTrust, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Unstake w/o Genesis NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        hashLastTrust = LLC::GetRand512();

        //payload
        tx[0] << uint8_t(OP::UNSTAKE) << uint64_t(1000) << uint64_t(50);

        //generate the prestates and poststates (trust w/o genesis should fail)
        REQUIRE(!tx.Build());

        //check register values
        {
            //check trust not indexed
            REQUIRE(!LLD::regDB->HasTrust(hashGenesis));

            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadState(hashTrust, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5000);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 0);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Genesis
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload with coinstake reward
        tx[0] << uint8_t(OP::GENESIS) << hashTrust << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //save last trust hash
        hashLastTrust = tx.GetHash();

        //check register values
        {
            //check trust indexed
            REQUIRE(LLD::regDB->HasTrust(hashGenesis));

            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test duplicate Genesis NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload with coinstake reward
        tx[0] << uint8_t(OP::GENESIS) << hashTrust << uint64_t(10);

        //generate the prestates and poststates
        REQUIRE(!tx.Build());

        //check register values
        {
            //check trust indexed
            REQUIRE(LLD::regDB->HasTrust(hashGenesis));

            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 5);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 0);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Trust
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 2;
        tx.nTimestamp  = runtime::timestamp();

        //payload with new trust score and coinstake reward
        tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(720) << uint64_t(6);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //save last trust hash
        hashLastTrust = tx.GetHash();

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 11);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 720);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test second Trust
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 3;
        tx.nTimestamp  = runtime::timestamp();

        //payload with new trust score and coinstake reward
        tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(2000) << uint64_t(7);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //save last trust hash
        hashLastTrust = tx.GetHash();

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 18);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //Test Stake exceeds balance NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 4;
        tx.nTimestamp  = runtime::timestamp();

        //payload with added stake amount
        tx[0] << uint8_t(OP::STAKE) << uint64_t(1000);

        //generate the prestates and poststates
        REQUIRE(!tx.Build());

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 18);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5000);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //Test Stake
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 4;
        tx.nTimestamp  = runtime::timestamp();

        //payload with added stake amount
        tx[0] << uint8_t(OP::STAKE) << uint64_t(15);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 3);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5015);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //Test Unstake exceeds stake NOTE: intended failure
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 5;
        tx.nTimestamp  = runtime::timestamp();

        //payload with removed stake amount and trust penalty
        tx[0] << uint8_t(OP::UNSTAKE) << uint64_t(15000) << uint64_t(2320);

        //generate the prestates and poststates
        REQUIRE(!tx.Build());

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 3);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 2000);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 5015);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //Test Unstake
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 5;
        tx.nTimestamp  = runtime::timestamp();

        //payload with removed stake amount and trust penalty
        tx[0] << uint8_t(OP::UNSTAKE) << uint64_t(2000) << uint64_t(800);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 2003);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 1200);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 3015);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }


    //test Trust after Stake/Unstake
    {
        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 6;
        tx.nTimestamp  = runtime::timestamp();

        //payload with new trust score and coinstake reward
        tx[0] << uint8_t(OP::TRUST) << hashLastTrust << uint64_t(2200) << uint64_t(5);

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //save last trust hash
        hashLastTrust = tx.GetHash();

        //check register values
        {
            TAO::Register::Object trustAccount;
            REQUIRE(LLD::regDB->ReadTrust(hashGenesis, trustAccount));

            //parse register
            REQUIRE(trustAccount.Parse());

            //check values
            REQUIRE(trustAccount.get<uint64_t>("balance") == 2008);
            REQUIRE(trustAccount.get<uint64_t>("trust")   == 2200);
            REQUIRE(trustAccount.get<uint64_t>("stake")   == 3015);
            REQUIRE(trustAccount.get<uint256_t>("token")  == 0);
        }
    }
}
