/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <Legacy/include/create.h>

#include <Legacy/types/coinbase.h>

#include <Legacy/include/enum.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/genesis.h>

#include <unit/catch2/catch.hpp>

TEST_CASE("UTXO Unit Tests", "[UTXO]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    {
        //reserve key from temp wallet
        Legacy::ReserveKey* pReserveKey = new Legacy::ReserveKey(&Legacy::Wallet::GetInstance());

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pReserveKey, coinbase, 1, 0, 7, tx));

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //get best
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();

        //conect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //add to wallet
        Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true);

        //check balance
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == 1000000);
    }


    //create a tritium transaction
    TAO::Ledger::BlockState state;
    state.nHeight = 150;

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

    //create a trust register from inputs spent on coinbase
    {
        uint256_t hashAccount    = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        uint256_t hashGenesis  = TAO::Ledger::Genesis(LLC::GetRand256(), true);

        uint512_t hashCoinbaseTx = 0;
        uint512_t hashLastTrust = LLC::GetRand512();
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx  = TAO::Ledger::STATE::HEAD;

            //payload (hashGenesis, coinbase reward, extra nonce)
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis << uint64_t(5000) << (uint64_t)0;

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

            //set the hash
            hashCoinbaseTx = tx.GetHash();
        }


        //Create a account register
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object account = CreateAccount(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check trust account initial values
            {
                Object trust;
                REQUIRE(LLD::Register->ReadState(hashAccount, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check register
                REQUIRE(trust.get<uint64_t>("balance") == 0);
                REQUIRE(trust.get<uint256_t>("token")  == 0);
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
            tx[0] << uint8_t(OP::CREDIT) << hashCoinbaseTx << uint32_t(0) << hashAccount << hashGenesis << uint64_t(5000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::Register->ReadState(hashAccount, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(trust.get<uint64_t>("balance") == 5000);
                REQUIRE(trust.get<uint256_t>("token") == 0);
            }
        }


        //test failures of legacy OP
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(1000);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //verify the prestates and poststates
            REQUIRE_FALSE(tx.Verify());
        }


        //test success for legacy opeation
        uint512_t hashTx = 0;
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();
            tx.hashNextTx = 1;

            //payload
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(1000);

            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //legacy payload
            Legacy::Script script;
            script.SetNexusAddress(address);

            //add to tx
            tx[0] << script;

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //write to disk
            hashTx = tx.GetHash();
            REQUIRE(LLD::Ledger->WriteTx(hashTx, tx));

            //check register values
            {
                Object account;
                REQUIRE(LLD::Register->ReadState(hashAccount, account));

                //parse register
                REQUIRE(account.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(account.get<uint64_t>("balance") == 4000);
            }
        }

        //try to spend an OP::LEGACY
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << uint32_t(0) << hashAccount << hashAccount << uint64_t(1000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check for error
            std::string error = debug::GetLastError();
            REQUIRE(error.find("tx claim is not a debit") != std::string::npos);

            //check register values
            {
                Object account;
                REQUIRE(LLD::Register->ReadState(hashAccount, account));

                //parse register
                REQUIRE(account.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(account.get<uint64_t>("balance") == 4000);
                REQUIRE(account.get<uint256_t>("token") == 0);
            }
        }


        {
            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //add the data into input script
            Legacy::TxIn in;
            in.prevout.hash = hashTx;
            in.prevout.n    = 0;

            Legacy::Transaction tx;
            tx.vin.push_back(in);

            Legacy::TxOut out;
            out.scriptPubKey.SetNexusAddress(address);

            //get inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE(tx.FetchInputs(inputs));

            //conect tx
            //REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));
        }
    }
}
