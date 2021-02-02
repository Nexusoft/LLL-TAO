/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/server.h>
#include <LLP/templates/data.h>
#include <LLP/templates/ddos.h>
#include <LLP/include/network.h>

#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/p2p.h>

#include <LLP/include/trust_address.h>

#include <Util/include/args.h>
#include <Util/include/signals.h>
#include <Util/include/version.h>

#include <LLP/include/permissions.h>
#include <functional>
#include <numeric>

#include <openssl/ssl.h>

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif


namespace LLP
{

    /*  Returns the name of the protocol type of this server. */
    template <class ProtocolType>
    std::string Server<ProtocolType>::Name()
    {
        return ProtocolType::Name();
    }


    /** Constructor **/
    template <class ProtocolType>
    Server<ProtocolType>::Server(const ServerConfig& config)
    : PORT            (config.nPort)
    , SSL_PORT        (config.nSSLPort)
    , hListenSocket   (-1, -1)
    , hSSLListenSocket(-1, -1)
    , fSSL            (config.fSSL)
    , fSSLRequired    (config.fSSLRequired)
    , DDOS_MAP        ( )
    , fDDOS           (config.fDDOS)
    , DDOS_TIMESPAN   (config.nDDOSTimespan)
    , MAX_THREADS     (config.nMaxThreads)
    , DATA_THREADS    ( )
    , MANAGER         ( )
    , LISTEN_THREAD   ( )
    , SSL_LISTEN_THREAD   ( )
    , METER_THREAD    ( )
    , UPNP_THREAD     ( )
    , SSL_UPNP_THREAD ( )
    , MANAGER_THREAD  ( )
    , pAddressManager (nullptr)
    , nSleepTime      (config.nManagerInterval)
    , nMaxIncoming    (config.nMaxIncoming)
    , nMaxConnections (config.nMaxConnections)
    , fRemote         (config.fRemote)
    {
        for(uint16_t nIndex = 0; nIndex < MAX_THREADS; ++nIndex)
        {
            DATA_THREADS.push_back(new DataThread<ProtocolType>(
                nIndex, config.fDDOS, config.nDDOSRScore, config.nDDOSCScore, config.nTimeout, config.fMeter));
        }

        /* Initialize the address manager. */
        if(config.fManager)
        {
            pAddressManager = new AddressManager(PORT);
            if(!pAddressManager)
                debug::error(FUNCTION, "Failed to allocate memory for address manager on port ", PORT);

            MANAGER_THREAD = std::thread((std::bind(&Server::Manager, this)));
        }

        /* Initialize the listeners. */
        if(config.fListen)
        {
            /* Open the required listening sockets */
            OpenListening();

            /* Bind the listening threads */
            if(!fSSLRequired)   
                LISTEN_THREAD = std::thread(std::bind(&Server::ListeningThread, this, true, false));  //IPv4 Listener

            if(fSSL)
                SSL_LISTEN_THREAD = std::thread(std::bind(&Server::ListeningThread, this, true, true));  //IPv4 SSL Listener

            /* Initialize the UPnP thread if remote connections are allowed. */
            if(fRemote && config::GetBoolArg(std::string("-upnp"), true))
            {
                if(!fSSLRequired)
                    UPNP_THREAD = std::thread(std::bind(&Server::UPnP, this, PORT));

                if(fSSL)
                    SSL_UPNP_THREAD = std::thread(std::bind(&Server::UPnP, this, SSL_PORT));
            }

        }

        /* Initialize the meter. */
        if(config.fMeter)
            METER_THREAD = std::thread(std::bind(&Server::Meter, this));

    }

    /** Default Destructor **/
    template <class ProtocolType>
    Server<ProtocolType>::~Server()
    {
        /* Wait for address manager. */
        if(pAddressManager)
        {
            if(MANAGER_THREAD.joinable())
                MANAGER_THREAD.join();
        }

        /* Wait for meter thread. */
        if(METER_THREAD.joinable())
            METER_THREAD.join();

        /* Wait for UPnP thread */
        if(UPNP_THREAD.joinable())
            UPNP_THREAD.join();

        /* Wait for SSL UPnP thread */
        if(SSL_UPNP_THREAD.joinable())
            SSL_UPNP_THREAD.join();

        /* Wait for listener thread. */
        if(LISTEN_THREAD.joinable())
            LISTEN_THREAD.join();

        /* Wait for SSLlistener thread. */
        if(SSL_LISTEN_THREAD.joinable())
            SSL_LISTEN_THREAD.join();

        /* Delete the data threads. */
        for(uint16_t nIndex = 0; nIndex < MAX_THREADS; ++nIndex)
        {
            delete DATA_THREADS[nIndex];
            DATA_THREADS[nIndex] = nullptr;
        }

        /* Delete the DDOS entries. */
        auto it = DDOS_MAP.begin();
        for(; it != DDOS_MAP.end(); ++it)
        {
            if(it->second)
                delete it->second;
        }
        DDOS_MAP.clear();

        /* Clear the address manager. */
        if(pAddressManager)
        {
            delete pAddressManager;
            pAddressManager = nullptr;
        }
    }


    /*  Returns the port number for this Server. */
    template <class ProtocolType>
    uint16_t Server<ProtocolType>::GetPort(bool fSSL) const
    {
        return fSSL ? SSL_PORT : PORT;
    }


    /*Returns the address manager instance for this server. */
    template <class ProtocolType>
    AddressManager* Server<ProtocolType>::GetAddressManager() const
    {
        return pAddressManager;
    }


     /*  Cleanup and shutdown subsystems */
    template <class ProtocolType>
    void Server<ProtocolType>::Shutdown()
    {
    }


    /*  Add a node address to the internal address manager */
    template <class ProtocolType>
    void Server<ProtocolType>::AddNode(std::string strAddress, bool fLookup)
    {
       /* Assemble the address from input parameters. */
       BaseAddress addr(strAddress, PORT, fLookup);

       /* Make sure address is valid. */
       if(!addr.IsValid())
            return;

       /* Add the address to the address manager if it exists. */
       if(pAddressManager)
          pAddressManager->AddAddress(addr, ConnectState::NEW);
    }


    /* Constructs a vector of all active connections across all threads */
    template <class ProtocolType>
    std::vector<std::shared_ptr<ProtocolType>> Server<ProtocolType>::GetConnections() const
    {
        std::vector<std::shared_ptr<ProtocolType>> vConnections;

        /* Iterate through threads */
        for(uint16_t nThread = 0; nThread < DATA_THREADS.size(); ++nThread)
        {
            /* Loop through connections in data thread. */
            uint16_t nSize = static_cast<uint16_t>(DATA_THREADS[nThread]->CONNECTIONS->size());
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Get the current atomic_ptr. */
                std::shared_ptr<ProtocolType> pConnection = DATA_THREADS[nThread]->CONNECTIONS->at(nIndex);
                
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
        uint32_t nConnections = 0;
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            nConnections += DATA_THREADS[nThread]->GetConnectionCount(nFlags);

        return nConnections;
    }


    /*  Select a random and currently open connections. */
    template <class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection()
    {
        /* List of connections to return. */
        uint64_t nLatency   = std::numeric_limits<uint64_t>::max();

        int16_t nRetThread = -1;
        int16_t nRetIndex  = -1;
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            uint16_t nSize = static_cast<uint16_t>(DATA_THREADS[nThread]->CONNECTIONS->size());
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Get the current atomic_ptr. */
                    std::shared_ptr<ProtocolType> CONNECTION = DATA_THREADS[nThread]->CONNECTIONS->at(nIndex);
                    if(!CONNECTION)
                        continue;

                    /* Push the active connection. */
                    if(CONNECTION->nLatency < nLatency)
                    {
                        nLatency = CONNECTION->nLatency;

                        nRetThread = nThread;
                        nRetIndex  = nIndex;
                    }
                }
                catch(const std::exception& e)
                {
                    //debug::error(FUNCTION, e.what());
                }
            }
        }

        /* Handle if no connections were found. */
        static std::shared_ptr<ProtocolType> pNULL;
        if(nRetThread == -1 || nRetIndex == -1)
            return pNULL;

        return DATA_THREADS[nRetThread]->CONNECTIONS->at(nRetIndex);
    }


    /*  Get the best connection based on latency. */
    template <class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection(const std::pair<uint32_t, uint32_t>& pairExclude)
    {
        /* List of connections to return. */
        uint64_t nLatency   = std::numeric_limits<uint64_t>::max();

        int16_t nRetThread = -1;
        int16_t nRetIndex  = -1;
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            /* Loop through connections in data thread. */
            uint16_t nSize = static_cast<uint16_t>(DATA_THREADS[nThread]->CONNECTIONS->size());
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Skip over excluded connection. */
                    if(pairExclude.first == nThread && pairExclude.second == nIndex)
                        continue;

                    /* Get the current atomic_ptr. */
                    std::shared_ptr<ProtocolType> CONNECTION = DATA_THREADS[nThread]->CONNECTIONS->at(nIndex);
                    if(!CONNECTION)
                        continue;

                    /* Push the active connection. */
                    if(CONNECTION->nLatency < nLatency && CONNECTION->fOUTGOING)
                    {
                        nLatency = CONNECTION->nLatency;

                        nRetThread = nThread;
                        nRetIndex  = nIndex;
                    }
                }
                catch(const std::exception& e)
                {
                    //debug::error(FUNCTION, e.what());
                }
            }
        }

        /* Handle if no connections were found. */
        static std::shared_ptr<ProtocolType> pNULL;
        if(nRetThread == -1 || nRetIndex == -1)
            return pNULL;

        return DATA_THREADS[nRetThread]->CONNECTIONS->at(nRetIndex);
    }


    /* Get the best connection based on data thread index. */
    template<class ProtocolType>
    std::shared_ptr<ProtocolType> Server<ProtocolType>::GetConnection(const uint32_t nDataThread, const uint32_t nDataIndex)
    {
        return DATA_THREADS[nDataThread]->CONNECTIONS->at(nDataIndex);
    }


    /*  Get the active connection pointers from data threads. */
    template <class ProtocolType>
    std::vector<LegacyAddress> Server<ProtocolType>::GetAddresses()
    {
        /* Loop through the data threads. */
        std::vector<LegacyAddress> vAddr;
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            /* Get the data threads. */
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

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
        for(uint16_t nIndex = 0; nIndex < MAX_THREADS; ++nIndex)
            DATA_THREADS[nIndex]->DisconnectAll();
    }


    /*  Tell the server an event has occured to wake up thread if it is sleeping. This can be used to orchestrate communication
     *  among threads if a strong ordering needs to be guaranteed. */
    template <class ProtocolType>
    void Server<ProtocolType>::NotifyEvent()
    {
        /* Notify the connection of each data thread that an event has occurred. */
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            DATA_THREADS[nThread]->NotifyEvent();
    }


    /* Returns true if SSL is enabled for this server */
    template <class ProtocolType>
    bool Server<ProtocolType>::SSLEnabled()
    {
        return fSSL;
    }


    /* Returns true if SSL is required for this server */
    template <class ProtocolType>
    bool Server<ProtocolType>::SSLRequired()
    {
        return fSSLRequired;
    }


     /*  Address Manager Thread. */
    template <class ProtocolType>
    void Server<ProtocolType>::Manager()
    {
        /* If manager is disabled, close down manager thread. */
        if(!pAddressManager)
            return;

        debug::log(0, FUNCTION, Name(), " Connection Manager Started");

        /* Address to select. */
        BaseAddress addr = BaseAddress();

        /* Read the address database. */
        pAddressManager->ReadDatabase();

        /* Set the port. */
        pAddressManager->SetPort(PORT);

        /* Timer to print the address manager debug info */
        runtime::timer TIMER;
        TIMER.Start();

        /* Loop connections. */
        while(!config::fShutdown.load())
        {
            /* Sleep between connection attempts. */
            runtime::sleep(100);

            /* Pick a weighted random priority from a sorted list of addresses. */
            if(GetConnectionCount(FLAGS::INCOMING) < nMaxIncoming
            && GetConnectionCount(FLAGS::ALL) < nMaxConnections
            && pAddressManager->StochasticSelect(addr))
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
                if(fSSL)
                   fConnected = AddConnection(addr.ToStringIP(), GetPort(true), true, false);

                /* If SSL connection failed or was not attempted and SSL is not required, attempt on the non-SSL port */
                if(!fConnected && !fSSLRequired)
                    fConnected = AddConnection(addr.ToStringIP(), GetPort(false), false, false);

                if(fConnected)
                {
                    /* If address is DNS, log message on connection. */
                    std::string strDNS;
                    if(pAddressManager->GetDNSName(addr, strDNS))
                        debug::log(3, FUNCTION, "Connected to DNS Address: ", strDNS);

                    /* Sleep in 1 second intervals for easy break on shutdown. */
                    for(int i = 0; i < (nSleepTime / 1000) && !config::fShutdown.load(); ++i)
                        runtime::sleep(1000);
                }
            }

            /* Print the debug info every 10s */
            if(TIMER.Elapsed() >= 10)
            {
                debug::log(3, FUNCTION, ProtocolType::Name(), " ", pAddressManager->ToString());
                TIMER.Reset();
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

        for(uint16_t nIndex = 0; nIndex < MAX_THREADS; ++nIndex)
        {
            /* Find least loaded thread */
            DataThread<ProtocolType> *dt = DATA_THREADS[nIndex];
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
            /* Set the listing socket descriptor on the pollfd.  We do this inside the loop in case the listening socket is 
               explicitly closed and reopened whilst the app is running (used for mobile) */
            fds[0].fd = get_listening_socket(fIPv4, fSSL);

            if (fds[0].fd != INVALID_SOCKET)
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
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                        debug::error(FUNCTION, "Socket error accept failed: ", WSAGetLastError());
                }
                else
                {
                    /* Check for max connections. */
                    if(GetConnectionCount(FLAGS::INCOMING) >= nMaxIncoming
                    || GetConnectionCount(FLAGS::ALL) >= nMaxConnections)
                    {
                        debug::log(3, FUNCTION, "Incoming Connection Request ",  addr.ToString(), " refused... Max connection count exceeded.");
                        closesocket(hSocket);
                        runtime::sleep(500);

                        continue;
                    }

                    /* Create new DDOS Filter if Needed. */
                    if(!DDOS_MAP.count(addr))
                        DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);


                    /* Establish a new socket with SSL on or off according to server. */
                    Socket sockNew(hSocket, addr, fSSL);

                    /* Check for errors accepting the connection */
                    if(sockNew.Errors())
                    {
                        debug::log(3, FUNCTION, "Incoming Connection Request ",  addr.ToString(), " failed.");
                        sockNew.Close();

                        continue;
                    }

                    /* DDOS Operations: Only executed when DDOS is enabled. */
                    if((fDDOS && DDOS_MAP[addr]->Banned()))
                    {
                        debug::log(3, FUNCTION, "Incoming Connection Request ",  addr.ToString(), " refused... Banned.");
                        sockNew.Close();

                        continue;
                    }
                    else if(!CheckPermissions(addr.ToStringIP(), fSSL ? SSL_PORT : PORT))
                    {
                        debug::log(3, FUNCTION, "Connection Request ",  addr.ToString(), " refused... Denied by allowip whitelist.");

                        sockNew.Close();

                        continue;
                    }

                    int32_t nThread = FindThread();
                    if(nThread < 0)
                    {
                        debug::error(FUNCTION, "Server has no spare connection capacity... dropping");
                        sockNew.Close();

                        continue;
                    }

                    /* Get the data thread. */
                    DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

                    /* Accept an incoming connection. */
                    dt->AddConnection(sockNew, DDOS_MAP[addr]);

                    /* Verbose output. */
                    debug::log(3, FUNCTION, "Accepted Connection ", addr.ToString(), " on port ", fSSL ? SSL_PORT : PORT);
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
    bool Server<ProtocolType>::BindListenPort(int32_t & hListenSocket, uint16_t nPort, bool fIPv4, bool fRemote)
    {
        std::string strError = "";
        /* Conditional declaration to avoid "unused variable" */
#if !defined WIN32 || defined SO_NOSIGPIPE
        int32_t nOne = 1;
#endif

        /* Create socket for listening for incoming connections */
        hListenSocket = socket(fIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if(hListenSocket == INVALID_SOCKET)
        {
            debug::error("Couldn't open socket for incoming connections (socket returned error)", WSAGetLastError());
            return false;
        }

        /* Different way of disabling SIGPIPE on BSD */
#ifdef SO_NOSIGPIPE
        setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int32_t));
#endif


        /* Allow binding if the port is still in TIME_WAIT state after the program was closed and restarted.  Not an issue on windows. */
#ifndef WIN32
        setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int32_t));
#endif

#ifndef WIN32
        /* Set the MSS to a lower than default value to support the increased bytes required for LISP */
        int nMaxSeg = 1300;
        if(setsockopt(hListenSocket, IPPROTO_TCP, TCP_MAXSEG, &nMaxSeg, sizeof(nMaxSeg)) == SOCKET_ERROR)
        { //TODO: this fails on OSX systems. Need to find out why
            //debug::error("setsockopt() MSS for connection failed: ", WSAGetLastError());
            //closesocket(hListenSocket);

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
            sockaddr.sin_addr.s_addr = fRemote ? INADDR_ANY : htonl(INADDR_LOOPBACK);

            sockaddr.sin_port = htons(nPort);
            if(::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEADDRINUSE)
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), "... Nexus is probably still running");
                else
                    return debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), " on this computer (bind returned error )",  nErr);
            }

            debug::log(0, FUNCTION,"(v4) Bound to port ", ntohs(sockaddr.sin_port));
        }
        else
        {
            struct sockaddr_in6 sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin6_family = AF_INET6;

            /* Bind to all interfaces if fRemote has been specified, otherwise only use local interface */
            if(fRemote)
                sockaddr.sin6_addr = IN6ADDR_ANY_INIT;
            else
                sockaddr.sin6_addr = IN6ADDR_LOOPBACK_INIT;

            sockaddr.sin6_port = htons(PORT);
            if(::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
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
        if(listen(hListenSocket, 4096) == SOCKET_ERROR)
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
        /* Exit if not enabled. */
        if(!config::GetBoolArg("-meters", false))
            return;

        /* Keep track of elapsed time. */
        runtime::timer TIMER;
        TIMER.Start();

        /* Loop until shutdown. */
        while(!config::fShutdown.load())
        {
            runtime::sleep(100);
            if(TIMER.Elapsed() < 30)
                continue;

            /* Get total connection count. */
            uint32_t nGlobalConnections = 0;
            for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            {
                DataThread<ProtocolType> *dt = DATA_THREADS[nThread];
                nGlobalConnections += (dt->nIncoming.load() + dt->nOutbound.load());
            }

            /* Total incoming and outgoing packets. */
            uint32_t RPS = ProtocolType::REQUESTS / TIMER.Elapsed();
            uint32_t PPS = ProtocolType::PACKETS / TIMER.Elapsed();

            /* Omit meter when zero values detected. */
            if((RPS == 0 && PPS == 0) || nGlobalConnections == 0)
                continue;

            /* Meter output. */
            debug::log(0,
                ANSI_COLOR_FUNCTION, Name(), " LLP : ", ANSI_COLOR_RESET,
                RPS, " Incoming/s | ",
                PPS, " Outgoing/s | ",
                RPS + PPS, " Packets/s | ",
                nGlobalConnections, " Connections."
            );

            /* Reset meter info. */
            TIMER.Reset();
            ProtocolType::REQUESTS.store(0);
            ProtocolType::PACKETS.store(0);
        }
    }


    /* UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding. */
    template <class ProtocolType>
    void Server<ProtocolType>::UPnP(uint16_t nPort)
    {
#ifndef USE_UPNP
        return;
#else

        if(!config::GetBoolArg("-upnp", true))
            return;

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

        nResult = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
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
                runtime::sleep(2000);
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


    /* Gets the listening socket handle */
    template <class ProtocolType>
    SOCKET Server<ProtocolType>::get_listening_socket(bool fIPv4, bool fSSL)
    {
        SOCKET hListen;

        if(fSSL)
            hListen = (fIPv4 ? hSSLListenSocket.first : hSSLListenSocket.second);
        else
            hListen = (fIPv4 ? hListenSocket.first : hListenSocket.second);

        return hListen;
    }


    /* Closes the listening sockets. */
    template <class ProtocolType>
    void Server<ProtocolType>::CloseListening()
    {
        debug::log(0, "Closing ", ProtocolType::Name(), " listening sockets");

        /* Close the listening sockets */
        if(!fSSLRequired)
        {
            closesocket(hListenSocket.first);
            hListenSocket.first = 0;
        }

        /* Close the ssl socket if running */
        if(fSSL)
        {
            closesocket(hSSLListenSocket.first);
            hSSLListenSocket.first = 0;
        }

    }


    /* Restarts the listening sockets */
    template <class ProtocolType>
    void Server<ProtocolType>::OpenListening()
    {
        debug::log(0, "Opening ", ProtocolType::Name(), " listening sockets");

        /* If SSL is required then don't listen on the standard port */
        if(!fSSLRequired)
        {
            /* Bind the Listener. */
            if(!BindListenPort(hListenSocket.first, PORT, true, fRemote))
            {
                ::Shutdown();
                return;
            }
        }

        if(fSSL)
        {
            if(!BindListenPort(hSSLListenSocket.first, SSL_PORT, true, fRemote))
            {
                ::Shutdown();
                return;
            }
        } 
    }




    /* Explicity instantiate all template instances needed for compiler. */
    template class Server<TritiumNode>;
    template class Server<TimeNode>;
    template class Server<APINode>;
    template class Server<RPCNode>;
    template class Server<Miner>;
    template class Server<P2PNode>;
}
