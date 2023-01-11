/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once

#include <LLP/packets/packet.h>
#include <LLP/templates/connection.h>

namespace LLP
{

    class LookupNode : public Connection
    {
        /** Types are objects that can be sent in packets. **/
        struct REQUEST
        {
            enum : Packet::message_t
            {
                /* Object Types. */
                DEPENDANT     = 0x01,
                TRIGGER       = 0x02,
                PING          = 0x03,
            };
        };

        /** Status returns available states. **/
        struct RESPONSE
        {
            enum : Packet::message_t
            {
                MERKLE       = 0x11,
                COMPLETED    = 0x12, //let node know an event was completed
                PONG         = 0x13,
            };
        };

        /** Specifiers describe object type in greater detail. **/
        struct SPECIFIER
        {
            enum : Packet::message_t
            {
                /* Specifier. */
                LEGACY       = 0x21, //specify for legacy data types
                TRITIUM      = 0x22, //specify for tritium data types
                REGISTER     = 0x23, //specify for register transaction
            };
        };

    public:


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Lookup"; }


        /** Constructor **/
        LookupNode();


        /** Constructor **/
        LookupNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        LookupNode(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /* Virtual destructor. */
        virtual ~LookupNode();


        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket();

    };
}
