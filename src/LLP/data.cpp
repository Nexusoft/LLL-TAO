/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/templates/data.h>
#include <LLP/include/network.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/events.h>
#include <LLP/templates/socket.h>

#include <LLP/types/tritium.h>
#include <LLP/types/legacy.h>
#include <LLP/types/time.h>
#include <LLP/types/corenode.h>
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
    , CONNECTIONS(0)
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
        DATA_THREAD.join();

        DisconnectAll();
    }


    /*  Adds a new connection to current Data Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::AddConnection(const Socket& SOCKET, DDOS_Filter* DDOS)
    {
        /* Create a new pointer on the heap. */
        ProtocolType* node = new ProtocolType(SOCKET, DDOS, fDDOS);
        node->Event(EVENT_CONNECT);
        node->fCONNECTED = true;

        {
            LOCK(MUTEX);

            int nSlot = find_slot();

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS.size())
                CONNECTIONS.push_back(nullptr);

            /* Assign the slot to the connection. */
            CONNECTIONS[nSlot] = node;

            if(fDDOS)
                DDOS -> cSCORE += 1;

            ++nConnections;

            CONDITION.notify_all();
        }
    }


    /*  Adds a new connection to current Data Thread */
    template <class ProtocolType>
    bool DataThread<ProtocolType>::AddConnection(std::string strAddress, uint16_t nPort, DDOS_Filter* DDOS)
    {
       /* Create a new pointer on the heap. */
       ProtocolType* node = new ProtocolType(DDOS, fDDOS);


       if(!node->Connect(strAddress, nPort))
       {
           node->Disconnect();
           delete node;

           return false;
       }

       {
           LOCK(MUTEX);

           /* Find a slot that is empty. */
           int nSlot = find_slot();
           if(nSlot == CONNECTIONS.size())
               CONNECTIONS.push_back(nullptr);

           CONNECTIONS[nSlot] = node;

           /* Set the outgoing flag. */
           CONNECTIONS[nSlot]->fOUTGOING = true;

           if(fDDOS)
               DDOS -> cSCORE += 1;

           CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
           ++nConnections;

           CONDITION.notify_all();
       }

       return true;
    }


    /*  Disconnects all connections by issuing a DISCONNECT_FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
       uint32_t nSize = static_cast<uint32_t>(CONNECTIONS.size());
       for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
           remove(nIndex);
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Thread()
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown)
        {
            /* Keep thread from consuming too many resources. */
            runtime::sleep(1);

            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            while (!(fDestruct.load() || config::fShutdown || nConnections.load() > 0))
              CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || config::fShutdown || nConnections.load() > 0; });

            /* We don't need to hold the condition lock. Nothing outside this current thread uses it. It is purely to wait for connections */
            CONDITION_LOCK.unlock();

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown)
                return;

            /* Wrapped mutex lock. */
            uint32_t nSize = 0;
            { LOCK(MUTEX);

                /* Get the total connections. */
                nSize = static_cast<uint32_t>(CONNECTIONS.size());

                /* We should have connections, as predicate of releasing condition wait. This is a precaution, checking after getting MUTEX lock */
                if (nSize == 0)
                    continue;

                /* Poll the sockets. */
#ifdef WIN32
                /* Windows returns intermittent SOCKET_ERROR with WSAEINVAL (bad parameters) if you pass pointer to connections
                 * so copy fd data and only pass pollfd struct
                 */
                std::vector<pollfd> pollfds;
                pollfds.resize(nSize);
                for (uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    pollfds[nIndex].fd = CONNECTIONS[nIndex]->fd;
                    pollfds[nIndex].events = POLLIN;
                    pollfds[nIndex].revents = 0;
                }

                int nPoll = WSAPoll((pollfd*)&pollfds[0], nSize, 100);
#else
                int nPoll = poll((pollfd*)CONNECTIONS[0], nSize, 100);
#endif
                /* Continue on poll errors. */
                if (nPoll == 0)
                {
                    /* No connections have data to read */
                    continue;
                }
                /* This was for testing. Uncomment if need to log info on potential SOCKET_ERROR
                 * Potentially spits out a ton of error messages if get this repeatedly
                 */
                // else if (nPoll == SOCKET_ERROR)
                // {
                //     debug::error(FUNCTION, "Error polling outgoing connections (Error code ", WSAGetLastError(), ")");

                //     debug::log(3, "DataThread contents:");

                //     for (uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                //         debug::log(3, "    Data thread node ", nIndex, " fd ", pollfds[nIndex].fd, " events ", pollfds[nIndex].events, " revents ", pollfds[nIndex].revents);

                //     continue;
                // }
                else if (nPoll < 0)
                {
                    /* Should only be SOCKET_ERROR if not 0 or >0 */
                    // debug::error(FUNCTION, "Invalid return value polling outgoing connections (", nPoll, ")");
                    continue;
                }

            }

            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Skip over Inactive Connections. */
                    if(!CONNECTIONS[nIndex] || !CONNECTIONS[nIndex]->Connected())
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


    /*  Fires off a Disconnect event with the given disconnect reason
     *  and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::disconnect_remove_event(uint32_t index, uint8_t reason)
    {
        CONNECTIONS[index]->Event(EVENT_DISCONNECT, reason);

        LOCK(MUTEX);
        remove(index);
    }


    /*  Removes given connection from current Data Thread.
     *  This happens with a timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove(int index)
    {
        if(CONNECTIONS[index])
        {
            /* Free the memory. */
            delete CONNECTIONS[index];
            CONNECTIONS[index] = nullptr;

            --nConnections;
        }

        CONDITION.notify_all();
    }


    /*  Returns the index of a component of the CONNECTIONS vector that
     *  has been flagged Disconnected */
    template <class ProtocolType>
    int DataThread<ProtocolType>::find_slot()
    {
       int nSize = CONNECTIONS.size();
       for(int index = 0; index < nSize; ++index)
           if(!CONNECTIONS[index])
               return index;

       return nSize;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class DataThread<TritiumNode>;
    template class DataThread<LegacyNode>;
    template class DataThread<TimeNode>;
    template class DataThread<CoreNode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;

}
