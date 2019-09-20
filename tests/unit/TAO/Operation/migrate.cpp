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

TEST_CASE( "Migrate Operation Test - Trust key with Genesis only", "[operation]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Generate random genesis */
    uint256_t hashGenesis  = LLC::GetRand256();

    /* Generate trust address deterministically */
    TAO::Register::Address hashTrust = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

    uint512_t hashLastTrust;
    uint512_t hashTx;
    Legacy::TrustKey trustKey;

    /* Retrieve any existing balance in temp wallet from prior tests */
    int64_t nBalance = Legacy::Wallet::GetInstance().GetBalance();

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
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << object.GetState();

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
            REQUIRE(LLD::Register->ReadState(hashTrust, trust));

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


    //Create a legacy coinstake transaction with genesis for a new trust key
    {
        //get key from temp wallet
        std::vector<uint8_t> vKey;
        REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));

        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));
        tx.nTime = runtime::timestamp(); //CreateCoinstake uses stateBest.nTime + 1 which will be incorrect

        //set up coinstake as genesis transaction for trust key with an arbitrary balance as output
        tx.vin[0].prevout.SetNull();
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << vKey << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = 15500010000; //15500 NXS + 0.01 for fee to send

        //Add coinstake output to temp wallet balance
        nBalance += 15500010000;

        //define empty inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinstake as its vtx[0]
        ++state.nHeight;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, state, true, true));

        //Update best height +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

        hashLastTrust = tx.GetHash();

        //Store trust key to database
        trustKey.vchPubKey = vKey;
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.nLastBlockTime = state.nTime;
        trustKey.nStakeRate = 0.05; //don't need this for test

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));
    }


    //set best
    state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashTrust); //set trust account register address as output

        //create sending vectors
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, 15500000000));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(Legacy::Wallet::GetInstance());

        //create transaction
        int64_t nFees;
        REQUIRE(Legacy::Wallet::GetInstance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));
        nBalance -= 15500010000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //index to block
        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with coinstake balance sent from trust key)
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //Create OP::MIGRATE to claim the legacy send (genesis only trust key)
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
        uint1024_t hashLastBlock;
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
        tx[0] << uint8_t(OP::MIGRATE) << txSend.GetHash() << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

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
            REQUIRE(trust.get<uint64_t>("stake")   == 15500000000);
            REQUIRE(trust.get<uint256_t>("token")  == 0);

            uint512_t hashLastStake;
            REQUIRE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
            REQUIRE(hashLastStake == hashLastTrust);
        }


    }


    //Switch user
    hashGenesis  = LLC::GetRand256();
    hashTrust = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);



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
            tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << object.GetState();

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
            REQUIRE(LLD::Register->ReadState(hashTrust, trust));

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


    //Create a legacy coinstake transaction with genesis for a new trust key
    {
        //create coinstake
        Legacy::Transaction tx;
        REQUIRE(Legacy::CreateCoinstake(tx));
        tx.nTime = runtime::timestamp(); //CreateCoinstake uses stateBest.nTime + 1 which will be incorrect

        //set up coinstake as genesis transaction for trust key with an arbitrary balance as output
        tx.vin[0].prevout.SetNull();
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey << trustKey.vchPubKey << Legacy::OP_CHECKSIG;
        tx.vout[0].nValue = 12500010000; //12500 NXS + 0.01 for fee to send

        //Add coinstake output to temp wallet balance
        nBalance += 12500010000;

        //define empty inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(tx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

        //store block with coinstake as its vtx[0]
        ++state.nHeight;
        state.hashNextBlock = LLC::GetRand1024();
        state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //connect tx
        REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, state, true, true));

        //Update best height +10 so coinstake is mature
        state.nHeight += 10;
        state.hashNextBlock = LLC::GetRand1024();

        TAO::Ledger::ChainState::stateBest.store(state);
        TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //check balance
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

        hashLastTrust = tx.GetHash();

        //Update trust key to database, do not change vchPubKey (migrating same trust key again)
        trustKey.hashGenesisBlock = state.GetHash();
        trustKey.hashLastBlock = state.GetHash();
        trustKey.nGenesisTime = state.nTime;
        trustKey.nLastBlockTime = state.nTime;

        uint576_t cKey;
        cKey.SetBytes(trustKey.vchPubKey);

        REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));
    }


    //set best
    state = TAO::Ledger::ChainState::stateBest.load();
    ++state.nHeight;
    state.hashNextBlock = LLC::GetRand1024();

    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

    REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


    //Create a legacy send-to-register to initiate trust key migration
    {
        Legacy::Script scriptPubKey;
        scriptPubKey.SetRegisterAddress(hashTrust); //set trust account register address as output

        //create sending vectors
        std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, 12500000000));

        //create the transaction
        Legacy::WalletTx wtx;
        wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

        //for change
        Legacy::ReserveKey changeKey(Legacy::Wallet::GetInstance());

        //create transaction
        int64_t nFees;
        REQUIRE(Legacy::Wallet::GetInstance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));
        nBalance -= 12500010000;

        //get inputs
        std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
        REQUIRE(wtx.FetchInputs(inputs));

        //write to disk
        REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

        //connect tx
        REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

        REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

        //index to block
        REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

        //add to wallet
        REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(wtx, state, true, true));

        //check wallet balance (should be back to original with coinstake balance sent from trust key)
        REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

        //set the hash to spend
        hashTx = wtx.GetHash();
    }


    //Create OP::MIGRATE to claim the legacy send (migrating same trust key again - intended fail)
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
        uint1024_t hashLastBlock;
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
        tx[0] << uint8_t(OP::MIGRATE) << txSend.GetHash() << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

        //generate the prestates and poststates
        REQUIRE(tx.Build());

        //verify the prestates and poststates
        REQUIRE(tx.Verify());

        //execute the operation
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


// TEST_CASE( "Migrate Operation Test", "[operation]")
// {
//     using namespace TAO::Register;
//     using namespace TAO::Operation;

//     /* Generate random genesis */
//     uint256_t hashGenesis  = LLC::GetRand256();

//     /* Generate trust address deterministically */
//     uint256_t hashTrust = uint256_t(std::string("trust"), hashGenesis, TAO::Register::Address);

//     uint512_t hashLastTrust;
//     uint512_t hashTx;
//     Legacy::TrustKey trustKey;

//     /* Retrieve any existing balance in temp wallet from prior tests */
//     int64_t nBalance = Legacy::Wallet::GetInstance().GetBalance();

//     //set best
//     TAO::Ledger::BlockState state = TAO::Ledger::ChainState::stateBest.load();
//     ++state.nHeight;
//     state.hashNextBlock = LLC::GetRand1024();

//     TAO::Ledger::ChainState::stateBest.store(state);
//     TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

//     REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


//     //create a trust account register
//     {
//         {
//             //create the transaction object
//             TAO::Ledger::Transaction tx;
//             tx.hashGenesis = hashGenesis;
//             tx.nSequence   = 0;
//             tx.nTimestamp  = runtime::timestamp();

//             //create object
//             Object object = CreateTrust();
//             object << std::string("testing") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT256_T) << uint256_t(555);

//             //payload
//             tx[0] << uint8_t(OP::CREATE) << hashTrust << uint8_t(REGISTER::OBJECT) << object.GetState();

//             //generate the prestates and poststates
//             REQUIRE(tx.Build());

//             //verify the prestates and poststates
//             REQUIRE(tx.Verify());

//             //commit to disk
//             REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
//         }


//         {
//             //check the trust register
//             TAO::Register::Object trust;
//             REQUIRE(LLD::Register->ReadState(hashTrust, trust));

//             //parse
//             REQUIRE(trust.Parse());

//             //check standards
//             REQUIRE(trust.Standard() == OBJECTS::TRUST);
//             REQUIRE(trust.Base()     == OBJECTS::ACCOUNT);

//             //check values
//             REQUIRE(trust.get<uint64_t>("balance") == 0);
//             REQUIRE(trust.get<uint64_t>("trust")   == 0);
//             REQUIRE(trust.get<uint64_t>("stake")   == 0);
//             REQUIRE(trust.get<uint256_t>("token")  == 0);
//         }
//     }


//     //Create a legacy coinstake transaction with genesis for a new trust key
//     {
//         //get key from temp wallet
//         std::vector<uint8_t> vKey;
//         REQUIRE(Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(vKey, false));

//         //create coinstake
//         Legacy::Transaction tx;
//         REQUIRE(Legacy::CreateCoinstake(tx));
//         tx.nTime = runtime::timestamp(); //CreateCoinstake uses stateBest.nTime + 1 which will be incorrect

//         //set up coinstake as genesis transaction for trust key with an arbitrary balance as output
//         tx.vin[0].prevout.SetNull();
//         tx.vout.resize(1);
//         tx.vout[0].scriptPubKey << vKey << Legacy::OP_CHECKSIG;
//         tx.vout[0].nValue = 15500010000; //15500 NXS + 0.01 for fee to send

//         //Add coinstake output to temp wallet balance
//         nBalance += 15500010000;

//         //define empty inputs
//         std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
//         REQUIRE(tx.FetchInputs(inputs));

//         //write to disk
//         REQUIRE(LLD::Legacy->WriteTx(tx.GetHash(), tx));

//         //store block with coinstake as its vtx[0]
//         ++state.nHeight;
//         state.hashNextBlock = LLC::GetRand1024();
//         state.vtx.push_back(std::make_pair(uint8_t(TAO::Ledger::TRANSACTION::LEGACY), tx.GetHash()));

//         REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

//         //connect tx
//         REQUIRE(tx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

//         REQUIRE(LLD::Ledger->IndexBlock(tx.GetHash(), state.GetHash()));

//         //add to wallet
//         REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(tx, state, true, true));

//         //Update best height +10 so coinstake is mature
//         state.nHeight += 10;
//         state.hashNextBlock = LLC::GetRand1024();

//         TAO::Ledger::ChainState::stateBest.store(state);
//         TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

//         REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

//         //check balance
//         REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

//         hashLastTrust = tx.GetHash();

//         //Store trust key to database
//         trustKey.vchPubKey = vKey;
//         trustKey.hashGenesisBlock = state.GetHash();
//         trustKey.hashLastBlock = state.GetHash();
//         trustKey.nGenesisTime = state.nTime;
//         trustKey.nLastBlockTime = state.nTime;
//         trustKey.nStakeRate = 0.05; //don't need this for test

//         uint576_t cKey;
//         cKey.SetBytes(trustKey.vchPubKey);

//         REQUIRE(LLD::Trust->WriteTrustKey(cKey, trustKey));
//     }


//     //set best
//     state = TAO::Ledger::ChainState::stateBest.load();
//     ++state.nHeight;
//     state.hashNextBlock = LLC::GetRand1024();

//     TAO::Ledger::ChainState::stateBest.store(state);
//     TAO::Ledger::ChainState::nBestHeight.store(state.nHeight);

//     REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));


//     //Create a legacy send-to-register to initiate trust key migration
//     {
//         Legacy::Script scriptPubKey;
//         scriptPubKey.SetRegisterAddress(hashTrust); //set trust account register address as output

//         //create sending vectors
//         std::vector< std::pair<Legacy::Script, int64_t> > vecSend;
//         vecSend.push_back(make_pair(scriptPubKey, 15500000000));

//         //create the transaction
//         Legacy::WalletTx wtx;
//         wtx.fromAddress = Legacy::NexusAddress(trustKey.vchPubKey);

//         //for change
//         Legacy::ReserveKey changeKey(Legacy::Wallet::GetInstance());

//         //create transaction
//         int64_t nFees;
//         REQUIRE(Legacy::Wallet::GetInstance().CreateTransaction(vecSend, wtx, changeKey, nFees, 1));
//         nBalance -= 15500010000;

//         //get inputs
//         std::map<uint512_t, std::pair<uint8_t, DataStream> > inputs;
//         REQUIRE(wtx.FetchInputs(inputs));

//         //write to disk
//         REQUIRE(LLD::Legacy->WriteTx(wtx.GetHash(), wtx));

//         //connect tx
//         REQUIRE(wtx.Connect(inputs, state, Legacy::FLAGS::BLOCK));

//         REQUIRE(LLD::Ledger->WriteBlock(state.GetHash(), state));

//         //index to block
//         REQUIRE(LLD::Ledger->IndexBlock(wtx.GetHash(), state.GetHash()));

//         //add to wallet
//         REQUIRE(Legacy::Wallet::GetInstance().AddToWalletIfInvolvingMe(wtx, state, true, true));

//         //check wallet balance (should be back to original with coinstake balance sent from trust key)
//         REQUIRE(Legacy::Wallet::GetInstance().GetBalance() == nBalance);

//         //set the hash to spend
//         hashTx = wtx.GetHash();
//     }


//     //Create OP::MIGRATE to claim the legacy send (genesis only trust key)
//     {
//         //Retrieve legacy tx used created for migrate
//         Legacy::Transaction txSend;
//         REQUIRE(LLD::Legacy->ReadTx(hashTx, txSend));

//         //Retrieve the destination register (trust account)
//         uint256_t hashAccount;
//         REQUIRE(txSend.vout.size() == 1);
//         REQUIRE(Legacy::ExtractRegister(txSend.vout[0].scriptPubKey, hashAccount));

//         TAO::Register::Object account;
//         REQUIRE(LLD::Register->ReadState(hashAccount, account));
//         REQUIRE(account.Parse());

//         //Verify trust key can be migrated
//         REQUIRE(account.Standard() == TAO::Register::OBJECTS::TRUST);
//         REQUIRE(account.get<uint64_t>("stake") == 0);
//         REQUIRE(account.get<uint64_t>("trust") == 0);

//         //Extract data needed for migrate
//         uint32_t nScore;
//         uint576_t hashTrust;
//         uint512_t hashLast;
//         uint1024_t hashLastBlock;
//         Legacy::TrustKey migratedKey;

//         REQUIRE(Legacy::FindMigratedTrustKey(txSend, migratedKey));

//         hashTrust.SetBytes(migratedKey.vchPubKey);
//         REQUIRE(!LLD::Legacy->HasTrustConversion(hashTrust));

//         TAO::Ledger::BlockState stateLast;
//         REQUIRE(LLD::Ledger->ReadBlock(migratedKey.hashLastBlock, stateLast));

//         Legacy::Transaction txLast;
//         REQUIRE(LLD::Legacy->ReadTx(stateLast.vtx[0].second, txLast));
//         hashLast = txLast.GetHash();

//         //Genesis coinstake has no trust
//         nScore = 0;

//         //create the transaction object
//         TAO::Ledger::Transaction tx;
//         tx.hashGenesis = hashGenesis;
//         tx.nSequence   = 1;
//         tx.nTimestamp  = runtime::timestamp();

//         //payload
//         tx[0] << uint8_t(OP::MIGRATE) << txSend.GetHash() << hashAccount << hashTrust << txSend.vout[0].nValue << nScore << hashLast;

//         //generate the prestates and poststates
//         REQUIRE(tx.Build());

//         //verify the prestates and poststates
//         REQUIRE(tx.Verify());

//         //commit to disk
//         REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

//         //check migrate results
//         {
//             //check trust key migrated
//             REQUIRE(LLD::Legacy->HasTrustConversion(hashTrust));

//             //check trust account indexed
//             REQUIRE(LLD::Register->HasTrust(hashGenesis));

//             TAO::Register::Object trust;
//             REQUIRE(LLD::Register->ReadTrust(hashGenesis, trust));

//             //parse register
//             REQUIRE(trust.Parse());

//             //check values
//             REQUIRE(trust.get<uint64_t>("balance") == 0);
//             REQUIRE(trust.get<uint64_t>("trust")   == 0);
//             REQUIRE(trust.get<uint64_t>("stake")   == 15500000000);
//             REQUIRE(trust.get<uint256_t>("token")  == 0);

//             uint512_t hashLastStake;
//             REQUIRE(LLD::Ledger->ReadStake(hashGenesis, hashLastStake));
//             REQUIRE(hashLastStake == hashLastTrust);
//         }


//     }

//     //Create a second coinstake with preset trust score to represent an established trust key

//     //Create a legacy send-to-register to initiate trust key migration

//     //Create OP::MIGRATE with invalid balance (intended fail)

//     //Create OP::MIGRATE with invalid trust score (intended fail)

//     //Create OP::MIGRATE with invalid trust key (intended fail)

//     //Create OP::MIGRATE with invalid last stake hash (intended fail)

//     //Create OP::MIGRATE to claim legacy send (full trust key)

// }
