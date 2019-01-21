/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_SERVER_H
#define NEXUS_LLP_TEMPLATES_SERVER_H

#include <LLP/templates/data.h>
#include <LLP/include/permissions.h>
#include <LLP/include/manager.h>
#include <LLP/include/address.h>
#include <LLP/include/addressinfo.h>

#include <functional>
#include <numeric>

namespace LLP
{

    /** Base Class to create a Custom LLP Server.
     *
     *  Protocol Type class must inherit Connection,
     *  and provide a ProcessPacket method.
     *  Optional Events by providing GenericEvent method.
     *
     **/
    template <class ProtocolType>
    class Server
    {
    private:
        /* The DDOS variables. */
        std::map<Service, DDOS_Filter *> DDOS_MAP;
        bool fDDOS;
        bool fLISTEN;
        bool fMETER;

        std::atomic<bool> fDestruct;

    public:
        uint32_t PORT;
        uint32_t MAX_THREADS;
        uint32_t DDOS_TIMESPAN;

        /* The data type to keep track of current running threads. */
        std::vector<DataThread<ProtocolType> *> DATA_THREADS;


        /* Connection Manager. */
        std::thread MANAGER_THREAD;


        /* Address manager */
        AddressManager *pAddressManager;


        /* Address of the server node */
        Address addrThisNode;

        /* returns the name of the protocol type of this server */
        std::string Name()
        {
            return ProtocolType::Name();
        }

        Server<ProtocolType>(int32_t nPort, int32_t nMaxThreads, int32_t nTimeout = 30, bool isDDOS = false,
                             int32_t cScore = 0, int32_t rScore = 0, int32_t nTimespan = 60, bool fListen = true,
                             bool fMeter = false, bool fManager = false)
        : fDDOS(isDDOS)
        , fLISTEN(fListen)
        , fMETER(fMeter)
        , fDestruct(false)
        , PORT(nPort)
        , MAX_THREADS(nMaxThreads)
        , DDOS_TIMESPAN(nTimespan)
        , DATA_THREADS(0)
        , pAddressManager(0)
        , addrThisNode()
        {
            for(int32_t index = 0; index < MAX_THREADS; ++index)
                DATA_THREADS.push_back(new DataThread<ProtocolType>(
                    index, fDDOS, rScore, cScore, nTimeout, fMeter));

            if(fManager)
            {
                pAddressManager = new AddressManager(nPort);
                pAddressManager->ReadDatabase();

                MANAGER_THREAD = std::thread((std::bind(&Server::Manager, this)));
            }

            LISTEN_THREAD_V4 = std::thread(std::bind(&Server::ListeningThread, this, true));  //IPv4 Listener
            LISTEN_THREAD_V6 = std::thread(std::bind(&Server::ListeningThread, this, false)); //IPv6 Listener
            METER_THREAD = std::thread(std::bind(&Server::Meter, this));
        }


        virtual ~Server<ProtocolType>()
        {
            /* Set all flags back to false. */
            fLISTEN = false;
            fMETER  = false;
            fDDOS   = false;

            /* Start the destructor. */
            fDestruct = true;

            /* Wait for address manager. */
            if(pAddressManager)
                MANAGER_THREAD.join();

            /* Wait for meter thread. */
            METER_THREAD.join();

            /* Wait for listeners. */
            LISTEN_THREAD_V4.join();
            LISTEN_THREAD_V6.join();

            /* Delete the data threads. */
            for(int32_t index = 0; index < MAX_THREADS; ++index)
            {
                delete DATA_THREADS[index];
                DATA_THREADS[index] = 0;
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
                delete pAddressManager;
        }


        /** Shutdown
         *
         *  Cleanup and shutdown subsystems
         *
         **/
        void Shutdown()
        {
            if(pAddressManager)
                pAddressManager->WriteDatabase();
        }


        /** AddNode
         *
         *  Add a node address to the internal address manager
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *
         *  @param[in] strPort		Port of outgoing connection
         *
         **/
        void AddNode(std::string strAddress, uint16_t nPort)
        {
            Service service(debug::strprintf("%s:%u", strAddress.c_str(), nPort).c_str(), false);
            Address addr = Address(service);

            if(pAddressManager)
                pAddressManager->AddAddress(addr, ConnectState::NEW);
        }


        /** AddConnection
         *
         *  Public Wraper to Add a Connection Manually.
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *
         *  @param[in] strPort		Port of outgoing connection
         *
         *  @return	Returns true if the connection was established successfully
         *
         **/
        bool AddConnection(std::string strAddress, int32_t nPort)
        {
            /* Initialize DDOS Protection for Incoming IP Address. */
            Service addrConnect(debug::strprintf("%s:%i", strAddress.c_str(), nPort).c_str(), false);

            /* Create new DDOS Filter if Needed. */
            if(!DDOS_MAP.count(addrConnect))
                DDOS_MAP[addrConnect] = new DDOS_Filter(DDOS_TIMESPAN);

            /* DDOS Operations: Only executed when DDOS is enabled. */
            if((fDDOS && DDOS_MAP[addrConnect]->Banned()))
                return false;

            /* Find a balanced Data Thread to Add Connection to. */
            int32_t nThread = FindThread();
            if(nThread < 0)
                return false;

            /* Select the proper data thread. */
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];
            if(!dt)
                return false;

            /* Attempt the connection. */
            if(!dt->AddConnection(strAddress, nPort, DDOS_MAP[addrConnect]))
                return false;

            return true;
        }


        /** GetConnections
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector
         *
         **/
        std::vector<ProtocolType*> GetConnections()
        {
            /* List of connections to return. */
            std::vector<ProtocolType *> vConnections;
            for(int32_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            {
                /* Get the data threads. */
                DataThread<ProtocolType> *dt = DATA_THREADS[nThread];
                if(!dt)
                    continue;

                /* Loop through connections in data thread. */
                int32_t nSize = dt->CONNECTIONS.size();
                for(int32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /* Skip over inactive connections. */
                    if(!dt->CONNECTIONS[nIndex] ||
                       !dt->CONNECTIONS[nIndex]->Connected())
                        continue;

                    /* Push the active connection. */
                    vConnections.push_back(dt->CONNECTIONS[nIndex]);
                }
            }

            return vConnections;
        }


        /** GetAddresses
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector
         *
         **/
        std::vector<Address> GetAddresses()
        {
            std::vector<Address> vAddr;

            if(pAddressManager)
                vAddr = pAddressManager->GetAddresses();

            return vAddr;
        }


        /** Manager
         *
         *  Connection Manager Thread.
         *
         **/
        void Manager()
        {
            
            /* Address to select. */
            Address addr;

            /* Loop connections. */
            while(!fDestruct.load())
            {
                runtime::sleep(1000);

                /* assume the connect state is in a failed state */
                uint8_t state = static_cast<uint8_t>(ConnectState::FAILED);

                /* Pick a weighted random priority from a sorted list of addresses */
                if(pAddressManager && pAddressManager->StochasticSelect(addr))
                {

                    /* Check for connect to self. */
                    if(addr.ToStringIP() == addrThisNode.ToStringIP())
                    {
                        runtime::sleep(1000);

                        continue;
                    }

                    /* Get the IP in proper type. */
                    std::string ip = addr.ToStringIP();
                    uint16_t port = addr.GetPort();

                    /* Attempt the connection. */
                    debug::log(0, FUNCTION, ProtocolType::Name(), " Attempting Connection ", ip, ":", port);
                    pAddressManager->PrintStats();

                    if(AddConnection(ip, port))
                        state = static_cast<uint8_t>(ConnectState::CONNECTED);

                    /* Update the address state. */
                    pAddressManager->AddAddress(addr, state);
                }
            }
        }


    private:

        /* Basic Socket Handle Variables. */
        std::thread          LISTEN_THREAD_V4;
        std::thread          LISTEN_THREAD_V6;
        std::thread          METER_THREAD;


        /* Determine thfalsee thread with the least amount of active connections.
            This keeps the load balanced across all server threads. */
        int32_t FindThread()
        {
            int32_t nIndex = -1;
            int32_t nConnections = std::numeric_limits<int32_t>::max();

            for(int32_t index = 0; index < MAX_THREADS; ++index)
            {
                DataThread<ProtocolType> *dt = DATA_THREADS[index];

                if(!dt)
                    continue;

                if(dt->nConnections < nConnections)
                {
                    nIndex = index;
                    nConnections = dt->nConnections;
                }
            }

            return nIndex;
        }


        /** Main Listening Thread of LLP Server. Handles new Connections and DDOS associated with Connection if enabled. **/
        void ListeningThread(bool fIPv4)
        {
            int32_t hListenSocket;
            SOCKET hSocket;
            Address addr;
            socklen_t len_v4 = sizeof(struct sockaddr_in);
            socklen_t len_v6 = sizeof(struct sockaddr_in6);

            /* End the listening thread if LLP set to not listen. */
            if(!fLISTEN)
                return;

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
            while(!fDestruct.load())
            {
                if (hListenSocket != INVALID_SOCKET)
                {
                    /* Poll the sockets. */
                    int nPoll = poll(&fds[0], 1, 100);
                    if(nPoll < 0)
                        continue;

                    if(!(fds[0].revents & POLLIN))
                        continue;

                    if(fIPv4)
                    {
                        struct sockaddr_in sockaddr;

                        hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len_v4);
                        if (hSocket != INVALID_SOCKET)
                            addr = Address(sockaddr);
                    }
                    else
                    {
                        struct sockaddr_in6 sockaddr;

                        hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len_v6);
                        if (hSocket != INVALID_SOCKET)
                            addr = Address(sockaddr);
                    }

                    if (hSocket == INVALID_SOCKET)
                    {
                        if (GetLastError() != WSAEWOULDBLOCK)
                            debug::error("socket error accept failed: ", GetLastError());
                    }
                    else
                    {
                        /* Create new DDOS Filter if Needed. */
                        if(!DDOS_MAP.count((Service)addr))
                            DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);

                        /* DDOS Operations: Only executed when DDOS is enabled. */
                        if((fDDOS && DDOS_MAP[(Service)addr]->Banned()))
                        {
                            debug::log(0, FUNCTION, "Connection Request ",  addr.ToString(), " refused... Banned.");
                            close(hSocket);

                            continue;
                        }

                        Socket_t sockNew(hSocket, addr);

                        int32_t nThread = FindThread();

                        if(nThread < 0)
                            continue;

                        DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

                        if(!dt)
                            continue;

                        dt->AddConnection(sockNew, DDOS_MAP[(Service)addr]);
                        debug::log(3, FUNCTION, "Accepted Connection ", addr.ToString(), " on port ",  PORT);
                    }
                }
            }
        }

        bool BindListenPort(int32_t & hListenSocket, bool fIPv4 = true)
        {
            std::string strError = "";
            int32_t nOne = 1;

            #ifdef WIN32
                // Initialize Windows Sockets
                WSADATA wsadata;
                int32_t ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
                if (ret != NO_ERROR)
                {
                    debug::error("TCP/IP socket library failed to start (WSAStartup returned error )", ret);
                    return false;
                }
            #endif

            /* Create socket for listening for incoming connections */
            hListenSocket = socket(fIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (hListenSocket == INVALID_SOCKET)
            {
                debug::error("Couldn't open socket for incoming connections (socket returned error )", GetLastError());
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
                    int32_t nErr = GetLastError();
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
                    int32_t nErr = GetLastError();
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
                debug::error("Listening for incoming connections failed (listen returned error )", GetLastError());
                return false;
            }

            return true;
        }


        /* LLP Meter Thread. Tracks the Requests / Second. */
        void Meter()
        {
            if(!config::GetBoolArg("-meters", false))
                return;

            if(!fMETER)
                return;

            runtime::timer TIMER;
            TIMER.Start();

            while(!fDestruct.load())
            {
                runtime::sleep(100);
                if(TIMER.Elapsed() < 10)
                    continue;

                uint32_t nGlobalConnections = 0;
                for(int32_t nThread = 0; nThread < MAX_THREADS; ++nThread)
                {
                    DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

                    if(!dt)
                        continue;

                    nGlobalConnections += dt->nConnections;
                }


                uint32_t RPS = TotalRequests() / TIMER.Elapsed();
                debug::log(0, FUNCTION, "LLP Running at ", RPS, " requests/s with ", nGlobalConnections, " connections.");

                TIMER.Reset();
                ClearRequests();
            }

            printf("Meter closed..\n");
        }


        /** Used for Meter. Adds up the total amount of requests from each Data Thread. **/
        int32_t TotalRequests()
        {
            int32_t nTotalRequests = 0;
            for(int32_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            {
                DataThread<ProtocolType> *dt = DATA_THREADS[nThread];
                if(!dt)
                    continue;

                nTotalRequests += dt->REQUESTS;
            }

            return nTotalRequests;
        }

        /** Used for Meter. Resets the REQUESTS variable to 0 in each Data Thread. **/
        void ClearRequests()
        {
            for(int32_t nThread = 0; nThread < MAX_THREADS; ++nThread)
            {
                DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

                if(!dt)
                    continue;

                dt->REQUESTS = 0;
            }
        }
    };

}

#endif
