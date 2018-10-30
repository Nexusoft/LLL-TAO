/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

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


        Server<ProtocolType>(int nPort, int nMaxThreads, int nTimeout = 30, bool isDDOS = false, int cScore = 0, int rScore = 0, int nTimespan = 60, bool fListen = true, bool fMeter = false) :
            fDDOS(isDDOS), fLISTEN(fListen), fMETER(fMeter), PORT(nPort), MAX_THREADS(nMaxThreads), DDOS_TIMESPAN(nTimespan), DATA_THREADS(0), LISTEN_THREAD(std::bind(&Server::ListeningThread, this)), METER_THREAD(std::bind(&Server::MeterThread, this))
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

            @param[in] strAddress	IPv4 Address of outgoing connection
            @param[in] strPort		Port of outgoing connection

            @return	Returns true if the connection was established successfully */
        bool AddConnection(std::string strAddress, int nPort)
        {
            /* Initialize DDOS Protection for Incoming IP Address. */
            CService addrConnect(strprintf("%s:%i", strAddress.c_str(), nPort).c_str(), nPort);

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


        /** Get the active connection pointers from data threads.
        *
        * @return Returns the list of active connections in a vector
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


    private:

        /* Basic Socket Handle Variables. */
        std::thread          LISTEN_THREAD;
        std::thread          METER_THREAD;
        std::recursive_mutex DDOS_MUTEX;
        int                  hListenSocket;


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
        void ListeningThread()
        {
            /* End the listening thread if LLP set to not listen. */
            if(!fLISTEN)
                return;

            /* Bind the Listener. */
            BindListenPort();

            /* Don't listen until all data threads are created. */
            while(DATA_THREADS.size() < MAX_THREADS)
                Sleep(1000);

            /* Main listener loop. */
            while(true)
            {
                Sleep(10);

                if (hListenSocket != INVALID_SOCKET)
                {
                    struct sockaddr_in sockaddr;
                    socklen_t len = sizeof(sockaddr);

                    SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);

                    CAddress addr;
                    if (hSocket != INVALID_SOCKET)
                        addr = CAddress(sockaddr);

                    if (hSocket == INVALID_SOCKET)
                    {
                        if (GetLastError() != WSAEWOULDBLOCK)
                            printf("socket error accept failed: %d\n", GetLastError());
                    }
                    else
                    {

                        /* Create new DDOS Filter if Needed. */
                        if(!DDOS_MAP.count((CService)addr))
                            DDOS_MAP[addr] = new DDOS_Filter(DDOS_TIMESPAN);

                        /* DDOS Operations: Only executed when DDOS is enabled. */
                        if((fDDOS && DDOS_MAP[(CService)addr]->Banned()))
                        {
                            printf(NODE "Connection Request %s refused... Banned.", addr.ToString().c_str());
                            close(hSocket);

                            continue;
                        }

                        Socket_t sockNew(hSocket, addr);

                        int nThread = FindThread();
                        DATA_THREADS[nThread]->AddConnection(sockNew, DDOS_MAP[(CService)addr]);

                        printf(NODE "Accepted Connection %s on port %u\n", addr.ToString().c_str(), PORT);
                    }
                }
            }
        }

        bool BindListenPort()
        {
            std::string strError = "";
            int nOne = 1;

            #ifdef WIN32
                // Initialize Windows Sockets
                WSADATA wsadata;
                int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
                if (ret != NO_ERROR)
                {
                    printf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);

                    return false;
                }
            #endif

            // Create socket for listening for incoming connections
            hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (hListenSocket == INVALID_SOCKET)
            {
                printf("Error: Couldn't open socket for incoming connections (socket returned error %d)", GetLastError());

                return false;
            }

            #ifdef SO_NOSIGPIPE
                // Different way of disabling SIGPIPE on BSD
                setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
            #endif

            #ifndef WIN32
                // Allow binding if the port is still in TIME_WAIT state after
                // the program was closed and restarted.  Not an issue on windows.
                setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
            #endif


            // The sockaddr_in structure specifies the address family,
            // IP address, and port for the socket that is being bound
            struct sockaddr_in sockaddr;
            memset(&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_addr.s_addr = INADDR_ANY; // bind to all IPs on this computer
            sockaddr.sin_port = htons(PORT);
            if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                int nErr = GetLastError();
                if (nErr == WSAEADDRINUSE)
                    printf("Error:Unable to bind to port %d on this computer.  Nexus is probably already running.", ntohs(sockaddr.sin_port));
                else
                    printf("Error: Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin_port), nErr);

                return false;
            }
            printf(NODE "Bound to port %d\n", ntohs(sockaddr.sin_port));

            // Listen for incoming connections
            if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                printf("Error: Listening for incoming connections failed (listen returned error %d)", GetLastError());

                return false;
            }

            return true;
        }


        /* LLP Meter Thread. Tracks the Requests / Second. */
        void MeterThread()
        {
            if(!GetBoolArg("-meters", false))
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
                printf("***** LLP Running at %u requests/s with %u connections.\n", RPS, nGlobalConnections);

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
