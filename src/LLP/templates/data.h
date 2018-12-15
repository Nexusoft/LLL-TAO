/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_DATA_H
#define NEXUS_LLP_TEMPLATES_DATA_H

#include <LLP/templates/types.h>
#include <condition_variable>
#include <functional>

namespace LLP
{

    /** Base Template Thread Class for Server base. Used for Core LLP Packet Functionality.
        Not to be inherited, only for use by the LLP Server Base Class. **/
    template <class ProtocolType> class DataThread
    {
        //Need Pointer Reference to Object in Server Class to push data from Data Thread Messages into Server Class

    public:

        /* Variables to track Connection / Request Count. */
        bool fDDOS, fMETER;

        uint32_t nConnections, ID, REQUESTS, TIMEOUT, DDOS_rSCORE, DDOS_cSCORE;


        /* Vector to store Connections. */
        std::vector< ProtocolType* > CONNECTIONS;


        /* The condition for thread sleeping. */
        std::condition_variable CONDITION;


        /* Data Thread. */
        std::thread DATA_THREAD;


        DataThread<ProtocolType>(uint32_t id, bool isDDOS, uint32_t rScore, uint32_t cScore, uint32_t nTimeout, bool fMeter = false) :
            fDDOS(isDDOS), fMETER(fMeter), nConnections(0), ID(id), REQUESTS(0), TIMEOUT(nTimeout),  DDOS_rSCORE(rScore), DDOS_cSCORE(cScore), CONNECTIONS(0), DATA_THREAD(std::bind(&DataThread::Thread, this)) { }


        virtual ~DataThread<ProtocolType>() { fMETER  = false; }


        /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
        int FindSlot()
        {
            int nSize = CONNECTIONS.size();
            for(int index = 0; index < nSize; index++)
                if(!CONNECTIONS[index])
                    return index;

            return nSize;
        }

        /* Adds a new connection to current Data Thread */
        void AddConnection(Socket_t SOCKET, DDOS_Filter* DDOS)
        {
            int nSlot = FindSlot();
            if(nSlot == CONNECTIONS.size())
                CONNECTIONS.push_back(nullptr);

            if(fDDOS)
                DDOS -> cSCORE += 1;

            CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
            CONNECTIONS[nSlot]->fCONNECTED = true;

            nConnections ++;

            CONDITION.notify_all();
        }

        /* Adds a new connection to current Data Thread */
        bool AddConnection(std::string strAddress, int nPort, DDOS_Filter* DDOS)
        {
            int nSlot = FindSlot();
            if(nSlot == CONNECTIONS.size())
                CONNECTIONS.push_back(nullptr);


            Socket_t SOCKET;
            CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
            CONNECTIONS[nSlot]->fOUTGOING = true;

            if(!CONNECTIONS[nSlot]->Connect(strAddress, nPort))
            {
                delete CONNECTIONS[nSlot];
                CONNECTIONS[nSlot] = nullptr;

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

            delete CONNECTIONS[index];

            CONNECTIONS[index] = nullptr;

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
            while(!config::fShutdown)
            {
                /* Keep data threads waiting for work. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                CONDITION.wait_for(CONDITION_LOCK, std::chrono::milliseconds(1000), [this]{ return CONNECTIONS.size() > 0; });

                /* Check all connections for data and packets. */
                int nSize = CONNECTIONS.size();
                for(int nIndex = 0; nIndex < nSize; nIndex++)
                {
                    try
                    {
                        //TODO: Cleanup threads and sleeps. Make more efficient to reduce total CPU cycles
                        runtime::Sleep(10);

                        /* Skip over Inactive Connections. */
                        if(!CONNECTIONS[nIndex] || !CONNECTIONS[nIndex]->Connected())
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
                            if(CONNECTIONS[nIndex]->DDOS->rSCORE.Score() > DDOS_rSCORE || CONNECTIONS[nIndex]->DDOS->cSCORE.Score() > DDOS_cSCORE)
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
