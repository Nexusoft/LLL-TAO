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

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>



TEST_CASE( "Transfer Primitive Tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    //test a forced transfer (tokenize)
    {
        //create object
        uint256_t hashToken = LLC::GetRand256();
        uint256_t hashAsset  = LLC::GetRand256();
        uint256_t hashGenesis  = LLC::GetRand256();

        // Create token first
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

        // then create an asset
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object asset = CreateAsset();

            // add some data
            asset << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << std::string("somedata");

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAsset << uint8_t(REGISTER::OBJECT) << asset.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        //now transfer the asset to the token which is a forced transfer
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::TRANSFER) << hashAsset << hashToken << uint8_t(TAO::Operation::TRANSFER::FORCE);

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
                Object asset;
                REQUIRE(LLD::Register->ReadState(hashAsset, asset));

                /* check that the asset owner is the token, indicating that the forced transfer has worked
                   and no claim is required */
                REQUIRE(asset.hashOwner == hashToken);
            }
        }

    }


}
