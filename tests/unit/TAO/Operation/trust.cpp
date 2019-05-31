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

TEST_CASE( "Trust Primitive Tests", "[operation]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //create a trust object register
    {
        uint256_t hashAddress = LLC::GetRand256();
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = LLC::GetRand256();
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object object = CreateTrust();

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //commit to disk
            REQUIRE(Execute(tx, FLAGS::WRITE));
        }

        {
            //check values all match
            TAO::Register::Object object;
            REQUIRE(LLD::regDB->ReadState(hashAddress, object));

            //parse
            REQUIRE(object.Parse());

            //check standards
            REQUIRE(object.Standard() == OBJECTS::TRUST);
            REQUIRE(object.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(object.get<uint64_t>("balance") == 0);
            REQUIRE(object.get<uint64_t>("trust")   == 0);
            REQUIRE(object.get<uint256_t>("identifier") == 0);
        }
    }
}
