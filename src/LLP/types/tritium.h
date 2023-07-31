/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TYPES_TRITIUM_H
#define NEXUS_LLP_TYPES_TRITIUM_H

#include <LLC/include/random.h>

#include <LLP/include/network.h>
#include <LLP/include/version.h>
#include <LLP/packets/message.h>
#include <LLP/templates/base_connection.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/templates/trigger.h>

#include <TAO/Ledger/types/tritium.h>

#include <Util/include/memory.h>

namespace LLP
{

    /** TritiumNode
     *
     *  A Node that processes packets and messages for the Tritium Server
     *
     **/
    class TritiumNode : public BaseConnection<MessagePacket>
    {
        /** @class Inventory class
         *
         *  Track inventory with expiring cache.
         *
         **/
        class Inventory
        {
            /** Inventory set with expiring cache. **/
            std::map<uint512_t, uint64_t> mapInventory;


            /** Mutex to lock internal map inventory. **/
            std::recursive_mutex MUTEX;

        public:

            /** Default Constructor. **/
            Inventory ( )
            : mapInventory ( )
            , MUTEX        ( )
            {
            }


            /** Expired
             *
             *  Check if given txid cache is expired to be requested from peer.
             *
             *  @param[in] hashTx The txid that we want to filter out.
             *  @param[in] nTimeout The time to wait for cache timeout.
             *
             *  @return true if the txid is available for request.
             *
             **/
            bool Expired(const uint512_t& hashTx, const uint32_t nTimeout = 15)
            {
                RECURSIVE(MUTEX);

                /* Check if we don't have it in our map. */
                if(!mapInventory.count(hashTx))
                    return true;

                /* Check our timeout here. */
                if(mapInventory[hashTx] + nTimeout < runtime::unifiedtimestamp())
                {
                    /* Cleanup our memory records. */
                    mapInventory.erase(hashTx);
                    return true;
                }

                return false; //otherwise don't request it
            }


            /** Cache
             *
             *  Caches current txid to be filtered for given timestamp.
             *
             *  @param[in] hashTx The txid that we want to filter out.
             *
             **/
            void Cache(const uint512_t& hashTx)
            {
                RECURSIVE(MUTEX);

                /* Set our inventory value to our current time. */
                mapInventory[hashTx] = runtime::unifiedtimestamp();
            }
        };

    public: //encapsulate protocol messages inside node class

        /** Actions invoke behavior in remote node. **/
        struct ACTION
        {
            /** Limit for maximum items that can be requested per packet. **/
            static const uint32_t GET_MAX_ITEMS = 100;


            /** Limit for maximum notifications that can be broadcast per packet. **/
            static const uint32_t NOTIFY_MAX_ITEMS = 100;


            /** Limit for maximum event notifications that can be broadcast per packet. **/
            static const uint32_t LIST_NOTIFICATIONS_MAX_ITEMS = 100;


            /** Limit for maximum subscriptions that can be requested per packet. **/
            static const uint32_t SUBSCRIBE_MAX_ITEMS = 100;


            /* Message enumeration values. */
            enum : MessagePacket::message_t
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
        };


        /** Types are objects that can be sent in packets. **/
        struct TYPES
        {
            enum : MessagePacket::message_t
            {
                /* Key Types. */
                UINT256_T     = 0x20,

                UINT512_T     = 0x21,
                UINT1024_T    = 0x22,
                STRING        = 0x23,
                BYTES         = 0x24,
                LOCATOR       = 0x25,
                LASTINDEX     = 0x26, //sends a last index notify after list

                /* Object Types. */
                BLOCK         = 0x30,
                TRANSACTION   = 0x31,
                TIMESEED      = 0x32,
                BESTHEIGHT    = 0x33,
                CHECKPOINT    = 0x34,
                ADDRESS       = 0x35,
                BESTCHAIN     = 0x36,
                MEMPOOL       = 0x37,
                SIGCHAIN      = 0x38,
                MERKLE        = 0x39,
                GENESIS       = 0x3a,
                NOTIFICATION  = 0x3b,
                TRIGGER       = 0x3c,
                REGISTER      = 0x3d,
            };
        };


        /** Specifiers describe object type in greater detail. **/
        struct SPECIFIER
        {
            enum : MessagePacket::message_t
            {
                /* Specifier. */
                LEGACY       = 0x40, //specify for legacy data types
                TRITIUM      = 0x41, //specify for tritium data types
                SYNC         = 0x42, //specify a sync block type
                TRANSACTIONS = 0x43, //specify to send memory transactions first
                CLIENT       = 0x44, //specify for blocks to be sent and received for clients
                REGISTER     = 0x45, //specify that a register is being received and should only keep memory of it.
                DEPENDANT    = 0x46, //specify that a transaction is a dependant and therefore only process the ledger layer.
            };
        };


        /** Status returns available states. **/
        struct RESPONSE
        {
            enum : MessagePacket::message_t
            {
                ACCEPTED     = 0x50,
                REJECTED     = 0x51,
                STALE        = 0x52,
                UNSUBSCRIBED = 0x53, //let node know it was unsubscribed successfully
                AUTHORIZED   = 0x54,
                COMPLETED    = 0x55, //let node know an event was completed
            };
        };


        /** Subscription flags. */
        struct SUBSCRIPTION
        {
            enum : MessagePacket::message_t
            {
                BLOCK           = (1 << 1),
                TRANSACTION     = (1 << 2),
                TIMESEED        = (1 << 3),
                BESTHEIGHT      = (1 << 4),
                CHECKPOINT      = (1 << 5),
                ADDRESS         = (1 << 6),
                LASTINDEX       = (1 << 7),
                BESTCHAIN       = (1 << 8),
                SIGCHAIN        = (1 << 9),
                REGISTER        = (1 << 10),
            };
        };


    private:


        /** State of if this node has logged in to remote node. **/
        std::atomic<bool> fLoggedIn;


        /** State of if remote node has currently verified signature. **/
        std::atomic<bool> fAuthorized;


        /** State to keep track of if node has completed initialization process. **/
        std::atomic<bool> fInitialized;


        /** The current subscriptions. **/
        std::atomic<uint16_t> nSubscriptions;


        /** The current notifications. **/
        std::atomic<uint16_t> nNotifications;


        /** Sig chain genesis hashes / register addresses that the peer has subscribed to notifications for **/
        std::set<uint256_t> setSubscriptions;


        /** Inventory class to track caches. **/
        static Inventory tInventory;


    public:

        /** Mutex for connected sessions. **/
        static std::mutex CLIENT_MUTEX;


        /** Mutex for connected sessions. **/
        static std::mutex SESSIONS_MUTEX;


        /** Set for connected session. **/
        static std::map<uint64_t, std::pair<uint32_t, uint32_t>> mapSessions;


        /** Name
         *
         *  Returns a string for the name of this type of Node.
         *
         **/
        static std::string Name() { return "Tritium"; }


        /** Switch Node
         *
         *  Helper function to switch available nodes.
         *
         **/
        static void SwitchNode();


        /** The block height at the start of the last sync session **/
        static std::atomic<uint32_t> nSyncStart;


        /** The block height at the end of the last sync session **/
        static std::atomic<uint32_t> nSyncStop;


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


        /** This node's address, as seen by the peer **/
        static memory::atomic<LLP::BaseAddress> addrThis;


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
         *  Main message handler once a packet is received.
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
         *  Determine if a node is authorized and therefore trusted.
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
         *  @param[in] fSubscribe Flag to determine whether subscribing or unsubscribing
         *
         **/
        void Subscribe(const uint16_t nFlags, bool fSubscribe = true);


        /** Unsubscribe
         *
         *  Unsubscribe from another node for notifications.
         *
         *  @param[in] hashAddress The address to unsubscribe notifications for.
         *
         **/
        void UnsubscribeAddress(const uint256_t& hashAddress);


        /** Subscribe
         *
         *  Subscribe to another node for notifications.
         *
         *  @param[in] hashAddress The address to unsubscribe notifications for..
         *  @param[in] fSubscribe Flag to determine whether subscribing or unsubscribing
         *
         **/
        void SubscribeAddress(const uint256_t& hashAddress, bool fSubscribe = true);


        /** RelayFilter
         *
         *  Checks if a node is subscribed to receive a notification.
         *
         *  @return a data stream with relevant relay information
         *
         **/
        const DataStream RelayFilter(const uint16_t nMsg, const DataStream& ssData) const;


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


        /** Syncing
         *
         *  Determine whether a node is syncing.
         *
         *  @return true if session is connected.
         *
         **/
        static bool Syncing();


        /** GetNode
         *
         *  Get a node by connected session.
         *
         *  @param[in] nSession The session to receive
         *
         *  @return a pointer to connected node.
         *
         **/
        static std::shared_ptr<TritiumNode> GetNode(const uint64_t nSession);


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
            ((ssData << args), ...);

            WritePacket(NewMessage(nMsg, ssData));

            debug::log(4, NODE, "sent message ", std::hex, nMsg, " of ", std::dec, ssData.size(), " bytes");
        }


        /** BlockingMessage
         *
         *  Adds a tritium packet to the queue and waits for the peer to send a COMPLETED message.
         *
         *  @param[in] pNode Pointer to the TritiumNode connection instance to push the message to.
         *  @param[in] nMsg The message type.
         *  @param[in] args variable args to be sent in the message.
         **/
        template<typename... Args>
        static void BlockingMessage(const uint32_t nTimeout, LLP::TritiumNode* pNode, const uint16_t nMsg, Args&&... args)
        {
            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Create our trigger nonce. */
            uint64_t nNonce = LLC::GetRand();
            pNode->PushMessage(LLP::TritiumNode::TYPES::TRIGGER, nNonce);

            /* Request the inventory message. */
            pNode->PushMessage(nMsg, std::forward<Args>(args)...);

            /* Create the condition variable trigger. */
            LLP::Trigger REQUEST_TRIGGER;
            pNode->AddTrigger(LLP::TritiumNode::RESPONSE::COMPLETED, &REQUEST_TRIGGER);

            /* Process the event. */
            REQUEST_TRIGGER.wait_for_timeout(nNonce, nTimeout);

            /* Cleanup our event trigger. */
            pNode->Release(LLP::TritiumNode::RESPONSE::COMPLETED);
        }


        /** BlockingMessage
         *
         *  Adds a tritium packet to the queue and waits for the peer to send a COMPLETED message.
         *
         *  @param[in] pNode Pointer to the TritiumNode connection instance to push the message to.
         *  @param[in] nMsg The message type.
         *  @param[in] args variable args to be sent in the message.
         **/
        template<typename... Args>
        static void BlockingMessage(LLP::TritiumNode* pNode, const uint16_t nMsg, Args&&... args)
        {
            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Create our trigger nonce. */
            uint64_t nNonce = LLC::GetRand();
            pNode->PushMessage(LLP::TritiumNode::TYPES::TRIGGER, nNonce);

            /* Request the inventory message. */
            pNode->PushMessage(nMsg, std::forward<Args>(args)...);

            /* Create the condition variable trigger. */
            LLP::Trigger REQUEST_TRIGGER;
            pNode->AddTrigger(LLP::TritiumNode::RESPONSE::COMPLETED, &REQUEST_TRIGGER);

            /* Process the event. */
            REQUEST_TRIGGER.wait_for_nonce(nNonce);

            /* Cleanup our event trigger. */
            pNode->Release(LLP::TritiumNode::RESPONSE::COMPLETED);
        }


        /** Sync
         *
         *  Initiates a chain synchronization from the peer.
         *
         **/
        void Sync();

    };
} // end namespace LLP

#endif
