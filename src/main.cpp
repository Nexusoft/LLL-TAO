/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>
#include <LLP/include/port.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/include/lisp.h>
#include <LLP/include/port.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/cmd.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/dispatch.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/signals.h>
#include <Util/include/daemon.h>

#include <Legacy/include/ambassador.h>
#include <Legacy/include/global.h>
#include <Legacy/wallet/wallet.h>

#ifndef WIN32
#include <sys/resource.h>
#endif


/** Startup
 *
 *  Wrap all our initialization logic here that needs to run in the background after startup.
 *
 **/
void Startup()
{
    /* Add our connections from commandline. */
    LLP::MakeConnections<LLP::TimeNode>   (LLP::TIME_SERVER);
    LLP::MakeConnections<LLP::TritiumNode>(LLP::TRITIUM_SERVER);

    /* Run our autologin scripts now. */
    if(config::GetBoolArg("-autocreate", false) || config::GetBoolArg("-autologin", false))
    {
        /* Check that we are in single-user mode. */
        if(config::fMultiuser.load())
        {
            /* Output our new warning message if the API was disabled. */
            debug::log(0, ANSI_COLOR_BRIGHT_RED, "-autocreate and -autologin DISABLED", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "You cannot use -multiuser=1 with -auto(type) arguments.", ANSI_COLOR_RESET);

            return;
        }

        /* Create a JSON encoding and call the main API endpoint. */
        encoding::json jParams =
        {
            { "username", config::GetArg("-username", "") },
            { "password", config::GetArg("-password", "") },
            { "pin",      config::GetArg("-pin", "")      }
        };

        /* Handle for -autocreate if specified. */
        std::string strCreate = "create/master";
        if(config::GetBoolArg("-autocreate", false))
        {
            try { TAO::API::Commands::Invoke("profiles", strCreate, jParams); }
            catch(const TAO::API::Exception e){ debug::notice(FUNCTION, "::autocreate:", e.what()); }
        }

        /* Handle for -autologin if specified. */
        if(config::GetBoolArg("-autologin", false))
        {
            try
            {
                /* Create our local session first. */
                std::string strLogin = "create/local";
                TAO::API::Commands::Invoke("sessions", strLogin, jParams);

                /* Wait for dynamic indexing services. */
                std::string strStatus = "status/local";
                while(!config::fShutdown.load())
                {
                    /* Check our current status against indexing services. */
                    const encoding::json jStatus =
                        TAO::API::Commands::Invoke("sessions", strStatus, jParams);

                    /* Break once we have indexed sigchain. */
                    if(!jStatus["indexing"].get<bool>())
                        break;
                }

                /* Create a JSON encoding and call the main API endpoint. */
                encoding::json jUnlock =
                {
                    { "pin",  config::GetArg("-pin", "") },
                    { "notifications",              "1" },
                    { "mining",                     "1" },
                    { "staking",                    "1" }
                };

                /* Unlock our local session now. */
                std::string strUnlock = "unlock/local";
                TAO::API::Commands::Invoke("sessions", strUnlock, jUnlock);
            }
            catch(const TAO::API::Exception e){ debug::notice(FUNCTION, "::autologin: ", e.what()); }
        }
    }
}


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


    /* Initalize the debug logger. */
    debug::Initialize();


    /* Handle Commandline switch */
    for(int i = 1; i < argc; ++i)
    {
        /* Handle for commandline API/RPC */
        if(!convert::IsSwitchChar(argv[i][0]))
        {
            /* As a helpful shortcut, if the method name includes a "/" then we will assume it is meant for the API. */
            const std::string strEndpoint = std::string(argv[i]);

            /* Handle for API if symbol detected. */
            if(strEndpoint.find('/') != strEndpoint.npos || config::GetBoolArg(std::string("-api")))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    /* Log the startup information now. */
    debug::LogStartup(argc, argv);


    /* Run the process as Daemon RPC/LLP Server if Flagged. */
    if(config::fDaemon)
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


    /* Check for failures or shutdown. */
    bool fFailed = config::fShutdown.load();
    if(!fFailed)
    {
        /* Initialize LLD. */
        LLD::Initialize();


        /* Initialize dispatch relays. */
        TAO::Ledger::Dispatch::Initialize();


        /* Initialize ChainState. */
        TAO::Ledger::ChainState::Initialize();


        /* Run our LLD indexing operations. */
        LLD::Indexing();


        /* Initialize Legacy Environment. */
        if(!Legacy::Initialize())
        {
            config::fShutdown.store(true);
            fFailed = true;
        }


        /* Initialize the Lower Level Protocol. */
        LLP::Initialize();


        /* Startup performance metric. */
        debug::log(0, FUNCTION, "Started up in ", timer.ElapsedMilliseconds(), "ms");


        /* Set the initialized flags. */
        config::fInitialized.store(true);


        /* Kick off our startup thread for post-startup processing. */
        std::thread tStartup = std::thread(Startup);


        /* Initialize generator thread. */
        std::thread thread;
        if(config::fHybrid.load())
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


        /* Wait for our startup thread to finish. */
        tStartup.join();


        /* Stop stake minter if running. Minter ignores request if not running, so safe to just call both */
        TAO::Ledger::StakeMinter::GetInstance().Stop();


        /* Wait for the private condition. */
        if(config::fHybrid.load())
        {
            TAO::Ledger::PRIVATE_CONDITION.notify_all();
            thread.join();
        }


        /* Shutdown dispatch. */
        TAO::Ledger::Dispatch::Shutdown();
    }


    /* Shutdown metrics. */
    timer.Reset();


    /* Release network triggers. */
    LLP::Release();


    /* Shutdown the API subsystems. */
    TAO::API::Shutdown();


    /* Shutdown network subsystem. */
    LLP::Shutdown();


    /* Shutdown LLL sub-systems. */
    LLD::Shutdown();


    /* We check failed here as wallet could have cause failed startup via the fShutdown flag. */
    if(!fFailed)
        Legacy::Shutdown();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", timer.ElapsedMilliseconds(), "ms");


    /* Close the debug log file once and for all. */
    debug::Shutdown();


    return 0;
}
