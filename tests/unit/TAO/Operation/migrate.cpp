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

#include <TAO/Operation/types/stream.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/transaction.h>

#include <Legacy/include/create.h>
#include <Legacy/include/evaluate.h>
#include <Legacy/include/signature.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>
#include <Legacy/types/txin.h>
#include <Legacy/wallet/reservekey.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/wallettx.h>

#include <unit/catch2/catch.hpp>

TEST_CASE( "Migrate Operation Test - Genesis coinstake", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Generate random genesis */
    uint256_t hashGenesis  = LLC::GetRand256();

    /* Generate trust address deterministically */
    TAO::Register::Address hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    uint512_t hashLastTrust;
    uint512_t hashTx;
    Legacy::TrustKey trustKey;

    /* Retrieve any existing balance in temp wallet from prior tests */
    Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();
    int64_t nBalance = wallet.GetBalance();
    int64_t nTrustKeyBalance = 0;

    //set best
    TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //first test normal migration of trust key with Genesis coinstake

    //create a trust account register
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object object = CreateTrust();
            object << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //check the trust register
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashRegister, trust));

            //parse
            REQUIRE(trust.Parse());

            //check standards
            REQUIRE(trust.Standard() == OBJECTS::TRUST);
            REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);
        }
    }


    //Create a coinbase to provide input for the coinstake transaction
    {
        //reserve key from temp wallet to use for coinbase
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pKey, coinbase, 1, 0, 7, tx));
        tx.vout[0].nValue = 15500000000; //15500 NXS

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        nBalance += 15500000000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinbase as its vtx[0]
        ++state.nHeight;
        state.nChannel = 1;
        state.nTime = runtime::timestamp() - (86400 * 4); //set 4 days ago for coinstake reward calculation in next section
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //set best +10 so coinbase is mature
        state = TAO::Ledger::ChainState::stateBest.load();
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save the hash to spend in next section
        hashTx = tx.GetHash();
    }


    //Create a trust key via a Genesis coinstake transaction with balance from coinbase we just generated
    {
        //reserve key from temp wallet to use for trust key
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));
        tx.nTime = runtime::timestamp(); //CreateCoinstake uses stateBest.nTime + 1 which will be incorrect

        //set up coinstake as Genesis tx for trust key
        tx.vin[0].prevout.SetNull();

        //"spend" the previous coinbase as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the coinbase tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == 15500000000);

        // Add coinbase as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Genesis coinstake data at 0 and previous coinbase input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << pKey->GetReservedKey() << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());
        REQUIRE(tx.IsGenesis());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nTime = tx.nTime + 10; //block must be after tx
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (15500000000 + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Store trust key to database
        trustKey.vchPubKey = pKey->GetReservedKey();
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.hashGenesisTx = tx.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005;

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save last stake tx hash
        hashLastTrust = tx.GetHash();
    }


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashRegister); //set trust account register address as output

        //create sending vectors
        nTrustKeyBalance -= 10000; //tx will send balance less transaction fee
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nTrustKeyBalance));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(wallet);

        //create transaction
        int64_t nFees;
        REQUIRE(wallet.CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

        //update tracked balances
        nBalance -= (nTrustKeyBalance + nFees); //amount sent

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with trust key balance sent)
        REQUIRE(wallet.GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //For this first test, also verify the OP::DEBIT created to process legacy send
    //This is used within OP::MIGRATE, which extends the normal OP::DEBIT created to process a legacy send-to-register
    //If that ever changes without a corresponding update to OP::MIGRATE process, this test will fail.
    {
        Contract debit = LLD::Ledger->ReadContract(hashTx, 0);
        REQUIRE(::Legacy::BuildMigrateDebit(debit, hashTx));

        debit.Reset();

        uint8_t OP;
        debit >> OP;
        REQUIRE(OP == OP::DEBIT);

        debit.Seek(220);
        REQUIRE(debit.End());
    }


    //Create OP::MIGRATE to credit the legacy send and complete trust key migration (genesis only trust key)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Genesis coinstake has no trust
        nScore = 0;

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migrated
            REQUIRE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account indexed
            REQUIRE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadTrust(hashGenesis, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == nTrustKeyBalance);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            uint512_t hashLastStake;
            REQUIRE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
            REQUIRE(hashLastStake == hashLastTrust);
        }
    }


    //Switch user
    hashGenesis  = LLC::GetRand256();
    hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    //set best
    state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //Test a second migration attempt using same trust key (attempt should fail)

    //create trust account register for second user
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object object = CreateTrust();
            object << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //check the trust register
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashRegister, trust));

            //parse
            REQUIRE(trust.Parse());

            //check standards
            REQUIRE(trust.Standard() == OBJECTS::TRUST);
            REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);
        }
    }


    //Create a coinbase to provide input for the coinstake transaction
    {
        //reserve key from temp wallet to use for coinbase
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pKey, coinbase, 1, 0, 7, tx));
        tx.vout[0].nValue = 10500000000; //10500 NXS

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        nBalance += 10500000000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinbase as its vtx[0]
        ++state.nHeight;
        state.nChannel = 1;
        state.nTime = runtime::timestamp() - (86400 * 3); //set 3 days ago for coinstake reward calculation in next section
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //set best +10 so coinbase is mature
        state = TAO::Ledger::ChainState::stateBest.load();
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save the hash to spend in next section
        hashTx = tx.GetHash();
    }


    //Create a different (unspent) Genesis coinstake transaction using the same trust key (ie, add new trust key balance)
    {
        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));
        tx.nTime = runtime::timestamp(); //CreateCoinstake uses stateBest.nTime + 1 which will be incorrect

        //set up coinstake as Genesis tx for trust key
        tx.vin[0].prevout.SetNull();

        //"spend" the previous coinbase as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the coinbase tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == 10500000000);

        // Add coinbase as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Genesis coinstake data at 0 and previous coinbase input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << trustKey.vchPubKey << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());
        REQUIRE(tx.IsGenesis());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nTime = tx.nTime + 10; //block must be after tx
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (10500000000 + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Update trust key (do not change vchPubKey -- we are attempting to migrate the same key again)
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.hashGenesisTx = tx.GetHash();
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005;

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        //save last stake tx hash
        hashLastTrust = tx.GetHash();
    }


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashRegister); //set trust account register address as output

        //create sending vectors
        nTrustKeyBalance -= 10000; //tx will send balance less transaction fee
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nTrustKeyBalance));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(wallet);

        //create transaction
        int64_t nFees;
        REQUIRE(wallet.CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

        //update tracked balances
        nBalance -= (nTrustKeyBalance + nFees); //amount sent

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with trust key balance sent)
        REQUIRE(wallet.GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //Create OP::MIGRATE to credit the legacy send (migrating same trust key again - intended fail)
    {
        //Retrieve legacy tx used created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key is new and can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //Previous conversion should already be recorded, but we will build OP::MIGRATE anyway to verify the op fails, too
        REQUIRE(LLD::Legacy->HasTrustConversion(hashTrust));

        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Genesis coinstake has no trust
        nScore = 0;

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation (this should fail as duplicate migration)
        REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migration fails, but still has trust conversion recorded from first time
            REQUIRE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account not indexed
            REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashAccount, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values (unchanged)
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //No last stake
            uint512_t hashLastStake;
            REQUIRE_FALSE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
        }
    }
}


TEST_CASE( "Migrate Operation Test - Trust coinstake", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Generate random genesis */
    uint256_t hashGenesis  = LLC::GetRand256();

    /* Generate trust address deterministically */
    TAO::Register::Address hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    uint512_t hashLastTrust;
    uint512_t hashTx;
    Legacy::TrustKey trustKey;

    /* Retrieve any existing balance in temp wallet from prior tests */
    Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();
    int64_t nBalance = wallet.GetBalance();
    int64_t nTrustKeyBalance;
    uint32_t nScoreMigrated;

    //set best
    TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //create a trust account register
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object object = CreateTrust();
            object << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //check the trust register
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashRegister, trust));

            //parse
            REQUIRE(trust.Parse());

            //check standards
            REQUIRE(trust.Standard() == OBJECTS::TRUST);
            REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);
        }
    }


    //Create a coinbase to provide input for the initial Genesis coinstake transaction
    {
        //reserve key from temp wallet to use for coinbase
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pKey, coinbase, 1, 0, 7, tx));
        tx.vout[0].nValue = 12750000000; //12750 NXS

        //set time for remaining tests
        state.nTime = runtime::timestamp() - (3600 * 7); //set 7 hours ago (we will move up for rewards calc of the 2 coinstakes)
        tx.nTime = state.nTime -10;

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        nBalance += 12750000000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinbase as its vtx[0]
        ++state.nHeight;
        state.nChannel = 1;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //set best +10 so coinbase is mature
        state = TAO::Ledger::ChainState::stateBest.load();
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save the hash to spend in next section
        hashTx = tx.GetHash();
    }


    //Create a trust key via a Genesis coinstake transaction with balance from coinbase we just generated
    {
        //reserve key from temp wallet to use for trust key
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));

        //add coinstake reward, set state to 2 hours ago (generates Genesis reward for 5 hours), tx must be before state time
        //this value must be less than TRUST_KEY_TIMESPAN_TESTNET or trust score decays to zero
        state.nTime = runtime::timestamp() - (3600 * 2);
        tx.nTime = state.nTime - 10;

        //set up coinstake as Genesis tx for trust key
        tx.vin[0].prevout.SetNull();

        //"spend" the previous coinbase as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the coinbase tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == 12750000000);

        // Add coinbase as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Genesis coinstake data at 0 and previous coinbase input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << pKey->GetReservedKey() << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (12750000000 + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Store trust key to database
        trustKey.vchPubKey = pKey->GetReservedKey();
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.hashGenesisTx = tx.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005;

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save last stake tx hash
        hashTx = tx.GetHash();
        hashLastTrust = hashTx;
    }


    //Set best with updated timestamp (for trust score calculation)
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();
    state.nTime = runtime::timestamp() - 30;
    state.vtx.clear();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //Now generate a Trust coinstake that uses the Genesis coinstake for its input
    {
        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));

        //Trust calculation needs previous block data
        TAO::Ledger::BlockState statePrev = state;
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(trustKey.hashLastBlock, stateLast));
        state.hashPrevBlock = statePrev.GetHash();
        state.nTime = runtime::timestamp();

        //"spend" the previous Genesis as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the Genesis coinstake tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == nTrustKeyBalance);

        uint32_t nSequence = 1;
        nScoreMigrated = statePrev.nTime - stateLast.nTime;

        //set up Trust coinstake
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = trustKey.GetHash();

        DataStream scriptPub(tx.vin[0].scriptSig, SER_NETWORK, LLP::PROTOCOL_VERSION);
        scriptPub << trustKey.hashLastBlock << nSequence << nScoreMigrated;

        tx.vin[0].scriptSig.clear();
        tx.vin[0].scriptSig.insert(tx.vin[0].scriptSig.end(), scriptPub.begin(), scriptPub.end());

        //add previous Genesis coinstake as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Trust coinstake data at 0 and previous Genesis input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << trustKey.vchPubKey << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());
        REQUIRE(tx.IsTrust());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //Verify the Trust data is correct
        REQUIRE(tx.CheckTrust(state, trustKey));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (txPrev.vout[0].nValue + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Update trust key
        trustKey.hashLastBlock = state.GetHash();
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005; //not used

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        //save last stake tx hash
        hashLastTrust = tx.GetHash();
    }


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashRegister); //set trust account register address as output

        //create sending vectors
        nTrustKeyBalance -= 10000; //tx will send balance less transaction fee
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nTrustKeyBalance));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(wallet);

        //create transaction
        int64_t nFees;
        REQUIRE(wallet.CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

        //update tracked balances
        nBalance -= (nTrustKeyBalance + nFees); //amount sent

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with trust key balance sent)
        REQUIRE(wallet.GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //Create OP::MIGRATE to credit the legacy send and complete trust key migration (genesis only trust key)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        //Retrieve the last stake block
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        //Retrieve the last coinstake tx from the last stake block (our test Trust coinstake)
        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Extract the trust score from the last coinstake
        uint1024_t hashLastBlock;
        uint32_t nSequence;
        REQUIRE(txLast.ExtractTrust(hashLastBlock, nSequence, nScore));

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //commit to disk
        REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migrated
            REQUIRE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account indexed
            REQUIRE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadTrust(hashGenesis, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == nScoreMigrated);
            REQUIRE(trust.get<uint64_t>("stake")   == nTrustKeyBalance);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            uint512_t hashLastStake;
            REQUIRE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
            REQUIRE(hashLastStake == hashLastTrust);
        }
    }
}


TEST_CASE( "Migrate Operation Test - Invalid OP::MIGRATE tests", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Generate random genesis */
    uint256_t hashGenesis  = LLC::GetRand256();

    /* Generate trust address deterministically */
    TAO::Register::Address hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    uint512_t hashLastTrust;
    uint512_t hashTx;
    uint512_t hashTxCoinbase;
    Legacy::TrustKey trustKey;

    /* Retrieve any existing balance in temp wallet from prior tests */
    Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();
    int64_t nBalance = wallet.GetBalance();
    int64_t nTrustKeyBalance;
    uint32_t nScoreMigrated;

    //set best
    TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //create a trust account register
    {
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();

            //create object
            Object object = CreateTrust();
            object << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashRegister << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //verify the prestates and poststates
            REQUIRE(tx.Verify());

            //commit to disk
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
        }


        {
            //check the trust register
            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashRegister, trust));

            //parse
            REQUIRE(trust.Parse());

            //check standards
            REQUIRE(trust.Standard() == OBJECTS::TRUST);
            REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

            //check values
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);
        }
    }


    //Create a coinbase to provide input for the initial Genesis coinstake transaction
    {
        //reserve key from temp wallet to use for coinbase
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //dummy coinbase
        Legacy::Coinbase coinbase;

        //make the coinbase tx
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinbase(*pKey, coinbase, 1, 0, 7, tx));
        tx.vout[0].nValue = 12750000000; //12750 NXS

        //set time for remaining tests
        state.nTime = runtime::timestamp() - (3600 * 7); //set 7 hours ago (we will move up for rewards calc of the 2 coinstakes)
        tx.nTime = state.nTime -10;

        //add the data into input script
        tx.vin[0].scriptSig << uint32_t(555);

        nBalance += 12750000000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinbase as its vtx[0]
        ++state.nHeight;
        state.nChannel = 1;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //set best +10 so coinbase is mature
        state = TAO::Ledger::ChainState::stateBest.load();
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, TAO::Ledger::ChainState::stateGenesis, true));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save the hash to spend in next section
        hashTx = tx.GetHash();
        hashTxCoinbase = hashTx; //will use for invalid last stake hash test
    }


    //Create a trust key via a Genesis coinstake transaction with balance from coinbase we just generated
    {
        //reserve key from temp wallet to use for trust key
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);

        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));

        //add coinstake reward, set state to 2 hours ago (generates Genesis reward for 5 hours), tx must be before state time
        //this value must be less than TRUST_KEY_TIMESPAN_TESTNET or trust score decays to zero
        state.nTime = runtime::timestamp() - (3600 * 2);
        tx.nTime = state.nTime - 10;

        //set up coinstake as Genesis tx for trust key
        tx.vin[0].prevout.SetNull();

        //"spend" the previous coinbase as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the coinbase tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == 12750000000);

        // Add coinbase as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Genesis coinstake data at 0 and previous coinbase input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << pKey->GetReservedKey() << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (12750000000 + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Store trust key to database
        trustKey.vchPubKey = pKey->GetReservedKey();
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.hashGenesisTx = tx.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005;

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        pKey->KeepKey();
        delete pKey;

        //save last stake tx hash
        hashTx = tx.GetHash();
        hashLastTrust = hashTx;
    }


    //Set best with updated timestamp (for trust score calculation)
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();
    state.nTime = runtime::timestamp() - 30;
    state.vtx.clear();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //Now generate a Trust coinstake that uses the Genesis coinstake for its input
    {
        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));

        //Trust calculation needs previous block data
        TAO::Ledger::BlockState statePrev = state;
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(trustKey.hashLastBlock, stateLast));
        state.hashPrevBlock = statePrev.GetHash();
        state.nTime = runtime::timestamp();

        //"spend" the previous Genesis as input (at least one valid input is needed to have valid coinstake tx)
        Legacy::Transaction txPrev;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txPrev));

        // Verify we retrieved the Genesis coinstake tx we just added
        REQUIRE(txPrev.GetHash() == hashTx);
        REQUIRE(txPrev.vout[0].nValue == nTrustKeyBalance);

        uint32_t nSequence = 1;
        nScoreMigrated = statePrev.nTime - stateLast.nTime;

        //set up Trust coinstake
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = trustKey.GetHash();

        DataStream scriptPub(tx.vin[0].scriptSig, SER_NETWORK, LLP::PROTOCOL_VERSION);
        scriptPub << trustKey.hashLastBlock << nSequence << nScoreMigrated;

        tx.vin[0].scriptSig.clear();
        tx.vin[0].scriptSig.insert(tx.vin[0].scriptSig.end(), scriptPub.begin(), scriptPub.end());

        //add previous Genesis coinstake as input
        Legacy::TxIn txin;
        txin.prevout.hash = hashTx;
        txin.prevout.n = 0;
        tx.vin.push_back(txin);
        REQUIRE(tx.vin.size() == 2); //should only have Trust coinstake data at 0 and previous Genesis input at 1

        //Set up coinstake output
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << trustKey.vchPubKey << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = txPrev.vout[0].nValue;

        //ensure we have valid coinstake transaction at this point
        REQUIRE(tx.IsCoinStake());
        REQUIRE(tx.IsTrust());

        //add coinstake reward
        uint64_t nStakeReward;
        REQUIRE(tx.CoinstakeReward(state, nStakeReward));
        tx.vout[0].nValue += nStakeReward;

        //sign the input
        REQUIRE(SignSignature(wallet, txPrev, tx, 1));
        REQUIRE(VerifySignature(txPrev, tx, 1, 0));

        //Verify the Trust data is correct
        REQUIRE(tx.CheckTrust(state, trustKey));

        //store coinstake in block as its vtx[0]
        ++state.nHeight;
        state.nChannel = 0;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        //Add coinstake reward to tracked wallet balance
        nBalance += nStakeReward;

        //save trust key balance for migrate
        nTrustKeyBalance = tx.vout[0].nValue;
        REQUIRE(nTrustKeyBalance == (txPrev.vout[0].nValue + nStakeReward));

        //retrieve inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(tx, state, true, true));

        //Update trust key
        trustKey.hashLastBlock = state.GetHash();
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.005; //not used

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));

        //Set best +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.clear();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(wallet.GetBalance() == nBalance);

        //save last stake tx hash
        hashLastTrust = tx.GetHash();
    }


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashRegister); //set trust account register address as output

        //create sending vectors
        nTrustKeyBalance -= 10000; //tx will send balance less transaction fee
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nTrustKeyBalance));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(wallet);

        //create transaction
        int64_t nFees;
        REQUIRE(wallet.CreateTransaction(vecSend, wtx, changeKey, nFees, 1));

        //update tracked balances
        nBalance -= (nTrustKeyBalance + nFees); //amount sent

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));
        REQUIRE(inputs.size() == 1);

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        //write and index block
        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(wallet.AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with trust key balance sent)
        REQUIRE(wallet.GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //Create OP::MIGRATE with invalid balance (intended fail)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        //Retrieve the last stake block
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        //Retrieve the last coinstake tx from the last stake block (our test Trust coinstake)
        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Extract the trust score from the last coinstake
        uint1024_t hashLastBlock;
        uint32_t nSequence;
        REQUIRE(txLast.ExtractTrust(hashLastBlock, nSequence, nScore));

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload (attempt to credit 1 million NXS)
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << (uint64_t)1000000000000 << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation (this should fail as debit and credit value mismatch)
        REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migration fails, but still has trust conversion recorded from first time
            REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account not indexed
            REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashAccount, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values (unchanged)
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //No last stake
            uint512_t hashLastStake;
            REQUIRE_FALSE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
        }
    }


    //Create OP::MIGRATE with invalid trust score (intended fail)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        //Retrieve the last stake block
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        //Retrieve the last coinstake tx from the last stake block (our test Trust coinstake)
        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Extract the trust score from the last coinstake
        uint1024_t hashLastBlock;
        uint32_t nSequence;
        REQUIRE(txLast.ExtractTrust(hashLastBlock, nSequence, nScore));

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << txSend.vout[0].nValue << (uint32_t)50000000 << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation (this should fail as debit and credit trust score mismatch)
        REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migration fails, but still has trust conversion recorded from first time
            REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account not indexed
            REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashAccount, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values (unchanged)
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //No last stake
            uint512_t hashLastStake;
            REQUIRE_FALSE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
        }
    }


    //Create OP::MIGRATE with invalid last stake hash (intended fail)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        //Retrieve the last stake block
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        //Retrieve the last coinstake tx from the last stake block (our test Trust coinstake)
        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Extract the trust score from the last coinstake
        uint1024_t hashLastBlock;
        uint32_t nSequence;
        REQUIRE(txLast.ExtractTrust(hashLastBlock, nSequence, nScore));

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashTxCoinbase;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation (this should fail as debit and credit stake hash mismatch)
        REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migration fails, but still has trust conversion recorded from first time
            REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

            //check trust account not indexed
            REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashAccount, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values (unchanged)
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //No last stake
            uint512_t hashLastStake;
            REQUIRE_FALSE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
        }
    }


    //Create OP::MIGRATE with invalid trust key hash (intended fail)
    {
        //Retrieve legacy tx created for migrate
        Legacy::Transaction txSend;
        REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

        //Retrieve the destination register (trust account)
        uint256_t hashAccount;
        REQUIRE(txSend.vout.size() == 1);
        REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));
        REQUIRE(hashAccount == hashRegister);

        TAO::Register::Object account;
        REQUIRE(LLD::Register->ReadState(hashAccount, account));
        REQUIRE(account.Parse());

        //Verify trust key can be migrated
        REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
        REQUIRE(account.get<uint64_t>("balance") == 0);
        REQUIRE(account.get<uint64_t>("stake") == 0);
        REQUIRE(account.get<uint64_t>("trust") == 0);
        REQUIRE(account.get<uint256_t>("token") == 0);
        REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis)); //trust account must be new (pre-Genesis)

        //Extract data needed for migrate
        uint32_t nScore;
        uint576_t hashTrust;
        uint512_t hashLast;
        Legacy::TrustKey migratedKey;

        REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

        hashTrust.SetBytes(migratedKey.vchPubKey);

        //key can only be migrated if not previously migrated
        REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));

        //Retrieve the last stake block
        TAO::Ledger::BlockState stateLast;
        REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

        //Retrieve the last coinstake tx from the last stake block (our test Trust coinstake)
        Legacy::Transaction txLast;
        REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
        hashLast = txLast.GetHash();

        //Extract the trust score from the last coinstake
        uint1024_t hashLastBlock;
        uint32_t nSequence;
        REQUIRE(txLast.ExtractTrust(hashLastBlock, nSequence, nScore));

        //create the transaction object
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = 1;
        tx.nTimestamp  = runtime::timestamp();

        //Create invalid key
        Legacy::ReserveKey* pKey = new Legacy::ReserveKey(wallet);
        uint576_t hashTrustInvalid;
        hashTrustInvalid.SetBytes(pKey->GetReservedKey());
        delete pKey;

        //payload
        tx[0] << uint8_t(OP::MIGRATE) << hashTx << hashAccount << hashTrustInvalid << txSend.vout[0].nValue << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation (this should fail as debit and credit trust key mismatch)
        REQUIRE_FALSE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

        //check migrate results
        {
            //check trust key migration fails, but still has trust conversion recorded from first time
            REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrust));
            REQUIRE_FALSE(LLD::Legacy->HasTrustConversion(hashTrustInvalid));

            //check trust account not indexed
            REQUIRE_FALSE(LLD::Register->HasTrust(hashGenesis));

            TAO::Register::Object trust;
            REQUIRE(LLD::Register->ReadState(hashAccount, trust));

            //parse register
            REQUIRE(trust.Parse());

            //check values (unchanged)
            REQUIRE(trust.get<uint64_t>("balance") == 0);
            REQUIRE(trust.get<uint64_t>("trust")   == 0);
            REQUIRE(trust.get<uint64_t>("stake")   == 0);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            //No last stake
            uint512_t hashLastStake;
            REQUIRE_FALSE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
        }
    }
}
