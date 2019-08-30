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

TEST_CASE( "Credit Primitive Tests", "[operation]")
{
    /* check a credit from token */
    {
        /* create object */
        TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);


        TAO::Register::Address hashToken2 = TAO::Register::Address(TAO::Register::Address::TOKEN);
        uint256_t hashAccount2  = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis2  = TAO::Ledger::Genesis(LLC::GetRand256(), true);


        uint512_t hashTx = 0;

        /* create the first token object */
        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence = 0;
            tx.nTimestamp = runtime::timestamp();

            /* create object */
            TAO::Register::Object token = TAO::Register::CreateToken(hashToken, 1000, 100);

            /* payload */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashToken << uint8_t(TAO::Register::REGISTER::OBJECT) << token.GetState();

            /* generate prestates and poststates */
            REQUIRE(tx.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());

            /* commit to disk */
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        /* create the second token object */
        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence = 0;
            tx.nTimestamp = runtime::timestamp();

            /* create object */
            TAO::Register::Object token2 = TAO::Register::CreateToken(hashToken2, 1000, 100);

            /* payload */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashToken2 << uint8_t(TAO::Register::REGISTER::OBJECT) << token2.GetState();

            /* generate prestates and poststates */
            REQUIRE(tx.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());

            /* commit to disk */
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        /* create the first token account object */
        {

            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence = 1;
            tx.nTimestamp = runtime::timestamp();

            /* create object */
            TAO::Register::Object account = TAO::Register::CreateAccount(hashToken);

            /* payload */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashAccount << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

            /* generate prestates and poststates */
            REQUIRE(tx.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());

            /* commit to disk */
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        /* create the second token account object */
        {

            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis2;
            tx.nSequence = 1;
            tx.nTimestamp = runtime::timestamp();

            /* create object */
            TAO::Register::Object account2 = TAO::Register::CreateAccount(hashToken2);

            /* payload */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashAccount2 << uint8_t(TAO::Register::REGISTER::OBJECT) << account2.GetState();

            /* generate prestates and poststates */
            REQUIRE(tx.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());

            /* commit to disk */
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        /* create a token debit transaction. */
        {
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence = 2;
            tx.nTimestamp = runtime::timestamp();
            tx.hashNextTx = TAO::Ledger::STATE::HEAD;

            tx[0] << uint8_t(TAO::Operation::OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            /* generate prestates and poststates */
            REQUIRE(tx.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());

            /* write transaction */
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            /* commit to disk */
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }




    }

    //
}
