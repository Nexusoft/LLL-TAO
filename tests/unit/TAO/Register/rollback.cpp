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

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Register Rollback Tests", "[register]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //test initialize
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

        //check for tx
        REQUIRE(LLD::legDB->WriteTx(tx.GetHash(), tx));

        //generate the prestates and poststates
        REQUIRE(TAO::Operation::Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

        //commit to disk
        REQUIRE(TAO::Operation::Execute(tx, FLAGS::WRITE));

        //check for reserved identifier
        REQUIRE(LLD::regDB->HasIdentifier(11));
    }
}
