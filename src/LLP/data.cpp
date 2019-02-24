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
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>

#include <Util/include/hex.h>

#include <new> //std::bad_alloc

namespace LLP
{

    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(uint32_t id, bool isDDOS,
                                         uint32_t rScore, uint32_t cScore,
                                         uint32_t nTimeout, bool fMeter)
    : MUTEX()
    , fDDOS(isDDOS)
    , fMETER(fMeter)
    , fDestruct(false)
    , nConnections(0)
    , ID(id)
    , REQUESTS(0)
    , TIMEOUT(nTimeout)
    , DDOS_rSCORE(rScore)
    , DDOS_cSCORE(cScore)
    , CONNECTIONS(0)
    , POLLFDS(0)
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
        pollfd pollfdForConnection;

        node->fCONNECTED.store(true);

        {
            LOCK(MUTEX);

            int nSlot = find_slot();

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS.size())
            {
                CONNECTIONS.push_back(memory::atomic_ptr<ProtocolType>(node));
                POLLFDS.push_back(pollfdForConnection);
            }
            else
                CONNECTIONS[nSlot].store(node);

            POLLFDS[nSlot].fd = node->fd;
            POLLFDS[nSlot].events = node->events;

            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
        }

        if(fDDOS)
            DDOS -> cSCORE += 1;

        ++nConnections;

        CONDITION.notify_all();
    }


    /*  Adds a new connection to current Data Thread */
    template <class ProtocolType>
    bool DataThread<ProtocolType>::AddConnection(std::string strAddress, uint16_t nPort, DDOS_Filter* DDOS)
    {
        /* Create a new pointer on the heap. */
        ProtocolType* node = new ProtocolType(DDOS, fDDOS);
        pollfd pollfdForConnection;

        if(!node->Connect(strAddress, nPort))
        {
            node->Disconnect();
            delete node;

            return false;
        }

        node->fOUTGOING = true;

        {
            LOCK(MUTEX);

            int nSlot = find_slot();

            /* Find a slot that is empty. */
            if(nSlot == CONNECTIONS.size())
            {
                CONNECTIONS.push_back(memory::atomic_ptr<ProtocolType>(node));
                POLLFDS.push_back(pollfdForConnection);
            }
            else
                CONNECTIONS[nSlot].store(node);

            POLLFDS[nSlot].fd = node->fd;
            POLLFDS[nSlot].events = node->events;

            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
        }

        if(fDDOS)
            DDOS -> cSCORE += 1;

        ++nConnections;

        CONDITION.notify_all();

        return true;
    }


    /*  Disconnects all connections by issuing a DISCONNECT_FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
       uint32_t nSize = 0;
       {
           LOCK(MUTEX);
           nSize = static_cast<uint32_t>(CONNECTIONS.size());
       }

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
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK, [this]{ return fDestruct.load() || config::fShutdown || nConnections.load() > 0; });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Wrapped mutex lock. */
            uint32_t nSize = 0;
            {
                LOCK(MUTEX);

                /* Get the total connections. */
                nSize = static_cast<uint32_t>(CONNECTIONS.size());

                /* We should have connections, as predicate of releasing condition wait. This is a precaution, checking after getting MUTEX lock */
                if (nSize == 0)
                    continue;

                /* Initialize the revents for all connection pollfd structures. One connection must be live, so verify that and skip if none */
                bool fHasValidConnections = false;
                for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    POLLFDS[nIndex].revents = 0;
                    if (POLLFDS[nIndex].fd != INVALID_SOCKET)
                        fHasValidConnections = true;
                }

                if (!fHasValidConnections)
                    continue;
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
                    /* Grab a connection handle for this index. */
                    ProtocolType *pNode = CONNECTIONS[nIndex].load();

                    /* Skip over Inactive Connections. */
                    if(!pNode || !pNode->Connected())
                        continue;

                    /* Disconnect if there was a polling error */
                    if((POLLFDS[nIndex].revents & POLLERR) || (POLLFDS[nIndex].revents & POLLNVAL))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(pNode->Errors())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors. */
                    if(pNode->Timeout(TIMEOUT))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT_TIMEOUT);
                        continue;
                    }

                    /* Handle any DDOS Filters. */
                    if(fDDOS && pNode->DDOS)
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(pNode->DDOS->rSCORE.Score() > DDOS_rSCORE || pNode->DDOS->cSCORE.Score() > DDOS_cSCORE)
                            pNode->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(pNode->DDOS->Banned())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_DDOS);
                            continue;
                        }
                    }

                    /* Generic event for Connection. */
                    pNode->Event(EVENT_GENERIC);

                    /* Flush the write buffer. */
                    pNode->Flush();

                    /* Work on Reading a Packet. **/
                    pNode->ReadPacket();

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(pNode->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        debug::log(4, FUNCTION, "Recieved Message (", pNode->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::GetArg("-verbose", 0) >= 5)
                            PrintHex(pNode->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++REQUESTS;
                        if(fDDOS)
                            pNode->DDOS->rSCORE += 1;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!pNode->ProcessPacket())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT_FORCE);
                            continue;
                        }

                        pNode->ResetPacket();
                    }
                }
                catch(const std::bad_alloc &e)
                {
                    debug::error(FUNCTION, "Memory allocation failed ", e.what());
                    debug::error(FUNCTION, "Currently running ", nConnections, " connections.");
                    disconnect_remove_event(nIndex, DISCONNECT_ERRORS);
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


    /*  Get the number of active connection pointers from data threads. */
     template <class ProtocolType>
     uint16_t DataThread<ProtocolType>::GetConnectionCount()
     {
         uint16_t nConnectionCount = 0;
         uint16_t nSize = 0;
         uint16_t nIndex = 0;

         LOCK(MUTEX);

         nSize = static_cast<uint16_t>(CONNECTIONS.size());

         /* Loop through connections in data thread and add any that are connected to count. */
         for(; nIndex < nSize; ++nIndex)
         {
             /* Skip over inactive connections. */
             if(!CONNECTIONS[nIndex])
                 continue;

             ++nConnectionCount;
         }

         return nConnectionCount;
     }

     template <class ProtocolType>
     uint16_t DataThread<ProtocolType>::GetBestConnection(const BaseAddress& addrExclude, uint32_t &nLatency)
     {
          uint16_t nIndex = 0;
          uint16_t nSize = 0;

          {
              LOCK(MUTEX);
              nSize = static_cast<uint16_t>(CONNECTIONS.size());
          }

          for(uint16_t i = 0; i < nSize; ++i)
          {
              try
              {
                  /* Skip over inactive connections. */
                  if(!CONNECTIONS[i])
                      continue;

                  /* Skip over exclusion address. */
                  if(CONNECTIONS[i]->GetAddress() == addrExclude)
                      continue;

                  /* Choose the active index, update lowest latency output. */
                  if(CONNECTIONS[i]->nLatency < nLatency)
                  {
                      nLatency = CONNECTIONS[i]->nLatency;
                      nIndex = i;
                  }
              }
              catch(const std::runtime_error& e)
              {
                  debug::error(FUNCTION, e.what());
              }
          }

          return nIndex;
     }


    /*  Fires off a Disconnect event with the given disconnect reason
     *  and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::disconnect_remove_event(uint32_t index, uint8_t reason)
    {
        CONNECTIONS[index]->Event(EVENT_DISCONNECT, reason);

        remove(index);
    }


    /*  Removes given connection from current Data Thread.
     *  This happens with a timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove(int index)
    {
        /* Get the total connections. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS.size());

        if(index < nSize && CONNECTIONS[index] != nullptr)
        {
            /* Free the memory. */
            CONNECTIONS[index].free();
            POLLFDS[index].fd = INVALID_SOCKET;

            --nConnections;
        }

        CONDITION.notify_all();
    }


    /*  Returns the index of a component of the CONNECTIONS vector that
     *  has been flagged Disconnected */
    template <class ProtocolType>
    int DataThread<ProtocolType>::find_slot()
    {
        /* Get the total connections. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS.size());

        for(int index = 0; index < nSize; ++index)
        {
            if(CONNECTIONS[index] == nullptr)
                return index;
        }

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
