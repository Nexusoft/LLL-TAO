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
        /* The DDOS variables. */
        std::map<BaseAddress, DDOS_Filter *> DDOS_MAP;
        std::atomic<bool> fDDOS;

        std::condition_variable MANAGER;

        /* Basic Socket Handle Variables. */
        std::thread          LISTEN_THREAD_V4;
        std::thread          LISTEN_THREAD_V6;
        std::thread          METER_THREAD;

    public:
        uint16_t PORT;
        uint16_t MAX_THREADS;
        uint32_t DDOS_TIMESPAN;

        /* The data type to keep track of current running threads. */
        std::vector<DataThread<ProtocolType> *> DATA_THREADS;


        /* Connection Manager. */
        std::thread MANAGER_THREAD;


        /* Address manager */
        AddressManager *pAddressManager;


        /* The sleep time of address manager. */
        uint32_t nSleepTime;


        /** Name
         *
         *  Returns the name of the protocol type of this server.
         *
         **/
        std::string Name();


        /** Constructor **/
        Server<ProtocolType>(uint16_t nPort,
                             uint16_t nMaxThreads,
                             uint32_t nTimeout = 30,
                             bool fDDOS_ = false,
                             uint32_t cScore = 0,
                             uint32_t rScore = 0,
                             uint32_t nTimespan = 60,
                             bool fListen = true,
                             bool fMeter = false,
                             bool fManager = false,
                             uint32_t nSleepTimeIn = 1000);


        /** Default Destructor **/
        virtual ~Server<ProtocolType>();


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
         *  @param[in] strPort		Port of outgoing connection
         *
         **/
        void AddNode(std::string strAddress, uint16_t nPort);


        /** AddConnection
         *
         *  Public Wraper to Add a Connection Manually.
         *
         *  @param[in] strAddress	IPv4 Address of outgoing connection
         *  @param[in] strPort		Port of outgoing connection
         *
         *  @return	Returns 1 If successful, 0 if unsuccessful, -1 on errors.
         *
         **/
        bool AddConnection(std::string strAddress, uint16_t nPort);


        /** GetConnectionCount
         *
         *  Get the number of active connection pointers from data threads.
         *
         *  @return Returns the count of active connections
         *
         **/
        uint32_t GetConnectionCount();


        /** Get Connection
         *
         *  Get the best connection based on latency
         *
         **/
        memory::atomic_ptr<ProtocolType>& GetConnection(const BaseAddress& addrExclude);


        /** Relay
         *
         *  Relays data to all nodes on the network.
         *
         **/
        template<typename MessageType, typename DataType>
        void Relay(MessageType message, DataType data)
        {
            /* Relay message to each data thread, which will relay message to
               each connection of each data thread */
            for(uint16_t nThread = 0; nThread < MAX_THREADS; ++nThread)
                DATA_THREADS[nThread]->Relay(message, data);
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
         *  @param[in] fIPv4
         *
         **/
        void ListeningThread(bool fIPv4);


        /** BindListenPort
         *
         *  Bind connection to a listening port.
         *
         *  @param[in] hListenSocket
         *  @param[in] fIPv4
         *
         *  @return
         *
         **/
        bool BindListenPort(int32_t & hListenSocket, bool fIPv4 = true);


        /** Meter
         *
         * LLP Meter Thread. Tracks the Requests / Second.
         *
         **/
        void Meter();


        /** TotalRequests
         *
         *  Used for Meter. Adds up the total amount of requests from each
         *  Data Thread.
         *
         *  @return Returns the total number of requests.
         *
         **/
        uint32_t TotalRequests();


        /** ClearRequests
         *
         *  Used for Meter. Resets the REQUESTS variable to 0 in each Data Thread.
         *
         **/
        void ClearRequests();

    };

}

#endif
