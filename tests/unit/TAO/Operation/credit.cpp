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
#include <TAO/Ledger/include/chainstate.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Credit Primitive Tests", "[operation]")
{
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
            REQUIRE(TAO::Operation::Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
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
            REQUIRE(TAO::Operation::Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
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
            REQUIRE(TAO::Operation::Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }

        TAO::Ledger::Transaction tx;
        TAO::Ledger::Transaction tx2;

        uint64_t nReference = 0;
        uint64_t nAmount = 0;


        /* check a token credit from a debit with an improper credit amount. */
        {
            tx.hashGenesis = hashGenesis;
            tx.nSequence = 2;
            tx.nTimestamp = runtime::timestamp();

            tx2.hashGenesis = hashGenesis;
            tx2.nSequence = 3;
            tx2.nTimestamp = runtime::timestamp();

            nReference = 0;
            nAmount = 500;

            /* create a token debit transaction. */
            tx[0] << uint8_t(TAO::Operation::OP::DEBIT) << hashToken << hashAccount << nAmount << nReference;
            REQUIRE(tx.Build());

            /* create a token credit transaction. */
            tx2[0] << uint8_t(TAO::Operation::OP::CREDIT) << tx.GetHash() << uint32_t(0) << hashAccount << hashToken << nAmount + 1;
            REQUIRE(tx2.Build());

            /* verify prestates and poststates */
            REQUIRE(tx.Verify());
            REQUIRE(tx2.Verify());

            /* write transaction */
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));
            REQUIRE(LLD::Ledger->WriteTx(tx2.GetHash(), tx2));
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::Genesis()));

            /* commit to disk */
            REQUIRE(TAO::Operation::Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            REQUIRE_FALSE(TAO::Operation::Execute(tx2[0], TAO::Ledger::FLAGS::BLOCK));

            tx2[0].Clear();
        }


        /* check a token credit return back to a token register. */
        {
            tx2.hashGenesis = hashGenesis;
            tx2.nSequence = 3;
            tx2.nTimestamp = runtime::timestamp();

            nAmount = 500;

            /* create a token credit transaction. */
            tx2[0] << uint8_t(TAO::Operation::OP::CREDIT) << tx.GetHash() << uint32_t(0) << hashToken << hashToken << nAmount;
            REQUIRE(tx2.Build());

            /* verify prestates and poststates, write tx, commit to disk */
            REQUIRE(tx2.Verify());
            REQUIRE(LLD::Ledger->WriteTx(tx2.GetHash(), tx2));
            REQUIRE(TAO::Operation::Execute(tx2[0], TAO::Ledger::FLAGS::BLOCK));

            /* cleanup */
            tx[0].Clear();
            tx2[0].Clear();
        }


        /* check a token credit from a debit with proper amount. */
        {
            tx.hashGenesis = hashGenesis;
            tx.nSequence = 4;
            tx.nTimestamp = runtime::timestamp();

            tx2.hashGenesis = hashGenesis;
            tx2.nSequence = 5;
            tx2.nTimestamp = runtime::timestamp();

            nReference = 0;
            nAmount = 500;

            /* create a token debit transaction. */
            tx[0] << uint8_t(TAO::Operation::OP::DEBIT) << hashToken << hashAccount << nAmount << nReference;
            REQUIRE(tx.Build());

            /* create a token credit transaction. */
            tx2[0] << uint8_t(TAO::Operation::OP::CREDIT) << tx.GetHash() << uint32_t(0) << hashAccount << hashToken << nAmount;
            REQUIRE(tx2.Build());

            /* verify prestates and poststates, write tx, commit to disk */
            REQUIRE(tx.Verify());
            REQUIRE(tx2.Verify());

            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));
            REQUIRE(LLD::Ledger->WriteTx(tx2.GetHash(), tx2));
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::Genesis()));

            REQUIRE(TAO::Operation::Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
            REQUIRE(TAO::Operation::Execute(tx2[0], TAO::Ledger::FLAGS::BLOCK));

            /* cleanup */
            //tx[0].Clear();
            tx2[0].Clear();
        }


        /* attempt a spent token credit return back to a token register. */
        {
            tx2.hashGenesis = hashGenesis;
            tx2.nSequence = 5;
            tx2.nTimestamp = runtime::timestamp();

            nAmount = 500;

            /* create a token credit transaction. */
            tx2[0] << uint8_t(TAO::Operation::OP::CREDIT) << tx.GetHash() << uint32_t(0) << hashToken << hashToken << nAmount;
            REQUIRE(tx2.Build());

            /* verify prestates and poststates, write tx, commit to disk */
            REQUIRE(tx2.Verify());
            REQUIRE(LLD::Ledger->WriteTx(tx2.GetHash(), tx2));
            REQUIRE_FALSE(TAO::Operation::Execute(tx2[0], TAO::Ledger::FLAGS::BLOCK));

            /* cleanup */
            tx2[0].Clear();
        }


        /* attempt a double credit. */
        {
            tx2.hashGenesis = hashGenesis;
            tx2.nSequence = 5;
            tx2.nTimestamp = runtime::timestamp();

            /* create a token credit transaction. */
            tx2[0] << uint8_t(TAO::Operation::OP::CREDIT) << tx.GetHash() << uint32_t(0) << hashAccount << hashToken << nAmount;
            REQUIRE(tx2.Build());

            /* verify prestates and poststates, write tx, commit to disk */
            REQUIRE(tx2.Verify());
            REQUIRE(LLD::Ledger->WriteTx(tx2.GetHash(), tx2));
            REQUIRE_FALSE(TAO::Operation::Execute(tx2[0], TAO::Ledger::FLAGS::BLOCK));

            /* cleanup */
            tx[0].Clear();
            tx2[0].Clear();
        }

    }

}
