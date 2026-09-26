/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_STATELESS_CONNECTION_H
#define NEXUS_LLP_TEMPLATES_STATELESS_CONNECTION_H


#include <LLP/templates/base_connection.h>
#include <LLP/packets/stateless_packet.h>

namespace LLP
{

    /** StatelessConnection
     *
     *  A Connection class for handling 16-bit stateless mining protocol packets.
     *
     *  Packet Format (NexusMiner compatible):
     *  - HEADER: 2 bytes (big-endian)
     *  - LENGTH: 4 bytes (big-endian)
     *  - DATA:   LENGTH bytes
     *
     *  This is the proper framing for stateless mining opcodes (0xD007, 0xD008, etc.)
     *  and replaces the legacy 0xD0 prefix hack.
     *
     **/
    class StatelessConnection : public BaseConnection<StatelessPacket>
    {
    public:


        /** Default Constructor **/
        StatelessConnection();


        /** Constructor **/
        StatelessConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


         /** Constructor **/
         StatelessConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /** Default destructor **/
        virtual ~StatelessConnection();


        /** ReadPacket
         *
         *  Stateless Connection Read Packet Method.
         *  Reads 16-bit framed packets: HEADER (2) + LENGTH (4) + DATA (LENGTH)
         *
         **/
        void ReadPacket() final;

    };
}

#endif
