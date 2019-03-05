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

#include <cstdint>
#include <string>
#include <vector>

#if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)
#include <sys/types.h>
#include <sys/stat.h>
#endif


namespace LLP
{
    /* Declare the Global LLP Instances. */
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>   * TIME_SERVER;


    /** CreateMiningServer
     *
     *  Helper for creating Mining Servers.
     *
     *  @return Returns a templated mining server.
     *
     **/
    template <class ProtocolType>
    Server<ProtocolType> *CreateMiningServer()
    {
        uint16_t port = static_cast<uint16_t>(config::GetArg(std::string("-miningport"), config::fTestNet ? 8325 : 9325));
        uint16_t threads = static_cast<uint16_t>(config::GetArg(std::string("-threads"), 10));
        uint32_t timeout = 30;
        bool f_ddos = false;
        uint32_t c_score = 0;
        uint32_t r_score = 0;
        uint32_t timespan = 60;
        bool f_listen = config::GetBoolArg(std::string("-listen"), true);
        bool f_meter = false;
        bool f_manager = false;


        return new Server<ProtocolType>(
            port,
            threads,
            timeout,
            f_ddos,
            c_score,
            r_score,
            timespan,
            f_listen,
            f_meter,
            f_manager);
    }


    template <class ProtocolType>
    void MakeConnections(Server<ProtocolType> *pServer)
    {
        /* -connect means try to establish a connection first. */
        if(config::mapMultiArgs["-connect"].size() > 0)
        {
            /* Add connections and resolve potential DNS lookups. */
            for(const auto& node : config::mapMultiArgs["-connect"])
                pServer->AddConnection(node, pServer->GetPort(), true);
        }

        /* -addnode means add to address manager and let it make connections. */
        if(config::mapMultiArgs["-addnode"].size() > 0)
        {
            /* Add nodes and resolve potential DNS lookups. */
            for(const auto& node : config::mapMultiArgs["-addnode"])
                pServer->AddNode(node, pServer->GetPort(), true);
        }
    }


    /** CreateTAOServer
     *
     *  Helper for creating Legacy/Tritium Servers.
     *
     *  we can change the name of this if we want since Legacy is not a part of
     *  the TAO framework. I just think it's a future proof name :)
     *
     *  @param[in] port The unique port for the server type
     *
     *  @return Returns a templated server.
     *
     **/
    template <class ProtocolType>
    Server<ProtocolType>* CreateTAOServer(uint16_t port)
    {
        /* Create the new server object. */
        return new Server<ProtocolType>(

            /* The port this server listens on. */
            port,

            /* The total data I/O threads. */
            static_cast<uint16_t>(config::GetArg(std::string("-threads"), 10),

            /* The timeout value (default: 30 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-timeout"), 30),

            /* The DDOS if enabled. */
            config::GetBoolArg(std::string("-ddos"), false),

            /* The connection score (total connections per second). */
            static_cast<uint32_t>(config::GetArg(std::string("-cscore"), 1),

            /* The request score (total packets per second.) */
            static_cast<uint32_t>(config::GetArg(std::string("-rscore"), 50),

            /* The DDOS moving average timespan (default: 60 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-timespan"), 60),

            /* Flag to determine if server should listen. */
            config::GetBoolArg(std::string("-listen"), true),

            /* Flag to determine if meters should be active. */
            config::GetBoolArg(std::string("-meters"), false),

            /* Flag to determine if the connection manager should try new connections. */
            config::GetBoolArg(std::string("-manager"), true));
    }


    /** ShutdownServer
     *
     *  Performs a shutdown and cleanup of resources on a server if it exists.
     *
     *  pServer The potential server.
     *
     **/
    template <class ProtocolType>
    void ShutdownServer(Server<ProtocolType> *pServer)
    {
        if(pServer)
        {
            pServer->Shutdown();
            delete pServer;

            debug::log(0, FUNCTION, ProtocolType::Name());
        }
    }
}

/** Daemonize
 *
 *  Daemonize by forking the parent process
 *
 **/
void Daemonize()
{
 #if !defined(WIN32) && !defined(QT_GUI) && !defined(NO_DAEMON)


    pid_t pid = fork();
    if (pid < 0)
    {
        debug::error(FUNCTION, "fork() returned ", pid, " errno ", errno);
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
        debug::error(FUNCTION, "setsid() returned ", sid, " errno %d", errno);
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


    /* Read the configuration file. Pass argc and argv for possible -datadir setting */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs, argc, argv);


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();


    /* Handle Commandline switch */
    for (int i = 1; i < argc; ++i)
    {
        if (!convert::IsSwitchChar(argv[i][0]))
        {
            if(config::GetBoolArg(std::string("-api")))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    /* Initialize system logging. */
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
    if(config::GetBoolArg(std::string("-rescan")))
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
        config::GetBoolArg(std::string("-unified"), false),
        config::GetBoolArg(std::string("-meters"), false),
        config::GetBoolArg(std::string("-manager"), true),
        30000);


    /** Handle the beta server. */
    if(!config::GetBoolArg(std::string("-beta")))
    {
        /* Get the port for Tritium Server. */
        port = static_cast<uint16_t>(config::GetArg(std::string("-port"), config::fTestNet ? 8888 : 9888));

        /* Initialize the Tritium Server. */
        LLP::TRITIUM_SERVER = LLP::CreateTAOServer<LLP::TritiumNode>(port);

        /* Handle Manual Connections from Command Line, if there are any. */
        LLP::MakeConnections<LLP::TritiumNode>(LLP::TRITIUM_SERVER);
    }
    else
    {
        /* Get the port for Legacy Server. */
        port = static_cast<uint16_t>(config::GetArg(std::string("-port"), config::fTestNet ? 8323 : 9323));

        /* Initialize the Legacy Server. */
        LLP::LEGACY_SERVER = LLP::CreateTAOServer<LLP::LegacyNode>(port);

        /* Handle Manual Connections from Command Line, if there are any. */
        LLP::MakeConnections<LLP::LegacyNode>(LLP::LEGACY_SERVER);
    }


    /* Get the port for the Core API Server. */
    port = static_cast<uint16_t>(config::GetArg(std::string("-apiport"), 8080));

    /* Create the Core API Server. */
    CORE_SERVER = new LLP::Server<LLP::CoreNode>(
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
    port = static_cast<uint16_t>(config::GetArg(std::string("-rpcport"), config::fTestNet? 8336 : 9336));

    /* Set up RPC server */
    RPC_SERVER = new LLP::Server<LLP::RPCNode>(
        port,
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
    if(config::GetBoolArg(std::string("-mining")))
    {
      if(config::GetBoolArg(std::string("-beta")))
          LEGACY_MINING_SERVER  = LLP::CreateMiningServer<LLP::LegacyMiner>();
      else
          TRITIUM_MINING_SERVER = LLP::CreateMiningServer<LLP::TritiumMiner>();
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
    Legacy::StakeMinter::GetInstance().StopStakeMinter();

    /* Shutdown the time server and its subsystems. */
    LLP::ShutdownServer<LLP::TimeNode>(LLP::TIME_SERVER);

    /* Shutdown the tritium server and its subsystems. */
    LLP::ShutdownServer<LLP::TritiumNode>(LLP::TRITIUM_SERVER);

    /* Shutdown the legacy server and its subsystems. */
    LLP::ShutdownServer<LLP::LegacyNode>(LLP::LEGACY_SERVER);

    /* Shutdown the core API server and its subsystems. */
    LLP::ShutdownServer<LLP::CoreNode>(CORE_SERVER);

    /* Shutdown the RPC server and its subsystems. */
    LLP::ShutdownServer<LLP::RPCNode>(RPC_SERVER);

    /* Shutdown the legacy mining server and its subsystems. */
    LLP::ShutdownServer<LLP::LegacyMiner>(LEGACY_MINING_SERVER);

    /* Shutdown the tritium mining server and its subsystems. */
    LLP::ShutdownServer<LLP::TritiumMiner>(TRITIUM_MINING_SERVER);

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
    if (config::GetBoolArg(std::string("-flushwallet"), true))
        Legacy::WalletDB::ShutdownFlushThread();

    Legacy::BerkeleyDB::EnvShutdown();


    /* Elapsed Milliseconds from timer. */
    nElapsed = timer.ElapsedMilliseconds();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", nElapsed, "ms");


    /* Close the debug log file once and for all. */
    debug::shutdown();

    return 0;
}
