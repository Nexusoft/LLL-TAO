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
#include <TAO/Register/include/system.h>

#include <TAO/Ledger/types/mempool.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Mempool and memory sequencing tests", "[ledger]" )
{
    using namespace TAO::Register;
    using namespace TAO::Operation;


    //create a list of transactions
    {

        //cleanup
        LLD::regDB->EraseIdentifier(11);

        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        uint512_t hashPrivKey  = LLC::GetRand512();

        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.NextHash(hashPrivKey);

            //create object
            Object token = CreateToken(11, 1000, 100);

            //payload
            tx << uint8_t(OP::REGISTER) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(Execute(tx, FLAGS::PRESTATE | FLAGS::POSTSTATE));

            //sign
            tx.Sign(hashPrivKey);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));
        }
    }
}
