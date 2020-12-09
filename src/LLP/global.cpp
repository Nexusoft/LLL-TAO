/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/random.h>

#include <LLP/include/global.h>
#include <LLP/include/network.h>

namespace LLP
{
    /* Declare the Global LLP Instances. */
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<TimeNode>   * TIME_SERVER;
    Server<APINode>*     API_SERVER;
    Server<RPCNode>*     RPC_SERVER;
    std::atomic<Server<Miner>*>        MINING_SERVER;
    Server<P2PNode>*  P2P_SERVER;


    /* Current session identifier. */
    const uint64_t SESSION_ID = Util::GetRand();


    /*  Initialize the LLP. */
    bool Initialize()
    {
        /* Initialize the underlying network resources such as sockets, etc */
        if(!NetworkInitialize())
            return debug::error(FUNCTION, "NetworkInitialize: Failed initializing network resources.");

        return true;
    }


    /*  Shutdown the LLP. */
    void Shutdown()
    {
        /* Shutdown the time server and its subsystems. */
        Shutdown<TimeNode>(TIME_SERVER);

        /* Shutdown the tritium server and its subsystems. */
        Shutdown<TritiumNode>(TRITIUM_SERVER);

        /* Shutdown the core API server and its subsystems. */
        Shutdown<APINode>(API_SERVER);

        /* Shutdown the RPC server and its subsystems. */
        Shutdown<RPCNode>(RPC_SERVER);

        /* Shutdown the mining server and its subsystems. */
        Shutdown<Miner>(MINING_SERVER);

        /* Shutdown the P2P server and its subsystems. */
        Shutdown<P2PNode>(P2P_SERVER);

        /* After all servers shut down, clean up underlying network resources. */
        NetworkShutdown();
    }

    /* Closes the listening sockets on all running servers. */
    void CloseListening()
    {
        if(TIME_SERVER)
            TIME_SERVER->CloseListening();

        if(TRITIUM_SERVER)
            TRITIUM_SERVER->CloseListening();

        if(API_SERVER)
            API_SERVER->CloseListening();

        if(RPC_SERVER)
            RPC_SERVER->CloseListening();

        if(MINING_SERVER)
            MINING_SERVER.load()->CloseListening();

        if(P2P_SERVER)
            P2P_SERVER->CloseListening();
    }

    /* Restarts the listening sockets on all running servers. */
    void OpenListening()
    {
        if(TIME_SERVER)
            TIME_SERVER->OpenListening();

        if(TRITIUM_SERVER)
            TRITIUM_SERVER->OpenListening();

        if(API_SERVER)
            API_SERVER->OpenListening();

        if(RPC_SERVER)
            RPC_SERVER->OpenListening();

        if(MINING_SERVER)
            MINING_SERVER.load()->OpenListening();

        if(P2P_SERVER)
            P2P_SERVER->OpenListening();
    }


    /*  Creates and returns the mining server. */
    Server<Miner>* CreateMiningServer()
    {
        LLP::ServerConfig config;

        /* The port this server listens on. */
        config.nPort = static_cast<uint16_t>(GetMiningPort());

        /* The total data I/O threads. */
        config.nMaxThreads = static_cast<uint16_t>(config::GetArg(std::string("-miningthreads"), 4));

        /* The timeout value (default: 30 seconds). */
        config.nTimeout = static_cast<uint32_t>(config::GetArg(std::string("-miningtimeout"), 30));

        /* The DDOS if enabled. */
        config.fDDOS = config::GetBoolArg(std::string("-miningddos"), false);

        /* The connection score (total connections per second). */
        config.nDDOSCScore = static_cast<uint32_t>(config::GetArg(std::string("-miningcscore"), 1));

        /* The request score (total packets per second.) */
        config.nDDOSRScore = static_cast<uint32_t>(config::GetArg(std::string("-miningrscore"), 50));

        /* The DDOS moving average timespan (default: 60 seconds). */
        config.nDDOSTimespan = static_cast<uint32_t>(config::GetArg(std::string("-miningtimespan"), 60));

        /* Mining server should always listen */
        config.fListen = true;

        /* Mining server should always allow remote connections */
        config.fRemote = true;

        /* Flag to determine if meters should be active. */
        config.fMeter = config::GetBoolArg(std::string("-meters"), false);

        /* Mining server should never make outgoing connections. */
        config.fMeter = false;

        /* Create the mining server object. */
        return new Server<Miner>(config);
    }


    /* Helper for creating the Time server. */
    Server<TimeNode>* CreateTimeServer()
    {
        /* Startup the time server. */
        LLP::ServerConfig config;
        
        /* Time server port */
        config.nPort = GetTimePort();
        
        /* Always use 10 threads */
        config.nMaxThreads = 10;
        
        /* Timeout of 10s */
        config.nTimeout = 10;
        
        /* Always use DDOS for time server */
        config.fDDOS = true;
        
        /* The connection score, total connections per second. */
        config.nDDOSCScore = 1;
        
        /* The request score, total requests per second. */
        config.nDDOSRScore = 10;
        
        /* Calculate the rscore and cscore over a 10s moving average */
        config.nDDOSTimespan = 10;
        
        /* Listen for incoming connections if unified is specified in config */
        config.fListen = config::fClient.load() ? false : config::GetBoolArg(std::string("-unified"), false);
        
        /* Always allow remote connections */
        config.fRemote = true;
        
        /* Turn on meters */
        config.fMeter = config::GetBoolArg(std::string("-meters"), false);
        
        /* Always use address manager */
        config.fManager = true;
        
        /* Make new connections every 60s */
        config.nManagerInterval = 60000;

        /* Max incoming connections */
        config.nMaxIncoming = static_cast<uint32_t>(config::GetArg(std::string("-maxincoming"), 84));

        /* Max connections */
        config.nMaxConnections = static_cast<uint32_t>(config::GetArg(std::string("-maxconnections"), 100));

        /* Instantiate the time server */
        return new LLP::Server<LLP::TimeNode>(config);
    }

    /* Helper for creating the RPC server. */
    Server<RPCNode>* CreateRPCServer()
    {
        LLP::ServerConfig config;
        
        /* Port for the RPC server */
        config.nPort = static_cast<uint16_t>(config::GetArg(std::string("-rpcport"), config::fTestNet.load() ? TESTNET_RPC_PORT : MAINNET_RPC_PORT));

        /* SSL port for RPC server */
        config.nSSLPort = static_cast<uint16_t>(config::GetArg(std::string("-rpcsslport"), config::fTestNet.load() ? TESTNET_RPC_SSL_PORT : MAINNET_RPC_SSL_PORT));
        
        /* Max threads based on config */
        config.nMaxThreads = static_cast<uint16_t>(config::GetArg(std::string("-rpcthreads"), 4));
        
        /* 30s timeout */
        config.nTimeout = 30;

        /* Always use DDOS for RPC server */
        config.fDDOS = true;

        /* The connection score, total connections per second. Default to 5*/
        config.nDDOSCScore = static_cast<uint32_t>(config::GetArg(std::string("-rpccscore"), 5));

        /* The request score, total requests per second. Default is 5*/
        config.nDDOSRScore = static_cast<uint32_t>(config::GetArg(std::string("-rpcrscore"), 5));

        /* Calculate the rscore and cscore over a 60s moving average or as configured */
        config.nDDOSTimespan = static_cast<uint32_t>(config::GetArg(std::string("-rpctimespan"), 60));

        /* Always listen for incoming connections */
        config.fListen = true;

        /* Allow remote connections based on config */
        config.fRemote = config::GetBoolArg(std::string("-rpcremote"), false);

        /* No meters */
        config.fMeter = false;

        /* no manager */
        config.fManager = false;
        
        /* Enable SSL if configured */
        config.fSSL = config::GetBoolArg(std::string("-rpcssl")) || config::GetBoolArg(std::string("-rpcsslrequired"), false);

        /* Require SSL if configured */
        config.fSSLRequired = config::GetBoolArg(std::string("-rpcsslrequired"), false);

        /* Instantiate the RPC server */
        return new LLP::Server<LLP::RPCNode>(config);
    }


    /* Helper for creating the API server. */
    Server<APINode>* CreateAPIServer()
    {
        LLP::ServerConfig config;
        
        /* Port for the API server */
        config.nPort = static_cast<uint16_t>(config::GetArg(std::string("-apiport"), config::fTestNet.load() ? TESTNET_API_PORT : MAINNET_API_PORT));
        
        /* SSL port for API server */
        config.nSSLPort = static_cast<uint16_t>(config::GetArg(std::string("-apisslport"), config::fTestNet.load() ? TESTNET_API_SSL_PORT : MAINNET_API_SSL_PORT));
        
        /* Max threads based on config */
        config.nMaxThreads = static_cast<uint16_t>(config::GetArg(std::string("-apithreads"), 10));
        
        /* Timeout based on config, 30s default */
        config.nTimeout = static_cast<uint32_t>(config::GetArg(std::string("-apitimeout"), 30));

        /* Always use DDOS for API server */
        config.fDDOS = true;

        /* The connection score, total connections per second. Default to 5*/
        config.nDDOSCScore = static_cast<uint32_t>(config::GetArg(std::string("-apicscore"), 5));

        /* The request score, total requests per second. Default is 5*/
        config.nDDOSRScore = static_cast<uint32_t>(config::GetArg(std::string("-apirscore"), 5));

        /* Calculate the rscore and cscore over a 60s moving average or as configured */
        config.nDDOSTimespan = static_cast<uint32_t>(config::GetArg(std::string("-apitimespan"), 60));

        /* Always listen for incoming connections */
        config.fListen = true;

        /* Allow remote connections based on config */
        config.fRemote = config::GetBoolArg(std::string("-apiremote"), false);

        /* No meters */
        config.fMeter = false;

        /* No manager , not required for API as connections are ephemeral*/
        config.fManager = false;
        
        /* Enable SSL if configured */
        config.fSSL = config::GetBoolArg(std::string("-apissl")) || config::GetBoolArg(std::string("-apisslrequired"), false);

        /* Require SSL if configured */
        config.fSSLRequired = config::GetBoolArg(std::string("-apisslrequired"), false);

        /* Instantiate the API server */
        return new LLP::Server<LLP::APINode>(config);
    }


}
