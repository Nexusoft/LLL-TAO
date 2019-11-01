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
#include <LLP/types/legacy.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <LLP/include/manager.h>
#include <LLP/include/trust_address.h>

#include <Util/include/args.h>
#include <Util/include/signals.h>
#include <Util/include/version.h>

#include <LLP/include/permissions.h>
#include <functional>
#include <numeric>

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
    Server<ProtocolType>::Server(uint16_t nPort, uint16_t nMaxThreads, uint32_t nTimeout, bool fDDOS_,
                         uint32_t cScore, uint32_t rScore, uint32_t nTimespan, bool fListen, bool fRemote,
                         bool fMeter, bool fManager, uint32_t nSleepTimeIn)
    : fDDOS(fDDOS_)
    , PORT(nPort)
    , MAX_THREADS(nMaxThreads)
    , DDOS_TIMESPAN(nTimespan)
    , DATA_THREADS()
    , pAddressManager(nullptr)
    , nSleepTime(nSleepTimeIn)
    , hListenSocket(-1, -1)
    {
        for(uint16_t nIndex = 0; nIndex < MAX_THREADS; ++nIndex)
        {
            DATA_THREADS.push_back(new DataThread<ProtocolType>(
                nIndex, fDDOS_, rScore, cScore, nTimeout, fMeter));
        }

        /* Initialize the address manager. */
        if(fManager)
        {
            pAddressManager = new AddressManager(nPort);
            if(!pAddressManager)
                debug::error(FUNCTION, "Failed to allocate memory for address manager on port ", nPort);

            MANAGER_THREAD = std::thread((std::bind(&Server::Manager, this)));
        }

        /* Initialize the listeners. */
        if(fListen)
        {
            /* Bind the Listener. */
            if(!BindListenPort(hListenSocket.first, true, fRemote))
            {
                ::Shutdown();
                return;
            }

            LISTEN_THREAD_V4 = std::thread(std::bind(&Server::ListeningThread, this, true));  //IPv4 Listener

            /* Initialize the UPnP thread if remote connections are allowed. */
            if(fRemote && config::GetBoolArg(std::string("-upnp"), true))
                UPNP_THREAD = std::thread(std::bind(&Server::UPnP, this));
                
        }

        /* Initialize the meter. */
        if(fMeter)
            METER_THREAD = std::thread(std::bind(&Server::Meter, this));

    }

    /** Default Destructor **/
    template <class ProtocolType>
    Server<ProtocolType>::~Server()
    {
        /* Set all flags back to false. */
        fDDOS.store(false);

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

        /* Wait for listeners. */
        if(LISTEN_THREAD_V4.joinable())
            LISTEN_THREAD_V4.join();

        if(LISTEN_THREAD_V6.joinable())
            LISTEN_THREAD_V6.join();

        /* Delete the data threads. */
        for(uint16_t index = 0; index < MAX_THREADS; ++index)
        {
            delete DATA_THREADS[index];
            DATA_THREADS[index] = nullptr;
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
    uint16_t Server<ProtocolType>::GetPort() const
    {
        return PORT;
    }


     /*  Cleanup and shutdown subsystems */
    template <class ProtocolType>
    void Server<ProtocolType>::Shutdown()
    {
        /* DEPRECATED. address write to database on update, not shutdown. */
        //if(pAddressManager)
        //  pAddressManager->WriteDatabase();
    }


   /*  Add a node address to the internal address manager */
   template <class ProtocolType>
   void Server<ProtocolType>::AddNode(std::string strAddress, uint16_t nPort, bool fLookup)
   {
       /* Assemble the address from input parameters. */
       BaseAddress addr(strAddress, nPort, fLookup);

       /* Make sure address is valid. */
       if(!addr.IsValid())
            return;

       /* Add the address to the address manager if it exists. */
       if(pAddressManager)
          pAddressManager->AddAddress(addr, ConnectState::NEW);
   }


   /*  Public Wraper to Add a Connection Manually. */
   template <class ProtocolType>
   bool Server<ProtocolType>::AddConnection(std::string strAddress, uint16_t nPort, bool fLookup)
   {
       /* Initialize DDOS Protection for Incoming IP Address. */
       BaseAddress addrConnect(strAddress, nPort, fLookup);

       /* Make sure address is valid. */
       if(!addrConnect.IsValid())
       {
           /* Ban the address. */
           if(pAddressManager)
              pAddressManager->Ban(addrConnect);

           return false;
       }


       /* Create new DDOS Filter if Needed. */
       if(fDDOS.load())
       {
           if(!DDOS_MAP.count(addrConnect))
               DDOS_MAP[addrConnect] = new DDOS_Filter(DDOS_TIMESPAN);

           /* DDOS Operations: Only executed when DDOS is enabled. */
           if(DDOS_MAP[addrConnect]->Banned())
               return false;
       }

       /* Find a balanced Data Thread to Add Connection to. */
       int32_t nThread = FindThread();
       if(nThread < 0)
           return false;

       /* Select the proper data thread. */
       DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

       /* Attempt the connection. */
       if(!dt->AddConnection(addrConnect, DDOS_MAP[addrConnect]))
       {
           /* Add the address to the address manager if it exists. */
           if(pAddressManager)
              pAddressManager->AddAddress(addrConnect, ConnectState::FAILED);

           return false;
       }

       /* Add the address to the address manager if it exists. */
       if(pAddressManager)
          pAddressManager->AddAddress(addrConnect, ConnectState::CONNECTED);

       return true;
   }


   /*  Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t Server<ProtocolType>::GetConnectionCount()
    {
        uint32_t nConnectionCount = 0;
        uint16_t nThread = 0;

        for(; nThread < MAX_THREADS; ++nThread)
            nConnectionCount += DATA_THREADS[nThread]->GetConnectionCount();

        return nConnectionCount;
    }


    /*  Get the best connection based on latency. */
    template <class ProtocolType>
    memory::atomic_ptr<ProtocolType>& Server<ProtocolType>::GetConnection(const std::pair<uint32_t, uint32_t>& pairExclude)
    {
        /* List of connections to return. */
        uint32_t nLatency   = std::numeric_limits<uint32_t>::max();

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
                    memory::atomic_ptr<ProtocolType>& CONNECTION = DATA_THREADS[nThread]->CONNECTIONS->at(nIndex);
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
        static memory::atomic_ptr<ProtocolType> pNULL;
        if(nRetThread == -1 || nRetIndex == -1)
            return pNULL;

        return DATA_THREADS[nRetThread]->CONNECTIONS->at(nRetIndex);
    }


    /* Get the best connection based on data thread index. */
    template<class ProtocolType>
    memory::atomic_ptr<ProtocolType>& Server<ProtocolType>::GetConnection(const uint32_t nDataThread, const uint32_t nDataIndex)
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
        for(uint16_t index = 0; index < MAX_THREADS; ++index)
            DATA_THREADS[index]->DisconnectAll();
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

        /* Get the max connections. Default is 16 if maxconnections isn't specified. */
        uint32_t nMaxConnections = static_cast<uint32_t>(config::GetArg(std::string("-maxconnections"), 16));

        /* Loop connections. */
        while(!config::fShutdown.load())
        {
            /* Sleep between connection attempts. */
            runtime::sleep(5000);

            /* Pick a weighted random priority from a sorted list of addresses. */
            if(GetConnectionCount() < nMaxConnections && pAddressManager->StochasticSelect(addr))
            {
                /* Check for invalid address */
                if(!addr.IsValid())
                {
                    runtime::sleep(nSleepTime);
                    debug::log(3, FUNCTION, ProtocolType::Name(), " Invalid address, removing address", addr.ToString());
                    pAddressManager->Ban(addr);
                    continue;
                }

                /* Attempt the connection. */
                debug::log(3, FUNCTION, ProtocolType::Name(), " Attempting Connection ", addr.ToString());
                if(AddConnection(addr.ToStringIP(), addr.GetPort()))
                {
                    /* If address is DNS, log message on connection. */
                    std::string dns_name;
                    if(pAddressManager->GetDNSName(addr, dns_name))
                        debug::log(3, FUNCTION, "Connected to DNS Address: ", dns_name);

                    /* Sleep in 1 second intervals for easy break on shutdown. */
                    for(int i = 0; i < (nSleepTime / 1000) && !config::fShutdown.load(); ++i)
                        runtime::sleep(1000);
                }
            }

            debug::log(3, FUNCTION, ProtocolType::Name(), " ", pAddressManager->ToString());
        }
    }


    /*  Determine the first thread with the least amount of active connections.
     *  This keeps them load balanced across all server threads. */
    template <class ProtocolType>
    int32_t Server<ProtocolType>::FindThread()
    {
        int32_t nIndex = -1;
        uint32_t nConnections = std::numeric_limits<uint32_t>::max();

        for(uint16_t index = 0; index < MAX_THREADS; ++index)
        {
            DataThread<ProtocolType> *dt = DATA_THREADS[index];

            /* Limit data threads to 32 connections per thread. */
            if(dt->nConnections < nConnections)
            {
                nIndex = index;
                nConnections = dt->nConnections;
            }
        }

        return nIndex;
    }


    /*  Main Listening Thread of LLP Server. Handles new Connections and
     *  DDOS associated with Connection if enabled. */
    template <class ProtocolType>
    void Server<ProtocolType>::ListeningThread(bool fIPv4)
    {
        SOCKET hSocket;
        BaseAddress addr;
        socklen_t len_v4 = sizeof(struct sockaddr_in);
        socklen_t len_v6 = sizeof(struct sockaddr_in6);

        /* Setup poll objects. */
        pollfd fds[1];
        fds[0].events = POLLIN;
        fds[0].fd     = (fIPv4 ? hListenSocket.first : hListenSocket.second);

        /* Main listener loop. */
        while(!config::fShutdown.load())
        {
            if ((fIPv4 ? hListenSocket.first : hListenSocket.second) != INVALID_SOCKET)
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
                    continue;

                if(!(fds[0].revents & POLLIN))
                    continue;

                if(fIPv4)
                {
                    struct sockaddr_in sockaddr;

                    hSocket = accept(hListenSocket.first, (struct sockaddr*)&sockaddr, &len_v4);
                    if (hSocket != INVALID_SOCKET)
                        addr = BaseAddress(sockaddr);
                }
                else
                {
                    struct sockaddr_in6 sockaddr;

                    hSocket = accept(hListenSocket.second, (struct sockaddr*)&sockaddr, &len_v6);
                    if (hSocket != INVALID_SOCKET)
                        addr = BaseAddress(sockaddr);
                }

                if(hSocket == INVALID_SOCKET)
                {
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                        debug::error(FUNCTION, "Socket error accept failed: ", WSAGetLastError());
                }
                else
                {
                    /* Create new DDOS Filter if Needed. */
                    if(!DDOS_MAP.count(addr))
                        DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);

                    Socket sockNew(hSocket, addr);

                    /* DDOS Operations: Only executed when DDOS is enabled. */
                    if((fDDOS && DDOS_MAP[addr]->Banned()))
                    {
                        debug::log(3, FUNCTION, "Incoming Connection Request ",  addr.ToString(), " refused... Banned.");
                        sockNew.Close();

                        continue;
                    }
                    else if(!CheckPermissions(addr.ToStringIP(), PORT))
                    {
                        debug::log(3, FUNCTION, "Connection Request ",  addr.ToString(), " refused... Denied by allowip whitelist.");

                        closesocket(hSocket);

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
                    debug::log(3, FUNCTION, "Accepted Connection ", addr.ToString(), " on port ",  PORT);


                    /* Update the address state. */
                    if(pAddressManager)
                        pAddressManager->AddAddress(addr, ConnectState::CONNECTED);
                }
            }
        }

        closesocket(fIPv4 ? hListenSocket.first : hListenSocket.second);
    }


    /*  Bind connection to a listening port. */
    template <class ProtocolType>
    bool Server<ProtocolType>::BindListenPort(int32_t & hListenSocket, bool fIPv4, bool fRemote)
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

            sockaddr.sin_port = htons(PORT);
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
       if(!config::GetBoolArg("-meters", false))
           return;

       runtime::timer TIMER;
       TIMER.Start();

       while(!config::fShutdown.load())
       {
           runtime::sleep(100);
           if(TIMER.Elapsed() < 10)
               continue;

           uint32_t nGlobalConnections = 0;
           for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
           {
               DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

               nGlobalConnections += dt->nConnections;
           }


           uint32_t RPS = TotalRequests() / TIMER.Elapsed();
           debug::log(0, FUNCTION, "LLP Running at ", RPS, " requests/s with ", nGlobalConnections, " connections.");

           TIMER.Reset();
           ClearRequests();
       }

       debug::log(0, FUNCTION, "Meter closed..");
    }


    /*  Used for Meter. Adds up the total amount of requests from each
     *  Data Thread. */
    template <class ProtocolType>
    uint32_t Server<ProtocolType>::TotalRequests()
    {
       uint32_t nTotalRequests = 0;
       for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
       {
           DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

           nTotalRequests += dt->REQUESTS;
       }

       return nTotalRequests;
    }


    /*  Used for Meter. Resets the REQUESTS variable to 0 in each Data Thread. */
    template <class ProtocolType>
    void Server<ProtocolType>::ClearRequests()
    {
        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

            dt->REQUESTS = 0;
        }
    }


    /* UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding. */
    template <class ProtocolType>
    void Server<ProtocolType>::UPnP()
    {
#ifndef USE_UPNP
        return;
#endif

        if(!config::GetBoolArg("-upnp", true))
            return;

        char port[6];
        sprintf(port, "%d", PORT);

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
                debug::log(1, "UPnP Port Mapping successful.");
            
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
                        debug::log(1, "UPnP Port Mapping successful.");
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
    }




    /* Explicity instantiate all template instances needed for compiler. */
    template class Server<TritiumNode>;
    template class Server<LegacyNode>;
    template class Server<TimeNode>;
    template class Server<APINode>;
    template class Server<RPCNode>;
    template class Server<Miner>;
}
