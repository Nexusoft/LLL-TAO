/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_DATA_H
#define NEXUS_LLP_TEMPLATES_DATA_H

#include <LLP/templates/events.h>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace LLP
{

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
        std::mutex MUTEX;

        /* Variables to track Connection / Request Count. */
        bool fDDOS;
        bool fMETER;

        /* Destructor flag. */
        std::atomic<bool> fDestruct;
        std::atomic<uint32_t> nConnections;
        uint32_t ID;
        uint32_t REQUESTS;
        uint32_t TIMEOUT;
        uint32_t DDOS_rSCORE;
        uint32_t DDOS_cSCORE;

        /* Vector to store Connections. */
        std::vector<ProtocolType *> CONNECTIONS;

        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;

        /* Data Thread. */
        std::thread DATA_THREAD;


        /** Default Constructor
         *
         **/
        DataThread<ProtocolType>(uint32_t id, bool isDDOS, uint32_t rScore, uint32_t cScore,
                                 uint32_t nTimeout, bool fMeter = false)
        : fDDOS(isDDOS)
        , fMETER(fMeter)
        , fDestruct(false)
        , nConnections(0)
        , ID(id)
        , REQUESTS(0)
        , TIMEOUT(nTimeout)
        , DDOS_rSCORE(rScore)
        , DDOS_cSCORE(cScore)
        , CONNECTIONS(0)
        , CONDITION()
        , DATA_THREAD(std::bind(&DataThread::Thread, this)) { }


        /** Default Destructor
         *
         **/
        virtual ~DataThread<ProtocolType>()
        {
            fDestruct = true;

            CONDITION.notify_all();
            DATA_THREAD.join();

            for(auto& CONNECTION : CONNECTIONS)
                if(CONNECTION)
                    delete CONNECTION;
        }


        /** AddConnection
         *
         *  Adds a new connection to current Data Thread.
         *
         *  @param[in] SOCKET The socket for the connection.
         *  @param[in] DDOS The pointer to the DDOS filter to add to connection.
         *
         **/
        void AddConnection(Socket_t SOCKET, DDOS_Filter* DDOS)
        {
            LOCK(MUTEX);

            int nSlot = find_slot();
            if(nSlot == CONNECTIONS.size())
            {
                CONNECTIONS.push_back(new ProtocolType(SOCKET, DDOS, fDDOS));
            }
            else
            {
                CONNECTIONS[nSlot]->fd = SOCKET.fd;
                CONNECTIONS[nSlot]->fDDOS = fDDOS;
                CONNECTIONS[nSlot]->DDOS  = DDOS;
            }

            if(fDDOS)
                DDOS -> cSCORE += 1;

            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
            CONNECTIONS[nSlot]->fCONNECTED = true;
            CONNECTIONS[nSlot]->Reset();

            ++nConnections;

            CONDITION.notify_all();
        }


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
        bool AddConnection(std::string strAddress, uint16_t nPort, DDOS_Filter* DDOS)
        {
            LOCK(MUTEX);

            int nSlot = find_slot();
            if(nSlot == CONNECTIONS.size())
            {
                Socket SOCKET;
                CONNECTIONS.push_back(new ProtocolType(SOCKET, DDOS, fDDOS));
            }
            else
            {
                CONNECTIONS[nSlot]->fDDOS = fDDOS;
                CONNECTIONS[nSlot]->DDOS  = DDOS;
            }

            /* Set the outgoing flag. */
            CONNECTIONS[nSlot]->fOUTGOING = true;
            if(!CONNECTIONS[nSlot]->Connect(strAddress, nPort))
            {
                CONNECTIONS[nSlot]->SetNull();

                return false;
            }

            if(fDDOS)
                DDOS -> cSCORE += 1;

            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
            ++nConnections;

            CONDITION.notify_all();

            return true;
        }


        /** DisconnectAll
         *
         *  Disconnects all connections by issuing a DISCONNECT_FORCE event message
         *  and then removes the connection from this data thread.
         *
         **/
        void DisconnectAll()
        {
            uint32_t nSize = static_cast<uint32_t>(CONNECTIONS.size());
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                disconnect_remove_event(nIndex, DISCONNECT_FORCE);
        }


        /** Thread
         *
         *  Thread that handles all the Reading / Writing of Data from Sockets.
         *  Creates a Packet QUEUE on this connection to be processed by an
         *  LLP Messaging Thread.
         *
         **/
        void Thread()
        {
            /* The mutex for the condition. */
            std::mutex CONDITION_MUTEX;

            /* The main connection handler loop. */
            while(!fDestruct.load())
            {
                /* Keep thread from consuming too many resources. */
                runtime::sleep(1);

                /* Keep data threads waiting for work. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || nConnections.load() > 0; });

                /* Check for close. */
                if(fDestruct.load())
                    return;

                { LOCK(MUTEX);

                    /* Poll the sockets. */
                    int nPoll = poll((pollfd*)CONNECTIONS[0], CONNECTIONS.size(), 100);
                    if(nPoll < 0)
                        continue;
                }

                /* Check all connections for data and packets. */
                uint32_t nSize = 0;
                { LOCK(MUTEX);
                    nSize = static_cast<uint32_t>(CONNECTIONS.size());
                }
                for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    try
                    {
                        /* Skip over Inactive Connections. */
                        if(CONNECTIONS[nIndex]->IsNull() || !CONNECTIONS[nIndex]->Connected())
                            continue;

                        /* Remove Connection if it has Timed out or had any Errors. */
                        if(CONNECTIONS[nIndex]->Errors())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                            continue;
                        }

                        /* Remove Connection if it has Timed out or had any Errors. */
                        if(CONNECTIONS[nIndex]->Timeout(TIMEOUT))
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_TIMEOUT);
                            continue;
                        }

                        /* Handle any DDOS Filters. */
                        if(fDDOS)
                        {
                            /* Ban a node if it has too many Requests per Second. **/
                            if(CONNECTIONS[nIndex]->DDOS->rSCORE.Score() > DDOS_rSCORE ||
                               CONNECTIONS[nIndex]->DDOS->cSCORE.Score() > DDOS_cSCORE)
                                CONNECTIONS[nIndex]->DDOS->Ban();

                            /* Remove a connection if it was banned by DDOS Protection. */
                            if(CONNECTIONS[nIndex]->DDOS->Banned())
                            {
                                disconnect_remove_event(nIndex, DISCONNECT_DDOS);
                                continue;
                            }
                        }

                        /* Generic event for Connection. */
                        CONNECTIONS[nIndex]->Event(EVENT_GENERIC);

                        /* Flush the write buffer. */
                        CONNECTIONS[nIndex]->Flush();

                        /* Work on Reading a Packet. **/
                        CONNECTIONS[nIndex]->ReadPacket();

                        /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                        if(CONNECTIONS[nIndex]->PacketComplete())
                        {
                            /* Debug dump of message type. */
                            debug::log(4, FUNCTION, "Recieved Message (", CONNECTIONS[nIndex]->INCOMING.GetBytes().size(), " bytes)");

                            /* Debug dump of packet data. */
                            if(config::GetArg("-verbose", 0) >= 5)
                                PrintHex(CONNECTIONS[nIndex]->INCOMING.GetBytes());

                            /* Handle Meters and DDOS. */
                            if(fMETER)
                                ++REQUESTS;
                            if(fDDOS)
                                CONNECTIONS[nIndex]->DDOS->rSCORE += 1;

                            /* Packet Process return value of False will flag Data Thread to Disconnect. */
                            if(!CONNECTIONS[nIndex]->ProcessPacket())
                            {
                                disconnect_remove_event(nIndex, DISCONNECT_FORCE);
                                continue;
                            }

                            CONNECTIONS[nIndex]->ResetPacket();
                        }
                    }
                    catch(std::exception& e)
                    {
                        debug::error(FUNCTION, "data connection: ", e.what());
                        disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                    }
                }
            }
        }

      private:


        /** disconnect_remove_event
         *
         *  Fires off a Disconnect event with the given disconnect reason
         *  and also removes the data thread connection.
         *
         *  @param[in] index The data thread index to disconnect.
         *
         *  @param[in] reason The reason why the connection is to be disconnected.
         *
         **/
        void disconnect_remove_event(uint32_t index, uint8_t reason)
        {
            CONNECTIONS[index]->Event(EVENT_DISCONNECT, reason);
            remove(index);
        }


        /** remove
         *
         *  Removes given connection from current Data Thread.
         *  This happens with a timeout/error, graceful close, or disconnect command.
         *
         *  @param[in] The index of the connection to remove.
         *
         **/
        void remove(int index)
        {
            LOCK(MUTEX);

            CONNECTIONS[index]->Disconnect();
            CONNECTIONS[index]->SetNull();

            --nConnections;

            CONDITION.notify_all();
        }


        /** find_slot
         *
         *  Returns the index of a component of the CONNECTIONS vector that
         *  has been flagged Disconnected
         *
         **/
        int find_slot()
        {
            int nSize = CONNECTIONS.size();
            for(int index = 0; index < nSize; ++index)
                if(CONNECTIONS[index]->IsNull())
                    return index;

            return nSize;
        }

    };
}

#endif
