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
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <LLP/include/manager.h>
#include <LLP/include/trust_address.h>

#include <Util/include/args.h>
#include <functional>
#include <numeric>

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
    Server<ProtocolType>::Server(uint16_t nPort, uint16_t nMaxThreads, uint32_t nTimeout, bool isDDOS,
                         uint32_t cScore, uint32_t rScore, uint32_t nTimespan, bool fListen,
                         bool fMeter, bool fManager, uint32_t nSleepTimeIn)
    : fDDOS(isDDOS)
    , MANAGER()
    , PORT(nPort)
    , MAX_THREADS(nMaxThreads)
    , DDOS_TIMESPAN(nTimespan)
    , DATA_THREADS()
    , pAddressManager(nullptr)
    , nSleepTime(nSleepTimeIn)
    {
        for(uint16_t index = 0; index < MAX_THREADS; ++index)
        {
            DATA_THREADS.push_back(new DataThread<ProtocolType>(
                index, fDDOS, rScore, cScore, nTimeout, fMeter));
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
            LISTEN_THREAD_V4 = std::thread(std::bind(&Server::ListeningThread, this, true));  //IPv4 Listener
            LISTEN_THREAD_V6 = std::thread(std::bind(&Server::ListeningThread, this, false)); //IPv6 Listener
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
            MANAGER.notify_all();

            if(MANAGER_THREAD.joinable())
                MANAGER_THREAD.join();
        }

        /* Wait for meter thread. */
        if(METER_THREAD.joinable())
            METER_THREAD.join();

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
            {
                delete it->second;
            }
        }
        DDOS_MAP.clear();

        /* Clear the address manager. */
        if(pAddressManager)
        {
            delete pAddressManager;
            pAddressManager = nullptr;
        }

    }


     /*  Cleanup and shutdown subsystems */
    template <class ProtocolType>
    void Server<ProtocolType>::Shutdown()
    {
        if(pAddressManager)
          pAddressManager->WriteDatabase();
    }


   /*  Add a node address to the internal address manager */
   template <class ProtocolType>
   void Server<ProtocolType>::AddNode(std::string strAddress, uint16_t nPort)
   {
       BaseAddress addr(strAddress, nPort, false);

       /* Make sure manager is enabled. */
       if(pAddressManager)
          pAddressManager->AddAddress(addr, ConnectState::NEW);
   }


   /*  Public Wraper to Add a Connection Manually. */
   template <class ProtocolType>
   bool Server<ProtocolType>::AddConnection(std::string strAddress, uint16_t nPort)
   {
       /* Initialize DDOS Protection for Incoming IP Address. */
       BaseAddress addrConnect(strAddress, nPort);

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
       if(!dt->AddConnection(strAddress, nPort, DDOS_MAP[addrConnect]))
           return false;

       return true;
   }


   /*  Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t Server<ProtocolType>::GetConnectionCount()
    {
        uint32_t nConnectionCount = 0;

        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            /* Get the data threads. */
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

            /* Lock the data thread. */
            uint16_t nSize = 0;
            {
                LOCK(dt->MUTEX);
                
                nSize = static_cast<uint16_t>(dt->CONNECTIONS.size());
            }

            /* Loop through connections in data thread and add any that are connected to count. */
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Skip over inactive connections. */
                if(dt->CONNECTIONS[nIndex] == nullptr)
                    continue;

                ++nConnectionCount;
            }
        }

        return nConnectionCount;
    }


    /*  Get the best connection based on latency */
    template <class ProtocolType>
    memory::atomic_ptr<ProtocolType>& Server<ProtocolType>::GetConnection(const BaseAddress& addrExclude)
    {
        /* List of connections to return. */
        uint32_t nLatency   = std::numeric_limits<uint32_t>::max();
        uint16_t nRetThread = 0;
        uint16_t nRetIndex  = 0;

        for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
        {
            /* Get the data threads. */
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

            /* Lock the data thread. */
            uint16_t nSize = 0;
            {
                LOCK(dt->MUTEX);

                nSize = static_cast<uint16_t>(dt->CONNECTIONS.size());
            }

            /* Loop through connections in data thread. */
            for(uint16_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                /* Skip over inactive connections. */
                if(dt->CONNECTIONS[nIndex] == nullptr)
                    continue;

                /* Skip over exclusion address. */
                if(dt->CONNECTIONS[nIndex]->GetAddress() == addrExclude)
                    continue;

                /* Push the active connection. */
                if(dt->CONNECTIONS[nIndex]->nLatency < nLatency)
                {
                    nLatency = dt->CONNECTIONS[nIndex]->nLatency;

                    nRetThread = nThread;
                    nRetIndex  = nIndex;
                }
            }
        }

        return DATA_THREADS[nRetThread]->CONNECTIONS[nRetIndex];
    }


    /*  Get the active connection pointers from data threads. */
    template <class ProtocolType>
    std::vector<LegacyAddress> Server<ProtocolType>::GetAddresses()
    {
        std::vector<BaseAddress> vAddr;
        std::vector<LegacyAddress> vLegacyAddr;

        if(pAddressManager)
        {
            /* Get the base addresses from address manager and convert
            into legacy addresses */
            pAddressManager->GetAddresses(vAddr);

            for(auto it = vAddr.begin(); it != vAddr.end(); ++it)
                vLegacyAddr.push_back((LegacyAddress)*it);
        }

        return vLegacyAddr;
    }


    /*  Notifies all data threads to disconnect their connections */
    template <class ProtocolType>
    void Server<ProtocolType>::DisconnectAll()
    {
        for(uint16_t index = 0; index < MAX_THREADS; ++index)
            DATA_THREADS[index]->DisconnectAll();
    }


     /*  Address Manager Thread. */
    template <class ProtocolType>
    void Server<ProtocolType>::Manager()
    {
        /* If manager is disabled, close down manager thread. */
        if(!pAddressManager)
            return;

        /* Address to select. */
        BaseAddress addr;

        /* Read the address database. */
        pAddressManager->ReadDatabase();

        /* Set the port. */
        pAddressManager->SetPort(PORT);


        /* Wait for data threads to startup. */
        while(DATA_THREADS.size() < MAX_THREADS)
            runtime::sleep(1000);

        /* Loop connections. */
        while(!config::fShutdown.load())
        {
            /* Sleep in 1 second intervals for easy break on shutdown. */
            for(int i = 0; i < (nSleepTime / 1000) && !config::fShutdown.load(); ++i)
                runtime::sleep(1000);

            /* Pick a weighted random priority from a sorted list of addresses. */
            if(pAddressManager->StochasticSelect(addr))
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
                    pAddressManager->AddAddress(addr, ConnectState::CONNECTED);
                else
                    pAddressManager->AddAddress(addr, ConnectState::FAILED);
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
            if(dt->nConnections < nConnections && dt->nConnections < 32)
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
        int32_t hListenSocket = 0;
        SOCKET hSocket;
        BaseAddress addr;
        socklen_t len_v4 = sizeof(struct sockaddr_in);
        socklen_t len_v6 = sizeof(struct sockaddr_in6);

        /* Bind the Listener. */
        if(!BindListenPort(hListenSocket, fIPv4))
            return;

        /* Don't listen until all data threads are created. */
        while(DATA_THREADS.size() < MAX_THREADS)
            runtime::sleep(1000);

        /* Setup poll objects. */
        pollfd fds[1];
        fds[0].events = POLLIN;
        fds[0].fd     = hListenSocket;

        /* Main listener loop. */
        while(!config::fShutdown.load())
        {
            if (hListenSocket != INVALID_SOCKET)
            {
                /* Poll the sockets. */
#ifdef WIN32
                int nPoll = WSAPoll(&fds[0], 1, 100);
#else
                int nPoll = poll(&fds[0], 1, 100);
#endif

                if(nPoll < 0)
                    continue;

                if(!(fds[0].revents & POLLIN))
                    continue;

                if(fIPv4)
                {
                    struct sockaddr_in sockaddr;

                    hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len_v4);
                    if (hSocket != INVALID_SOCKET)
                        addr = BaseAddress(sockaddr);
                }
                else
                {
                    struct sockaddr_in6 sockaddr;

                    hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len_v6);
                    if (hSocket != INVALID_SOCKET)
                        addr = BaseAddress(sockaddr);
                }

                if (hSocket == INVALID_SOCKET)
                {
                    if (WSAGetLastError() != WSAEWOULDBLOCK)
                        debug::error("socket error accept failed: ", WSAGetLastError());
                }
                else
                {
                    /* Create new DDOS Filter if Needed. */
                    if(!DDOS_MAP.count(addr))
                        DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);

                    /* DDOS Operations: Only executed when DDOS is enabled. */
                    if((fDDOS && DDOS_MAP[addr]->Banned()))
                    {
                        debug::log(3, FUNCTION, "Connection Request ",  addr.ToString(), " refused... Banned.");

                        closesocket(hSocket);

                        continue;
                    }

                    Socket sockNew(hSocket, addr);

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
    }


    /*  Bind connection to a listening port. */
    template <class ProtocolType>
    bool Server<ProtocolType>::BindListenPort(int32_t & hListenSocket, bool fIPv4)
    {
        std::string strError = "";
        /* Conditional declaration to avoid "unused variable" */
#if !defined WIN32 || defined SO_NOSIGPIPE
        int32_t nOne = 1;
#endif

        /* Create socket for listening for incoming connections */
        hListenSocket = socket(fIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (hListenSocket == INVALID_SOCKET)
        {
            debug::error("Couldn't open socket for incoming connections (socket returned error )", WSAGetLastError());
            return false;
        }

        /* Different way of disabling SIGPIPE on BSD */
#ifdef SO_NOSIGPIPE
        setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int32_t));
#endif


        /* Allow binding if the port is still in TIME_WAIT state after the program was closed and restarted.  Not an issue on windows. */
#ifndef WIN32
        setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int32_t));
        setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&nOne, sizeof(int32_t));
#endif


        /* The sockaddr_in structure specifies the address family, IP address, and port for the socket that is being bound */
        if(fIPv4)
        {
            struct sockaddr_in sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_addr.s_addr = INADDR_ANY; // bind to all IPs on this computer
            sockaddr.sin_port = htons(PORT);
            if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEADDRINUSE)
                    debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), " on this computer. Address already in use.");
                else
                    debug::error("Unable to bind to port ", ntohs(sockaddr.sin_port), " on this computer (bind returned error )",  nErr);

                return false;
            }

            debug::log(0, FUNCTION,"(v4) Bound to port ", ntohs(sockaddr.sin_port));
        }
        else
        {
            struct sockaddr_in6 sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin6_family = AF_INET6;
            sockaddr.sin6_addr = IN6ADDR_ANY_INIT; // bind to all IPs on this computer
            sockaddr.sin6_port = htons(PORT);
            if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEADDRINUSE)
                    debug::error("Unable to bind to port ", ntohs(sockaddr.sin6_port), " on this computer. Address already in use.");
                else
                    debug::error("Unable to bind to port ", ntohs(sockaddr.sin6_port), " on this computer (bind returned error )",  nErr);

                return false;
            }

            debug::log(0, FUNCTION, "(v6) Bound to port ", ntohs(sockaddr.sin6_port));
        }

        /* Listen for incoming connections */
        if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            debug::error("Listening for incoming connections failed (listen returned error )", WSAGetLastError());
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


    /* Explicity instantiate all template instances needed for compiler. */
    template class Server<TritiumNode>;
    template class Server<LegacyNode>;
    template class Server<TimeNode>;
    template class Server<CoreNode>;
    template class Server<RPCNode>;
    template class Server<Miner>;

}
