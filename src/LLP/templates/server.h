/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_SERVER_H
#define NEXUS_LLP_TEMPLATES_SERVER_H


#include <LLP/templates/data.h>
#include <LLP/include/legacy_address.h>
#include <LLP/include/manager.h>
#include <LLP/include/config.h>

#include <Util/include/memory.h>

#include <map>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <cstdint>

namespace LLP
{
    /* forward declarations */
    class AddressManager;
    class DDOS_Filter;
    class InfoAddress;
    typedef struct ssl_st SSL;


    /** Server
     *
     *  Base Class to create a Custom LLP Server.
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

        /** Server's listenting port. **/
        uint16_t PORT;


        /** Server's listenting port for SSL connections. **/
        uint16_t SSL_PORT;


        /** The listener socket instance. **/
        std::pair<int32_t, int32_t> hListenSocket;


        /** The SSL listener socket instance. **/
        std::pair<int32_t, int32_t> hSSLListenSocket;


        /** Determine if Server should make / accept SSL connections. **/
        std::atomic<bool> fSSL;


        /** Determine if Server should only make / accept SSL connections. **/
        std::atomic<bool> fSSLRequired;


        /** The DDOS variables. **/
        memory::atomic_ptr< std::map<BaseAddress, DDOS_Filter *> > DDOS_MAP;


        /** DDOS flag for off or on. **/
        std::atomic<bool> fDDOS;


        /** The moving average timespan for DDOS throttling. **/
        uint32_t DDOS_TIMESPAN;


        /** Maximum number of data threads for this server. **/
        uint16_t MAX_THREADS;


        /** The data type to keep track of current running threads. **/
        std::vector<DataThread<ProtocolType> *> DATA_THREADS;


        /** condition variable for manager thread. **/
        std::condition_variable MANAGER;


        /** Listener Thread for accepting incoming connections. **/
        std::thread LISTEN_THREAD;


        /** Listener Thread for accepting incoming SSL connections. **/
        std::thread SSL_LISTEN_THREAD;


        /** Meter Thread for tracking incoming and outgoing packet counts. **/
        std::thread METER_THREAD;


        /** Port mapping thread for opening port in router. **/
        std::thread UPNP_THREAD;


        /** Port mapping thread for opening the SSL port in router. **/
        std::thread SSL_UPNP_THREAD;


        /** Connection Manager thread. **/
        std::thread MANAGER_THREAD;


        /** Address for handling outgoing connections **/
        AddressManager *pAddressManager;


        /** The sleep time of address manager. **/
        uint32_t nSleepTime;


        /** Max number of incoming connections this server can make. **/
        uint32_t nMaxIncoming;


        /** Maximum number connections in total that this server can handle.  Must be greater than nMaxIncoming **/
        uint32_t nMaxConnections;

        /** Flag indicating that the server should listen for remote connections **/
        bool fRemote;


    public:



        /** Name
         *
         *  Returns the name of the protocol type of this server.
         *
         **/
        std::string Name();


        /** Constructor **/
        Server<ProtocolType>(const ServerConfig& config);


        /** Default Destructor **/
        virtual ~Server<ProtocolType>();


        /** GetPort
         *
         *  Returns the port number for this Server.
         *
         *  @param[in] fSSL.  Flag indicating whether to return the SSL port or not
         *
         **/
        uint16_t GetPort(bool fSSL = false) const;


        /** GetAddressManager
         *
         *  Returns the address manager instance for this server.
         *
         **/
        AddressManager* GetAddressManager() const;


        /** Shutdown
         *
         *  Cleanup and shutdown subsystems
         *
         **/
        void Shutdown();


        /** AddNode
         *
         *  Add a node address to the internal address manager
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *
         **/
        void AddNode(std::string strAddress, bool fLookup = false);


        /** AddConnection
         *
         *  Public Wraper to Add a Connection Manually.
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[in] strPort		Port of outgoing connection
         *  @param[in] fSSL         Flag indicating SSL should be used for this connection
         *  @param[in] fLookup		Flag indicating whether address lookup should occur
         *  @param[in] args         Variadic args to forward to the data thread constructor
         *
         *  @return	Returns true on successful, false otherwise
         *
         **/
        template<typename... Args>
        bool AddConnection(std::string strAddress, uint16_t nPort, bool fSSL, bool fLookup, Args&&... args)
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
                /* Add new filter to map if it doesn't exist. */
                if(!DDOS_MAP->count(addrConnect))
                    DDOS_MAP->emplace(std::make_pair(addrConnect, new DDOS_Filter(DDOS_TIMESPAN)));

                /* DDOS Operations: Only executed when DDOS is enabled. */
                if(DDOS_MAP->at(addrConnect)->Banned())
                    return false;
            }

            /* Find a balanced Data Thread to Add Connection to. */
            int32_t nThread = FindThread();
            if(nThread < 0)
                return false;

            /* Select the proper data thread. */
            DataThread<ProtocolType> *dt = DATA_THREADS[nThread];

            /* Attempt the connection. */
            if(!dt->NewConnection(addrConnect, fDDOS.load() ? DDOS_MAP->at(addrConnect) : nullptr, fSSL, std::forward<Args>(args)...))
            {
                /* Add the address to the address manager if it exists. */
                if(pAddressManager)
                {
                    /* We want to add failures up to a specified limit. */
                    if(pAddressManager->Has(addrConnect))
                    {
                        /* Grab our trust address if valid. */
                        const TrustAddress& addrInfo = pAddressManager->Get(addrConnect);

                        /* Kill address if we haven't found any valid connections. */
                        const uint32_t nLimit = config::GetArg("-prunefailed", 0);
                        if(nLimit > 0 && addrInfo.nFailed > nLimit  && addrInfo.nConnected == 0)
                        {
                            /* Remove from database */
                            pAddressManager->RemoveAddress(addrConnect);
                            debug::log(3, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "CLEAN: ", ANSI_COLOR_RESET, "address has reached failure limit: ", addrInfo.nFailed);
                        }
                        else
                            pAddressManager->AddAddress(addrConnect, ConnectState::FAILED);
                    }

                }

                return false;
            }

            /* Add the address to the address manager if it exists. */
            if(pAddressManager)
                pAddressManager->AddAddress(addrConnect, ConnectState::CONNECTED);

            return true;
        }


        /** GetConnections
         *
         *  Constructs a vector of all active connections across all threads
         *
         *  @return Returns a vector of active connections
         **/
        std::vector<std::shared_ptr<ProtocolType>> GetConnections() const;


        /** GetConnectionCount
         *
         *  Get the number of active connection pointers from data threads.
         *
         *  @return Returns the count of active connections
         *
         **/
        uint32_t GetConnectionCount(const uint8_t nFlags = FLAGS::ALL);


        /** Get Connection
         *
         *  Select a random and currently open connection
         *
         **/
        std::shared_ptr<ProtocolType> GetConnection();


        /** Get Connection
         *
         *  Get the best connection based on latency
         *
         *  @param[in] pairExclude The connection that should be excluded from the search.
         *
         **/
        std::shared_ptr<ProtocolType> GetConnection(const std::pair<uint32_t, uint32_t>& pairExclude);


        /** Get Connection
         *
         *  Get the best connection based on data thread index.
         *
         **/
        std::shared_ptr<ProtocolType> GetConnection(const uint32_t nDataThread, const uint32_t nDataIndex);


        /** GetSpecificConnection
         *
         *  Get connection matching variable args, which are passed on to the ProtocolType instance.
         *
         **/
        template<typename... Args>
        std::shared_ptr<ProtocolType> GetSpecificConnection(Args&&... args)
        {
            /* Thread ID and index of the matchingconnection */
            int16_t nRetThread = -1;
            int16_t nRetIndex  = -1;

            /* Loop through all threads */
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

                        /* check the details */
                        if(CONNECTION->Matches(std::forward<Args>(args)...))
                        {

                            nRetThread = nThread;
                            nRetIndex  = nIndex;

                            /* Break out as we have found a match */
                            break;
                        }
                    }
                    catch(const std::exception& e)
                    {
                        //debug::error(FUNCTION, e.what());
                    }
                }

                /* break if we have a match */
                if(nRetThread != -1 && nRetIndex != -1)
                    break;

            }

            /* Handle if no connections were found. */
            static std::shared_ptr<ProtocolType> pNULL;
            if(nRetThread == -1 || nRetIndex == -1)
                return pNULL;

            return DATA_THREADS[nRetThread]->CONNECTIONS->at(nRetIndex);
        }


        /** Relay
         *
         *  Relays data to all nodes on the network.
         *
         **/
        template<typename MessageType, typename... Args>
        void Relay(const MessageType& message, Args&&... args)
        {
            /* Relay message to each data thread, which will relay message to each connection of each data thread */
            for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
                DATA_THREADS[nThread]->Relay(message, args...);
        }


        /** Relay_
         *
         *  Relays raw binary data to the network. Accepts only binary stream pre-serialized.
         *
         **/
        template<typename MessageType>
        void _Relay(const MessageType& message, const DataStream& ssData)
        {
            /* Relay message to each data thread, which will relay message to each connection of each data thread */
            for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
                DATA_THREADS[nThread]->_Relay(message, ssData);
        }


        /** GetAddresses
         *
         *  Get the active connection pointers from data threads.
         *
         *  @return Returns the list of active connections in a vector.
         *
         **/
        std::vector<LegacyAddress> GetAddresses();


        /** DisconnectAll
        *
        *  Notifies all data threads to disconnect their connections
        *
        **/
        void DisconnectAll();


        /** NotifyEvent
         *
         *  Tell the server an event has occured to wake up thread if it is sleeping. This can be used to orchestrate communication
         *  among threads if a strong ordering needs to be guaranteed.
         *
         **/
        void NotifyEvent();


        /** SSLEnabled
         *
         *  Returns true if SSL is enabled for this server
         *
         **/
        bool SSLEnabled();


        /** SSLRequired
         *
         *  Returns true if SSL is required for this server
         *
         **/
        bool SSLRequired();


        /** CloseListening
         *
         *  Closes the listening sockets.
         *
         **/
        void CloseListening();


        /** OpenListening
         *
         *  Restarts the listening sockets.
         *
         **/
        void OpenListening();


    private:


        /** Manager
         *
         *  Address Manager Thread.
         *
         **/
        void Manager();


        /** FindThread
         *
         *  Determine the first thread with the least amount of active connections.
         *  This keeps them load balanced across all server threads.
         *
         *  @return Returns the index of the found thread. or -1 if not found.
         *
         **/
        int32_t FindThread();


        /** ListeningThread
         *
         *  Main Listening Thread of LLP Server. Handles new Connections and
         *  DDOS associated with Connection if enabled.
         *
         *  @param[in] fIPv4 Flag indicating to listen on the IPv4 interface
         *  @param[in] fSSL Flag indicating that connections should use SSL
         *
         **/
        void ListeningThread(bool fIPv4, bool fSSL);


        /** BindListenPort
         *
         *  Bind connection to a listening port.
         *
         *  @param[in] hListenSocket The socket to bind to
         *  @param[in] nPort The port to listen on
         *  @param[in] fIPv4 Flag indicating the connection is IPv4
         *  @param[in] fRemote Flag indicating that the socket should listen on all interfaced (true) or local only (false)
         *
         *  @return
         *
         **/
        bool BindListenPort(int32_t & hListenSocket, uint16_t nPort, bool fIPv4, bool fRemote);


        /** Meter
         *
         *  LLP Meter Thread. Tracks the Requests / Second.
         *
         **/
        void Meter();


        /** UPnP
         *
         *  UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding.
         *
         *  @param[in] nPort The port to forward
         *
         **/
        void UPnP(uint16_t nPort);


        /** get_listening_socket
         *
         *  Gets the listening socket handle
         *
         *  @param[in] nPort The port to listen on
         *  @param[in] fIPv4 Flag indicating the connection is IPv4
         *
         *  @return the listening socket handle
         *
         **/
        SOCKET get_listening_socket(bool fIPv4, bool fSSL);

    };

}

#endif
