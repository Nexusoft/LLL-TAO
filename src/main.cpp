/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <TAO/API/include/cmd.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/API/include/supply.h>
#include <TAO/API/include/accounts.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <LLP/types/miner.h>


namespace Legacy
{
    Legacy::CWallet* pwalletMain;
}

/* Declare the Global LLD Instances. */
namespace LLD
{
    RegisterDB* regDB;
    LedgerDB*   legDB;
    LocalDB*    locDB;
}


/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
}


int main(int argc, char** argv)
{

    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Read the configuration file. */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);


    /* Handle Commandline switch */
    for (int i = 1; i < argc; ++i)
    {
        if (!IsSwitchChar(argv[i][0]))
        {
            if(config::GetBoolArg("-api"))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    /* Create directories if they don't exist yet. */
    if(!filesystem::exists(config::GetDataDir(false)) && filesystem::create_directory(config::GetDataDir(false)))
        debug::log(0, FUNCTION "Generated Path %s", __PRETTY_FUNCTION__, config::GetDataDir(false).c_str());


    /* Create the database instances. */
    runtime::timer time;
    LLD::regDB = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::APPEND);
    LLD::locDB = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::legDB = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /** Load the Wallet Database. **/
    bool fFirstRun;
    Legacy::pwalletMain = new Legacy::CWallet(config::GetArg("-wallet", "wallet.dat"));
    int nLoadWalletRet = Legacy::pwalletMain->LoadWallet(fFirstRun);
    if (nLoadWalletRet != Legacy::DB_LOAD_OK)
    {
        if (nLoadWalletRet == Legacy::DB_CORRUPT)
            return debug::error("failed loading wallet.dat: Wallet corrupted");

        else if (nLoadWalletRet == Legacy::DB_TOO_NEW)
            return debug::error("failed loading wallet.dat: Wallet requires newer version of Nexus");
        else if (nLoadWalletRet == Legacy::DB_NEED_REWRITE)
            return debug::error("wallet needed to be rewritten: restart Nexus to complete");
        else
            return debug::error("failed loading wallet.dat");
    }


    /* Initialize the Tritium Server. */
    LLP::TRITIUM_SERVER = new LLP::Server<LLP::TritiumNode>(
        config::GetArg("-port", config::fTestNet ? 8888 : 9888),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        config::GetBoolArg("-meters", false),
        true);


    /* Add node to Tritium server */
    if(config::mapMultiArgs["-addnode"].size() > 0)
    {
        for(auto node : config::mapMultiArgs["-addnode"])
        {
            LLP::TRITIUM_SERVER->AddConnection(
                node,
                config::GetArg("-port", config::fTestNet ? 8888 : 9888));
        }
    }


    /* Initialize the Legacy Server.
    LLP::LEGACY_SERVER = new LLP::Server<LLP::LegacyNode>(
        config::GetArg("-port", config::fTestNet ? 8323 : 9323),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        config::GetBoolArg("-meters", false),
        true);


    /* Add node to Legacy server
    if(config::mapMultiArgs["-addnode"].size() > 0)
    {
        for(auto node : config::mapMultiArgs["-addnode"])
        {
            LLP::LEGACY_SERVER->AddConnection(
                node,
                config::GetArg("-port", config::fTestNet ? 8323 : 9323));
        }
    }
    */

    /* Create the Core API Server. */
    LLP::Server<LLP::CoreNode>* CORE_SERVER = new LLP::Server<LLP::CoreNode>(
        config::GetArg("-apiport", 8080),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);

    /* Set up RPC server */
    TAO::API::RPCCommands = new TAO::API::RPC();
    LLP::Server<LLP::RPCNode>* RPC_SERVER = new LLP::Server<LLP::RPCNode>(
        config::GetArg("-rpcport", config::fTestNet? 8336 : 9336),
        1,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);


    /* Busy wait for Shutdown. */
    while(!config::fShutdown)
        runtime::sleep(1000);


    /* Cleanup the databases. */
    if(LLD::legDB)
        delete LLD::legDB;

    if(LLD::regDB)
        delete LLD::regDB;

    if(LLD::locDB)
        delete LLD::locDB;


    /* Shutdown the servers and their subsystems */
    if(LLP::TRITIUM_SERVER)
    {
        LLP::TRITIUM_SERVER->Shutdown();
        delete LLP::TRITIUM_SERVER;
    }

    if(LLP::LEGACY_SERVER)
    {
        LLP::LEGACY_SERVER->Shutdown();
        delete LLP::LEGACY_SERVER;
    }

    if(CORE_SERVER)
    {
        CORE_SERVER->Shutdown();
        delete CORE_SERVER;
    }

    if(RPC_SERVER)
    {
        RPC_SERVER->Shutdown();
        delete RPC_SERVER;
    }


    return 0;
}
