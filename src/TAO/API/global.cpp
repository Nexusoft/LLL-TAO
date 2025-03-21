/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/API/types/commands/assets.h>
#include <TAO/API/types/commands/ledger.h>
#include <TAO/API/types/commands/local.h>
#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/crypto.h>
#include <TAO/API/types/commands/finance.h>
#include <TAO/API/types/commands/market.h>
#include <TAO/API/types/commands/network.h>
#include <TAO/API/types/commands/profiles.h>
#include <TAO/API/types/commands/register.h>
#include <TAO/API/types/commands/sessions.h>
#include <TAO/API/types/commands/supply.h>
#include <TAO/API/types/commands/system.h>
#include <TAO/API/types/commands/tokens.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/notifications.h>

#include <Util/include/debug.h>

namespace TAO::API
{
    std::map<std::string, Base*> Commands::mapTypes;

    /*  Instantiate global instances of the API. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing API");

        /* Initialize our authentication system. */
        Authentication::Initialize();

        /* Create the API instances. */
        Commands::Register<Assets>();
        //Commands::Register<Crypto>();
        Commands::Register<Market>(config::fClient.load()); //DISABLED for -client mode
        Commands::Register<Finance>();
        Commands::Register<Invoices>();
        Commands::Register<Ledger>();
        Commands::Register<Local>();
        Commands::Register<Network>(config::fClient.load()); //DISABLED for -client mode
        Commands::Register<Names>();
        Commands::Register<Profiles>();
        Commands::Register<Register>(config::fClient.load()); //DISABLED for -client mode
        Commands::Register<Sessions>();
        Commands::Register<Supply>();
        Commands::Register<System>();
        Commands::Register<Tokens>();

        /* Import our standard objects to Register API. */
        if(!config::fClient.load())
        {
            Commands::Instance<Register>()->Import<Assets>();
            Commands::Instance<Register>()->Import<Finance>();
            Commands::Instance<Register>()->Import<Invoices>();
            Commands::Instance<Register>()->Import<Names>();
            Commands::Instance<Register>()->Import<Supply>();
        }

        /* Initialize our indexing services. */
        Indexing::Register<Market>();
        Indexing::Register<Names> ();

        /* Kick off our indexing sub-system now. */
        Indexing::Initialize();

        /* Fire up notifications processors. */
        Notifications::Initialize();

        /* API_SERVER instance */
        if((config::HasArg("-apiuser") && config::HasArg("-apipassword")) || !config::GetBoolArg("-apiauth", true))
        {
            /* Generate our config object and use correct settings. */
            LLP::Config CONFIG     = LLP::Config(LLP::GetAPIPort());
            CONFIG.ENABLE_LISTEN   = true;
            CONFIG.ENABLE_UPNP     = false;
            CONFIG.ENABLE_METERS   = config::GetBoolArg(std::string("-apimeters"), false);
            CONFIG.ENABLE_DDOS     = config::GetBoolArg(std::string("-apiddos"), true);
            CONFIG.ENABLE_MANAGER  = false;
            CONFIG.ENABLE_SSL      = config::GetBoolArg(std::string("-apissl"));
            CONFIG.ENABLE_REMOTE   = config::GetBoolArg(std::string("-apiremote"), false);
            CONFIG.REQUIRE_SSL     = config::GetBoolArg(std::string("-apisslrequired"), false);
            CONFIG.PORT_SSL        = LLP::GetAPIPort(true); //switch API port based on boolean argument
            CONFIG.MAX_INCOMING    = 128;
            CONFIG.MAX_CONNECTIONS = 128;
            CONFIG.MAX_THREADS     = config::GetArg(std::string("-apithreads"), 8);
            CONFIG.DDOS_CSCORE     = config::GetArg(std::string("-apicscore"), 5);
            CONFIG.DDOS_RSCORE     = config::GetArg(std::string("-apirscore"), 5);
            CONFIG.DDOS_TIMESPAN   = config::GetArg(std::string("-apitimespan"), 60);
            CONFIG.MANAGER_SLEEP   = 0; //this is disabled
            CONFIG.SOCKET_TIMEOUT  = config::GetArg(std::string("-apitimeout"), 30);

            /* Create the server instance. */
            LLP::API_SERVER = new LLP::Server<LLP::APINode>(CONFIG);
        }
        else
        {
            /* Output our new warning message if the API was disabled. */
            debug::notice(ANSI_COLOR_BRIGHT_RED, "API SERVER DISABLED", ANSI_COLOR_RESET);
            debug::notice(ANSI_COLOR_BRIGHT_YELLOW, "You must set apiuser=<user> and apipassword=<password> configuration.", ANSI_COLOR_RESET);
            debug::notice(ANSI_COLOR_BRIGHT_YELLOW, "If you intend to run the API server without authenticating, set apiauth=0", ANSI_COLOR_RESET);
        }
    }


    /*  Delete global instances of the API. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down API");

        /* Shutdown notifications subsystem. */
        Notifications::Shutdown();

        /* Shut down indexing and commands. */
        Indexing::Shutdown();
        Commands::Shutdown();

        /* Shutdown authentication. */
        Authentication::Shutdown();
    }
}
