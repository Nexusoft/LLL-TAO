/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <unit/catch2/catch.hpp>

#include <Legacy/wallet/wallet.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/chainstate.h>

#include <LLP/include/global.h>
#include <LLP/types/apinode.h>

#include <Util/include/filesystem.h>
#include <Util/include/args.h>

TEST_CASE("Arguments Tests", "[args]")
{
    config::fTestNet = true;
    config::mapArgs["-testnet"] = "92349234";
    config::mapArgs["-flushwallet"] = "false";

    /* To simplify the API testing we will always use multiuser mode */
    config::fMultiuser = true;
    config::mapArgs["-private"] = "1";

    REQUIRE(config::fTestNet.load() == true);
    REQUIRE(config::GetArg("-testnet", 0) == 92349234);
    REQUIRE(config::fMultiuser.load() == true);

    //get the data directory
    std::string strPath = config::GetDataDir();

    //test the filesystem remove and also clear from previous unit tests
    if(filesystem::exists(strPath))
    {
        REQUIRE(filesystem::remove_directories(strPath));
        REQUIRE_FALSE(filesystem::exists(strPath));
    }

    //create LLD instances
    LLD::Contract = new LLD::ContractDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::Register = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::Local    = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::Ledger   = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::Trust    = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLD::Legacy   = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);


    //load wallet
    bool fFirstRun;
    REQUIRE(Legacy::Wallet::InitializeWallet(Legacy::WalletDB::DEFAULT_WALLET_DB));
    REQUIRE(Legacy::Wallet::GetInstance().LoadWallet(fFirstRun) == Legacy::DB_LOAD_OK);


    //initialize chain state
    REQUIRE(TAO::Ledger::ChainState::Initialize());

    //create best chain.
    TAO::Ledger::BlockState state;
    state.nHeight = 200;
    state.nBits   = 555;

    //write best to disk
    LLD::Ledger->WriteBlock(state.GetHash(), state);

    //set best block
    TAO::Ledger::ChainState::stateBest.store(state);
    TAO::Ledger::ChainState::nBestHeight.store(200);

    //check best
    REQUIRE_FALSE(TAO::Ledger::ChainState::stateBest.load().IsNull());


    /** Initialize network resources. (Need before RPC/API for WSAStartup call in Windows) **/
    REQUIRE(LLP::Initialize());

    /* Create the API instances. */
    TAO::API::Initialize();

    /* Create the Core API Server. */
    LLP::API_SERVER = new LLP::Server<LLP::APINode>(
        8080,
        10,
        30,
        false,
        0,
        0,
        60,
        true,
        false,
        false);
}
