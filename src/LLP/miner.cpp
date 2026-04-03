/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/genesis_constants.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/node_cache.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/get_block_policy.h>
#include <LLP/include/mining_session_health.h>
#include <LLP/include/push_notification.h>
#include <LLP/include/keepalive_v2.h>
#include <LLP/include/session_status.h>
#include <LLP/include/session_start_packet.h>
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
#include <Util/include/runtime.h>

#include <LLP/include/colin_mining_agent.h>
#include <LLP/include/node_session_registry.h>
#include <LLP/include/legacy_get_block_handler.h>

#include <cstring>


namespace LLP
{
    namespace
    {
        using Diagnostics::FullHexOrUnset;
        using Diagnostics::KeyFingerprint;
        using Diagnostics::YesNo;
    }

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
    , fStatelessProtocol(false)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    , nMinerPrevblockSuffix({})
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
    , fStatelessProtocol(false)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    , nMinerPrevblockSuffix({})
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
    , fStatelessProtocol(false)
    , vChaChaKey()
    , fEncryptionReady(false)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , fSubscribedToNotifications(false)
    , nSubscribedChannel(0)
    , nLastTemplateChannelHeight(0)
    , nMinerPrevblockSuffix({})
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
        fStatelessProtocol = false;
        hashKeyID = 0;

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

                /* Interrupt any in-flight SendChannelNotification() path immediately. */
                m_shutdownRequested.store(true, std::memory_order_release);

                /* Notify NodeSessionRegistry that this legacy lane miner has disconnected.
                 * hashKeyID is set during Falcon authentication; zero means never authenticated. */
                if(fMinerAuthenticated && hashKeyID != 0)
                    NodeSessionRegistry::Get().MarkDisconnected(hashKeyID, ProtocolLane::LEGACY);

                /* Notify Colin agent on disconnect (only if genesis was known) */
                if(hashGenesis != 0)
                {
                    ColinMiningAgent::Get().on_miner_disconnected(hashGenesis.SubString(8));
                }
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

            /* Handle SESSION_KEEPALIVE (212 / 0xD4) — unified keepalive handler.
             * Request: 8 bytes — [session_id LE][hashPrevBlock_lo32 BE]
             * Response: 32 bytes — [session_id LE][all other fields BE] */
            if(PACKET.HEADER == SESSION_KEEPALIVE)
            {
                debug::log(2, FUNCTION, "════════════════════════════════════");
                debug::log(2, FUNCTION, "SESSION_KEEPALIVE received");

                /* Parse 8-byte keepalive payload (session_id LE, hashPrevBlock_lo32 BE) */
                uint32_t nKeepaliveSession = 0;
                std::array<uint8_t, 4> prevblockSuffixBytes = {};

                if(!KeepaliveV2::ParsePayload(PACKET.DATA, nKeepaliveSession, prevblockSuffixBytes))
                {
                    debug::error(FUNCTION, "Invalid SESSION_KEEPALIVE: need 8-byte payload, got ", PACKET.DATA.size(), " bytes");
                    return true;
                }

                /* Derive hashPrevBlock_lo32 from suffix bytes (BE) */
                uint32_t nMinerPrevHashLo32 =
                    (uint32_t(prevblockSuffixBytes[0]) << 24) |
                    (uint32_t(prevblockSuffixBytes[1]) << 16) |
                    (uint32_t(prevblockSuffixBytes[2]) <<  8) |
                     uint32_t(prevblockSuffixBytes[3]);

                /* Store prevblock suffix */
                nMinerPrevblockSuffix = prevblockSuffixBytes;
                char suffixHex[9];
                std::snprintf(suffixHex, sizeof(suffixHex), "%02x%02x%02x%02x",
                              prevblockSuffixBytes[0], prevblockSuffixBytes[1],
                              prevblockSuffixBytes[2], prevblockSuffixBytes[3]);
                debug::log(2, FUNCTION, "SESSION_KEEPALIVE: prevblock_suffix=", suffixHex);
                debug::log(3, FUNCTION, "SESSION_KEEPALIVE request:"
                           " session=0x", std::hex, nKeepaliveSession,
                           " miner_prevhash_lo32=0x", nMinerPrevHashLo32, std::dec);

                debug::log(2, FUNCTION, "Session ID: 0x", std::hex, nKeepaliveSession, std::dec);

                auto optContext = StatelessMinerManager::Get().GetMinerContextBySessionID(nKeepaliveSession);
                if(!optContext.has_value())
                {
                    debug::error(FUNCTION, "Unknown session_id: 0x", std::hex, nKeepaliveSession, std::dec);
                    debug::error(FUNCTION, "Session may have expired or never existed");
                    Disconnect();
                    return true;
                }

                /* Capture address before atomic transform for session recovery */
                std::string strSessionAddress = optContext.value().strAddress;

                /* Atomic transform: update timestamp, keepalive count, and prevblock suffix
                 * directly on the CURRENT value in mapMiners, avoiding TOCTOU race where
                 * NotifyNewRound could overwrite connection-specific fields. */
                MiningContext transformedCtx;
                std::array<uint8_t, 4> prevSuffix = nMinerPrevblockSuffix;
                StatelessMinerManager::Get().TransformMinerBySession(nKeepaliveSession,
                    [prevSuffix, &transformedCtx](const MiningContext& current) {
                        transformedCtx = current
                            .WithTimestamp(runtime::unifiedtimestamp())
                            .WithKeepaliveCount(current.nKeepaliveCount + 1)
                            .WithMinerPrevblockSuffix(prevSuffix);
                        return transformedCtx;
                    });

                if(transformedCtx.fAuthenticated && transformedCtx.hashKeyID != 0)
                    SessionRecoveryManager::Get().SaveSession(transformedCtx);

                /* Reset connection activity timer to prevent idle disconnection */
                this->Reset();

                constexpr uint64_t SECONDS_PER_DAY = 86400;
                debug::log(2, FUNCTION, "Session refreshed:");
                debug::log(2, FUNCTION, "  Keepalives: ", transformedCtx.nKeepaliveCount);
                debug::log(2, FUNCTION, "  Liveness window: ", SECONDS_PER_DAY, "s (",
                           (SECONDS_PER_DAY / SECONDS_PER_DAY), "d)");

                /* Build unified 32-byte response */
                TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
                uint32_t nUnifiedHeight = stateBest.nHeight;

                TAO::Ledger::BlockState stateChannel = stateBest;
                uint32_t nPrimeHeight = 0;
                if(TAO::Ledger::GetLastState(stateChannel, 1))
                    nPrimeHeight = stateChannel.nChannelHeight;

                stateChannel = stateBest;
                uint32_t nHashHeight = 0;
                if(TAO::Ledger::GetLastState(stateChannel, 2))
                    nHashHeight = stateChannel.nChannelHeight;

                stateChannel = stateBest;
                uint32_t nStakeHeight = 0;
                if(TAO::Ledger::GetLastState(stateChannel, 0))
                    nStakeHeight = stateChannel.nChannelHeight;

                uint1024_t hashBestChain = stateBest.GetHash();
                uint32_t nHashTipLo32 = static_cast<uint32_t>(hashBestChain.Get64(0) & 0xFFFFFFFF);

                uint32_t nForkScore = (nMinerPrevHashLo32 != 0 && nMinerPrevHashLo32 != nHashTipLo32) ? 1u : 0u;

                std::vector<uint8_t> vV2 = KeepaliveV2::BuildUnifiedResponse(
                    nKeepaliveSession,
                    nMinerPrevHashLo32,   // echo miner's fork canary
                    nUnifiedHeight,
                    nHashTipLo32,
                    nPrimeHeight,
                    nHashHeight,
                    nStakeHeight,
                    nForkScore);

                debug::log(2, FUNCTION, "Sent SESSION_KEEPALIVE unified reply (32 bytes):",
                           " unified=", nUnifiedHeight,
                           " prime=", nPrimeHeight,
                           " hash=", nHashHeight,
                           " stake=", nStakeHeight,
                           " hash_tip_lo32=0x", std::hex, nHashTipLo32,
                           " miner_prevhash_lo32=0x", nMinerPrevHashLo32,
                           " fork_score=", std::dec, nForkScore);

                respond(SESSION_KEEPALIVE, vV2);

                /* Notify Colin agent with keepalive telemetry (observability hook) */
                if(hashGenesis != 0)
                {
                    ColinMiningAgent::Get().on_keepalive_ack(
                        hashGenesis.SubString(8),
                        nUnifiedHeight,
                        nPrimeHeight,
                        nHashHeight,
                        nStakeHeight,
                        nForkScore);
                }

                debug::log(2, FUNCTION, "════════════════════════════════════");

                return true;
            }

            /* Handle SESSION_STATUS (219 / 0xDB) — miner queries lane health on legacy port */
            if(PACKET.HEADER == OpcodeUtility::Opcodes::SESSION_STATUS)
            {
                debug::log(2, FUNCTION, "SESSION_STATUS received from ", GetAddress().ToStringIP());

                SessionStatus::SessionStatusRequest req;
                if(!req.Parse(PACKET.DATA))
                {
                    debug::error(FUNCTION, "SESSION_STATUS: malformed payload (size=", PACKET.DATA.size(), ")");
                    return true;
                }

                /* Validate session */
                auto optContext = StatelessMinerManager::Get().GetMinerContextBySessionID(req.session_id);
                if(!optContext.has_value())
                {
                    debug::error(FUNCTION, "SESSION_STATUS: unknown session_id=0x", std::hex, req.session_id, std::dec);
                    /* Respond with zero flags — miner knows session is invalid */
                    auto vAck = SessionStatus::BuildAckPayload(req.session_id, 0u, 0u, req.status_flags);
                    respond(OpcodeUtility::Opcodes::SESSION_STATUS_ACK, vAck);
                    return true;
                }

                /* Build lane health flags */
                uint32_t nLaneHealth = 0;
                nLaneHealth |= SessionStatus::LANE_SECONDARY_ALIVE;   // legacy lane: we are ON it
                nLaneHealth |= SessionStatus::LANE_AUTHENTICATED;     // session validated above

                const uint32_t nUptime = static_cast<uint32_t>(optContext->GetSessionDuration(runtime::unifiedtimestamp()));
                auto vAck = SessionStatus::BuildAckPayload(req.session_id, nLaneHealth, nUptime, req.status_flags);
                respond(OpcodeUtility::Opcodes::SESSION_STATUS_ACK, vAck);

                debug::log(2, FUNCTION, "SESSION_STATUS_ACK sent: lane_health=0x", std::hex, nLaneHealth, std::dec);

                /* TWO-STEP RE-ARM INVARIANT (PR #375):
                 * SendChannelNotification() consumes m_force_next_push (sets it false)
                 * AND resets m_last_template_push_time to "now". Without re-arming
                 * m_force_next_push before any follow-up template send, the push is
                 * throttled (elapsed ~0 ms). See stateless SESSION_STATUS handler and
                 * handle_miner_ready_stateless() for the canonical two-step pattern. */
                if((req.status_flags & SessionStatus::MINER_DEGRADED) &&
                    fSubscribedToNotifications && (nSubscribedChannel == 1 || nSubscribedChannel == 2))
                {
                    debug::log(0, FUNCTION, "⚠️ Miner reports DEGRADED — forcing recovery push via legacy SESSION_STATUS");
                    {
                        LOCK(MUTEX);
                        m_force_next_push = true;
                        m_get_block_cooldown = AutoCoolDown(std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS));
                    }
                    SendChannelNotification();
                    debug::log(0, FUNCTION, "✓ Degraded-recovery notification pushed on legacy lane");
                }

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

                /* Route auth/session/config opcodes through LegacyLaneHandler for per-opcode
                 * MUTEX isolation.  Each handler class (AuthResponseHandler, SessionStartHandler,
                 * etc.) owns its own mutex, so authentication doesn't serialize against keepalive,
                 * channel selection, or reward binding.
                 *
                 * Opcodes NOT registered in LegacyLaneHandler (GET_BLOCK, SUBMIT_BLOCK, etc.)
                 * fall through to StatelessMiner::ProcessPacket() which delegates them to the
                 * connection layer via "Unknown packet type" → ProcessPacketStateless(). */
                const uint16_t nMirroredOpcode = OpcodeUtility::Stateless::Mirror(
                    static_cast<uint8_t>(PACKET.HEADER));

                /* ProcessResult has const members → not assignable.  Use a dispatching
                 * lambda so we can initialize result once via copy-elision (RVO). */
                auto dispatchLegacyPacket = [&]() -> ProcessResult
                {
                    if(m_laneHandler.CanHandle(nMirroredOpcode))
                    {
                        /* Build StatelessPacket from legacy Packet for unified handler interface */
                        StatelessPacket sp(nMirroredOpcode);
                        sp.DATA   = PACKET.DATA;
                        sp.LENGTH = PACKET.LENGTH;

                        debug::log(2, FUNCTION, "LegacyLaneHandler: dispatching opcode ",
                                   uint32_t(PACKET.HEADER), " (mirrored 0x",
                                   std::hex, nMirroredOpcode, std::dec, ")");

                        return m_laneHandler.Dispatch(context, sp);
                    }

                    /* Fall through to StatelessMiner::ProcessPacket for connection-layer opcodes */
                    return StatelessMiner::ProcessPacket(context, PACKET);
                };
                ProcessResult result = dispatchLegacyPacket();

                /* Legacy lane response transmission.
                 * Both LegacyLaneHandler and StatelessMiner::ProcessPacket() return 16-bit
                 * stateless opcodes (0xD0xx).  Legacy lane expects 8-bit opcodes, so we
                 * unmirror and convert. */

                /* Handle result */
                if(result.fSuccess)
                {
                    /* Reset connection activity timer to prevent idle disconnection */
                    this->Reset();
                    
                    /* Update connection state from result context */
                    const bool fWasAuthenticated = fMinerAuthenticated;
                    nChannel = result.context.nChannel;
                    nBestHeight = result.context.nHeight;
                    fMinerAuthenticated = result.context.fAuthenticated;
                    nSessionId = result.context.nSessionId;
                    hashGenesis = result.context.hashGenesis;

                    /* Mark connection as using stateless protocol framing after successful
                     * Falcon authentication.  All subsequent responses on this connection
                     * will use 16-bit stateless opcodes via respond_auto(). */
                    if(!fWasAuthenticated && result.context.fAuthenticated)
                        fStatelessProtocol = true;
                    
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

                        if(updatedContext.nSessionStart == 0)
                            updatedContext = updatedContext.WithSessionStart(nNow);
                    }

                    /* Register session in NodeSessionRegistry for cross-port session identity.
                     * This is the node-side outer wrapper that mirrors NodeSession on the miner side.
                     * One Falcon identity (hashKeyID) maps to one canonical nSessionId across both ports. */
                    if(updatedContext.fAuthenticated && updatedContext.hashKeyID != 0)
                    {
                        auto [canonicalSessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
                            updatedContext.hashKeyID,
                            updatedContext.hashGenesis,
                            updatedContext,
                            ProtocolLane::LEGACY
                        );

                        /* CRITICAL: If the registry returned a different nSessionId than what we derived,
                         * it means this miner has already authenticated on the other port.
                         * We MUST use the canonical ID to prevent session_id=0 bugs on reconnection. */
                        if(canonicalSessionId != updatedContext.nSessionId)
                        {
                            debug::log(0, FUNCTION, "⚠ Cross-port session recovery: Overriding derived sessionId ",
                                       updatedContext.nSessionId, " → ", canonicalSessionId,
                                       " (already registered on other port)");
                            updatedContext = updatedContext.WithSession(canonicalSessionId);
                            nSessionId = canonicalSessionId;  // Update connection field too
                        }

                        if(isNew)
                        {
                            debug::log(0, FUNCTION, "✓ Registered NEW session in NodeSessionRegistry: sessionId=",
                                       canonicalSessionId, " keyID=", updatedContext.hashKeyID.SubString());
                        }
                        else
                        {
                            debug::log(0, FUNCTION, "✓ Refreshed EXISTING session in NodeSessionRegistry: sessionId=",
                                       canonicalSessionId, " keyID=", updatedContext.hashKeyID.SubString());
                        }

                        /* Store hashKeyID as member for DISCONNECT handler cleanup */
                        hashKeyID = updatedContext.hashKeyID;
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

                    /* Atomic transform: apply auth completion state to CURRENT value in mapMiners.
                     * Captures all auth-specific fields from updatedContext and applies them
                     * atomically, preserving any concurrent height/timestamp updates. */
                    {
                        MiningContext authCtx = updatedContext;
                        StatelessMinerManager::Get().TransformMiner(updatedContext.strAddress,
                            [authCtx](const MiningContext& current) {
                                MiningContext result = current
                                    .WithAuth(authCtx.fAuthenticated)
                                    .WithSession(authCtx.nSessionId)
                                    .WithKeyId(authCtx.hashKeyID)
                                    .WithGenesis(authCtx.hashGenesis)
                                    .WithUserName(authCtx.strUserName)
                                    .WithPubKey(authCtx.vMinerPubKey)
                                    .WithFalconVersion(authCtx.nFalconVersion)
                                    .WithTimestamp(runtime::unifiedtimestamp());
                                if(authCtx.nSessionStart != 0)
                                    result = result.WithSessionStart(authCtx.nSessionStart);
                                if(authCtx.fEncryptionReady && !authCtx.vChaChaKey.empty())
                                    result = result.WithChaChaKey(authCtx.vChaChaKey);
                                if(!authCtx.vDisposablePubKey.empty())
                                    result = result.WithDisposableKey(authCtx.vDisposablePubKey, authCtx.hashDisposableKeyID);
                                return result;
                            }, 0);
                    }

                    /* Notify Colin agent when genesis is authenticated for the first time */
                    if(!fWasAuthenticated && fMinerAuthenticated && hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_miner_connected(
                            hashGenesis.SubString(8),
                            GetAddress().ToStringIP()
                        );
                    }

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

                    /* BUG FIX: Legacy lane (port 8323) previously never sent SESSION_START.
                     * After successful authentication, send SESSION_START to advertise the
                     * node's session timeout configuration to the miner.  This is a sibling
                     * of the IsNull() check (not nested inside it) for robustness.
                     * Uses shared SessionStartPacket::BuildPayload() for wire-format
                     * consistency with the stateless lane (port 9323). */
                    if(PACKET.HEADER == MINER_AUTH_RESPONSE && result.fSuccess && fMinerAuthenticated)
                    {
                        debug::log(0, FUNCTION, "Sending SESSION_START after successful authentication (legacy lane)");

                        /* Build SESSION_START payload using shared utility.
                         * The session liveness timeout is a node-wide constant from NodeCache,
                         * NOT a per-context field.  nSessionTimeout was removed from MiningContext. */
                        const uint64_t nLivenessTimeout = NodeCache::GetSessionLivenessTimeout(updatedContext.strAddress);
                        std::vector<uint8_t> vSessionStart = SessionStartPacket::BuildPayload(
                            nSessionId, nLivenessTimeout, hashGenesis);

                        /* Send via respond_auto: uses 16-bit stateless framing after Falcon auth */
                        respond_auto(OpcodeUtility::Opcodes::SESSION_START, vSessionStart);

                        debug::log(0, FUNCTION, "SESSION_START: sessionId=", nSessionId,
                                  " timeout=", static_cast<uint32_t>(nLivenessTimeout),
                                  "s session_genesis=", (hashGenesis != 0 ? hashGenesis.SubString() : "none"));
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
            
            /* SESSION::DEFAULT not found — new_block() or CreateBlockForStatelessMining()
             * tried to access the wallet session that was never established (or expired).
             * This happens when the node is started without -autologin=user:pass and the
             * wallet has not been unlocked for mining via the API.
             *
             * CRITICAL FIX: Do NOT silently return true here.  The miner sent GET_BLOCK
             * and is waiting for a response; swallowing the exception leaves the miner
             * in template-starvation (push_ahead N, round_ahead 0).  We must send an
             * explicit TEMPLATE_SOURCE_UNAVAILABLE wire response so the miner backs off
             * and retries, rather than hanging indefinitely.
             */
            if(strError.find("Session not found") != std::string::npos)
            {
                debug::error(FUNCTION, "MinerLLP: SESSION::DEFAULT not available for mining from ",
                             GetAddress().ToStringIP(),
                             " — node requires -autologin=user:pass or manual unlock for mining."
                             " GET_BLOCK will return TEMPLATE_SOURCE_UNAVAILABLE to miner");

                /* Send explicit wire response so the miner knows to back off and retry.
                 * Keep the connection alive — the session may become available later. */
                try
                {
                    respond_auto(BLOCK_REJECTED,
                        BuildGetBlockControlPayload(
                            GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE, 5000u));
                }
                catch(...) { /* best-effort send — ignore secondary send failure */ }

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

            /* PONG_DIAG (legacy lane) — echo back to Colin Agent */
            case static_cast<uint8_t>(ColinDiagOpcodes::PONG_DIAG_LEGACY & 0xFF):
            {
                if(PACKET.DATA.size() >= 64 && hashGenesis != 0)
                    ColinMiningAgent::Get().on_pong_received(hashGenesis.SubString(8), PACKET.DATA);
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


    /* Unified response dispatch with lane auto-detection.
     * Checks fStatelessProtocol to determine framing:
     *   - true:  16-bit stateless framing via respond_stateless()
     *   - false: 8-bit legacy framing via respond() */
    void Miner::respond_auto(uint8_t nLegacyOpcode, const std::vector<uint8_t>& vData)
    {
        if(fStatelessProtocol)
            respond_stateless(OpcodeUtility::Stateless::Mirror(nLegacyOpcode), vData);
        else
            respond(nLegacyOpcode, vData);
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
        uint1024_t hashCurrentBest = TAO::Ledger::ChainState::hashBestChain.load();

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
        {
            /* Hash-based staleness check: detect same-height reorgs that nBestHeight misses.
             * mapBlockHashes stores hashBestChain at the moment each template was created.
             * If any stored snapshot differs from the current best, the chain has reorged. */
            bool fHashChanged = false;
            for(const auto& entry : mapBlockHashes)
            {
                if(entry.second != hashCurrentBest)
                {
                    fHashChanged = true;
                    break;
                }
            }

            if(!fHashChanged)
                return false;

            /* hashBestChain changed at the same height (reorg) — clear stale templates. */
            clear_map();
            debug::log(2, FUNCTION, "Mining map cleared: hashBestChain changed at height ", nChainStateHeight);
            return true;
        }

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

        /* Clear the parallel hash-snapshot map. */
        mapBlockHashes.clear();

        /* Clear the cross-connection SUBMIT_BLOCK deduplication cache on new round.
         * Same block solutions submitted on both SIM Link lanes after the round
         * ends would be expired anyway, but clearing eagerly avoids false positives. */
        ColinMiningAgent::Get().clear_dedup_cache();

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
        /* SESSION::DEFAULT health pre-check: fail fast before diving into
         * CreateBlockForStatelessMining() which requires the wallet session.
         * If the session is unavailable, log clearly and return nullptr so
         * the GET_BLOCK handler can send TEMPLATE_SOURCE_UNAVAILABLE to the miner. */
        if(!LLP::IsDefaultSessionReady())
        {
            debug::error(FUNCTION, "SESSION::DEFAULT not available for mining"
                         " — node requires -autologin=user:pass or manual unlock."
                         " GET_BLOCK will return TEMPLATE_SOURCE_UNAVAILABLE to miner");
            return nullptr;
        }

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

        /* Defense-in-depth: reject any resolved reward address whose type byte is not a valid
         * TritiumGenesis. Coinbase::Verify() enforces this on all network peers — a block with
         * a Register Address as coinbase recipient will be rejected by the entire network. */
        if(!LLP::GenesisConstants::IsValidGenesisType(hashReward))
        {
            debug::error(FUNCTION, "Resolved reward address has invalid type byte 0x",
                         std::hex, static_cast<int>(hashReward.GetType()), std::dec,
                         " — Register Addresses cannot be used as coinbase recipient.",
                         " Block creation aborted. Set a valid TritiumGenesis via MINER_SET_REWARD.");
            return nullptr;
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
    bool Miner::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot, const std::vector<uint8_t>& vOffsets)
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
            /* Build a canonical solved candidate from the immutable template.
             *
             * For Prime (channel 1):
             *   - Copy all consensus-critical template fields unchanged.
             *   - Apply miner-submitted nNonce and vOffsets.
             *   - Preserve nTime (ProofHash for Prime excludes nTime).
             *   - Clear vchBlockSig so FinalizeWalletSignatureForSolvedBlock re-signs.
             *
             * For Hash (channel 2):
             *   - Copy all consensus-critical template fields unchanged.
             *   - Apply miner-submitted nNonce.
             *   - Clear vOffsets (Hash channel invariant — no Cunningham chain).
             *   - Preserve nTime (ProofHash for Hash also excludes nTime).
             *   - Clear vchBlockSig so FinalizeWalletSignatureForSolvedBlock re-signs.
             *
             * The UpdateTime() call previously made here for all channels is removed
             * for both Prime and Hash: neither ProofHash computation includes nTime,
             * so mutating nTime after template issuance has no proof-correctness
             * benefit and can violate the "immutable anchor field" invariant. */
            if(pBlock->nChannel == TAO::Ledger::CHANNEL::PRIME)
            {
                *pBlock = TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(*pBlock, nNonce, vOffsets);

                /* Structural validation of miner-submitted Prime offsets.
                 * The prior GetOffsets(GetPrime()) equivalence check was broken: it
                 * returned empty offsets whenever GetPrime() was not itself prime,
                 * causing false rejections for otherwise valid Prime submissions.
                 * VerifySubmittedPrimeOffsets() performs lightweight structural checks
                 * only; the authoritative proof-of-work gate is VerifyWork() in Check().
                 *
                 * The empty check guards the legacy-fallback path below: when the miner
                 * does not submit vOffsets (compact wrapper), we fall through to GetOffsets()
                 * rather than rejecting. Only non-empty submissions are validated here. */
                if(!pBlock->vOffsets.empty() &&
                   !TAO::Ledger::VerifySubmittedPrimeOffsets(*pBlock, pBlock->vOffsets))
                {
                    return debug::error(FUNCTION, "Prime vOffsets structural validation failed");
                }

                /* Legacy fallback: derive offsets locally when the miner did not submit
                 * them (compact wrapper path).  This preserves backwards compatibility
                 * with older miners that do not include vOffsets in the payload. */
                if(pBlock->vOffsets.empty())
                    TAO::Ledger::GetOffsets(pBlock->GetPrime(), pBlock->vOffsets);
            }
            else if(pBlock->nChannel == TAO::Ledger::CHANNEL::HASH)
            {
                *pBlock = TAO::Ledger::BuildSolvedHashCandidateFromTemplate(*pBlock, nNonce);
            }
            else
            {
                /* Unknown channel — apply nNonce and clear offsets as a safe fallback. */
                pBlock->nNonce = nNonce;
                pBlock->vOffsets.clear();
                pBlock->vchBlockSig.clear();
            }

            /* Generate the canonical block signature using the shared wallet-signature
             * utility.  FinalizeWalletSignatureForSolvedBlock() unlocks the mining
             * sigchain and signs SignatureHash() with the producer key (Falcon or
             * Brainpool), supporting both key types.
             *
             * This replaces the prior inline credential unlock + switch/case that was
             * duplicated here.  The shared function is the canonical signing path for
             * both the legacy miner lane and the stateless lane, ensuring consistent
             * behaviour across channels. */
            if(!TAO::Ledger::FinalizeWalletSignatureForSolvedBlock(*pBlock))
                return debug::error(FUNCTION, "FinalizeWalletSignatureForSolvedBlock failed for ",
                                    hashMerkleRoot.SubString());

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

                /* Invalidate the failed template from the cache so the miner's
                 * next new_block() receives a fresh template rather than the
                 * stale one that failed to land. */
                auto itFailed = mapBlocks.find(hashMerkleRoot);
                if(itFailed != mapBlocks.end())
                {
                    debug::log(0, FUNCTION, "Invalidating failed template ",
                        hashMerkleRoot.SubString(), " from cache — next new_block() will regenerate");
                    mapBlocks.erase(itFailed);
                }

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
        ctx.nProtocolLane = ProtocolLane::LEGACY;  // Legacy Miner uses 8-bit opcodes
        return ctx;
    }


    /* SendChannelNotification - Send push notification to subscribed miner (legacy lane) */
    void Miner::SendChannelNotification()
    {
        auto shouldAbortNotification = [this]()
        {
            return GracefulShutdown::ShouldAbortChannelNotification(
                Connected(),
                m_shutdownRequested.load(std::memory_order_acquire),
                config::fShutdown.load()
            );
        };

        if(shouldAbortNotification())
        {
            debug::log(1, FUNCTION, "Skipping channel notification for ", GetAddress().ToStringIP(),
                       " due to disconnect/shutdown state");
            return;
        }

        /* Push throttle — drop if a template was sent less than
         * TEMPLATE_PUSH_MIN_INTERVAL_MS ago (guards against fork-resolution bursts).
         * Re-subscription responses bypass via m_force_next_push. */
        {
            LOCK(MUTEX);
            if(shouldAbortNotification())
            {
                debug::log(1, FUNCTION, "Aborting channel notification for ", GetAddress().ToStringIP(),
                           " after acquiring lock due to disconnect/shutdown state");
                return;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_template_push_time).count();
            if (m_force_next_push)
            {
                /* Re-subscription bypass: miner explicitly requested fresh work. */
                m_force_next_push = false;
            }
            else if (m_last_template_push_time != std::chrono::steady_clock::time_point{} &&
                elapsed < MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS)
            {
                debug::log(1, FUNCTION, "⏳ Push throttled — ", elapsed, "ms since last push (min ",
                           MiningConstants::TEMPLATE_PUSH_MIN_INTERVAL_MS, "ms); miner must wait");
                return;
            }
            m_last_template_push_time = now;
        }

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

        /* Keep the tip hash consistent with the loaded best-state snapshot. */
        const uint1024_t hashBestChain =
            PushNotificationBuilder::BestChainHashForNotification(stateBest);

        if(shouldAbortNotification())
        {
            debug::log(1, FUNCTION, "Aborting channel notification for ", GetAddress().ToStringIP(),
                       " before send due to disconnect/shutdown state");
            return;
        }
        
        /* Build notification using unified builder (8-bit opcodes for legacy lane) */
        Packet notification = PushNotificationBuilder::BuildChannelNotification<Packet>(
            nSubscribedChannel, ProtocolLane::LEGACY, stateBest, stateChannel, nDifficulty,
            hashBestChain);
        
        /* Send to miner */
        respond(notification.HEADER, notification.DATA);
        
        debug::log(0, FUNCTION, "[BLOCK CREATE] hashPrevBlock = ", hashBestChain.SubString(),
                   " (template anchor embedded in push notification, unified height ", stateBest.nHeight + 1, ")");
        debug::log(2, FUNCTION, "Sent ", GetChannelName(nSubscribedChannel), 
                   " notification to ", GetAddress().ToStringIP(),
                   " (unified=", stateBest.nHeight, 
                   ", channelHeight=", stateChannel.nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");

        /* Notify Colin agent: template pushed via push notification */
        if(hashGenesis != 0)
            ColinMiningAgent::Get().on_template_pushed(nSubscribedChannel, stateBest.nHeight + 1);
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
            respond_auto(MINER_AUTH_RESULT, vFail);
            return debug::error(FUNCTION, "Authentication required for stateless miner commands");
        }

        /* Check if reward address is bound for stateless miners */
        if(!fRewardBound)
        {
            respond_auto(BLOCK_REJECTED, BuildGetBlockControlPayload(GetBlockPolicyReason::TEMPLATE_NOT_READY, 0));
            return debug::error(FUNCTION, "GET_BLOCK: reward address not set - send MINER_SET_REWARD first");
        }

        /* AutoCoolDown only applies to unauthenticated or over-budget miners.
         * Authentication is already confirmed above; skip the cooldown gate so that
         * an authenticated miner who is within the rate-limit budget always receives
         * BLOCK_DATA (invariant: authenticated + in-budget → BLOCK_DATA MUST follow). */

        /* ── Legacy Lane: Delegate to LegacyGetBlockHandler ─────────────────────
         * Build the request with per-connection rate limiter.  The handler
         * performs per-connection rate limiting (25/60s), calls new_block(),
         * and serialises the 228-byte BLOCK_DATA payload.
         * No cross-lane state sharing. */
        {
            LOCK(MUTEX);

            /* BUG #4 fix: Validate nSessionId is non-zero before rate-limit lookups.
             * If nSessionId is 0 (uninitialized), all such connections would share a
             * single rate limiter bucket, allowing one miner to exhaust the budget for others. */
            if(nSessionId == 0)
            {
                debug::error(FUNCTION, "GET_BLOCK rejected: nSessionId is 0 (uninitialized) from ",
                             GetAddress().ToStringIP());
                respond_auto(BLOCK_REJECTED,
                    BuildGetBlockControlPayload(GetBlockPolicyReason::SESSION_INVALID, 0));
                return true;
            }

            LegacyGetBlockRequest req;
            req.nSessionId = nSessionId;
            req.pRateLimiter = &m_getBlockRateLimiter;
            req.fnCreateBlock = [this]() -> TAO::Ledger::Block*
            {
                return new_block();
            };

            LegacyGetBlockResult result = LegacyGetBlockHandler(req);

            if(!result.fSuccess)
            {
                respond_auto(BLOCK_REJECTED,
                    BuildGetBlockControlPayload(result.eReason, result.nRetryAfterMs));

                if(result.eReason == GetBlockPolicyReason::RATE_LIMIT_EXCEEDED)
                {
                    /* Track consecutive rate-limit violations (legacy lane). */
                    const uint32_t nStrikes = ++m_nConsecutiveRateLimitStrikes;

                    if(nStrikes >= MiningConstants::RATE_LIMIT_STRIKE_THRESHOLD)
                    {
                        debug::error(FUNCTION,
                            "Closing miner connection (legacy lane) — ", nStrikes,
                            " consecutive GET_BLOCK rate-limit violations without a"
                            " successful request (tight-loop self-DDoS prevention)"
                            " peer=", GetAddress().ToStringIP());

                        /* SAFETY: lk is std::unique_lock (from LOCK(MUTEX) macro).
                         * Explicit unlock before Disconnect() avoids holding the lock
                         * during socket teardown.  The unique_lock destructor is a
                         * no-op on an already-unlocked lock (owns_lock() == false). */
                        lk.unlock();
                        Disconnect();
                        return true;
                    }

                    /* Suspend reads for retry_after_ms (floor: 1 second). */
                    const uint32_t nSleepMs = std::max(result.nRetryAfterMs, 1000u);

                    /* SAFETY: same pattern — release the lock before sleeping so
                     * other threads (e.g. push notifications) can progress. */
                    lk.unlock();
                    runtime::sleep(nSleepMs);
                    return true;
                }

                return true;
            }

            /* Successful GET_BLOCK — reset the consecutive rate-limit strike counter. */
            m_nConsecutiveRateLimitStrikes = 0;

            /* Invariant: authenticated + in-budget → non-empty BLOCK_DATA */
            assert(!result.vPayload.empty());

            /* BLOCK_DATA (0xD000) is the request-response path: miner explicitly
             * requested a template via GET_BLOCK.  This is distinct from the push
             * path (0xD081 GET_BLOCK) used by SendStatelessTemplate() which proactively
             * delivers templates on chain-tip advance.  Logging this distinction helps
             * miner developers trace latency between the two delivery mechanisms. */
            debug::log(2, FUNCTION, "BLOCK_DATA (request-response) → ", GetAddress().ToStringIP(),
                       " session=", nSessionId, " payload=", result.vPayload.size(), " bytes");

            respond_auto(BLOCK_DATA, result.vPayload);
        }

        /* Update last template channel height after sending template.
         * Uses channel-specific height (not unified) to prevent false template
         * refreshes when other channels mine blocks. */
        {
            TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
            TAO::Ledger::BlockState stateChannel = stateBest;
            if(TAO::Ledger::GetLastState(stateChannel, nChannel))
                nLastTemplateChannelHeight = stateChannel.nChannelHeight;
        }

        /* Notify Colin agent: template pushed via GET_BLOCK */
        if(hashGenesis != 0)
        {
            ColinMiningAgent::Get().on_get_block_received(hashGenesis.SubString(8), false);
        }

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

        /* Write-ahead: update and persist the session BEFORE sending the push notification.
         * If the connection drops after SendChannelNotification but before the session is saved,
         * the recovery cache would hold a stale channel height.  Persisting first ensures the
         * recovered session reflects the correct channel height on reconnect. */
        {
            const std::string strLookupAddr = GetAddress().ToStringIP() + ":" + std::to_string(GetAddress().GetPort());
            auto optCtx = StatelessMinerManager::Get().GetMinerContext(strLookupAddr);
            if(optCtx.has_value() && optCtx->fAuthenticated && optCtx->hashKeyID != 0)
            {
                MiningContext updatedCtx = optCtx.value();

                /* Record the current channel height so recovery can avoid stale-height throttling */
                TAO::Ledger::BlockState stateChannel = TAO::Ledger::ChainState::tStateBest.load();
                uint32_t nChannelHeight = 0;
                if(TAO::Ledger::GetLastState(stateChannel, nChannel))
                    nChannelHeight = stateChannel.nChannelHeight;
                updatedCtx = updatedCtx.WithLastTemplateChannelHeight(nChannelHeight);

                SessionRecoveryManager::Get().SaveSession(updatedCtx);
                debug::log(2, FUNCTION, "Session saved (write-ahead) before push notification: channelHeight=",
                           nChannelHeight, " keyID=", updatedCtx.hashKeyID.SubString());
            }
        }

        /* Send immediate notification.
         * Force-bypass the push throttle — miner explicitly re-subscribed and needs
         * fresh work immediately regardless of when the previous push was sent.
         * Also reset the AutoCoolDown so the recovery GET_BLOCK is served immediately. */
        {
            LOCK(MUTEX);
            m_force_next_push = true;
            // Reassign (not Reset()) — we want Ready() to return true immediately
            // so the recovery GET_BLOCK is served without waiting 2 s.
            // Reset() would START a new 2-second cooldown; reassignment
            // restores the "never triggered" state where Ready() returns true.
            m_get_block_cooldown = AutoCoolDown(std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS));
        }
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
            respond_auto(MINER_AUTH_RESULT, vFail);
            return debug::error(FUNCTION, "Authentication required for stateless miner commands");
        }

        /* R-02: Session consistency gate — validate the authoritative session context
         * persisted in StatelessMinerManager before proceeding with block processing.
         * The live connection context has hashKeyID=0 for legacy-lane miners; the
         * StatelessMinerManager carries the full context populated during auth.
         *
         * Also resolves nCrossLaneSessionId for the cross-lane SUBMIT_BLOCK fallback
         * path (GAP 3 hardening: IP-only lookup if IP:port misses on reconnect). */
        uint32_t nCrossLaneSessionId = nSessionId;   /* default: per-connection session */
        {
            const std::string strLookupAddr = GetAddress().ToStringIP() + ":" + std::to_string(GetAddress().GetPort());
            const auto optCtx = StatelessMinerManager::Get().GetMinerContext(strLookupAddr);
            if(optCtx.has_value())
            {
                nCrossLaneSessionId = optCtx->nSessionId;
                const SessionConsistencyResult consistency = optCtx->ValidateConsistency();
                if(consistency != SessionConsistencyResult::Ok)
                {
                    debug::log(0, FUNCTION, "Session consistency violation at SUBMIT_BLOCK (legacy): ",
                               SessionConsistencyResultString(consistency));
                    respond_auto(BLOCK_REJECTED);
                    return true;
                }
            }
            else
            {
                /* Fallback: IP-only lookup (port may have changed on reconnect) */
                const auto optCtxIP = StatelessMinerManager::Get().GetMinerContextByIP(GetAddress().ToStringIP());
                if(optCtxIP.has_value())
                {
                    nCrossLaneSessionId = optCtxIP->nSessionId;
                    debug::log(1, FUNCTION, "Legacy lane: resolved session via IP-only fallback, session=",
                               nCrossLaneSessionId);
                }
                else
                {
                    debug::log(2, FUNCTION, "Legacy lane: no session context found, cross-lane resolution unavailable");
                }
            }
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
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        if(PACKET.DATA.size() > MAX_SIZE)
        {
            debug::log(0, FUNCTION, "SUBMIT_BLOCK packet too large: ",
                       PACKET.DATA.size(), " > ", MAX_SIZE);
            respond_auto(BLOCK_REJECTED);
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
        std::vector<uint8_t> vPrimeOffsets;

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
                    respond_auto(BLOCK_REJECTED);
                    return true;
                }

                TAO::Ledger::FalconWrappedSubmitBlockParseResult fullBlockSubmission;
                if(TAO::Ledger::VerifyFalconWrappedSubmitBlock(vWorkData, vMinerPubKey, fullBlockSubmission))
                {
                    hashMerkle = fullBlockSubmission.hashMerkle;
                    nonce = fullBlockSubmission.nonce;
                    vPrimeOffsets = fullBlockSubmission.vOffsets;
                    fParsed = true;

                    debug::log(2, FUNCTION, "Disposable Falcon signature verified (legacy lane, shared full-block parser, channel=",
                               fullBlockSubmission.nChannel, ", offsets=", fullBlockSubmission.vOffsets.size(), ")");
                }
                else
                {
                    /* Pure verification — no wrapper creation, no keypair generation.
                     * The node VERIFIES the miner's Disposable Falcon signature using the public
                     * key stored during MINER_AUTH_INIT, then DISCARDS it. The signature is never
                     * forwarded to the Nexus P2P network. */
                    DisposableFalcon::SignedWorkSubmission submission;
                    if(!DisposableFalcon::VerifyWorkSubmission(vWorkData, vMinerPubKey, submission))
                    {
                        debug::error(FUNCTION, "SUBMIT_BLOCK verify failed: Disposable Falcon signature invalid");
                        respond_auto(BLOCK_REJECTED);  // auto-detect lane framing
                        return true;
                    }

                    /* Signature verified — extract merkle root and nonce */
                    hashMerkle = submission.hashMerkleRoot;
                    nonce = submission.nNonce;
                    vPrimeOffsets.clear();
                    fParsed = true;

                    debug::log(2, FUNCTION, "Disposable Falcon signature verified (legacy lane compact wrapper, Port 8323)");
                }
            }
            else
            {
                const uint256_t hashSessionKeyID = !vMinerPubKey.empty() ? LLC::SK256(vMinerPubKey) : uint256_t(0);
                const auto optRecovery = SessionRecoveryManager::Get().RecoverSessionByIdentity(
                    hashSessionKeyID,
                    GetAddress().ToStringIP() + ":" + std::to_string(GetAddress().GetPort())
                );
                const bool fRecoveryGenesisMatches = optRecovery.has_value() &&
                    optRecovery->hashGenesis == hashGenesis;

                debug::warning(FUNCTION, "SUBMIT_BLOCK: ChaCha20 decryption failed before plaintext fallback");
                debug::log(0, FUNCTION, "SUBMIT_BLOCK SESSION DIAGNOSTIC");
                debug::log(0, FUNCTION, "- session id: ", nSessionId);
                debug::log(0, FUNCTION, "- authenticated: ", YesNo(fMinerAuthenticated));
                debug::log(0, FUNCTION, "- connection address: ", GetAddress().ToStringIP(), ":", GetAddress().GetPort());
                debug::log(0, FUNCTION, "- bound reward hash: ", FullHexOrUnset(hashRewardAddress));
                debug::log(0, FUNCTION, "- bound reward source: ",
                           (fRewardBound && hashRewardAddress != 0) ? "current legacy connection reward binding" : "not configured");
                debug::log(0, FUNCTION, "- session genesis used for KDF: ", FullHexOrUnset(hashGenesis));
                debug::log(0, FUNCTION, "- derived key fingerprint: ", KeyFingerprint(vChaChaKey));
                debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(optRecovery.has_value()));
                debug::log(0, FUNCTION, "- recovered session genesis: ",
                           optRecovery.has_value() ? FullHexOrUnset(optRecovery->hashGenesis) : "NOT AVAILABLE");
                debug::log(0, FUNCTION, "- recovered session genesis matches live context: ", YesNo(fRecoveryGenesisMatches));
                debug::log(0, FUNCTION, "- consistency result: FAIL");
            }
        }

        if(!fParsed)
        {
            TAO::Ledger::ParseResult parseResult =
                TAO::Ledger::ParseStatelessWorkSubmission(PACKET.DATA);
            if(!parseResult.success)
            {
                debug::error(FUNCTION, "SUBMIT_BLOCK parse failed: ", parseResult.reason);
                respond_auto(BLOCK_REJECTED);
                return true;
            }

            hashMerkle = parseResult.hashMerkle;
            nonce = parseResult.nonce;
        }

        debug::log(3, FUNCTION, "Block merkle root: ", hashMerkle.SubString(), " nonce: ", nonce);

        /* GAP 1: Pre-fetch cross-lane block BEFORE acquiring MUTEX to avoid
         * holding two locks simultaneously.  FindSessionBlock() acquires
         * m_sessionBlockMutex internally; MUTEX is per-connection.
         * Convention: nSessionId == 0 means "not yet established" (same sentinel
         * used throughout the miner authentication path).
         *
         * Cross-lane block lookup removed — each lane's templates are
         * per-connection only (no shared session block store). */

        LOCK(MUTEX);

        /* Make sure the block was created by this mining server. */
        const bool fLocalBlock = find_block(hashMerkle);

        if(!fLocalBlock)
        {
            debug::log(2, FUNCTION, "Block not found in map");
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::TritiumBlock* pTritium = nullptr;

        {
            /* Local path: sign_block mutates the block in-place via mapBlocks */
            if(!sign_block(nonce, hashMerkle, vPrimeOffsets))
            {
                respond_auto(BLOCK_REJECTED);
                return true;
            }

            pTritium = dynamic_cast<TAO::Ledger::TritiumBlock*>(mapBlocks[hashMerkle]);
        }

        if(!pTritium)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK unexpected non-Tritium block for merkle ",
                         hashMerkle.SubString());
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        /* Diagnostic log — cross-reference with miner's [SUBMIT AUDIT] log. */
        const uint1024_t hashCurrentBest = TAO::Ledger::ChainState::hashBestChain.load();
        debug::log(2, FUNCTION, "[BLOCK SUBMIT] nHeight=", pTritium->nHeight, " (unified)",
                   " channel=", pTritium->nChannel,
                   " hashPrevBlock=", pTritium->hashPrevBlock.SubString(),
                   " hashBestChain=", hashCurrentBest.SubString(),
                   " match=", (pTritium->hashPrevBlock == hashCurrentBest));
        /* Full hashPrevBlock hex (MSB-first via GetHex()) for cross-verification with miner's GetBytes()[0..7] log. */
        debug::log(2, FUNCTION, "[BLOCK SUBMIT] hashPrevBlock FULL (MSB-first): ", pTritium->hashPrevBlock.GetHex());

        /* Hash-based staleness guard — mirrors StakeMinter pattern.
         * hashPrevBlock is the PRIMARY staleness anchor baked into the template.
         * This catches reorgs at the same integer height that nBestHeight misses. */
        if(pTritium->hashPrevBlock != hashCurrentBest)
        {
            debug::log(0, FUNCTION, "SUBMIT_BLOCK rejected STALE — hashPrevBlock=",
                       pTritium->hashPrevBlock.SubString(),
                       " != hashBestChain=", hashCurrentBest.SubString());
            respond_auto(ORPHAN_BLOCK);
            return true;
        }

        /* ── SIM Link deduplication check ─────────────────────────────────────
         *  When a miner runs two simultaneous connections (NexusMiner SIM Link:
         *  one on legacy port 8323, one on stateless port 9323) both lanes may
         *  submit the same solution within milliseconds of each other.  Deduplicate
         *  by caching a hash of (nHeight, nNonce, hashMerkleRoot) for 10 seconds.
         *  Only the first submission is forwarded to ValidateMinedBlock / AcceptMinedBlock.
         *  The second is silently rejected with BLOCK_REJECTED.
         */
        if(ColinMiningAgent::Get().check_and_record_submission(
                pTritium->nHeight, nonce, hashMerkle.GetHex()))
        {
            debug::log(0, FUNCTION, "SUBMIT_BLOCK: Duplicate submission detected "
                       "(height=", pTritium->nHeight, " nonce=", nonce,
                       ") — second connection submission ignored (SIM Link dedup)");
            if(hashGenesis != 0)
            {
                ColinMiningAgent::Get().on_block_submitted(
                    hashGenesis.SubString(8), pTritium->nChannel,
                    false, "DUPLICATE_SUBMISSION");
            }
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        /* ── Merkle root immutability anchor ──
         * After sign_block() the hashMerkleRoot is frozen: it was part of the
         * ProofHash the miner solved against.  No pre-validation step may
         * mutate it or the proof-of-work becomes invalid. */
        const uint512_t hashMerkleFrozen = pTritium->hashMerkleRoot;

        /* Pre-validation vtx check — detect transactions already committed by
         * another block.  Mutating the block to remove them would change the
         * merkle root and invalidate the proof-of-work. */
        if(!TAO::Ledger::ValidateVtxNotCommitted(*pTritium))
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK: vtx already committed — template stale, rejecting");
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        /* Pre-validation producer freshness — detect stale producer sigchain.
         * Mutating the producer would change its hash, changing the merkle root,
         * and invalidating the proof-of-work. */
        if(!TAO::Ledger::ValidateProducerFreshness(*pTritium))
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK: producer sigchain stale — template stale, rejecting");
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        /* Pre-connect vtx sigchain staleness check — detect stale vtx transactions
         * before AcceptMinedBlock() so the miner gets BLOCK_REJECTED and can
         * request a fresh template rather than receiving a false BLOCK_ACCEPTED. */
        if(!TAO::Ledger::ValidateVtxSigchainConsistency(*pTritium))
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK: vtx sigchain stale — rejecting");
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        /* Merkle root immutability assertion — all pre-validation steps above
         * are detection-only and must NOT have mutated the block.  If this
         * fires, a code change has reintroduced a mutation bug. */
        if(pTritium->hashMerkleRoot != hashMerkleFrozen)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK BUG: hashMerkleRoot mutated after sign_block!"
                         " frozen=", hashMerkleFrozen.SubString(),
                         " current=", pTritium->hashMerkleRoot.SubString());
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::BlockValidationResult validationResult =
            TAO::Ledger::ValidateMinedBlock(*pTritium);
        if(!validationResult.valid)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK rejected: ", validationResult.reason);
            if(hashGenesis != 0)
            {
                ColinMiningAgent::Get().on_block_submitted(
                    hashGenesis.SubString(8), pTritium->nChannel,
                    false, validationResult.reason);
            }
            respond_auto(BLOCK_REJECTED);
            return true;
        }

        TAO::Ledger::BlockAcceptanceResult acceptanceResult =
            TAO::Ledger::AcceptMinedBlock(*pTritium);
        if(!acceptanceResult.accepted)
        {
            debug::error(FUNCTION, "SUBMIT_BLOCK ledger write failed: ", acceptanceResult.reason);
            if(hashGenesis != 0)
            {
                ColinMiningAgent::Get().on_block_submitted(
                    hashGenesis.SubString(8), pTritium->nChannel,
                    false, acceptanceResult.reason);
            }
            respond_auto(BLOCK_REJECTED);
        }
        else
        {
            debug::log(0, FUNCTION, "BLOCK ACCEPTED — unified nHeight=", pTritium->nHeight,
                       " channel=", pTritium->nChannel,
                       " hashPrevBlock=", pTritium->hashPrevBlock.SubString(),
                       " merkle=", hashMerkle.SubString());
            respond_auto(BLOCK_ACCEPTED);

            if(hashGenesis != 0)
            {
                ColinMiningAgent::Get().on_block_submitted(
                    hashGenesis.SubString(8), pTritium->nChannel, true, "");
            }
        }
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
            respond_auto(MINER_AUTH_RESULT, vFail);
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
        respond_auto(NEW_ROUND, vResponse);

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
                        /* Build 12-byte metadata prefix + 216-byte block = 228 bytes */
                        uint32_t nUnifiedHeightRound = static_cast<uint32_t>(TAO::Ledger::ChainState::nBestHeight.load());
                        uint32_t nBitsVal = pBlock->nBits;
                        std::vector<uint8_t> vPayload;
                        vPayload.reserve(12 + vBlockData.size());
                        vPayload.push_back((nUnifiedHeightRound  >> 24) & 0xFF);
                        vPayload.push_back((nUnifiedHeightRound  >> 16) & 0xFF);
                        vPayload.push_back((nUnifiedHeightRound  >>  8) & 0xFF);
                        vPayload.push_back((nUnifiedHeightRound       ) & 0xFF);
                        vPayload.push_back((nCurrentChannelHeight >> 24) & 0xFF);
                        vPayload.push_back((nCurrentChannelHeight >> 16) & 0xFF);
                        vPayload.push_back((nCurrentChannelHeight >>  8) & 0xFF);
                        vPayload.push_back((nCurrentChannelHeight      ) & 0xFF);
                        vPayload.push_back((nBitsVal             >> 24) & 0xFF);
                        vPayload.push_back((nBitsVal             >> 16) & 0xFF);
                        vPayload.push_back((nBitsVal             >>  8) & 0xFF);
                        vPayload.push_back((nBitsVal                  ) & 0xFF);
                        vPayload.insert(vPayload.end(), vBlockData.begin(), vBlockData.end());

                        respond_stateless(OpcodeUtility::Stateless::BLOCK_DATA, vPayload);

                        debug::log(2, FUNCTION, "Auto-sent BLOCK_DATA (",
                                   vPayload.size(), " bytes) channel=",
                                   pBlock->nChannel, " height=", pBlock->nHeight,
                                   " [nUnifiedHeight=", nUnifiedHeightRound,
                                   " nChannelHeight=", nCurrentChannelHeight,
                                   " nBits=", nBitsVal, "]");

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


    /* SendNodeShutdown - Notify legacy miner of graceful node shutdown via NODE_SHUTDOWN (0xD0FF) */
    void Miner::SendNodeShutdown(uint32_t nReasonCode)
    {
        if(!m_nodeShutdownNotification.MarkSent())
        {
            debug::log(1, FUNCTION, "NODE_SHUTDOWN already sent to legacy miner ",
                       GetAddress().ToStringIP(), " - skipping duplicate");
            return;
        }

        /* Build 4-byte reason code payload (big-endian) */
        std::vector<uint8_t> vData;
        vData.push_back(static_cast<uint8_t>((nReasonCode >> 24) & 0xFF));
        vData.push_back(static_cast<uint8_t>((nReasonCode >> 16) & 0xFF));
        vData.push_back(static_cast<uint8_t>((nReasonCode >>  8) & 0xFF));
        vData.push_back(static_cast<uint8_t>((nReasonCode >>  0) & 0xFF));

        debug::log(1, FUNCTION, "Sending NODE_SHUTDOWN (0xD0FF) to legacy miner ",
                   GetAddress().ToStringIP(), " reason=", nReasonCode);

        respond_stateless(OpcodeUtility::Stateless::NODE_SHUTDOWN, vData);

        debug::log(1, FUNCTION, "Queued NODE_SHUTDOWN for legacy miner ", GetAddress().ToStringIP(),
                   " buffered=", Buffered(), " bytes");
    }

}
