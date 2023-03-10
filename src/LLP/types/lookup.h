/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/random.h>

#include <LLP/packets/packet.h>
#include <LLP/templates/connection.h>

#include <Util/types/lock_unique_ptr.h>

namespace LLP
{

    class LookupNode : public Connection
    {
    public:

        /** Types are objects that can be sent in packets. **/
        struct REQUEST
        {
            enum : Packet::message_t
            {
                //begin protocol requests
                RESERVED1     = 0x00,

                /* Main protocol requests. */
                CONNECT       = 0x01,
                DEPENDANT     = 0x02,
                PROOF         = 0x03,

                //end protocol requests
                RESERVED2     = 0x04,
            };

            /** VALID
             *
             *  Inline function to check if message request is valid request.
             *
             *  @param[in] nMsg The message value to check if valid.
             *
             *  @return true if the request was in range.
             */
            static inline bool VALID(const Packet::message_t nMsg)
            {
                return (nMsg > RESERVED1 && nMsg < RESERVED2);
            }
        };

        /** Status returns available states. **/
        struct RESPONSE
        {
            enum : Packet::message_t
            {
                MERKLE       = 0x11, //for legacy data types
                MISSING      = 0x12, //if the data was not found
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
                PROOF        = 0x24, //specify for a proof type
            };
        };


        /** Set our static locked ptr. **/
        static util::atomic::lock_unique_ptr<std::set<uint64_t>> setRequests;


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


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void PushMessage(const uint8_t nMsg, Args&&... args)
        {
            /* Buyild a datastream with the message parameters. */
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            ((ssData << args), ...);

            /* Build the message packet. */
            Packet RESPONSE(nMsg);
            RESPONSE.SetData(ssData);

            /* Write to sockets. */
            WritePacket(RESPONSE);

            /* Log the packet details if verbose is set. */
            if(config::nVerbose >= 4)
                debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** BlockingLookup
         *
         *  Adds a packet to the queue and waits for the peer to respond to the message.
         *
         *  @param[in] pNode Pointer to the TritiumNode connection instance to push the message to.
         *  @param[in] nRequest The message request for lookup
         *  @param[in] args variable args to be sent in the message.
         **/
        template<typename... Args>
        void BlockingLookup(const uint32_t nTimeout, const uint8_t nRequest, Args&&... args)
        {
            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Create our trigger nonce. */
            const uint64_t nRequestID = LLC::GetRand();

            /* Add to our request tracker and make request. */
            LookupNode::setRequests->insert(nRequestID);
            PushMessage(nRequest, nRequestID, std::forward<Args>(args)...);

            /* Create the condition variable trigger. */
            LLP::Trigger REQUEST_TRIGGER;
            AddTrigger(RESPONSE::MERKLE, &REQUEST_TRIGGER);

            /* Process the event. */
            REQUEST_TRIGGER.wait_for_timeout(nRequestID, nTimeout);

            /* Cleanup our event trigger. */
            Release(RESPONSE::MERKLE);
        }
    };
}
