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

namespace LLP
{
    /* Declare the Global LLP Instances. */
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
    Server<TimeNode>   * TIME_SERVER;
    Server<APINode>*     API_SERVER;
    Server<RPCNode>*     RPC_SERVER;
    Server<Miner>*       MINING_SERVER;


    /* Current session identifier. */
    const uint64_t SESSION_ID = LLC::GetRand();


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

        /* Shutdown the legacy server and its subsystems. */
        Shutdown<LegacyNode>(LEGACY_SERVER);

        /* Shutdown the core API server and its subsystems. */
        Shutdown<APINode>(API_SERVER);

        /* Shutdown the RPC server and its subsystems. */
        Shutdown<RPCNode>(RPC_SERVER);

        /* Shutdown the mining server and its subsystems. */
        Shutdown<Miner>(MINING_SERVER);

        /* After all servers shut down, clean up underlying network resources. */
        NetworkShutdown();
    }


    /*  Creates and returns the mining server. */
    Server<Miner>* CreateMiningServer()
    {

        /* Create the mining server object. */
        return new Server<Miner>(

            /* The port this server listens on. */
            static_cast<uint16_t>(config::GetArg(std::string("-miningport"), config::fTestNet.load() ? TESTNET_MINING_LLP_PORT : MAINNET_MINING_LLP_PORT)),

            /* The total data I/O threads. */
            static_cast<uint16_t>(config::GetArg(std::string("-miningthreads"), 4)),

            /* The timeout value (default: 30 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-miningtimeout"), 30)),

            /* The DDOS if enabled. */
            config::GetBoolArg(std::string("-miningddos"), false),

            /* The connection score (total connections per second). */
            static_cast<uint32_t>(config::GetArg(std::string("-miningcscore"), 1)),

            /* The request score (total packets per second.) */
            static_cast<uint32_t>(config::GetArg(std::string("-miningrscore"), 50)),

            /* The DDOS moving average timespan (default: 60 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-miningtimespan"), 60)),

            /* Mining server should always listen */
            true,

            /* Mining server should always allow remote connections */
            true,

            /* Flag to determine if meters should be active. */
            config::GetBoolArg(std::string("-meters"), false),

            /* Mining server should never make outgoing connections. */
            false

        );
    }

}
