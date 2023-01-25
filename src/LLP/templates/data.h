/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_DATA_H
#define NEXUS_LLP_TEMPLATES_DATA_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>

#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>

#include <Util/include/mutex.h>
#include <Util/types/lock_unique_ptr.h>

#include <Util/templates/datastream.h>


#include <atomic>
#include <vector>
#include <thread>
#include <cstdint>
#include <queue>
#include <condition_variable>

namespace LLP
{
    /* forward declarations */
    class Socket;
    class DDOS_Filter;
    class BaseAddress;


    /* Flags for connection count. */
    namespace FLAGS
    {
        enum
        {
            INCOMING = (1 << 1),
            OUTGOING = (1 << 2),
            ALL      = (INCOMING | OUTGOING),
        };
    }


    /** DataThread
     *
     *  Base Template Thread Class for Server base. Used for Core LLP Packet Functionality.
     *  Not to be inherited, only for use by the LLP Server Base Class.
     *
     **/
    template <class ProtocolType>
    class DataThread
    {
    public:

        /* Variables to track Connection / Request Count. */
        std::atomic<bool> fDDOS;
        bool fMETER;

        /* Destructor flag. */
        std::atomic<bool> fDestruct;
        std::atomic<uint32_t> nIncoming;
        std::atomic<uint32_t> nOutbound;

        uint32_t ID;
        uint32_t TIMEOUT;
        uint32_t DDOS_rSCORE;
        uint32_t DDOS_cSCORE;


        /* Vector to store Connections. */
        util::atomic::lock_unique_ptr<std::vector< std::shared_ptr<ProtocolType>>> CONNECTIONS;


        /** Queu to process outbound relay messages. **/
        util::atomic::lock_unique_ptr<std::queue<std::pair<typename ProtocolType::message_t, DataStream>>> RELAY;


        /** The condition for thread sleeping. **/
        std::condition_variable CONDITION;


        /** Data Thread. **/
        std::thread DATA_THREAD;


        /** The condition for thread sleeping. **/
        std::condition_variable FLUSH_CONDITION;


        /** Data Thread. **/
        std::thread FLUSH_THREAD;


        /** Default Constructor. **/
        DataThread<ProtocolType>(const uint32_t nID, const bool ffDDOSIn, const uint32_t rScore, const uint32_t cScore,
                                 const uint32_t nTimeout, const bool fMeter = false);


        /** Default Destructor. **/
        virtual ~DataThread<ProtocolType>();


        /** AddConnection
         *
         *  Adds an existing socket connection to current Data Thread.
         *
         *  @param[in] SOCKET The socket for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to connection.
         *  @param[in] args variadic args to forward to the LLP protocol constructor
         *
         **/
        template<typename... Args>
        void AddConnection(const Socket& SOCKET, DDOS_Filter* DDOS, Args&&... args)
        {
            try
            {
                /* Create a new pointer on the heap. */
                ProtocolType* pnode = new ProtocolType(SOCKET, DDOS, fDDOS, std::forward<Args>(args)...);
                pnode->fCONNECTED.store(true);

                /* Find an available slot. */
                uint32_t nSlot = find_slot();

                /* Update the indexes. */
                pnode->nDataThread     = ID;
                pnode->nDataIndex      = nSlot;
                pnode->FLUSH_CONDITION = &FLUSH_CONDITION;

                /* Find a slot that is empty. */
                if(nSlot == CONNECTIONS->size())
                    CONNECTIONS->push_back(std::shared_ptr<ProtocolType>(pnode));
                else
                    CONNECTIONS->at(nSlot) = std::shared_ptr<ProtocolType>(pnode);

                /* Fire the connected event. */
                pnode->Event(EVENTS::CONNECT);

                /* Iterate the DDOS cScore (Connection score). */
                if(fDDOS.load())
                    DDOS -> cSCORE += 1;

                /* Check for inbound socket. */
                if(pnode->Incoming())
                    ++nIncoming;
                else
                    ++nOutbound;

                /* Notify data thread to wake up. */
                CONDITION.notify_all();
            }
            catch(const std::runtime_error& e)
            {
                debug::error(FUNCTION, e.what()); //catch any atomic_ptr exceptions
            }
        }


        /** NewConnection
         *
         *  Establishes a new connection and adds it to current Data Thread
         *
         *  @param[in] addr Address class instnace containing the IP address and port for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to the connection.
         *  @param[in] fSSL Flag indicating if this connection should use SSL
         *  @param[in] args variadic args to forward to the LLP protocol constructor
         *
         *  @return Returns true if successfully added, false otherwise.
         *
         **/
        template<typename... Args>
        bool NewConnection(const BaseAddress &addr, DDOS_Filter* DDOS, const bool& fSSL, Args&&... args)
        {
            try
            {
                /* Create a new pointer on the heap. */
                ProtocolType* pnode = new ProtocolType(nullptr, false, std::forward<Args>(args)...); //turn off DDOS for outgoing connections

                /* Set the SSL flag */
                pnode->SetSSL(fSSL);

                /* Attempt to make the connection. */
                if(!pnode->Connect(addr))
                {
                    delete pnode;
                    return false;
                }

                /* Find an available slot. */
                uint32_t nSlot = find_slot();

                /* Update the indexes. */
                pnode->nDataThread     = ID;
                pnode->nDataIndex      = nSlot;
                pnode->FLUSH_CONDITION = &FLUSH_CONDITION;

                /* Find a slot that is empty. */
                if(nSlot == CONNECTIONS->size())
                    CONNECTIONS->push_back(std::shared_ptr<ProtocolType>(pnode));
                else
                    CONNECTIONS->at(nSlot) = std::shared_ptr<ProtocolType>(pnode);

                /* Fire the connected event. */
                pnode->Event(EVENTS::CONNECT);

                /* Check for inbound socket. */
                if(pnode->Incoming())
                    ++nIncoming;
                else
                    ++nOutbound;

                /* Notify data thread to wake up. */
                CONDITION.notify_all();

            }
            catch(const std::runtime_error& e)
            {
                debug::error(FUNCTION, e.what()); //catch any atomic_ptr exceptions

                return false;
            }

            return true;
        }


        /** NewConnection
         *
         *  Establishes a new connection and adds it to current Data Thread and returns the active connection pointer.
         *
         *  @param[in] addr Address class instnace containing the IP address and port for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to the connection.
         *  @param[in] fSSL Flag indicating if this connection should use SSL
         *  @param[in] args variadic args to forward to the LLP protocol constructor
         *
         *  @return Returns true if successfully added, false otherwise.
         *
         **/
        bool NewConnection(const BaseAddress &addr, DDOS_Filter* DDOS, const bool& fSSL, std::shared_ptr<ProtocolType> &pNodeRet);


        /** DisconnectAll
         *
         *  Disconnects all connections by issuing a DISCONNECT::FORCE event message
         *  and then removes the connection from this data thread.
         *
         **/
        void DisconnectAll();


        /** NotifyTriggers
         *
         *  Release all pending triggers from BlockingMessages
         *
         **/
        void NotifyTriggers();


        /** Thread
         *
         *  Thread that handles all the Reading of Data from Sockets.
         *  Creates a Packet QUEUE on this connection to be processed by an
         *  LLP Messaging Thread.
         *
         **/
        void Thread();


        /** Flush
         *
         *  Thread to handle flushing write buffers.
         *
         **/
        void Flush();


        /** Relay
         *
         *  Relays data to all nodes on the network.
         *
         **/
        template<typename MessageType, typename... Args>
        void Relay(const MessageType& message, Args&&... args)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ((ssData << args), ...);

            /* Push the relay message to outbound queue. */
            RELAY->push(std::make_pair(message, ssData));

            /* Wake up the flush thread. */
            FLUSH_CONDITION.notify_all();
        }


        /** _Relay
         *
         *  Relays raw binary data to the network. Accepts only binary stream pre-serialized.
         *
         **/
        template<typename MessageType>
        void _Relay(const MessageType& message, const DataStream& ssData)
        {
            /* Push the relay message to outbound queue. */
            RELAY->push(std::make_pair(message, ssData));

            /* Wake up the flush thread. */
            FLUSH_CONDITION.notify_all();
        }


        /** GetConnectionCount
         *
         *  Returns the number of active connections.
         *
         **/
        uint32_t GetConnectionCount(const uint8_t nFlags = FLAGS::ALL);


        /** NotifyEvent
         *
         *  Tell the data thread an event has occured and notify each connection.
         *
         **/
        void NotifyEvent();


      private:


        /** remove_connection_with_event
         *
         *  Fires off a Disconnect event with the given disconnect reason
         *  and also removes the data thread connection.
         *
         *  @param[in] nIndex The data thread index to disconnect.
         *  @param[in] nReason The reason why the connection is to be disconnected.
         *
         **/
        void remove_connection_with_event(const uint32_t nIndex, const uint8_t nReason);


        /** remove
         *
         *  Removes given connection from current Data Thread.
         *  This happens with a timeout/error, graceful close, or disconnect command.
         *
         *  @param[in] nIndex The index of the connection to remove.
         *
         **/
        void remove_connection(const uint32_t nIndex);


        /** find_slot
         *
         *  Returns the index of a component of the CONNECTIONS vector that
         *  has been flagged Disconnected
         *
         **/
        uint32_t find_slot();

    };
}

#endif
