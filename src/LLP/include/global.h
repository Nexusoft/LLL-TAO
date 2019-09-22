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
#include <LLP/types/legacy.h>
#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/templates/server.h>

#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

namespace LLP
{
    extern Server<LegacyNode>*   LEGACY_SERVER;
    extern Server<TritiumNode>*  TRITIUM_SERVER;
    extern Server<TimeNode>*     TIME_SERVER;
    extern Server<APINode>*      API_SERVER;
    extern Server<RPCNode>*      RPC_SERVER;
    extern Server<Miner>*        MINING_SERVER;


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


    /** CreateMiningServer
     *
     *  Creates and returns the mining server.
     *
     **/
    Server<Miner>* CreateMiningServer();


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
    Server<ProtocolType>* CreateTAOServer(uint16_t nPort)
    {
        /* Create the new server object. */
        return new Server<ProtocolType>(

            /* The port this server listens on. */
            nPort,

            /* The total data I/O threads. */
            static_cast<uint16_t>(config::GetArg(std::string("-threads"), 8)),

            /* The timeout value (default: 30 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-timeout"), 120)),

            /* The DDOS if enabled. */
            config::GetBoolArg(std::string("-ddos"), true),

            /* The connection score (total connections per second). */
            static_cast<uint32_t>(config::GetArg(std::string("-cscore"), 1)),

            /* The request score (total packets per second.) */
            static_cast<uint32_t>(config::GetArg(std::string("-rscore"), 2000)),

            /* The DDOS moving average timespan (default: 20 seconds). */
            static_cast<uint32_t>(config::GetArg(std::string("-timespan"), 20)),

            /* Flag to determine if server should listen. */
            config::GetBoolArg(std::string("-listen"), true),

            /* Flag to determine if meters should be active. */
            config::GetBoolArg(std::string("-meters"), false),

            /* Flag to determine if the connection manager should try new connections. */
            config::GetBoolArg(std::string("-manager"), true)

        );
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
