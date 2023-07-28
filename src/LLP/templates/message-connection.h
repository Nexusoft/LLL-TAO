/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once


#include <LLP/templates/base_connection.h>
#include <LLP/packets/message.h>

namespace LLP
{

    /** Connection
     *
     *  A Default Connection class that handles the Reading of a default Packet Type.
     *
     **/
    class MessageConnection : public BaseConnection<MessagePacket>
    {
    public:


        /** Default Constructor **/
        MessageConnection();


        /** Constructor **/
        MessageConnection(const Socket &SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /** Constructor **/
        MessageConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn = false, bool fOutgoing = false);


        /** Default destructor **/
        virtual ~MessageConnection();


        /** ReadPacket
         *
         *  Regular Connection Read Packet Method.
         *
         **/
        void ReadPacket() final;

    };
}
