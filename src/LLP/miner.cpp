/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/node_cache.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/push_notification.h>
#include <LLP/types/miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/types/tritium.h>


#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/types/reservekey.h>

#include <LLC/include/flkey.h>
#include <LLC/include/random.h>
#include <LLC/include/encrypt.h>
#include <LLC/include/chacha20_helpers.h>
#include <LLC/include/mining_session_keys.h>
#include <LLC/hash/SK.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <Util/include/config.h>
#include <Util/include/convert.h>
#include <Util/include/args.h>
#include <Util/include/hex.h>

#include <cstring>


namespace LLP
{
    /* The last height that the notifications processor was run at.  This is used to ensure that events are only processed once
        across all threads when the height changes */
    std::atomic<uint32_t> Miner::nLastNotificationsHeight(0);


    /* The block iterator to act as extra nonce. */
    std::atomic<uint32_t> Miner::nBlockIterator(0);

    /* Default Constructor */
    Miner::Miner()
    : Connection()
    , tCoinbaseTx()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nHashLast(0)
    , vMinerPubKey()
    , strMinerId()
    , vAuthNonce()
    , fMinerAuthenticated(false)
    , hashGenesis(0)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    {
    }


    /* Constructor */
    Miner::Miner(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(SOCKET_IN, DDOS_IN, fDDOSIn)
    , tCoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nHashLast(0)
    , vMinerPubKey()
    , strMinerId()
    , vAuthNonce()
    , fMinerAuthenticated(false)
    , hashGenesis(0)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    {
    }


    /* Constructor */
    Miner::Miner(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(DDOS_IN, fDDOSIn)
    , tCoinbaseTx()
    , MUTEX()
    , mapBlocks()
    , nBestHeight(0)
    , nSubscribed(0)
    , nChannel(0)
    , nHashLast(0)
    , vMinerPubKey()
    , strMinerId()
    , vAuthNonce()
    , fMinerAuthenticated(false)
    , hashGenesis(0)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    {
    }


    /* Default Destructor */
    Miner::~Miner()
    {
        LOCK(MUTEX);
        clear_map();

        /* Clear authentication state on connection close */
        vMinerPubKey.clear();
        strMinerId.clear();
        vAuthNonce.clear();
        fMinerAuthenticated = false;

        /* Send a notification to wake up sleeping thread to finish shutdown process. */
        this->NotifyEvent();
    }


    /* Handle custom message events. */
    void Miner::Event(uint8_t EVENT, uint32_t LENGTH)
    {
        /* Handle any DDOS Packet Filters. */
        switch(EVENT)
        {
            /* Handle for a Packet Header Read. */
            case EVENTS::HEADER:
            {
                /* Log packet header received */
                if(Incoming())
                {
                    Packet PACKET   = this->INCOMING;
                    debug::log(1, FUNCTION, "MinerLLP: HEADER from ", GetAddress().ToStringIP(), 
                               " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec, 
                               " length=", PACKET.LENGTH);
                }

                if(fDDOS.load() && Incoming())
                {
                    Packet PACKET   = this->INCOMING;
                    if(PACKET.HEADER == BLOCK_DATA)
                        DDOS->Ban();

                    if(PACKET.HEADER == SUBMIT_BLOCK && PACKET.LENGTH > 72)
                        DDOS->Ban();

                    if(PACKET.HEADER == BLOCK_HEIGHT)
                        DDOS->Ban();

                    if(PACKET.HEADER == SET_CHANNEL && PACKET.LENGTH > 4)
                        DDOS->Ban();

                    if(PACKET.HEADER == BLOCK_REWARD)
                        DDOS->Ban();

                    if(PACKET.HEADER == SET_COINBASE && PACKET.LENGTH > 20 * 1024)
                        DDOS->Ban();

                    if(PACKET.HEADER == GOOD_BLOCK)
                        DDOS->Ban();

                    if(PACKET.HEADER == ORPHAN_BLOCK)
                        DDOS->Ban();

                    if(PACKET.HEADER == CHECK_BLOCK && PACKET.LENGTH > 128)
                        DDOS->Ban();

                    if(PACKET.HEADER == SUBSCRIBE && PACKET.LENGTH > 4)
                        DDOS->Ban();

                    if(PACKET.HEADER == BLOCK_ACCEPTED)
                        DDOS->Ban();

                    if(PACKET.HEADER == BLOCK_REJECTED)
                        DDOS->Ban();

                    if(PACKET.HEADER == COINBASE_SET)
                        DDOS->Ban();

                    if(PACKET.HEADER == COINBASE_FAIL)
                        DDOS->Ban();

                    if(PACKET.HEADER == NEW_ROUND)
                        DDOS->Ban();

                    if(PACKET.HEADER == OLD_ROUND)
                        DDOS->Ban();

                    /* Ban request opcodes that should never have payloads */
                    if(OpcodeUtility::IsHeaderOnlyRequest(PACKET.HEADER) && PACKET.LENGTH > 0)
                        DDOS->Ban();

                    /* Ban SESSION_KEEPALIVE with oversized payload
                     * Defense-in-depth: This check happens at HEADER stage (before full
                     * packet body is read), allowing immediate rejection of malicious packets.
                     * ValidatePacketLength() provides the same check later, but this early
                     * detection prevents resource allocation for obviously invalid packets. */
                    if(PACKET.HEADER == SESSION_KEEPALIVE && PACKET.LENGTH > 8)
                        DDOS->Ban();

                }
            }


            /* Handle for a Packet Data Read. */
            case EVENTS::PACKET:
                return;


            /* On Generic Event, Broadcast new block if flagged. */
            case EVENTS::GENERIC:
            {
                /* On generic events, return if no workers subscribed. */
                uint32_t count = nSubscribed.load();
                if(count == 0)
                    return;

                /* Check for a new round. */
                {
                    LOCK(MUTEX);
                    if(check_best_height())
                        return;
                }

                /* Alert workers of new round. */
                respond(NEW_ROUND);

                uint1024_t hashBlock;
                std::vector<uint8_t> vData;
                TAO::Ledger::Block *pBlock = nullptr;

                for(uint32_t i = 0; i < count; ++i)
                {
                    {
                        LOCK(MUTEX);

                        /* Create a new block */
                        pBlock = new_block();

                        /* Handle if the block failed to be created. */
                        if(!pBlock)
                        {
                            debug::log(2, FUNCTION, "Failed to create block.");
                            return;
                        }

                        /* Store the new block in the memory map of recent blocks being worked on. */
                        mapBlocks[pBlock->hashMerkleRoot] = pBlock;

                        /* Serialize the block vData */
                        vData = pBlock->Serialize();

                        /* Get the block hash for display purposes */
                        hashBlock = pBlock->GetHash();
                    }

                    /* Create and send a packet response */
                    respond(BLOCK_DATA, vData);

                    /* Debug output with channel height */
                    debug::log(2, FUNCTION, "BLOCK_DATA: channel=", pBlock->nChannel, 
                               " height=", pBlock->nHeight, " (block ", hashBlock.SubString(), ")");
                }
                return;
            }


            /* On Connect Event, Assign the Proper Daemon Handle. */
            case EVENTS::CONNECT:
            {
                /* Log connection details with remote address and port */
                debug::log(0, FUNCTION, "MinerLLP: New connection accepted from ", GetAddress().ToStringIP(), ":", GetAddress().GetPort());

                try
                {
                    /* Cache the last transaction ID of the sig chain so that we can detect if
                       new transactions enter the mempool for this sig chain. */
                    LLD::Ledger->ReadLast(TAO::API::Authentication::Caller(), nHashLast, TAO::Ledger::FLAGS::MEMPOOL);

                    /* Debug output. */
                    debug::log(3, FUNCTION, "Session found for miner connection from ", GetAddress().ToStringIP());
                }
                catch(const TAO::API::Exception& e)
                {
                    /* All connections now use stateless protocol (no TAO API session required) */
                    /* This is the new standard for all mining connections */
                    
                    /* Log connection established message */
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner connection from ", 
                               GetAddress().ToStringIP(), ":", GetAddress().GetPort());
                    debug::log(0, FUNCTION, "  Mode: Stateless (Falcon-based authentication)");
                    debug::log(0, FUNCTION, "  Remote mining: ENABLED");
                }

                return;
            }


            /* On Disconnect Event, Reduce the Connection Count for Daemon */
            case EVENTS::DISCONNECT:
            {
                /* Classify disconnect reason as network or software */
                uint32_t reason = LENGTH;
                std::string strReason;
                std::string strCategory;

                switch(reason)
                {
                    /* Network disconnects - real TCP/connection failures */
                    case DISCONNECT::TIMEOUT:
                        strReason = "DISCONNECT::TIMEOUT (socket read idle)";
                        strCategory = "NETWORK";
                        break;
                    case DISCONNECT::ERRORS:
                        strReason = "DISCONNECT::ERRORS (socket I/O error)";
                        strCategory = "NETWORK";
                        break;
                    case DISCONNECT::POLL_ERROR:
                        strReason = "DISCONNECT::POLL_ERROR (poll failure)";
                        strCategory = "NETWORK";
                        break;
                    case DISCONNECT::POLL_EMPTY:
                        strReason = "DISCONNECT::POLL_EMPTY (EOF from remote)";
                        strCategory = "NETWORK";
                        break;
                    case DISCONNECT::PEER:
                        strReason = "DISCONNECT::PEER (remote closed connection)";
                        strCategory = "NETWORK";
                        break;

                    /* Software disconnects - local policy decisions */
                    case DISCONNECT::DDOS:
                        strReason = "DISCONNECT::DDOS (rate limit exceeded)";
                        strCategory = "SOFTWARE";
                        break;
                    case DISCONNECT::FORCE:
                        strReason = "DISCONNECT::FORCE (server shutdown)";
                        strCategory = "SOFTWARE";
                        break;
                    case DISCONNECT::BUFFER:
                        strReason = "DISCONNECT::BUFFER (send buffer overflow)";
                        strCategory = "SOFTWARE";
                        break;
                    case DISCONNECT::TIMEOUT_WRITE:
                        strReason = "DISCONNECT::TIMEOUT_WRITE (write stall)";
                        strCategory = "SOFTWARE";
                        break;
                    default:
                        strReason = "UNKNOWN";
                        strCategory = "UNKNOWN";
                        break;
                }
                debug::log(0, FUNCTION, "[", strCategory, "] Disconnecting ", GetAddress().ToStringIP(), " (", strReason, ")");
                return;
            }
        }

    }


    /* Helper function to get packet name for logging - delegates to OpcodeUtility */
    static std::string GetMinerPacketName(uint8_t header)
    {
        return OpcodeUtility::GetOpcodeName(header);
    }


    /* This function is necessary for a template LLP server. It handles a custom message system, interpreting from raw packets. */
    bool Miner::ProcessPacket()
    {
        try
        {
            /* Get the incoming packet */
            Packet PACKET = this->INCOMING;

            /* Log incoming packet */
            debug::log(1, FUNCTION, "MinerLLP: ProcessPacket from ", GetAddress().ToStringIP(),
                       " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                       " (", OpcodeUtility::GetOpcodeName(PACKET.HEADER), ")",
                       " length=", PACKET.LENGTH);

            /* Strict legacy lane enforcement (port 8323):
             * 0xD0 is a valid 8-bit auth opcode here (MINER_AUTH_CHALLENGE), not a stateless prefix.
             * Large payloads on this opcode indicate a wrong-lane stateless frame. */
            /* Legacy MINER_AUTH_CHALLENGE max payload is 40 bytes (4-byte length + 32-byte nonce + 4-byte padding).
             * Anything larger on opcode 0xD0 is likely a misframed stateless 16-bit header (0xD0xx). */
            constexpr uint32_t MAX_LEGACY_AUTH_CHALLENGE_LENGTH = 40;
            if(PACKET.HEADER == MINER_AUTH_CHALLENGE &&
               PACKET.LENGTH > MAX_LEGACY_AUTH_CHALLENGE_LENGTH)
            {
                debug::error(FUNCTION, "Wrong protocol lane on legacy mining port (expected 8-bit framing)");
                debug::error(FUNCTION, "Rejecting stateless-framed packet from ", GetAddress().ToStringIP());
                Disconnect();
                return false;
            }

            /* Validate packet length using opcode utility */
            std::string strLengthReason;
            if(!OpcodeUtility::ValidatePacketLength(PACKET, &strLengthReason))
            {
                debug::error(FUNCTION, "MinerLLP: Packet length validation failed from ", 
                           GetAddress().ToStringIP(), ": ", strLengthReason);
                Disconnect();
                return false;
            }

            /* Handle SESSION_KEEPALIVE (212 / 0xD4) locally to refresh cache expiry */
            if(PACKET.HEADER == SESSION_KEEPALIVE)
            {
                debug::log(2, FUNCTION, "════════════════════════════════════");
                debug::log(2, FUNCTION, "SESSION_KEEPALIVE received");

                uint32_t nKeepaliveSession = 0;
                if(PACKET.DATA.size() >= 4)
                {
                    nKeepaliveSession =
                        static_cast<uint32_t>(PACKET.DATA[0]) |
                        (static_cast<uint32_t>(PACKET.DATA[1]) << 8) |
                        (static_cast<uint32_t>(PACKET.DATA[2]) << 16) |
                        (static_cast<uint32_t>(PACKET.DATA[3]) << 24);
                }
                else if(nSessionId != 0)
                {
                    nKeepaliveSession = nSessionId;
                    debug::log(2, FUNCTION, "SESSION_KEEPALIVE missing session ID, using connection session");
                }
                else
                {
                    debug::error(FUNCTION, "Invalid SESSION_KEEPALIVE length");
                    return true;
                }

                debug::log(2, FUNCTION, "Session ID: 0x", std::hex, nKeepaliveSession, std::dec);

                auto optContext = StatelessMinerManager::Get().GetMinerContextBySessionID(nKeepaliveSession);
                if(!optContext.has_value())
                {
                    debug::error(FUNCTION, "Unknown session_id: 0x", std::hex, nKeepaliveSession, std::dec);
                    debug::error(FUNCTION, "Session may have expired or never existed");
                    Disconnect();
                    return true;
                }

                MiningContext sessionContext = optContext.value();
                uint64_t nNow = runtime::unifiedtimestamp();
                const uint32_t nExpirySeconds =
                    static_cast<uint32_t>(NodeCache::GetPurgeTimeout(sessionContext.strAddress));

                sessionContext = sessionContext
                    .WithTimestamp(nNow)
                    .WithSessionTimeout(nExpirySeconds)
                    .WithKeepaliveCount(sessionContext.nKeepaliveCount + 1);

                auto optLane = StatelessMinerManager::Get().GetMinerLane(sessionContext.strAddress);
                StatelessMinerManager::Get().UpdateMiner(
                    sessionContext.strAddress,
                    sessionContext,
                    optLane.value_or(0));

                if(sessionContext.fAuthenticated && sessionContext.hashKeyID != 0)
                    SessionRecoveryManager::Get().SaveSession(sessionContext);

                /* Reset connection activity timer to prevent idle disconnection */
                this->Reset();

                constexpr uint64_t SECONDS_PER_DAY = 86400;
                debug::log(2, FUNCTION, "Session refreshed:");
                debug::log(2, FUNCTION, "  Keepalives: ", sessionContext.nKeepaliveCount);
                debug::log(2, FUNCTION, "  New expiry: ", (nNow + nExpirySeconds), " (",
                           (nExpirySeconds / SECONDS_PER_DAY), "d)");

                std::vector<uint8_t> vResponse(4);
                vResponse[0] = static_cast<uint8_t>(nExpirySeconds & 0xFF);
                vResponse[1] = static_cast<uint8_t>((nExpirySeconds >> 8) & 0xFF);
                vResponse[2] = static_cast<uint8_t>((nExpirySeconds >> 16) & 0xFF);
                vResponse[3] = static_cast<uint8_t>((nExpirySeconds >> 24) & 0xFF);

                respond(SESSION_KEEPALIVE, vResponse);

                debug::log(2, FUNCTION, "Sent SESSION_KEEPALIVE response");
                debug::log(2, FUNCTION, "════════════════════════════════════");

                return true;
            }

            /* Route ALL stateless mining packets to StatelessMiner.
             * 
             * ARCHITECTURAL NOTE:
             * This Miner class is a THIN WRAPPER for backward compatibility.
             * All stateless mining packets are routed to StatelessMiner processor.
             * 
             * Stateless mining uses:
             *   - MiningContext for state (not TAO API sessions)
             *   - hashRewardAddress for payouts (not hashGenesis)  
             *   - Falcon signatures for auth (not username/password)
             * 
             * Packet categories routed to StatelessMiner:
             *   - Authentication: 207, 208, 209, 210 (Falcon challenge-response)
             *   - Session: 211, 212 (optional session management)
             *   - Configuration: 3, 206 (channel selection + ack)
             *   - Rewards: 213, 214 (reward address binding)
             *   - Mining: 0, 1, 129, 200, 201 (block operations)
             *   - Info: 130 (height polling)
             *   - Notifications: 216 (push notification subscription)
             */
            if(PACKET.HEADER == BLOCK_DATA ||             // 0   - node → miner: Block template
               PACKET.HEADER == SUBMIT_BLOCK ||           // 1   - miner → node: Submit solution
               PACKET.HEADER == SET_CHANNEL ||            // 3   - miner → node: Set channel
               PACKET.HEADER == GET_BLOCK ||              // 129 - miner → node: Request template
               PACKET.HEADER == GET_HEIGHT ||             // 130 - miner → node: Request height
               PACKET.HEADER == BLOCK_ACCEPTED ||         // 200 - node → miner: Block accepted
               PACKET.HEADER == BLOCK_REJECTED ||         // 201 - node → miner: Block rejected
               PACKET.HEADER == CHANNEL_ACK ||            // 206 - node → miner: Channel ack (has data!)
               PACKET.HEADER == MINER_AUTH_INIT ||        // 207 - miner → node: Start auth
               PACKET.HEADER == MINER_AUTH_CHALLENGE ||   // 208 - node → miner: Challenge
               PACKET.HEADER == MINER_AUTH_RESPONSE ||    // 209 - miner → node: Signature
               PACKET.HEADER == MINER_AUTH_RESULT ||      // 210 - node → miner: Auth result
               PACKET.HEADER == SESSION_START ||          // 211 - Session init
               PACKET.HEADER == SESSION_KEEPALIVE ||      // 212 - Session keepalive
               PACKET.HEADER == MINER_SET_REWARD ||       // 213 - miner → node: Set reward address
               PACKET.HEADER == MINER_REWARD_RESULT ||    // 214 - node → miner: Reward result
               PACKET.HEADER == MINER_READY)              // 216 - miner → node: Subscribe to push notifications
            {
                /* Build MiningContext from current connection state */
                std::string strAddress = GetAddress().ToStringIP() + ":" + std::to_string(GetAddress().GetPort());
                
                MiningContext context(
                    nChannel,                      // Current channel
                    nBestHeight,                   // Current height
                    runtime::unifiedtimestamp(),   // Current timestamp
                    strAddress,                    // Connection address
                    1,                             // Protocol version
                    fMinerAuthenticated,           // Authentication state
                    nSessionId,                    // Session ID
                    uint256_t(0),                  // hashKeyID (set to 0, will be derived in StatelessMiner from pubkey)
                    hashGenesis                    // Genesis hash
                );

                /* Add authentication nonce if present */
                if(!vAuthNonce.empty())
                    context = context.WithNonce(vAuthNonce);

                /* Add miner public key if present */
                if(!vMinerPubKey.empty())
                    context = context.WithPubKey(vMinerPubKey);

                /* Add reward address if bound */
                if(fRewardBound)
                    context = context.WithRewardAddress(hashRewardAddress);

                /* Add ChaCha20 key if encryption is ready
                 * CRITICAL: This ensures the context sent to StatelessMiner includes
                 * the encryption key derived from genesis. Without this, the context
                 * in StatelessMinerManager will be missing encryption state. */
                if(fEncryptionReady && !vChaChaKey.empty())
                    context = context.WithChaChaKey(vChaChaKey);

                /* Route ALL authentication/session packet processing to StatelessMiner */
                ProcessResult result = StatelessMiner::ProcessPacket(context, PACKET);

                /* Legacy lane response transmission.
                 * StatelessMiner::ProcessPacket() returns 16-bit stateless opcodes (0xD0xx).
                 * Legacy lane expects 8-bit opcodes, so we unmirror and convert. */

                /* Handle result */
                if(result.fSuccess)
                {
                    /* Reset connection activity timer to prevent idle disconnection */
                    this->Reset();
                    
                    /* Update connection state from result context */
                    nChannel = result.context.nChannel;
                    nBestHeight = result.context.nHeight;
                    fMinerAuthenticated = result.context.fAuthenticated;
                    nSessionId = result.context.nSessionId;
                    hashGenesis = result.context.hashGenesis;
                    
                    /* Update authentication state */
                    if(!result.context.vAuthNonce.empty())
                        vAuthNonce = result.context.vAuthNonce;
                    
                    if(!result.context.vMinerPubKey.empty())
                        vMinerPubKey = result.context.vMinerPubKey;

                    /* Update reward address if bound */
                    if(result.context.fRewardBound)
                    {
                        hashRewardAddress = result.context.hashRewardAddress;
                        fRewardBound = true;
                    }

                    /* Derive ChaCha20 key from genesis using unified helper */
                    if(hashGenesis != 0 && !fEncryptionReady)
                    {
                        vChaChaKey = LLC::MiningSessionKeys::DeriveChaCha20Key(hashGenesis);
                        fEncryptionReady = true;
                    }

                    /* Update the context with the ChaCha20 key before sending to manager.
                     * CRITICAL: The context returned from StatelessMiner may not include
                     * the ChaCha20 key in all cases:
                     * 1. If it was just derived above, result.context won't have it
                     * 2. If encryption was already ready, result.context should have it from line 452
                     * We always add/update it here to ensure StatelessMinerManager has complete state. */
                    MiningContext updatedContext = result.context;
                    if(fEncryptionReady && !vChaChaKey.empty())
                    {
                        updatedContext = updatedContext.WithChaChaKey(vChaChaKey);
                    }

                    if(updatedContext.fAuthenticated && updatedContext.nSessionId != 0)
                    {
                        uint64_t nNow = runtime::unifiedtimestamp();
                        const uint32_t nExpirySeconds =
                            static_cast<uint32_t>(NodeCache::GetPurgeTimeout(updatedContext.strAddress));

                        if(updatedContext.nSessionStart == 0)
                            updatedContext = updatedContext.WithSessionStart(nNow);

                        updatedContext = updatedContext.WithSessionTimeout(nExpirySeconds);
                    }

                    /* Persist session and lane state for cross-lane recovery */
                    if(updatedContext.fAuthenticated && updatedContext.hashKeyID != 0)
                    {
                        SessionRecoveryManager::Get().SaveSession(updatedContext);
                        SessionRecoveryManager::Get().UpdateLane(updatedContext.hashKeyID, 0);
                        if(fEncryptionReady && !vChaChaKey.empty())
                        {
                            uint256_t hashKey(vChaChaKey);
                            SessionRecoveryManager::Get().SaveChaCha20State(
                                updatedContext.hashKeyID,
                                hashKey,
                                0
                            );
                        }
                    }

                    /* Update StatelessMinerManager with COMPLETE context including encryption state */
                    StatelessMinerManager::Get().UpdateMiner(updatedContext.strAddress, updatedContext, 0);

                    /* Send response if present */
                    if(!result.response.IsNull())
                    {
                        /* LEGACY LANE (port 8323): Unmirror 16-bit → 8-bit */
                        uint16_t nResponseHeader = result.response.HEADER;
                        if(OpcodeUtility::Stateless::IsStateless(nResponseHeader))
                        {
                            nResponseHeader = OpcodeUtility::Stateless::Unmirror(nResponseHeader);
                            debug::log(2, FUNCTION, "Legacy lane: Unmirror 0x",
                                      std::hex, uint32_t(result.response.HEADER),
                                      " → 0x", nResponseHeader, std::dec);
                        }

                        /* Convert StatelessPacket response to legacy Packet for writing */
                        Packet legacyResponse;
                        legacyResponse.HEADER = static_cast<uint8_t>(nResponseHeader);
                        legacyResponse.LENGTH = result.response.LENGTH;
                        legacyResponse.DATA = result.response.DATA;

                        WritePacket(legacyResponse);
                    }

                    return true;
                }
                else
                {
                    if(result.strError.find("Unknown packet type") != std::string::npos)
                    {
                        /* Mining operations (GET_BLOCK, SUBMIT_BLOCK, GET_ROUND, etc.) are
                         * delegated by StatelessMiner to the connection layer.
                         * Fall through to ProcessPacketStateless which handles these. */
                        debug::log(2, FUNCTION, "StatelessMiner delegated opcode 0x",
                                   std::hex, uint32_t(PACKET.HEADER), std::dec,
                                   " to ProcessPacketStateless");
                        return ProcessPacketStateless(PACKET);
                    }
                    
                    /* Processing error - log and disconnect */
                    debug::error(FUNCTION, "MinerLLP: Processing error from ", GetAddress().ToStringIP(),
                                ": ", result.strError);
                    
                    /* Try to send error response if available */
                    if(!result.response.IsNull())
                    {
                        /* Legacy lane: unmirror and convert */
                        uint16_t nErrHeader = result.response.HEADER;
                        if(OpcodeUtility::Stateless::IsStateless(nErrHeader))
                        {
                            nErrHeader = OpcodeUtility::Stateless::Unmirror(nErrHeader);
                            debug::log(2, FUNCTION, "Legacy lane: Unmirror error 0x",
                                      std::hex, uint32_t(result.response.HEADER),
                                      " → 0x", nErrHeader, std::dec);
                        }

                        /* Convert StatelessPacket response to legacy Packet for writing */
                        Packet legacyResponse;
                        legacyResponse.HEADER = static_cast<uint8_t>(nErrHeader);
                        legacyResponse.LENGTH = result.response.LENGTH;
                        legacyResponse.DATA = result.response.DATA;

                        WritePacket(legacyResponse);
                    }
                    
                    this->Disconnect();
                    return false;
                }
            }

            /* All other packets (block operations, PING, etc.) handled by ProcessPacketStateless */
            return ProcessPacketStateless(PACKET);
        }
        catch(const std::exception& e)
        {
            /* Get the error message */
            std::string strError = e.what();
            
            /* Suppress "Session not found" errors for stateless mining.
             * These exceptions are thrown by legacy TAO API code that expects
             * user sessions (login/create/unlock), but stateless mining uses
             * MiningContext tracked by StatelessMinerManager instead.
             * 
             * Common sources:
             * - new_block() trying to access TAO::API::Authentication::Credentials()
             * - new_block() calling TAO::API::Authentication::Unlock() 
             * - Legacy mining code expecting session-based authentication
             * - TAO API methods called during block template creation
             * 
             * This is safe to suppress because:
             * 1. Stateless mining has its own auth (Falcon challenge-response)
             * 2. State is tracked in StatelessMinerManager via MiningContext
             * 3. No TAO API sessions are needed for mining operations
             */
            if(strError.find("Session not found") != std::string::npos)
            {
                debug::log(2, FUNCTION, "MinerLLP: Session error suppressed for stateless mining from ", GetAddress().ToStringIP(), " - stateless protocol uses MiningContext, not TAO API sessions");
                
                /* Return true to continue processing - the packet has been handled,
                 * we just encountered legacy code trying to access non-existent sessions.
                 * For stateless mining, this is expected and not an error. */
                return true;
            }
            
            /* For all other exceptions, maintain existing behavior: log and disconnect */
            debug::error(FUNCTION, "MinerLLP: Exception in ProcessPacket from ", GetAddress().ToStringIP(),
                        ": ", e.what());
            this->Disconnect();
            return false;
        }
    }


    /* Handles packets from stateless localhost miners without TAO API session. */
    bool Miner::ProcessPacketStateless(const Packet& PACKET)
    {
        /* Log entry to stateless handler */
        debug::log(0, FUNCTION, "MinerLLP: STATELESS ProcessPacket handler for ", GetAddress().ToStringIP(),
                   " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                   " length=", PACKET.LENGTH);

        /* Compute local testnet flag (session-free check) */
        bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

        /* Get current Tritium peer connection count (session-free) */
        uint16_t nConnections = (TRITIUM_SERVER ? TRITIUM_SERVER->GetConnectionCount() : 0);

        /* Check network connections (skip if local testnet) */
        if(!fLocalTestnet && nConnections == 0)
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=NO_NETWORK (stateless) fLocalTestnet=", fLocalTestnet,
                       " nConnections=", nConnections);
            return debug::error(FUNCTION, "No network connections.");
        }

        /* Check if synchronizing */
        if(TAO::Ledger::ChainState::Synchronizing())
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=SYNCHRONIZING (stateless)");
            return debug::error(FUNCTION, "Cannot mine while ledger is synchronizing.");
        }

        /* Check if wallet is locked */
        if(is_locked())
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=WALLET_LOCKED (stateless)");
            return debug::error(FUNCTION, "Cannot mine while wallet is locked.");
        }

        /* Log that we've reached the switch statement */
        debug::log(0, FUNCTION, "MinerLLP: >>> STATELESS REACHED_SWITCH with header=0x", std::hex, 
                   uint32_t(PACKET.HEADER), std::dec, " length=", PACKET.LENGTH);

        /* Reset connection activity timer to prevent idle disconnection on any packet processing */
        this->Reset();

        /* Evaluate the packet header to determine what to do. */
        switch(PACKET.HEADER)
        {
            /* NOTE: Authentication and session management packets are routed to StatelessMiner
             * in ProcessPacket() above via the MINER_AUTH_*, SESSION_*, SET_CHANNEL, and 
             * MINER_SET_REWARD checks. They will never reach this switch statement. */

            /* Return a Ping if Requested. */
            case PING:
            {
                debug::log(3, FUNCTION, "PING received from ", GetAddress().ToStringIP());
                respond(PING);
                return true;
            }


            /* Clear the Block Map if Requested by Client. */
            case CLEAR_MAP:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted CLEAR_MAP before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                LOCK(MUTEX);
                clear_map();
                return true;
            }


            /* MINER_READY (216 / 0xD8) - Subscribe to Push Notifications
             * 
             * Miner sends this after authentication to subscribe to channel-specific
             * push notifications instead of polling GET_ROUND.
             */
            case MINER_READY:
            {
                return handle_miner_ready_stateless();
            }


            /* Respond to the miner with the new height. */
            case GET_HEIGHT:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_HEIGHT before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                {
                    /* Check the best height before responding. */
                    LOCK(MUTEX);
                    check_best_height();
                }

                debug::log(0, FUNCTION, "GET_HEIGHT request from ", GetAddress().ToStringIP(), 
                           " - responding with height ", nBestHeight + 1);

                /* Create the response packet and write. */
                respond(BLOCK_HEIGHT, convert::uint2bytes(nBestHeight + 1));

                return true;
            }


            /* Respond to a miner if it is a new round. */
            case GET_ROUND:
            {
                return handle_get_round_stateless();
            }


            /* Respond with the block reward in a given round. */
            case GET_REWARD:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_REWARD before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                debug::log(2, FUNCTION, "GET_REWARD request from ", GetAddress().ToStringIP());

                /* Get the mining reward amount for the channel currently set. */
                uint64_t nReward = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::tStateBest.load(), nChannel.load(), 0);

                /* Check to make sure the reward is greater than zero. */
                if(nReward == 0)
                    return debug::error(FUNCTION, "No coinbase reward.");

                /* Respond with BLOCK_REWARD message. */
                respond(BLOCK_REWARD, convert::uint2bytes64(nReward));

                /* Debug output. */
                debug::log(2, FUNCTION, "Sent Coinbase Reward of ", nReward);
                return true;
            }


            /* Set the number of subscribed blocks. */
            case SUBSCRIBE:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted SUBSCRIBE before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                /* Don't allow mining llp requests for proof of stake channel */
                if(nChannel.load() == 0)
                    return debug::error(FUNCTION, "Cannot subscribe to Stake Channel");

                /* Get the number of subscribed blocks. */
                nSubscribed = convert::bytes2uint(PACKET.DATA);

                /* Check for zero blocks. */
                if(nSubscribed.load() == 0)
                    return debug::error(FUNCTION, "No blocks subscribed");

                /* Debug output. */
                debug::log(2, FUNCTION, "Subscribed to ", nSubscribed.load(), " Blocks");
                return true;
            }


            /* Get a new block for the miner. */
            case GET_BLOCK:
            {
                return handle_get_block_stateless();
            }


            /* Submit a block using the merkle root as the key. */
            case SUBMIT_BLOCK:
            {
                return handle_submit_block_stateless(PACKET);
            }


            /* Allows a client to check if a block is part of the main chain. */
            case CHECK_BLOCK:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted CHECK_BLOCK before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                uint1024_t hashBlock;
                TAO::Ledger::BlockState state;

                /* Extract the block hash. */
                hashBlock.SetBytes(PACKET.DATA);

                /* Read the block state from disk. */
                if(LLD::Ledger->ReadBlock(hashBlock, state))
                {
                    /* If the block state is not in main chain, send a orphan response. */
                    if(!state.IsInMainChain())
                    {
                        respond(ORPHAN_BLOCK, PACKET.DATA);
                        return true;
                    }
                }

                /* Block state is in the main chain, send a good response */
                respond(GOOD_BLOCK, PACKET.DATA);
                return true;
            }

            /* Fallback for unknown commands - log and return error */
            default:
                debug::log(0, FUNCTION, "MinerLLP: COMMAND NOT FOUND (stateless) header=0x", std::hex, 
                           uint32_t(PACKET.HEADER), std::dec, " length=", PACKET.LENGTH);
                return debug::error(FUNCTION, "Command not found 0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
        }
    }


    /* Sends a packet response. */
    void Miner::respond(uint8_t nHeader, const std::vector<uint8_t>& vData)
    {
        Packet RESPONSE;

        RESPONSE.HEADER = nHeader;
        RESPONSE.LENGTH = vData.size();
        RESPONSE.DATA   = vData;

        /* Log outgoing response with packet name */
        debug::log(1, FUNCTION, "MinerLLP: RESPOND to ", GetAddress().ToStringIP(), 
                   " - ", GetMinerPacketName(nHeader), " (0x", std::hex, uint32_t(nHeader), std::dec, ")",
                   " length=", vData.size());

        this->WritePacket(RESPONSE);
    }


    /* For Tritium, this checks the mempool to make sure that there are no new transactions that would be orphaned */
    bool Miner::check_round()
    {
        /* All mining connections now use stateless protocol - skip session-dependent checks */
        return true;
    }


    /* Checks the current height index and updates best height. Clears the block map if the height is outdated or stale. */
    bool Miner::check_best_height()
    {
        uint32_t nChainStateHeight = TAO::Ledger::ChainState::nBestHeight.load();

        /* Introduced as part of Tritium upgrade. We can't rely on existing mining software to use the GET_ROUND to check that the
           the current round is still valid, so we additionally check the round whenever the height is checked.  If we find that it
           is not valid, the only way we can force miners to request new block data is to send through a height change. So here we
           set the height to 0 which will trigger miners to stop and request new block data, and then immediately the height will be
           set to the correct height again so they can carry on with the new block data.
           NOTE: there is no need to check the round if the height has changed as this obviously  will result in a new block*/
        if(nBestHeight == nChainStateHeight && !check_round())
        {
            /* Set the height temporarily to 0 */
            nBestHeight = 0;

            return true;
        }

        /* Return early if the height doesn't change. */
        if(nBestHeight == nChainStateHeight)
            return false;

        /* Clear the map of blocks if a new block has been accepted. */
        clear_map();

        /* Set the new best height. */
        nBestHeight = nChainStateHeight;
        debug::log(2, FUNCTION, "Mining best height changed to ", nBestHeight);

        /* Notify StatelessMinerManager of new round for unified miner-node interaction.
         * This enables seamless template distribution to all connected stateless miners. */
        StatelessMinerManager::Get().NotifyNewRound(nBestHeight);

        /* make sure the notifications processor hasn't been run already at this height */
        if(nLastNotificationsHeight.load() != nBestHeight)
        {
            /* Store our new height now. */
            nLastNotificationsHeight.store(nBestHeight);

            /* All mining connections use stateless protocol - no session-dependent operations needed */
        }

        return true;
    }


    /*  Clear the blocks map. */
    void Miner::clear_map()
    {
        /* Delete the dynamically allocated blocks in the map. */
        for(auto &block : mapBlocks)
        {
            if(block.second)
                delete block.second;
        }
        mapBlocks.clear();

        /* Reset the coinbase transaction. */
        tCoinbaseTx.SetNull();

        /* Set the block iterator back to zero so we can iterate new blocks next round. */
        nBlockIterator = 0;

        /* NOTE: Authentication state is NOT cleared here on purpose.
         * The miner remains authenticated across round changes. Authentication
         * is only invalidated when the connection closes (see destructor ~Miner())
         * where auth state is explicitly cleared.
         * 
         * This ensures SOLO miners don't get de-authenticated during normal
         * mining operations when the block height changes or a new round starts.
         *
         * Previously, clearing auth state here caused SOLO mining issues where
         * authenticated miners would be rejected after any block height change.
         */

        debug::log(3, FUNCTION, "Cleared map of blocks");
    }


    /*  Determines if the block exists. */
    bool Miner::find_block(const uint512_t& hashMerkleRoot)
    {
        /* Check that the block exists. */
        if(!mapBlocks.count(hashMerkleRoot))
        {
            debug::log(2, FUNCTION, "Block Not Found ", hashMerkleRoot.SubString());

            return false;
        }

        return true;
    }


    /*  Adds a new block to the map. */
    TAO::Ledger::Block *Miner::new_block()
    {
        /* Determine reward address - priority order:
         * 1. hashRewardAddress (explicit via MINER_SET_REWARD)
         * 2. hashGenesis (fallback from Falcon auth - PR #92)
         * 3. Wallet genesis (legacy TAO API mode)
         */
        uint256_t hashReward = 0;
        
        if(fRewardBound && hashRewardAddress != 0) {
            hashReward = hashRewardAddress;
            debug::log(2, FUNCTION, "Reward: explicit address");
        }
        else if(hashGenesis != 0) {
            hashReward = hashGenesis;
            debug::log(2, FUNCTION, "Reward: genesis fallback");
        }
        else {
            // Wallet mode fallback (legacy)
            try {
                SecureString strPIN;
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
                const auto& pCredentials = TAO::API::Authentication::Credentials();
                hashReward = pCredentials->Genesis();
                debug::log(2, FUNCTION, "Reward: wallet genesis");
            }
            catch(const std::exception& e) {
                debug::error(FUNCTION, "No reward address available: ", e.what());
                return nullptr;
            }
        }
        
        /* Prime channel optimization */
        const uint32_t nBitMask = config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;
        TAO::Ledger::TritiumBlock* pBlock = nullptr;
        
        /* Create block using simplified utility */
        while(true) {
            pBlock = TAO::Ledger::CreateBlockForStatelessMining(
                nChannel.load(),
                ++nBlockIterator,
                hashReward
            );
            
            if(!pBlock) return nullptr;
            if(is_prime_mod(nBitMask, pBlock)) break;
            
            delete pBlock;
            pBlock = nullptr;
        }
        
        debug::log(2, FUNCTION, "Created block ", pBlock->ProofHash().SubString());
        return pBlock;
    }


    /*  signs the block. */
    bool Miner::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
    {
        TAO::Ledger::Block *pBaseBlock = mapBlocks[hashMerkleRoot];

        /* Update block with the nonce and time. */
        if(pBaseBlock)
            pBaseBlock->nNonce = nNonce;

        /* If the block dynamically casts to a legacy block, validate the legacy block. */
        {
            Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(pBaseBlock);
            if(pBlock)
            {
                #ifndef NO_WALLET

                /* Update the block's timestamp. */
                pBlock->UpdateTime();

                /* Sign the block with a key from wallet. */
                if(!Legacy::SignBlock(*pBlock, Legacy::Wallet::Instance()))
                    return debug::error(FUNCTION, "Unable to Sign Legacy Block ", hashMerkleRoot.SubString());

                #endif

                return true;
            }
        }

        /* If the block dynamically casts to a tritium block, validate the tritium block. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(pBaseBlock);
        if(pBlock)
        {
            /* Update the block's timestamp. */
            pBlock->UpdateTime();

            /* Calculate prime offsets before signing. */
            TAO::Ledger::GetOffsets(pBlock->GetPrime(), pBlock->vOffsets);

            /* Unlock sigchain to create new block. */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));

            /* Get an instance of our credentials. */
            const auto& pCredentials =
                TAO::API::Authentication::Credentials(uint256_t(TAO::API::Authentication::SESSION::DEFAULT));

            /* Generate a new sigchain key for signing. */
            std::vector<uint8_t> vBytes = pCredentials->Generate(pBlock->producer.nSequence, strPIN).GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(pBlock->producer.nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case TAO::Ledger::SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret))
                        return debug::error(FUNCTION, "FLKey::SetSecret failed for ", hashMerkleRoot.SubString());

                    /* Generate the signature. */
                    if(!pBlock->GenerateSignature(key))
                        return debug::error(FUNCTION, "GenerateSignature failed for Tritium Block ", hashMerkleRoot.SubString());

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret, true))
                        return debug::error(FUNCTION, "ECKey::SetSecret failed for ", hashMerkleRoot.SubString());

                    /* Generate the signature. */
                    if(!pBlock->GenerateSignature(key))
                        return debug::error(FUNCTION, "GenerateSignature failed for Tritium Block ", hashMerkleRoot.SubString());

                    break;
                }

                default:
                    return debug::error(FUNCTION, "Unknown signature type");
            }

            return true;
        }

        /* If we get here, the block is null or doesn't exist. */
        return debug::error(FUNCTION, "null block");
    }


    /*  validates the block. */
    bool Miner::validate_block(const uint512_t& hashMerkleRoot)
    {
        /* If the block dynamically casts to a legacy block, validate the legacy block. */
        {
            Legacy::LegacyBlock *pBlock = dynamic_cast<Legacy::LegacyBlock *>(mapBlocks[hashMerkleRoot]);

            if(pBlock)
            {
                debug::log(2, FUNCTION, "Legacy");
                pBlock->print();

                #ifndef NO_WALLET

                /* Check the Proof of Work for submitted block. */
                if(!Legacy::CheckWork(*pBlock, Legacy::Wallet::Instance()))
                    return false;

                #endif

                return true;
            }
        }

        /* If the block dynamically casts to a tritium block, validate the tritium block. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock*>(mapBlocks[hashMerkleRoot]);
        if(pBlock)
        {
            debug::log(2, FUNCTION, "Tritium");
            pBlock->print();

            /* Log block found */
            if(config::nVerbose > 0)
            {
                std::string strTimestamp(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));
                if(pBlock->nChannel == 1)
                    debug::log(1, FUNCTION, "new prime block found at unified time ", strTimestamp);
                else
                    debug::log(1, FUNCTION, "new hash block found at unified time ", strTimestamp);
            }

            const TAO::Ledger::BlockValidationResult validationResult =
                TAO::Ledger::ValidateMinedBlock(*pBlock);
            if(!validationResult.valid)
            {
                debug::error(FUNCTION, "ValidateMinedBlock failed: ", validationResult.reason);
                return false;
            }

            const TAO::Ledger::BlockAcceptanceResult acceptanceResult =
                TAO::Ledger::AcceptMinedBlock(*pBlock);
            if(!acceptanceResult.accepted)
            {
                debug::error(FUNCTION, "AcceptMinedBlock failed: ", acceptanceResult.reason);
                return false;
            }

            return true;
        }

        /* If we get here, the block is null or doesn't exist. */
        return false;
    }


    /*  Determines if the mining wallet is unlocked. */
    bool Miner::is_locked()
    {
        return !TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING);
    }


   /*  Helper function used for prime channel modification rule in loop.
    *  Returns true if the condition is satisfied, false otherwise. */
    bool Miner::is_prime_mod(uint32_t nBitMask, TAO::Ledger::Block *pBlock)
    {
        /* Get the proof hash. */
        uint1024_t hashProof = pBlock->ProofHash();

        /* Skip if not prime channel or version less than 5 */
        if(nChannel.load() != 1 || pBlock->nVersion < 5)
            return true;

        /* Exit loop when the block is above minimum prime origins and less than 1024-bit hashes */
        if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
            return true;

        /* Otherwise keep looping. */
        return false;
    }


    /* GetContext - Returns a MiningContext populated from the connection's subscription state */
    MiningContext Miner::GetContext()
    {
        LOCK(MUTEX);

        MiningContext ctx;
        ctx.nChannel = nChannel;
        ctx.fSubscribedToNotifications = fSubscribedToNotifications;
        ctx.nSubscribedChannel = nSubscribedChannel;
        ctx.strAddress = GetAddress().ToStringIP();
        return ctx;
    }


    /* SendChannelNotification - Send push notification to subscribed miner (legacy lane) */
    void Miner::SendChannelNotification()
    {
        /* Get blockchain state */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Get channel-specific state */
        TAO::Ledger::BlockState stateChannel = stateBest;
        if (!TAO::Ledger::GetLastState(stateChannel, nSubscribedChannel))
        {
            debug::error(FUNCTION, "Failed to get channel state for channel ", nSubscribedChannel);
            return;
        }
        
        /* Get difficulty */
        uint32_t nDifficulty = LLP::StatelessMinerConnection::GetCachedDifficulty(nSubscribedChannel);
        
        /* Build notification using unified builder (8-bit opcodes for legacy lane) */
        Packet notification = PushNotificationBuilder::BuildChannelNotification<Packet>(
            nSubscribedChannel, ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty);
        
        /* Send to miner */
        respond(notification.HEADER, notification.DATA);
        
        debug::log(2, FUNCTION, "Sent ", GetChannelName(nSubscribedChannel), 
                   " notification to ", GetAddress().ToStringIP(),
                   " (unified=", stateBest.nHeight, 
                   ", channelHeight=", stateChannel.nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");
    }


    /* Stateless handler for GET_BLOCK - creates a block template and sends BLOCK_DATA */
    bool Miner::handle_get_block_stateless()
    {
        debug::log(2, FUNCTION, "GET_BLOCK from ", GetAddress().ToStringIP());

        /* Check authentication for stateless miners */
        if(!fMinerAuthenticated)
        {
            debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_BLOCK before authentication from ",
                       GetAddress().ToStringIP());
            std::vector<uint8_t> vFail(1, 0x00);
            respond(MINER_AUTH_RESULT, vFail);
            return debug::error(FUNCTION, "Authentication required for stateless miner commands");
        }

        /* Check if reward address is bound for stateless miners */
        if(!fRewardBound)
        {
            debug::error(FUNCTION, "GET_BLOCK: reward address not set - send MINER_SET_REWARD first");
            return debug::error(FUNCTION, "Reward address required for mining");
        }

        TAO::Ledger::Block *pBlock = nullptr;

        /* Prepare the data to serialize on request. */
        std::vector<uint8_t> vData;
        {
            LOCK(MUTEX);

            /* Create a new block */
            pBlock = new_block();

            /* Handle if the block failed to be created. */
            if(!pBlock)
            {
                debug::log(2, FUNCTION, "Failed to create block.");
                return true;
            }

            /* Store the new block in the memory map of recent blocks being worked on. */
            mapBlocks[pBlock->hashMerkleRoot] = pBlock;

            /* Serialize the block vData */
            vData = pBlock->Serialize();
        }

        /* Create and write the response packet. */
        respond(BLOCK_DATA, vData);

        /* Update last template channel height after sending template.
         * Uses channel-specific height (not unified) to prevent false template
         * refreshes when other channels mine blocks. */
        {
            TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
            TAO::Ledger::BlockState stateChannel = stateBest;
            if(TAO::Ledger::GetLastState(stateChannel, nChannel))
                nLastTemplateChannelHeight = stateChannel.nChannelHeight;
        }

        debug::log(2, FUNCTION, "Sent BLOCK_DATA (", vData.size(), " bytes)"
                   " channel=", pBlock->nChannel, " height=", pBlock->nHeight);

        return true;
    }


    /* Stateless handler for MINER_READY - subscribes to push notifications and sends template */
    bool Miner::handle_miner_ready_stateless()
    {
        debug::log(0, FUNCTION, "MINER_READY received from ", GetAddress().ToStringIP(), " (legacy port)");

        /* Validate channel (1=Prime, 2=Hash only) */
        if(nChannel != 1 && nChannel != 2)
        {
            debug::error(FUNCTION, "Invalid channel for MINER_READY: ", nChannel);
            debug::error(FUNCTION, "  Channel must be 1 (Prime) or 2 (Hash)");
            debug::error(FUNCTION, "  Miner must send SET_CHANNEL before MINER_READY");
            Disconnect();
            return false;
        }

        /* Update subscription state */
        fSubscribedToNotifications = true;
        nSubscribedChannel = nChannel;

        debug::log(0, FUNCTION, "Miner subscribed to channel ", nChannel, " (", GetChannelName(nChannel), ")");

        /* Send immediate notification */
        SendChannelNotification();

        return true;
    }


    /* Stateless handler for SUBMIT_BLOCK - validates and processes a block submission */
    bool Miner::handle_submit_block_stateless(const Packet& PACKET)
    {
        /* Check authentication for stateless miners */
        if(!fMinerAuthenticated)
        {
            debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted SUBMIT_BLOCK before authentication from ",
                       GetAddress().ToStringIP());
            std::vector<uint8_t> vFail(1, 0x00);
            respond(MINER_AUTH_RESULT, vFail);
            return debug::error(FUNCTION, "Authentication required for stateless miner commands");
        }

        debug::log(2, FUNCTION, "SUBMIT_BLOCK from ", GetAddress().ToStringIP(),
                   " size=", PACKET.DATA.size());

        /* Validate packet size using FalconConstants */
        const size_t MIN_SIZE = FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE;
        const size_t MAX_SIZE = FalconConstants::SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX;

        if(PACKET.DATA.size() < MIN_SIZE)
        {
            debug::log(0, FUNCTION, "SUBMIT_BLOCK packet too small: ",
                       PACKET.DATA.size(), " < ", MIN_SIZE);
            respond(BLOCK_REJECTED);
            return true;
        }

        if(PACKET.DATA.size() > MAX_SIZE)
        {
            debug::log(0, FUNCTION, "SUBMIT_BLOCK packet too large: ",
                       PACKET.DATA.size(), " > ", MAX_SIZE);
            respond(BLOCK_REJECTED);
            return true;
        }

        /* Log signature mode for diagnostics */
        if(PACKET.DATA.size() > FalconConstants::SUBMIT_BLOCK_WRAPPER_MAX)
        {
            debug::log(2, FUNCTION, "SUBMIT_BLOCK: Dual-signature mode detected");
        }
        else if(PACKET.DATA.size() >= FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN)
        {
            debug::log(3, FUNCTION, "SUBMIT_BLOCK: Single-signature mode");
        }
        else
        {
            debug::log(3, FUNCTION, "SUBMIT_BLOCK: Legacy format");
        }

        uint512_t hashMerkle;
        uint64_t nonce = 0;
        bool fParsed = false;
        std::vector<uint8_t> vWorkData = PACKET.DATA;

        /* Attempt to unwrap ChaCha20-encrypted payloads */
        if(fEncryptionReady && !vChaChaKey.empty() &&
           PACKET.DATA.size() >= FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN)
        {
            std::vector<uint8_t> vDecrypted;
            if(LLC::DecryptPayloadChaCha20(PACKET.DATA, vChaChaKey, vDecrypted))
            {
                debug::log(2, FUNCTION, "SUBMIT_BLOCK: ChaCha20 decryption succeeded");
                vWorkData = std::move(vDecrypted);

                if(vMinerPubKey.empty())
                {
                    debug::error(FUNCTION, "SUBMIT_BLOCK verify failed: missing Falcon public key");
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Pure verification — no wrapper creation, no keypair generation.
                 * The node VERIFIES the miner's Disposable Falcon signature using the public
                 * key stored during MINER_AUTH_INIT, then DISCARDS it. The signature is never
                 * forwarded to the Nexus P2P network. */
                DisposableFalcon::SignedWorkSubmission submission;
                if(!DisposableFalcon::VerifyWorkSubmission(vWorkData, vMinerPubKey, submission))
                {
                    debug::error(FUNCTION, "SUBMIT_BLOCK verify failed: Disposable Falcon signature invalid");
                    respond(BLOCK_REJECTED);  // 8-bit opcode — correct for legacy lane
                    return true;
                }

                /* Signature verified — extract merkle root and nonce */
                hashMerkle = submission.hashMerkleRoot;
                nonce = submission.nNonce;
                fParsed = true;

                debug::log(2, FUNCTION, "Disposable Falcon signature verified (legacy lane, Port 8323)");
            }
        }

        if(!fParsed)
        {
            TAO::Ledger::ParseResult parseResult =
                TAO::Ledger::ParseStatelessWorkSubmission(PACKET.DATA);
            if(!parseResult.success)
            {
                debug::error(FUNCTION, "SUBMIT_BLOCK parse failed: ", parseResult.reason);
                respond(BLOCK_REJECTED);
                return true;
            }

            hashMerkle = parseResult.hashMerkle;
            nonce = parseResult.nonce;
        }

        debug::log(3, FUNCTION, "Block merkle root: ", hashMerkle.SubString(), " nonce: ", nonce);

        LOCK(MUTEX);

        /* Make sure the block was created by this mining server. */
        if(!find_block(hashMerkle))
        {
            debug::log(2, FUNCTION, "Block not found in map");
            respond(BLOCK_REJECTED);
            return true;
        }

        /* Make sure there is no inconsistencies in signing block. */
        if(!sign_block(nonce, hashMerkle))
        {
            respond(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::TritiumBlock* pTritium =
            dynamic_cast<TAO::Ledger::TritiumBlock*>(mapBlocks[hashMerkle]);
        if(!pTritium)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK unexpected non-Tritium block for merkle ",
                         hashMerkle.SubString());
            respond(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::BlockValidationResult validationResult =
            TAO::Ledger::ValidateMinedBlock(*pTritium);
        if(!validationResult.valid)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK rejected: ", validationResult.reason);
            respond(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::BlockAcceptanceResult acceptanceResult =
            TAO::Ledger::AcceptMinedBlock(*pTritium);
        if(!acceptanceResult.accepted)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK rejected: ", acceptanceResult.reason);
            respond(BLOCK_REJECTED);
            return true;
        }

        debug::log(0, FUNCTION, "BLOCK_ACCEPTED merkle=", hashMerkle.SubString(),
                   " channel=", acceptanceResult.nChannel, " height=", acceptanceResult.nHeight);
        respond(BLOCK_ACCEPTED);
        return true;
    }


    /* Stateless handler for GET_ROUND - sends NEW_ROUND with height information */
    bool Miner::handle_get_round_stateless()
    {
        debug::log(2, FUNCTION, "GET_ROUND from ", GetAddress().ToStringIP());

        /* Check authentication for stateless miners */
        if(!fMinerAuthenticated)
        {
            debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_ROUND before authentication from ",
                       GetAddress().ToStringIP());
            std::vector<uint8_t> vFail(1, 0x00);
            respond(MINER_AUTH_RESULT, vFail);
            return debug::error(FUNCTION, "Authentication required for stateless miner commands");
        }

        /* Verify blockchain is ready */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();

        /* Check if blockchain is initialized (height == 0 means not yet initialized) */
        if(tStateBest.nHeight == 0)
        {
            debug::error(FUNCTION, "GET_ROUND: Blockchain not initialized from ", GetAddress().ToStringIP());
            return true;
        }

        /* Get unified height */
        uint32_t nUnifiedHeight = tStateBest.nHeight;

        /* Reuse a single BlockState for all GetLastState calls to reduce memory allocation. */
        TAO::Ledger::BlockState stateChannel = tStateBest;

        /* Get Prime channel height (Channel 1) */
        uint32_t nPrimeHeight = 0;
        stateChannel = tStateBest;
        if(TAO::Ledger::GetLastState(stateChannel, 1))
            nPrimeHeight = stateChannel.nChannelHeight;

        /* Get Hash channel height (Channel 2) */
        uint32_t nHashHeight = 0;
        stateChannel = tStateBest;
        if(TAO::Ledger::GetLastState(stateChannel, 2))
            nHashHeight = stateChannel.nChannelHeight;

        /* Get Stake channel height (Channel 0) */
        uint32_t nStakeHeight = 0;
        stateChannel = tStateBest;
        if(TAO::Ledger::GetLastState(stateChannel, 0))
            nStakeHeight = stateChannel.nChannelHeight;

        /* Build 16-byte NEW_ROUND response
         * Format: [Unified(4)][Prime(4)][Hash(4)][Stake(4)] = 16 bytes total */
        std::vector<uint8_t> vResponse;
        vResponse.reserve(16);

        std::vector<uint8_t> vUnified = convert::uint2bytes(nUnifiedHeight);
        std::vector<uint8_t> vPrime = convert::uint2bytes(nPrimeHeight);
        std::vector<uint8_t> vHash = convert::uint2bytes(nHashHeight);
        std::vector<uint8_t> vStake = convert::uint2bytes(nStakeHeight);

        vResponse.insert(vResponse.end(), vUnified.begin(), vUnified.end());
        vResponse.insert(vResponse.end(), vPrime.begin(), vPrime.end());
        vResponse.insert(vResponse.end(), vHash.begin(), vHash.end());
        vResponse.insert(vResponse.end(), vStake.begin(), vStake.end());

        /* Verify packet size */
        if(vResponse.size() != 16)
        {
            debug::error(FUNCTION, "GET_ROUND: Response size mismatch (", vResponse.size(), " != 16)");
            return true;
        }

        debug::log(2, FUNCTION, "Sending NEW_ROUND (16 bytes): unified=", nUnifiedHeight,
                   " prime=", nPrimeHeight, " hash=", nHashHeight, " stake=", nStakeHeight);

        /* Send the NEW_ROUND response */
        respond(NEW_ROUND, vResponse);

        /* GET_ROUND COMPATIBILITY: AUTO-SEND TEMPLATE
         * Legacy miners using GET_ROUND polling expect to receive BLOCK_DATA automatically
         * when the height changes, without needing to explicitly request GET_BLOCK.
         *
         * CRITICAL: Use channel-specific height comparison (not unified height).
         * Only send a new template when the miner's OWN channel advances.
         * Using unified height would trigger unnecessary template sends when OTHER
         * channels mine blocks, causing ~40% wasted mining work. */
        uint32_t nCurrentChannelHeight = 0;
        if(nChannel == 1)
            nCurrentChannelHeight = nPrimeHeight;
        else if(nChannel == 2)
            nCurrentChannelHeight = nHashHeight;

        bool fChannelHeightChanged = (nLastTemplateChannelHeight != nCurrentChannelHeight);

        if(fChannelHeightChanged)
        {
            debug::log(2, FUNCTION, "Channel ", nChannel.load(), " advanced: ",
                       nLastTemplateChannelHeight, " -> ", nCurrentChannelHeight,
                       " - auto-sending template");

            TAO::Ledger::Block* pBlock = new_block();

            if(!pBlock)
            {
                debug::error(FUNCTION, "GET_ROUND auto-send: new_block() returned nullptr");
            }
            else
            {
                try {
                    std::vector<uint8_t> vBlockData = pBlock->Serialize();

                    if(!vBlockData.empty())
                    {
                        respond(BLOCK_DATA, vBlockData);

                        debug::log(2, FUNCTION, "Auto-sent BLOCK_DATA (",
                                   vBlockData.size(), " bytes) channel=",
                                   pBlock->nChannel, " height=", pBlock->nHeight);

                        StatelessMinerManager::Get().IncrementTemplatesServed();

                        /* Update last template channel height only after successful send */
                        nLastTemplateChannelHeight = nCurrentChannelHeight;
                    }
                }
                catch(const std::exception& e) {
                    debug::error(FUNCTION, "GET_ROUND auto-send exception: ", e.what());
                }
            }
        }

        return true;
    }


    /* Sends a stateless (16-bit) protocol response packet */
    void Miner::respond_stateless(uint16_t nOpcode, const std::vector<uint8_t>& vData)
    {
        /* Build raw packet with 16-bit opcode (big-endian) and 4-byte length */
        std::vector<uint8_t> vPacket;
        vPacket.reserve(6 + vData.size());

        /* Write 16-bit opcode (big-endian) */
        vPacket.push_back((nOpcode >> 8) & 0xFF);
        vPacket.push_back(nOpcode & 0xFF);

        /* Write 4-byte length (big-endian) */
        uint32_t nLength = static_cast<uint32_t>(vData.size());
        vPacket.push_back((nLength >> 24) & 0xFF);
        vPacket.push_back((nLength >> 16) & 0xFF);
        vPacket.push_back((nLength >> 8) & 0xFF);
        vPacket.push_back(nLength & 0xFF);

        /* Append payload */
        vPacket.insert(vPacket.end(), vData.begin(), vData.end());

        /* Write raw bytes to connection */
        Write(vPacket, vPacket.size());

        debug::log(3, FUNCTION, "Stateless response: opcode=0x", std::hex, nOpcode,
                   " length=", std::dec, nLength, " to ", GetAddress().ToStringIP());
    }

}
