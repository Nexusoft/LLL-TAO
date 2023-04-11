/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_key.h>

#include <LLP/types/tritium.h>
#include <LLP/include/global.h>
#include <LLP/include/manager.h>
#include <LLP/templates/events.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/build.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/process.h>

#include <TAO/Ledger/types/client.h>
#include <TAO/Ledger/types/locator.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/syncblock.h>
#include <TAO/Ledger/types/credentials.h>

#ifndef NO_WALLET
#include <Legacy/wallet/wallet.h>
#else
#include <Legacy/types/merkle.h>
#endif

#include <Legacy/include/evaluate.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/version.h>


#include <climits>
#include <memory>
#include <iomanip>
#include <bitset>

namespace LLP
{
    /* Inventory class to track recent relays with expiring cache. */
    TritiumNode::Inventory TritiumNode::tInventory;


    /* Declaration of client mutex for synchronizing client mode transactions. */
    std::mutex TritiumNode::CLIENT_MUTEX;


    /* Declaration of sessions mutex. (private). */
    std::mutex TritiumNode::SESSIONS_MUTEX;


    /* Declaration of sessions sets. (private). */
    std::map<uint64_t, std::pair<uint32_t, uint32_t>> TritiumNode::mapSessions;


    /* Declaration of block height at the start of last sync. */
    std::atomic<uint32_t> TritiumNode::nSyncStart(0);


    /* Declaration of block height at the end of last sync. */
    std::atomic<uint32_t> TritiumNode::nSyncStop(0);


    /* Declaration of timer to track sync time */
    runtime::timer TritiumNode::SYNCTIMER;


    /* If node is completely sychronized. */
    std::atomic<bool> TritiumNode::fSynchronized(false);


    /* Last block that was processed. */
    std::atomic<uint64_t> TritiumNode::nLastTimeReceived(0);


    /* Remaining time left to finish syncing. */
    std::atomic<uint64_t> TritiumNode::nRemainingTime(0);


    /** This node's address, as seen by the peer **/
    memory::atomic<LLP::BaseAddress> TritiumNode::addrThis;


    /** Default Constructor **/
    TritiumNode::TritiumNode()
    : BaseConnection<MessagePacket>()
    , fLoggedIn(false)
    , fAuthorized(false)
    , fInitialized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , setSubscriptions()
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , hashBestChain(0)
    , hashLastIndex(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , nUnsubscribed(0)
    , nTriggerNonce(0)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket>(SOCKET_IN, DDOS_IN, fDDOSIn)
    , fLoggedIn(false)
    , fAuthorized(false)
    , fInitialized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , setSubscriptions()
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , hashBestChain(0)
    , hashLastIndex(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , nUnsubscribed(0)
    , nTriggerNonce(0)
    {
    }


    /** Constructor **/
    TritiumNode::TritiumNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket>(DDOS_IN, fDDOSIn)
    , fLoggedIn(false)
    , fAuthorized(false)
    , fInitialized(false)
    , nSubscriptions(0)
    , nNotifications(0)
    , setSubscriptions()
    , nLastPing(0)
    , nLastSamples(0)
    , mapLatencyTracker()
    , hashGenesis(0)
    , nTrust(0)
    , nProtocolVersion(0)
    , nCurrentSession(0)
    , nCurrentHeight(0)
    , hashCheckpoint(0)
    , hashBestChain(0)
    , hashLastIndex(0)
    , nConsecutiveOrphans(0)
    , nConsecutiveFails(0)
    , strFullVersion()
    , nUnsubscribed(0)
    , nTriggerNonce(0)
    {
    }


    /** Default Destructor **/
    TritiumNode::~TritiumNode()
    {
    }


    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void TritiumNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            /* Once a new connection is established, this event is fired off. */
            case EVENTS::CONNECT:
            {
                /* Check for incoming connections for -client mode. */
                if(config::fClient.load() && !fOUTGOING)
                {
                    debug::drop(NODE, "Incoming connections disabled in -client mode");
                    break;
                }

                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming", " Connection Established");

                /* Set the laset ping time. */
                nLastPing    = runtime::unifiedtimestamp();

                /* Respond with version message if incoming connection. */
                if(fOUTGOING)
                {
                    /* If we are on version 3.1, we want to send their address on connect. */
                    if(MinorVersion(LLP::PROTOCOL_VERSION, 3) >= 1) //3 is major version, 1 is minor (3.1)
                    {
                        /* Respond with version message. */
                        PushMessage(
                            ACTION::VERSION,
                            PROTOCOL_VERSION,
                            SESSION_ID,
                            version::CLIENT_VERSION_BUILD_STRING,
                            BaseAddress(GetAddress())
                        );
                    }
                    else
                    {
                        /* Respond with version message. */
                        PushMessage(
                            ACTION::VERSION,
                            PROTOCOL_VERSION,
                            SESSION_ID,
                            version::CLIENT_VERSION_BUILD_STRING
                        );
                    }
                }


                break;
            }

            /* Once the packet header is completed, this event is fired off. */
            case EVENTS::HEADER:
            {
                /* Check for initialization. */
                if(nCurrentSession == 0 && nProtocolVersion == 0 && INCOMING.MESSAGE != ACTION::VERSION && DDOS)
                    DDOS->rSCORE += 25;

                break;
            }

            /* Once a chuck of data is read into the packet, this event is fired off. */
            case EVENTS::PACKET:
            {
                /* Check a packet's validity once it is finished being read. */
                if(Incoming())
                {
                    /* Give higher score for Bad Packets. */
                    if(INCOMING.Complete() && !INCOMING.IsValid() && DDOS)
                        DDOS->rSCORE += 15;
                }

                /* Dump packet hex if it has completed reading. */
                if(INCOMING.Complete())
                {
                    if(config::nVerbose >= 5)
                        PrintHex(INCOMING.GetBytes());
                }

                break;
            }


            /* Processed event is used for events triggers. */
            case EVENTS::PROCESSED:
            {
                break;
            }


            case EVENTS::GENERIC:
            {
                /* Make sure node responded on unsubscriion within 30 seconds. */
                if(nUnsubscribed != 0 && nUnsubscribed + 30 < runtime::timestamp())
                {
                    /* Debug output. */
                    debug::drop(NODE, "failed to receive unsubscription within 30 seconds");

                    /* Disconnect this node. */
                    Disconnect();

                    return;
                }

                /* Handle sending the pings to remote node.. */
                if(nLastPing + 15 < runtime::unifiedtimestamp())
                {
                    /* Create a random nonce. */
                    uint64_t nNonce = LLC::GetRand();
                    nLastPing = runtime::unifiedtimestamp();

                    /* Keep track of latency for this ping. */
                    mapLatencyTracker.insert(std::pair<uint64_t, runtime::timer>(nNonce, runtime::timer()));
                    mapLatencyTracker[nNonce].Start();

                    /* Push new message. */
                    PushMessage(ACTION::PING, nNonce);

                    /* Rebroadcast transactions. */
                    if(!TAO::Ledger::ChainState::Synchronizing())
                    {
                        #ifndef NO_WALLET
                        Legacy::Wallet::Instance().ResendWalletTransactions();
                        #endif
                    }
                }

                /* Handle subscribing to events from other nodes. */
                if(!fInitialized.load() && fSynchronized.load() && nCurrentSession != 0)
                {
                    /* Simple log to let us know we are making the subscription requests. */
                    debug::log(1, NODE, "Initializing Subscriptions for ", std::hex, nCurrentSession);

                    /* Grab list of memory pool transactions. */
                    if(!config::fClient.load())
                    {
                        /* Subscribe to notifications. */
                        Subscribe(
                               SUBSCRIPTION::BESTCHAIN
                             | SUBSCRIPTION::BESTHEIGHT
                             | SUBSCRIPTION::CHECKPOINT
                             | SUBSCRIPTION::BLOCK
                             | SUBSCRIPTION::TRANSACTION
                        );

                        PushMessage(ACTION::LIST, uint8_t(TYPES::MEMPOOL));
                    }
                    else
                    {
                        /* Subscribe to notifications. */
                        Subscribe(
                               SUBSCRIPTION::BESTCHAIN
                             | SUBSCRIPTION::BESTHEIGHT
                             | SUBSCRIPTION::BLOCK
                        );
                    }

                    /* Set node as initialized. */
                    fInitialized.store(true);
                }


                /* Disable AUTH for older protocol versions. */
                if(nProtocolVersion >= MIN_TRITIUM_VERSION && config::fClient.load() && !fLoggedIn.load() && fOUTGOING)
                {
                    /* Generate an AUTH message to send to all peers */
                    DataStream ssMessage = LLP::TritiumNode::GetAuth(true);
                    if(ssMessage.size() > 0)
                    {
                        /* Authorize before we subscribe. */
                        WritePacket(NewMessage(ACTION::AUTH, ssMessage));
                        fLoggedIn.store(true);
                    }
                }


                /* Unreliabilitiy re-requesting (max time since getblocks) */
                if(TAO::Ledger::ChainState::Synchronizing()
                && nCurrentSession == TAO::Ledger::nSyncSession.load()
                && nCurrentSession != 0
                && nLastTimeReceived.load() + 30 < runtime::timestamp())
                {
                    debug::log(0, NODE, "Sync Node Timeout");

                    /* Switch to a new node. */
                    SwitchNode();

                    /* Reset the event timeouts. */
                    nLastTimeReceived.store(runtime::timestamp());
                }

                break;
            }

            /* Once a connection is terminated, this event will be fired off. */
            case EVENTS::DISCONNECT:
            {
                /* Track whether to mark as failure or dropped. */
                uint8_t nState = ConnectState::DROPPED;

                /* Debut output. */
                std::string strReason;
                switch(LENGTH)
                {
                    /* Casual timeout event. */
                    case DISCONNECT::TIMEOUT:
                        strReason = "Timeout";
                        nState    = ConnectState::DROPPED;
                        break;

                    /* Generic errors catch all. */
                    case DISCONNECT::ERRORS:
                        strReason = "Errors";
                        nState    = ConnectState::FAILED;
                        break;

                    /* Socket related for POLLERR. */
                    case DISCONNECT::POLL_ERROR:
                        strReason = "Poll Error";
                        nState    = ConnectState::FAILED;
                        break;

                    /* Special condition for linux where there's presumed data that can't be read, causing large CPU usage. */
                    case DISCONNECT::POLL_EMPTY:
                        strReason = "Unavailable";
                        nState    = ConnectState::FAILED;
                        break;

                    /* Distributed Denial Of Service score threshold. */
                    case DISCONNECT::DDOS:
                        strReason = "DDOS";
                        break;

                    /* Forced disconnects come from returning false in ProcessPacket(). */
                    case DISCONNECT::FORCE:
                        strReason = "Force";
                        nState    = ConnectState::DROPPED;
                        break;

                    /* Graceful disconnect from peer's socket. */
                    case DISCONNECT::PEER:
                        strReason = "Peer Hangup";
                        nState    = ConnectState::DROPPED;
                        break;

                    /* Buffer flood control, triggered when MAX_SEND_BUFFER is exceeded. */
                    case DISCONNECT::BUFFER:
                        strReason = "Flood Control";
                        nState    = ConnectState::DROPPED;
                        break;

                    /* Buffer Flood control, triggered when buffer is full but remote node isn't reading the data. */
                    case DISCONNECT::TIMEOUT_WRITE:
                        strReason = "Flood Control Timeout";
                        nState    = ConnectState::DROPPED;
                        break;

                    /* Default catch-all errors. */
                    default:
                        strReason = "Unknown";
                        nState    = ConnectState::DROPPED;
                        break;
                }

                /* Debug output for node disconnect. */
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                    " Disconnected (", strReason, ")");

                /* Update address status. */
                const BaseAddress& addr = GetAddress();
                if(TRITIUM_SERVER->GetAddressManager() && TRITIUM_SERVER->GetAddressManager()->Has(addr))
                    TRITIUM_SERVER->GetAddressManager()->AddAddress(addr, nState);

                /* Check if we need to switch sync nodes. */
                if(nCurrentSession != 0)
                {
                    /* Handle if sync node is disconnected and this is not a duplicate connection. */
                    if(nCurrentSession == TAO::Ledger::nSyncSession.load())
                    {
                        /* Debug output for node disconnect. */
                        debug::log(0, NODE, "Sync Node Disconnected (", strReason, ")");

                        SwitchNode();
                    }


                    LOCK(SESSIONS_MUTEX);

                    /* Free the session as long as it is not a duplicate connection that we are closing. */
                    if(mapSessions.count(nCurrentSession))
                        mapSessions.erase(nCurrentSession);

                    /* Reset session value. */
                    nCurrentSession = 0;
                }

                /* Reset session, notifications, subscriptions etc */
                nUnsubscribed   = 0;
                nNotifications  = 0;

                break;
            }
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool TritiumNode::ProcessPacket()
    {
        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, PROTOCOL_VERSION);
        switch(INCOMING.MESSAGE)
        {
            /* Handle for the version command. */
            case ACTION::VERSION:
            {
                /* Check for duplicate version messages. */
                if(nCurrentSession != 0)
                    return debug::drop(NODE, "duplicate version message");

                /* Hard requirement for version. */
                ssPacket >> nProtocolVersion;

                /* Check versions. */
                if(nProtocolVersion < MIN_PROTO_VERSION)
                    return debug::drop(NODE, "connection using obsolete protocol version");

                /* Client mode only wants connections to correct version. */
                if(config::fClient.load() && nProtocolVersion < MIN_TRITIUM_CLIENT_VERSION)
                {
                    /* Remove address from address manager. */
                    if(TRITIUM_SERVER->GetAddressManager())
                        TRITIUM_SERVER->GetAddressManager()->RemoveAddress(GetAddress());

                    return debug::drop(NODE, "-client mode requires version ", MIN_TRITIUM_CLIENT_VERSION);
                }

                /* Get the current session-id. */
                uint64_t nSessionCheck = 0;
                ssPacket >> nSessionCheck;

                /* Check for invalid session-id. */
                if(nSessionCheck == 0)
                    return debug::drop(NODE, "invalid session-id");

                /* Check for a connect to self. */
                if(nSessionCheck == SESSION_ID)
                {
                    /* Cache self-address in the banned list of the Address Manager. */
                    if(TRITIUM_SERVER->GetAddressManager())
                        TRITIUM_SERVER->GetAddressManager()->Ban(GetAddress());

                    return debug::drop(NODE, "connected to self");
                }

                /* Check if session is already connected. */
                {
                    LOCK(SESSIONS_MUTEX);
                    if(mapSessions.count(nSessionCheck))
                        return debug::drop(NODE, "duplicate connection");

                    /* Set this to the current session. */
                    mapSessions[nSessionCheck] = std::make_pair(nDataThread, nDataIndex);
                }

                /* Set our nCurrentSession after duplicate checks so we don't get false positives to switch sync nodes. */
                nCurrentSession = nSessionCheck;

                /* Get the version string. */
                ssPacket >> strFullVersion;

                /* Handle augmenting Tritium Protocol with new minor version 3.1. */
                if(MinorVersion(nProtocolVersion, 3) >= 1) //NOTE: 3 is for major version 3, we want to check against minor (0.1)
                {
                    /* Deserialize the address. */
                    BaseAddress addr;
                    ssPacket >> addr;

                    /* Set our internal address value if possible. */
                    BaseAddress addrCurrent = addrThis.load();
                    if(!addrCurrent.IsValid())
                    {
                        addrThis.store(addr);
                        debug::log(0, NODE, "ACTION::VERSION set current address as ", addr.ToStringIP());
                    }
                    else if(addrCurrent != addr)
                        debug::warning(NODE, "ACTION::VERSION conflicting address ",
                            addr.ToStringIP(), " != ", addrCurrent.ToStringIP()); //TODO: vAddrThis: can have multiple addr (IPv4/6, LTE, Wifi, etc.)

                    /* Respond with version message if incoming connection. */
                    if(Incoming())
                    {
                        /* Respond with version message. */
                        PushMessage(
                            ACTION::VERSION,
                            PROTOCOL_VERSION,
                            SESSION_ID,
                            version::CLIENT_VERSION_BUILD_STRING,
                            BaseAddress(GetAddress())
                        );
                    }

                    /* Special check for -client modes. */
                    else if(config::fClient.load() && LLP::LOOKUP_SERVER)
                    {
                        /* Get a copy of our current address. */
                        const std::string strAddress =
                            GetAddress().ToStringIP();

                        /* Check that this node has a valid lookup server active. */
                        std::shared_ptr<LLP::LookupNode> pConnection;
                        if(!LLP::LOOKUP_SERVER->ConnectNode(strAddress, pConnection))
                        {
                            /* Delete this from manager. */
                            if(LLP::TRITIUM_SERVER->GetAddressManager())
                                LLP::TRITIUM_SERVER->GetAddressManager()->Ban(GetAddress());

                            return false;
                        }
                    }
                }
                else
                {
                    /* Respond with version message if incoming connection. */
                    if(Incoming())
                    {
                        /* Respond with version message. */
                        PushMessage(
                            ACTION::VERSION,
                            PROTOCOL_VERSION,
                            SESSION_ID,
                            version::CLIENT_VERSION_BUILD_STRING
                        );
                    }
                }


                /* Add to address manager. */
                if(TRITIUM_SERVER->GetAddressManager())
                    TRITIUM_SERVER->GetAddressManager()->AddAddress(GetAddress(), ConnectState::CONNECTED);

                #ifdef DEBUG_MISSING
                fSynchronized.store(true);
                #endif

                /* If not synchronized and making an outbound connection, start the sync */
                if(!fSynchronized.load())
                {
                    /* See if this is a local testnet, in which case we will allow a sync on incoming or outgoing */
                    const bool fLocalTestnet =
                        (config::fTestNet.load() && !config::GetBoolArg("-dns", true));

                    /* Check if we need to override sync process. */
                    if(!config::GetBoolArg("-sync", true) || fLocalTestnet) //hard value to rely on if needed
                        fSynchronized.store(true);

                    /* Start sync on startup, or override any legacy syncing currently in process. */
                    else if(TAO::Ledger::nSyncSession.load() == 0 && !Incoming())
                    {
                        /* Initiate the sync process */
                        Sync();
                    }
                }

                /* Relay to subscribed nodes a new connection was seen. */
                TRITIUM_SERVER->Relay
                (
                    ACTION::NOTIFY,
                    uint8_t(TYPES::ADDRESS),
                    BaseAddress(GetAddress())
                );

                /* Subscribe to address notifications only. */
                Subscribe(SUBSCRIPTION::ADDRESS);

                break;
            }


            /* Handle for auth / deauthcommand. */
            case ACTION::AUTH:
            case ACTION::DEAUTH:
            {
                /* Disable AUTH for older protocol versions. */
                if(nProtocolVersion < MIN_TRITIUM_VERSION)
                    return true;

                /* Check for client mode since this method should never be called except by a client. */
                if(config::fClient.load())
                    return debug::drop(NODE, "ACTION::AUTH disabled in -client mode");

                /* Disable AUTH messages when synchronizing. */
                if(TAO::Ledger::ChainState::Synchronizing())
                    return true;

                /* Hard requirement for genesis. */
                ssPacket >> hashGenesis;

                /* Get the signature information. */
                if(hashGenesis == 0)
                    return debug::drop(NODE, "ACTION::AUTH: cannot authorize with reserved genesis");

                /* Get the timestamp */
                uint64_t nTimestamp;
                ssPacket >> nTimestamp;

                /* Track our difference as a signed int for (+) values indicating older message. */
                if(nTimestamp > runtime::unifiedtimestamp() || nTimestamp < runtime::unifiedtimestamp() - 10)
                    return debug::drop(NODE, "ACTION::AUTH: message is stale by ", int64_t(runtime::unifiedtimestamp() - nTimestamp), " seconds");

                /* Get the nonce */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                /* Check the nNonce for expected values. */
                if(nNonce != nCurrentSession)
                    return debug::drop(NODE, "ACTION::AUTH: invalid session-id ", nNonce);

                /* Get the public key. */
                //std::vector<uint8_t> vchPubKey;
                //ssPacket >> vchPubKey;

                /* Build the byte stream from genesis+nonce in order to verify the signature */
                DataStream ssCheck(SER_NETWORK, PROTOCOL_VERSION);
                ssCheck << hashGenesis << nTimestamp << nNonce;

                /* Get a hash of the data. */
                //uint256_t hashCheck = LLC::SK256(ssCheck.begin(), ssCheck.end());

                /* Get the signature. */
                //std::vector<uint8_t> vchSig;
                //ssPacket >> vchSig;

                /* Verify the signature */
                //if(!TAO::Ledger::Credentials::Verify(hashGenesis, "network", hashCheck.GetBytes(), vchPubKey, vchSig))
                //    return debug::drop(NODE, "ACTION::AUTH: invalid transaction signature");

                /* Set to authorized node if passed all cryptographic checks. */
                if(INCOMING.MESSAGE == ACTION::AUTH)
                {
                    /* Get the trust object register. */
                    //TAO::Register::Object trust;
                    //if(!LLD::Register->ReadState(TAO::Register::Address(std::string("trust"),
                    //    hashGenesis, TAO::Register::Address::TRUST), trust, TAO::Ledger::FLAGS::MEMPOOL))
                    //    return debug::drop(NODE, "ACTION::AUTH: authorization failed, missing trust register");

                    /* Parse the object. */
                    //if(!trust.Parse())
                    //    return debug::drop(NODE, "ACTION::AUTH: failed to parse trust register");

                    /* Set as authorized and respond with acknowledgement. */
                    fAuthorized = true;
                    PushMessage(RESPONSE::AUTHORIZED, hashGenesis);

                    /* Set the node's current trust score. */
                    //nTrust = trust.get<uint64_t>("trust");

                    debug::log(0, NODE, "ACTION::AUTH: ", hashGenesis.SubString(), " AUTHORIZATION ACCEPTED");
                }
                else if(INCOMING.MESSAGE == ACTION::DEAUTH)
                {
                    /* Set deauthorization flag. */
                    fAuthorized = false;

                    /* Reset sigchain specific values. */
                    hashGenesis = 0;
                    nTrust = 0;

                    debug::log(0, NODE, "ACTION::DEAUTH: ", hashGenesis.SubString(), " DE-AUTHORIZATION ACCEPTED");
                }

                break;
            }


            /* Positive AUTH response. */
            case RESPONSE::AUTHORIZED:
            {
                /* Grab the genesis. */
                uint256_t hashSigchain;
                ssPacket >> hashSigchain;

                /* Subscribe to sigchain events now. */
                Subscribe(SUBSCRIPTION::SIGCHAIN);

                debug::log(0, NODE, "RESPONSE::AUTHORIZED: ", hashSigchain.SubString(), " AUTHORIZATION ACCEPTED");

                break;
            }


            /* Handle for the subscribe command. */
            case ACTION::SUBSCRIBE:
            case ACTION::UNSUBSCRIBE:
            case RESPONSE::UNSUBSCRIBED:
            {
                /* Let node know it unsubscribed successfully. */
                if(INCOMING.MESSAGE == RESPONSE::UNSUBSCRIBED)
                {
                    /* Check for unsoliced messages. */
                    if(nUnsubscribed == 0)
                        return debug::drop(NODE, "unsolicted RESPONSE::UNSUBSCRIBE");

                    /* Reset the timer. */
                    nUnsubscribed = 0;
                }

                /* Set our max items we can get to 100 per packet. */
                uint32_t nLimits = 0;
                while(!ssPacket.End())
                {
                    /* Check our internal limits now. */
                    if(++nLimits > ACTION::SUBSCRIBE_MAX_ITEMS)
                        return debug::drop(NODE, "ACTION::SUBSCRIBE_MAX_ITEMS reached ", VARIABLE(nLimits));

                    /* Read the type. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Switch based on type. */
                    switch(nType)
                    {
                        /* Subscribe to getting blocks. */
                        case TYPES::BLOCK:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the block flag. */
                                nNotifications |= SUBSCRIPTION::BLOCK;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE: BLOCK: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the block flag. */
                                nNotifications &= ~SUBSCRIPTION::BLOCK;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::BLOCK: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the block flag. */
                                nSubscriptions &= ~SUBSCRIPTION::BLOCK;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::BLOCK: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting transactions. */
                        case TYPES::TRANSACTION:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the block flag. */
                                nNotifications |= SUBSCRIPTION::TRANSACTION;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::TRANSACTION: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the transaction flag. */
                                nNotifications &= ~SUBSCRIPTION::TRANSACTION;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::TRANSACTION: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the transaction flag. */
                                nSubscriptions &= ~SUBSCRIPTION::TRANSACTION;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::TRANSACTION: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting best height. */
                        case TYPES::BESTHEIGHT:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the best height flag. */
                                nNotifications |= SUBSCRIPTION::BESTHEIGHT;

                                /* Notify node of current block height. */
                                PushMessage(ACTION::NOTIFY,
                                    uint8_t(TYPES::BESTHEIGHT), TAO::Ledger::ChainState::nBestHeight.load());

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::BESTHEIGHT: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the height flag. */
                                nNotifications &= ~SUBSCRIPTION::BESTHEIGHT;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::BESTHEIGHT: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the height flag. */
                                nSubscriptions &= ~SUBSCRIPTION::BESTHEIGHT;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::BESTHEIGHT: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting checkpoints. */
                        case TYPES::CHECKPOINT:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the checkpoints flag. */
                                nNotifications |= SUBSCRIPTION::CHECKPOINT;

                                /* Notify node of current block height. */
                                PushMessage(ACTION::NOTIFY,
                                    uint8_t(TYPES::CHECKPOINT), TAO::Ledger::ChainState::hashCheckpoint.load());

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::CHECKPOINT: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the checkpoints flag. */
                                nNotifications &= ~SUBSCRIPTION::CHECKPOINT;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::CHECKPOINT: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the checkpoints flag. */
                                nSubscriptions &= ~SUBSCRIPTION::CHECKPOINT;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::CHECKPOINT: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting addresses. */
                        case TYPES::ADDRESS:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the address flag. */
                                nNotifications |= SUBSCRIPTION::ADDRESS;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::ADDRESS: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the address flag. */
                                nNotifications &= ~SUBSCRIPTION::ADDRESS;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::ADDRESS: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the address flag. */
                                nSubscriptions &= ~SUBSCRIPTION::ADDRESS;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::ADDRESS: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting last index on list commands. */
                        case TYPES::LASTINDEX:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::SUBSCRIBE::LASTINDEX disabled in -client mode");

                                /* Set the last flag. */
                                nNotifications |= SUBSCRIPTION::LASTINDEX;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::LAST: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::UNSUBSCRIBE::LASTINDEX disabled in -client mode");

                                /* Unset the last flag. */
                                nNotifications &= ~SUBSCRIPTION::LASTINDEX;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::LAST: ", std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the last flag. */
                                nSubscriptions &= ~SUBSCRIPTION::LASTINDEX;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::LASTINDEX: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting transactions. */
                        case TYPES::BESTCHAIN:
                        {
                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Set the best chain flag. */
                                nNotifications |= SUBSCRIPTION::BESTCHAIN;

                                /* Notify node of current block height. */
                                PushMessage(ACTION::NOTIFY,
                                    uint8_t(TYPES::BESTCHAIN), TAO::Ledger::ChainState::hashBestChain.load());

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::BESTCHAIN: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Unset the bestchain flag. */
                                nNotifications &= ~SUBSCRIPTION::BESTCHAIN;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::BESTCHAIN: " , std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the bestchain flag. */
                                nSubscriptions &= ~SUBSCRIPTION::BESTCHAIN;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::BESTCHAIN: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }


                        /* Subscribe to getting transactions. */
                        case TYPES::SIGCHAIN:
                        {
                            /* Check for available protocol version. */
                            if(nProtocolVersion < MIN_TRITIUM_VERSION)
                                return true;

                            /* Check that node is logged in. */
                            if(!fAuthorized || hashGenesis == 0)
                                return debug::drop(NODE, "ACTION::SUBSCRIBE::SIGCHAIN: Access Denied");

                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::SUBSCRIBE::SIGCHAIN disabled in -client mode");

                                /* Set the best chain flag. */
                                nNotifications |= SUBSCRIPTION::SIGCHAIN;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::SIGCHAIN: ", std::bitset<16>(nNotifications));
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::UNSUBSCRIBE::SIGCHAIN disabled in -client mode");

                                /* Unset the bestchain flag. */
                                nNotifications &= ~SUBSCRIPTION::SIGCHAIN;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::SIGCHAIN: " , std::bitset<16>(nNotifications));
                            }
                            else
                            {
                                /* Unset the bestchain flag. */
                                nSubscriptions &= ~SUBSCRIPTION::SIGCHAIN;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::SIGCHAIN: ", std::bitset<16>(nSubscriptions));
                            }

                            break;
                        }

                        /* Subscribe to getting event transcations. */
                        case TYPES::REGISTER:
                        {
                            /* Check for available protocol version. */
                            if(nProtocolVersion < MIN_TRITIUM_VERSION)
                                return true;

                            /* Check that node is logged in. */
                            if(!fAuthorized || hashGenesis == 0)
                                return debug::drop(NODE, "ACTION::SUBSCRIBE::REGISTER: Access Denied");

                            /* Deserialize the address for the event subscription */
                            uint256_t hashAddress;
                            ssPacket >> hashAddress;

                            /* Subscribe. */
                            if(INCOMING.MESSAGE == ACTION::SUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::SUBSCRIBE::REGISTER disabled in -client mode");

                                /* Check that peer hasn't already subscribed to too many addresses, for overflow protection */
                                if(setSubscriptions.size() >= 10000)
                                    return debug::drop(NODE, "ACTION::SUBSCRIBE::REGISTER exceeded max subscriptions");

                                /* Check for our transfer event. */
                                if(!LLD::Ledger->HasEvent(hashAddress, 0))
                                    return debug::drop(NODE, "ACTION::SUBSCRIBE::REGISTER register is not a valid tokenized asset");

                                /* Add the address to the notifications vector for this peer */
                                setSubscriptions.insert(hashAddress);

                                /* Set the subscription flag. */
                                nNotifications |= SUBSCRIPTION::REGISTER;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::SUBSCRIBE::REGISTER: ", hashAddress.ToString());
                            }
                            else if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                            {
                                /* Check for client mode since this method should never be called except by a client. */
                                if(config::fClient.load())
                                    return debug::drop(NODE, "ACTION::UNSUBSCRIBE::REGISTER disabled in -client mode");

                                /* Remove the address from the notifications vector for this peer */
                                setSubscriptions.erase(hashAddress);

                                /* Unset the subscription flag. */
                                nNotifications &= ~SUBSCRIPTION::REGISTER;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::UNSUBSCRIBE::REGISTER: " , hashAddress.ToString());
                            }
                            else
                            {
                                /* Unset the bestchain flag. */
                                nSubscriptions &= ~SUBSCRIPTION::REGISTER;

                                /* Debug output. */
                                debug::log(3, NODE, "RESPONSE::UNSUBSCRIBED::REGISTER: ", hashAddress.ToString());
                            }

                            break;
                        }

                        /* Catch unsupported types. */
                        default:
                        {
                            /* Give score for bad types. */
                            if(fDDOS.load())
                                DDOS->rSCORE += 50;
                        }
                    }
                }

                /* Let node know it unsubscribed successfully. */
                if(INCOMING.MESSAGE == ACTION::UNSUBSCRIBE)
                    WritePacket(NewMessage(RESPONSE::UNSUBSCRIBED, ssPacket));

                break;
            }


            /* Handle for list command. */
            case ACTION::LIST:
            {
                /* Check for client mode since this method should never be called except by a client. */
                if(config::fClient.load())
                    return true; //gracefully ignore these for now since there is no current way for remote nodes to know we are in client mode

                /* Set the limits. 3000 seems to be the optimal amount to overcome higher-latency connections during sync */
                int32_t nLimits = 3000;

                /* Get the next type in stream. */
                uint8_t nType = 0;
                ssPacket >> nType;

                /* Check for legacy or transactions specifiers. */
                bool fLegacy = false, fTransactions = false, fSyncBlock = false, fClientBlock = false;
                if(nType == SPECIFIER::LEGACY || nType == SPECIFIER::TRANSACTIONS
                || nType == SPECIFIER::SYNC   || nType == SPECIFIER::CLIENT)
                {
                    /* Set specifiers. */
                    fLegacy       = (nType == SPECIFIER::LEGACY);
                    fTransactions = (nType == SPECIFIER::TRANSACTIONS);
                    fSyncBlock    = (nType == SPECIFIER::SYNC);
                    fClientBlock  = (nType == SPECIFIER::CLIENT);

                    /* Go to next type in stream. */
                    ssPacket >> nType;
                }

                /* Switch based on codes. */
                switch(nType)
                {
                    /* Standard type for a block. */
                    case TYPES::BLOCK:
                    {
                        /* Get the index of block. */
                        uint1024_t hashStart;

                        /* Get the object type. */
                        uint8_t nObject = 0;
                        ssPacket >> nObject;

                        /* Switch based on object. */
                        switch(nObject)
                        {
                            /* Check for start from uint1024 type. */
                            case TYPES::UINT1024_T:
                            {
                                /* Deserialize start. */
                                ssPacket >> hashStart;

                                break;
                            }

                            /* Check for start from a locator. */
                            case TYPES::LOCATOR:
                            {
                                /* Deserialize locator. */
                                TAO::Ledger::Locator locator;
                                ssPacket >> locator;

                                /* Check locator size. */
                                uint32_t nSize = locator.vHave.size();
                                if(nSize > 30)
                                    return debug::drop(NODE, "locator size ", nSize, " is too large");

                                /* Find common ancestor block. */
                                for(uint32_t n = 0; n < locator.vHave.size(); ++n)
                                {
                                    /* Get a reference of our current locator. */
                                    const uint1024_t& tHave = locator.vHave[n];

                                    /* Check the database for the ancestor block. */
                                    if(LLD::Ledger->HasBlock(tHave))
                                    {
                                        /* Check if locator found genesis. */
                                        if(tHave != TAO::Ledger::ChainState::Genesis())
                                        {
                                            /* Grab the block that's found. */
                                            TAO::Ledger::BlockState state;
                                            if(!LLD::Ledger->ReadBlock(tHave, state))
                                                return debug::drop(NODE, "failed to read locator block");

                                            /* Check for being in main chain. */
                                            if(!state.IsInMainChain())
                                                continue;

                                            hashStart = state.hashPrevBlock;
                                        }
                                        else //on genesis, don't revert to previous block
                                            hashStart = tHave;

                                        break;
                                    }

                                    /* Check if we are at genesis for better debug data. */
                                    else if(n == locator.vHave.size() - 1)
                                        return debug::drop
                                        (
                                            NODE, "ACTION::LIST: Locator: ", tHave.SubString(),
                                            " has different Genesis: ", TAO::Ledger::ChainState::Genesis().SubString()
                                        );
                                }

                                /* Debug output. */
                                if(config::nVerbose >= 3)
                                    debug::log(3, NODE, "ACTION::LIST: Locator ", hashStart.SubString(), " found");

                                break;
                            }

                            default:
                                return debug::drop(NODE, "malformed starting index");
                        }


                        /* Get the ending hash. */
                        uint1024_t hashStop;
                        ssPacket >> hashStop;

                        /* Keep track of the last state. */
                        TAO::Ledger::BlockState stateLast;
                        if(!LLD::Ledger->ReadBlock(hashStart, stateLast))
                            return debug::drop(NODE, "failed to read starting block");

                        /* Do a sequential read to obtain the list.
                           3000 seems to be the optimal amount to overcome higher-latency connections during sync */
                        std::vector<TAO::Ledger::BlockState> vStates;
                        while(!fBufferFull.load() && --nLimits >= 0 && hashStart != hashStop && LLD::Ledger->BatchRead(hashStart, "block", vStates, 3000, true))
                        {
                            /* Loop through all available states. */
                            for(auto& state : vStates)
                            {
                                /* Update start every iteration. */
                                hashStart = state.GetHash();

                                /* Skip if not in main chain. */
                                if(!state.IsInMainChain())
                                    continue;

                                /* Check for matching hashes. */
                                if(state.hashPrevBlock != stateLast.GetHash())
                                {
                                    if(config::nVerbose >= 3)
                                        debug::log(3, FUNCTION, "Reading block ", stateLast.hashNextBlock.SubString());

                                    /* Read the correct block from next index. */
                                    if(!LLD::Ledger->ReadBlock(stateLast.hashNextBlock, state))
                                    {
                                        nLimits = 0;
                                        break;
                                    }

                                    /* Update hashStart. */
                                    hashStart = state.GetHash();
                                }

                                /* Cache the block hash. */
                                stateLast = state;

                                /* Handle for special sync block type specifier. */
                                if(fSyncBlock)
                                {
                                    /* Build the sync block from state. */
                                    TAO::Ledger::SyncBlock block(state);

                                    /* Push message in response. */
                                    PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::SYNC), block);
                                }

                                /* Handle for a client block header. */
                                else if(fClientBlock)
                                {
                                    /* Build the client block from state. */
                                    TAO::Ledger::ClientBlock block(state);

                                    /* Push message in response. */
                                    PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::CLIENT), block);
                                }
                                else
                                {
                                    /* Check for version to send correct type */
                                    if(state.nVersion < 7)
                                    {
                                        /* Build the legacy block from state. */
                                        Legacy::LegacyBlock block(state);

                                        /* Push message in response. */
                                        PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::LEGACY), block);
                                    }
                                    else
                                    {
                                        /* Build the legacy block from state. */
                                        TAO::Ledger::TritiumBlock block(state);

                                        /* Check for transactions. */
                                        if(fTransactions)
                                        {
                                            /* Loop through transactions. */
                                            for(const auto& proof : block.vtx)
                                            {
                                                /* Basic checks for legacy transactions. */
                                                if(proof.first == TAO::Ledger::TRANSACTION::LEGACY)
                                                {
                                                    /* Check the memory pool. */
                                                    Legacy::Transaction tx;
                                                    if(!LLD::Legacy->ReadTx(proof.second, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                                        continue;

                                                    /* Push message of transaction. */
                                                    PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::LEGACY), tx);
                                                }

                                                /* Basic checks for tritium transactions. */
                                                else if(proof.first == TAO::Ledger::TRANSACTION::TRITIUM)
                                                {
                                                    /* Check the memory pool. */
                                                    TAO::Ledger::Transaction tx;
                                                    if(!LLD::Ledger->ReadTx(proof.second, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                                        continue;


                                                    /* Push message of transaction. */
                                                    PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::TRITIUM), tx);
                                                }
                                            }
                                        }

                                        /* Push message in response. */
                                        PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::TRITIUM), block);
                                    }
                                }

                                /* Check for stop hash. */
                                if(--nLimits <= 0 || hashStart == hashStop || fBufferFull.load()) //1MB limit
                                {
                                    /* Regular debug for normal limits */
                                    if(config::nVerbose >= 3)
                                    {
                                        /* Special message for full write buffers. */
                                        if(fBufferFull.load())
                                            debug::log(3, FUNCTION, "Buffer is FULL ", Buffered(), " bytes");

                                        debug::log(3, FUNCTION, "Limits ", nLimits, " Reached ", hashStart.SubString(), " == ", hashStop.SubString());
                                    }

                                    break;
                                }
                            }
                        }

                        /* Check for last subscription. */
                        if(nNotifications & SUBSCRIPTION::LASTINDEX)
                            PushMessage(ACTION::NOTIFY, uint8_t(TYPES::LASTINDEX), uint8_t(TYPES::BLOCK), fBufferFull.load() ? stateLast.hashPrevBlock : hashStart);

                        break;
                    }


                    /* Standard type for a block. */
                    case TYPES::MEMPOOL:
                    {
                        /* Get a list of transactions from mempool. */
                        std::vector<uint512_t> vHashes;

                        /* List tritium transactions if legacy isn't specified. */
                        if(!fLegacy)
                        {
                            if(TAO::Ledger::mempool.List(vHashes, std::numeric_limits<uint32_t>::max(), false))
                            {
                                /* Loop through the available hashes. */
                                for(const auto& hash : vHashes)
                                {
                                    /* Get the transaction from memory pool. */
                                    TAO::Ledger::Transaction tx;
                                    if(!TAO::Ledger::mempool.Get(hash, tx))
                                        break; //we don't want to add more dependants if this fails

                                    /* Push the transaction. */
                                    PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::TRITIUM), tx);
                                }
                            }
                        }

                        /* Get a list of legacy transactions from pool. */
                        vHashes.clear();
                        if(TAO::Ledger::mempool.List(vHashes, std::numeric_limits<uint32_t>::max(), true))
                        {
                            /* Loop through the available hashes. */
                            for(const auto& hash : vHashes)
                            {
                                /* Get the transaction from memory pool. */
                                Legacy::Transaction tx;
                                if(!TAO::Ledger::mempool.Get(hash, tx))
                                    break; //we don't want to add more dependants if this fails

                                /* Push the transaction. */
                                PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::LEGACY), tx);
                            }
                        }

                        break;
                    }


                    /* Standard type for a sigchain listing. */
                    case TYPES::SIGCHAIN:
                    {
                        /* Check for available protocol version. */
                        if(nProtocolVersion < MIN_TRITIUM_VERSION)
                            return true;

                        /* Get the sigchain-id. */
                        uint256_t hashSigchain;
                        ssPacket >> hashSigchain;

                        /* Get the txid to list from */
                        uint512_t hashStart;
                        ssPacket >> hashStart;

                        /* Get the last event */
                        int32_t nLimits = ACTION::LIST_NOTIFICATIONS_MAX_ITEMS + 1;
                        debug::log(1, NODE, "ACTION::LIST: SIGCHAIN for ", hashSigchain.SubString());

                        /* Check for empty hash stop. */
                        uint512_t hashThis;
                        if(!LLD::Ledger->ReadLast(hashSigchain, hashThis, TAO::Ledger::FLAGS::MEMPOOL))
                            break;

                        /* Read sigchain entries. */
                        std::vector<uint512_t> vHashes;
                        while(!config::fShutdown.load())
                        {
                            /* Read from disk. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx(hashThis, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                break;

                            /* Check for genesis. */
                            if(hashStart == hashThis)
                                break;

                            /* Track our list of hashes without filling up our memory. */
                            vHashes.push_back(hashThis);

                            /* Check for genesis. */
                            if(hashStart == hashThis || tx.IsFirst())
                                break;

                            hashThis = tx.hashPrevTx;
                        }

                        /* Reverse container to message forward. */
                        for(auto hash = vHashes.rbegin(); hash != vHashes.rend() && --nLimits > 0; ++hash)
                        {
                            /* Read from disk. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx((*hash), tx, TAO::Ledger::FLAGS::MEMPOOL))
                                break;

                            /* Build a markle transaction. */
                            TAO::Ledger::MerkleTx merkle = TAO::Ledger::MerkleTx(tx);

                            /* Build the merkle branch if the tx has been confirmed (i.e. it is not in the mempool) */
                            if(!TAO::Ledger::mempool.Has(*hash))
                                merkle.BuildMerkleBranch();

                            PushMessage(TYPES::MERKLE, uint8_t(SPECIFIER::TRITIUM), merkle);
                        }

                        break;
                    }


                    /* Standard type for a sigchain listing. */
                    case TYPES::NOTIFICATION:
                    {
                        /* Check for available protocol version. */
                        if(nProtocolVersion < MIN_TRITIUM_VERSION)
                            return true;

                        /* Get the sigchain-id. */
                        uint256_t hashSigchain;
                        ssPacket >> hashSigchain;

                        /* Get the txid to list from */
                        uint32_t nSequence;
                        ssPacket >> nSequence;

                        /* Get the last event */
                        debug::log(1, NODE, "ACTION::LIST: ", fLegacy ? "LEGACY " : "", "NOTIFICATION from ", nSequence, " for ", hashSigchain.SubString());

                        /* Check for legacy. */
                        int32_t nLimits = ACTION::LIST_NOTIFICATIONS_MAX_ITEMS + 1;
                        if(fLegacy)
                        {
                            /* Build our list of events. */
                            std::vector<Legacy::MerkleTx> vtx;

                            /* Look back through all events to find those that are not yet processed. */
                            Legacy::Transaction tx;
                            while(--nLimits > 0 && LLD::Legacy->ReadEvent(hashSigchain, nSequence++, tx))
                            {
                                /* Build a markle transaction. */
                                Legacy::MerkleTx merkle = Legacy::MerkleTx(tx);
                                merkle.BuildMerkleBranch();

                                /* Insert into container. */
                                vtx.push_back(merkle);

                                /* We want to limit our batch size. */
                                if(vtx.size() >= ACTION::LIST_NOTIFICATIONS_MAX_ITEMS)
                                    break;
                            }

                            /* Reverse container to message forward. */
                            for(auto tx = vtx.begin(); tx != vtx.end(); ++tx)
                                PushMessage(TYPES::MERKLE, uint8_t(SPECIFIER::LEGACY), (*tx));
                        }
                        else
                        {
                            /* Build our list of events. */
                            std::vector<TAO::Ledger::MerkleTx> vtx;

                            /* Look back through all events to find those that are not yet processed. */
                            TAO::Ledger::Transaction tx;
                            while(--nLimits > 0 && LLD::Ledger->ReadEvent(hashSigchain, nSequence++, tx))
                            {
                                /* Build a markle transaction. */
                                TAO::Ledger::MerkleTx merkle = TAO::Ledger::MerkleTx(tx);
                                merkle.BuildMerkleBranch();

                                /* Insert into container. */
                                vtx.push_back(merkle);

                                /* We want to limit our batch size. */
                                if(vtx.size() >= ACTION::LIST_NOTIFICATIONS_MAX_ITEMS)
                                    break;
                            }

                            /* Reverse container to message forward. */
                            for(auto tx = vtx.begin(); tx != vtx.end(); ++tx)
                                PushMessage(TYPES::MERKLE, uint8_t(SPECIFIER::DEPENDANT), (*tx));
                        }

                        break;
                    }


                    /* Catch malformed notify binary streams. */
                    default:
                        return debug::drop(NODE, "ACTION::LIST malformed binary stream");
                }

                /* Check for trigger nonce. */
                if(nTriggerNonce != 0)
                {
                    PushMessage(RESPONSE::COMPLETED, nTriggerNonce);
                    nTriggerNonce = 0;
                }

                break;
            }


            /* Handle for get command. */
            case ACTION::GET:
            {
                /* Set our max items we can get to 100 per packet. */
                uint32_t nLimits = 0;
                while(!ssPacket.End())
                {
                    /* We want to drop this connection if exceeding our limits for now. */
                    if(++nLimits > ACTION::GET_MAX_ITEMS)
                        return debug::drop(NODE, "ACTION::GET_MAX_ITEMS reached ", VARIABLE(nLimits));

                    /* Get the next type in stream. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Check for legacy or transactions specifiers. */
                    bool fLegacy = false, fTransactions = false, fClient = false;
                    if(nType == SPECIFIER::LEGACY || nType == SPECIFIER::TRANSACTIONS || nType == SPECIFIER::CLIENT)
                    {
                        /* Set specifiers. */
                        fLegacy       = (nType == SPECIFIER::LEGACY);
                        fTransactions = (nType == SPECIFIER::TRANSACTIONS);
                        fClient       = (nType == SPECIFIER::CLIENT);

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }

                    /* Switch based on codes. */
                    switch(nType)
                    {
                        /* Standard type for a block. */
                        case TYPES::BLOCK:
                        {
                            /* Check for valid specifier. */
                            if(fLegacy)
                                return debug::drop(NODE, "ACTION::GET: invalid specifier for TYPES::BLOCK");

                            /* Check for client mode since this method should never be called except by a client. */
                            if(config::fClient.load())
                                return debug::drop(NODE, "ACTION::GET::BLOCK disabled in -client mode");

                            /* Get the index of block. */
                            uint1024_t hashBlock;
                            ssPacket >> hashBlock;

                            /* Check the database for the block. */
                            TAO::Ledger::BlockState state;
                            if(LLD::Ledger->ReadBlock(hashBlock, state))
                            {
                                /* Push legacy blocks for less than version 7. */
                                if(state.nVersion < 7)
                                {
                                    /* Check for bad client requests. */
                                    if(fClient)
                                        return debug::drop(NODE, "ACTION::GET: CLIENT specifier disabled for legacy blocks");

                                    /* Build legacy block from state. */
                                    Legacy::LegacyBlock block(state);

                                    /* Push block as response. */
                                    PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::LEGACY), block);
                                }
                                else
                                {
                                    /* Handle for client blocks. */
                                    if(fClient)
                                    {
                                        /* Build the client block and send off. */
                                        TAO::Ledger::ClientBlock block(state);

                                        /* Push the new client block. */
                                        PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::CLIENT), block);

                                        /* Debug output. */
                                        debug::log(3, NODE, "ACTION::GET: CLIENT::BLOCK ", hashBlock.SubString());

                                        break;
                                    }

                                    /* Build tritium block from state. */
                                    TAO::Ledger::TritiumBlock block(state);

                                    /* Check for transactions. */
                                    if(fTransactions)
                                    {
                                        /* Loop through transactions. */
                                        for(const auto& proof : block.vtx)
                                        {
                                            /* Basic checks for legacy transactions. */
                                            if(proof.first == TAO::Ledger::TRANSACTION::LEGACY)
                                            {
                                                /* Check the memory pool. */
                                                Legacy::Transaction tx;
                                                if(!LLD::Legacy->ReadTx(proof.second, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                                    continue;

                                                /* Push message of transaction. */
                                                PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::LEGACY), tx);
                                            }

                                            /* Basic checks for tritium transactions. */
                                            else if(proof.first == TAO::Ledger::TRANSACTION::TRITIUM)
                                            {
                                                /* Check the memory pool. */
                                                TAO::Ledger::Transaction tx;
                                                if(!LLD::Ledger->ReadTx(proof.second, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                                    continue;


                                                /* Push message of transaction. */
                                                PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::TRITIUM), tx);
                                            }
                                        }
                                    }

                                    /* Push block as response. */
                                    PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::TRITIUM), block);
                                }
                            }

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::GET: BLOCK ", hashBlock.SubString());

                            break;
                        }

                        /* Standard type for a transaction. */
                        case TYPES::TRANSACTION:
                        {
                            /* Check for valid specifier. */
                            if(fTransactions || fClient)
                                return debug::drop(NODE, "ACTION::GET::TRANSACTION: invalid specifier for TYPES::TRANSACTION");

                            /* Get the index of transaction. */
                            uint512_t hashTx;
                            ssPacket >> hashTx;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check legacy database. */
                                Legacy::Transaction tx;
                                if(LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                    PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::LEGACY), tx);
                            }
                            else
                            {
                                /* Check ledger database. */
                                TAO::Ledger::Transaction tx;
                                if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                {
                                    /* Check if producer is being asked for, and send block instead. */
                                    if(tx.IsCoinBase() || tx.IsCoinStake() || tx.IsHybrid())
                                    {
                                        /* Read block state from disk. */
                                        TAO::Ledger::BlockState state;
                                        if(LLD::Ledger->ReadBlock(hashTx, state))
                                        {
                                            /* Send off tritium block. */
                                            TAO::Ledger::TritiumBlock block(state);
                                            PushMessage(TYPES::BLOCK, uint8_t(SPECIFIER::TRITIUM), block);
                                        }
                                    }
                                    else
                                        PushMessage(TYPES::TRANSACTION, uint8_t(SPECIFIER::TRITIUM), tx);
                                }

                            }

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::GET: TRANSACTION ", hashTx.SubString());

                            break;
                        }


                        /* Standard type for a merkle transaction. */
                        case TYPES::MERKLE:
                        {
                            /* Check for available protocol version. */
                            if(nProtocolVersion < MIN_TRITIUM_VERSION)
                                return true;

                            /* Check for valid specifier. */
                            if(fTransactions || fClient)
                                return debug::drop(NODE, "ACTION::GET::MERKLE: invalid specifier for TYPES::MERKLE");

                            /* Get the index of transaction. */
                            uint512_t hashTx;
                            ssPacket >> hashTx;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check legacy database. */
                                Legacy::Transaction tx;
                                if(LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                {
                                    /* Build a markle transaction. */
                                    Legacy::MerkleTx merkle = Legacy::MerkleTx(tx);

                                    /* Build the merkle branch if the tx has been confirmed (i.e. it is not in the mempool) */
                                    if(LLD::Ledger->HasIndex(hashTx))
                                        merkle.BuildMerkleBranch();

                                    /* Check for dependant specifier. */
                                    PushMessage(TYPES::MERKLE, uint8_t(SPECIFIER::LEGACY), merkle);

                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::GET::LEGACY: MERKLE TRANSACTION ", hashTx.SubString());
                                }
                            }
                            else
                            {
                                /* Check ledger database. */
                                TAO::Ledger::Transaction tx;
                                if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                {
                                    /* Build a markle transaction. */
                                    TAO::Ledger::MerkleTx merkle = TAO::Ledger::MerkleTx(tx);

                                    /* Build the merkle branch if the tx has been confirmed (i.e. it is not in the mempool) */
                                    if(LLD::Ledger->HasIndex(hashTx))
                                        merkle.BuildMerkleBranch();

                                    /* Check for dependant specifier. */
                                    PushMessage(TYPES::MERKLE, uint8_t(SPECIFIER::TRITIUM), merkle);
                                }

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::GET: MERKLE TRANSACTION ", hashTx.SubString());
                            }

                            break;
                        }

                        /* Catch malformed notify binary streams. */
                        default:
                            return debug::drop(NODE, "ACTION::GET malformed binary stream");
                    }
                }

                /* Check for trigger nonce. */
                if(nTriggerNonce != 0)
                {
                    PushMessage(RESPONSE::COMPLETED, nTriggerNonce);
                    nTriggerNonce = 0;
                }

                break;
            }


            /* Handle for notify command. */
            case ACTION::NOTIFY:
            {
                /* Create response data stream. */
                DataStream ssResponse(SER_NETWORK, PROTOCOL_VERSION);

                /* Set our max limits to 100 notifications per packet. */
                uint32_t nLimits = 0;
                while(!ssPacket.End())
                {
                    /* We want to drop this connection if exceeding our limits for now. */
                    if(++nLimits > ACTION::NOTIFY_MAX_ITEMS)
                        return debug::drop(NODE, "ACTION::NOTIFY_MAX_ITEMS reached ", VARIABLE(nLimits));

                    /* Get the next type in stream. */
                    uint8_t nType = 0;
                    ssPacket >> nType;

                    /* Check for legacy or poolstake specifier. */
                    bool fLegacy = false;
                    if(nType == SPECIFIER::LEGACY)
                    {
                        /* Set specifiers. */
                        fLegacy    = (nType == SPECIFIER::LEGACY);

                        /* Go to next type in stream. */
                        ssPacket >> nType;
                    }

                    /* Switch based on codes. */
                    switch(nType)
                    {
                        /* Standard type for a block. */
                        case TYPES::BLOCK:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::BLOCK))
                                return debug::drop(NODE, "BLOCK: unsolicited notification");

                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "block notify can't have legacy specifier");

                            /* Get the index of block. */
                            uint1024_t hashBlock;
                            ssPacket >> hashBlock;

                            /* Check for client mode. */
                            if(config::fClient.load())
                            {
                                /* Check the database for the block. */
                                if(!LLD::Client->HasBlock(hashBlock))
                                    ssResponse << uint8_t(SPECIFIER::CLIENT) << uint8_t(TYPES::BLOCK) << hashBlock;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::NOTIFY: CLIENT BLOCK ", hashBlock.SubString());
                            }
                            else
                            {
                                /* Check the database for the block. */
                                if(!LLD::Ledger->HasBlock(hashBlock))
                                    ssResponse << uint8_t(TYPES::BLOCK) << hashBlock;

                                /* Debug output. */
                                debug::log(3, NODE, "ACTION::NOTIFY: BLOCK ", hashBlock.SubString());
                            }

                            break;
                        }

                        /* Standard type for a block. */
                        case TYPES::TRANSACTION:
                        case TYPES::SIGCHAIN:
                        case TYPES::REGISTER:
                        {
                            /* Check for active subscriptions. */
                            if(nType == TYPES::TRANSACTION && !(nSubscriptions & SUBSCRIPTION::TRANSACTION))
                                return debug::drop(NODE, "ACTION::NOTIFY::TRANSACTION: unsolicited notification");

                            /* Sigchain specific validation and de-serialization. */
                            if(nType == TYPES::SIGCHAIN)
                            {
                                /* Check for available protocol version. */
                                if(nProtocolVersion < MIN_TRITIUM_VERSION)
                                    return true;

                                /* Check for subscription. */
                                if(!(nSubscriptions & SUBSCRIPTION::SIGCHAIN))
                                    return debug::drop(NODE, "ACTION::NOTIFY::SIGCHAIN: unsolicited notification");

                                /* Get the sigchain genesis. */
                                uint256_t hashSigchain;
                                ssPacket >> hashSigchain;

                                /* Check for expected genesis. */
                                const uint256_t hashSession =
                                    TAO::API::Authentication::Caller(uint256_t(TAO::API::Authentication::SESSION::DEFAULT), false);

                                /* Check this notification is for a valid logged in session. */
                                if(hashSigchain != hashSession)
                                    return debug::drop(NODE, "ACTION::NOTIFY::SIGCHAIN: unexpected genesis-id ", hashSession.SubString());
                            }

                            /* Notification validation */
                            else if(nType == TYPES::REGISTER)
                            {
                                /* Check for available protocol version. */
                                if(nProtocolVersion < MIN_TRITIUM_VERSION)
                                    return true;

                                /* Check for subscription. */
                                if(!(nSubscriptions & SUBSCRIPTION::REGISTER))
                                    return debug::drop(NODE, "ACTION::NOTIFY::REGISTER: unsolicited notification");

                                /* Check for legacy. */
                                if(fLegacy)
                                    return debug::drop(NODE, "ACTION::NOTIFY::REGISTER: cannot include legacy specifier");

                                /* Get the  address . */
                                uint256_t hashAddress;
                                ssPacket >> hashAddress;

                                /* If the address is a genesis hash, then make sure that it is for the currently logged in user */
                                if(hashAddress.GetType() == TAO::Ledger::GENESIS::UserType())
                                    return debug::drop(NODE, "ACTION::NOTIFY::REGISTER: notification cannot be genesis");
                            }

                            /* Get the index of transaction. */
                            uint512_t hashTx = 0;
                            ssPacket >> hashTx;

                            /* Handle for -client mode which deals with merkle transactions. */
                            if(nType == TYPES::SIGCHAIN && config::fClient.load())
                            {
                                /* Check ledger database. */
                                if(tInventory.Expired(hashTx, 60)) //60 second exipring cache
                                {
                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::NOTIFY: MERKLE TRANSACTION ", hashTx.SubString());

                                    /* Add legacy flag if necessary */
                                    if(fLegacy)
                                        ssResponse << uint8_t(SPECIFIER::LEGACY);

                                    /* Build our response to get transaction. */
                                    ssResponse << uint8_t(TYPES::MERKLE) << hashTx;

                                    /* Add to our cache inventory. */
                                    tInventory.Cache(hashTx);
                                }
                                else
                                    debug::log(3, NODE, "ACTION::NOTIFY: [CACHED] MERKLE TRANSACTION ", hashTx.SubString());

                                break;
                            }

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                /* Check legacy database. */
                                if(tInventory.Expired(hashTx, 60)) //60 second exipring cache
                                {
                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::NOTIFY: LEGACY TRANSACTION ", hashTx.SubString());

                                    /* Build our response to get transaction. */
                                    ssResponse << uint8_t(SPECIFIER::LEGACY) << uint8_t(TYPES::TRANSACTION) << hashTx;

                                    /* Add to our cache inventory. */
                                    tInventory.Cache(hashTx);
                                }
                                else
                                    debug::log(3, NODE, "ACTION::NOTIFY: [CACHED] LEGACY TRANSACTION ", hashTx.SubString());
                            }
                            else
                            {
                                /* Check ledger database. */
                                if(tInventory.Expired(hashTx, 60)) //60 second exipring cache
                                {
                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::NOTIFY: TRITIUM TRANSACTION ", hashTx.SubString());

                                    /* Build our response to get transaction. */
                                    ssResponse << uint8_t(TYPES::TRANSACTION) << hashTx;

                                    /* Add to our cache inventory. */
                                    tInventory.Cache(hashTx);
                                }
                                else
                                    debug::log(3, NODE, "ACTION::NOTIFY: [CACHED] TRITIUM TRANSACTION ", hashTx.SubString());
                            }

                            break;
                        }

                        /* Standard type for height. */
                        case TYPES::BESTHEIGHT:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::BESTHEIGHT))
                                return debug::drop(NODE, "BESTHEIGHT: unsolicited notification");

                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "height can't have legacy specifier");

                            /* Keep track of current height. */
                            ssPacket >> nCurrentHeight;

                            /* If this is syncing node, cache this as our stopping block. */
                            if(nCurrentSession == TAO::Ledger::nSyncSession.load())
                                nSyncStop.store(nCurrentHeight);

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: BESTHEIGHT ", nCurrentHeight);

                            break;
                        }

                        /* Standard type for a checkpoint. */
                        case TYPES::CHECKPOINT:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::CHECKPOINT))
                                return debug::drop(NODE, "CHECKPOINT: unsolicited notification");

                            /* Check for legacy. */
                            if(fLegacy)
                                return debug::drop(NODE, "checkpoint can't have legacy specifier");

                            /* Keep track of current checkpoint. */
                            ssPacket >> hashCheckpoint;

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: CHECKPOINT ", hashCheckpoint.SubString());

                            break;
                        }


                        /* Standard type for a checkpoint. */
                        case TYPES::LASTINDEX:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::LASTINDEX))
                                return debug::drop(NODE, "ACTION::NOTIFY: LASTINDEX: unsolicited notification");

                            /* Get the data type. */
                            uint8_t nType = 0;
                            ssPacket >> nType;

                            /* Switch based on different last index values. */
                            switch(nType)
                            {
                                /* Last index for a block is always uint1024_t. */
                                case TYPES::BLOCK:
                                {
                                    /* Check for legacy. */
                                    if(fLegacy)
                                        return debug::drop(NODE, "ACTION::NOTIFY: LASTINDEX: block can't have legacy specifier");

                                    /* Keep track of current checkpoint. */
                                    uint1024_t hashLast;
                                    ssPacket >> hashLast;

                                    /* Check if is sync node. */
                                    if(nCurrentSession == TAO::Ledger::nSyncSession.load())
                                    {
                                        /* Check for complete synchronization. */
                                        if(hashLast == TAO::Ledger::ChainState::hashBestChain.load()
                                        && hashLast == hashBestChain)
                                        {
                                            /* Set state to synchronized. */
                                            fSynchronized.store(true);
                                            TAO::Ledger::nSyncSession.store(0);

                                            /* Unsubcribe from last. */
                                            Unsubscribe(SUBSCRIPTION::LASTINDEX);

                                            /* Total blocks synchronized */
                                            uint32_t nBlocks = TAO::Ledger::ChainState::stateBest.load().nHeight - nSyncStart.load();

                                            /* Calculate the time to sync*/
                                            uint32_t nElapsed = SYNCTIMER.Elapsed();

                                            /* Log that sync is complete. */
                                            debug::log(0, NODE, "ACTION::NOTIFY: Synchronization COMPLETE at ", hashBestChain.SubString());
                                            debug::log(0, NODE, "ACTION::NOTIFY: Synchronized ", nBlocks, " blocks in ", nElapsed,
                                                " seconds [", double(nBlocks / (nElapsed + 1.0)), " blocks/s]" );
                                        }
                                        else
                                        {
                                            /* Ask for list of blocks. */
                                            PushMessage(ACTION::LIST,
                                                config::fClient.load() ? uint8_t(SPECIFIER::CLIENT) : uint8_t(SPECIFIER::SYNC),
                                                uint8_t(TYPES::BLOCK),
                                                uint8_t(TYPES::UINT1024_T),
                                                hashLast,
                                                uint1024_t(0)
                                            );
                                        }
                                    }

                                    /* Set the last index. */
                                    hashLastIndex = hashLast;

                                    /* Debug output. */
                                    debug::log(3, NODE, "ACTION::NOTIFY: LASTINDEX ", hashLast.SubString());

                                    break;
                                }
                            }

                            break;
                        }


                        /* Standard type for a checkpoint. */
                        case TYPES::BESTCHAIN:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::BESTCHAIN))
                                return debug::drop(NODE, "BESTCHAIN: unsolicited notification");

                            /* Keep track of current checkpoint. */
                            ssPacket >> hashBestChain;

                            /* Check if is sync node. */
                            if(TAO::Ledger::nSyncSession.load() != 0
                            && nCurrentSession == TAO::Ledger::nSyncSession.load()
                            && LLD::Ledger->HasBlock(hashBestChain))
                            {
                                /* Set state to synchronized. */
                                fSynchronized.store(true);
                                TAO::Ledger::nSyncSession.store(0);

                                /* Unsubcribe from last. */
                                Unsubscribe(SUBSCRIPTION::LASTINDEX);

                                /* Log that sync is complete. */
                                debug::log(0, NODE, "ACTION::NOTIFY: Synchronization COMPLETE at ", hashBestChain.SubString());
                            }

                            /* Debug output. */
                            debug::log(3, NODE, "ACTION::NOTIFY: BESTCHAIN ", hashBestChain.SubString());

                            break;
                        }


                        /* Standard type for na address. */
                        case TYPES::ADDRESS:
                        {
                            /* Check for subscription. */
                            if(!(nSubscriptions & SUBSCRIPTION::ADDRESS))
                                return debug::drop(NODE, "ADDRESS: unsolicited notification");

                            /* Get the base address. */
                            BaseAddress addr;
                            ssPacket >> addr;

                            /* We don't want to relay new addresses for client mode just yet. */
                            if(!config::fClient.load())
                            {
                                /* Add addresses to manager.. */
                                if(TRITIUM_SERVER->GetAddressManager())
                                {
                                    /* Only output debug info if new address. */
                                    if(TRITIUM_SERVER->GetAddressManager()->Has(addr))
                                    {
                                        /* Only notify address logs if more than ten minutes since last connection. */
                                        const LLP::TrustAddress addrInfo =
                                            TRITIUM_SERVER->GetAddressManager()->Get(addr);

                                        /* Calculate how long it has been since last connection. */
                                        const uint64_t nTimeAway =
                                            (runtime::unifiedtimestamp() - addrInfo.nLastSeen);

                                        /* Special debug output for new address recurring time. */
                                        if(addrInfo.nLastSeen > runtime::unifiedtimestamp() || addrInfo.nLastSeen == 0)
                                            debug::log(0, NODE, "ACTION::NOTIFY: UPDATE ADDRESS ", addr.ToStringIP());
                                        else if(nTimeAway > 1800)
                                        {
                                            /* Default time seen in minutes. */
                                            std::string strLastSeen =
                                                debug::safe_printstr((nTimeAway / 60), " MINUTES AGO");

                                            /* Handle if time seen is in hours. */
                                            if(nTimeAway > (60 * 60))
                                                strLastSeen = debug::safe_printstr((nTimeAway / (60 * 60)), " HOURS AGO");

                                            /* Handle if time seen is in days. */
                                            if(nTimeAway > (60 * 60 * 24))
                                                strLastSeen = debug::safe_printstr((nTimeAway / (60 * 60 * 24)), " DAYS AGO");

                                            debug::log(0, NODE, "ACTION::NOTIFY: ONLINE ADDRESS ", addr.ToStringIP(), " LAST SEEN ", strLastSeen);
                                        }

                                    }
                                    else
                                        debug::log(0, NODE, "ACTION::NOTIFY: NEW ADDRESS ", addr.ToStringIP());

                                    /* Add address to manager now. */
                                    TRITIUM_SERVER->GetAddressManager()->AddAddress(addr);
                                }
                            }

                            break;
                        }

                        /* Catch malformed notify binary streams. */
                        default:
                            return debug::drop(NODE, "ACTION::NOTIFY malformed binary stream");
                    }
                }

                /* Push a request for the data from notifications. */
                if(ssResponse.size() != 0)
                    WritePacket(NewMessage(ACTION::GET, ssResponse));

                break;
            }


            /* Handle for ping command. */
            case ACTION::PING:
            {
                /* Get the nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                /* Push the pong response. */
                PushMessage(ACTION::PONG, nNonce);

                /* Bump DDOS score. */
                if(fDDOS.load()) //a ping shouldn't be sent too much
                    DDOS->rSCORE += 10;

                break;
            }


            /* Handle a pong command. */
            case ACTION::PONG:
            {
                /* Get the nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                /* If the nonce was not received or known from pong. */
                if(!mapLatencyTracker.count(nNonce))
                {
                    /* Bump DDOS score for spammed PONG messages. */
                    if(fDDOS.load())
                        DDOS->rSCORE += 10;

                    return true;
                }

                /* Calculate the Average Latency of the Connection. */
                nLatency = mapLatencyTracker[nNonce].ElapsedMilliseconds();
                mapLatencyTracker.erase(nNonce);

                /* Set the latency used for address manager within server */
                if(TRITIUM_SERVER->GetAddressManager())
                    TRITIUM_SERVER->GetAddressManager()->SetLatency(nLatency, GetAddress());

                /* Debug Level 3: output Node Latencies. */
                debug::log(3, NODE, "Latency (Nonce ", std::hex, nNonce, " - ", std::dec, nLatency, " ms)");

                break;
            }


            /* Standard type for a timeseed. */
            case TYPES::TIMESEED:
            {
                /* Check for subscription. */
                if(!(nSubscriptions & SUBSCRIPTION::TIMESEED))
                    return debug::drop(NODE, "TYPES::TIMESEED: unsolicited data");

                /* Check for authorized node. */
                if(!Authorized())
                    return debug::drop(NODE, "cannot send timeseed if not authorized");

                /* Check trust threshold. */
                if(nTrust < 60 * 60)
                    return debug::drop(NODE, "cannot send timeseed with no trust");

                /* Get the time seed from network. */
                int64_t nTimeSeed = 0;
                ssPacket >> nTimeSeed;

                /* Keep track of the time seeds if accepted. */
                debug::log(2, NODE, "timeseed ", nTimeSeed, " ACCEPTED");

                break;
            }


            /* Standard type for an address. */
            case TYPES::ADDRESS:
            {
                /* Check for subscription. */
                if(!(nSubscriptions & SUBSCRIPTION::ADDRESS))
                    return debug::drop(NODE, "TYPES::ADDRESS: unsolicited data");

                /* Get the base address. */
                BaseAddress addr;
                ssPacket >> addr;

                /* Add addresses to manager.. */
                if(!config::fClient.load() && TRITIUM_SERVER->GetAddressManager())
                    TRITIUM_SERVER->GetAddressManager()->AddAddress(addr);

                break;
            }


            /* Handle incoming block. */
            case TYPES::BLOCK:
            {
                /* Check for subscription. */
                if(!(nSubscriptions & SUBSCRIPTION::BLOCK) && TAO::Ledger::nSyncSession.load() != nCurrentSession)
                    return debug::drop(NODE, "TYPES::BLOCK: unsolicited data");

                /* Star the sync timer if this is the first sync block */
                if(!SYNCTIMER.Running())
                    SYNCTIMER.Start();

                /* Get the specifier. */
                uint8_t nSpecifier = 0;
                ssPacket >> nSpecifier;

                /* Switch based on specifier. */
                uint8_t nStatus = 0;
                switch(nSpecifier)
                {
                    /* Handle for a legacy transaction. */
                    case SPECIFIER::LEGACY:
                    {
                        /* Check for client mode since this method should never be called except by a client. */
                        if(config::fClient.load())
                            return debug::drop(NODE, "TYPES::BLOCK::LEGACY: disabled in -client mode");

                        /* Get the block from the stream. */
                        Legacy::LegacyBlock block;
                        ssPacket >> block;

                        /* Process the block. */
                        TAO::Ledger::Process(block, nStatus);

                        /* Check for duplicate and ask for previous block. */
                        if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                        && !(nStatus & TAO::Ledger::PROCESS::IGNORED)
                        &&  (nStatus & TAO::Ledger::PROCESS::ORPHAN))
                        {
                            /* Ask for list of blocks. */
                            PushMessage(ACTION::LIST,
                                #ifndef DEBUG_MISSING
                                (config::fClient.load() ? uint8_t(SPECIFIER::CLIENT) : uint8_t(SPECIFIER::TRANSACTIONS)),
                                #endif
                                uint8_t(TYPES::BLOCK),
                                uint8_t(TYPES::LOCATOR),
                                TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                uint1024_t(block.hashPrevBlock)
                            );
                        }

                        break;
                    }

                    /* Handle for a tritium transaction. */
                    case SPECIFIER::TRITIUM:
                    {
                        /* Check for client mode since this method should never be called except by a client. */
                        if(config::fClient.load())
                            return debug::drop(NODE, "TYPES::BLOCK::TRITIUM: disabled in -client mode");

                        /* Get the block from the stream. */
                        TAO::Ledger::TritiumBlock block;
                        ssPacket >> block;

                        /* Process the block. */
                        TAO::Ledger::Process(block, nStatus);

                        /* Check for missing transactions. */
                        if(nStatus & TAO::Ledger::PROCESS::INCOMPLETE)
                        {
                            /* Check for repeated missing loops. */
                            if(fDDOS.load())
                            {
                                /* Iterate a failure for missing transactions. */
                                nConsecutiveFails += block.vMissing.size();

                                /* Bump DDOS score. */
                                DDOS->rSCORE += (block.vMissing.size() * 10);
                            }

                            /* Create response data stream. */
                            DataStream ssResponse(SER_NETWORK, PROTOCOL_VERSION);

                            /* Create a list of requested transactions. */
                            uint32_t nTotalItems = 0;
                            for(const auto& tx : block.vMissing)
                            {
                                /* Check for legacy. */
                                if(tx.first == TAO::Ledger::TRANSACTION::LEGACY)
                                    ssResponse << uint8_t(SPECIFIER::LEGACY);

                                /* Push to stream. */
                                ssResponse << uint8_t(TYPES::TRANSACTION) << tx.second;

                                /* Log the missing data. */
                                debug::log(0, FUNCTION, "requesting missing tx ", tx.second.SubString());

                                /* Check if we need to create new protocol message. */
                                if(++nTotalItems >= ACTION::GET_MAX_ITEMS || tx == block.vMissing.back())
                                {
                                    debug::log(0, FUNCTION, "broadcasting packet with ", nTotalItems, " items");

                                    /* Write our packet with our total items. */
                                    WritePacket(NewMessage(ACTION::GET, ssResponse));

                                    /* Clear our response data. */
                                    ssResponse.clear();

                                    /* Reset our counters. */
                                    nTotalItems = 0;
                                }
                            }

                            /* Expired our missing block last. */
                            PushMessage(ACTION::GET, uint8_t(TYPES::BLOCK), block.hashMissing);
                        }

                        /* Check for duplicate and ask for previous block. */
                        if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                        && !(nStatus & TAO::Ledger::PROCESS::IGNORED)
                        && !(nStatus & TAO::Ledger::PROCESS::INCOMPLETE)
                        &&  (nStatus & TAO::Ledger::PROCESS::ORPHAN))
                        {
                            /* Ask for list of blocks. */
                            PushMessage(ACTION::LIST,
                                #ifndef DEBUG_MISSING
                                (config::fClient.load() ? uint8_t(SPECIFIER::CLIENT) : uint8_t(SPECIFIER::TRANSACTIONS)),
                                #endif
                                uint8_t(TYPES::BLOCK),
                                uint8_t(TYPES::LOCATOR),
                                TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                uint1024_t(block.hashPrevBlock)
                            );
                        }

                        break;
                    }

                    /* Handle for a tritium transaction. */
                    case SPECIFIER::SYNC:
                    {
                        /* Check for client mode since this method should never be called except by a client. */
                        if(config::fClient.load())
                            return debug::drop(NODE, "TYPES::BLOCK::SYNC: disabled in -client mode");

                        /* Check if this is an unsolicited sync block. */
                        //if(nCurrentSession != TAO::Ledger::nSyncSession)
                        //    return debug::drop(FUNCTION, "unsolicted sync block");

                        /* Get the block from the stream. */
                        TAO::Ledger::SyncBlock block;
                        ssPacket >> block;

                        /* Check version switch. */
                        if(block.nVersion >= 7)
                        {
                            /* Build a tritium block from sync block. */
                            TAO::Ledger::TritiumBlock tritium(block);

                            /* Verbose debug output. */
                            if(config::nVerbose >= 3)
                                debug::log(3, FUNCTION, "received sync block ", tritium.GetHash().SubString(), " height = ", block.nHeight);

                            /* Process the block. */
                            TAO::Ledger::Process(tritium, nStatus);
                        }
                        else
                        {
                            /* Build a tritium block from sync block. */
                            Legacy::LegacyBlock legacy(block);

                            /* Verbose debug output. */
                            if(config::nVerbose >= 3)
                                debug::log(3, FUNCTION, "received sync block ", legacy.GetHash().SubString(), " height = ", block.nHeight);

                            /* Process the block. */
                            TAO::Ledger::Process(legacy, nStatus);
                        }

                        break;
                    }


                    /* Handle for a tritium transaction. */
                    case SPECIFIER::CLIENT:
                    {
                        /* Get the block from the stream. */
                        TAO::Ledger::ClientBlock block;
                        ssPacket >> block;

                        /* Process the block. */
                        TAO::Ledger::Process(block, nStatus);

                        /* Check for duplicate and ask for previous block. */
                        if(!(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                        && !(nStatus & TAO::Ledger::PROCESS::IGNORED)
                        &&  (nStatus & TAO::Ledger::PROCESS::ORPHAN))
                        {
                            /* Ask for list of blocks. */
                            PushMessage(ACTION::LIST,
                                uint8_t(SPECIFIER::CLIENT),
                                uint8_t(TYPES::BLOCK),
                                uint8_t(TYPES::LOCATOR),
                                TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
                                uint1024_t(block.hashPrevBlock)
                            );
                        }

                        break;

                        /* Log received. */
                        debug::log(3, FUNCTION, "received client block ", block.GetHash().SubString(), " height = ", block.nHeight);

                        break;
                    }

                    /* Default catch all. */
                    default:
                        return debug::drop(NODE, "invalid type specifier for block");
                }

                /* Check for specific status messages. */
                if(nStatus & TAO::Ledger::PROCESS::ACCEPTED)
                {
                    /* Reset the fails and orphans. */
                    nConsecutiveOrphans = 0;
                    nConsecutiveFails   = 0;

                    /* Reset last time received. */
                    if(nCurrentSession == TAO::Ledger::nSyncSession.load())
                        nLastTimeReceived.store(runtime::timestamp());
                }

                /* Check for failure status messages. */
                if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                    ++nConsecutiveFails;

                /* Check for orphan status messages. */
                if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
                    ++nConsecutiveOrphans;

                /* Detect large orphan chains and ask for new blocks from origin again. */
                if(nConsecutiveOrphans >= 1000)
                {
                    {
                        LOCK(TAO::Ledger::PROCESSING_MUTEX);

                        /* Clear the memory to prevent DoS attacks. */
                        TAO::Ledger::mapOrphans.clear();
                    }

                    /* Switch to another available node. */
                    if(TAO::Ledger::ChainState::Synchronizing() && TAO::Ledger::nSyncSession.load() == nCurrentSession)
                        SwitchNode();

                    /* Disconnect from a node with large orphan chain. */
                    return debug::drop(NODE, "node reached orphan limit");
                }


                /* Check for failure limit on node. */
                if(nConsecutiveFails >= 1000)
                {
                    /* Switch to another available node. */
                    if(TAO::Ledger::ChainState::Synchronizing())
                    {
                        /* If this is our sync session, switch nodes and restart syncing. */
                        if(TAO::Ledger::nSyncSession.load() == nCurrentSession)
                        {
                            SwitchNode();
                            return true;
                        }

                        return debug::drop(NODE, "has sent ", nConsecutiveFails, " invalid consecutive transactions");
                    }

                    /* Drop pesky nodes. */
                    return debug::ban(this, NODE, "has sent ", nConsecutiveFails, " invalid consecutive blocks");
                }

                break;
            }


            /* Handle incoming transaction. */
            case TYPES::TRANSACTION:
            {
                /* Check for subscription. */
                if(!(nSubscriptions & SUBSCRIPTION::TRANSACTION))
                    return debug::drop(NODE, "TYPES::TRANSACTION: unsolicited data");

                /* Get the specifier. */
                uint8_t nSpecifier = 0;
                ssPacket >> nSpecifier;

                /* Switch based on type. */
                switch(nSpecifier)
                {
                    /* Handle for a legacy transaction. */
                    case SPECIFIER::LEGACY:
                    {
                        /* Get the transction from the stream. */
                        Legacy::Transaction tx;
                        ssPacket >> tx;

                        /* Accept into memory pool. */
                        if(TAO::Ledger::mempool.Accept(tx, this))
                        {
                            /* Relay the transaction notification. */
                            TRITIUM_SERVER->Relay
                            (
                                ACTION::NOTIFY,
                                uint8_t(SPECIFIER::LEGACY),
                                uint8_t(TYPES::TRANSACTION),
                                tx.GetHash()
                            );

                            /* Reset consecutive failures. */
                            nConsecutiveFails   = 0;
                            nConsecutiveOrphans = 0;
                        }
                        else
                            ++nConsecutiveFails;

                        break;
                    }

                    /* Handle for a tritium transaction. */
                    case SPECIFIER::TRITIUM:
                    {
                        /* Get the transction from the stream. */
                        TAO::Ledger::Transaction tx;
                        ssPacket >> tx;

                        /* Accept into memory pool. */
                        if(TAO::Ledger::mempool.Accept(tx, this))
                        {
                            /* Relay the transaction notification. */
                            uint512_t hashTx = tx.GetHash();
                            TRITIUM_SERVER->Relay
                            (
                                /* Standard transaction relay. */
                                ACTION::NOTIFY,
                                uint8_t(TYPES::TRANSACTION),
                                hashTx,

                                /* Handle sigchain related notifications. */
                                uint8_t(TYPES::SIGCHAIN),
                                tx.hashGenesis,
                                hashTx
                            );

                            /* Reset consecutive failures. */
                            nConsecutiveFails   = 0;
                            nConsecutiveOrphans = 0;
                        }
                        else
                        {
                            /* Check for obsolete transaction version and ban accordingly. */
                            if(!TAO::Ledger::TransactionVersionActive(tx.nTimestamp, tx.nVersion))
                                return debug::drop(NODE, "invalid transaction version, dropping node");

                            ++nConsecutiveFails;
                        }

                        break;
                    }

                    /* Default catch all. */
                    default:
                        return debug::drop(NODE, "invalid type specifier for transaction");
                }

                /* Check for failure limit on node. */
                if(nConsecutiveFails >= 100)
                {
                    /* Only drop the node when syncronizing the chain. */
                    if(TAO::Ledger::ChainState::Synchronizing())
                        return debug::drop(NODE, "has sent ", nConsecutiveFails, " invalid consecutive transactions");

                    /* Otherwise ban this node for consecutive failures. */
                    return debug::ban(this, NODE, "has sent ", nConsecutiveFails, " invalid consecutive transactions");
                }


                /* Check for orphan limit on node. */
                if(nConsecutiveOrphans >= 1000)
                    return debug::drop(NODE, "TX::node reached ORPHAN limit");

                break;
            }


            /* Handle incoming merkle transaction. */
            case TYPES::MERKLE:
            {
                /* Check for subscription. */
                if(!config::fClient.load())
                    return debug::drop(NODE, "TYPES::MERKLE: unavailable when not in -client mode");

                //if(!(nSubscriptions & SUBSCRIPTION::SIGCHAIN))
                //    return debug::drop(NODE, "TYPES::MERKLE: unsolicited data");

                /* Get the specifier. */
                uint8_t nSpecifier = 0;
                ssPacket >> nSpecifier;

                /* Switch based on type. */
                switch(nSpecifier)
                {
                    /* Handle for a legacy transaction. */
                    case SPECIFIER::LEGACY:
                    {
                        /* Get the transction from the stream. */
                        Legacy::MerkleTx tx;
                        ssPacket >> tx;

                        /* Cache the txid. */
                        const uint512_t hashTx = tx.GetHash();

                        /* Run basic merkle tx checks */
                        if(!tx.Verify())
                            return debug::drop(NODE, "FLAGS::LOOKUP: ", hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                        /* Start our ACID transaction in case we have any failures here. */
                        { LOCK(CLIENT_MUTEX);

                            LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK);

                            /* Build indexes if we don't have them. */
                            if(!LLD::Client->HasIndex(hashTx))
                            {
                                /* Commit transaction to disk. */
                                if(!LLD::Client->WriteTx(hashTx, tx))
                                    return debug::abort(TAO::Ledger::FLAGS::BLOCK, FUNCTION, "failed to write transaction");

                                /* Index the transaction to it's block. */
                                if(!LLD::Client->IndexBlock(hashTx, tx.hashBlock))
                                    return debug::abort(TAO::Ledger::FLAGS::BLOCK, FUNCTION, "failed to write block indexing entry");
                            }

                            /* Add an indexing event. */
                            TAO::API::Indexing::IndexDependant(hashTx, tx);

                            /* Commit our ACID transaction across LLD instances. */
                            LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK);
                        }

                        /* Verbose=3 dumps transaction data. */
                        if(config::nVerbose >= 3)
                            tx.print();

                        /* Write Success to log. */
                        debug::log(3, "MERKLE::LEGACY: ", hashTx.SubString(), " ACCEPTED");

                        break;
                    }

                    /* Handle for a tritium transaction. */
                    case SPECIFIER::TRITIUM:
                    case SPECIFIER::DEPENDANT:
                    {
                        /* Get the transction from the stream. */
                        TAO::Ledger::MerkleTx tx;
                        ssPacket >> tx;

                        /* Cache the txid. */
                        const uint512_t hashTx = tx.GetHash();

                        /* Check for empty merkle tx. */
                        if(tx.hashBlock != 0)
                        {
                            /* Verbose=3 dumps transaction data. */
                            if(config::nVerbose >= 3)
                                tx.print();

                            /* Run basic merkle tx checks */
                            if(!tx.Verify())
                                return debug::error(FUNCTION, hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                            /* Start our ACID transaction in case we have any failures here. */
                            { LOCK(CLIENT_MUTEX);

                                LLD::TxnBegin(TAO::Ledger::FLAGS::BLOCK);

                                /* Only write to disk and index if not completed already. */
                                if(!LLD::Client->HasIndex(hashTx))
                                {
                                    /* Commit transaction to disk. */
                                    if(!LLD::Client->WriteTx(hashTx, tx))
                                        return debug::abort(TAO::Ledger::FLAGS::BLOCK, FUNCTION, "failed to write transaction");

                                    /* Index the transaction to it's block. */
                                    if(!LLD::Client->IndexBlock(hashTx, tx.hashBlock))
                                        return debug::abort(TAO::Ledger::FLAGS::BLOCK, FUNCTION, "failed to write block indexing entry");
                                }

                                /* Dependant specifier only needs to index dependant. */
                                if(nSpecifier == SPECIFIER::DEPENDANT)
                                    TAO::API::Indexing::IndexDependant(hashTx, tx);
                                else
                                {
                                    /* Connect transaction in memory. */
                                    if(!tx.Connect(TAO::Ledger::FLAGS::BLOCK))
                                        return debug::abort(TAO::Ledger::FLAGS::BLOCK, FUNCTION, hashTx.SubString(), " REJECTED: ", debug::GetLastError());

                                    /* Add an indexing event. */
                                    TAO::API::Indexing::IndexSigchain(hashTx);
                                }

                                /* Commit our ACID transaction across LLD instances. */
                                LLD::TxnCommit(TAO::Ledger::FLAGS::BLOCK);
                            }

                            /* Write Success to log. */
                            debug::log(3, "MERKLE::TRITIUM: ", hashTx.SubString(), " ACCEPTED");
                        }
                        else
                        {
                            debug::log(0, "No merkle branch for tx ", hashTx.SubString());
                            TAO::Ledger::mempool.Accept(tx, this);
                        }

                        break;
                    }

                    /* Default catch all. */
                    default:
                        return debug::drop(NODE, "TYPES::MERKLE: invalid type specifier for TYPES::MERKLE");
                }

                break;
            }


            /* Handle an event trigger. */
            case TYPES::TRIGGER:
            {
                /* De-serialize the trigger nonce. */
                ssPacket >> nTriggerNonce;

                break;
            }


            /* Handle an event trigger. */
            case RESPONSE::COMPLETED:
            {
                /* De-serialize the trigger nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;

                TriggerEvent(INCOMING.MESSAGE, nNonce);

                break;
            }

            default:
                return debug::drop(NODE, "invalid protocol message ", INCOMING.MESSAGE);
        }

        /* Check for authorization. */
        if(fDDOS.load() && !Authorized())
            DDOS->rSCORE += 5; //untrusted nodes get less requests

        /* Check for a version message. */
        if(nProtocolVersion == 0 || nCurrentSession == 0)
            return debug::drop(NODE, "first message wasn't a version message");

        return true;
    }


    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
    void TritiumNode::ReadPacket()
    {
        if(!INCOMING.Complete())
        {
            /** Handle Reading Packet Length Header. **/
            if(!INCOMING.Header() && Available() >= 8)
            {
                std::vector<uint8_t> BYTES(8, 0);
                if(Read(BYTES, 8) == 8)
                {
                    DataStream ssHeader(BYTES, SER_NETWORK, MIN_PROTO_VERSION);
                    ssHeader >> INCOMING;

                    Event(EVENTS::HEADER);
                }
            }

            /** Handle Reading Packet Data. **/
            uint32_t nAvailable = Available();
            if(INCOMING.Header() && nAvailable > 0 && !INCOMING.IsNull() && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                /* The maximum number of bytes to read is th number of bytes specified in the message length,
                   minus any already read on previous reads*/
                uint32_t nMaxRead = (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size());

                /* Vector to receve the read bytes. This should be the smaller of the number of bytes currently available or the
                   maximum amount to read */
                std::vector<uint8_t> DATA(std::min(nAvailable, nMaxRead), 0);

                /* Read up to the buffer size. */
                int32_t nRead = Read(DATA, DATA.size());

                /* If something was read, insert it into the packet data.  NOTE: that due to SSL packet framing we could end up
                   reading less bytes than appear available.  Therefore we only copy the number of bytes actually read */
                if(nRead > 0)
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.begin() + nRead);

                /* If the packet is now considered complete, fire the packet complete event */
                if(INCOMING.Complete())
                    Event(EVENTS::PACKET, static_cast<uint32_t>(DATA.size()));
            }
        }
    }


    /* Determine if a node is authorized and therfore trusted. */
    bool TritiumNode::Authorized() const
    {
        return hashGenesis != 0 && fAuthorized;
    }


    /* Unsubscribe from another node for notifications. */
    void TritiumNode::Unsubscribe(const uint16_t nFlags)
    {
        /* Set the timestamp that we unsubscribed at. */
        nUnsubscribed = runtime::timestamp();

        /* Unsubscribe over the network. */
        Subscribe(nFlags, false);
    }


    /* Subscribe to another node for notifications. */
    void TritiumNode::Subscribe(const uint16_t nFlags, bool fSubscribe)
    {
        /* Build subscription message. */
        DataStream ssMessage(SER_NETWORK, MIN_PROTO_VERSION);

        /* Check for block. */
        if(nFlags & SUBSCRIPTION::BLOCK)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::BLOCK);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::BLOCK;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO BLOCK ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM BLOCK ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for transaction. */
        if(nFlags & SUBSCRIPTION::TRANSACTION)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::TRANSACTION);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::TRANSACTION;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO TRANSACTION ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM TRANSACTION ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for time seed. */
        if(nFlags & SUBSCRIPTION::TIMESEED)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::TIMESEED);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::TIMESEED;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO TIMESEED ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM TIMESEED ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for height. */
        if(nFlags & SUBSCRIPTION::BESTHEIGHT)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::BESTHEIGHT);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::BESTHEIGHT;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO BESTHEIGHT ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM BESTHEIGHT ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for checkpoint. */
        if(nFlags & SUBSCRIPTION::CHECKPOINT)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::CHECKPOINT);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::CHECKPOINT;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO CHECKPOINT ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM CHECKPOINT ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for address. */
        if(nFlags & SUBSCRIPTION::ADDRESS)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::ADDRESS);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::ADDRESS;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO ADDRESS ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM ADDRESS ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for last. */
        if(nFlags & SUBSCRIPTION::LASTINDEX)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::LASTINDEX);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::LASTINDEX;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO LASTINDEX ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM LASTINDEX ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Check for last. */
        if(nFlags & SUBSCRIPTION::BESTCHAIN)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::BESTCHAIN);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::BESTCHAIN;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO BESTCHAIN ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM BESTCHAIN ", std::bitset<16>(nSubscriptions));
            }
        }


        /* Check for sigchain. */
        if(nFlags & SUBSCRIPTION::SIGCHAIN)
        {
            /* Build the message. */
            ssMessage << uint8_t(TYPES::SIGCHAIN);

            /* Check for subscription. */
            if(fSubscribe)
            {
                /* Set the flag. */
                nSubscriptions |=  SUBSCRIPTION::SIGCHAIN;

                /* Debug output. */
                debug::log(3, NODE, "SUBSCRIBING TO SIGCHAIN ", std::bitset<16>(nSubscriptions));
            }
            else
            {
                /* Debug output. */
                debug::log(3, NODE, "UNSUBSCRIBING FROM SIGCHAIN ", std::bitset<16>(nSubscriptions));
            }
        }

        /* Write the subscription packet. */
        WritePacket(NewMessage((fSubscribe ? ACTION::SUBSCRIBE : ACTION::UNSUBSCRIBE), ssMessage));
    }


    /* Unsubscribe from another node for notifications. */
    void TritiumNode::UnsubscribeAddress(const uint256_t& hashAddress)
    {
        /* Set the timestamp that we unsubscribed at. */
        nUnsubscribed = runtime::timestamp();

        /* Unsubscribe over the network. */
        SubscribeAddress(hashAddress, false);
    }


    /* Subscribe to another node for notifications. */
    void TritiumNode::SubscribeAddress(const uint256_t& hashAddress, bool fSubscribe)
    {
        /* Build subscription message. */
        DataStream ssMessage(SER_NETWORK, MIN_PROTO_VERSION);

        /* Build the message. */
        ssMessage << uint8_t(TYPES::NOTIFICATION) << hashAddress;

        /* Check for subscription. */
        if(fSubscribe)
        {
            /* Set the flag. */
            nSubscriptions |=  SUBSCRIPTION::REGISTER;

            /* Store the address subscribed to so that we can validate when the peer sends us notifications */
            setSubscriptions.insert(hashAddress);

            /* Debug output. */
            debug::log(3, NODE, "SUBSCRIBING TO NOTIFICATION ", std::bitset<16>(nSubscriptions));
        }
        else
        {
            /* Remove the address from the notifications vector for this user */
            setSubscriptions.erase(hashAddress);

            /* Debug output. */
            debug::log(3, NODE, "UNSUBSCRIBING FROM NOTIFICATION ", std::bitset<16>(nSubscriptions));
        }

        /* Write the subscription packet. */
        WritePacket(NewMessage((fSubscribe ? ACTION::SUBSCRIBE : ACTION::UNSUBSCRIBE), ssMessage));
    }


    /* Builds an Auth message for this node.*/
    DataStream TritiumNode::GetAuth(bool fAuth)
    {
        /* Build auth message. */
        DataStream ssMessage(SER_NETWORK, MIN_PROTO_VERSION);

        /* Get the hash genesis. */
        const uint256_t hashSigchain = TAO::API::Authentication::Caller(uint256_t(TAO::API::Authentication::SESSION::DEFAULT), false); //no parameter goes to default session

        /* Only send auth messages if the auth key has been cached */
        if(hashSigchain != 0)
        {
            /* Unlock sigchain to create new block. */
            //SecureString strPIN;
            //RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::NETWORK));

            /* Get an instance of our credentials. */
            //const auto& pCredentials =
            //    TAO::API::Authentication::Credentials(uint256_t(TAO::API::Authentication::SESSION::DEFAULT));

            /* Get our current network timestamps. */
            const uint64_t nTimestamp = runtime::unifiedtimestamp();
            ssMessage << hashSigchain << nTimestamp << SESSION_ID;

            /* Build a hash of our current message. */
            const uint256_t hashCheck =
                LLC::SK256(ssMessage.begin(), ssMessage.end());

            /* Build our public key and signature. */
            //std::vector<uint8_t> vchPubKey;
            //std::vector<uint8_t> vchSig;

            /* Sign our message now with our network key. */
            //pCredentials->Sign("network", hashCheck.GetBytes(), pCredentials->Key("network", 0, ), vchPubKey, vchSig);

            //ssMessage << vchPubKey;
            //ssMessage << vchSig;

            debug::log(0, FUNCTION, "SIGNING MESSAGE: ", hashSigchain.SubString(), " at timestamp ", nTimestamp);
        }

        return ssMessage;
    }


    /*  Authorize this node to the connected node */
    void TritiumNode::Auth(bool fAuth)
    {
        /* Get the auth message */
        DataStream ssMessage = GetAuth(fAuth);

        /* Check whether it is valid before sending it */
        if(ssMessage.size() > 0)
            WritePacket(NewMessage((fAuth ? ACTION::AUTH : ACTION::DEAUTH), ssMessage));
    }


    /* Checks if a node is subscribed to receive a notification. */
    const DataStream TritiumNode::RelayFilter(const uint16_t nMsg, const DataStream& ssData) const
    {
        /* The data stream to relay*/
        DataStream ssRelay(SER_NETWORK, MIN_PROTO_VERSION);

        /* Switch based on message type */
        switch(nMsg)
        {
            /* Filter notifications. */
            case ACTION::NOTIFY:
            {
                /* Build a response data stream. */
                while(!ssData.End())
                {
                    /* Get the first notify type. */
                    uint8_t nType = 0;
                    ssData >> nType;

                    /* Check for legacy or poolstake specifier. */
                    bool fLegacy = false;
                    if(nType == SPECIFIER::LEGACY)
                    {
                        /* Set specifiers. */
                        fLegacy    = (nType == SPECIFIER::LEGACY);

                        /* Go to next type in stream. */
                        ssData >> nType;
                    }

                    /* Switch based on type. */
                    switch(nType)
                    {
                        /* Check for block subscription. */
                        case TYPES::BLOCK:
                        {
                            /* Get the index. */
                            uint1024_t hashBlock;
                            ssData >> hashBlock;

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::BLOCK)
                            {
                                /* Write block to stream. */
                                ssRelay << uint8_t(TYPES::BLOCK);
                                ssRelay << hashBlock;
                            }

                            break;
                        }


                        /* Check for transaction subscription. */
                        case TYPES::TRANSACTION:
                        {
                            /* Get the index. */
                            uint512_t hashTx;
                            ssData >> hashTx;

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::TRANSACTION)
                            {
                                /* Check for legacy. */
                                if(fLegacy)
                                    ssRelay << uint8_t(SPECIFIER::LEGACY);

                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::TRANSACTION);
                                ssRelay << hashTx;
                            }

                            break;
                        }


                        /* Check for height subscription. */
                        case TYPES::BESTHEIGHT:
                        {
                            /* Get the index. */
                            uint32_t nHeight;
                            ssData >> nHeight;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                debug::warning(FUNCTION, "BESTHEIGHT cannot have legacy specifier");
                                break;
                            }

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::BESTHEIGHT)
                            {
                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::BESTHEIGHT);
                                ssRelay << nHeight;
                            }

                            break;
                        }


                        /* Check for checkpoint subscription. */
                        case TYPES::CHECKPOINT:
                        {
                            /* Get the index. */
                            uint1024_t hashCheck;
                            ssData >> hashCheck;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                debug::error(FUNCTION, "CHECKPOINT cannot have legacy specifier");
                                continue;
                            }

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::CHECKPOINT)
                            {
                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::CHECKPOINT);
                                ssRelay << hashCheck;
                            }

                            break;
                        }


                        /* Check for best chain subscription. */
                        case TYPES::BESTCHAIN:
                        {
                            /* Get the index. */
                            uint1024_t hashBest;
                            ssData >> hashBest;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                debug::error(FUNCTION, "BESTCHAIN cannot have legacy specifier");
                                continue;
                            }

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::BESTCHAIN)
                            {
                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::BESTCHAIN);
                                ssRelay << hashBest;
                            }

                            break;
                        }


                        /* Check for address subscription. */
                        case TYPES::ADDRESS:
                        {
                            /* Get the index. */
                            BaseAddress addr;
                            ssData >> addr;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                debug::error(FUNCTION, "ADDRESS cannot have legacy specifier");
                                continue;
                            }

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::ADDRESS)
                            {
                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::ADDRESS);
                                ssRelay << addr;
                            }

                            break;
                        }


                        /* Check for sigchain subscription. */
                        case TYPES::SIGCHAIN:
                        {
                            /* Get the index. */
                            uint256_t hashSigchain = 0;
                            ssData >> hashSigchain;

                            /* Get the txid. */
                            uint512_t hashTx = 0;
                            ssData >> hashTx;

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::SIGCHAIN)
                            {
                                /* Check for matching sigchain-id. */
                                if(hashSigchain != hashGenesis)
                                    break;

                                /* Check for legacy. */
                                if(fLegacy)
                                    ssRelay << uint8_t(SPECIFIER::LEGACY);

                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::SIGCHAIN);
                                ssRelay << hashSigchain;
                                ssRelay << hashTx;
                            }

                            break;
                        }


                        /* Check for notification subscription. */
                        case TYPES::REGISTER:
                        {
                            /* Get the sig chain / register that the transaction relates to. */
                            uint256_t hashAddress = 0;
                            ssData >> hashAddress;

                            /* Get the txid. */
                            uint512_t hashTx = 0;
                            ssData >> hashTx;

                            /* Check for legacy. */
                            if(fLegacy)
                            {
                                debug::warning(FUNCTION, "REGISTER cannot have legacy specifier");
                                break;
                            }

                            /* Check subscription. */
                            if(nNotifications & SUBSCRIPTION::REGISTER)
                            {
                                /* Check that the address is one that has been subscribed to */
                                if(!setSubscriptions.count(hashAddress))
                                    break;

                                /* Write transaction to stream. */
                                ssRelay << uint8_t(TYPES::REGISTER);
                                ssRelay << hashAddress;
                                ssRelay << hashTx;
                            }

                            break;
                        }

                        /* Default catch (relay up to this point) */
                        default:
                        {
                            debug::error(FUNCTION, "Malformed binary stream");
                            return ssRelay;
                        }
                    }
                }

                break;
            }

            default:
            {
                /* default behaviour is to let the message be relayed */
                ssRelay = ssData;
                break;
            }
        }

        return ssRelay;
    }


    /* Determine whether a node is syncing. */
    bool TritiumNode::Syncing()
    {
        LOCK(SESSIONS_MUTEX);

        /* Check if sync session is active. */
        const uint64_t nSession = TAO::Ledger::nSyncSession.load();
        if(nSession == 0)
            return false;

        return mapSessions.count(nSession);
    }


    /* Get a node by connected session. */
    std::shared_ptr<TritiumNode> TritiumNode::GetNode(const uint64_t nSession)
    {
        LOCK(SESSIONS_MUTEX);

        /* Check for connected session. */
        static std::shared_ptr<TritiumNode> pNULL;
        if(!mapSessions.count(nSession))
            return pNULL;

        /* Get a reference of session. */
        const std::pair<uint32_t, uint32_t>& pair = mapSessions[nSession];
        return TRITIUM_SERVER->GetConnection(pair.first, pair.second);
    }


    /* Helper function to switch the nodes on sync. */
    void TritiumNode::SwitchNode()
    {
        std::pair<uint32_t, uint32_t> pairSession;
        { LOCK(SESSIONS_MUTEX);

            /* Check for session. */
            if(!mapSessions.count(TAO::Ledger::nSyncSession.load()))
                return;

            /* Set the current session. */
            pairSession = mapSessions[TAO::Ledger::nSyncSession.load()];
        }

        /* Normal case of asking for a getblocks inventory message. */
        std::shared_ptr<TritiumNode> pnode = TRITIUM_SERVER->GetConnection(pairSession);
        if(pnode != nullptr)
        {
            /* Send out another getblocks request. */
            try
            {
                /* Get the current sync node. */
                std::shared_ptr<TritiumNode> pcurrent = TRITIUM_SERVER->GetConnection(pairSession.first, pairSession.second);
                pcurrent->Unsubscribe(SUBSCRIPTION::LASTINDEX | SUBSCRIPTION::BESTCHAIN);

                /* Initiate the sync */
                pnode->Sync();
            }
            catch(const std::exception& e)
            {
                /* Recurse on failure. */
                debug::error(FUNCTION, e.what());

                SwitchNode();
            }
        }
        else
        {
            /* Reset the current sync node. */
            TAO::Ledger::nSyncSession.store(0);

            /* Logging to verify (for debugging). */
            debug::log(0, FUNCTION, "No Sync Nodes Available");
        }
    }


    /* Initiates a chain synchronization from the peer. */
    void TritiumNode::Sync()
    {
        debug::log(0, NODE, "New sync address set ", std::hex, nCurrentSession, ", syncing from ", TAO::Ledger::ChainState::hashBestChain.load().SubString());

        /* Set the sync session-id. */
        TAO::Ledger::nSyncSession.store(nCurrentSession);

        /* Reset last time received. */
        nLastTimeReceived.store(runtime::unifiedtimestamp());

        /* Cache the height at the start of the sync */
        nSyncStart.store(TAO::Ledger::ChainState::stateBest.load().nHeight);

        /* Make sure the sync timer is stopped.  We don't start this until we receive our first sync block*/
        SYNCTIMER.Stop();

        /* Subscribe to this node. */
        Subscribe(SUBSCRIPTION::LASTINDEX | SUBSCRIPTION::BESTCHAIN | SUBSCRIPTION::BESTHEIGHT);

        /* Ask for list of blocks if this is current sync node. */
        PushMessage(ACTION::LIST,
            config::fClient.load() ? uint8_t(SPECIFIER::CLIENT) : uint8_t(SPECIFIER::SYNC),
            uint8_t(TYPES::BLOCK),
            uint8_t(TYPES::LOCATOR),
            TAO::Ledger::Locator(TAO::Ledger::ChainState::hashBestChain.load()),
            uint1024_t(0)
        );
    }
}
