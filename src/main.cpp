/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLP/include/global.h>
#include <LLP/include/network.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/legacy_miner.h>
#include <LLP/types/tritium_miner.h>

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
#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/memory.h>
#include <Util/include/runtime.h>
#include <Util/include/signals.h>
#include <Util/include/version.h>

#include <Legacy/include/ambassador.h>
#include <Legacy/types/minter.h>
#include <Legacy/wallet/db.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>

#if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)
#include <sys/types.h>
#include <sys/stat.h>
#endif

/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>   * TIME_SERVER;
}

/* Daemonize by forking the parent process*/
void Daemonize()
{
 #if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)


    pid_t pid = fork();
    if (pid < 0)
    {
        debug::error("Error: fork() returned ", pid, " errno ", errno);
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        /* generate a pid file so that we can keep track of the forked process */
        filesystem::CreatePidFile(filesystem::GetPidFile(), pid);

        /* Success: Let the parent terminate */
        exit(EXIT_SUCCESS);
    }

    pid_t sid = setsid();
    if (sid < 0)
    {
        debug::error("Error: setsid() returned ", sid, " errno %d", errno);
        exit(EXIT_FAILURE);
    }

    debug::log(0, "Nexus server starting");

    /* Set new file permissions */
    umask(0);

    /* close stdin, stderr, stdout so that the tty no longer receives output */
    if (int fdnull = open("/dev/null", O_RDWR))
    {
        dup2 (fdnull, STDIN_FILENO);
        dup2 (fdnull, STDOUT_FILENO);
        dup2 (fdnull, STDERR_FILENO);
        close(fdnull);
    }
    else
    {
        debug::error(FUNCTION, "Failed to open /dev/null");
        exit(EXIT_FAILURE);
    }
#endif
}


int main(int argc, char** argv)
{
    LLP::Server<LLP::CoreNode>* CORE_SERVER = nullptr;
    LLP::Server<LLP::RPCNode>* RPC_SERVER = nullptr;
    LLP::Server<LLP::LegacyMiner>*  LEGACY_MINING_SERVER = nullptr;
    LLP::Server<LLP::TritiumMiner>* TRITIUM_MINING_SERVER = nullptr;

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
        if (!convert::IsSwitchChar(argv[i][0]))
        {
            if(config::GetBoolArg("-api"))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    if(!debug::init())
        printf("Unable to initalize system logging\n");

    /* Log system startup now, after branching to API/RPC where appropriate */
    debug::InitializeLog(argc, argv);


    /** Run the process as Daemon RPC/LLP Server if Flagged. **/
    if (config::fDaemon)
    {
        Daemonize();
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
        return debug::error("Failed initializing wallet");


    /** Check the wallet loading for errors. **/
    uint32_t nLoadWalletRet = Legacy::Wallet::GetInstance().LoadWallet(fFirstRun);
    if (nLoadWalletRet != Legacy::DB_LOAD_OK)
    {
        if (nLoadWalletRet == Legacy::DB_CORRUPT)
            return debug::error("Failed loading wallet.dat: Wallet corrupted");
        else if (nLoadWalletRet == Legacy::DB_TOO_NEW)
            return debug::error("Failed loading wallet.dat: Wallet requires newer version of Nexus");
        else if (nLoadWalletRet == Legacy::DB_NEED_REWRITE)
            return debug::error("Wallet needed to be rewritten: restart Nexus to complete");
        else
            return debug::error("Failed loading wallet.dat");
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


    /** Initialize network resources. **/
    if (!LLP::NetworkStartup())
        debug::error("Failed initializing network resources");


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
    if(config::GetBoolArg("-mining") && config::GetBoolArg("-beta"))
    {
        LEGACY_MINING_SERVER = new LLP::Server<LLP::LegacyMiner>(
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
    else if(config::GetBoolArg("-mining") )
    {
        TRITIUM_MINING_SERVER = new LLP::Server<LLP::TritiumMiner>(
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


    /* If wallet is not encrypted, it is unlocked by default. Start stake minter now. It will run until stopped by system shutdown. */
    if (!Legacy::Wallet::GetInstance().IsCrypted())
        Legacy::StakeMinter::GetInstance().StartStakeMinter();


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
        SHUTDOWN.wait(SHUTDOWN_LOCK, []{ return config::fShutdown.load(); });
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


    /* Stop stake minter if it is running (before server shutdown). */
    Legacy::StakeMinter::GetInstance().StopStakeMinter();


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
    if(LEGACY_MINING_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Mining Server");

        LEGACY_MINING_SERVER->Shutdown();
        delete LEGACY_MINING_SERVER;
    }

    if(TRITIUM_MINING_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Mining Server");

        TRITIUM_MINING_SERVER->Shutdown();
        delete TRITIUM_MINING_SERVER;
    }


    /** After all servers shut down, clean up underlying networking resources **/
    LLP::NetworkShutdown();


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


    /* Shut down wallet database environment. */
    if (config::GetBoolArg("-flushwallet", true))
        Legacy::WalletDB::ShutdownFlushThread();

    //Legacy::BerkeleyDB::EnvShutdown();


    /* Elapsed Milliseconds from timer. */
    nElapsed = timer.ElapsedMilliseconds();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", nElapsed, "ms");


    /* Close the debug log file once and for all. */
    debug::shutdown();

    return 0;
}
