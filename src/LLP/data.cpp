/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2021

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/base_address.h>
#include <LLP/templates/data.h>
#include <LLP/templates/static.h>
#include <LLP/templates/socket.h>

#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/lookup.h>

#include <Util/include/hex.h>


namespace LLP
{
    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(const uint32_t nID, const bool ffDDOSIn,
                                         const uint32_t rScore, const uint32_t cScore,
                                         const uint32_t nTimeout, const bool fMeter)
    : fDDOS           (ffDDOSIn)
    , fMETER          (fMeter)
    , fDestruct       (false)
    , nIncoming       (0)
    , nOutbound       (0)
    , ID              (nID)
    , TIMEOUT         (nTimeout)
    , DDOS_rSCORE     (rScore)
    , DDOS_cSCORE     (cScore)
    , CONNECTIONS     (util::atomic::lock_unique_ptr<std::vector<std::shared_ptr<ProtocolType>> >(new std::vector<std::shared_ptr<ProtocolType>>()))
    , RELAY           (util::atomic::lock_unique_ptr<std::queue<std::pair<typename ProtocolType::message_t, DataStream>> >(new std::queue<std::pair<typename ProtocolType::message_t, DataStream>>()))
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

        /* Wait for all data threads. */
        if(DATA_THREAD.joinable())
            DATA_THREAD.join();

        FLUSH_CONDITION.notify_all();

        /* Wait for any threads still flushing buffers. */
        if(FLUSH_THREAD.joinable())
            FLUSH_THREAD.join();
    }


    /* Establishes a new connection and adds it to current Data Thread and returns the active connection pointer. */
    template <class ProtocolType>
    bool DataThread<ProtocolType>::NewConnection(const BaseAddress &addr, DDOS_Filter* DDOS, const bool& fSSL, std::shared_ptr<ProtocolType> &pNodeRet)
    {
        try
        {
            /* Create a new pointer on the heap. */
            ProtocolType* pnode = new ProtocolType(nullptr, false); //turn off DDOS for outgoing connections

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

            /* Set our return connection pointer. */
            pNodeRet = std::shared_ptr<ProtocolType>(pnode);

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS->size())
                CONNECTIONS->push_back(pNodeRet);
            else
                CONNECTIONS->at(nSlot) = pNodeRet;

            /* Check for inbound socket. */
            if(pnode->Incoming())
                ++nIncoming;
            else
                ++nOutbound;

            /* Fire the connected event. */
            pnode->Event(EVENTS::CONNECT);

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


    /*  Disconnects all connections by issuing a DISCONNECT::FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
        /* Iterate through connections to remove.*/
        const uint32_t nSize = CONNECTIONS->size();
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            /* When on destruct or shutdown, remove the connection without events. */
            if(fDestruct.load() || config::fShutdown.load())
                remove_connection(nIndex);

            /* Otherwise, remove with events to inform the address manager so it knows to re-attempt this connection. */
            else
                remove_connection_with_event(nIndex, DISCONNECT::FORCE);
        }
    }

    /* Release all pending triggers from BlockingMessages */
    template <class ProtocolType>
    void DataThread<ProtocolType>::NotifyTriggers()
    {
        /* Iterate through connections to remove.*/
        const uint32_t nSize = CONNECTIONS->size();
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            /* Skip over inactive connections. */
            if(!CONNECTIONS->at(nIndex))
                continue;

            /* Notify connection triggers. */
            CONNECTIONS->at(nIndex)->NotifyTriggers();
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Thread()
    {
        /* Cache sleep time if applicable. */
        const uint32_t nSleep = config::GetArg("-llpsleep", 0);
        const uint32_t nWait  = config::GetArg("-llpwait", 1);

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
            CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                /* Check for suspended state. */
                if(config::fSuspendProtocol.load())
                    return false;

                return fDestruct.load()
                || config::fShutdown.load()
                || nIncoming.load() > 0
                || nOutbound.load() > 0;
            });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Check if we are suspended. */
            if(config::fSuspendProtocol.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Wrapped mutex lock. */
            const uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

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
                    POLLFDS.at(nIndex).events  = POLLIN;// | POLLRDHUP;
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
                /* Access the shared pointer. */
                std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);
                try
                {
                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;

                    /* Disconnect if there was a polling error */
                    if(POLLFDS.at(nIndex).revents & POLLERR)
                    {
                         remove_connection_with_event(nIndex, DISCONNECT::POLL_ERROR);
                         continue;
                    }

                    /* Disconnect if the socket was disconnected by peer (need for Windows) */
                    if(POLLFDS.at(nIndex).revents & POLLHUP)
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::PEER);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(CONNECTION->Errors())
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors. */
                    if(CONNECTION->Timeout(TIMEOUT * 1000, Socket::READ))
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::TIMEOUT);
                        continue;
                    }

                    /* Disconnect if pollin signaled with no data for 1ms consistently (This happens on Linux). */
                    if((POLLFDS.at(nIndex).revents & POLLIN)
                    && CONNECTION->Timeout(nWait, Socket::READ)
                    && CONNECTION->Available() == 0)
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::POLL_EMPTY);
                        continue;
                    }

                    /* Disconnect if buffer is full and remote host isn't reading at all. */
                    if(CONNECTION->Buffered()
                    && CONNECTION->Timeout(15000, Socket::WRITE))
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::TIMEOUT_WRITE);
                        continue;
                    }

                    /* Check that write buffers aren't overflowed. */
                    if(CONNECTION->Buffered() > config::GetArg("-maxsendbuffer", MAX_SEND_BUFFER))
                    {
                        remove_connection_with_event(nIndex, DISCONNECT::BUFFER);
                        continue;
                    }

                    /* Generic event for Connection. */
                    CONNECTION->Event(EVENTS::GENERIC);

                    /* Work on Reading a Packet. **/
                    CONNECTION->ReadPacket();

                    /* Handle any DDOS Filters. */
                    if(fDDOS.load() && CONNECTION->DDOS && !CONNECTION->addr.IsLocal())
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(CONNECTION->DDOS->rSCORE.Score() > DDOS_rSCORE
                        || CONNECTION->DDOS->cSCORE.Score() > DDOS_cSCORE)
                            CONNECTION->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(!CONNECTION->GetAddress().IsLocal() && CONNECTION->DDOS->Banned())
                        {
                            debug::log(0, ProtocolType::Name(), " BANNED: ", CONNECTION->GetAddress().ToString());
                            remove_connection_with_event(nIndex, DISCONNECT::DDOS);
                            continue;
                        }
                    }

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(CONNECTION->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        if(config::nVerbose.load() >= 4)
                            debug::log(4, FUNCTION, "Received Message (", CONNECTION->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::nVerbose.load() >= 5)
                            PrintHex(CONNECTION->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++ProtocolType::REQUESTS;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!CONNECTION->ProcessPacket())
                        {
                            remove_connection_with_event(nIndex, DISCONNECT::FORCE);
                            continue;
                        }

                        /* Increment rScore. */
                        if(fDDOS.load() && CONNECTION->DDOS)
                            CONNECTION->DDOS->rSCORE += 1;

                        /* Run procssed event for connection triggers. */
                        CONNECTION->Event(EVENTS::PROCESSED);
                        CONNECTION->ResetPacket();
                    }
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, "Data Connection: ", e.what());
                    remove_connection_with_event(nIndex, DISCONNECT::ERRORS);
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
            [this]
            {
                /* Break on shutdown or destructor. */
                if(fDestruct.load() || config::fShutdown.load())
                    return true;

                /* Check for suspended state. */
                if(config::fSuspendProtocol.load())
                    return false;

                /* Check for data in the queue. */
                if(!RELAY->empty())
                    return true;

                /* Check for buffered connection. */
                const uint32_t nSize = CONNECTIONS->size();
                for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    try
                    {
                        /* Get shared pointer to prevent race condition on the internal connection pointer. */
                        std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);

                        /* Skip over Inactive Connections. */
                        if(!CONNECTION || !CONNECTION->Connected())
                            continue;

                        /* Check for buffered connection. */
                        if(CONNECTION->Buffered())
                            return true;
                    }
                    catch(const std::exception& e) { }
                }

                return false;
            });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Check if we are suspended. */
            if(config::fSuspendProtocol.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* Pair to store the relay from the queue. */
            std::pair<typename ProtocolType::message_t, DataStream> qRelay =
                std::make_pair(typename ProtocolType::message_t(), DataStream(SER_NETWORK, MIN_PROTO_VERSION));

            /* Grab data from queue. */
            if(!RELAY->empty())
            {
                /* Make a copy of the relay data. */
                qRelay = RELAY->front();
                RELAY->pop();
            }

            /* Check all connections for data and packets. */
            uint32_t nSize = CONNECTIONS->size();
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Reset stream read position. */
                    qRelay.second.Reset();

                    /* Get shared pointer to prevent race condition on the internal connection pointer. */
                    std::shared_ptr<ProtocolType> CONNECTION = CONNECTIONS->at(nIndex);

                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;

                    /* Relay if there are active subscriptions. */
                    const DataStream ssRelay = CONNECTION->RelayFilter(qRelay.first, qRelay.second);
                    if(ssRelay.size() != 0)
                    {
                        /* Build the sender packet. */
                        typename ProtocolType::packet_t PACKET = typename ProtocolType::packet_t(qRelay.first);
                        PACKET.SetData(ssRelay);

                        /* Write packet to socket. */
                        CONNECTION->WritePacket(PACKET);
                    }

                    /* Attempt to flush data when buffer is available. */
                    if(CONNECTION->Buffered() && CONNECTION->Flush() < 0)
                        runtime::sleep(std::min(5u, CONNECTION->nConsecutiveErrors.load() / 1000)); //we want to sleep when we have periodic failures
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
        typename std::vector<std::shared_ptr<ProtocolType>>::iterator ITT = CONNECTIONS->begin();

        while(ITT != CONNECTIONS->end())
        {
            /* Access the shared pointer of the connection */
            std::shared_ptr<ProtocolType> CONNECTION = *ITT;

            if(CONNECTION)
            {
                try { CONNECTION->NotifyEvent(); }
                catch(const std::exception& e) { }
            }
        }
    }


    /* Disconnects given connection from current Data Thread. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::Disconnect(const uint32_t nIndex)
    {
        /* Make sure connection is active. */
        if(CONNECTIONS->at(nIndex))
        {
            /* First disconnect our sockets. */
            CONNECTIONS->at(nIndex)->Disconnect();

            /* Now remove it from our connections vector. */
            remove_connection_with_event(nIndex, DISCONNECT::FORCE);
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
    void DataThread<ProtocolType>::remove_connection_with_event(const uint32_t nIndex, const uint8_t nReason)
    {
        /* Check that we have an active connection here. */
        if(!CONNECTIONS->at(nIndex))
            return;

        /* Fire off our disconnect event now. */
        CONNECTIONS->at(nIndex)->Event(EVENTS::DISCONNECT, nReason);
        remove_connection(nIndex);
    }


    /* Removes given connection from current Data Thread. This happens on timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove_connection(const uint32_t nIndex)
    {
        /* Check that we have an active connection here. */
        if(!CONNECTIONS->at(nIndex))
            return;

        /* Adjust our internal counters for incoming/outbound. */
        if(CONNECTIONS->at(nIndex)->Incoming())
        {
            --nIncoming;

            /* Handle Meters and DDOS. */
            if(fMETER)
                ++ProtocolType::DISCONNECTS;
        }
        else
            --nOutbound;

        /* Free the memory and notify threads. */
        CONNECTIONS->at(nIndex) = nullptr;
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
    template class DataThread<LookupNode>;
    template class DataThread<TimeNode>;
    template class DataThread<APINode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;
}
