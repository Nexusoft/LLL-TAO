/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

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

#include <Util/types/lock_unique_ptr.h>

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
    public:

        /** The internal server configuration variables. **/
        const Config CONFIG;


        /** The DDOS variables. **/
        util::atomic::lock_unique_ptr<std::map<BaseAddress, DDOS_Filter*>> DDOS_MAP;

    private:


        /** The listener socket instance. **/
        std::pair<int32_t, int32_t> hListenBase;


        /** The listener socket instance. **/
        std::pair<int32_t, int32_t> hListenSSL;


        /** Address for handling outgoing connections **/
        AddressManager* pAddressManager;


        /** The data type to keep track of current running threads. **/
        std::vector<DataThread<ProtocolType>*> THREADS_DATA;


        /** Listener Thread for accepting incoming connections. **/
        std::vector<std::thread> THREAD_LISTEN;


        /** Meter Thread for tracking incoming and outgoing packet counts. **/
        std::thread THREAD_METER;


        /** Connection Manager thread. **/
        std::thread THREAD_MANAGER;


    public:


        /** Constructor **/
        Server<ProtocolType>(const Config& config);


        /** Default Destructor **/
        virtual ~Server<ProtocolType>();


        /** Name
         *
         *  Returns the name of the protocol type of this server.
         *
         **/
        std::string Name();


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


        /** AddNode
         *
         *  Add a node address to the internal address manager
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *
         **/
        void AddNode(const std::string& strAddress, bool fLookup = false);


        /** ConnectNode
         *
         *  Connect a node address to the internal server manager
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[out] pNode The node that we have just connected to.
         *  @param[in] fLookup Flag to determine if DNS record should be looked up.
         *
         *  @return true if the connection was established.
         *
         **/
        bool ConnectNode(const std::string& strAddress, std::shared_ptr<ProtocolType> &pNodeRet, bool fLookup = false);


        /** AddConnection
         *
         *  Public Wrapper to Add a Connection Manually.
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
        bool AddConnection(const std::string& strAddress, uint16_t nPort, bool fSSL, bool fLookup, Args&&... args)
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
            if(CONFIG.ENABLE_DDOS)
            {
                /* Add new filter to map if it doesn't exist. */
                if(!DDOS_MAP->count(addrConnect))
                    DDOS_MAP->emplace(std::make_pair(addrConnect, new DDOS_Filter(CONFIG.DDOS_TIMESPAN)));

                /* DDOS Operations: Only executed when DDOS is enabled. */
                if(!addrConnect.IsLocal() && DDOS_MAP->at(addrConnect)->Banned())
                    return false;
            }

            /* Find a balanced Data Thread to Add Connection to. */
            int32_t nThread = FindThread();
            if(nThread < 0)
                return false;

            /* Select the proper data thread. */
            DataThread<ProtocolType> *dt = THREADS_DATA[nThread];

            /* Attempt the connection. */
            if(!dt->NewConnection(addrConnect, CONFIG.ENABLE_DDOS ? DDOS_MAP->at(addrConnect) : nullptr, fSSL, std::forward<Args>(args)...))
            {
                /* Add the address to the address manager if it exists. */
                if(pAddressManager)
                {
                    /* We want to add failures up to a specified limit. */
                    if(pAddressManager->Has(addrConnect))
                        pAddressManager->AddAddress(addrConnect, ConnectState::FAILED);

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


        /** Relay
         *
         *  Relays data to all nodes on the network.
         *
         **/
        template<typename MessageType, typename... Args>
        void Relay(const MessageType& message, Args&&... args)
        {
            /* Relay message to each data thread, which will relay message to each connection of each data thread */
            for(uint16_t nThread = 0; nThread < CONFIG.MAX_THREADS; ++nThread)
                THREADS_DATA[nThread]->Relay(message, args...);
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
            for(uint16_t nThread = 0; nThread <CONFIG.MAX_THREADS; ++nThread)
                THREADS_DATA[nThread]->_Relay(message, ssData);
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


        /** Disconnect
         *
         *  Force a connection disconnect and cleanup server connections.
         *
         *  @param[in] nDataThread The data thread index.
         *  @param[in] nDataIndex The index to remove for.
         *
         **/
        void Disconnect(const uint32_t nDataThread, const uint32_t nDataIndex);


        /** NotifyTriggers
         *
         *  Release all pending triggers from BlockingMessages
         *
         **/
        void NotifyTriggers();


        /** NotifyEvent
         *
         *  Tell the server an event has occurred to wake up thread if it is sleeping. This can be used to orchestrate communication
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
         *  @param[in] hListenBase The socket to bind to
         *  @param[in] nPort The port to listen on
         *  @param[in] fIPv4 Flag indicating the connection is IPv4
         *  @param[in] fRemote Flag indicating that the socket should listen on all interfaced (true) or local only (false)
         *
         *  @return
         *
         **/
        bool BindListenPort(int32_t & hListenBase, uint16_t nPort, bool fIPv4, bool fRemote);


        /** Meter
         *
         *  LLP Meter Thread. Tracks the Requests / Second.
         *
         **/
        void Meter();


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
