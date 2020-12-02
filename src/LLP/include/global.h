/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GLOBAL_H
#define NEXUS_LLP_INCLUDE_GLOBAL_H

#include <LLP/include/port.h>
#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/templates/server.h>

#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/p2p.h>

namespace LLP
{
    extern Server<TritiumNode>*  TRITIUM_SERVER;
    extern Server<TimeNode>*     TIME_SERVER;
    extern Server<APINode>*      API_SERVER;
    extern Server<RPCNode>*      RPC_SERVER;
    extern std::atomic<Server<Miner>*>        MINING_SERVER;
    extern Server<P2PNode>*      P2P_SERVER;


    /** Current session identifier. **/
    const extern uint64_t SESSION_ID;


    /** Initialize
     *
     *  Initialize the LLP.
     *
     **/
    bool Initialize();


    /** Shutdown
     *
     *  Shutdown the LLP.
     *
     **/
    void Shutdown();

    /** CloseListening
     *
     *  Closes the listening sockets on all running servers.
     *
     **/
    void CloseListening();


    /** OpenListening
     *
     *  Restarts the listening sockets on all running servers.
     *
     **/
    void OpenListening();


    /** CreateMiningServer
     *
     *  Creates and returns the mining server.
     *
     **/
    Server<Miner>* CreateMiningServer();


    /** CreateTimeServer
     *
     *  Helper for creating the Time server.
     *
     *  @return Returns the server.
     *
     **/
    Server<TimeNode>* CreateTimeServer();


    /** CreateRPCServer
     *
     *  Helper for creating the RPC server.
     *
     *  @return Returns a new RPC server.
     *
     **/
    Server<RPCNode>* CreateRPCServer();


    /** CreateAPIServer
     *
     *  Helper for creating the API server.
     *
     *  @return Returns a new API server.
     *
     **/
    Server<APINode>* CreateAPIServer();


    /** MakeConnections
     *
     *  Makes connections from -connect and -addnode arguments for the specified server.
     *
     *  @param[in] pServer The server to make connections with.
     *
     **/
    template <class ProtocolType>
    void MakeConnections(Server<ProtocolType> *pServer)
    {
        /* -connect means try to establish a connection first. */
        if(config::mapMultiArgs["-connect"].size() > 0)
        {
            /* Add connections and resolve potential DNS lookups. */
            for(const auto& address : config::mapMultiArgs["-connect"])
            {
                /* Flag indicating connection was successful */
                bool fConnected = false;
                
                /* First attempt SSL if configured */
                if(pServer->SSLEnabled())
                   fConnected = pServer->AddConnection(address, pServer->GetPort(true), true, true);

                /* If SSL connection failed or was not attempted and SSL is not required, attempt on the non-SSL port */
                if(!fConnected && !pServer->SSLRequired())
                    fConnected = pServer->AddConnection(address, pServer->GetPort(false), false, true);
            }
        }

        /* -addnode means add to address manager and let it make connections. */
        if(config::mapMultiArgs["-addnode"].size() > 0)
        {
            /* Add nodes and resolve potential DNS lookups. */
            for(const auto& node : config::mapMultiArgs["-addnode"])
                pServer->AddNode(node, true);
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
     *  @param[in] port The SSL port for the server type
     *
     *  @return Returns a templated server.
     *
     **/
    template <class ProtocolType>
    Server<ProtocolType>* CreateTAOServer(uint16_t nPort, uint16_t nSSLPort)
    {
        LLP::ServerConfig config;

        /* The port this server listens on. */
        config.nPort =  nPort;

        /* The SSL port this server listens on. */
        config.nSSLPort =  nSSLPort;

        /* The total data I/O threads. */
        config.nMaxThreads = static_cast<uint16_t>(config::GetArg(std::string("-threads"), 8));

        /* The timeout value (default: 30 seconds). */
        config.nTimeout = static_cast<uint32_t>(config::GetArg(std::string("-timeout"), 120));

        /* The DDOS if enabled. */
        config.fDDOS = config::GetBoolArg(std::string("-ddos"), false);

        /* The connection score (total connections per second). */
        config.nDDOSCScore = static_cast<uint32_t>(config::GetArg(std::string("-cscore"), 1));

        /* The request score (total packets per second.) */
        config.nDDOSRScore = static_cast<uint32_t>(config::GetArg(std::string("-rscore"), 2000));

        /* The DDOS moving average timespan (default: 20 seconds). */
        config.nDDOSTimespan = static_cast<uint32_t>(config::GetArg(std::string("-timespan"), 20));

        /* Flag to determine if server should listen. */
        config.fListen = (config::fClient.load() ? false : config::GetBoolArg(std::string("-listen"), true));

        /* Flag to determine if server should allow remote connections. */
        config.fRemote = true;

        /* Flag to determine if meters should be active. */
        config.fMeter = config::GetBoolArg(std::string("-meters"), false);

        /* Flag to determine if the connection manager should try new connections. */
        config.fManager = config::GetBoolArg(std::string("-manager"), true);
        
        /* Default sleep */
        config.nManagerInterval = 1000;
        
        /* Enable SSL if configured */
        config.fSSL = config::GetBoolArg(std::string("-ssl"), false) || config::GetBoolArg(std::string("-sslrequired"), false);

        /* Require SSL if configured */
        config.fSSLRequired = config::GetBoolArg(std::string("-sslrequired"), false);

        /* Max outgoing connections */
        config.nMaxIncoming = static_cast<uint32_t>(config::GetArg(std::string("-maxincoming"), 84));

        /* Max connections */
        config.nMaxConnections = static_cast<uint32_t>(config::GetArg(std::string("-maxconnections"), 100));

        /* Create the new server object. */
        return new Server<ProtocolType>(config);
    }


    /** CreateP2PServer
     *
     *  Helper for creating P2P Servers.
     *
     *  @param[in] port The unique port for the server type
     *  @param[in] port The SSL port for the server type
     *
     *  @return Returns a templated server.
     *
     **/
    template <class ProtocolType>
    Server<ProtocolType>* CreateP2PServer(uint16_t nPort, uint16_t nSSLPort)
    {
        LLP::ServerConfig config;

        /* The port this server listens on. */
        config.nPort =  nPort;

        /* The SSL port this server listens on. */
        config.nSSLPort =  nSSLPort;

        /* The total data I/O threads. */
        config.nMaxThreads = static_cast<uint16_t>(config::GetArg(std::string("-threads"), 8));

        /* The timeout value (default: 30 seconds). */
        config.nTimeout = static_cast<uint32_t>(config::GetArg(std::string("-timeout"), 120));

        /* The DDOS if enabled. */
        config.fDDOS = config::GetBoolArg(std::string("-ddos"), false);

        /* The connection score (total connections per second). */
        config.nDDOSCScore = static_cast<uint32_t>(config::GetArg(std::string("-cscore"), 1));

        /* The request score (total packets per second.) */
        config.nDDOSRScore = static_cast<uint32_t>(config::GetArg(std::string("-rscore"), 2000));

        /* The DDOS moving average timespan (default: 20 seconds). */
        config.nDDOSTimespan = static_cast<uint32_t>(config::GetArg(std::string("-timespan"), 20));

        /* Flag to determine if server should listen. */
        config.fListen = true;

        /* Flag to determine if server should allow remote connections. */
        config.fRemote = true;

        /* Flag to determine if meters should be active. */
        config.fMeter = config::GetBoolArg(std::string("-meters"), false);

        /* Never use connection manager */
        config.fManager = false;
        
        /* Enable SSL if configured */
        config.fSSL = config::GetBoolArg(std::string("-p2pssl"), false) || config::GetBoolArg(std::string("-p2psslrequired"), false);

        /* Require SSL if configured */
        config.fSSLRequired = config::GetBoolArg(std::string("-p2psslrequired"), false);

        /* Create the new server object. */
        return new Server<ProtocolType>(config);

    }


    /** Shutdown
     *
     *  Performs a shutdown and cleanup of resources on a server if it exists.
     *
     *  pServer The potential server.
     *
     **/
    template <class ProtocolType>
    void Shutdown(Server<ProtocolType> *pServer)
    {
        if(pServer)
        {
            pServer->Shutdown();
            delete pServer;

            debug::log(0, FUNCTION, ProtocolType::Name());
        }
    }

}

#endif
