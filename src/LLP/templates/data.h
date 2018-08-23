/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TEMPLATES_DATA_H
#define NEXUS_LLP_TEMPLATES_DATA_H

#include "types.h"

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


        /* Data Thread. */
        Thread_t DATA_THREAD;


        DataThread<ProtocolType>(uint32_t id, bool isDDOS, uint32_t rScore, uint32_t cScore, uint32_t nTimeout, bool fMeter = false) :
            fDDOS(isDDOS), fMETER(fMeter), nConnections(0), ID(id), REQUESTS(0), TIMEOUT(nTimeout),  DDOS_rSCORE(rScore), DDOS_cSCORE(cScore), CONNECTIONS(0), DATA_THREAD(boost::bind(&DataThread::Thread, this)) { }


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
                CONNECTIONS.push_back(NULL);

            if(fDDOS)
                DDOS -> cSCORE += 1;

            CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
            CONNECTIONS[nSlot]->fCONNECTED = true;

            nConnections ++;
        }

        /* Adds a new connection to current Data Thread */
        bool AddConnection(std::string strAddress, int nPort, DDOS_Filter* DDOS)
        {
            int nSlot = FindSlot();
            if(nSlot == CONNECTIONS.size())
                CONNECTIONS.push_back(NULL);


            Socket_t SOCKET;
            CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
            CONNECTIONS[nSlot]->fOUTGOING = true;

            if(!CONNECTIONS[nSlot]->Connect(strAddress, nPort))
            {
                printf("Socket Failure %s\n", strAddress.c_str());

                delete CONNECTIONS[nSlot];
                CONNECTIONS[nSlot] = NULL;

                return false;
            }

            if(fDDOS)
                DDOS -> cSCORE += 1;

            CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
            CONNECTIONS[nSlot]->fCONNECTED = true;

            nConnections ++;

            return true;
        }

        /* Removes given connection from current Data Thread.
            Happens with a timeout / error, graceful close, or disconnect command. */
        void RemoveConnection(int index)
        {
            CONNECTIONS[index]->Disconnect();

            delete CONNECTIONS[index];

            CONNECTIONS[index] = NULL;

            nConnections --;
        }

        /* Thread that handles all the Reading / Writing of Data from Sockets.
            Creates a Packet QUEUE on this connection to be processed by an LLP Messaging Thread. */
        void Thread()
        {
            while(!fShutdown)
            {
                /* Keep data threads at 1000 FPS Maximum. */
                Sleep(1);

                /* Check all connections for data and packets. */
                int nSize = CONNECTIONS.size();
                for(int nIndex = 0; nIndex < nSize; nIndex++)
                {
                    Sleep(1);

                    try
                    {

                        /* Skip over Inactive Connections. */
                        if(!CONNECTIONS[nIndex])
                            continue;

                        /* Skip over Connection if not Connected. */
                        if(!CONNECTIONS[nIndex]->Connected())
                            continue;

                        /* Remove Connection if it has Timed out or had any Errors. */
                        if(CONNECTIONS[nIndex]->Timeout(TIMEOUT))
                        {
                            CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_TIMEOUT);

                            RemoveConnection(nIndex);

                            continue;
                        }

                        /* Remove Connection if it has Timed out or had any Errors. */
                        if(CONNECTIONS[nIndex]->Errors())
                        {
                            CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_ERRORS);

                            RemoveConnection(nIndex);

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

                                RemoveConnection(nIndex);

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

                            /* Packet Process return value of False will flag Data Thread to Disconnect. */
                            if(!CONNECTIONS[nIndex] -> ProcessPacket())
                            {
                                CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_FORCE);

                                RemoveConnection(nIndex);

                                continue;
                            }

                            CONNECTIONS[nIndex] -> ResetPacket();

                            if(fMETER)
                                REQUESTS++;

                            if(fDDOS)
                                CONNECTIONS[nIndex]->DDOS->rSCORE += 1;

                        }
                    }
                    catch(std::exception& e)
                    {
                        printf("data connection:  %s\n", e.what());

                        CONNECTIONS[nIndex]->Event(EVENT_DISCONNECT, DISCONNECT_ERRORS);

                        RemoveConnection(nIndex);
                    }
                }
            }
        }
    };
}

#endif
