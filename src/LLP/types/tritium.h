/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_TRITIUM_H
#define NEXUS_LLP_TYPES_TRITIUM_H

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/tritium.h>
#include <LLP/templates/base_connection.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>

#include <TAO/Ledger/types/tritium.h>

#include <Util/include/memory.h>

namespace LLP
{

    /** Actions invoke behavior in remote node. **/
    namespace ACTION
    {
        enum
        {
            RESERVED     = 0,

            /* Verbs. */
            LIST         = 0x10,
            GET          = 0x11,
            NOTIFY       = 0x12,
            AUTH         = 0x13,
            DEAUTH       = 0x14,
            VERSION      = 0x15,
            SUBSCRIBE    = 0x16,
            UNSUBSCRIBE  = 0x17,

            /* Protocol. */
            PING         = 0x1a,
            PONG         = 0x1b

        };
    }


    /** Types are objects that can be sent in packets. **/
    namespace TYPES
    {
        enum
        {
            /* Key Types. */
            UINT256_T    = 0x20,
            UINT512_T    = 0x21,
            UINT1024_T   = 0x22,
            STRING       = 0x23,
            BYTES        = 0x24,
            LOCATOR      = 0x25,
            LASTINDEX    = 0x26, //sends a last index notify after list

            /* Object Types. */
            BLOCK        = 0x30,
            TRANSACTION  = 0x31,
            TIMESEED     = 0x32,
            BESTHEIGHT   = 0x33,
            CHECKPOINT   = 0x34,
            ADDRESS      = 0x35,
            BESTCHAIN    = 0x36,
            MEMPOOL      = 0x37,
            SIGCHAIN     = 0x38,
            MERKLE       = 0x39,
            GENESIS      = 0x3a,
            NOTIFICATION = 0x3b,
            TRIGGER      = 0x3c,
        };
    }


    /** Specifiers describe object type in greater detail. **/
    namespace SPECIFIER
    {
        enum
        {
            /* Specifier. */
            LEGACY       = 0x40, //specify for legacy data types
            TRITIUM      = 0x41, //specify for tritium data types
            SYNC         = 0x42,  //specify a sync block type
            TRANSACTIONS = 0x43, //specify to send memory transactions first
            CLIENT       = 0x44, //specify for blocks to be sent and received for clients
        };
    }


    /** Status returns available states. **/
    namespace RESPONSE
    {
        enum
        {
            ACCEPTED     = 0x50,
            REJECTED     = 0x51,
            STALE        = 0x52,
            UNSUBSCRIBED = 0x53, //let node know it was unsubscribed successfully
            AUTHORIZED   = 0x54,
            COMPLETED    = 0x55, //let node know an event was completed
        };
    }


    /** Subscription flags. */
    namespace SUBSCRIPTION
    {
        enum
        {
            BLOCK       = (1 << 1),
            TRANSACTION = (1 << 2),
            TIMESEED    = (1 << 3),
            BESTHEIGHT  = (1 << 4),
            CHECKPOINT  = (1 << 5),
            ADDRESS     = (1 << 6),
            LASTINDEX   = (1 << 7),
            BESTCHAIN   = (1 << 8),
            SIGCHAIN    = (1 << 9),
        };
    }


    /** TritiumNode
     *
     *  A Node that processes packets and messages for the Tritium Server
     *
     **/
    class TritiumNode : public BaseConnection<TritiumPacket>
    {

        /** Switch Node
         *
         *  Helper function to switch available nodes.
         *
         **/
        static void SwitchNode();


        /** message_args
         *
         *  Overload of variadic templates
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *
         **/
        template<class Head>
        void message_args(DataStream& s, Head&& head)
        {
            s << std::forward<Head>(head);
        }


        /** message_args
         *
         *  Variadic template pack to handle any message size of any type.
         *
         *  @param[out] s The data stream to write to
         *  @param[in] head The object being written
         *  @param[in] tail The variadic paramters
         *
         **/
        template<class Head, class... Tail>
        void message_args(DataStream& s, Head&& head, Tail&&... tail)
        {
            s << std::forward<Head>(head);
            message_args(s, std::forward<Tail>(tail)...);
        }


        /** State of if this node has logged in to remote node. **/
        std::atomic<bool> fLoggedIn;


        /** State of if remote node has currently verified signature. **/
        std::atomic<bool> fAuthorized;


        /** State to keep track of if node has completed initialization process. **/
        std::atomic<bool> fInitialized;


        /** Mutex for connected sessions. **/
        static std::mutex SESSIONS_MUTEX;


        /** Set for connected session. **/
        static std::map<uint64_t, std::pair<uint32_t, uint32_t>> mapSessions;


        /** The current subscriptions. **/
        uint16_t nSubscriptions;


        /** The current notifications. **/
        uint16_t nNotifications;


    public:

        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Tritium"; }


        /** The block height at the start of the last sync session **/
        static std::atomic<uint32_t> nSyncStart;


        /** Tracks the time taken to synchronize  **/
        static runtime::timer SYNCTIMER;


        /** Keeps track of if this node is fully synchronized. **/
        static std::atomic<bool> fSynchronized;


        /** The last time a block was accepted. **/
        static std::atomic<uint64_t> nLastTimeReceived;


        /** Default Constructor **/
        TritiumNode();


        /** Constructor **/
        TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Constructor **/
        TritiumNode(DDOS_Filter* DDOS_IN, bool fDDOSIn = false);


        /** Default Destructor **/
        virtual ~TritiumNode();


        /** Counter to keep track of the last time a ping was made. **/
        std::atomic<uint64_t> nLastPing;


        /** Counter to keep track of last time sample request. */
        std::atomic<uint64_t> nLastSamples;


        /** timer object to keep track of ping latency. **/
        std::map<uint64_t, runtime::timer> mapLatencyTracker;


        /** The current genesis-id of connected peer. **/
        uint256_t hashGenesis;


        /** The current trust of the connected peer. **/
        uint64_t nTrust;


        /** This node's protocol version. **/
        uint32_t nProtocolVersion;


        /** This node's session-id. **/
        uint64_t nCurrentSession;


        /** This node's current height. **/
        uint32_t nCurrentHeight;


        /** This node's current checkpoint. **/
        uint1024_t hashCheckpoint;


        /** This node's best block. **/
        uint1024_t hashBestChain;


        /** This node's current last index. **/
        uint1024_t hashLastIndex;


        /** Counter of total orphans. **/
        uint32_t nConsecutiveOrphans;


        /** Counter of total failures. **/
        uint32_t nConsecutiveFails;


        /** The node's full version string. **/
        std::string strFullVersion;


        /** Timestamp of unsubscription. **/
        uint64_t nUnsubscribed;


        /** Current nonce trigger. **/
        uint64_t nTriggerNonce;


        /** Remaining time for sync meter. **/
        static std::atomic<uint64_t> nRemainingTime;


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


        /** Authorized
         *
         *  Determine if a node is authorized and therfore trusted.
         *
         **/
        bool Authorized() const;


        /** Unsubscribe
         *
         *  Unsubscribe from another node for notifications.
         *
         *  @param[in] nFlags The subscription flags.
         *
         **/
        void Unsubscribe(const uint16_t nFlags);


        /** Subscribe
         *
         *  Subscribe to another node for notifications.
         *
         *  @param[in] nFlags The subscription flags.
         *  @param[in] fSubscribe Flag to determine whether subscibing or unsubscribing
         *
         **/
        void Subscribe(const uint16_t nFlags, bool fSubscribe = true);


        /** Notifications
         *
         *  Checks if a node is subscribed to receive a notification.
         *
         *  @return a data stream with relevant relay information
         *
         **/
        const DataStream Notifications(const uint16_t nMsg, const DataStream& ssData) const;


        /** Auth
         *
         *  Authorize this node to the connected node .
         *
         *  @param[in] fAuth Flag to determine whether authorizing or de-authorizing
         *
         **/
        void Auth(bool fAuth);


        /** GetAuth
         *
         *  Builds an Auth message for this node.
         *
         *  @param[in] fAuth Flag to determine whether authorizing or de-authorizing
         *
         **/
        static DataStream GetAuth(bool fAuth);


        /** SessionActive
         *
         *  Determine whether a session is connected.
         *
         *  @param[in] nSession The session to check for
         *
         *  @return true if session is connected.
         *
         **/
        static bool SessionActive(const uint64_t nSession);


        /** GetNode
         *
         *  Get a node by connected session.
         *
         *  @param[in] nSession The session to receive
         *
         *  @return a pointer to connected node.
         *
         **/
        static memory::atomic_ptr<TritiumNode>& GetNode(const uint64_t nSession);


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
        static TritiumPacket NewMessage(const uint16_t nMsg, const DataStream& ssData)
        {
            TritiumPacket RESPONSE(nMsg);
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
            TritiumPacket RESPONSE(nMsg);
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

    };

}

#endif
