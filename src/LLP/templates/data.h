/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_DATA_H
#define NEXUS_LLP_TEMPLATES_DATA_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>

#include <Util/include/mutex.h>
#include <Util/include/memory.h>

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
        /** message_args
         *
         *  Overload of variadic templates
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *
         **/
        template<class Head>
        void message_args(DataStream& s, Head&& head)
        {
            s << std::forward<Head>(head);
        }


        /** message_args
         *
         *  Variadic template pack to handle any message size of any type.
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *  @param[in] tail The variadic paramters
         *
         **/
        template<class Head, class... Tail>
        void message_args(DataStream& s, Head&& head, Tail&&... tail)
        {
            s << std::forward<Head>(head);
            message_args(s, std::forward<Tail>(tail)...);
        }


        /** Lock access to find slot to ensure no race conditions happend between threads. **/
        std::mutex SLOT_MUTEX;


    public:

        /* Variables to track Connection / Request Count. */
        bool fDDOS;
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
        memory::atomic_ptr< std::vector< memory::atomic_ptr<ProtocolType>> > CONNECTIONS;


        /** Queu to process outbound relay messages. **/
        memory::atomic_ptr< std::queue<std::pair<typename ProtocolType::message_t, DataStream>> > RELAY;


        /** The condition for thread sleeping. **/
        std::condition_variable CONDITION;


        /** Data Thread. **/
        std::thread DATA_THREAD;


        /** The condition for thread sleeping. **/
        std::condition_variable FLUSH_CONDITION;


        /** Data Thread. **/
        std::thread FLUSH_THREAD;


        /** Default Constructor
         *
         **/
        DataThread<ProtocolType>(uint32_t nID, bool ffDDOSIn, uint32_t rScore, uint32_t cScore,
                                 uint32_t nTimeout, bool fMeter = false);


        /** Default Destructor
         *
         **/
        virtual ~DataThread<ProtocolType>();


        /** AddConnection
         *
         *  Adds a new connection to current Data Thread.
         *
         *  @param[in] SOCKET The socket for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to connection.
         *
         **/
        void AddConnection(const Socket& SOCKET, DDOS_Filter* DDOS);


        /** AddConnection
         *
         *  Adds a new connection to current Data Thread
         *
         *  @param[in] strAddress The IP address for the connection.
         *  @param[in] nPort The port number for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to the connection.
         *
         *  @return Returns true if successfully added, false otherwise.
         *
         **/
        bool AddConnection(const BaseAddress &addr, DDOS_Filter* DDOS);


        /** DisconnectAll
         *
         *  Disconnects all connections by issuing a DISCONNECT_FORCE event message
         *  and then removes the connection from this data thread.
         *
         **/
        void DisconnectAll();


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
            message_args(ssData, std::forward<Args>(args)...);

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


        /** disconnect_remove_event
         *
         *  Fires off a Disconnect event with the given disconnect reason
         *  and also removes the data thread connection.
         *
         *  @param[in] nIndex The data thread index to disconnect.
         *  @param[in] nReason The reason why the connection is to be disconnected.
         *
         **/
        void disconnect_remove_event(uint32_t nIndex, uint8_t nReason);


        /** remove
         *
         *  Removes given connection from current Data Thread.
         *  This happens with a timeout/error, graceful close, or disconnect command.
         *
         *  @param[in] The index of the connection to remove.
         *
         **/
        void remove(uint32_t nIndex);


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
