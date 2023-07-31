/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLD/include/global.h>

#include <Legacy/include/create.h>

#include <Legacy/types/coinbase.h>
#include <Legacy/include/signature.h>

#include <Legacy/include/enum.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/constants.h>
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
        Legacy::ReserveKey* pReserveKey = new Legacy::ReserveKey(&Legacy::Wallet::Instance());

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pReserveKey, coinbase, 1, 0, 7, tx));
        tx.vout[0].nValue = 1000000; //1 NXS

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, TAO::Ledger::ChainState::tStateGenesis, TAO::Ledger::FLAGS::BLOCK));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::tStateGenesis.GetHash()));

        //add to wallet
        REQUIRE(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::tStateGenesis, true));

        //check balance
        REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1000000);
    }


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

            //payload (hashGenesis, coinbase reward, extra nonce)
            tx[0] << uint8_t(OP::COINBASE) << hashGenesis << uint64_t(1000000) << (uint64_t)0;

            //write transaction
            REQUIRE(LLD::Ledger->WriteTx(tx.GetHash(), tx));

            //write index
            REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), TAO::Ledger::ChainState::tStateGenesis.GetHash()));

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
            tx[0] << uint8_t(OP::CREDIT) << hashCoinbaseTx << uint32_t(0) << hashAccount << hashGenesis << uint64_t(1000000);

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
                REQUIRE(trust.get<uint64_t>("balance") == 1000000);
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
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(900000);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());

            //verify the prestates and poststates
            REQUIRE_FALSE(tx.Verify());

            //add to wallet
            REQUIRE_THROWS(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::tStateGenesis, true));
        }


        //test success for legacy operation
        uint512_t hashTx = 0;
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::LEGACY) << hashAccount << uint64_t(900000);

            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::Instance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //legacy payload
            Legacy::Script script;
            script.SetNexusAddress(address);

            //add to tx
            tx[0] << script;

            //add extra junk contracts
            Object account = CreateAccount(0);
            tx[1] << uint8_t(OP::CREATE) << hashAccount + 1 << uint8_t(REGISTER::OBJECT) << account.GetState();
            tx[2] << uint8_t(OP::CREATE) << hashAccount + 2 << uint8_t(REGISTER::OBJECT) << account.GetState();
            tx[3] << uint8_t(OP::CREATE) << hashAccount + 3 << uint8_t(REGISTER::OBJECT) << account.GetState();


            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //write to disk
            hashTx = tx.GetHash();

            REQUIRE(LLD::Ledger->WriteTx(hashTx, tx));

            //index block
            REQUIRE(LLD::Ledger->IndexBlock(hashTx, TAO::Ledger::ChainState::tStateGenesis.GetHash()));

            //add to wallet
            REQUIRE(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::tStateGenesis, true));

            //check register values
            {
                Object account;
                REQUIRE(LLD::Register->ReadState(hashAccount, account));

                //parse register
                REQUIRE(account.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(account.get<uint64_t>("balance") == 100000);
            }

            //check wallet balance
            REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1900000);
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
                REQUIRE(account.get<uint64_t>("balance") == 100000);
                REQUIRE(account.get<uint256_t>("token") == 0);
            }
        }


        {
            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::Instance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //create a transaction
            Legacy::Script scriptPubKey;
            scriptPubKey.SetNexusAddress(address);

            //create sending vectors
            std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
            vecSend.push_back(make_pair(scriptPubKey, 1500000));

            //create the transaction
            Legacy::WalletTx wtx;

            //for change
            Legacy::ReserveKey changeKey(Legacy::Wallet::Instance());

            //create transaction
            int64_t nFees;
            REQUIRE(Legacy::Wallet::Instance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

            //get best
            TAO::Ledger::BlockState state = TAO::Ledger::ChainState::tStateBest.load();
            state.hashNextBlock = LLC::GetRand1024();

            REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

            //get inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE(wtx.FetchInputs(inputs));

            //write to disk
            REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

            //connect tx
            REQUIRE(wtx.Connect(inputs, state, TAO::Ledger::FLAGS::BLOCK));

            REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

            //add to wallet
            REQUIRE(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(wtx, state, true));

            //check wallet balance
            REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1890000);
        }

        //set best
        TAO::Ledger::BlockState state = TAO::Ledger::ChainState::tStateBest.load();
        ++state.nHeight;
        state.hashNextBlock = LLC::GetRand1024();

        TAO::Ledger::ChainState::tStateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        {
            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::Instance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //create a transaction
            Legacy::Script scriptPubKey;
            scriptPubKey.SetNexusAddress(address);

            //create sending vectors
            std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
            vecSend.push_back(make_pair(scriptPubKey, 1500000));

            //create the transaction
            Legacy::WalletTx wtx;

            //for change
            Legacy::ReserveKey changeKey(Legacy::Wallet::Instance());

            //create transaction
            int64_t nFees;
            REQUIRE(Legacy::Wallet::Instance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

            //get inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE(wtx.FetchInputs(inputs));

            //write to disk
            REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

            //connect tx
            REQUIRE(wtx.Connect(inputs, state, TAO::Ledger::FLAGS::BLOCK));

            //index to block
            REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

            //add to wallet
            REQUIRE(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(wtx, state, true));

            //check wallet balance
            REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1880000);
        }


        //set best
        ++state.nHeight;
        state.hashNextBlock = LLC::GetRand1024();

        TAO::Ledger::ChainState::tStateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        hashTx = 0;
        {
            //create a transaction
            Legacy::Script scriptPubKey;
            scriptPubKey.SetRegisterAddress(hashAccount);

            //create sending vectors
            std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
            vecSend.push_back(make_pair(scriptPubKey, 20000));

            //create the transaction
            Legacy::WalletTx wtx;

            //for change
            Legacy::ReserveKey changeKey(Legacy::Wallet::Instance());

            //create transaction
            int64_t nFees;
            REQUIRE(Legacy::Wallet::Instance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

            //get inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE(wtx.FetchInputs(inputs));

            //write to disk
            REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

            //connect tx
            REQUIRE(wtx.Connect(inputs, state, TAO::Ledger::FLAGS::BLOCK));

            //index to block
            REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

            //add to wallet
            REQUIRE(Legacy::Wallet::Instance().AddToWalletIfInvolvingMe(wtx, state, true));

            //check wallet balance
            REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1850000);

            //set the hash to spend
            hashTx = wtx.GetHash();
        }


        //set best
        ++state.nHeight;
        state.hashNextBlock = LLC::GetRand1024();

        TAO::Ledger::ChainState::tStateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

/* @Colin this test seems to fail almost all of the time for me.  It fails on REQUIRE_FALSE(wtx.Connect...
   If I step through the code to figure out why it fails, it very often works.  This suggests a race somewhere, but I don't
   know the legacy code well enough to debug it any further
        //try to spend the legacy to tritium transaction INTENDED FAILURE
        {
            //legacy get key
            std::vector<uint8_t> vKey;
            REQUIRE(Legacy::Wallet::Instance().GetKeyPool().GetKeyFromPool(vKey, false));
            Legacy::NexusAddress address(vKey);

            //create a transaction
            Legacy::Script scriptPubKey;
            scriptPubKey.SetNexusAddress(address);

            //create the transaction
            Legacy::WalletTx wtx;

            uint32_t nOut = 0;
            {
                Legacy::Transaction tx;
                REQUIRE(LLD::Legacy->ReadTx(hashTx, tx));

                for(; nOut < tx.vout.size(); ++nOut)
                    if(tx.vout[nOut].nValue == 20000)
                        break;

                wtx.vin.push_back(Legacy::TxIn(hashTx, nOut));
                REQUIRE_FALSE(Legacy::SignSignature(Legacy::Wallet::Instance(), tx, wtx, 0));
            }

            wtx.vout.push_back(Legacy::TxOut(10000, scriptPubKey));

            //get inputs
            std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
            REQUIRE(wtx.FetchInputs(inputs));

            //connect tx
            REQUIRE_FALSE(wtx.Connect(inputs, state, TAO::Ledger::FLAGS::BLOCK));

            //check wallet balance
            REQUIRE(Legacy::Wallet::Instance().GetBalance() == 1850000);
        }
 */

        //Add balance to trust account by crediting from Coinbase tx
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            uint32_t nOut = 0;
            {
                Legacy::Transaction tx;
                REQUIRE(LLD::Legacy->ReadTx(hashTx, tx));

                for(; nOut < tx.vout.size(); ++nOut)
                    if(tx.vout[nOut].nValue == 20000)
                        break;
            }

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << nOut << hashAccount << TAO::Register::WILDCARD_ADDRESS << uint64_t(20000);

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
                REQUIRE(trust.get<uint64_t>("balance") == 120000);
                REQUIRE(trust.get<uint256_t>("token") == 0);
            }
        }


        //try tou double spend
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.nTimestamp  = runtime::timestamp();

            uint32_t nOut = 0;
            {
                Legacy::Transaction tx;
                REQUIRE(LLD::Legacy->ReadTx(hashTx, tx));

                for(; nOut < tx.vout.size(); ++nOut)
                    if(tx.vout[nOut].nValue == 20000)
                        break;
            }

            //payload
            tx[0] << uint8_t(OP::CREDIT) << hashTx << nOut << hashAccount << TAO::Register::WILDCARD_ADDRESS << uint64_t(20000);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            //check register values
            {
                Object trust;
                REQUIRE(LLD::Register->ReadState(hashAccount, trust));

                //parse register
                REQUIRE(trust.Parse());

                //check balance (claimed Coinbase amount added to balance)
                REQUIRE(trust.get<uint64_t>("balance") == 120000);
                REQUIRE(trust.get<uint256_t>("token") == 0);
            }
        }
    }
}
