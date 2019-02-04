/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>



#include <LLP/include/global.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <LLD/include/global.h>

#include <TAO/API/include/rpc.h>
#include <TAO/API/include/cmd.h>
#include <TAO/API/include/supply.h>
#include <TAO/API/include/accounts.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Operation/include/execute.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <Legacy/include/ambassador.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <iostream>
#include <sstream>


/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>*    TIME_SERVER;
}


int main(int argc, char** argv)
{
    LLP::Server<LLP::CoreNode>* CORE_SERVER = nullptr;
    LLP::Server<LLP::RPCNode>* RPC_SERVER = nullptr;
    LLP::Server<LLP::Miner>* MINING_SERVER = nullptr;

    uint16_t port = 0;

    /* Setup the timer timer. */
    runtime::timer timer;
    timer.Start();

    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Read the configuration file. */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();


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
    if(!filesystem::exists(config::GetDataDir(false)) &&
        filesystem::create_directory(config::GetDataDir(false)))
    {
        debug::log(0, FUNCTION, "Generated Path ", config::GetDataDir(false));
    }


    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::APPEND | LLD::FLAGS::FORCE);
    LLD::locDB = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::legDB = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /* Initialize the Legacy Database. */
    LLD::trustDB  = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::legacyDB = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /* Handle database recovery mode. */
    LLD::TxnRecovery();


    /** Load the Wallet Database. **/
    bool fFirstRun;
    if (!Legacy::Wallet::InitializeWallet(config::GetArg("-wallet", Legacy::WalletDB::DEFAULT_WALLET_DB)))
        return debug::error("failed initializing wallet");


    /** Check the wallet loading for errors. **/
    uint32_t nLoadWalletRet = Legacy::Wallet::GetInstance().LoadWallet(fFirstRun);
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

    /** Rebroadcast transactions. **/
    Legacy::Wallet::GetInstance().ResendWalletTransactions();


    /** Initialize ChainState. */
    TAO::Ledger::ChainState::Initialize();


    /** Initialize the scripts for legacy mode. **/
    Legacy::InitializeScripts();


    /** Handle Rescanning. **/
    if(config::GetBoolArg("-rescan"))
        Legacy::Wallet::GetInstance().ScanForWalletTransactions(&TAO::Ledger::ChainState::stateGenesis, true);


    /** Startup the time server. **/
    LLP::TIME_SERVER = new LLP::Server<LLP::TimeNode>(
        9324,
        10,
        30,
        false,
        0,
        0,
        10,
        config::GetBoolArg("-unified", false),
        config::GetBoolArg("-meters", false),
        config::GetBoolArg("-manager", true),
        30000);


    /** Handle the beta server. */
    if(!config::GetBoolArg("-beta"))
    {
        /** Get the port for Tritium Server. **/
        port = static_cast<uint16_t>(config::GetArg("-port", config::fTestNet ? 8888 : 9888));


        /* Initialize the Tritium Server. */
        LLP::TRITIUM_SERVER = new LLP::Server<LLP::TritiumNode>(
            port,
            config::GetArg("-threads", 10),
            config::GetArg("-timeout", 30),
            config::GetBoolArg("-ddos", false),
            config::GetArg("-cscore", 1),
            config::GetArg("-rscore", 50),
            config::GetArg("-timespan", 60),
            config::GetBoolArg("-listen", true),
            config::GetBoolArg("-meters", false),
            config::GetBoolArg("-manager", true));


        /* -connect means  try to establish a connection */
        if(config::mapMultiArgs["-connect"].size() > 0)
        {
            for(const auto& node : config::mapMultiArgs["-connect"])
                LLP::TRITIUM_SERVER->AddConnection(node, port);
        }

        /* -addnode means add to address manager */
        if(config::mapMultiArgs["-addnode"].size() > 0)
        {
            for(const auto& node : config::mapMultiArgs["-addnode"])
                LLP::TRITIUM_SERVER->AddNode(node, port);
        }

    }
    /* Initialize the Legacy Server. */
    else
    {
        port = static_cast<uint16_t>(config::GetArg("-port", config::fTestNet ? 8323 : 9323));

        LLP::LEGACY_SERVER = new LLP::Server<LLP::LegacyNode>(
            port,
            config::GetArg("-threads", 10),
            config::GetArg("-timeout", 30),
            config::GetBoolArg("-ddos", false),
            config::GetArg("-cscore", 1),
            config::GetArg("-rscore", 50),
            config::GetArg("-timespan", 60),
            config::GetBoolArg("-listen", true),
            config::GetBoolArg("-meters", false),
            config::GetBoolArg("-manager", true));

        /* -connect means  try to establish a connection */
        if(config::mapMultiArgs["-connect"].size() > 0)
        {
            for(const auto& node : config::mapMultiArgs["-connect"])
                LLP::LEGACY_SERVER->AddConnection(node, port);
        }

        /* -addnode means add to address manager */
        if(config::mapMultiArgs["-addnode"].size() > 0)
        {
            for(const auto& node : config::mapMultiArgs["-addnode"])
                LLP::LEGACY_SERVER->AddNode(node, port);
        }
    }


    /* Create the Core API Server. */
    CORE_SERVER = new LLP::Server<LLP::CoreNode>(
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
    RPC_SERVER = new LLP::Server<LLP::RPCNode>(
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


    /* Set up Mining Server */
    if(config::GetBoolArg("-mining"))
    {
        MINING_SERVER = new LLP::Server<LLP::Miner>(
            config::GetArg("-miningport", config::fTestNet ? 8325 : 9325),
            10,
            30,
            false,
            0,
            0,
            60,
            config::GetBoolArg("-listen", true),
            false,
            false);
    }


    /* Elapsed Milliseconds from timer. */
    uint32_t nElapsed = timer.ElapsedMilliseconds();
    timer.Stop();


    /* Sleep before output. */
    runtime::sleep(100);


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Started up in ", nElapsed, "ms");


    /* Initialize generator thread. */
    std::thread thread;
    if(config::GetBoolArg("-private"))
        thread = std::thread(TAO::Ledger::ThreadGenerator);


    /* Wait for shutdown. */
    if(!config::GetBoolArg("-gdb"))
    {
        std::mutex SHUTDOWN_MUTEX;
        std::unique_lock<std::mutex> SHUTDOWN_LOCK(SHUTDOWN_MUTEX);
        SHUTDOWN.wait(SHUTDOWN_LOCK, []{ return config::fShutdown; });
    }

    /* GDB mode waits for keyboard input to initiate clean shutdown. */
    else
    {
        getchar();
        config::fShutdown = true;
    }


    /* Wait for the private condition. */
    if(config::GetBoolArg("-private"))
    {
        TAO::Ledger::PRIVATE_CONDITION.notify_all();
        thread.join();
    }


    /* Shutdown metrics. */
    timer.Reset();


    /* Shutdown the tritium server and its subsystems */
    if(LLP::TIME_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Time Server");

        LLP::TIME_SERVER->Shutdown();
        delete LLP::TIME_SERVER;
    }


    /* Shutdown the tritium server and its subsystems */
    if(LLP::TRITIUM_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Tritium Server");

        LLP::TRITIUM_SERVER->Shutdown();
        delete LLP::TRITIUM_SERVER;
    }


    /* Shutdown the legacy server and its subsystems */
    if(LLP::LEGACY_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Legacy Server");

        LLP::LEGACY_SERVER->Shutdown();
        delete LLP::LEGACY_SERVER;
    }


    /* Shutdown the core API server and its subsystems */
    if(CORE_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down API Server");

        CORE_SERVER->Shutdown();
        delete CORE_SERVER;
    }


    /* Shutdown the RPC server and its subsystems */
    if(RPC_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down RPC Server");

        RPC_SERVER->Shutdown();
        delete RPC_SERVER;
    }


    /* Shutdown the mining server and its subsystems */
    if(MINING_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Mining Server");

        MINING_SERVER->Shutdown();
        delete MINING_SERVER;
    }


    /* Cleanup the ledger database. */
    if(LLD::legDB)
    {
        debug::log(0, FUNCTION, "Shutting down ledgerDB");

        delete LLD::legDB;
    }


    /* Cleanup the register database. */
    if(LLD::regDB)
    {
        debug::log(0, FUNCTION, "Shutting down registerDB");

        delete LLD::regDB;
    }


    /* Cleanup the local database. */
    if(LLD::locDB)
    {
        debug::log(0, FUNCTION, "Shutting down localDB");

        delete LLD::locDB;
    }


    /* Cleanup the legacy database. */
    if(LLD::legacyDB)
    {
        debug::log(0, FUNCTION, "Shutting down legacyDB");

        delete LLD::legacyDB;
    }


    /* Cleanup the local database. */
    if(LLD::trustDB)
    {
        debug::log(0, FUNCTION, "Shutting down trustDB");

        delete LLD::trustDB;
    }



    /* Elapsed Milliseconds from timer. */
    nElapsed = timer.ElapsedMilliseconds();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", nElapsed, "ms");

    return 0;
}
