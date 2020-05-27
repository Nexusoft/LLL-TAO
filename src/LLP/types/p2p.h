/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/random.h>

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/message.h>
#include <LLP/templates/base_connection.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/trigger.h>

#include <Util/include/memory.h>

namespace LLP
{
    /* P2P namespace to encapsulate P2P LLP message types */
    namespace P2P 
    {
        /* Used to determine the features available in the P2P LLP */
        const uint32_t PROTOCOL_VERSION = 1000000;

         /* Used to define the minimum protocol version that this node can communicate with. */
        const uint32_t MIN_P2P_VERSION = 1000000;

        /** Actions invoke behavior in remote node. **/
        namespace ACTION
        {
            enum
            {
                RESERVED     = 0,

                /* Verbs. */
                INITIALIZE   = 0x10, // starts a new P2P session
                TERMINATE    = 0x11, // terminates the current P2P session
                NOTIFY       = 0x12, 
                
                /* Protocol. */
                PING         = 0x1a,
                PONG         = 0x1b,

            };
        }


        /** Types are objects that can be sent in packets. **/
        namespace TYPES
        {
            enum
            {
                /* Key Types. */
                P2PMESSAGE    = 0x20,
                TRIGGER    = 0x21,                
            };
        }


    
        /** Response message codes. **/
        namespace RESPONSE
        {
            enum
            {
                ACCEPTED     = 0x50,
                REJECTED     = 0x51,
                COMPLETED    = 0x52, //let node know an event was completed
            };
        }

    } // End namespace P2P


    /** P2PNode
     *
     *  A Node that processes packets and messages for the P2P connection
     *
     **/
    class P2PNode : public BaseConnection<MessagePacket>
    {

        /** State to keep track of if node has completed initialization process. **/
        std::atomic<bool> fInitialized;


        /** Mutex for connected sessions. **/
        static std::mutex SESSIONS_MUTEX;


        /** Map connected sessions.  The key to this map is a tuple consisting of the app ID, genesis hash and peer genesis hash **/
        static std::map<std::tuple<uint64_t, uint256_t, uint256_t>, std::pair<uint32_t, uint32_t>> mapSessions;

    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "P2P"; }



        /** Default Constructor **/
        P2PNode();


        /** Constructor for incoming connection **/
        P2PNode(const Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn);


        /** Constructor for outgoing connection **/
        P2PNode(DDOS_Filter* DDOS_IN, bool fDDOSIn,
                const uint64_t& APPID,
                const uint256_t& HASHGENESIS,
                const uint256_t& HASHPEER,
                const uint64_t& SESSIONID);

        /** Constructor for unauthorized outgoing connection **/
        P2PNode(DDOS_Filter* DDOS_IN, bool fDDOSIn);


        /** Default Destructor **/
        virtual ~P2PNode();

        /** The application ID for this connection **/
        uint64_t nAppID;


        /** The current genesis hash of this user. **/
        uint256_t hashGenesis;


        /** The genesis hash of connected peer. **/
        uint256_t hashPeer;


        /** This node's session-id. **/
        uint64_t nSession;


        /** This node's protocol version. **/
        uint32_t nProtocolVersion;


        /** Counter to keep track of the last time a ping was made. **/
        std::atomic<uint64_t> nLastPing;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** Current nonce trigger. **/
        uint64_t nTriggerNonce;


    
        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0) final;


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket() final;


        /** ReadPacket
         *
         *  Non-Blocking Packet reader to build a packet from TCP Connection.
         *  This keeps thread from spending too much time for each Connection.
         *
         **/
        void ReadPacket() final;


       
        /** SessionActive
         *
         *  Determine whether a session is connected for the specified app / genesis / peer.
         *
         *  @param[in] nAppID The app ID to check for
         *  @param[in] hashGenesis The genesis hash to search for
         *  @param[in] hashGenesis The peer genesis hash to search for 
         *
         *  @return true if session is connected.
         *
         **/
        static bool SessionActive(const uint64_t& nAppID, const uint256_t& hashGenesis, const uint256_t& hashPeer);


        /** Matches
         *
         *  Checks to see if this connection instance matches the specified app / genesis / peer.
         *
         *  @param[in] nAppID The app ID to check for
         *  @param[in] hashGenesis The genesis hash to search for
         *  @param[in] hashGenesis The peer genesis hash to search for 
         *
         *  @return true if this connection instance matches.
         *
         **/
        bool Matches(const uint64_t& nAppID, const uint256_t& hashGenesis, const uint256_t& hashPeer);


        /** NewMessage
         *
         *  Creates a new message with a commands and data.
         *
         *  @param[in] nMsg The message type.
         *  @param[in] ssData A datastream object with data to write.
         *
         *  @return Returns a filled out tritium packet.
         *
         **/
        static MessagePacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            MessagePacket RESPONSE(nMsg);
            RESPONSE.SetData(ssData);

            return RESPONSE;
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         *  @param[in] nMsg The message type.
         *
         **/
        void PushMessage(const uint16_t nMsg)
        {
            MessagePacket RESPONSE(nMsg);
            WritePacket(RESPONSE);
        }


        /** PushMessage
         *
         *  Adds a tritium packet to the queue to write to the socket.
         *
         **/
        template<typename... Args>
        void PushMessage(const uint16_t nMsg, Args&&... args)
        {
            DataStream ssData(SER_NETWORK, MIN_PROTO_VERSION);
            message_args(ssData, std::forward<Args>(args)...);

            WritePacket(NewMessage(nMsg, ssData));

            debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** BlockingMessage
         *
         *  Adds a tritium packet to the queue and waits for the peer to send a COMPLETED message.
         *  NOTE: this is a static method taking the node reference as a parameter to avoid locking access to the connection
         *  in the atomic_ptr.  If we did not do this, the data threads could not access the atomic_ptr to process the incoming
         *  messages until trigger timed out and this method returned
         *
         *  @param[in] pNode Pointer to the P2PNode connection instance to push the message to.
         *  @param[in] nMsg The message type.
         *  @param[in] args variable args to be sent in the message.
         **/
        template<typename... Args>
        static void BlockingMessage(const uint32_t nTimeout, memory::atomic_ptr<LLP::P2PNode>& pNode, const uint16_t nMsg, Args&&... args)
        {
            /* Create our trigger nonce. */
            uint64_t nNonce = LLC::GetRand();
            pNode->PushMessage(LLP::P2P::TYPES::TRIGGER, nNonce);

            /* Request the inventory message. */
            pNode->PushMessage(nMsg, std::forward<Args>(args)...);

            /* Create the condition variable trigger. */
            LLP::Trigger REQUEST_TRIGGER;
            pNode->AddTrigger(LLP::P2P::RESPONSE::COMPLETED, &REQUEST_TRIGGER);

            /* Process the event. */
            REQUEST_TRIGGER.wait_for_nonce(nNonce, nTimeout);

            /* Cleanup our event trigger. */
            pNode->Release(LLP::P2P::RESPONSE::COMPLETED);

        }

    };

}

