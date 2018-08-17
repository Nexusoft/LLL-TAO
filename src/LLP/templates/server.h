/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_SERVER_H
#define NEXUS_LLP_TEMPLATES_SERVER_H

#include "data.h"
#include "../include/permissions.h"

namespace LLP
{

    /** Base Class to create a Custom LLP Server. Protocol Type class must inherit Connection,
    * and provide a ProcessPacket method. Optional Events by providing GenericEvent method.  */
    template <class ProtocolType> class Server
    {
        /* The DDOS variables. Tracks the Requests and Connections per Second
            from each connected address. */
        std::map<unsigned int,   DDOS_Filter*> DDOS_MAP;
        bool fDDOS, fLISTEN, fMETER;

    public:
        unsigned int PORT, MAX_THREADS, DDOS_TIMESPAN;

        /* The data type to keep track of current running threads. */
        std::vector< DataThread<ProtocolType>* > DATA_THREADS;


        Server<ProtocolType>(int nPort, int nMaxThreads, int nTimeout = 30, bool isDDOS = false, int cScore = 0, int rScore = 0, int nTimespan = 60, bool fListen = true, bool fMeter = false) : 
            fDDOS(isDDOS), fLISTEN(fListen), fMETER(fMeter), PORT(nPort), MAX_THREADS(nMaxThreads), DDOS_TIMESPAN(nTimespan), DATA_THREADS(0), LISTENER(SERVICE), LISTEN_THREAD(boost::bind(&Server::ListeningThread, this)), METER_THREAD(boost::bind(&Server::MeterThread, this))
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

            @param[in] SOCKET		The socket object to add

        */
        void AddConnection(Socket_t SOCKET)
        {
            /* Initialize DDOS Protection for Incoming IP Address. */
            std::vector<unsigned char> vAddress(4, 0);
            sscanf(SOCKET->remote_endpoint().address().to_string().c_str(), "%hhu.%hhu.%hhu.%hhu", &vAddress[0], &vAddress[1], &vAddress[2], &vAddress[3]);
            unsigned int ADDRESS = (vAddress[0] << 24) + (vAddress[1] << 16) + (vAddress[2] << 8) + vAddress[3];

            /* Create new DDOS Filter if NEeded. */
            if(!DDOS_MAP.count(ADDRESS))
                DDOS_MAP[ADDRESS] = new DDOS_Filter(DDOS_TIMESPAN);

            /* DDOS Operations: Only executed when DDOS is enabled. */
            if((fDDOS && DDOS_MAP[ADDRESS]->Banned()))
                return;

            /* Find a balanced Data Thread to Add Connection to. */
            int nThread = FindThread();
            DATA_THREADS[nThread]->AddConnection(SOCKET, DDOS_MAP[ADDRESS]);
        }


        /** Public Wraper to Add a Connection Manually.

            @param[in] strAddress	IPv4 Address of outgoing connection
            @param[in] strPort		Port of outgoing connection

            @return	Returns true if the connection was established successfully */
        bool AddConnection(std::string strAddress, std::string strPort)
        {
            /* Initialize DDOS Protection for Incoming IP Address. */
            std::vector<unsigned char> vAddress(4, 0);
            sscanf(strAddress.c_str(), "%hhu.%hhu.%hhu.%hhu", &vAddress[0], &vAddress[1], &vAddress[2], &vAddress[3]);
            unsigned int ADDRESS = (vAddress[0] << 24) + (vAddress[1] << 16) + (vAddress[2] << 8) + vAddress[3];

            /* Create new DDOS Filter if NEeded. */
            if(!DDOS_MAP.count(ADDRESS))
                DDOS_MAP[ADDRESS] = new DDOS_Filter(DDOS_TIMESPAN);

            /* DDOS Operations: Only executed when DDOS is enabled. */
            if((fDDOS && DDOS_MAP[ADDRESS]->Banned()))
                return false;

            /* Find a balanced Data Thread to Add Connection to. */
            int nThread = FindThread();
            if(!DATA_THREADS[nThread]->AddConnection(strAddress, strPort, DDOS_MAP[ADDRESS]))
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
        Service_t   SERVICE;
        Listener_t  LISTENER;
        Error_t     ERROR_HANDLE;
        Thread_t    LISTEN_THREAD;
        Thread_t    METER_THREAD;
        Mutex_t     DDOS_MUTEX;


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

            /** Don't listen until all data threads are created. **/
            while(DATA_THREADS.size() < MAX_THREADS)
                Sleep(1000);

            /** Basic Socket Options for Boost ASIO. Allow aborted connections, don't allow lingering. **/
            boost::asio::socket_base::enable_connection_aborted    CONNECTION_ABORT(true);
            boost::asio::socket_base::linger                       CONNECTION_LINGER(false, 0);
            boost::asio::ip::tcp::acceptor::reuse_address          CONNECTION_REUSE(true);
            boost::asio::ip::tcp::endpoint 						  		 ENDPOINT(boost::asio::ip::tcp::v4(), PORT);

            /** Open the listener with maximum of 1000 queued Connections. **/
            LISTENER.open(ENDPOINT.protocol());
            LISTENER.set_option(CONNECTION_ABORT);
            LISTENER.set_option(CONNECTION_REUSE);
            LISTENER.set_option(CONNECTION_LINGER);
            LISTENER.bind(ENDPOINT);
            LISTENER.listen(1000, ERROR_HANDLE);

            //printf("LLP Server Listening on Port %u\n", PORT);
            while(!fShutdown)
            {
                /** Limit listener to allow maximum of 100 new connections per second. **/
                Sleep(10);

                try
                {
                    /** Accept a new connection, then process DDOS. **/
                    int nThread = FindThread();
                    Socket_t SOCKET(new boost::asio::ip::tcp::socket(DATA_THREADS[nThread]->IO_SERVICE));
                    LISTENER.accept(*SOCKET);

                    /** Initialize DDOS Protection for Incoming IP Address. **/
                    std::vector<unsigned char> vAddress(4, 0);
                    sscanf(SOCKET->remote_endpoint().address().to_string().c_str(), "%hhu.%hhu.%hhu.%hhu", &vAddress[0], &vAddress[1], &vAddress[2], &vAddress[3]);
                    unsigned int ADDRESS = (vAddress[0] << 24) + (vAddress[1] << 16) + (vAddress[2] << 8) + vAddress[3];

                    { //LOCK(DDOS_MUTEX);
                        if(!DDOS_MAP.count(ADDRESS))
                            DDOS_MAP[ADDRESS] = new DDOS_Filter(30);

                        /** DDOS Operations: Only executed when DDOS is enabled. **/
                        if((fDDOS && DDOS_MAP[ADDRESS]->Banned()) || !CheckPermissions(strprintf("%u.%u.%u.%u:%u",vAddress[0], vAddress[1], vAddress[2],vAddress[3]), PORT))
                        {
                            SOCKET -> shutdown(boost::asio::ip::tcp::socket::shutdown_both, ERROR_HANDLE);
                            SOCKET -> close();

                            printf("XXXXX BLOCKED: LLP Connection Request from %u.%u.%u.%u to Port %u\n", vAddress[0], vAddress[1], vAddress[2], vAddress[3], PORT);

                            continue;
                        }


                        /** Add new connection if passed all DDOS checks. **/
                        DATA_THREADS[nThread]->AddConnection(SOCKET, DDOS_MAP[ADDRESS]);
                    }
                }
                catch(std::exception& e)
                {
                    printf("error: %s\n", e.what());
                }
            }
        }


        /* LLP Meter Thread. Tracks the Requests / Second. */
        void MeterThread()
        {
            Timer TIMER;
            TIMER.Start();

            while(fMETER)
            {
                Sleep(10000);

                unsigned int nGlobalConnections = 0;
                for(int nIndex = 0; nIndex < MAX_THREADS; nIndex++)
                    nGlobalConnections += DATA_THREADS[nIndex]->nConnections;

                double RPS = (double) TotalRequests() / TIMER.Elapsed();
                printf("METER LLP Running at %f Requests per Second with %u Connections.\n", RPS, nGlobalConnections);

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
