/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_SERVER_H
#define NEXUS_LLP_TEMPLATES_SERVER_H

#include <functional>

#include <LLP/templates/data.h>
#include <LLP/include/permissions.h>

namespace LLP
{

    /** Base Class to create a Custom LLP Server. Protocol Type class must inherit Connection,
    * and provide a ProcessPacket method. Optional Events by providing GenericEvent method.  */
    template <class ProtocolType> class Server
    {
        /* The DDOS variables. Tracks the Requests and Connections per Second
            from each connected address. */
        std::map<CService,   DDOS_Filter*> DDOS_MAP;
        bool fDDOS, fLISTEN, fMETER;

    public:
        uint32_t PORT, MAX_THREADS, DDOS_TIMESPAN;

        /* The data type to keep track of current running threads. */
        std::vector< DataThread<ProtocolType>* > DATA_THREADS;

        /* List of internal addresses. */
        std::recursive_mutex MUTEX;
        std::vector<CAddress> vAddr;

        /* Connection Manager. */
        std::thread MANAGER;

        /* Address of this instance. */
        CAddress addrThisNode;


        Server<ProtocolType>(int nPort, int nMaxThreads, int nTimeout = 30, bool isDDOS = false, int cScore = 0, int rScore = 0, int nTimespan = 60, bool fListen = true, bool fMeter = false) :
            fDDOS(isDDOS),
            fLISTEN(fListen),
            fMETER(fMeter),
            PORT(nPort),
            MAX_THREADS(nMaxThreads),
            DDOS_TIMESPAN(nTimespan),
            DATA_THREADS(0),
            MANAGER(std::bind(&Server::Manager, this)),
            addrThisNode(),
            LISTEN_THREAD_V4(std::bind(&Server::ListeningThread, this, true)),  //IPv4 Listener
            LISTEN_THREAD_V6(std::bind(&Server::ListeningThread, this, false)), //IPv6 Listener
            METER_THREAD(std::bind(&Server::Meter, this))
        {
            for(int index = 0; index < MAX_THREADS; index++)
                DATA_THREADS.push_back(new DataThread<ProtocolType>(index, fDDOS, rScore, cScore, nTimeout, fMeter));
        }


        virtual ~Server<ProtocolType>()
        {

            fLISTEN = false;
            fMETER  = false;

            for(int index = 0; index < MAX_THREADS; index++)
                delete DATA_THREADS[index];
        }


        /** Public Wraper to Add a Connection Manually.
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[in] strPort		Port of outgoing connection
         *
         *  @return	Returns true if the connection was established successfully
         *
         **/
        bool AddConnection(std::string strAddress, int nPort)
        {
            /* Initialize DDOS Protection for Incoming IP Address. */
            CService addrConnect(debug::strprintf("%s:%i", strAddress.c_str(), nPort).c_str(), false);

            /* Create new DDOS Filter if Needed. */
            if(!DDOS_MAP.count(addrConnect))
                DDOS_MAP[addrConnect] = new DDOS_Filter(DDOS_TIMESPAN);

            /* DDOS Operations: Only executed when DDOS is enabled. */
            if((fDDOS && DDOS_MAP[addrConnect]->Banned()))
                return false;

            /* Find a balanced Data Thread to Add Connection to. */
            int nThread = FindThread();
            if(!DATA_THREADS[nThread]->AddConnection(strAddress, nPort, DDOS_MAP[addrConnect]))
                return false;

            return true;
        }


        /** Add Address
         *
         *  Add a connection to internal list.
         *
         *  @param[in] addr The address to add
         *
         **/
        void AddAddress(CAddress addr)
        {
            LOCK(MUTEX);

            /* Set default port */
            addr.SetPort(config::GetArg("-port", config::fTestNet ? 8888 : 9888));

            if(addrThisNode != addr)
            {
                vAddr.push_back(addr);

                debug::log(1, FUNCTION "added address %s\n", __PRETTY_FUNCTION__, addr.ToString().c_str());
            }
        }


        /** Get Connections
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector
         *
         **/
        std::vector<ProtocolType*> GetConnections()
        {
            std::vector<ProtocolType*> vConnections;
            for(int nThread = 0; nThread < MAX_THREADS; nThread++)
            {
                int nSize = DATA_THREADS[nThread]->CONNECTIONS.size();
                for(int nIndex = 0; nIndex < nSize; nIndex ++)
                {
                    if(!DATA_THREADS[nThread]->CONNECTIONS[nIndex] || !DATA_THREADS[nThread]->CONNECTIONS[nIndex]->Connected())
                        continue;

                    vConnections.push_back(DATA_THREADS[nThread]->CONNECTIONS[nIndex]);
                }
            }

            return vConnections;
        }


        /** Get Addresses
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector
         *
         **/
        std::vector<CAddress> GetAddresses()
        {
            std::vector<CAddress> vAddr;
            for(int nThread = 0; nThread < MAX_THREADS; nThread++)
            {
                int nSize = DATA_THREADS[nThread]->CONNECTIONS.size();
                for(int nIndex = 0; nIndex < nSize; nIndex ++)
                {
                    if(!DATA_THREADS[nThread]->CONNECTIONS[nIndex] || !DATA_THREADS[nThread]->CONNECTIONS[nIndex]->Connected())
                        continue;

                    vAddr.push_back(DATA_THREADS[nThread]->CONNECTIONS[nIndex]->GetAddress());
                }
            }

            return vAddr;
        }


        /** Manager
         *
         *  Connection Manager Thread.
         *
         **/
        void Manager()
        {
            /* Loop connections. */
            while(!config::fShutdown)
            {
                Sleep(1000);
                if(vAddr.empty())
                    continue;

                { LOCK(MUTEX);

                    /* Get list of currently connected addresses. */
                    std::vector<CAddress> vConnected = GetAddresses();

                    /* Randomize the selection. */
                    std::random_shuffle(vAddr.begin(), vAddr.end());
                    CAddress addr = vAddr.back();
                    vAddr.pop_back();

                    /* Check if the address is already connected. */
                    if(std::find(vConnected.begin(), vConnected.end(), addr) != vConnected.end())
                    {
                        debug::log(0, FUNCTION "Address already connected %s\n", __PRETTY_FUNCTION__, addr.ToStringIP().c_str());

                        continue;
                    }

                    /* Attempt the connection. */
                    debug::log(0, FUNCTION "Attempting Connection %s\n", __PRETTY_FUNCTION__, addr.ToStringIP().c_str());
                    AddConnection(addr.ToStringIP(), addr.GetPort());
                }
            }
        }


    private:

        /* Basic Socket Handle Variables. */
        std::thread          LISTEN_THREAD_V4;
        std::thread          LISTEN_THREAD_V6;

        std::thread          METER_THREAD;
        std::recursive_mutex DDOS_MUTEX;


        /* Determine the thread with the least amount of active connections.
            This keeps the load balanced across all server threads. */
        int FindThread()
        {
            int nIndex = 0, nConnections = DATA_THREADS[0]->nConnections;
            for(int index = 1; index < MAX_THREADS; index++)
            {
                if(DATA_THREADS[index]->nConnections < nConnections)
                {
                    nIndex = index;
                    nConnections = DATA_THREADS[index]->nConnections;
                }
            }

            return nIndex;
        }


        /** Main Listening Thread of LLP Server. Handles new Connections and DDOS associated with Connection if enabled. **/
        void ListeningThread(bool fIPv4)
        {
            int hListenSocket;

            /* End the listening thread if LLP set to not listen. */
            if(!fLISTEN)
                return;

            /* Bind the Listener. */
            if(!BindListenPort(hListenSocket, fIPv4))
                return;

            /* Don't listen until all data threads are created. */
            while(DATA_THREADS.size() < MAX_THREADS)
                Sleep(1000);

            /* Main listener loop. */
            while(true)
            {
                if (hListenSocket != INVALID_SOCKET)
                {
                    SOCKET hSocket;
                    CAddress addr;

                    if(fIPv4)
                    {
                        struct sockaddr_in sockaddr;
                        socklen_t len = sizeof(sockaddr);

                        hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
                        if (hSocket != INVALID_SOCKET)
                            addr = CAddress(sockaddr);
                    }
                    else
                    {
                        struct sockaddr_in6 sockaddr;
                        socklen_t len = sizeof(sockaddr);

                        hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
                        if (hSocket != INVALID_SOCKET)
                            addr = CAddress(sockaddr);
                    }

                    if (hSocket == INVALID_SOCKET)
                    {
                        if (GetLastError() != WSAEWOULDBLOCK)
                            debug::error("socket error accept failed: %d\n", GetLastError());
                    }
                    else
                    {

                        /* Create new DDOS Filter if Needed. */
                        if(!DDOS_MAP.count((CService)addr))
                            DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);

                        /* DDOS Operations: Only executed when DDOS is enabled. */
                        if((fDDOS && DDOS_MAP[(CService)addr]->Banned()))
                        {
                            debug::log(0, NODE "Connection Request %s refused... Banned.", addr.ToString().c_str());
                            close(hSocket);

                            continue;
                        }

                        Socket_t sockNew(hSocket, addr);

                        int nThread = FindThread();
                        DATA_THREADS[nThread]->AddConnection(sockNew, DDOS_MAP[(CService)addr]);

                        debug::log(3, NODE "Accepted Connection %s on port %u\n", addr.ToString().c_str(), PORT);
                    }
                }
            }
        }

        bool BindListenPort(int & hListenSocket, bool fIPv4 = true)
        {
            std::string strError = "";
            int nOne = 1;

            #ifdef WIN32
                // Initialize Windows Sockets
                WSADATA wsadata;
                int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
                if (ret != NO_ERROR)
                {
                    debug::error("TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);

                    return false;
                }
            #endif

            /* Create socket for listening for incoming connections */
            hListenSocket = socket(fIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (hListenSocket == INVALID_SOCKET)
            {
                debug::error("Couldn't open socket for incoming connections (socket returned error %d)", GetLastError());

                return false;
            }

            /* Different way of disabling SIGPIPE on BSD */
            #ifdef SO_NOSIGPIPE
                setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
            #endif


            /* Allow binding if the port is still in TIME_WAIT state after the program was closed and restarted.  Not an issue on windows. */
            #ifndef WIN32
                setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
                setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&nOne, sizeof(int));
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
                    int nErr = GetLastError();
                    if (nErr == WSAEADDRINUSE)
                        debug::error("Unable to bind to port %d on this computer. Address already in use.", ntohs(sockaddr.sin_port));
                    else
                        debug::error("Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin_port), nErr);

                    return false;
                }

                debug::log(0, NODE "(v4) Bound to port %d\n", ntohs(sockaddr.sin_port));
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
                    int nErr = GetLastError();
                    if (nErr == WSAEADDRINUSE)
                        debug::error("Unable to bind to port %d on this computer. Address already in use.", ntohs(sockaddr.sin6_port));
                    else
                        debug::error("Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin6_port), nErr);

                    return false;
                }

                debug::log(0, NODE "(v6) Bound to port %d\n", ntohs(sockaddr.sin6_port));
            }

            /* Listen for incoming connections */
            if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                debug::error("Listening for incoming connections failed (listen returned error %d)", GetLastError());

                return false;
            }

            return true;
        }


        /* LLP Meter Thread. Tracks the Requests / Second. */
        void Meter()
        {
            if(!config::GetBoolArg("-meters", false))
                return;

            Timer TIMER;
            TIMER.Start();

            while(fMETER)
            {
                Sleep(10000);

                uint32_t nGlobalConnections = 0;
                for(int nIndex = 0; nIndex < MAX_THREADS; nIndex++)
                    nGlobalConnections += DATA_THREADS[nIndex]->nConnections;

                uint32_t RPS = TotalRequests() / TIMER.Elapsed();
                debug::log(0, FUNCTION "LLP Running at %u requests/s with %u connections.\n", __PRETTY_FUNCTION__, RPS, nGlobalConnections);

                TIMER.Reset();
                ClearRequests();
            }
        }


        /** Used for Meter. Adds up the total amount of requests from each Data Thread. **/
        int TotalRequests()
        {
            int nTotalRequests = 0;
            for(int nThread = 0; nThread < MAX_THREADS; nThread++)
                nTotalRequests += DATA_THREADS[nThread]->REQUESTS;

            return nTotalRequests;
        }

        /** Used for Meter. Resets the REQUESTS variable to 0 in each Data Thread. **/
        void ClearRequests()
        {
            for(int nThread = 0; nThread < MAX_THREADS; nThread++)
                DATA_THREADS[nThread]->REQUESTS = 0;
        }
    };

}

#endif
