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
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <Util/include/hex.h>


namespace LLP
{

    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(uint32_t nID, bool fIsDDOS,
                                         uint32_t rScore, uint32_t cScore,
                                         uint32_t nTimeout, bool fMeter)
    : SLOT_MUTEX      ( )
    , fDDOS           (fIsDDOS)
    , fMETER          (fMeter)
    , fDestruct       (false)
    , nIncoming       (0)
    , nOutbound       (0)
    , ID              (nID)
    , TIMEOUT         (nTimeout)
    , DDOS_rSCORE     (rScore)
    , DDOS_cSCORE     (cScore)
    , CONNECTIONS     (memory::atomic_ptr< std::vector<memory::atomic_ptr<ProtocolType>> >(new std::vector<memory::atomic_ptr<ProtocolType>>()))
    , CONDITION       ( )
    , DATA_THREAD     (std::bind(&DataThread::Thread, this))
    , FLUSH_CONDITION ( )
    , FLUSH_THREAD    (std::bind(&DataThread::Flush, this))
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

        FLUSH_CONDITION.notify_all();
        if(FLUSH_THREAD.joinable())
            FLUSH_THREAD.join();

        //DisconnectAll();

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

            {
                LOCK(SLOT_MUTEX);

                /* Find an available slot. */
                uint32_t nSlot = find_slot();

                /* Update the indexes. */
                pnode->nDataThread     = ID;
                pnode->nDataIndex      = nSlot;
                pnode->FLUSH_CONDITION = &FLUSH_CONDITION;

                /* Find a slot that is empty. */
                if(nSlot == CONNECTIONS->size())
                    CONNECTIONS->push_back(memory::atomic_ptr<ProtocolType>(pnode));
                else
                    CONNECTIONS->at(nSlot).store(pnode);

                /* Fire the connected event. */
                memory::atomic_ptr<ProtocolType>& CONNECTION = CONNECTIONS->at(nSlot);
                CONNECTION->Event(EVENT_CONNECT);

                /* Iterate the DDOS cScore (Connection score). */
                if(DDOS)
                    DDOS -> cSCORE += 1;

                /* Check for inbound socket. */
                if(CONNECTION->Incoming())
                    ++nIncoming;
                else
                    ++nOutbound;
            }

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
    bool DataThread<ProtocolType>::AddConnection(const BaseAddress& addr, DDOS_Filter* DDOS)
    {
        try
        {
            /* Create a new pointer on the heap. */
            ProtocolType* pnode = new ProtocolType(nullptr, false); //turn off DDOS for outgoing connections

            /* Attempt to make the connection. */
            if(!pnode->Connect(addr))
            {
                delete pnode;
                return false;
            }

            {
                LOCK(SLOT_MUTEX);

                /* Find an available slot. */
                uint32_t nSlot = find_slot();

                /* Update the indexes. */
                pnode->nDataThread     = ID;
                pnode->nDataIndex      = nSlot;
                pnode->FLUSH_CONDITION = &FLUSH_CONDITION;

                /* Find a slot that is empty. */
                if(nSlot == CONNECTIONS->size())
                    CONNECTIONS->push_back(memory::atomic_ptr<ProtocolType>(pnode));
                else
                    CONNECTIONS->at(nSlot).store(pnode);

                /* Fire the connected event. */
                memory::atomic_ptr<ProtocolType>& CONNECTION = CONNECTIONS->at(nSlot);
                CONNECTION->Event(EVENT_CONNECT);

                /* Check for inbound socket. */
                if(CONNECTION->Incoming())
                    ++nIncoming;
                else
                    ++nOutbound;
            }

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
        /* Iterate through connections to remove. When call on destruct, simply remove the connection. Otherwise,
         * force a disconnect event. This will inform address manager so it knows to attempt new connections.
         */
        for(uint32_t nIndex = 0; nIndex < CONNECTIONS->size(); ++nIndex)
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
        /* Cache sleep time if applicable. */
        uint32_t nSleep = config::GetArg("-llpsleep", 0);

        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* This mirrors CONNECTIONS with pollfd settings for passing to poll methods.
         * Windows throws SOCKET_ERROR intermittently if pass CONNECTIONS directly.
         */
        std::vector<pollfd> POLLFDS;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Check for data thread sleep (helps with cpu usage). */
            if(nSleep > 0)
                runtime::sleep(nSleep);

            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK, [this]
                                                {
                                                    return fDestruct.load()
                                                    || config::fShutdown.load()
                                                    || nIncoming.load() > 0
                                                    || nOutbound.load() > 0;
                                                });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Wrapped mutex lock. */
            uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

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
                    POLLFDS.at(nIndex).revents = 0; //reset return events

                    /* Set to invalid socket if connection is inactive. */
                    if(!CONNECTIONS->at(nIndex))
                    {
                        POLLFDS.at(nIndex).fd = INVALID_SOCKET;

                        continue;
                    }

                    /* Set the correct file descriptor. */
                    POLLFDS.at(nIndex).fd = CONNECTIONS->at(nIndex)->fd;
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, e.what());
                }
            }

            /* Poll the sockets. */
#ifdef WIN32
            int32_t nPoll = WSAPoll((pollfd*)&POLLFDS[0], nSize, 100);
#else
            int32_t nPoll = poll((pollfd*)&POLLFDS[0], nSize, 100);
#endif

            /* Check poll for available sockets. */
            if(nPoll < 0)
            {
                runtime::sleep(1);
                continue;
            }


            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Load the atomic pointer raw data. */
                    ProtocolType* CONNECTION = CONNECTIONS->at(nIndex).load();

                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;

                    /* Disconnect if there was a polling error */
                    if(POLLFDS.at(nIndex).revents & POLLERR)
                    {
                         disconnect_remove_event(nIndex, DISCONNECT_POLL_ERROR);
                         continue;
                    }

                    /* Disconnect if the socket was disconnected by peer (need for Windows) */
                    if(POLLFDS.at(nIndex).revents & POLLHUP)
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_PEER);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(CONNECTION->Errors())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors. */
                    if(CONNECTION->Timeout(TIMEOUT))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_TIMEOUT);
                        continue;
                    }

                    /* Check that write buffers aren't overflowed. */
                    if(CONNECTION->Buffered() > (MAX_SEND_BUFFER * 10))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_BUFFER);
                        continue;
                    }

                    /* Handle any DDOS Filters. */
                    if(fDDOS && CONNECTION->DDOS)
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(CONNECTION->DDOS->rSCORE.Score() > DDOS_rSCORE
                        || CONNECTION->DDOS->cSCORE.Score() > DDOS_cSCORE)
                            CONNECTION->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(CONNECTION->DDOS->Banned())
                        {
                            debug::log(0, "BANNED: ", CONNECTION->GetAddress().ToString());
                            disconnect_remove_event(nIndex, DISCONNECT_DDOS);
                            continue;
                        }
                    }

                    /* Generic event for Connection. */
                    CONNECTION->Event(EVENT_GENERIC);

                    /* Work on Reading a Packet. **/
                    CONNECTION->ReadPacket();

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(CONNECTION->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        if(config::GetArg("-verbose", 0) >= 4)
                            debug::log(4, FUNCTION, "Recieved Message (", CONNECTION->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::GetArg("-verbose", 0) >= 5)
                            PrintHex(CONNECTION->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++ProtocolType::REQUESTS;

                        /* Increment rScore. */
                        if(fDDOS && CONNECTION->DDOS)
                            CONNECTION->DDOS->rSCORE += 1;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!CONNECTION->ProcessPacket())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_FORCE);
                            continue;
                        }

                        CONNECTION->ResetPacket();
                    }
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, "Data Connection: ", e.what());
                    disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                }
            }
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Flush()
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            FLUSH_CONDITION.wait(CONDITION_LOCK,
                [this]{

                    /* Break on shutdown or destructor. */
                    if(fDestruct.load() || config::fShutdown.load())
                        return true;

                    /* Check for buffered connection. */
                    for(uint32_t nIndex = 0; nIndex < CONNECTIONS->size(); ++nIndex)
                    {
                        try
                        {
                            /* Check for buffered connection. */
                            if(CONNECTIONS->at(nIndex)->Buffered())
                                return true;
                        }
                        catch(const std::exception& e) { }
                    }

                    return false;
                });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < CONNECTIONS->size(); ++nIndex)
            {
                try
                {
                    /* Check for buffered connection. */
                    if(CONNECTIONS->at(nIndex)->Buffered())
                    {
                        /* Attempt to flush to the socket. */
                        CONNECTIONS->at(nIndex)->Flush();
                        if(CONNECTIONS->at(nIndex)->Timeout(10, Socket::WRITE))
                            disconnect_remove_event(nIndex, DISCONNECT_BUFFER);
                    }

                }
                catch(const std::exception& e) { }
            }
        }
    }


    /* Tell the data thread an event has occured and notify each connection. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::NotifyEvent()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            try { CONNECTIONS->at(nIndex)->NotifyEvent(); }
            catch(const std::exception& e) { }
        }
    }


    /* Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::GetConnectionCount(const uint8_t nFlags)
    {
        /* Check for incoming connections. */
        uint32_t nConnections = 0;
        if(nFlags & FLAGS::INCOMING)
            nConnections += nIncoming.load();

        /* Check for outgoing connections. */
        if(nFlags & FLAGS::OUTGOING)
            nConnections += nOutbound.load();

        return nConnections;
    }


    /* Fires off a Disconnect event with the given disconnect reason and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::disconnect_remove_event(uint32_t nIndex, uint8_t nReason)
    {
        ProtocolType* raw = CONNECTIONS->at(nIndex).load(); //we use raw pointer here because event could contain switch node that will cause deadlocks
        raw->Event(EVENT_DISCONNECT, nReason);

        remove(nIndex);
    }


    /* Removes given connection from current Data Thread. This happens on timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove(uint32_t nIndex)
    {
        LOCK(SLOT_MUTEX);

        /* Check for inbound socket. */
        if(CONNECTIONS->at(nIndex)->Incoming())
            --nIncoming;
        else
            --nOutbound;

        /* Free the memory. */
        CONNECTIONS->at(nIndex).free();
        CONDITION.notify_all();
    }


    /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::find_slot()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            if(!CONNECTIONS->at(nIndex))
                return nIndex;
        }

        return nSize;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class DataThread<TritiumNode>;
    template class DataThread<TimeNode>;
    template class DataThread<APINode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;
}
