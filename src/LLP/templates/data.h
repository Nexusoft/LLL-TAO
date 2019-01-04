/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

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

    /** Base Template Thread Class for Server base. Used for Core LLP Packet Functionality.
        Not to be inherited, only for use by the LLP Server Base Class. **/
    template <class ProtocolType>
    class DataThread
    {
        std::recursive_mutex MUTEX;
        //Need Pointer Reference to Object in Server Class to push data from Data Thread Messages into Server Class

    public:

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
        std::vector< ProtocolType* > CONNECTIONS;


        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;


        /* Data Thread. */
        std::thread DATA_THREAD;


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


        virtual ~DataThread<ProtocolType>()
        {
            fDestruct = true;

            CONDITION.notify_all();
            DATA_THREAD.join();

            for(auto & CONNECTION : CONNECTIONS)
                if(CONNECTION)
                    delete CONNECTION;
        }


        /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
        int FindSlot()
        {
            int nSize = CONNECTIONS.size();
            for(int index = 0; index < nSize; ++index)
                if(CONNECTIONS[index]->IsNull())
                    return index;

            return nSize;
        }


        /* Adds a new connection to current Data Thread */
        void AddConnection(Socket_t SOCKET, DDOS_Filter* DDOS)
        {
            LOCK(MUTEX);

            int nSlot = FindSlot();
            if(nSlot == CONNECTIONS.size())
            {
                CONNECTIONS.push_back(nullptr);
                CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
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


        /* Adds a new connection to current Data Thread */
        bool AddConnection(std::string strAddress, int nPort, DDOS_Filter* DDOS)
        {
            LOCK(MUTEX);

            int nSlot = FindSlot();
            if(nSlot == CONNECTIONS.size())
            {
                Socket SOCKET;
                CONNECTIONS.push_back(nullptr);
                CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
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


        /* Removes given connection from current Data Thread.
            Happens with a timeout / error, graceful close, or disconnect command. */
        void Remove(int index)
        {
            CONNECTIONS[index]->Disconnect();
            CONNECTIONS[index]->SetNull();

            --nConnections;

            CONDITION.notify_all();
        }


        /* Thread that handles all the Reading / Writing of Data from Sockets.
            Creates a Packet QUEUE on this connection to be processed by an LLP Messaging Thread. */
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
                uint32_t nSize = static_cast<uint32_t>(CONNECTIONS.size());
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
                            CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_ERRORS);

                            Remove(nIndex);

                            continue;
                        }


                        /* Remove Connection if it has Timed out or had any Errors. */
                        if(CONNECTIONS[nIndex]->Timeout(TIMEOUT))
                        {
                            CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_TIMEOUT);

                            Remove(nIndex);

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
                                CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_DDOS);

                                Remove(nIndex);

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
                            debug::log(4, NODE "Recieved Message (%u bytes)", CONNECTIONS[nIndex]->INCOMING.GetBytes().size());

                            /* Debug dump of packet data. */
                            if(config::GetArg("-verbose", 0) >= 5)
                                PrintHex(CONNECTIONS[nIndex]->INCOMING.GetBytes());

                            /* Handle Meters and DDOS. */
                            if(fMETER)
                                REQUESTS++;
                            if(fDDOS)
                                CONNECTIONS[nIndex]->DDOS->rSCORE += 1;

                            /* Packet Process return value of False will flag Data Thread to Disconnect. */
                            if(!CONNECTIONS[nIndex]->ProcessPacket())
                            {
                                CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_FORCE);

                                Remove(nIndex);

                                continue;
                            }

                            CONNECTIONS[nIndex]->ResetPacket();
                        }
                    }
                    catch(std::exception& e)
                    {
                        debug::log(0, "data connection:  %s", e.what());

                        CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_ERRORS);

                        Remove(nIndex);
                    }
                }
            }
        }
    };
}

#endif
