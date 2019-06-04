/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/include/global.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/legacy_miner.h>
#include <LLP/types/tritium_miner.h>
#include <LLP/include/lisp.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/cmd.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/filesystem.h>
#include <Util/include/signals.h>
#include <Util/include/daemon.h>

#include <Legacy/include/ambassador.h>
#include <Legacy/types/legacy_minter.h>
#include <Legacy/wallet/wallet.h>

#ifndef WIN32
#include <sys/resource.h>
#endif

int main(int argc, char** argv)
{

    /* Setup the timer timer. */
    runtime::timer timer;
    timer.Start();


    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Read the configuration file. Pass argc and argv for possible -datadir setting */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs, argc, argv);


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();


    /** Initialize network resources. (Need before RPC/API for WSAStartup call in Windows) **/
    if (!LLP::NetworkStartup())
    {
        printf("ERROR: Failed initializing network resources");

        return 0;
    }


    /* Handle Commandline switch */
    for (int i = 1; i < argc; ++i)
    {
        if (!convert::IsSwitchChar(argv[i][0]))
        {
            int nRet = 0;

            /* As a helpful shortcut, if the method name includes a "/" then we will assume it is meant for the API
               since none of the RPC commands support a "/" in the method name */
            bool fIsAPI = false;

            std::string endpoint = std::string(argv[i]);
            std::string::size_type pos = endpoint.find('/');
            if(pos != endpoint.npos || config::GetBoolArg(std::string("-api")))
                fIsAPI = true;

            if(fIsAPI)
                nRet = TAO::API::CommandLineAPI(argc, argv, i);
            else
                nRet = TAO::API::CommandLineRPC(argc, argv, i);

            return nRet;
        }
    }


    /* Log system startup now, after branching to API/RPC where appropriate */
    debug::InitializeLog(argc, argv);


    /** Run the process as Daemon RPC/LLP Server if Flagged. **/
    if (config::fDaemon)
    {
        debug::log(0, FUNCTION, "-daemon flag enabled. Running in background");
        Daemonize();
    }


    /* Create directories if they don't exist yet. */
    if(!filesystem::exists(config::GetDataDir()) &&
        filesystem::create_directory(config::GetDataDir()))
    {
        debug::log(0, FUNCTION, "Generated Path ", config::GetDataDir());
    }


    /* Create the database instances. */
    LLD::Contract = new LLD::ContractDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::Register = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::Local    = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::Ledger   = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /* Initialize the Legacy Database. */
    LLD::Trust  = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::Legacy = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /* Handle database recovery mode. */
    LLD::TxnRecovery();


    /** Load the Wallet Database. **/
    bool fFirstRun;
    if (!Legacy::Wallet::InitializeWallet(config::GetArg(std::string("-wallet"), Legacy::WalletDB::DEFAULT_WALLET_DB)))
        return debug::error("Failed initializing wallet");


    /** Check the wallet loading for errors. **/
    uint32_t nLoadWalletRet = Legacy::Wallet::GetInstance().LoadWallet(fFirstRun);
    if (nLoadWalletRet != Legacy::DB_LOAD_OK)
    {
        if (nLoadWalletRet == Legacy::DB_CORRUPT)
            return debug::error("Failed loading wallet.dat: Wallet corrupted");
        else if (nLoadWalletRet == Legacy::DB_TOO_NEW)
            return debug::error("Failed loading wallet.dat: Wallet requires newer version of Nexus");
        else
            return debug::error("Failed loading wallet.dat");
    }


    /** Rebroadcast transactions. **/
    Legacy::Wallet::GetInstance().ResendWalletTransactions();


    /* Create the API instances. */
    TAO::API::Initialize();


    /** Initialize ChainState. */
    TAO::Ledger::ChainState::Initialize();


    /** Initialize the scripts for legacy mode. **/
    Legacy::InitializeScripts();


    /** Handle Rescanning. **/
    if(config::GetBoolArg(std::string("-rescan")))
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
        config::GetBoolArg(std::string("-unified"), false),
        config::GetBoolArg(std::string("-meters"), false),
        config::GetBoolArg(std::string("-manager"), true),
        30000);


    /** Handle the beta server. */
    uint16_t port = 0;
    if(!config::GetBoolArg(std::string("-beta")))
    {
        /* Get the port for Tritium Server. */
        port = static_cast<uint16_t>(config::GetArg(std::string("-port"), config::fTestNet.load() ? (8888 + (config::GetArg("-testnet", 0) - 1)) : 9888));

        /* Initialize the Tritium Server. */
        LLP::TRITIUM_SERVER = LLP::CreateTAOServer<LLP::TritiumNode>(port);

        /* Handle Manual Connections from Command Line, if there are any. */
        LLP::MakeConnections<LLP::TritiumNode>(LLP::TRITIUM_SERVER);
    }
    else
    {
        /* Get the port for Legacy Server. */
        port = static_cast<uint16_t>(config::GetArg(std::string("-port"), config::fTestNet.load() ? (8323 + (config::GetArg("-testnet", 0) - 1)) : 9323));

        /* Initialize the Legacy Server. */
        LLP::LEGACY_SERVER = LLP::CreateTAOServer<LLP::LegacyNode>(port);

        /* Handle Manual Connections from Command Line, if there are any. */
        LLP::MakeConnections<LLP::LegacyNode>(LLP::LEGACY_SERVER);
    }


    /* Get the port for the Core API Server. */
    port = static_cast<uint16_t>(config::GetArg(std::string("-apiport"), 8080));

    /* Create the Core API Server. */
    LLP::CORE_SERVER = new LLP::Server<LLP::CoreNode>(
        port,
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);


    /* Get the port for the Core API Server. */
    port = static_cast<uint16_t>(config::GetArg(std::string("-rpcport"), config::fTestNet.load() ? 8336 : 9336));

    /* Set up RPC server */
    LLP::RPC_SERVER = new LLP::Server<LLP::RPCNode>(
        port,
        static_cast<uint16_t>(config::GetArg(std::string("-rpcthreads"), 1)),
        10,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);


    /* Set up Mining Server */
    if(config::GetBoolArg(std::string("-mining")))
    {
      if(config::GetBoolArg(std::string("-beta")))
          LLP::LEGACY_MINING_SERVER  = LLP::CreateMiningServer<LLP::LegacyMiner>();
      else
          LLP::TRITIUM_MINING_SERVER = LLP::CreateMiningServer<LLP::TritiumMiner>();
    }

    /* Elapsed Milliseconds from timer. */
    uint32_t nElapsed = timer.ElapsedMilliseconds();
    timer.Stop();


    /* If wallet is not encrypted, it is unlocked by default. Start stake minter now. It will run until stopped by system shutdown. */
    if (!Legacy::Wallet::GetInstance().IsCrypted() && config::GetBoolArg(std::string("-beta")))
        Legacy::LegacyMinter::GetInstance().StartStakeMinter();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Started up in ", nElapsed, "ms");


    /* Initialize generator thread. */
    std::thread thread;
    if(config::GetBoolArg(std::string("-private")))
        thread = std::thread(TAO::Ledger::ThreadGenerator);


    /* Wait for shutdown. */
    if(!config::GetBoolArg(std::string("-gdb")))
    {
        std::mutex SHUTDOWN_MUTEX;
        std::unique_lock<std::mutex> SHUTDOWN_LOCK(SHUTDOWN_MUTEX);
        SHUTDOWN.wait(SHUTDOWN_LOCK, []{ return config::fShutdown.load(); });
    }

    /* GDB mode waits for keyboard input to initiate clean shutdown. */
    else
    {
        getchar();
        config::fShutdown = true;
    }


    /* Wait for the private condition. */
    if(config::GetBoolArg(std::string("-private")))
    {
        TAO::Ledger::PRIVATE_CONDITION.notify_all();
        thread.join();
    }


    /* Shutdown metrics. */
    timer.Reset();


    /* Stop stake minter if it is running (before server shutdown). */
    if (config::GetBoolArg(std::string("-beta")))
        Legacy::LegacyMinter::GetInstance().StopStakeMinter();


    /* Shutdown the time server and its subsystems. */
    LLP::ShutdownServer<LLP::TimeNode>(LLP::TIME_SERVER);


    /* Shutdown the tritium server and its subsystems. */
    LLP::ShutdownServer<LLP::TritiumNode>(LLP::TRITIUM_SERVER);


    /* Shutdown the legacy server and its subsystems. */
    LLP::ShutdownServer<LLP::LegacyNode>(LLP::LEGACY_SERVER);


    /* Shutdown the core API server and its subsystems. */
    LLP::ShutdownServer<LLP::CoreNode>(LLP::CORE_SERVER);


    /* Shutdown the RPC server and its subsystems. */
    LLP::ShutdownServer<LLP::RPCNode>(LLP::RPC_SERVER);


    /* Shutdown the legacy mining server and its subsystems. */
    LLP::ShutdownServer<LLP::LegacyMiner>(LLP::LEGACY_MINING_SERVER);


    /* Shutdown the tritium mining server and its subsystems. */
    LLP::ShutdownServer<LLP::TritiumMiner>(LLP::TRITIUM_MINING_SERVER);


    /** After all servers shut down, clean up underlying networking resources **/
    LLP::NetworkShutdown();


    /* Cleanup the API. */
    TAO::API::Shutdown();


    /* Cleanup the ledger database. */
    if(LLD::Contract)
    {
        debug::log(0, FUNCTION, "Shutting down ContractDB");

        delete LLD::Contract;
    }

    /* Cleanup the ledger database. */
    if(LLD::Ledger)
    {
        debug::log(0, FUNCTION, "Shutting down LedgerDB");

        delete LLD::Ledger;
    }


    /* Cleanup the register database. */
    if(LLD::Register)
    {
        debug::log(0, FUNCTION, "Shutting down RegisterDB");

        delete LLD::Register;
    }


    /* Cleanup the local database. */
    if(LLD::Local)
    {
        debug::log(0, FUNCTION, "Shutting down LocalDB");

        delete LLD::Local;
    }


    /* Cleanup the legacy database. */
    if(LLD::Legacy)
    {
        debug::log(0, FUNCTION, "Shutting down LegacyDB");

        delete LLD::Legacy;
    }


    /* Cleanup the trust database. */
    if(LLD::Trust)
    {
        debug::log(0, FUNCTION, "Shutting down TrustDB");

        delete LLD::Trust;
    }


    /* Shutdown wallet database environment. */
    if (config::GetBoolArg(std::string("-flushwallet"), true))
        Legacy::WalletDB::ShutdownFlushThread();


    /* Shutdown the Berkely database environment. */
    Legacy::BerkeleyDB::GetInstance().EnvShutdown();


    /* Elapsed Milliseconds from timer. */
    nElapsed = timer.ElapsedMilliseconds();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", nElapsed, "ms");


    /* Close the debug log file once and for all. */
    debug::shutdown();

    return 0;
}
