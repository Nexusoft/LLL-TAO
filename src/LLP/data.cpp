/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/base_address.h>
#include <LLP/templates/data.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>
#include <LLP/templates/socket.h>

#include <LLP/types/tritium.h>
#include <LLP/types/legacy.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <Util/include/hex.h>


namespace LLP
{

    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(uint32_t id, bool isDDOS,
                                         uint32_t rScore, uint32_t cScore,
                                         uint32_t nTimeout, bool fMeter)
    : fDDOS(isDDOS)
    , fMETER(fMeter)
    , fDestruct(false)
    , nConnections(0)
    , ID(id)
    , REQUESTS(0)
    , TIMEOUT(nTimeout)
    , DDOS_rSCORE(rScore)
    , DDOS_cSCORE(cScore)
    , CONNECTIONS(memory::atomic_ptr< std::vector<memory::atomic_ptr<ProtocolType>> >(new std::vector<memory::atomic_ptr<ProtocolType>>()))
    , CONDITION()
    , DATA_THREAD(std::bind(&DataThread::Thread, this))
    {
    }


    /** Default Destructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::~DataThread()
    {
        fDestruct = true;

        CONDITION.notify_all();

        if(DATA_THREAD.joinable())
            DATA_THREAD.join();

        DisconnectAll();

        CONNECTIONS.free();
    }


    /*  Adds a new connection to current Data Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::AddConnection(const Socket& SOCKET, DDOS_Filter* DDOS)
    {
        try
        {
            /* Create a new pointer on the heap. */
            ProtocolType* pnode = new ProtocolType(SOCKET, DDOS, fDDOS);
            pnode->fCONNECTED.store(true);

            /* Find an available slot. */
            int nSlot = find_slot();

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS->size())
                CONNECTIONS->push_back(memory::atomic_ptr<ProtocolType>(pnode));
            else
                CONNECTIONS->at(nSlot).store(pnode);

            /* Fire the connected event. */
            CONNECTIONS->at(nSlot)->Event(EVENT_CONNECT);

            /* Iterate the DDOS cScore (Connection score). */
            if(fDDOS)
                DDOS -> cSCORE += 1;

            /* Bump the total connections atomic counter. */
            ++nConnections;

            /* Set the indexes. */
            pnode->nDataThread = ID;
            pnode->nDataIndex  = nSlot;

            /* Notify data thread to wake up. */
            CONDITION.notify_all();
        }
        catch(const std::runtime_error& e)
        {
            debug::error(FUNCTION, e.what()); //catch any atomic_ptr exceptions
        }
    }


    /*  Adds a new connection to current Data Thread */
    template <class ProtocolType>
    bool DataThread<ProtocolType>::AddConnection(const BaseAddress &addr, DDOS_Filter* DDOS)
    {
        try
        {
            /* Create a new pointer on the heap. */
            ProtocolType* pnode = new ProtocolType(DDOS, fDDOS);
            if(!pnode->Connect(addr))
            {
                pnode->Disconnect();
                delete pnode;

                return false;
            }

            /* Set the pnode to outgoing. */
            pnode->fOUTGOING = true;

            /* Search for an available slot. */
            int nSlot = find_slot();

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS->size())
                CONNECTIONS->push_back(memory::atomic_ptr<ProtocolType>(pnode));
            else
                CONNECTIONS->at(nSlot).store(pnode);

            /* Fire the connected event. */
            CONNECTIONS->at(nSlot)->Event(EVENT_CONNECT);

            /* Iterate the DDOS cScore (Connection score). */
            if(fDDOS)
                DDOS -> cSCORE += 1;

            /* Bump the total connections atomic counter. */
            ++nConnections;

            /* Set the indexes. */
            pnode->nDataThread = ID;
            pnode->nDataIndex  = nSlot;

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


    /*  Disconnects all connections by issuing a DISCONNECT_FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
        /* Get the size of the vector. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

        /* Iterate through connections to remove. When call on destruct, simply remove the connection. Otherwise,
         * force a disconnect event. This will inform address manager so it knows to attempt new connections.
         */
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            if(!fDestruct.load())
                disconnect_remove_event(nIndex, DISCONNECT_FORCE);
            else
                remove(nIndex);
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Thread()
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* This mirrors CONNECTIONS with pollfd settings for passing to poll methods.
         * Windows throws SOCKET_ERROR intermittently if pass CONNECTIONS directly.
         */
        std::vector<pollfd> POLLFDS;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || config::fShutdown.load() || nConnections.load() > 0; });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Wrapped mutex lock. */
            uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

            /* We should have connections, as predicate of releasing condition wait.
             * This is a precaution, checking after getting MUTEX lock
             */
            if(nConnections.load() == 0)
                continue;

            /* Check the pollfd's size. */
            if(POLLFDS.size() != nSize)
                POLLFDS.resize(nSize);

            /* Initialize the revents for all connection pollfd structures.
             * One connection must be live, so verify that and skip if none
             */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Set the proper POLLIN flags. */
                    POLLFDS.at(nIndex).events = POLLIN;// | POLLRDHUP;

                    /* Set to invalid socket if connection is inactive. */
                    if(!CONNECTIONS->at(nIndex))
                    {
                        POLLFDS.at(nIndex).fd = INVALID_SOCKET;

                        continue;
                    }

                    /* Set the correct file descriptor. */
                    POLLFDS.at(nIndex).fd = CONNECTIONS->at(nIndex)->fd;
                }
                catch(const std::runtime_error& e)
                {
                    debug::error(FUNCTION, e.what());
                }
            }

            /* Poll the sockets. */
#ifdef WIN32
            WSAPoll((pollfd*)&POLLFDS[0], nSize, 100);
#else
            poll((pollfd*)&POLLFDS[0], nSize, 100);
#endif


            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Load the atomic pointer raw data. */
                    ProtocolType* connection = CONNECTIONS->at(nIndex).load();

                    /* Skip over Inactive Connections. */
                    if(!connection)
                        continue;

                    /* Check if connected. */
                    if(!connection->Connected())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_FORCE);
                        continue;
                    }

                    /* Disconnect if there was a polling error */
                    if((POLLFDS.at(nIndex).revents & POLLERR))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_POLL_ERROR);
                        continue;
                    }

#ifdef WIN32
                    /* Disconnect if the socket was disconnected by peer (need for Windows) */
                    if((POLLFDS.at(nIndex).revents & POLLHUP))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_PEER);
                        continue;
                    }
#endif

                    /* Disconnect if pollin signaled with no data (This happens on Linux). */
                    if((POLLFDS.at(nIndex).revents & POLLIN)
                    && connection->Available() == 0)
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_POLL_EMPTY);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(connection->Errors())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors. */
                    if(connection->Timeout(TIMEOUT))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_TIMEOUT);
                        continue;
                    }

                    /* Handle any DDOS Filters. */
                    if(fDDOS && !connection->fOUTGOING && connection->DDOS)
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(connection->DDOS->rSCORE.Score() > DDOS_rSCORE
                        || connection->DDOS->cSCORE.Score() > DDOS_cSCORE)
                            connection->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(connection->DDOS->Banned())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_DDOS);
                            continue;
                        }
                    }

                    /* Generic event for Connection. */
                    connection->Event(EVENT_GENERIC);

                    /* Flush the write buffer. */
                    connection->Flush();

                    /* Work on Reading a Packet. **/
                    connection->ReadPacket();

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(connection->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        debug::log(4, FUNCTION, "Recieved Message (", connection->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::GetArg("-verbose", 0) >= 5)
                            PrintHex(connection->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++REQUESTS;
                        if(fDDOS)
                            connection->DDOS->rSCORE += 1;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!connection->ProcessPacket())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_FORCE);
                            continue;
                        }

                        connection->ResetPacket();
                    }
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, "Data Connection: ", e.what());
                    debug::error(FUNCTION, "Currently running ", nConnections, " connections.");
                    disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                }
            }
        }
    }


    /* Tell the data thread an event has occured and notify each connection. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::NotifyEvent()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t i = 0; i < nSize; ++i)
        {
            ProtocolType* connection = CONNECTIONS->at(i).load();

            if(!connection)
                continue;

            /* Notify the connection that an event has occurred. */
            connection->NotifyEvent();
        }
    }


    /* Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::GetConnectionCount()
    {
        return nConnections.load();
    }


    /* Fires off a Disconnect event with the given disconnect reason and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::disconnect_remove_event(uint32_t index, uint8_t reason)
    {
        ProtocolType* connection = CONNECTIONS->at(index).load();
        connection->Event(EVENT_DISCONNECT, reason);

        remove(index);
    }


    /* Removes given connection from current Data Thread. This happens on timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove(int index)
    {
        /* Free the memory. */
        CONNECTIONS->at(index).free();

        --nConnections;

        CONDITION.notify_all();
    }


    /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
    template <class ProtocolType>
    int DataThread<ProtocolType>::find_slot()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(int index = 0; index < nSize; ++index)
        {
            if(!CONNECTIONS->at(index))
                return index;
        }

        return nSize;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class DataThread<TritiumNode>;
    template class DataThread<LegacyNode>;
    template class DataThread<TimeNode>;
    template class DataThread<APINode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;
}
