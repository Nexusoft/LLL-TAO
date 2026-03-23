/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/server.h>
#include <LLP/templates/data.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/static.h>
#include <LLP/include/network.h>

#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/filenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/lookup.h>
#include <LLP/types/stateless_miner_connection.h>

#include <LLP/include/trust_address.h>
#include <LLP/include/auto_cooldown_manager.h>
#include <LLP/include/node_cache.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/node_session_registry.h>
#include <LLP/include/miner_push_dispatcher.h>
#include <LLP/include/mining_constants.h>

#include <Util/include/args.h>
#include <Util/include/signals.h>
#include <Util/include/version.h>

#include <LLP/include/permissions.h>
#include <functional>
#include <numeric>
#include <type_traits>

#include <openssl/ssl.h>

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif


namespace LLP
{
    /* Type trait to detect if a protocol type supports channel mining notifications.
     * This is used with if constexpr to only compile mining-specific code for types that support it. */
    template <typename T, typename = void>
    struct has_mining_notifications : std::false_type {};
    
    template <typename T>
    struct has_mining_notifications<T, std::void_t<
        decltype(std::declval<T>().GetContext()),
        decltype(std::declval<T>().SendChannelNotification())
    >> : std::true_type {};
    
    template <typename T>
    inline constexpr bool has_mining_notifications_v = has_mining_notifications<T>::value;

    template <typename T>
    inline constexpr bool is_miner_protocol_v =
        std::is_same_v<T, Miner> || std::is_same_v<T, StatelessMinerConnection>;
}


namespace LLP
{
    /** Constructor **/
    template <class ProtocolType>
    Server<ProtocolType>::Server(const Config& config)
    : CONFIG            (config)
    , DDOS_MAP          (new std::map<BaseAddress, DDOS_Filter*>())
    , hListenBase       (-1, -1)
    , hListenSSL        (-1, -1)
    , pAddressManager   (nullptr)
    , THREADS_DATA      ( )
    , THREAD_LISTEN     ( )
    , THREAD_UPNP       ( )
    , THREAD_METER      ( )
    , THREAD_MANAGER    ( )
    {
        /* -banned means do not let node connect */
        if(config::HasArg("-banned"))
        {
            RECURSIVE(config::ARGS_MUTEX);

            /* Add connections and resolve potential DNS lookups. */
            for(const auto& rAddress : config::mapMultiArgs["-banned"])
            {
                /* Add banned address to DDOS map. */
                DDOS_MAP->insert(std::make_pair(BaseAddress(rAddress), new DDOS_Filter(CONFIG.DDOS_TIMESPAN)));
                DDOS_MAP->at(rAddress)->nBanTimestamp.store(std::numeric_limits<uint64_t>::max());

                debug::notice(FUNCTION, "-banned commandline set for ", rAddress);
            }
        }

        /* Add the individual data threads to the vector that will be holding their state. */
        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
        {
            THREADS_DATA.push_back(new DataThread<ProtocolType>
            (
                nIndex,
                CONFIG.ENABLE_DDOS,
                CONFIG.DDOS_RSCORE,
                CONFIG.DDOS_CSCORE,
                CONFIG.SOCKET_TIMEOUT,
                CONFIG.ENABLE_METERS
            ));
        }

        /* Initialize the address manager. */
        if(CONFIG.ENABLE_MANAGER)
        {
            pAddressManager = new AddressManager(CONFIG.PORT_BASE);
            if(!pAddressManager)
                debug::warning(FUNCTION, "memory allocation failed for manager on port ", CONFIG.PORT_BASE);

            THREAD_MANAGER = std::thread((std::bind(&Server::Manager, this)));
        }

        /* Open listeners if enabled. */
        if(CONFIG.ENABLE_LISTEN)
        {
            /* Open listening sockets. */
            OpenListening();

            /* We don't create plaintext listener if SSL is required. */
            if(!CONFIG.REQUIRE_SSL)
            {
                /* Create our listening thread now. */
                THREAD_LISTEN.push_back
                (
                    std::thread
                    (
                        std::bind
                        (
                            &Server::ListeningThread,
                            this,
                            true, //IPv4
                            false //No SSL
                        )
                    )
                );

                /* Initialize the UPnP thread if remote connections are allowed. */
                #ifdef USE_UPNP
                if(CONFIG.ENABLE_REMOTE && CONFIG.ENABLE_UPNP)
                {
                    THREAD_UPNP.push_back
                    (
                        std::thread
                        (
                            std::bind
                            (
                                &Server::UPnP,
                                this,
                                CONFIG.PORT_BASE
                            )
                        )
                    );
                }
                #endif
            }

            /* Add our SSL listener if enabled. */
            if(CONFIG.ENABLE_SSL)
            {
                /* Create our listening thread now. */
                THREAD_LISTEN.push_back
                (
                    std::thread
                    (
                        std::bind
                        (
                            &Server::ListeningThread,
                            this,
                            true, //IPv4
                            true //Enable SSL
                        )
                    )
                );

                /* Initialize the UPnP thread if remote connections are allowed. */
                #ifdef USE_UPNP
                if(CONFIG.ENABLE_REMOTE && CONFIG.ENABLE_UPNP)
                {
                    THREAD_UPNP.push_back
                    (
                        std::thread
                        (
                            std::bind
                            (
                                &Server::UPnP,
                                this,
                                CONFIG.PORT_SSL
                            )
                        )
                    );
                }
                #endif
            }
        }

        /* Start meters if enabled.
         * For miner protocols, always start the Meter thread (even without -meters) because
         * it drives the heartbeat template refresh that prevents miners from entering
         * degraded mode during dry spells. */
        if(CONFIG.ENABLE_METERS || is_miner_protocol_v<ProtocolType>)
            THREAD_METER = std::thread(std::bind(&Server::Meter, this));
    }


    /** Default Destructor **/
    template <class ProtocolType>
    Server<ProtocolType>::~Server()
    {
        debug::log(1, FUNCTION, "Shutting down ", Name(), " server - waiting for threads...");

        /* ── Pre-close all connection sockets BEFORE joining threads ──────────
         * Closing the socket fd causes the kernel to return POLLHUP/POLLERR on
         * the next poll() call inside DataThread::Thread(). This means the data
         * thread exits its connection-processing loop immediately (rather than
         * waiting up to 100ms for the poll timeout), sees fDestruct/fShutdown,
         * and returns — allowing DataThread::~DataThread() join to succeed fast.
         *
         * Without this, with N active miner connections, shutdown could take
         * up to N × 100ms just for poll() to naturally time out per thread.
         */
        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
        {
            if(!THREADS_DATA[nIndex])
                continue;

            const uint32_t nSize = static_cast<uint32_t>(THREADS_DATA[nIndex]->CONNECTIONS->size());
            for(uint32_t nConn = 0; nConn < nSize; ++nConn)
            {
                try
                {
                    std::shared_ptr<ProtocolType> conn = THREADS_DATA[nIndex]->CONNECTIONS->at(nConn);
                    if(conn && conn->Connected())
                        conn->Disconnect();  /* closes fd, unblocks poll() */
                }
                catch(...) {}
            }

            /* Wake the data thread's condition so it sees fDestruct/fShutdown
             * immediately after the socket close, without waiting for spurious
             * wakeup or the next poll() timeout. */
            THREADS_DATA[nIndex]->CONDITION.notify_all();
            THREADS_DATA[nIndex]->FLUSH_CONDITION.notify_all();
        }

        /* Wait for address manager. */
        debug::log(1, FUNCTION, "  Joining address manager thread...");
        if(THREAD_MANAGER.joinable())
            THREAD_MANAGER.join();
        debug::log(1, FUNCTION, "  Address manager thread joined");

        /* Wait for meter thread. */
        debug::log(1, FUNCTION, "  Joining meter thread...");
        if(THREAD_METER.joinable())
            THREAD_METER.join();
        debug::log(1, FUNCTION, "  Meter thread joined");

        /* Check all registered listening threads. */
        debug::log(1, FUNCTION, "  Joining ", THREAD_LISTEN.size(), " listening threads...");
        for(auto& THREAD : THREAD_LISTEN)
        {
            if(THREAD.joinable())
                THREAD.join();
        }
        debug::log(1, FUNCTION, "  Listening threads joined");

        /* Check all registered upnp threads. */
        debug::log(1, FUNCTION, "  Joining ", THREAD_UPNP.size(), " UPNP threads...");
        for(auto& THREAD : THREAD_UPNP)
        {
            if(THREAD.joinable())
                THREAD.join();
        }
        debug::log(1, FUNCTION, "  UPNP threads joined");

        /* Delete the data threads. */
        debug::log(1, FUNCTION, "  Deleting ", CONFIG.MAX_THREADS, " data threads...");
        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
        {
            debug::log(2, FUNCTION, "    Deleting data thread ", nIndex);
            delete THREADS_DATA[nIndex];
            THREADS_DATA[nIndex] = nullptr;
        }
        debug::log(1, FUNCTION, "  Data threads deleted");

        /* Delete the DDOS entries. */
        debug::log(1, FUNCTION, "  Deleting DDOS entries...");
        for(auto it = DDOS_MAP->begin(); it != DDOS_MAP->end(); ++it)
        {
            if(it->second)
                delete it->second;
        }
        debug::log(1, FUNCTION, "  DDOS entries deleted");

        /* Clear the address manager. */
        if(pAddressManager)
        {
            debug::log(1, FUNCTION, "  Deleting address manager...");
            delete pAddressManager;
            pAddressManager = nullptr;
            debug::log(1, FUNCTION, "  Address manager deleted");
        }

        debug::log(1, FUNCTION, Name(), " server shutdown complete");
    }


    /*  Returns the name of the protocol type of this server. */
    template <class ProtocolType>
    std::string Server<ProtocolType>::Name()
    {
        return ProtocolType::Name();
    }


    /*  Returns the port number for this Server. */
    template <class ProtocolType>
    uint16_t Server<ProtocolType>::GetPort(bool fSSL) const
    {
        return (fSSL ? CONFIG.PORT_SSL : CONFIG.PORT_BASE);
    }


    /*Returns the address manager instance for this server. */
    template <class ProtocolType>
    AddressManager* Server<ProtocolType>::GetAddressManager() const
    {
        return pAddressManager;
    }


    /*  Add a node address to the internal address manager */
    template <class ProtocolType>
    void Server<ProtocolType>::AddNode(const std::string& strAddress, bool fLookup)
    {
       /* Assemble the address from input parameters. */
       BaseAddress addr(strAddress, CONFIG.PORT_BASE, fLookup);

       /* Make sure address is valid. */
       if(!addr.IsValid())
            return;

       /* Add the address to the address manager if it exists. */
       if(pAddressManager)
          pAddressManager->AddAddress(addr, ConnectState::NEW);
    }


    /* Connect a node address to the internal server manager */
    template <class ProtocolType>
    bool Server<ProtocolType>::ConnectNode(const std::string& strAddress, std::shared_ptr<ProtocolType> &pNodeRet, bool fLookup)
    {
        /* Assemble the address from input parameters. */
        BaseAddress addrConnect(strAddress, CONFIG.PORT_BASE, fLookup);

        /* Make sure address is valid. */
        if(!addrConnect.IsValid())
             return debug::error(FUNCTION, "address ", addrConnect.ToStringIP(), " is invalid");

         /* Create new DDOS Filter if Needed. */
         if(CONFIG.ENABLE_DDOS)
         {
             /* Add new filter to map if it doesn't exist. */
             if(!DDOS_MAP->count(addrConnect))
                 DDOS_MAP->emplace(std::make_pair(addrConnect, new DDOS_Filter(CONFIG.DDOS_TIMESPAN)));

             /* DDOS Operations: Only executed when DDOS is enabled. */
             if(!addrConnect.IsLocal() && DDOS_MAP->at(addrConnect)->Banned())
                 return debug::drop(FUNCTION, "address ", addrConnect.ToStringIP(), " is banned");
         }

         /* Find a balanced Data Thread to Add Connection to. */
         int32_t nThread = FindThread();
         if(nThread < 0)
             return debug::error(FUNCTION, "no available connections for ", ProtocolType::Name());

         /* Select the proper data thread. */
         DataThread<ProtocolType> *dt = THREADS_DATA[nThread];

         /* Attempt the connection. */
         if(!dt->NewConnection(addrConnect, CONFIG.ENABLE_DDOS ? DDOS_MAP->at(addrConnect) : nullptr, false, pNodeRet))
             return debug::drop(FUNCTION, "connection attempt for address ", addrConnect.ToStringIP(), " failed");

         /* Add the address to the address manager if it exists. */
         if(pAddressManager)
             pAddressManager->AddAddress(addrConnect, ConnectState::CONNECTED);

         return true;
    }


    /* Constructs a vector of all active connections across all threads */
    template <class ProtocolType>
    std::vector<std::shared_ptr<ProtocolType>> Server<ProtocolType>::GetConnections() const
    {
        /* Iterate through threads */
        std::vector<std::shared_ptr<ProtocolType>> vConnections;
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            uint16_t nSize = static_cast<uint16_t>(THREADS_DATA[nThread]->CONNECTIONS->size());
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Get the current atomic_ptr. */
                std::shared_ptr<ProtocolType> pConnection = THREADS_DATA[nThread]->CONNECTIONS->at(nIndex);

                /* Check to see if it is null */
                if(!pConnection)
                    continue;

                /* Add it to the return vector */
                vConnections.push_back(pConnection);
            }
        }

        return vConnections;
    }


    /*  Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t Server<ProtocolType>::GetConnectionCount(const uint8_t nFlags)
    {
        /* Tally the total connections by aggregating the values for each data thread. */
        uint32_t nConnections = 0;
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
            nConnections += THREADS_DATA[nThread]->GetConnectionCount(nFlags);

        return nConnections;
    }


    /*  Broadcast channel-specific notification to subscribed miners on this lane. */
    template <class ProtocolType>
    uint32_t Server<ProtocolType>::NotifyChannelMiners(uint32_t nChannel, bool fHeartbeat)
    {
        /* Use compile-time check to only execute for protocols that support mining notifications */
        if constexpr (has_mining_notifications_v<ProtocolType>)
        {
            /* Determine lane name at compile time for clear per-lane logging */
            constexpr const char* strLane =
                std::is_same_v<ProtocolType, StatelessMinerConnection> ? "Stateless" : "Legacy";

            /* Early exit if shutdown is in progress */
            if (config::fShutdown.load())
            {
                debug::log(1, FUNCTION, "[", strLane, "] Shutdown in progress; skipping NotifyChannelMiners");
                return 0;
            }
            
            /* Validate channel */
            if (nChannel != 1 && nChannel != 2)
            {
                debug::error(FUNCTION, "[", strLane, "] Invalid channel: ", nChannel);
                return 0;
            }
            
            const std::string strChannelName = (nChannel == 1) ? "Prime" : "Hash";
            debug::log(1, FUNCTION, "[", strLane, "][", strChannelName, "] Broadcasting block notification",
                       (fHeartbeat ? " [HEARTBEAT]" : ""));
            
            /* Get all connections */
            std::vector<std::shared_ptr<ProtocolType>> vConnections = GetConnections();
            
            if (vConnections.empty())
            {
                debug::log(1, FUNCTION, "[", strLane, "][", strChannelName, "] No active miners (0 notified)");
                return 0;
            }
            
            uint32_t nNotified = 0;
            uint32_t nSkippedWrongChannel = 0;
            uint32_t nSkippedUnsubscribed = 0;
            
            /* SERVER-SIDE FILTERING: Only notify miners subscribed to the matching channel */
            for (auto pConnection : vConnections)
            {
                /* CRITICAL: Skip null connections WITHOUT any counting */
                if (!pConnection)
                    continue;
                
                /* Verify connection is still active before processing
                 * Prevents ghost connection counting from stale disconnected connections */
                if (!pConnection->Connected())
                    continue;
                
                /* Check for shutdown during iteration to exit quickly if needed */
                if (config::fShutdown.load())
                {
                    debug::log(1, FUNCTION, "[", strLane, "] Shutdown detected during iteration; stopping");
                    break;
                }
                
                /* Get mining context - returns by value, so use auto (not auto&) */
                auto context = pConnection->GetContext();
                
                /* Check subscription */
                if (!context.fSubscribedToNotifications)
                {
                    nSkippedUnsubscribed++;
                    continue;  // Miner using GET_ROUND polling instead of push notifications
                }
                
                /* Channel filter: only notify miners subscribed to this specific channel */
                if (context.nSubscribedChannel != nChannel)
                {
                    nSkippedWrongChannel++;
                    continue;  // Wrong channel; skip to avoid duplicate notifications
                }
                
                /* Heartbeat path: reset per-connection rate-limit state before sending so
                 * that the push notification bypasses TEMPLATE_PUSH_MIN_INTERVAL_MS and
                 * the miner's subsequent GET_BLOCK is not deferred by the 2-second cooldown
                 * floor.  This is the same state reset that MINER_READY performs, applied
                 * proactively by the heartbeat refresh. */
                if (fHeartbeat)
                {
                    if constexpr (is_miner_protocol_v<ProtocolType>)
                        pConnection->PrepareHeartbeatNotification();
                }

                /* Send notification — exactly once per miner per event per lane */
                pConnection->SendChannelNotification();
                nNotified++;
            }
            
            /* Log per-lane per-channel result for deduplication verification */
            debug::log(0, FUNCTION, "[PUSH][", strLane, "][", strChannelName, "] Notified ", nNotified,
                       " miners (skipped: ", nSkippedWrongChannel, " wrong-channel, ",
                       nSkippedUnsubscribed, " polling)");

            return nNotified;
        }
        else
        {
            /* No-op for protocol types that don't support mining notifications */
            return 0;
        }
    }


    /*  Select lowest latency and currently open connection. */
    template <class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection()
    {
        /* Set our internal return pointer. */
        std::shared_ptr<ProtocolType> pRet;

        /* List of connections to return. */
        uint64_t nLatency   = std::numeric_limits<uint64_t>::max();
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            const uint16_t nSize =
                static_cast<uint16_t>(THREADS_DATA[nThread]->CONNECTIONS->size());

            /* Loop through all connections. */
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Get the current atomic_ptr. */
                    std::shared_ptr<ProtocolType> CONNECTION = THREADS_DATA[nThread]->CONNECTIONS->at(nIndex);
                    if(!CONNECTION)
                        continue;

                    /* Push the active connection. */
                    if(CONNECTION->nLatency < nLatency)
                    {
                        /* Set our new latency value. */
                        nLatency = CONNECTION->nLatency;

                        /* Set our return value. */
                        pRet = CONNECTION;
                    }
                }
                catch(const std::exception& e)
                {
                    //debug::error(FUNCTION, e.what());
                }
            }
        }

        return pRet;
    }


    /*  Select a random and currently open connections. */
    template <class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::RandomConnection()
    {
        /* Get the total count of connections in this server. */
        const uint32_t nTotalConnections =
            GetConnectionCount();

        /* Check for no connections. */
        if(nTotalConnections == 0)
            return nullptr;

        /* Get a random integer for this connection. */
        uint32_t nConnectionIndex =
            LLC::GetRandInt(nTotalConnections);

        /* List of connections to return. */
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            const uint16_t nSize =
                static_cast<uint16_t>(THREADS_DATA[nThread]->CONNECTIONS->size());

            /* Loop through all connections. */
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Get the current atomic_ptr. */
                    std::shared_ptr<ProtocolType> CONNECTION = THREADS_DATA[nThread]->CONNECTIONS->at(nIndex);
                    if(!CONNECTION)
                        continue;

                    /* Push the active connection. */
                    if(nConnectionIndex-- == 0)
                        return CONNECTION;
                }
                catch(const std::exception& e)
                {
                    //debug::error(FUNCTION, e.what());
                }
            }
        }

        return RandomConnection();
    }


    /*  Get the best connection based on latency. */
    template <class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection(const std::pair<uint32_t, uint32_t>& pairExclude)
    {
        /* Set our internal return pointer. */
        std::shared_ptr<ProtocolType> pRet;

        /* List of connections to return. */
        uint64_t nLatency   = std::numeric_limits<uint64_t>::max();
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            uint16_t nSize = static_cast<uint16_t>(THREADS_DATA[nThread]->CONNECTIONS->size());
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Skip over excluded connection. */
                    if(pairExclude.first == nThread && pairExclude.second == nIndex)
                        continue;

                    /* Get the current atomic_ptr. */
                    std::shared_ptr<ProtocolType> CONNECTION = THREADS_DATA[nThread]->CONNECTIONS->at(nIndex);
                    if(!CONNECTION)
                        continue;

                    /* Push the active connection. */
                    if(CONNECTION->nLatency < nLatency && CONNECTION->fOUTGOING)
                    {
                        /* Set our return pointer here now. */
                        nLatency = CONNECTION->nLatency;
                        pRet     = CONNECTION;
                    }
                }
                catch(const std::exception& e)
                {
                    //debug::error(FUNCTION, e.what());
                }
            }
        }

        return pRet;
    }


    /* Get the best connection based on data thread index. */
    template<class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection(const uint32_t nDataThread, const uint32_t nDataIndex)
    {
        return THREADS_DATA[nDataThread]->CONNECTIONS->at(nDataIndex);
    }


    /*  Get the active connection pointers from data threads. */
    template <class ProtocolType>
    std::vector<LegacyAddress> Server<ProtocolType>::GetAddresses()
    {
        /* Loop through the data threads. */
        std::vector<LegacyAddress> vAddr;
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
        {
            /* Get the data threads. */
            DataThread<ProtocolType> *dt = THREADS_DATA[nThread];

            /* Lock the data thread. */
            uint16_t nSize = static_cast<uint16_t>(dt->CONNECTIONS->size());

            /* Loop through connections in data thread. */
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Skip over inactive connections. */
                    if(!dt->CONNECTIONS->at(nIndex))
                        continue;

                    /* Push the active connection. */
                    if(dt->CONNECTIONS->at(nIndex)->Connected())
                        vAddr.emplace_back(dt->CONNECTIONS->at(nIndex)->addr);
                }
                catch(const std::runtime_error& e)
                {
                    debug::error(FUNCTION, e.what());
                }
            }
        }

        return vAddr;
    }


    /*  Notifies all data threads to disconnect their connections */
    template <class ProtocolType>
    void Server<ProtocolType>::DisconnectAll()
    {
        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
            THREADS_DATA[nIndex]->DisconnectAll();
    }


    /* Force a connection disconnect and cleanup server connections. */
    template <class ProtocolType>
    void Server<ProtocolType>::Disconnect(const uint32_t nDataThread, const uint32_t nDataIndex)
    {
        THREADS_DATA[nDataThread]->Disconnect(nDataIndex);
    }


    /* Release all pending triggers from BlockingMessages */
    template <class ProtocolType>
    void Server<ProtocolType>::NotifyTriggers()
    {
        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
            THREADS_DATA[nIndex]->NotifyTriggers();
    }


    /*  Tell the server an event has occured to wake up thread if it is sleeping. This can be used to orchestrate communication
     *  among threads if a strong ordering needs to be guaranteed. */
    template <class ProtocolType>
    void Server<ProtocolType>::NotifyEvent()
    {
        /* Notify the connection of each data thread that an event has occurred. */
        for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
            THREADS_DATA[nThread]->NotifyEvent();
    }


    /* Returns true if SSL is enabled for this server */
    template <class ProtocolType>
    bool Server<ProtocolType>::SSLEnabled()
    {
        return CONFIG.ENABLE_SSL;
    }


    /* Returns true if SSL is required for this server */
    template <class ProtocolType>
    bool Server<ProtocolType>::SSLRequired()
    {
        return CONFIG.REQUIRE_SSL;
    }


     /*  Address Manager Thread. */
    template <class ProtocolType>
    void Server<ProtocolType>::Manager()
    {
        /* If manager is disabled, close down manager thread. */
        if(!pAddressManager)
            return;

        debug::log(0, FUNCTION, Name(), " Connection Manager Started");

        /* Read the address database. */
        pAddressManager->ReadDatabase();
        pAddressManager->SetPort(CONFIG.PORT_BASE);

        /* Timer to print the address manager debug info */
        runtime::timer TIMER;
        TIMER.Start();

        /* Loop connections. */
        while(!config::fShutdown.load())
        {
            /* Check if we need to loop in suspended state. */
            if(config::fSuspended.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Address to select. */
            BaseAddress addr = BaseAddress();

            /* Get the number of incoming and total connection counts */
            uint32_t nConnections = GetConnectionCount(FLAGS::ALL);
            uint32_t nIncoming    = GetConnectionCount(FLAGS::INCOMING);

            /* Sleep between connection attempts.
               If there are no connections then sleep for a minimum interval until a connection is established. */
            if(nConnections == 0)
                runtime::sleep(1000);
            else
                /* Sleep in 1 second intervals for easy break on shutdown. */
                for(int i = 0; i < (CONFIG.MANAGER_SLEEP / 1000) && !config::fShutdown.load(); ++i)
                    runtime::sleep(1000);

            /* Pick a weighted random priority from a sorted list of addresses. */
            if(nConnections < CONFIG.MAX_CONNECTIONS && nIncoming < CONFIG.MAX_INCOMING && pAddressManager->StochasticSelect(addr))
            {
                /* Check for invalid address */
                if(!addr.IsValid())
                {
                    //runtime::sleep(nSleepTime);
                    debug::log(3, FUNCTION, ProtocolType::Name(), " Invalid address, removing address", addr.ToString());
                    pAddressManager->Ban(addr);
                    continue;
                }

                /* Attempt the connection. */
                debug::log(3, FUNCTION, ProtocolType::Name(), " Attempting Connection ", addr.ToString());

                /* Flag indicating connection was successful */
                bool fConnected = false;

                /* First attempt SSL if configured */
                if(CONFIG.ENABLE_SSL)
                   fConnected = AddConnection(addr.ToStringIP(), GetPort(true), true, false);

                /* If SSL connection failed or was not attempted and SSL is not required, attempt on the non-SSL port */
                if(!fConnected && !CONFIG.REQUIRE_SSL)
                    fConnected = AddConnection(addr.ToStringIP(), GetPort(false), false, false);

                if(fConnected)
                {
                    /* If address is DNS, log message on connection. */
                    std::string strDNS;
                    if(pAddressManager->GetDNSName(addr, strDNS))
                        debug::log(3, FUNCTION, "Connected to DNS Address: ", strDNS);
                }
            }
        }
    }


    /*  Determine the first thread with the least amount of active connections.
     *  This keeps them load balanced across all server threads. */
    template <class ProtocolType>
    int32_t Server<ProtocolType>::FindThread()
    {
        int32_t nThread = -1;
        uint32_t nConnections = std::numeric_limits<uint32_t>::max();

        for(uint16_t nIndex = 0; nIndex < CONFIG.MAX_THREADS; ++nIndex)
        {
            /* Find least loaded thread */
            DataThread<ProtocolType> *dt = THREADS_DATA[nIndex];
            if((dt->nIncoming.load() + dt->nOutbound.load()) < nConnections)
            {
                nThread = nIndex;
                nConnections = (dt->nIncoming.load() + dt->nOutbound.load());
            }
        }

        return nThread;
    }


    /*  Main Listening Thread of LLP Server. Handles new Connections and
     *  DDOS associated with Connection if enabled. */
    template <class ProtocolType>
    void Server<ProtocolType>::ListeningThread(bool fIPv4, bool fSSL)
    {
        SOCKET hSocket;
        BaseAddress addr;
        socklen_t len_v4 = sizeof(struct sockaddr_in);
        socklen_t len_v6 = sizeof(struct sockaddr_in6);

        /* Setup poll objects. */
        pollfd fds[1];
        fds[0].events = POLLIN;

        /* Main listener loop. */
        while(!config::fShutdown.load())
        {
            /* Check if we are suspended. */
            if(config::fSuspendProtocol.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Set the listing socket descriptor on the pollfd.  We do this inside the loop in case the listening socket is
               explicitly closed and reopened whilst the app is running (used for mobile) */
            fds[0].fd = get_listening_socket(fIPv4, fSSL);
            if(fds[0].fd != INVALID_SOCKET)
            {
                /* Poll the sockets. */
                fds[0].revents = 0;

#ifdef WIN32
                int32_t nPoll = WSAPoll(&fds[0], 1, 100);
#else
                int32_t nPoll = poll(&fds[0], 1, 100);
#endif

				/* Continue on poll error or no data to read */
                if(nPoll < 0)
                {
                    runtime::sleep(1); //prevent threads from running too fast.
                    continue;
                }

                /* Check for incoming connections. */
                if(!(fds[0].revents & POLLIN))
                {
                    runtime::sleep(1); //prevent threads from running too fast.
                    continue;
                }

                /* Attempt to accept the socket connection */
                struct sockaddr_in sockaddr;
                hSocket = accept(get_listening_socket(fIPv4, fSSL), (struct sockaddr*)&sockaddr, fIPv4 ? &len_v4 : &len_v6);
                if (hSocket != INVALID_SOCKET)
                    addr = BaseAddress(sockaddr);


                if(hSocket == INVALID_SOCKET)
                {
                    /* Catch our socket errors and sleep while waiting for listener to be activated. */
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        debug::error(FUNCTION, "failed to accept ", WSAGetLastError());
                        continue;
                    }
                }
                else
                {
                    /* Check for max connections. */
                    if(GetConnectionCount(FLAGS::INCOMING) >= CONFIG.MAX_INCOMING
                    || GetConnectionCount(FLAGS::ALL) >= CONFIG.MAX_CONNECTIONS)
                    {
                        debug::notice(FUNCTION, "Incoming Connection Request ",  addr.ToString(), " refused... Max connection count exceeded.");
                        closesocket(hSocket);
                        runtime::sleep(500);

                        continue;
                    }

                    /* Create new DDOS Filter if Needed. */
                    if(CONFIG.ENABLE_DDOS && !DDOS_MAP->count(addr))
                        DDOS_MAP->insert(std::make_pair(addr, new DDOS_Filter(CONFIG.DDOS_TIMESPAN)));

                    /* Establish a new socket with SSL on or off according to server. */
                    Socket sockNew(hSocket, addr, fSSL);

                    /* Check that an address is banned. */
                    if(DDOS_MAP->count(addr))
                    {
                        /* Iterate the DDOS cScore (Connection score). */
                        DDOS_MAP->at(addr)->cSCORE += 1;

                        /* Check if we have exceeded our maximum scores. */
                        if(DDOS_MAP->at(addr)-> cSCORE.Score() > CONFIG.DDOS_CSCORE)
                            DDOS_MAP->at(addr)->Ban();

                        /* Check if we have violated DDOS score thresholds. */
                        if(!addr.IsLocal() && DDOS_MAP->at(addr)->Banned())
                        {
                            debug::notice(FUNCTION, "Incoming Connection Request ",  addr.ToString(), " refused... Banned.");
                            sockNew.Close();

                            continue;
                        }
                    }


                    /* Check for errors accepting the connection */
                    if(sockNew.Errors())
                    {
                        debug::notice(FUNCTION, "Incoming Connection Request ",  addr.ToString(), " failed.");
                        sockNew.Close();

                        continue;
                    }

                    /* DDOS Operations: Only executed when DDOS is enabled. */
                    if(!CheckPermissions(addr.ToStringIP(), fSSL ? CONFIG.PORT_SSL : CONFIG.PORT_BASE))
                    {
                        debug::notice(FUNCTION, "Connection Request ",  addr.ToString(), " refused... Denied by allowip whitelist.");

                        sockNew.Close();

                        continue;
                    }

                    int32_t nThread = FindThread();
                    if(nThread < 0)
                    {
                        debug::notice(FUNCTION, "Server has no spare connection capacity... dropping");
                        sockNew.Close();

                        continue;
                    }

                    /* Get the data thread. */
                    DataThread<ProtocolType> *dt = THREADS_DATA[nThread];

                    /* Accept an incoming connection. */
                    dt->AddConnection(sockNew, CONFIG.ENABLE_DDOS ? DDOS_MAP->at(addr) : nullptr);

                    /* Add the address to the address manager if it exists. */
                    if(pAddressManager)
                        pAddressManager->AddAddress(addr, ConnectState::CONNECTED);

                    /* Verbose output. */
                    debug::log(4, FUNCTION, "Accepted Connection ", addr.ToString(), " on port ", fSSL ? CONFIG.PORT_SSL : CONFIG.PORT_BASE);
                }
            }
            else
            {
                /* If the listening socket is invalid then it is likely that it has been explicitly stopped.  In which case we can
                   sleep for an extended period to wait for it to be explicitly restarted */
                runtime::sleep(1000);
                continue;
            }
        }

        /* Thread exiting so close the listening socket */
        closesocket(get_listening_socket(fIPv4, fSSL));
    }


    /*  Bind connection to a listening port. */
    template <class ProtocolType>
    bool Server<ProtocolType>::BindListenPort(int32_t & hListenBase, uint16_t nPort, bool fIPv4, bool fRemote)
    {
        std::string strError = "";
        /* Conditional declaration to avoid "unused variable" */
#if !defined WIN32 || defined SO_NOSIGPIPE
        int32_t nOne = 1;
#endif

        /* Create socket for listening for incoming connections */
        hListenBase = socket(fIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if(hListenBase == INVALID_SOCKET)
        {
            debug::error("Couldn't open socket for incoming connections (socket returned error)", WSAGetLastError());
            return false;
        }

        /* Different way of disabling SIGPIPE on BSD */
#ifdef SO_NOSIGPIPE
        setsockopt(hListenBase, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int32_t));
#endif


        /* Allow binding if the port is still in TIME_WAIT state after the program was closed and restarted.  Not an issue on windows. */
#ifndef WIN32
        setsockopt(hListenBase, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int32_t));
#endif

#ifndef WIN32
        /* Set the MSS to a lower than default value to support the increased bytes required for LISP */
        int nMaxSeg = 1300;
        if(setsockopt(hListenBase, IPPROTO_TCP, TCP_MAXSEG, &nMaxSeg, sizeof(nMaxSeg)) == SOCKET_ERROR)
        { //TODO: this fails on OSX systems. Need to find out why
            //debug::error("setsockopt() MSS for connection failed: ", WSAGetLastError());
            //closesocket(hListenBase);

            //return false;
        }
#endif

        /* The sockaddr_in structure specifies the address family, IP address, and port for the socket that is being bound */
        if(fIPv4)
        {
            struct sockaddr_in sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin_family = AF_INET;

            /* Bind to all interfaces if fRemote has been specified, otherwise only use local interface */
            sockaddr.sin_addr.s_addr = CONFIG.ENABLE_REMOTE ? INADDR_ANY : htonl(INADDR_LOOPBACK);

            sockaddr.sin_port = htons(nPort);
            if(::bind(hListenBase, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEADDRINUSE)
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), "... Nexus is probably still running");
                else
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), " on this computer (bind returned error )",  nErr);
            }

            debug::log(0, FUNCTION, "(v4) Bound to port ", ntohs(sockaddr.sin_port));
        }
        else
        {
            struct sockaddr_in6 sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin6_family = AF_INET6;

            /* Bind to all interfaces if CONFIG.ENABLE_REMOTE has been specified, otherwise only use local interface */
            if(CONFIG.ENABLE_REMOTE)
                sockaddr.sin6_addr = IN6ADDR_ANY_INIT;
            else
                sockaddr.sin6_addr = IN6ADDR_LOOPBACK_INIT;

            sockaddr.sin6_port = htons(CONFIG.PORT_BASE);
            if(::bind(hListenBase, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEADDRINUSE)
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin6_port), "... Nexus is probably still running");
                else
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin6_port), " on this computer (bind returned error )",  nErr);
            }

            debug::log(0, FUNCTION, "(v6) Bound to port ", ntohs(sockaddr.sin6_port));
        }

        /* Listen for incoming connections */
        if(listen(hListenBase, 4096) == SOCKET_ERROR)
        {
            debug::error("Listening for incoming connections failed (listen returned error)", WSAGetLastError());
            return false;
        }

        return true;
    }


    /* LLP Meter Thread. Tracks the Requests / Second. */
    template <class ProtocolType>
    void Server<ProtocolType>::Meter()
    {
        /* For miner protocols the Meter thread always runs (even without -meters) to
         * drive the heartbeat template refresh.  For all other protocol types, exit
         * early when meters are disabled (preserving the original behaviour). */
        if(!CONFIG.ENABLE_METERS && !is_miner_protocol_v<ProtocolType>)
            return;

        /* Keep track of elapsed time. */
        runtime::timer TIMER;
        if(CONFIG.ENABLE_METERS)
            TIMER.Start();
        
        /* Keep track of cleanup timer (60 seconds for AutoCooldownManager) */
        runtime::timer CLEANUP_TIMER;
        CLEANUP_TIMER.Start();

        /* Heartbeat timer — fires HeartbeatRefreshCheck() every
         * HEARTBEAT_CHECK_INTERVAL_SECONDS for miner protocols. */
        runtime::timer HEARTBEAT_TIMER;
        if constexpr (is_miner_protocol_v<ProtocolType>)
            HEARTBEAT_TIMER.Start();

        /* Loop until shutdown. */
        while(!config::fShutdown.load())
        {
            runtime::sleep(100);

            /* Proactive template heartbeat refresh for miner protocols.
             * Runs regardless of ENABLE_METERS and before the zero-connection
             * skip so that a connected-but-idle miner still receives refreshes. */
            if constexpr (is_miner_protocol_v<ProtocolType>)
            {
                if(HEARTBEAT_TIMER.Elapsed() >= MiningConstants::HEARTBEAT_CHECK_INTERVAL_SECONDS)
                {
                    MinerPushDispatcher::HeartbeatRefreshCheck();
                    HEARTBEAT_TIMER.Reset();
                }
            }

            /* Skip metric logging if meters are disabled */
            if(!CONFIG.ENABLE_METERS)
                continue;

            if(TIMER.Elapsed() < 30)
                continue;

            /* Get total connection count. */
            uint32_t nGlobalConnections = 0;
            for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
            {
                DataThread<ProtocolType> *dt = THREADS_DATA[nThread];
                nGlobalConnections += (dt->nIncoming.load() + dt->nOutbound.load());
            }

            /* Total incoming and outgoing packets. */
            uint32_t RPS = ProtocolType::REQUESTS / TIMER.Elapsed();
            uint32_t CPS = ProtocolType::CONNECTIONS / TIMER.Elapsed();
            uint32_t DPS = ProtocolType::DISCONNECTS / TIMER.Elapsed();
            uint32_t PPS = ProtocolType::PACKETS / TIMER.Elapsed();

            /* Omit meter when zero values detected. */
            if((RPS == 0 && PPS == 0) || nGlobalConnections == 0)
                continue;

            /* Meter output. */
            debug::log(0,
                ANSI_COLOR_FUNCTION, Name(), " LLP : ", ANSI_COLOR_RESET,
                CPS, " Accepts/s | ",
                DPS, " Closing/s | ",
                RPS, " Incoming/s | ",
                PPS, " Outgoing/s | ",
                nGlobalConnections, " Connections"
            );

            /* Reset meter info. */
            TIMER.Reset();
            ProtocolType::REQUESTS.store(0);
            ProtocolType::PACKETS.store(0);
            ProtocolType::CONNECTIONS.store(0);
            ProtocolType::DISCONNECTS.store(0);
            
            /* Periodic cleanup cadence is 10 minutes, but the sweep cadence is not the
             * expiry threshold itself. Each pass compares entries against the much longer
             * session/cache timeouts (for example SESSION_LIVENESS_TIMEOUT_SECONDS = 86400s),
             * so running cleanup every 10 minutes only bounds stale-state retention latency
             * without shortening the underlying liveness window. */
            if(CLEANUP_TIMER.Elapsed() >= 600)
            {
                AutoCooldownManager::Get().CleanupExpired();

                if constexpr (is_miner_protocol_v<ProtocolType>)
                {
                    StatelessMinerManager::Get().CleanupInactive(NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS);
                    StatelessMinerManager::Get().PurgeInactiveMiners();
                    SessionRecoveryManager::Get().CleanupExpired(
                        SessionRecoveryManager::Get().GetSessionTimeout());
                    NodeSessionRegistry::Get().SweepExpired(NodeCache::SESSION_LIVENESS_TIMEOUT_SECONDS);
                }

                CLEANUP_TIMER.Reset();
            }
        }
    }


    /* Gets the listening socket handle */
    template <class ProtocolType>
    SOCKET Server<ProtocolType>::get_listening_socket(bool fIPv4, bool fSSL)
    {
        SOCKET hListen;

        if(fSSL)
            hListen = (fIPv4 ? hListenSSL.first : hListenSSL.second);
        else
            hListen = (fIPv4 ? hListenBase.first : hListenBase.second);

        return hListen;
    }


    /* Closes the listening sockets. */
    template <class ProtocolType>
    void Server<ProtocolType>::CloseListening()
    {
        debug::log(0, "Closing ", ProtocolType::Name(), " listening sockets");

        /* Close the listening sockets */
        if(!CONFIG.REQUIRE_SSL)
        {
            closesocket(hListenBase.first);
            hListenBase.first = 0;
        }

        /* Close the ssl socket if running */
        if(CONFIG.ENABLE_SSL)
        {
            closesocket(hListenSSL.first);
            hListenSSL.first = 0;
        }

    }


    /* Restarts the listening sockets */
    template <class ProtocolType>
    void Server<ProtocolType>::OpenListening()
    {
        /* Skip over opening listeners if listening is disabled. */
        if(!CONFIG.ENABLE_LISTEN)
            return;

        /* If SSL is required then don't listen on the standard port */
        if(!CONFIG.REQUIRE_SSL)
        {
            /* Bind the Listener. */
            if(!BindListenPort(hListenBase.first, CONFIG.PORT_BASE, true, CONFIG.ENABLE_REMOTE))
            {
                ::Shutdown();
                return;
            }

            debug::log(0, "Opening ", ProtocolType::Name(), " listening sockets on port ", CONFIG.PORT_BASE);
        }

        /* Handle for our SSL socket. */
        if(CONFIG.ENABLE_SSL)
        {
            if(!BindListenPort(hListenSSL.first, CONFIG.PORT_SSL, true, CONFIG.ENABLE_REMOTE))
            {
                ::Shutdown();
                return;
            }

            debug::log(0, "Opening ", ProtocolType::Name(), " SSL listening sockets on port ", CONFIG.PORT_SSL);
        }
    }


    /* UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding. */
    template <class ProtocolType>
    void Server<ProtocolType>::UPnP(const uint16_t nPort)
    {
    #ifndef USE_UPNP
        return;
    #else

        char port[6];
        sprintf(port, "%d", nPort);

        const char * multicastif = 0;
        const char * minissdpdpath = 0;
        struct UPNPDev * devlist = 0;
        char lanaddr[64];

        #ifndef UPNPDISCOVER_SUCCESS
            /* miniupnpc 1.5 */
            devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
        #elif MINIUPNPC_API_VERSION < 14
            /* miniupnpc 1.6 */
            int error = 0;
            devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
        #else
            /* miniupnpc 1.9.20150730 */
            int error = 0;
            devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, 2, &error);
        #endif

        struct UPNPUrls urls;
        struct IGDdatas data;
        int nResult;

        if(devlist == 0)
        {
            debug::error(FUNCTION, "No UPnP devices found");
            return;
        }

        nResult = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), nullptr, 0);
        if (nResult == 1)
        {

            std::string strDesc = version::CLIENT_VERSION_BUILD_STRING;
        #ifndef UPNPDISCOVER_SUCCESS
                /* miniupnpc 1.5 */
                nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0);
        #else
                /* miniupnpc 1.6 */
                nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
        #endif

            if(nResult != UPNPCOMMAND_SUCCESS)
                debug::error(FUNCTION, "AddPortMapping(", port, ", ", port, ", ", lanaddr, ") failed with code ", nResult, " (", strupnperror(nResult), ")");
            else
                debug::log(1, "UPnP Port Mapping successful for port: ", nPort);

            runtime::timer TIMER;
            TIMER.Start();

            while(!config::fShutdown.load())
            {
                if (TIMER.Elapsed() >= 600) // Refresh every 10 minutes
                {
                    TIMER.Reset();

        #ifndef UPNPDISCOVER_SUCCESS
                    /* miniupnpc 1.5 */
                    nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                        port, port, lanaddr, strDesc.c_str(), "TCP", 0);
        #else
                    /* miniupnpc 1.6 */
                    nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                        port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
        #endif

                    if(nResult != UPNPCOMMAND_SUCCESS)
                        debug::error(FUNCTION, "AddPortMapping(", port, ", ", port, ", ", lanaddr, ") failed with code ", nResult, " (", strupnperror(nResult), ")");
                    else
                        debug::log(1, "UPnP Port Mapping successful for port: ", nPort);
                }
                runtime::sleep(1000);
            }

            /* Shutdown sequence */
            nResult = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port, "TCP", 0);
            debug::log(1, "UPNP_DeletePortMapping() returned : ", nResult);
            freeUPNPDevlist(devlist); devlist = 0;
            FreeUPNPUrls(&urls);
        }
        else
        {
            debug::error(FUNCTION, "No valid UPnP IGDs found.");
            freeUPNPDevlist(devlist); devlist = 0;
            if (nResult != 0)
                FreeUPNPUrls(&urls);

            return;
        }

       debug::log(0, "UPnP closed.");
    #endif
    }




    /* Explicity instantiate all template instances needed for compiler. */
    template class Server<TritiumNode>;
    template class Server<LookupNode>;
    template class Server<TimeNode>;
    template class Server<APINode>;
    template class Server<FileNode>;
    template class Server<RPCNode>;
    template class Server<Miner>;
    template class Server<StatelessMinerConnection>;
}
