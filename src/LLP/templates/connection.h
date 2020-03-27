/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_CONNECTION_H
#define NEXUS_LLP_TEMPLATES_CONNECTION_H


#include <LLP/templates/base_connection.h>
#include <LLP/packets/packet.h>

namespace LLP
{

    /** Connection
     *
     *  A Default Connection class that handles the Reading of a default Packet Type.
     *
     **/
    class Connection : public BaseConnection<Packet>
    {
    public:


        /** Default Constructor **/
        Connection();


        /** Constructor **/
        Connection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


         /** Constructor **/
         Connection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /** Default destructor **/
        virtual ~Connection();


        /** ReadPacket
         *
         *  Regular Connection Read Packet Method.
         *
         **/
        void ReadPacket() final;

    };
}

#endif
