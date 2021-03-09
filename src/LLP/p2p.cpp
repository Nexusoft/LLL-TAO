/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/p2p.h>
#include <LLP/include/global.h>
#include <LLP/templates/events.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/sessionmanager.h>

#include <Util/include/runtime.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/version.h>



namespace LLP
{
    using namespace LLP::P2P;

    /* Declaration of sessions mutex. (private). */
    std::mutex P2PNode::SESSIONS_MUTEX;


    /* Declaration of sessions sets. (private). */
    std::map<std::tuple<std::string, uint256_t, uint256_t>, std::pair<uint32_t, uint32_t>> P2PNode::mapSessions;



    /** Default Constructor **/
    P2PNode::P2PNode()
    : BaseConnection<MessagePacket>()
    , fInitialized(false)
    , MESSAGES_MUTEX()
    , queueMessages()
    , strAppID()
    , hashGenesis(0)
    , hashPeer(0)
    , nSession(0)
    , nProtocolVersion(0)
    , nLastPing(0)
    , mapLatencyTracker()
    {
    }


    /** Constructor for incoming connection **/
    P2PNode::P2PNode(const Socket SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket>(SOCKET_IN, DDOS_IN, fDDOSIn)
    , fInitialized(false)
    , strAppID()
    , hashGenesis(0)
    , hashPeer(0)
    , nSession(0)
    , nProtocolVersion(0)
    , nLastPing(0)
    , mapLatencyTracker()
    {
    }


    /** Constructor for outgoing connection **/
    P2PNode::P2PNode(DDOS_Filter* DDOS_IN, bool fDDOSIn,
                const std::string& APPID,
                const uint256_t& HASHGENESIS,
                const uint256_t& HASHPEER,
                const uint64_t& SESSIONID)
    : BaseConnection<MessagePacket>(DDOS_IN, fDDOSIn)
    , fInitialized(false)
    , strAppID(APPID)
    , hashGenesis(HASHGENESIS)
    , hashPeer(HASHPEER)
    , nSession(SESSIONID)
    , nProtocolVersion(0)
    , nLastPing(0)
    , mapLatencyTracker()
    {
    }


    /** Constructor for unauthorized outgoing connection **/
    P2PNode::P2PNode(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : BaseConnection<MessagePacket>(DDOS_IN, fDDOSIn)
    , fInitialized(false)
    , strAppID()
    , hashGenesis(0)
    , hashPeer(0)
    , nSession(0)
    , nProtocolVersion(0)
    , nLastPing(0)
    , mapLatencyTracker()
    {
        /* This constructor is not allowed as we must have the appid, source and destination genesis hashes, and sessionID 
           of the peer we are connecting to */
        throw std::runtime_error("Invalid constructor called for P2PNode");
    }


    /** Default Destructor **/
    P2PNode::~P2PNode()
    {
    }


    /** Virtual Functions to Determine Behavior of Message LLP. **/
    void P2PNode::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        switch(EVENT)
        {
            case EVENTS::CONNECT:
            {
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming", " Connection Established");

                /* Set the laset ping time to now - 15 seconds so that we get an immediate ping-ping. */
                nLastPing = runtime::unifiedtimestamp() - 15;

                /* Respond with init message if outgoing connection. */
                if(fOUTGOING)
                {
                    /* Make sure the local user is logged in before  sending the initialization message. */
                    if(!TAO::API::users->LoggedIn(hashGenesis))
                    {
                        debug::error(NODE, "User not logged in. Disconnecting.");
                        Disconnect();
                    }

                    /* Get the API session ID for the local users genesis hash. */
                    TAO::API::Session& session = TAO::API::users->GetSession(hashGenesis);

                    /* Build the byte stream from the request data in order to generate the signature */
                    DataStream ssMsgData(SER_NETWORK, P2P::PROTOCOL_VERSION);
                    ssMsgData << P2P::PROTOCOL_VERSION << strAppID << hashGenesis << hashPeer << nSession;

                    /* Public key bytes*/
                    std::vector<uint8_t> vchPubKey;
                
                    /* Signature bytes */
                    std::vector<uint8_t> vchSig;
                    
                    /* Generate signature */
                    session.GetAccount()->Sign("network", ssMsgData.Bytes(), session.GetNetworkKey(), vchPubKey, vchSig);

                    debug::log(3, NODE, "Sending P2P Initialization request."); 
                    /* Respond with initialize message. 
                       Format is protocol version, app id, hashgenesis, hashpeer, session id, pub key, and signature*/
                    PushMessage(ACTION::INITIALIZE,
                        P2P::PROTOCOL_VERSION,
                        strAppID,
                        hashGenesis,
                        hashPeer,
                        nSession,
                        vchPubKey,
                        vchSig);
                }

                break;
            }

            case EVENTS::HEADER:
            {
                /* Check for initialization. */
                if(nSession == 0 && DDOS)
                    DDOS->rSCORE += 25;

                break;
            }

            case EVENTS::PACKET:
            {
                /* Check a packet's validity once it is finished being read. */
                if(Incoming())
                {
                    /* Give higher score for Bad Packets. */
                    if(INCOMING.Complete() && !INCOMING.IsValid() && DDOS)
                        DDOS->rSCORE += 15;
                }


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
                }

                break;
            }


            case EVENTS::DISCONNECT:
            {
                /* Debut output. */
                std::string strReason;
                switch(LENGTH)
                {
                    case DISCONNECT::TIMEOUT:
                        strReason = "Timeout";
                        break;

                    case DISCONNECT::ERRORS:
                        strReason = "Errors";
                        break;

                    case DISCONNECT::POLL_ERROR:
                        strReason = "Poll Error";
                        break;

                    case DISCONNECT::POLL_EMPTY:
                        strReason = "Unavailable";
                        break;

                    case DISCONNECT::DDOS:
                        strReason = "DDOS";
                        break;

                    case DISCONNECT::FORCE:
                        strReason = "Force";
                        break;

                    case DISCONNECT::PEER:
                        strReason = "Peer Hangup";
                        break;

                    case DISCONNECT::BUFFER:
                        strReason = "Flood Control";
                        break;

                    case DISCONNECT::TIMEOUT_WRITE:
                        strReason = "Flood Control Timeout";
                        break;

                    default:
                        strReason = "Unknown";
                        break;
                }

                /* Debug output for node disconnect. */
                debug::log(1, NODE, fOUTGOING ? "Outgoing" : "Incoming",
                    " Disconnected (", strReason, ")");


                {
                    LOCK(SESSIONS_MUTEX);

                    /* Generate session key */
                    std::tuple<std::string, uint256_t, uint256_t> key = std::make_tuple(strAppID, hashGenesis, hashPeer);

                    /* Check for sessions to free. */
                    if(mapSessions.count(key))
                    {
                        /* Make sure that we aren't freeing our session if handling duplicate connections. */
                        const std::pair<uint32_t, uint32_t>& pair = mapSessions[key];
                        if(pair.first == nDataThread && pair.second == nDataIndex)
                            mapSessions.erase(key);
                    }
                }

                /* Reset session, notifications, subscriptions etc */
                nSession = 0;
                fInitialized.store(false);

                break;
            }
        }
    }


    /** Main message handler once a packet is recieved. **/
    bool P2PNode::ProcessPacket()
    {
        /* Deserialize the packeet from incoming packet payload. */
        DataStream ssPacket(INCOMING.DATA, SER_NETWORK, P2P::PROTOCOL_VERSION);

        /** Current nonce trigger. **/
        uint64_t nTriggerNonce = 0;

        /* The incoming message */
        uint16_t nMsg = INCOMING.MESSAGE;

        /* Check the message to see if it is the TRIGGER identifier */
        if(nMsg == TYPES::TRIGGER)
        {
            /* Deserialize the trigger nonce */
            ssPacket >> nTriggerNonce;

            /* Deserialize the actual message */
            ssPacket >> nMsg; 
        }

        switch(nMsg)
        {
            /* Handle for the version command. */
            case ACTION::INITIALIZE:
            {
                debug::log(3, NODE, "Initialize message received."); 
                
                /* Check for duplicate version messages. */
                if(fInitialized.load())
                    return debug::drop(NODE, "duplicate initialize message");

                /* Initialize message format is protocol version, app id, genesis, session id, pub key, and signature */

                /* Hard requirement for version. */
                ssPacket >> nProtocolVersion;

                /* Check versions. */
                if(nProtocolVersion < P2P::MIN_P2P_VERSION)
                    return debug::drop(NODE, "connection using obsolete protocol version");

                /* Get the app-id. */
                std::string strIncomingAppID;
                ssPacket >> strIncomingAppID;

                /* Get the incoming genesis. */
                uint256_t hashIncomingGenesis;
                ssPacket >> hashIncomingGenesis;

                /* Get the incoming peer. */
                uint256_t hashIncomingPeer;
                ssPacket >> hashIncomingPeer;

                /* Get the incoming session-id. */
                uint64_t nIncomingSession;
                ssPacket >> nIncomingSession;

                /* Get the public key */
                std::vector<uint8_t> vchPubKey;
                ssPacket >> vchPubKey;

                /* Get the signature */
                std::vector<uint8_t> vchSig;
                ssPacket >> vchSig;

                /* If the peer has made the connection to us, then check that the details match a p2p request that we made */
                if(Incoming())
                {
                    /* Check that the recipient of this p2p session is logged in on this node*/
                    if(!TAO::API::users->LoggedIn(hashIncomingPeer))
                        return debug::drop(NODE, "User not logged in");

                    /* Store the incoming details */
                    strAppID = strIncomingAppID;

                    /* Since this is an incoming connection, the hashGenesis and hashPeer are the opposite way around */
                    hashGenesis = hashIncomingPeer;
                    hashPeer = hashIncomingGenesis;

                    /* Store the incoming session */
                    nSession = nIncomingSession;

                }
                else
                {
                    /* If we initiated the connection then verify that their genesis matches our expected peer hash  */
                    if(hashIncomingGenesis == 0 || hashIncomingGenesis != hashPeer)
                        return debug::drop(NODE, "invalid peer hash");

                    /* If we initiated the connection then verify that their peer hash matches our genesis hash  */
                    if(hashIncomingPeer == 0 || hashIncomingPeer != hashGenesis)
                        return debug::drop(NODE, "invalid genesis hash");

                    /* Check for invalid app-id. */
                    if(strIncomingAppID.empty() || strIncomingAppID != strAppID)
                        return debug::drop(NODE, "invalid app id");

                    /* Check for invalid session-id. */
                    if(nIncomingSession == 0 || nIncomingSession != nSession)
                        return debug::drop(NODE, "invalid session id");
                }

                /* Get the local users session */
                TAO::API::Session& session = TAO::API::users->GetSession(hashGenesis);

                /* Check that we are receiving an initialization message from a peer that we are expecting.  If this is an 
                   incoming connection then we need to check that the local user has made an outgoing request that the peer 
                   is responding to.  If this is an outgoing connection then check that the peer is responding with an 
                   initialization for an incoming request */
                if(!session.HasP2PRequest(strAppID, hashPeer, !Incoming()))
                    return debug::drop(NODE, "Unsolicited P2P connection");


                /* Build the byte stream from the request data in order to verify the signature */
                DataStream ssCheck(SER_NETWORK, P2P::PROTOCOL_VERSION);
                ssCheck << nProtocolVersion << strIncomingAppID << hashIncomingGenesis << hashIncomingPeer << nIncomingSession;
                
                /* Verify the incoming signature */
                if(!TAO::Ledger::SignatureChain::Verify(hashIncomingGenesis, "network", ssCheck.Bytes(), vchPubKey, vchSig))
                    return debug::error(NODE, "ACTION::REQUEST::P2P: invalid transaction signature");


                /* Check if session is already connected. */
                {
                    /* Generate session key */
                    std::tuple<std::string, uint256_t, uint256_t> key = std::make_tuple(strAppID, hashGenesis, hashPeer);
                    
                    LOCK(SESSIONS_MUTEX);
                    if(mapSessions.count(key))
                        return debug::drop(NODE, "duplicate connection");

                    /* Set this to the current session. */
                    mapSessions[key] = std::make_pair(nDataThread, nDataIndex);
                }


                /* Respond with our own initialize message if incoming connection. */
                if(Incoming())
                {
                    debug::log(3, NODE, "Incoming peer connection verified, sending Initialization response."); 

                    /* Get the API session ID for the recipient users genesis hash.  NOTE we have already established that this user
                       is logged in, so we know we will get a valid session ID */
                    /* Get the API session ID for the local users genesis hash. */
                    TAO::API::Session& session = TAO::API::users->GetSession(hashGenesis);

                    /* Build the byte stream from the request data in order to generate the signature */
                    DataStream ssMsgData(SER_NETWORK, P2P::PROTOCOL_VERSION);
                    ssMsgData << nProtocolVersion << strAppID << hashGenesis << hashPeer << nSession;

                    /* Generate signature */
                    session.GetAccount()->Sign("network", ssMsgData.Bytes(), session.GetNetworkKey(), vchPubKey, vchSig);

                    /* Respond with initialize message. 
                       Format is protocol version, app id, hashgenesis, hashpeer, session id, pub key, and signature*/
                    PushMessage(ACTION::INITIALIZE,
                        P2P::PROTOCOL_VERSION,
                        strAppID,
                        hashGenesis,
                        hashPeer,
                        nSession,
                        vchPubKey,
                        vchSig);

                }

                /* Secure P2P connection successfully established so remove the connection request. */
                session.DeleteP2PRequest(strAppID, hashPeer, !Incoming());

                /* Store the initialized state */
                fInitialized.store(true);               

                break;
            }


            /* Handle message . */
            case TYPES::MESSAGE:
            {
                /* Check that secure connection has been established. */
                if(!fInitialized.load())
                    return debug::drop(NODE, "Message received before secure connection initialization");

                /* Message to add to the queue */
                P2P::Message message;

                /* Set the message timestamp */
                message.nTimestamp = runtime::unifiedtimestamp();

                /* Get the message data */
                ssPacket >> message.vchData;

                debug::log(3, NODE, "Message received from peer"); 

                /* Add this to the message FIFO queue */
                {
                    LOCK(MESSAGES_MUTEX);
                    queueMessages.push(message);
                }

                /* Check for trigger nonce. */
                if(nTriggerNonce != 0)
                {
                    PushMessage(RESPONSE::COMPLETED, nTriggerNonce);
                    nTriggerNonce = 0;
                }

                break;
            }


            /* Handle terminate command. */
            case ACTION::TERMINATE:
            {
                /* Echo back terminate message so both sides gracefully stop */
                PushMessage(LLP::P2P::ACTION::TERMINATE, nSession);

                return debug::drop(NODE, "Connection terminated by peer");

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
                if(fDDOS) //a ping shouldn't be sent too much
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
                    if(fDDOS)
                        DDOS->rSCORE += 10;

                    return true;
                }

                /* Calculate the Average Latency of the Connection. */
                nLatency = mapLatencyTracker[nNonce].ElapsedMilliseconds();
                mapLatencyTracker.erase(nNonce);


                /* Debug Level 3: output Node Latencies. */
                debug::log(3, NODE, "Latency (Nonce ", std::hex, nNonce, " - ", std::dec, nLatency, " ms)");

                break;
            }

            /* Handle an event trigger. */
            case RESPONSE::COMPLETED:
            {
                /* De-serialize the trigger nonce. */
                uint64_t nNonce = 0;
                ssPacket >> nNonce;
                
                TriggerEvent(nMsg, nNonce);
                
                break;
            }


           
            default:
                return debug::drop(NODE, "invalid protocol message ", nMsg);
        }

        
        /* Check for a version message. */
        if(nProtocolVersion == 0 || nSession == 0)
            return debug::drop(NODE, "first message wasn't a initialize message");

        return true;
    }

    /*  Non-Blocking Packet reader to build a packet from TCP Connection.
     *  This keeps thread from spending too much time for each Connection. */
    void P2PNode::ReadPacket()
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


    /* Determine whether a session is connected for the specified app / genesis / peer. */
    bool P2PNode::SessionActive(const std::string& strAppID, const uint256_t& hashGenesis, const uint256_t& hashPeer)
    {
        LOCK(SESSIONS_MUTEX);

        /* Generate session key */
        std::tuple<std::string, uint256_t, uint256_t> key = std::make_tuple(strAppID, hashGenesis, hashPeer);

        return mapSessions.count(key);
    }


    /* Checks to see if this connection instance matches the specified app / genesis / peer. */
    bool P2PNode::Matches(const std::string& strAppID, const uint256_t& hashGenesis, const uint256_t& hashPeer)
    {
        return strAppID == this->strAppID && hashGenesis == this->hashGenesis && hashPeer == this->hashPeer;
    }


    /* Checks to see if there are any messages in the message queue. */
    bool P2PNode::HasMessage()
    {
        LOCK(MESSAGES_MUTEX);
        return queueMessages.size() != 0;
    }


    /* Returns the number of messages in the message queue. */
    uint32_t P2PNode::MessageCount()
    {
        LOCK(MESSAGES_MUTEX);
        return queueMessages.size();
    }


    /* Returns the message at the front of the queue but leaves the message in the queue. */
    P2P::Message P2PNode::PeekMessage()
    {
        /* Return the message at the front of the queue */
        LOCK(MESSAGES_MUTEX);
        P2P::Message message; 

        /* Get the message from the front of the queue */
        if(queueMessages.size() > 0)
            message = queueMessages.front();

        /* Return the message */
        return message;
    }

    
    /* Removes the message at the front of the queue and returns it. */
    P2P::Message P2PNode::PopMessage()
    {
        /* Retrieve the message at the front of the queue */
        LOCK(MESSAGES_MUTEX);
        P2P::Message message; 
        
        /* Check that there are messages in the queue before accessing the front message */
        if(queueMessages.size() > 0)
        {
            /* Get the message from the front of the queue */
            message = queueMessages.front();

            /* Pop the queue to remove the item */
            queueMessages.pop();
        }

        /* Return the message */
        return message;
    }


}
