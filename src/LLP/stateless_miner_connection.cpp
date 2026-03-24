/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/stateless_miner_connection.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/stateless_opcodes.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/falcon_verify.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/auto_cooldown_manager.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/push_notification.h>
#include <LLP/include/mining_constants.h>
#include <LLP/include/session_status.h>
#include <LLP/include/get_block_handler.h>
#include <LLP/templates/events.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <Legacy/include/create.h>
#include <Legacy/types/legacy.h>
#include <Legacy/wallet/wallet.h>

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>
#include <LLC/include/chacha20_helpers.h>
#include <LLC/include/mining_session_keys.h>
#include <LLC/include/falcon_constants_v2.h>
#include <LLC/types/bignum.h>

#include <Util/include/config.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/convert.h>
#include <Util/include/hex.h>

#include <LLP/include/colin_mining_agent.h>
#include <LLP/include/canonical_chain_state.h>
#include <LLP/include/failover_connection_tracker.h>
#include <LLP/include/channel_state_manager.h>
#include <LLP/include/node_session_registry.h>

#include <chrono>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <cassert>

namespace LLP
{
    namespace
    {
        using Diagnostics::FullHexOrUnset;
        using Diagnostics::KeyFingerprint;
        using Diagnostics::YesNo;
        using Diagnostics::PassFail;

        /* KEEPALIVE_V2 wire format is always 8 bytes:
         * session_id (4 bytes little-endian) + prevblock suffix / canary (4 bytes big-endian). */
        static constexpr uint32_t KEEPALIVE_V2_FALLBACK_SIZE = 8;

        inline StatelessPacket BuildSessionExpiredResponse(const uint32_t nSessionId, const uint8_t nReason = 0x01)
        {
            StatelessPacket response(OpcodeUtility::Stateless::SESSION_EXPIRED);
            response.DATA = {
                static_cast<uint8_t>(nSessionId & 0xFF),
                static_cast<uint8_t>((nSessionId >> 8) & 0xFF),
                static_cast<uint8_t>((nSessionId >> 16) & 0xFF),
                static_cast<uint8_t>((nSessionId >> 24) & 0xFF),
                nReason
            };
            response.LENGTH = static_cast<uint32_t>(response.DATA.size());
            return response;
        }

        inline StatelessPacket BuildFallbackKeepaliveV2Packet()
        {
            StatelessPacket packet(OpcodeUtility::Stateless::KEEPALIVE_V2);
            packet.DATA.resize(KEEPALIVE_V2_FALLBACK_SIZE, 0);
            packet.LENGTH = KEEPALIVE_V2_FALLBACK_SIZE;
            return packet;
        }

        std::atomic<uint64_t> g_get_block_requests_total{0};
        std::atomic<uint64_t> g_get_block_rate_limited_total{0};
        std::atomic<uint64_t> g_get_block_blockdata_sent_total{0};
        std::atomic<uint64_t> g_get_block_control_rate_limit_total{0};
        std::atomic<uint64_t> g_get_block_control_session_invalid_total{0};
        std::atomic<uint64_t> g_get_block_control_unauthenticated_total{0};
        std::atomic<uint64_t> g_get_block_control_template_not_ready_total{0};
        std::atomic<uint64_t> g_get_block_control_internal_retry_total{0};
        std::atomic<uint64_t> g_get_block_control_template_stale_total{0};
        std::atomic<uint64_t> g_get_block_control_rebuild_in_progress_total{0};
        std::atomic<uint64_t> g_get_block_control_source_unavailable_total{0};
        std::atomic<uint64_t> g_get_block_control_channel_not_set_total{0};
        std::atomic<uint64_t> g_get_block_legacy_empty_attempt_blocked_total{0};
        std::atomic<uint64_t> g_get_block_silent_drop_total{0};

        uint64_t IncrementControlCounter(const GetBlockPolicyReason eReason)
        {
            switch(eReason)
            {
                case GetBlockPolicyReason::RATE_LIMIT_EXCEEDED:
                    return ++g_get_block_control_rate_limit_total;
                case GetBlockPolicyReason::SESSION_INVALID:
                    return ++g_get_block_control_session_invalid_total;
                case GetBlockPolicyReason::UNAUTHENTICATED:
                    return ++g_get_block_control_unauthenticated_total;
                case GetBlockPolicyReason::TEMPLATE_NOT_READY:
                    return ++g_get_block_control_template_not_ready_total;
                case GetBlockPolicyReason::INTERNAL_RETRY:
                    return ++g_get_block_control_internal_retry_total;
                case GetBlockPolicyReason::TEMPLATE_STALE:
                    return ++g_get_block_control_template_stale_total;
                case GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS:
                    return ++g_get_block_control_rebuild_in_progress_total;
                case GetBlockPolicyReason::TEMPLATE_SOURCE_UNAVAILABLE:
                    return ++g_get_block_control_source_unavailable_total;
                case GetBlockPolicyReason::CHANNEL_NOT_SET:
                    return ++g_get_block_control_channel_not_set_total;
                case GetBlockPolicyReason::NONE:
                    return 0;
                default:
                    return 0;
            }
        }

        bool AllowLegacyEmptyBlockData()
        {
            return config::GetBoolArg("-allow_legacy_empty_block_data", false);
        }
    }

    /* Import opcode constants for stateless mining protocol */
    static constexpr uint16_t MINER_AUTH_INIT = OpcodeUtility::Stateless::AUTH_INIT;
    static constexpr uint16_t MINER_AUTH_CHALLENGE = OpcodeUtility::Stateless::AUTH_CHALLENGE;
    static constexpr uint16_t MINER_AUTH_RESPONSE = OpcodeUtility::Stateless::AUTH_RESPONSE;
    static constexpr uint16_t MINER_AUTH_RESULT = OpcodeUtility::Stateless::AUTH_RESULT;
    static constexpr uint16_t SESSION_START = OpcodeUtility::Stateless::SESSION_START;
    static constexpr uint16_t SESSION_KEEPALIVE = OpcodeUtility::Stateless::SESSION_KEEPALIVE;

    /**
     * DetectedFalconVersionString
     * 
     * Convert detected Falcon version to human-readable string for logging.
     * 
     * @param fDetected Whether version was detected
     * @param version The detected Falcon version
     * @return String like "Falcon-512", "Falcon-1024", or "Unknown"
     */
    inline static std::string DetectedFalconVersionString(bool fDetected, LLC::FalconVersion version)
    {
        if(!fDetected)
            return "Unknown";
        
        return (version == LLC::FalconVersion::FALCON_512) ? "Falcon-512" : "Falcon-1024";
    }

    /**
     * LogFalconSignatureInfo
     * 
     * Log Falcon signature configuration for diagnostics.
     * 
     * @param context Mining context containing version and signature info
     */
    inline static void LogFalconSignatureInfo(const MiningContext& context)
    {
        std::string disposableVersion = DetectedFalconVersionString(context.fFalconVersionDetected, context.nFalconVersion);
        
        debug::log(0, FUNCTION, "   [Disposable: ", disposableVersion, "]");
    }

    /* The block iterator to act as extra nonce. */
    std::atomic<uint32_t> StatelessMinerConnection::nBlockIterator(0);
    
    /* Difficulty cache static variables with padding to prevent false sharing */
    std::atomic<uint64_t> StatelessMinerConnection::nDiffCacheTime(0);
    StatelessMinerConnection::PaddedDifficultyCache StatelessMinerConnection::nDiffCacheValue[3];
    
    /** Default Constructor **/
    StatelessMinerConnection::StatelessMinerConnection()
    : StatelessConnection()
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    , m_pPrimeState(std::make_unique<PrimeStateManager>())
    , m_pHashState(std::make_unique<HashStateManager>())
    , m_getBlockRollingLimiter(RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE, GET_BLOCK_ROLLING_WINDOW)
    {
        /* Log channel manager initialization */
        debug::log(2, FUNCTION, "✓ Channel state managers initialized (Prime + Hash)");
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : StatelessConnection(SOCKET_IN, DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    , m_pPrimeState(std::make_unique<PrimeStateManager>())
    , m_pHashState(std::make_unique<HashStateManager>())
    , m_getBlockRollingLimiter(RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE, GET_BLOCK_ROLLING_WINDOW)
    {
        /* Log channel manager initialization */
        debug::log(2, FUNCTION, "✓ Channel state managers initialized (Prime + Hash)");
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : StatelessConnection(DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    , m_pPrimeState(std::make_unique<PrimeStateManager>())
    , m_pHashState(std::make_unique<HashStateManager>())
    , m_getBlockRollingLimiter(RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE, GET_BLOCK_ROLLING_WINDOW)
    {
        /* Log channel manager initialization */
        debug::log(2, FUNCTION, "✓ Channel state managers initialized (Prime + Hash)");
    }


    /** Default Destructor **/
    StatelessMinerConnection::~StatelessMinerConnection()
    {
        /* Clear session keys */
        {
            std::lock_guard<std::mutex> lock(SESSION_MUTEX);
            mapSessionKeys.clear();
        }
        
        /* Wrapper will auto-cleanup via unique_ptr */
        
        /* Clean up block map */
        LOCK(MUTEX);
        clear_map();
    }


    /** GetCachedDifficulty
     *
     *  Get difficulty with 1-second TTL cache to reduce expensive GetNextTargetRequired() calls.
     *  Cache is shared across all miner connections for consistency.
     *
     *  @param[in] nChannel Mining channel (0=PoS, 1=Prime, 2=Hash)
     *  @return Target difficulty bits for the channel
     *
     **/
    uint32_t StatelessMinerConnection::GetCachedDifficulty(uint32_t nChannel)
    {
        /* Validate channel range */
        if(nChannel > 2)
        {
            debug::error(FUNCTION, "Invalid channel ", nChannel, ", defaulting to Prime (1)");
            nChannel = 1;
        }
        
        /* Check if cache is still valid (within TTL)
         * runtime::unifiedtimestamp() returns seconds, compare with precalculated TTL in seconds
         * Note: Check for clock adjustments (nNow >= nCacheTime) to prevent underflow */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCacheTime = nDiffCacheTime.load(std::memory_order_acquire);
        
        if(nCacheTime > 0 && nNow >= nCacheTime && 
           (nNow - nCacheTime) < MiningConstants::DIFFICULTY_CACHE_TTL_SECONDS)
        {
            /* Cache hit - return cached value */
            uint32_t nCachedDiff = nDiffCacheValue[nChannel].nDifficulty.load(std::memory_order_acquire);
            debug::log(3, FUNCTION, "Difficulty cache HIT for channel ", nChannel, 
                      " (age: ", (nNow - nCacheTime), "s)");
            return nCachedDiff;
        }
        
        /* Cache miss, expired, or clock adjusted backwards - recalculate */
        if(nCacheTime > 0 && nNow < nCacheTime)
        {
            debug::log(2, FUNCTION, "⚠️  Clock adjustment detected (", nCacheTime, " → ", nNow, 
                       ") - invalidating difficulty cache");
        }
        
        /* Cache miss or expired - recalculate
         * Use compare-and-swap to ensure only one thread updates the cache.
         * Other threads will either get the new value or retry with the updated cache. */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        uint32_t nDiff = TAO::Ledger::GetNextTargetRequired(stateBest, nChannel);
        
        /* Validate stateBest hasn't changed during calculation */
        TAO::Ledger::BlockState stateBestCheck = TAO::Ledger::ChainState::tStateBest.load();
        if(stateBest.nHeight != stateBestCheck.nHeight)
        {
            debug::log(3, FUNCTION, "Blockchain advanced during calculation - recalculating");
            stateBest = stateBestCheck;
            nDiff = TAO::Ledger::GetNextTargetRequired(stateBest, nChannel);
            nNow = runtime::unifiedtimestamp();  // Update timestamp after recalculation
        }
        
        /* Try to update cache atomically - only succeeds if timestamp hasn't changed
         * (meaning no other thread beat us to it) */
        uint64_t nExpectedTime = nCacheTime;
        if(nDiffCacheTime.compare_exchange_strong(nExpectedTime, nNow, 
                                                   std::memory_order_release, 
                                                   std::memory_order_acquire))
        {
            /* We won the race - update the cached difficulty value */
            nDiffCacheValue[nChannel].nDifficulty.store(nDiff, std::memory_order_release);
            debug::log(3, FUNCTION, "Difficulty cache MISS for channel ", nChannel, 
                      " - recalculated: 0x", std::hex, nDiff, std::dec);
        }
        else
        {
            /* Another thread updated the cache - use their value to avoid redundant work */
            nDiff = nDiffCacheValue[nChannel].nDifficulty.load(std::memory_order_acquire);
            debug::log(3, FUNCTION, "Difficulty cache race avoided for channel ", nChannel,
                      " - using concurrent update");
        }
        
        return nDiff;
    }


    /** Helper function to format byte arrays as hex strings efficiently.
     *  This is used for diagnostic logging of encrypted/decrypted packet data.
     *  @param data The byte vector to format
     *  @param maxBytes Maximum number of bytes to format (default: all)
     *  @return Hex string representation with spaces between bytes
     */
    static std::string FormatHexDump(const std::vector<uint8_t>& data, size_t maxBytes = std::numeric_limits<size_t>::max())
    {
        /* Lookup table for hex characters (more efficient than snprintf) */
        static const char hex_chars[] = "0123456789abcdef";
        
        size_t count = std::min(data.size(), maxBytes);
        if(count == 0)
            return "";
        
        std::string result;
        result.reserve(count * 3);  // Pre-allocate: 2 hex chars + 1 space per byte
        
        for(size_t i = 0; i < count; ++i)
        {
            uint8_t byte = data[i];
            result += hex_chars[(byte >> 4) & 0x0F];  // High nibble
            result += hex_chars[byte & 0x0F];         // Low nibble
            result += ' ';
        }
        
        /* Remove trailing space if present (defensive check) */
        if(!result.empty() && result.back() == ' ')
            result.pop_back();
        
        return result;
    }


    /** Helper function to split hex dump into multiple lines for readability.
     *  @param hexDump The hex dump string to split (format: "ab cd ef ...")
     *  @param bytesPerLine Number of bytes to show per line (default: 32, max: 256)
     *  @return Vector of lines
     */
    static std::vector<std::string> SplitHexDump(const std::string& hexDump, size_t bytesPerLine = 32)
    {
        std::vector<std::string> lines;
        if(hexDump.empty())
            return lines;
        
        /* Clamp bytesPerLine to reasonable limits to prevent overflow */
        /* Max 256 bytes per line = 768 chars, well within size_t range */
        if(bytesPerLine == 0)
            bytesPerLine = 1;
        if(bytesPerLine > 256)
            bytesPerLine = 256;
        
        /* Each byte is "XX " (3 chars), except last which is "XX" (2 chars) */
        size_t pos = 0;
        while(pos < hexDump.length())
        {
            /* Calculate how many characters to take for this line */
            size_t remaining = hexDump.length() - pos;
            size_t charsThisLine;
            
            /* Safe calculation: bytesPerLine is clamped to [1, 256] so multiplication is safe */
            size_t maxCharsPerLine = bytesPerLine * 3;
            
            if(remaining <= maxCharsPerLine)
            {
                /* Last line or smaller - take everything */
                charsThisLine = remaining;
            }
            else
            {
                /* Full line - calculate exact char count */
                /* We want to split at a byte boundary (space character) */
                charsThisLine = maxCharsPerLine - 1;  // Leave room for missing trailing space
                
                /* If the next character is not a space, adjust to nearest space */
                if(pos + charsThisLine < hexDump.length() && hexDump[pos + charsThisLine] != ' ')
                {
                    /* Find the nearest space before this position */
                    size_t spacePos = hexDump.rfind(' ', pos + charsThisLine);
                    if(spacePos != std::string::npos && spacePos >= pos)
                        charsThisLine = spacePos - pos;
                }
            }
            
            lines.push_back(hexDump.substr(pos, charsThisLine));
            pos += charsThisLine;
            
            /* Skip any spaces at the boundary */
            while(pos < hexDump.length() && hexDump[pos] == ' ')
                pos++;
        }
        
        return lines;
    }


    /** Helper function to convert bytes to uint64_t in little-endian format.
     *  This is needed because Falcon protocol uses little-endian encoding.
     *  @param data The byte vector
     *  @param offset Starting offset in the vector
     *  @param count Number of bytes to read (max 8)
     *  @return uint64_t value in native endianness
     */
    static uint64_t bytes_to_uint64_le(const std::vector<uint8_t>& data, size_t offset, size_t count = 8)
    {
        uint64_t result = 0;
        size_t bytes = std::min(count, size_t(8));
        
        for(size_t i = 0; i < bytes && (offset + i) < data.size(); ++i)
        {
            result |= (uint64_t(data[offset + i]) << (i * 8));
        }
        
        return result;
    }


    /** Helper function to append uint64_t to byte vector in little-endian format.
     *  This is needed because Falcon protocol uses little-endian encoding.
     *  @param vec The byte vector to append to
     *  @param value The uint64_t value to append
     *  @param count Number of bytes to write (max 8, default 8)
     */
    static void append_uint64_le(std::vector<uint8_t>& vec, uint64_t value, size_t count = 8)
    {
        size_t bytes = std::min(count, size_t(8));
        
        for(size_t i = 0; i < bytes; ++i)
        {
            vec.push_back((value >> (i * 8)) & 0xFF);
        }
    }





    /** Handle custom message events. */
    void StatelessMinerConnection::Event(uint8_t EVENT, uint32_t LENGTH)
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
                    StatelessPacket PACKET = this->INCOMING;
                    debug::log(2, FUNCTION, "MinerLLP: HEADER from ", GetAddress().ToStringIP(),
                               " header=0x", std::hex, std::setw(4), std::setfill('0'),
                               uint32_t(PACKET.HEADER), std::dec,
                               " length=", PACKET.LENGTH);
                }
                break;
            }

            /* Handle for a Packet Data Read. */
            case EVENTS::PACKET:
                return;

            /* On Connect Event, Initialize Context. */
            case EVENTS::CONNECT:
            {
                /* Check auto-expiring cooldown FIRST before accepting connection */
                if (AutoCooldownManager::Get().IsInCooldown(GetAddress())) {
                    debug::log(0, FUNCTION, "Connection rejected - IP in cooldown: ", GetAddress().ToStringIP());
                    debug::log(0, FUNCTION, "   This is automated protection, not a ban");
                    debug::log(0, FUNCTION, "   Cooldown will auto-expire - try again later");
                    
                    /* Disconnect immediately */
                    Disconnect();
                    return;
                }
                
                /* Log connection details with remote address and port */
                debug::log(0, FUNCTION, "MinerLLP: New stateless connection from ",
                           GetAddress().ToStringIP(), ":", GetAddress().GetPort());

                /* Initialize context with connection info */
                LOCK(MUTEX);

                /* Create initial context with connection address for auth */
                std::string strAddr = GetAddress().ToStringIP();
                context = MiningContext()
                    .WithTimestamp(runtime::unifiedtimestamp())
                    .WithAuth(false); // Not authenticated yet

                /* Store address in context - needed for building auth message */
                /* Note: MiningContext doesn't have WithAddress method, so we need to */
                /* construct a new context with the address field set */
                context = MiningContext(
                    0,  // nChannel - not set yet
                    TAO::Ledger::ChainState::nBestHeight.load(),  // nHeight - current chain height
                    runtime::unifiedtimestamp(),
                    strAddr,  // strAddress - for Falcon auth message
                    0,  // nProtocolVersion
                    false,  // fAuthenticated
                    0,  // nSessionId
                    uint256_t(0),  // hashKeyID
                    uint256_t(0)   // hashGenesis
                );

                /* Set protocol lane to STATELESS (16-bit opcodes, port 9323) */
                context = context.WithProtocolLane(ProtocolLane::STATELESS);

                /* Attempt session recovery by IP address.
                 * If recovery fails for a non-localhost IP, this is a potential failover
                 * connection — the miner may have switched to this node after its primary
                 * node dropped.  We record it via FailoverConnectionTracker so that after
                 * the subsequent fresh Falcon handshake completes we can notify
                 * ChannelStateManager and update the Colin report. */
                MiningContext recoveredContext;
                bool fRecovered = SessionRecoveryManager::Get().RecoverSessionByAddress(strAddr, recoveredContext);
                if(!fRecovered && strAddr != "127.0.0.1" && strAddr != "::1")
                {
                    FailoverConnectionTracker::Get().RecordConnection(strAddr);
                    debug::log(0, FUNCTION, "No prior session for ", strAddr,
                               " — recording as potential failover connection");
                }

                /* Register with StatelessMinerManager for tracking */
                StatelessMinerManager::Get().UpdateMiner(strAddr, context, 1);

                return;
            }

            /* On Disconnect Event */
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

                debug::log(0, FUNCTION, "MinerLLP: [", strCategory, "] Disconnected from ", GetAddress().ToStringIP(),
                           " reason: ", strReason);

                /* hashKeyID is only considered valid after authenticated Falcon session setup.
                 * Zero remains the sentinel for "no authenticated session", and the registry
                 * also safely no-ops if the key was never registered. */
                if(context.fAuthenticated && context.hashKeyID != 0)
                    NodeSessionRegistry::Get().MarkDisconnected(context.hashKeyID, ProtocolLane::STATELESS);

                /* Remove from StatelessMinerManager tracking */
                {
                    LOCK(MUTEX);
                    StatelessMinerManager::Get().RemoveMiner(context.strAddress);
                }

                return;
            }
        }
    }


    /** Main packet processing using StatelessMiner processor */
    bool StatelessMinerConnection::ProcessPacket()
    {
        try
        {
            /* Reset connection activity timer to prevent idle disconnection on any packet processing */
            this->Reset();

            /* Get the incoming packet. */
            StatelessPacket PACKET = this->INCOMING;
            /* Log entry */
            debug::log(1, FUNCTION, "MinerLLP: ProcessPacket from ", GetAddress().ToStringIP(),
                       " header=0x", std::hex, std::setw(4), std::setfill('0'),
                       uint32_t(PACKET.HEADER), std::dec,
                       " length=", PACKET.LENGTH);

            /* ============================================================================
             * MINER_READY COMPATIBILITY REMAPPING
             * ============================================================================
             * Some miners send non-standard opcodes for MINER_READY:
             *   0x00D8 - 8-bit MINER_READY (216) in 16-bit frame with leading zero
             *   0xD090 - Alternative MINER_READY variant
             * Remap these to the canonical STATELESS_MINER_READY (0xD0D8) before
             * stateless range validation. */
            if(PACKET.HEADER == 0x00D8 || PACKET.HEADER == 0xD090)
            {
                debug::log(1, FUNCTION, "Compatibility: Remapping 0x", std::hex,
                           uint32_t(PACKET.HEADER), std::dec,
                           " → 0xD0D8 (STATELESS_MINER_READY) from ",
                           GetAddress().ToStringIP());
                PACKET.HEADER = StatelessOpcodes::STATELESS_MINER_READY;
            }

            /* Strict stateless lane enforcement (port 9323):
             * reject anything outside 0xD000-0xD0FF with no endian/lane fallback.
             * Exception: un-mirrored opcodes (KEEPALIVE_V2 0xD100, KEEPALIVE_V2_ACK 0xD101)
             * are handled explicitly above before this check. */
            if(!StatelessOpcodes::IsStateless(PACKET.HEADER) &&
               !OpcodeUtility::IsUnmirroredStatelessOpcode(PACKET.HEADER))
            {
                debug::error(FUNCTION, "Invalid stateless opcode: 0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                debug::error(FUNCTION, "  Stateless opcodes must be in range 0xD000-0xD0FF");
                debug::error(FUNCTION, "  Wrong protocol lane on stateless mining port (expected 16-bit framing)");
                debug::error(FUNCTION, "  Rejecting packet from ", GetAddress().ToStringIP());
                return false;
            }

            /* ============================================================================
             * 16-BIT OPCODE HANDLERS (Stateless Mining Protocol for NexusMiner)
             * ============================================================================ */
            
            /* Handle STATELESS_MINER_READY (0xD0D8 = Mirror(216)) - Subscribe to template push notifications */
            if(PACKET.GetOpcode() == StatelessOpcodes::STATELESS_MINER_READY)
            {
                debug::log(2, "📥 === STATELESS_MINER_READY (0xD0D8) REQUEST ===");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Authenticated: ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(0, "   Channel: ", context.nChannel);
                
                /* Validate authentication - required for stateless mining */
                if (!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "STATELESS_MINER_READY: authentication required");
                    debug::log(2, "📥 === STATELESS_MINER_READY: REJECTED (AUTH) ===");
                    return false;
                }
                
                /* CRITICAL: Only Prime (1) and Hash (2) support stateless mining */
                if (context.nChannel != 1 && context.nChannel != 2)
                {
                    debug::error(FUNCTION, "STATELESS_MINER_READY: invalid channel ", context.nChannel);
                    debug::error(FUNCTION, "  Valid: 1 (Prime), 2 (Hash)");
                    debug::error(FUNCTION, "  Stake (0) uses Proof-of-Stake, not mined");
                    debug::log(2, "📥 === STATELESS_MINER_READY: REJECTED (INVALID CHANNEL) ===");
                    return false;
                }

                /* Attempt lane-switch recovery based on live node context */
                context.strAddress = GetAddress().ToStringIP();
                auto optExisting = SessionRecoveryManager::Get().RecoverSessionByIdentity(context);
                if(optExisting.has_value())
                {
                    if(optExisting->fRewardBound && optExisting->hashRewardAddress != 0)
                        context = context.WithRewardAddress(optExisting->hashRewardAddress);

                    if(optExisting->fEncryptionReady && !optExisting->vChaCha20Key.empty())
                        context = context.WithChaChaKey(optExisting->vChaCha20Key);

                    if(!optExisting->vDisposablePubKey.empty())
                    {
                        {
                            std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                            if(optExisting->nSessionId != 0)
                                mapSessionKeys[optExisting->nSessionId] = optExisting->vDisposablePubKey;
                        }
                        /* Fix 2: Propagate recovered key into context so SaveSession re-persists it */
                        context = context.WithDisposableKey(optExisting->vDisposablePubKey,
                                                            optExisting->hashDisposableKeyID);
                    }
                    else
                    {
                        /* Fix 2: Fallback — try RestoreDisposableKey independently in case the
                         * recovery container lost the key. */
                        std::vector<uint8_t> vFallbackPubKey;
                        uint256_t hashFallbackKeyID;
                        if(context.hashKeyID != 0 &&
                           SessionRecoveryManager::Get().RestoreDisposableKey(
                               context.hashKeyID, vFallbackPubKey, hashFallbackKeyID) &&
                           !vFallbackPubKey.empty())
                        {
                            {
                                std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                                if(optExisting->nSessionId != 0)
                                    mapSessionKeys[optExisting->nSessionId] = vFallbackPubKey;
                            }
                            context = context.WithDisposableKey(vFallbackPubKey, hashFallbackKeyID);
                            debug::log(0, FUNCTION, "  Fallback: restored disposable Falcon key from dedicated key store");
                        }
                        else
                        {
                            debug::error(FUNCTION, "  ERROR: disposable Falcon key lost after recovery — block submissions will fail");
                            debug::error(FUNCTION, "  keyID=", FullHexOrUnset(context.hashKeyID));
                            debug::error(FUNCTION, "  Miner must re-authenticate to restore block submission capability");
                        }
                    }

                    debug::log(0, FUNCTION, "Session recovered from lane switch");
                    debug::log(0, FUNCTION, "  session state source: SessionRecoveryManager::RecoverSessionByIdentity");
                    debug::log(0, FUNCTION, "  recovered falcon key id: ", FullHexOrUnset(optExisting->hashKeyID));
                    debug::log(0, FUNCTION, "  recovered session genesis: ", FullHexOrUnset(optExisting->hashGenesis));
                    debug::log(0, FUNCTION, "  recovered reward hash: ", FullHexOrUnset(optExisting->hashRewardAddress));
                    debug::log(0, FUNCTION, "  recovered ChaCha20 key hash: ", FullHexOrUnset(optExisting->hashChaCha20Key));
                    debug::log(0, FUNCTION, "  recovered disposable Falcon key present: ", YesNo(!context.vDisposablePubKey.empty()));
                }
                
                /* Subscribe to notifications (same logic as 8-bit MINER_READY) */
                context = context.WithSubscription(context.nChannel);
                
                /* Ensure encryption is properly set up before mining starts
                 * CRITICAL: ChaCha20 key should have been derived during authentication.
                 * If not present, derive it now from genesis hash. */
                if(context.hashGenesis != 0 && !context.fEncryptionReady)
                {
                    /* Derive ChaCha20 encryption key from genesis hash */
                    context = context.WithChaChaKey(LLC::MiningSessionKeys::DeriveChaCha20Key(context.hashGenesis));
                    
                    debug::log(0, FUNCTION, "✓ Derived ChaCha20 key on MINER_READY");
                    debug::log(0, "   Session genesis used for KDF: ", context.GenesisHex());
                    debug::log(0, "   Derived key fingerprint: ", KeyFingerprint(context.vChaChaKey));
                    debug::log(0, "   Encryption ready: YES");
                }
                
                /* Update last template channel height and persist session BEFORE sending
                 * the template (write-ahead pattern): even if the connection drops after
                 * SaveSession but before the template arrives, the recovered session already
                 * carries the correct channel height, preventing stale-height throttling. */
                {
                    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
                    TAO::Ledger::BlockState stateChannel = stateBest;
                    uint32_t nChannelHeight = 0;
                    if(TAO::Ledger::GetLastState(stateChannel, context.nChannel))
                        nChannelHeight = stateChannel.nChannelHeight;
                    context = context.WithLastTemplateChannelHeight(nChannelHeight);
                }

                /* Persist session and lane state for cross-lane recovery */
                if(context.fAuthenticated && context.hashKeyID != 0)
                {
                    SessionRecoveryManager::Get().SaveSession(context);
                    SessionRecoveryManager::Get().UpdateLane(context.hashKeyID, 1);
                    if(context.fEncryptionReady && !context.vChaChaKey.empty())
                    {
                        uint256_t hashKey(context.vChaChaKey);
                        SessionRecoveryManager::Get().SaveChaCha20State(context.hashKeyID, hashKey, 0);
                    }
                }

                /* Update StatelessMinerManager with COMPLETE context including encryption state */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                
                debug::log(0, FUNCTION, "✓ Miner subscribed to ", 
                          context.strChannelName, " notifications (stateless protocol)");
                debug::log(0, "   Updated StatelessMinerManager with complete context");
                debug::log(0, "   Encryption ready: ", (context.fEncryptionReady ? "YES" : "NO"));
                debug::log(0, "   ChaCha key size: ", context.vChaChaKey.size(), " bytes");
                
                /* Send immediate template push using STATELESS_GET_BLOCK (0xD081 = Mirror(129))
                 * Force-bypass the push throttle — miner explicitly re-subscribed and needs
                 * fresh work immediately regardless of when the previous push was sent. */
                {
                    LOCK(MUTEX);
                    // Reset violation state so a miner that accumulated violations on a
                    // previous session gets a clean slate after re-authentication.
                    // fThrottleMode MUST NOT persist across reconnects — it would permanently
                    // block an authenticated, in-budget miner (violates the "auto-expire"
                    // reversibility principle).  Rolling window counters are NOT reset here;
                    // GetBlockRollingLimiter is keyed by session+lane and expires on its own.
                    m_rateLimit.ResetViolationState();
                    m_force_next_push = true;
                }
                SendStatelessTemplate();
                debug::log(0, FUNCTION, "✓ Recovery template delivered via STATELESS_MINER_READY push — miner should resume mining");

                /* Fix 4: Diagnostic assertion — verify mapSessionKeys is populated after recovery.
                 * A missing key here means SUBMIT_BLOCK signature verification will fail silently. */
                {
                    std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                    const auto itKey = mapSessionKeys.find(context.nSessionId);
                    const bool fKeyPresent = (itKey != mapSessionKeys.end() && !itKey->second.empty());
                    if(!fKeyPresent && context.nSessionId != 0)
                    {
                        debug::error(FUNCTION, "⚠ CRITICAL: mapSessionKeys empty after STATELESS_MINER_READY recovery — block submissions will fail");
                        debug::error(FUNCTION, "  session_id=", context.nSessionId, " key_id=", FullHexOrUnset(context.hashKeyID));
                    }
                    else if(context.nSessionId != 0)
                    {
                        debug::log(1, FUNCTION, "✓ Diagnostic: disposable Falcon key verified in mapSessionKeys for session ", context.nSessionId);
                    }
                }

                debug::log(2, "📥 === STATELESS_MINER_READY: SUCCESS ===");
                return true;
            }

            /* ============================================================================
             * 16-BIT STATELESS OPCODE HANDLERS
             * ============================================================================ */

            /* All opcodes reference OpcodeUtility::Stateless (via StatelessOpcodes alias) */
            using namespace StatelessOpcodes;
            
            /* Tritium block serialization constants */
            constexpr uint32_t TRITIUM_BLOCK_SIZE = 216;        // Total size of serialized Tritium block template
            constexpr uint32_t TRITIUM_OFFSET_NCHANNEL = 196;   // Offset of nChannel field in serialized block
            constexpr uint32_t TRITIUM_OFFSET_NHEIGHT = 200;    // Offset of nHeight field in serialized block

            LOCK(MUTEX);

            /* Handle GET_BLOCK - requires authentication and channel */
            if(PACKET.HEADER == GET_BLOCK)
            {
                /* Count every received GET_BLOCK attempt (including rejected/invalid) to
                 * keep get_block_requests_total aligned with inbound request volume. */
                const uint64_t nRequestsTotal = ++g_get_block_requests_total;
                debug::log(2, FUNCTION, "metric get_block_requests_total=", nRequestsTotal);
                
                debug::log(2, "📥 === GET_BLOCK REQUEST ===");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Authenticated: ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(0, "   Channel: ", context.nChannel);
                debug::log(0, "   Session ID: ", context.nSessionId);

                /* Explicit session validity policy handling. */
                const uint64_t nNow = runtime::unifiedtimestamp();
                if(context.nSessionId == 0 || context.IsSessionExpired(nNow))
                {
                    SendGetBlockControlResponse(GetBlockPolicyReason::SESSION_INVALID, 0, false);
                    return true;
                }
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    SendGetBlockControlResponse(GetBlockPolicyReason::UNAUTHENTICATED, 0, false);
                    debug::log(2, "📥 === GET_BLOCK: REJECTED (AUTH) ===");
                    return true;
                }

                /* ── SIM-LINK: Delegate to SharedGetBlockHandler ─────────────────────
                 * Session-scoped rate limit check, block creation, session block storage,
                 * and 228-byte payload serialization are all handled by the shared handler.
                 *
                 * Rate limiting is now session-scoped (shared 20/60s budget across both
                 * the legacy (8323) and stateless (9323) lanes for this session).
                 *
                 * The CheckRateLimit() call for GET_BLOCK is intentionally removed here;
                 * the session limiter inside SharedGetBlockHandler is the authoritative gate.
                 */

                /* Check channel is set — every path below MUST send a wire response. */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "Channel not set for authenticated in-budget miner; miner must call SET_CHANNEL first — sending TEMPLATE_NOT_READY");
                    debug::log(2, "📥 === GET_BLOCK: REJECTED (NO CHANNEL) ===");
                    SendGetBlockControlResponse(GetBlockPolicyReason::TEMPLATE_NOT_READY, MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS, true);
                    return true;
                }

                debug::log(2, FUNCTION, "Invariant: authenticated + in-budget + channel set → BLOCK_DATA MUST follow (SIM-LINK session-scoped rate check)");
                debug::log(0, "   ✅ Validation passed, delegating to SharedGetBlockHandler");

                /* CRITICAL: Release MUTEX before new_block() via SharedGetBlockHandler —
                 * new_block() acquires MUTEX internally.  std::mutex is non-recursive;
                 * holding lk and calling new_block() causes a deadlock. */
                lk.unlock();

                /* Build request for the shared handler */
                GetBlockRequest gbReq;
                gbReq.context   = context;           /* immutable snapshot (no MUTEX needed after unlock) */
                gbReq.nSessionId = context.nSessionId;
                gbReq.fnCreateBlock = [this]() -> TAO::Ledger::Block*
                {
                    return new_block();
                };

                /* Track GET_BLOCK response latency for observability */
                const auto tGetBlockStart = std::chrono::steady_clock::now();

                GetBlockResult gbResult = SharedGetBlockHandler(gbReq);

                const auto tGetBlockEnd = std::chrono::steady_clock::now();
                const auto nGetBlockLatencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    tGetBlockEnd - tGetBlockStart).count();
                debug::log(2, FUNCTION, "metric get_block_response_latency_ms=", nGetBlockLatencyMs);

                /* Re-acquire MUTEX for respond() and context mutations */
                lk.lock();

                if(!gbResult.fSuccess)
                {
                    if(gbResult.eReason == GetBlockPolicyReason::RATE_LIMIT_EXCEEDED)
                    {
                        const uint64_t nRateLimited = ++g_get_block_rate_limited_total;
                        debug::log(1, FUNCTION, "metric get_block_rate_limited_total=", nRateLimited);
                    }

                    /* Translate INTERNAL_RETRY to TEMPLATE_REBUILD_IN_PROGRESS when deadline exceeded */
                    constexpr int64_t GET_BLOCK_DEADLINE_MS = 500;
                    GetBlockPolicyReason eReportReason = gbResult.eReason;
                    if(eReportReason == GetBlockPolicyReason::INTERNAL_RETRY &&
                       nGetBlockLatencyMs >= GET_BLOCK_DEADLINE_MS)
                    {
                        eReportReason = GetBlockPolicyReason::TEMPLATE_REBUILD_IN_PROGRESS;
                    }

                    SendGetBlockControlResponse(eReportReason, gbResult.nRetryAfterMs, true);
                    debug::log(2, "📥 === GET_BLOCK: FAILED (reason=",
                               GetBlockPolicyReasonCode(eReportReason), " latency=", nGetBlockLatencyMs, "ms) ===");
                    return true;
                }

                /* Invariant: authenticated + in-budget → non-empty BLOCK_DATA */
                assert(!gbResult.vPayload.empty());

                /* Build canonical chain state snapshot using nBits from the created block */
                {
                    TAO::Ledger::BlockState stateGetBlock = TAO::Ledger::ChainState::tStateBest.load();
                    TAO::Ledger::BlockState stateGetBlockCh = stateGetBlock;
                    if(TAO::Ledger::GetLastState(stateGetBlockCh, gbResult.nBlockChannel))
                    {
                        CanonicalChainState canonicalSnap = CanonicalChainState::from_chain_state(
                            stateGetBlock, stateGetBlockCh, gbResult.nBlockBits);
                        debug::log(2, FUNCTION, "GET_BLOCK canonical snapshot: unified=",
                                   canonicalSnap.canonical_unified_height,
                                   " channel=", canonicalSnap.canonical_channel_height,
                                   " drift=", canonicalSnap.height_drift_from_canonical());
                        context = context.WithCanonicalSnap(canonicalSnap);
                    }
                }

                debug::log(0, "   📤 Sending BLOCK_DATA (SIM-LINK)...");
                debug::log(0, "      Packet DATA size: ", gbResult.vPayload.size());
                debug::log(0, "      [channel=", gbResult.nBlockChannel,
                           " height=", gbResult.nBlockHeight,
                           " nBits=", gbResult.nBlockBits, "]");

                SendGetBlockDataResponse(gbResult.vPayload, true);

                debug::log(0, "   ✅ Packet sent!");
                debug::log(2, "📥 === GET_BLOCK: SUCCESS (SIM-LINK) ===");

                /* Update context timestamp, height, last template channel height, and
                 * hashLastBlock snapshot (primary staleness anchor, StakeMinter pattern). */
                {
                    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
                    TAO::Ledger::BlockState stateChannel = stateBest;
                    uint32_t nChannelHeight = 0;
                    if(TAO::Ledger::GetLastState(stateChannel, context.nChannel))
                        nChannelHeight = stateChannel.nChannelHeight;
                    context = context.WithTimestamp(runtime::unifiedtimestamp())
                                     .WithHeight(stateBest.nHeight)
                                     .WithLastTemplateChannelHeight(nChannelHeight)
                                     .WithHashLastBlock(TAO::Ledger::ChainState::hashBestChain.load());
                }

                /* Update manager with new context after template served */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                StatelessMinerManager::Get().IncrementTemplatesServed();

                return true;
            }

            /* Handle SUBMIT_BLOCK - requires authentication and channel */
            /* Unified Hybrid Protocol supports two formats:
             * 1. Legacy format: [merkle_root][nonce (8 bytes)]
             * 2. Falcon-signed format: [merkle_root][nonce (8 bytes)][sig_len (2 bytes)][signature]
             *    Where signature is over (merkle_root || nonce)
             */
            if(PACKET.HEADER == SUBMIT_BLOCK)
            {
                // AUTOMATED RATE LIMIT CHECK (lenient for solutions)
                if (!CheckRateLimit(SUBMIT_BLOCK)) {
                    // Rate limit exceeded - reject submission
                    debug::log(1, FUNCTION, "SUBMIT_BLOCK rate limited for ", GetAddress().ToStringIP());
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    return true;  // Handled (rejected)
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK received ===", ANSI_COLOR_RESET);
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "❌ Authentication required");
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (AUTH) ===", ANSI_COLOR_RESET);
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "❌ Channel not set");
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (NO CHANNEL) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* MANDATORY: ChaCha20 encryption required for all modern miners */
                if(!context.fEncryptionReady || context.vChaChaKey.empty())
                {
                    debug::error(FUNCTION, "❌ REJECTED: ChaCha20 encryption REQUIRED");
                    debug::error(FUNCTION, "   Modern miners MUST use ChaCha20 + Falcon authentication");
                    debug::error(FUNCTION, "   fEncryptionReady: ", context.fEncryptionReady ? "true" : "false");
                    debug::error(FUNCTION, "   vChaChaKey size: ", context.vChaChaKey.size(), " (expected: 32)");
                    debug::error(FUNCTION, "   Legacy plaintext mining is no longer supported");
                    
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    response.DATA.push_back(0x0C);  // Reason: Encryption required
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 encryption required) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* R-02: Session consistency gate — reject any structurally inconsistent session
                 * before attempting key-material access or block validation.
                 * Mirrors the gate in MINER_AUTH and the recovery merge path. */
                {
                    const SessionConsistencyResult consistency = context.ValidateConsistency();
                    if(consistency != SessionConsistencyResult::Ok)
                    {
                        debug::log(0, FUNCTION, "Session consistency violation at SUBMIT_BLOCK: ",
                                   SessionConsistencyResultString(consistency));
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        return true;
                    }
                }

                /* Training Wheels Diagnostic Mode */
                debug::log(2, "════════════════════════════════════════════════════════");
                debug::log(0, "🚀 SUBMIT_BLOCK DIAGNOSTIC (Training Wheels Mode)");
                debug::log(2, "════════════════════════════════════════════════════════");
                
                /* Connection state */
                debug::log(0, "📡 CONNECTION:");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Session ID: 0x", std::hex, context.nSessionId, std::dec);
                debug::log(0, "   Authenticated: ", context.fAuthenticated ? "YES" : "NO");
                debug::log(0, "   Encryption ready: ", context.fEncryptionReady ? "YES" : "NO");
                debug::log(0, "   ChaCha key size: ", context.vChaChaKey.size(), " bytes");
                debug::log(0, "   Channel: ", context.nChannel);
                
                /* Packet info */
                debug::log(0, "📦 ENCRYPTED PACKET:");
                debug::log(0, "   Size: ", PACKET.DATA.size(), " bytes (expected: ~1035 for Tritium)");
                debug::log(0, "   First 64 bytes (hex):");
                {
                    std::string hexDump = FormatHexDump(PACKET.DATA, 64);
                    std::vector<std::string> lines = SplitHexDump(hexDump, 32);
                    for(const auto& line : lines)
                        debug::log(0, "      ", line);
                }
                
                debug::log(2, "════════════════════════════════════════════════════════");
                
                debug::log(2, FUNCTION, "SUBMIT_BLOCK from ", GetAddress().ToStringIP(),
                           " channel=", context.nChannel, " sessionId=", context.nSessionId,
                           " size=", PACKET.DATA.size(),
                           " bound_reward_hash=", FullHexOrUnset(context.hashRewardAddress));

                /* Validate packet size using FalconConstants */
                /* Minimum: merkle(64) + nonce(8) = 72 bytes (legacy format) */
                const size_t MIN_SIZE = FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE;
                
                /* Maximum: full block with ChaCha20 encryption = SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX */
                const size_t MAX_SIZE = FalconConstants::SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX;

                if(PACKET.DATA.size() < MIN_SIZE)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK packet too small: ", 
                               PACKET.DATA.size(), " < ", MIN_SIZE);
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                if(PACKET.DATA.size() > MAX_SIZE)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK packet too large: ",
                               PACKET.DATA.size(), " > ", MAX_SIZE);
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                uint512_t hashMerkle;
                uint64_t nonce = 0;
                bool fFalconVerified = false;
                std::vector<uint8_t> vPrimeOffsets;
                uint32_t nHeightFromBlock  = 0;   // Populated from full-block-body path (offset 200)
                uint32_t nChannelFromBlock = 0;   // Populated from full-block-body path (offset 196)
                bool fHeightFromBlock = false;    // True when nHeightFromBlock was decoded from the block body

                /* Check for Falcon-signed format: [merkle][nonce][timestamp][sig_len][signature] */
                /* Minimum for Falcon format: 64 + 8 + 8 + 2 = 82 bytes */
                const size_t FALCON_MIN_SIZE = FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN;

                if(PACKET.DATA.size() >= FALCON_MIN_SIZE)
                {
                        /* Get session public key */
                        std::vector<uint8_t> vSessionPubKey;
                        {
                            std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                            auto it = mapSessionKeys.find(context.nSessionId);
                            if(it != mapSessionKeys.end())
                            {
                                vSessionPubKey = it->second;
                            }
                        }
                        
                        debug::log(0, "🔐 FALCON SESSION KEY:");
                        debug::log(0, "   Found: ", !vSessionPubKey.empty() ? "YES" : "NO");
                        debug::log(0, "   Size: ", vSessionPubKey.size(), " bytes (expected: ",
                                   (context.nFalconVersion == LLC::FalconVersion::FALCON_1024)
                                       ? FalconConstants::FALCON1024_PUBKEY_SIZE    // 1793
                                       : FalconConstants::FALCON512_PUBKEY_SIZE,    // 897
                                   " for ",
                                   (context.nFalconVersion == LLC::FalconVersion::FALCON_1024) ? "Falcon-1024" : "Falcon-512",
                                   ")");
                        if(!vSessionPubKey.empty() && vSessionPubKey.size() >= 16)
                        {
                            debug::log(0, "   First 16 bytes (hex): ");
                            debug::log(0, "      ", FormatHexDump(vSessionPubKey, 16));
                        }
                        
                        if(vSessionPubKey.empty())
                        {
                            debug::error(FUNCTION, "❌ No Falcon pubkey stored for session");
                            debug::error(FUNCTION, "   Session may have expired or never authenticated properly");
                            
                            StatelessPacket response(STATELESS_BLOCK_REJECTED);
                            response.DATA.push_back(0x0D);  // Reason: No session key
                            response.LENGTH = 1;
                            respond(response);
                            
                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (No Falcon session key) ===", ANSI_COLOR_RESET);
                            return true;
                        }
                        else
                        {
                            /* Packet format: [full_block(216)][timestamp(8)][sig_len(2)][signature] */
                            /* The miner signed: [merkle(64)][nonce(8)][timestamp(8)] */
                            /* UnwrapWorkSubmission expects: [merkle(64)][nonce(8)][timestamp(8)][sig_len(2)][signature] */
                            
                            /* Extract merkle root and nonce from the full block */
                            if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_TRITIUM_MIN + 
                                                     FalconConstants::TIMESTAMP_SIZE + 
                                                     FalconConstants::LENGTH_FIELD_SIZE)
                            {
                                debug::log(2, FUNCTION, "   Extracting merkle and nonce from full block format");
                                
                                /* STEP 1: Decrypt ChaCha20 wrapper (MANDATORY) */
                                std::vector<uint8_t> decryptedData;
                                
                                /* ChaCha20 encryption is MANDATORY at this point because:
                                 * 1. We already checked fEncryptionReady && vChaChaKey earlier
                                 * 2. Legacy plaintext miners are no longer supported
                                 * 3. This ensures all work submissions are protected in transit */
                                
                                debug::log(0, "🔓 CHACHA20 DECRYPTION:");
                                debug::log(0, "   Encrypted payload size: ", PACKET.DATA.size(), " bytes");
                                
                                /* Decrypt using ChaCha20-Poly1305 helper
                                 * Note: No AAD (Additional Authenticated Data) is used here because
                                 * the entire SUBMIT_BLOCK packet is encrypted as-is without domain separation.
                                 * Unlike MINER_SET_REWARD which uses AAD for context binding, SUBMIT_BLOCK
                                 * encrypts the complete payload for transport-layer confidentiality. */
                                bool fDecrypted = LLC::DecryptPayloadChaCha20(
                                    PACKET.DATA,
                                    context.vChaChaKey,
                                    decryptedData
                                );
                                
                                if(!fDecrypted)
                                {
                                    const auto optRecovery = SessionRecoveryManager::Get().RecoverSessionByIdentity(
                                        context.hashKeyID,
                                        GetAddress().ToStringIP()
                                    );
                                    const bool fRecoveryGenesisMatches = optRecovery.has_value() &&
                                        optRecovery->hashGenesis == context.hashGenesis;

                                    debug::log(0, FUNCTION, "ChaCha20 decryption FAILED for SUBMIT_BLOCK");
                                    debug::log(0, FUNCTION, "SUBMIT_BLOCK SESSION DIAGNOSTIC");
                                    debug::log(0, FUNCTION, "- session id: ", context.nSessionId);
                                    debug::log(0, FUNCTION, "- authenticated: ", YesNo(context.fAuthenticated));
                                    debug::log(0, FUNCTION, "- connection address: ", GetAddress().ToStringIP());
                                    debug::log(0, FUNCTION, "- bound reward hash: ", FullHexOrUnset(context.hashRewardAddress));
                                    debug::log(0, FUNCTION, "- bound reward source: ", context.RewardBindingSource());
                                    debug::log(0, FUNCTION, "- submitted reward hash: NOT AVAILABLE (decryption failed before any standalone reward hash could be observed)");
                                    debug::log(0, FUNCTION, "- session genesis used for KDF: ", context.GenesisHex());
                                    debug::log(0, FUNCTION, "- derived key fingerprint: ", KeyFingerprint(context.vChaChaKey));
                                    debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(optRecovery.has_value()));
                                    debug::log(0, FUNCTION, "- recovered session genesis: ",
                                               optRecovery.has_value() ? FullHexOrUnset(optRecovery->hashGenesis) : "NOT AVAILABLE");
                                    debug::log(0, FUNCTION, "- recovered session genesis matches live context: ", YesNo(fRecoveryGenesisMatches));
                                    debug::log(0, FUNCTION, "- AAD used for decryption: '' (0 bytes, empty)");
                                    debug::log(0, FUNCTION, "- encrypted payload size received: ", PACKET.DATA.size(), " bytes");
                                    if(PACKET.DATA.size() >= 12)
                                    {
                                        debug::log(0, FUNCTION, "- nonce (first 12 bytes): ",
                                                   HexStr(PACKET.DATA.begin(), PACKET.DATA.begin() + 12));
                                    }
                                    debug::log(0, FUNCTION, "- consistency result: FAIL");
                                    
                                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                                    response.DATA.push_back(0x0B);  // Reason: ChaCha20 decryption failure
                                    response.LENGTH = 1;
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 decryption failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }
                                
                                debug::log(0, "   Status: ✅ SUCCESS");
                                debug::log(0, "   Decrypted size: ", decryptedData.size(), " bytes");
                                debug::log(0, "   First 64 bytes (hex):");
                                {
                                    std::string hexDump = FormatHexDump(decryptedData, 64);
                                    std::vector<std::string> lines = SplitHexDump(hexDump, 32);
                                    for(const auto& line : lines)
                                        debug::log(0, "      ", line);
                                }
                                
                                /* ===== NODE-SIDE PLAINTEXT LAYOUT DIAGNOSTIC =====
                                 * Log the decrypted payload's byte layout so we can cross-reference with miner's
                                 * PLAINTEXT LAYOUT DIAGNOSTIC output. Run at log level 1. */
                                debug::log(1, FUNCTION, "📐 NODE PLAINTEXT LAYOUT DIAGNOSTIC:");
                                debug::log(1, FUNCTION, "   Decrypted payload size: ", decryptedData.size(), " bytes");
                                if(decryptedData.size() >= FalconConstants::TIMESTAMP_SIZE) {
                                    debug::log(1, FUNCTION, "   First 8 bytes (hex): ",
                                               HexStr(decryptedData.begin(), decryptedData.begin() + std::min<size_t>(FalconConstants::TIMESTAMP_SIZE, decryptedData.size())));
                                }
                                if(decryptedData.size() >= FalconConstants::FULL_BLOCK_TRITIUM_MIN) {
                                    /* Probe sig_len at two possible offsets (depends on block size) */
                                    size_t probe_tritium = FalconConstants::FULL_BLOCK_TRITIUM_MIN + FalconConstants::TIMESTAMP_SIZE;  // offset if block is 216 bytes
                                    size_t probe_legacy  = FalconConstants::FULL_BLOCK_LEGACY_MIN  + FalconConstants::TIMESTAMP_SIZE;  // offset if block is 220 bytes
                                    if(decryptedData.size() > probe_tritium + 1) {
                                        uint16_t probe_siglen = static_cast<uint16_t>(decryptedData[probe_tritium]) |
                                                                (static_cast<uint16_t>(decryptedData[probe_tritium+1]) << 8);
                                        debug::log(1, FUNCTION, "   Probe: sig_len at offset ", probe_tritium,
                                                   " (assumes ", FalconConstants::FULL_BLOCK_TRITIUM_MIN, "-byte block) = ", probe_siglen,
                                                   " (valid range: ", FalconConstants::FALCON_SIG_MIN, "-", FalconConstants::FALCON_SIG_MAX_VALIDATION, ")");
                                    }
                                    if(decryptedData.size() > probe_legacy + 1) {
                                        uint16_t probe_siglen = static_cast<uint16_t>(decryptedData[probe_legacy]) |
                                                                (static_cast<uint16_t>(decryptedData[probe_legacy+1]) << 8);
                                        debug::log(1, FUNCTION, "   Probe: sig_len at offset ", probe_legacy,
                                                   " (assumes ", FalconConstants::FULL_BLOCK_LEGACY_MIN, "-byte block) = ", probe_siglen,
                                                   " (valid range: ", FalconConstants::FALCON_SIG_MIN, "-", FalconConstants::FALCON_SIG_MAX_VALIDATION, ")");
                                    }
                                }
                                /* ===== END NODE DIAGNOSTIC ===== */
                                
                                /* STEP 2: Extract from DECRYPTED data */
                                debug::log(0, "📊 DATA EXTRACTION:");
                                TAO::Ledger::FalconWrappedSubmitBlockParseResult fullBlockSubmission;
                                if(!TAO::Ledger::VerifyFalconWrappedSubmitBlock(decryptedData, vSessionPubKey, fullBlockSubmission))
                                {
                                    debug::error(FUNCTION, "❌ Falcon signature verification FAILED");
                                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                                    response.DATA.push_back(0x0C);  // Reason: Signature verification failed
                                    response.LENGTH = 1;
                                    respond(response);

                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Signature verification failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                debug::log(0, "📝 SIGNATURE VERIFICATION:");
                                debug::log(0, "   Block size: ", fullBlockSubmission.vBlockBytes.size(), " bytes");
                                debug::log(0, "   Signature length: ", fullBlockSubmission.nSignatureLength, " bytes");
                                debug::log(0, "   Total packet size: ", decryptedData.size(), " bytes");
                                debug::log(0, "   Channel: ", fullBlockSubmission.nChannel);
                                debug::log(0, "   Prime offsets: ", fullBlockSubmission.vOffsets.size(), " elements");
                                debug::log(0, "   Timestamp: ", fullBlockSubmission.timestamp);
                                debug::log(0, "   Merkle: ", fullBlockSubmission.hashMerkle.SubString());
                                debug::log(0, "   Nonce: 0x", std::hex, fullBlockSubmission.nonce, std::dec);
                                debug::log(2, FUNCTION, "✅ Disposable ",
                                          DetectedFalconVersionString(context.fFalconVersionDetected, context.nFalconVersion),
                                          " signature verified (shared full-block parser, ", fullBlockSubmission.vSignature.size(), " bytes)");
                                debug::log(2, "════════════════════════════════════════════════════════");

                                hashMerkle = fullBlockSubmission.hashMerkle;
                                nonce = fullBlockSubmission.nonce;
                                vPrimeOffsets = fullBlockSubmission.vOffsets;
                                fFalconVerified = true;
                                nChannelFromBlock = fullBlockSubmission.nChannel;
                                nHeightFromBlock = fullBlockSubmission.nUnifiedHeight;
                                /* Sanity-bound: reject heights implausibly far above any realistic chain.
                                 * 2,000,000,000 is far beyond any conceivable Tritium chain height while
                                 * still blocking the dangerous UINT32_MAX adversarial value. */
                                static constexpr uint32_t MAX_PLAUSIBLE_BLOCK_HEIGHT = 2'000'000'000u;  // ~2 B blocks
                                fHeightFromBlock = (nChannelFromBlock == 1 || nChannelFromBlock == 2)
                                                && nHeightFromBlock > 0
                                                && nHeightFromBlock < MAX_PLAUSIBLE_BLOCK_HEIGHT;
                                const uint64_t nTimestamp = fullBlockSubmission.timestamp;
                                
                                /* Check timestamp freshness (replay protection) */
                                /* NOTE: FALCON_TIMESTAMP_TOLERANCE_SECONDS is defined locally here
                                 * rather than in FalconConstants because it's a security policy parameter
                                 * for timestamp validation, not a protocol-level constant. It may be
                                 * made configurable in the future via command-line args or config file. */
                                const uint64_t FALCON_TIMESTAMP_TOLERANCE_SECONDS = 30;
                                
                                uint64_t nCurrentTime = std::chrono::duration_cast<std::chrono::seconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
                                
                                /* SECURITY: Safe timestamp comparison to prevent integer overflow attacks
                                 * We avoid casting to int64_t which could overflow with malicious timestamps.
                                 * Instead, we directly compare uint64_t values to determine the time difference. */
                                uint64_t nTimeDiff = 0;
                                
                                if(nTimestamp > nCurrentTime)
                                {
                                    /* Timestamp is in the future */
                                    nTimeDiff = nTimestamp - nCurrentTime;
                                }
                                else
                                {
                                    /* Timestamp is in the past */
                                    nTimeDiff = nCurrentTime - nTimestamp;
                                }
                                
                                /* Allow clock skew up to tolerance limit */
                                if(nTimeDiff > FALCON_TIMESTAMP_TOLERANCE_SECONDS)
                                {
                                    debug::error(FUNCTION, "❌ Falcon signature timestamp too old (", nTimeDiff, "s skew)");
                                    debug::error(FUNCTION, "   This prevents replay attacks");
                                    
                                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                                    response.DATA.push_back(0x08);  // Reason: stale timestamp
                                    response.LENGTH = 1;
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Stale Falcon signature) ===", ANSI_COLOR_RESET);
                                    return true;
                                }
                            }
                            else
                            {
                                /* Fallback: legacy Falcon wrapper [merkle][nonce][timestamp][sig_len][signature] */
                                std::vector<uint8_t> decryptedData;
                                if(!LLC::DecryptPayloadChaCha20(PACKET.DATA, context.vChaChaKey, decryptedData))
                                {
                                    const auto optRecovery = SessionRecoveryManager::Get().RecoverSessionByIdentity(
                                        context.hashKeyID,
                                        GetAddress().ToStringIP()
                                    );
                                    const bool fRecoveryGenesisMatches = optRecovery.has_value() &&
                                        optRecovery->hashGenesis == context.hashGenesis;
                                    debug::error(FUNCTION, "❌ ChaCha20 decryption FAILED (legacy wrapper)");
                                    debug::log(0, FUNCTION, "SUBMIT_BLOCK SESSION DIAGNOSTIC");
                                    debug::log(0, FUNCTION, "- session id: ", context.nSessionId);
                                    debug::log(0, FUNCTION, "- authenticated: ", YesNo(context.fAuthenticated));
                                    debug::log(0, FUNCTION, "- connection address: ", GetAddress().ToStringIP());
                                    debug::log(0, FUNCTION, "- bound reward hash: ", FullHexOrUnset(context.hashRewardAddress));
                                    debug::log(0, FUNCTION, "- bound reward source: ", context.RewardBindingSource());
                                    debug::log(0, FUNCTION, "- session genesis used for KDF: ", context.GenesisHex());
                                    debug::log(0, FUNCTION, "- derived key fingerprint: ", KeyFingerprint(context.vChaChaKey));
                                    debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(optRecovery.has_value()));
                                    debug::log(0, FUNCTION, "- recovered session genesis: ",
                                               optRecovery.has_value() ? FullHexOrUnset(optRecovery->hashGenesis) : "NOT AVAILABLE");
                                    debug::log(0, FUNCTION, "- recovered session genesis matches live context: ", YesNo(fRecoveryGenesisMatches));
                                    debug::log(0, FUNCTION, "- consistency result: FAIL");
                                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                                    response.DATA.push_back(0x0B);  // Reason: ChaCha20 decryption failure
                                    response.LENGTH = 1;
                                    respond(response);
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 decryption failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                /* ✅ CORRECT: Pure verification — no wrapper creation, no keypair generation */
                                LLP::DisposableFalcon::SignedWorkSubmission submission;
                                if(!LLP::DisposableFalcon::VerifyWorkSubmission(decryptedData, vSessionPubKey, submission))
                                {
                                    debug::error(FUNCTION, "❌ Disposable Falcon verification failed (legacy format)");
                                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                                    response.DATA.push_back(0x0C);  // Reason: Signature verification failed
                                    response.LENGTH = 1;
                                    respond(response);
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Signature verification failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                /* Signature verified — it is now DISCARDED (not forwarded to network) */
                                hashMerkle = submission.hashMerkleRoot;
                                nonce = submission.nNonce;
                                vPrimeOffsets.clear();
                                fFalconVerified = true;

                                debug::log(2, FUNCTION, "✅ Disposable Falcon signature verified (legacy format)");
                            }
                        }
                }
                else
                {
                    /* Packet too small for Falcon format - reject it */
                    debug::error(FUNCTION, "❌ Packet too small for Falcon-signed format");
                    debug::error(FUNCTION, "   Expected at least: ", FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN, " bytes");
                    debug::error(FUNCTION, "   Got: ", PACKET.DATA.size(), " bytes");
                    debug::error(FUNCTION, "   Legacy plaintext mining is no longer supported");
                    
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    response.DATA.push_back(0x0F);  // Reason: Packet too small
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Packet too small) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* At this point, fFalconVerified MUST be true because we enforced */
                /* ChaCha20 encryption and Falcon signatures above. If it's not true, */
                /* something went wrong in the logic above. */
                if(!fFalconVerified)
                {
                    debug::error(FUNCTION, "❌ LOGIC ERROR: Reached unreachable code!");
                    debug::error(FUNCTION, "   fFalconVerified should always be true at this point");
                    debug::error(FUNCTION, "   This indicates a bug in the SUBMIT_BLOCK handler");
                    
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    response.DATA.push_back(0xFF);  // Reason: Internal error
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Internal error) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* Continue with existing template lookup and validation... */
                StatelessMinerManager::Get().IncrementBlocksSubmitted();

                /* GAP 1: keep the cross-lane block alive through validation */
                std::shared_ptr<TAO::Ledger::Block> spCrossLaneHolder;
                std::unique_ptr<TAO::Ledger::Block> upCrossLaneClone;
                bool fCrossLane = false;

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
                {
                    /* DEPRECATED: SIM-LINK cross-lane template resolution.
                     *
                     * Cross-lane block lookup is scheduled for removal once real
                     * second-node failover (DualConnectionManager) is complete.
                     * New miners should connect to a dedicated failover node rather
                     * than relying on this cross-lane resolution path.
                     *
                     * To disable this fallback now and test the new failover model,
                     * start the node with -deprecate-simlink-fallback=1.
                     *
                     * See: docs/architecture/SIMLINK_DUAL_LANE_ARCHITECTURE.md */
                    if(!config::GetBoolArg("-deprecate-simlink-fallback", false))
                    {
                        /* Cross-lane fallback: template may have been issued on the
                         * legacy port (8323) and stored in the session block map by
                         * SharedGetBlockHandler on that lane. */
                        spCrossLaneHolder = StatelessMinerManager::Get().FindSessionBlock(
                            context.nSessionId, hashMerkle);

                        if(spCrossLaneHolder)
                            debug::log(0, FUNCTION,
                                "[DEPRECATED] SIM-LINK: cross-lane block resolved from legacy lane "
                                "session=", context.nSessionId, " merkle=", hashMerkle.SubString(),
                                " — consider connecting to a dedicated failover node instead");
                    }
                    else
                    {
                        debug::log(0, FUNCTION,
                            "[DEPRECATED] SIM-LINK: cross-lane fallback disabled via "
                            "-deprecate-simlink-fallback; treating as unknown template");
                    }

                    if(!spCrossLaneHolder)
                    {
                        debug::error(FUNCTION, "════════════════════════════════════════");
                        debug::error(FUNCTION, "   ❌ TEMPLATE NOT FOUND");
                        debug::error(FUNCTION, "════════════════════════════════════════");
                        debug::error(FUNCTION, "Submitted merkle root: ", hashMerkle.SubString());
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "Current blockchain state:");
                        debug::error(FUNCTION, "  Height: ", TAO::Ledger::ChainState::nBestHeight.load());
                        debug::error(FUNCTION, "  Synchronizing: ", TAO::Ledger::ChainState::Synchronizing() ? "YES" : "NO");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "Known templates (", mapBlocks.size(), " total):");
                        
                        if(mapBlocks.empty())
                        {
                            debug::error(FUNCTION, "  (none - all templates expired)");
                        }
                        else
                        {
                            for(const auto& entry : mapBlocks)
                            {
                                const TemplateMetadata& meta = entry.second;
                                uint64_t nAge = runtime::unifiedtimestamp() - meta.nCreationTime;
                                
                                debug::error(FUNCTION, "  ✓ ", entry.first.SubString());
                                debug::error(FUNCTION, "    Height: ", meta.nHeight, 
                                           " (current: ", TAO::Ledger::ChainState::nBestHeight.load() + 1, ")");
                                debug::error(FUNCTION, "    Channel Height: ", meta.nChannelHeight);
                                debug::error(FUNCTION, "    Age: ", nAge, "s (max: ", LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS, "s)");
                                debug::error(FUNCTION, "    Channel: ", meta.GetChannelName());
                                debug::error(FUNCTION, "    Valid: ", !meta.IsStale() ? "YES" : "NO");
                            }
                        }
                        
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "COMMON CAUSES:");
                        debug::error(FUNCTION, "  1. Template expired (height changed during mining)");
                        debug::error(FUNCTION, "     → Solution: Poll GET_ROUND more frequently");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "  2. Miner computed wrong merkle root");
                        debug::error(FUNCTION, "     → Solution: Check miner's merkle calculation");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "  3. Miner mining stale template (>60s old)");
                        debug::error(FUNCTION, "     → Solution: Reduce work time per template");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "  4. Template cleanup removed it");
                        debug::error(FUNCTION, "     → Solution: Request new template immediately");
                        debug::error(FUNCTION, "════════════════════════════════════════");
                        
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Unknown template) ===", ANSI_COLOR_RESET);
                        return true;
                    }

                    debug::log(1, FUNCTION, "SIM-LINK cross-lane SUBMIT_BLOCK resolved: session=",
                               context.nSessionId, " merkle=", hashMerkle.SubString());
                    upCrossLaneClone = std::unique_ptr<TAO::Ledger::Block>(spCrossLaneHolder->Clone());
                    if(!upCrossLaneClone)
                    {
                        debug::error(FUNCTION, "❌ SIM-LINK cross-lane block clone failed");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        return true;
                    }
                    fCrossLane = true;
                }

                TAO::Ledger::TritiumBlock* pTritium = nullptr;

                if(!fCrossLane)
                {
                    debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Found original template (wallet-signed)", ANSI_COLOR_RESET);

                    /* Get iterator to block template for processing */
                    auto it = mapBlocks.find(hashMerkle);
                    if(it == mapBlocks.end())
                    {
                        debug::error(FUNCTION, "❌ Template lookup failed (race condition)");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (template lookup failed) ===", ANSI_COLOR_RESET);
                        return true;
                    }

                    /* ── Canonical pre-check gate (WARN-ONLY) ───────────────────────────
                     *  Use the snapshot captured at GET_BLOCK / push time (MiningContext).
                     *  Node's validate_block() + ledger remain the final authority on
                     *  acceptance; these checks only emit diagnostic warnings.
                     */
                    {
                        const CanonicalChainState& snap = context.canonical_snap;

                        const bool fSnapStale = snap.is_canonically_stale();
                        const uint64_t nSnapAgeMs = snap.is_initialized()
                            ? static_cast<uint64_t>(
                                std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - snap.canonical_received_at).count())
                            : 0;

                        if(fSnapStale)
                        {
                            debug::warning(FUNCTION, "SUBMIT_BLOCK pre-check: canonical snapshot stale (>30s) — proceeding with caution");
                        }

                    /* Compare template height against canonical unified height (WARN only).
                     * Prefer the Falcon-authenticated miner-submitted height (nHeightFromBlock)
                     * when available (full-block-body decode path); fall back to the stored
                     * template height for the legacy wrapper path. */
                    const uint32_t nTemplateHeight = it->second.pBlock ? it->second.pBlock->nHeight : 0;
                    const uint32_t nCompareHeight  = fHeightFromBlock ? nHeightFromBlock : nTemplateHeight;
                    if(nCompareHeight > 0 && snap.canonical_unified_height > 0 &&
                       nCompareHeight != snap.canonical_unified_height)
                    {
                        debug::warning(FUNCTION, "SUBMIT_BLOCK height mismatch: ",
                                       fHeightFromBlock ? "submitted" : "template",
                                       " height=", nCompareHeight,
                                       " canonical=", snap.canonical_unified_height,
                                       " — may be stale template");
                    }

                    /* Notify Colin so the periodic report reflects staleness at submission time,
                     * not just at template-issue time. */
                    if(context.hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_canonical_snap_updated(
                            context.hashGenesis.SubString(8), nSnapAgeMs, fSnapStale);
                    }
                }

                    /* Make sure there is no inconsistencies in signing block. */
                    if(!sign_block(nonce, hashMerkle, vPrimeOffsets))
                    {
                        debug::error(FUNCTION, "❌ sign_block failed (nonce update failed)");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (sign_block failed) ===", ANSI_COLOR_RESET);
                        return true;
                    }

                    pTritium = dynamic_cast<TAO::Ledger::TritiumBlock*>(it->second.pBlock.get());
                    if(!pTritium)
                    {
                        debug::error(FUNCTION, "❌ invalid block type (expected TritiumBlock)");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (invalid block type) ===", ANSI_COLOR_RESET);
                        return true;
                    }
                } /* end if(!fCrossLane) */
                else
                {
                    /* Cross-lane path: clone the session-store block before applying
                     * nonce/offsets so the other lane never observes mutated template state.
                     * NOTE: This channel-dispatch logic mirrors the cross-lane path in
                     * Miner::handle_submit_block_stateless (miner.cpp). */
                    pTritium = dynamic_cast<TAO::Ledger::TritiumBlock*>(upCrossLaneClone.get());
                    if(!pTritium)
                    {
                        debug::error(FUNCTION, "❌ SIM-LINK cross-lane block is not a TritiumBlock");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        return true;
                    }

                    if(pTritium->nChannel == TAO::Ledger::CHANNEL::PRIME)
                    {
                        *pTritium = TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(
                            *pTritium, nonce, vPrimeOffsets);

                        if(!pTritium->vOffsets.empty() &&
                           !TAO::Ledger::VerifySubmittedPrimeOffsets(*pTritium, pTritium->vOffsets))
                        {
                            debug::error(FUNCTION, "Cross-lane: Prime vOffsets structural validation failed");
                            StatelessPacket response(STATELESS_BLOCK_REJECTED);
                            respond(response);
                            return true;
                        }

                        if(pTritium->vOffsets.empty())
                            TAO::Ledger::GetOffsets(pTritium->GetPrime(), pTritium->vOffsets);
                    }
                    else if(pTritium->nChannel == TAO::Ledger::CHANNEL::HASH)
                    {
                        *pTritium = TAO::Ledger::BuildSolvedHashCandidateFromTemplate(*pTritium, nonce);
                    }
                    else
                    {
                        pTritium->nNonce = nonce;
                        pTritium->vOffsets.clear();
                        pTritium->vchBlockSig.clear();
                    }

                    if(!TAO::Ledger::FinalizeWalletSignatureForSolvedBlock(*pTritium))
                    {
                        debug::error(FUNCTION, "Cross-lane: FinalizeWalletSignatureForSolvedBlock failed");
                        StatelessPacket response(STATELESS_BLOCK_REJECTED);
                        respond(response);
                        return true;
                    }
                }

                /* Hash-based staleness guard — mirrors StakeMinter pattern.
                 * hashPrevBlock is the PRIMARY staleness anchor baked into the 216-byte template.
                 * This catches reorgs at the same integer height that nBestHeight misses. */
                const uint1024_t hashCurrentBest = TAO::Ledger::ChainState::hashBestChain.load();
                debug::log(2, FUNCTION, "[BLOCK SUBMIT] nHeight=", pTritium->nHeight, " (unified)",
                           " channel=", pTritium->nChannel,
                           " hashPrevBlock=", pTritium->hashPrevBlock.SubString(),
                           " hashBestChain=", hashCurrentBest.SubString(),
                           " match=", (pTritium->hashPrevBlock == hashCurrentBest));
                /* Full hashPrevBlock hex (MSB-first via GetHex()) for cross-verification with miner's GetBytes()[0..7] log. */
                debug::log(2, FUNCTION, "[BLOCK SUBMIT] hashPrevBlock FULL (MSB-first): ", pTritium->hashPrevBlock.GetHex());

                if(pTritium->hashPrevBlock != hashCurrentBest)
                {
                    debug::log(0, FUNCTION, "SUBMIT_BLOCK rejected STALE — hashPrevBlock=",
                               pTritium->hashPrevBlock.SubString(),
                               " != hashBestChain=", hashCurrentBest.SubString());
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    response.DATA.push_back(static_cast<uint8_t>(OpcodeUtility::RejectionReason::STALE));
                    response.LENGTH = static_cast<uint32_t>(response.DATA.size());
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Stale block) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* ── SIM Link deduplication check ────────────────────────────────
                 *  Shared dedup cache (via ColinMiningAgent) detects when the same
                 *  solution is submitted on both the stateless lane (this handler,
                 *  port 9323) and the legacy lane (miner.cpp, port 8323).
                 */
                if(ColinMiningAgent::Get().check_and_record_submission(
                        pTritium->nHeight, nonce, hashMerkle.GetHex()))
                {
                    debug::log(0, FUNCTION, "SUBMIT_BLOCK: Duplicate submission detected "
                               "(height=", pTritium->nHeight, " nonce=", nonce,
                               ") — second connection submission ignored (SIM Link dedup)");
                    if(context.hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_block_submitted(
                            context.hashGenesis.SubString(8), pTritium->nChannel,
                            false, "DUPLICATE_SUBMISSION");
                    }
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                TAO::Ledger::BlockValidationResult validationResult =
                    TAO::Ledger::ValidateMinedBlock(*pTritium);
                if(!validationResult.valid)
                {
                    debug::error(FUNCTION, "❌ ValidateMinedBlock failed: ", validationResult.reason);
                    
                    /* Notify Colin agent on rejection */
                    if(context.hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_block_submitted(
                            context.hashGenesis.SubString(8), pTritium->nChannel,
                            false, validationResult.reason);
                    }

                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (", validationResult.reason, ") ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* PoW fully validated — notify the miner immediately so it can proceed
                 * to the next template without waiting for the ledger write in
                 * AcceptMinedBlock() / Process(), which may take hundreds of milliseconds.
                 * The block has been consensus-verified; the miner's obligation is fulfilled. */
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✅ Block validated — sending STATELESS_BLOCK_ACCEPTED immediately", ANSI_COLOR_RESET);
                debug::log(0, FUNCTION, "BLOCK ACCEPTED — unified nHeight=", pTritium->nHeight,
                           " channel=", pTritium->nChannel,
                           " hashMerkleRoot=", pTritium->hashMerkleRoot.SubString(),
                           " hashPrevBlock (validated == hashBestChain)=", pTritium->hashPrevBlock.SubString());

                /* Log signature configuration (PR #122) */
                LogFalconSignatureInfo(context);

                /* Get block for detailed logging */
                {
                    auto itLog = mapBlocks.find(hashMerkle);
                    if(itLog != mapBlocks.end() && itLog->second.pBlock)
                    {
                        TAO::Ledger::Block *pBlock = itLog->second.pBlock.get();
                        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   🎉 Block ", pBlock->nHeight, " submitted to Nexus network", ANSI_COLOR_RESET);
                        debug::log(0, "   Miner: ", GetAddress().ToStringIP());
                        debug::log(0, "   Channel: ", pBlock->nChannel, " (", MiningContext::ChannelName(pBlock->nChannel), ")");
                    }
                }

                {
                    StatelessPacket response(STATELESS_BLOCK_ACCEPTED);
                    respond(response);
                }
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK: SUCCESS ===", ANSI_COLOR_RESET);
                debug::log(2, FUNCTION, "✓ Disposable Falcon signature discarded after SUBMIT_BLOCK");

                TAO::Ledger::BlockAcceptanceResult acceptanceResult =
                    TAO::Ledger::AcceptMinedBlock(*pTritium);
                if(!acceptanceResult.accepted)
                {
                    /* STATELESS_BLOCK_ACCEPTED was already sent — the block's PoW was valid.
                     * The ledger-write failure (duplicate race, orphan, etc.) is a
                     * node-side issue; do not confuse the miner with a follow-up
                     * rejection.  Log clearly so operators can investigate. */
                    debug::error(FUNCTION, "❌ AcceptMinedBlock ledger write failed after STATELESS_BLOCK_ACCEPTED sent: ", acceptanceResult.reason);

                    /* Notify Colin agent on ledger-write failure */
                    if(context.hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_block_submitted(
                            context.hashGenesis.SubString(8), pTritium->nChannel,
                            false, acceptanceResult.reason);
                    }
                }
                else
                {
                    /* Block accepted - track in manager */
                    StatelessMinerManager::Get().IncrementBlocksAccepted();

                    /* Notify Colin agent: block accepted */
                    if(context.hashGenesis != 0)
                    {
                        ColinMiningAgent::Get().on_block_submitted(
                            context.hashGenesis.SubString(8), pTritium->nChannel, true, "");
                    }
                }

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp());

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);

                return true;
            }

            /* Handle GET_HEIGHT - requires authentication */
            if(PACKET.HEADER == GET_HEIGHT)
            {
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "GET_HEIGHT rejected - authentication required");
                    StatelessPacket response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    response.LENGTH = 1;
                    respond(response);
                    return true;
                }

                /* Get current blockchain height */
                uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();

                debug::log(2, FUNCTION, "GET_HEIGHT request from ", GetAddress().ToStringIP(),
                           " sessionId=", context.nSessionId,
                           " - responding with height ", nCurrentHeight + 1);

                /* Create the response packet with height (next block to mine, 4-byte little-endian) */
                StatelessPacket response(BLOCK_HEIGHT);
                response.DATA = convert::uint2bytes(nCurrentHeight + 1);
                response.LENGTH = static_cast<uint32_t>(response.DATA.size());
                
                respond(response);

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp())
                                 .WithHeight(nCurrentHeight);

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);

                return true;
            }

            /* Handle GET_REWARD - requires authentication and channel */
            if(PACKET.HEADER == GET_REWARD)
            {
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "GET_REWARD rejected - authentication required");
                    StatelessPacket response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    response.LENGTH = 1;
                    respond(response);
                    return true;
                }

                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "GET_REWARD rejected - channel not set");
                    debug::error(FUNCTION, "  Required flow: Auth → MINER_SET_REWARD → SET_CHANNEL → GET_REWARD");
                    
                    /* Send error response */
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                debug::log(2, FUNCTION, "GET_REWARD request from ", GetAddress().ToStringIP(),
                           " channel=", context.nChannel, " sessionId=", context.nSessionId);

                /* Get the mining reward amount for the channel currently set */
                uint64_t nReward = TAO::Ledger::GetCoinbaseReward(
                    TAO::Ledger::ChainState::tStateBest.load(), 
                    context.nChannel, 
                    0);

                /* Check to make sure the reward is greater than zero */
                if(nReward == 0)
                {
                    debug::error(FUNCTION, "No coinbase reward available for this channel");
                    
                    /* Send error response */
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                debug::log(2, FUNCTION, "Sending Coinbase Reward of ", nReward);

                /* Create the response packet with reward (8-byte little-endian) */
                StatelessPacket response(BLOCK_REWARD);
                response.DATA = convert::uint2bytes64(nReward);
                response.LENGTH = static_cast<uint32_t>(response.DATA.size());
                
                respond(response);

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp());

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);

                return true;
            }

            /* Handle GET_ROUND - requires authentication */
            /* FULL HEIGHT PICTURE: Returns [Unified(4)][Prime(4)][Hash(4)][Stake(4)] = 16 bytes total */
            if(PACKET.HEADER == GET_ROUND)
            {
                // AUTOMATED RATE LIMIT CHECK
                if (!CheckRateLimit(GET_ROUND)) {
                    // Request rejected - violation already recorded
                    // Send OLD_ROUND response to indicate rate limited
                    debug::log(1, FUNCTION, "GET_ROUND rate limited for ", GetAddress().ToStringIP());
                    StatelessPacket response(StatelessOpcodes::OLD_ROUND);
                    response.LENGTH = 0;
                    respond(response);
                    return true;  // Handled (rejected)
                }
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::log(0, FUNCTION, "Unauthenticated miner attempted GET_ROUND from ",
                               GetAddress().ToStringIP());
                    std::vector<uint8_t> vFail(1, 0x00);
                    StatelessPacket response(StatelessOpcodes::OLD_ROUND);
                    response.DATA = vFail;
                    respond(response);
                    return true;
                }

                /* Check that miner has set a valid channel for stateless mining
                 * Note: Stateless mining only supports Prime (1) and Hash (2) channels.
                 * Stake mining (channel 0) uses a different mechanism and does NOT use GET_ROUND.
                 * Therefore, context.nChannel == 0 means "not set" or "invalid for stateless mining".
                 */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "GET_ROUND: Miner has not set channel (use SET_CHANNEL)");
                    debug::error(FUNCTION, "   Valid channels: 1=Prime, 2=Hash (Stake mining uses different mechanism)");
                    StatelessPacket response(StatelessOpcodes::OLD_ROUND);
                    response.LENGTH = 0;
                    respond(response);
                    return true;
                }

                /* ═════════════════════════════════════════════════════════════════════════ */
                /* FULL HEIGHT PICTURE: GET_ROUND/NEW_ROUND 16-byte response               */
                /* ═════════════════════════════════════════════════════════════════════════ */

                /* Get current blockchain state */
                TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();

                /* Validate blockchain is ready */
                if(stateBest.nHeight == 0)
                {
                    debug::error(FUNCTION, "GET_ROUND: Blockchain not initialized");
                    return true;  // Don't send response
                }

                /* Guard 1 — mirror StakeMinter:534 pattern.
                 * Snapshot hashBestChain at GET_ROUND time and compare against
                 * context.hashLastBlock (captured when BLOCK_DATA was last pushed).
                 * This catches same-height reorgs that fChannelHeightChanged misses.
                 * Skip the check if no template has been pushed yet (hashLastBlock == 0). */
                const uint1024_t hashCurrentBest = TAO::Ledger::ChainState::hashBestChain.load();
                const bool fHashChanged = (context.hashLastBlock != uint1024_t(0) &&
                                           context.hashLastBlock != hashCurrentBest);

                uint32_t nUnifiedHeight = stateBest.nHeight;

                /* Get all three channel heights for full height picture */
                TAO::Ledger::BlockState stateChannel = stateBest;
                uint32_t nPrimeHeight = 0;
                uint32_t nHashHeight  = 0;
                uint32_t nStakeHeight = 0;

                stateChannel = stateBest;
                if(TAO::Ledger::GetLastState(stateChannel, 1))
                    nPrimeHeight = stateChannel.nChannelHeight;
                else
                    debug::warning(FUNCTION, "GET_ROUND: Could not get Prime channel state");

                stateChannel = stateBest;
                if(TAO::Ledger::GetLastState(stateChannel, 2))
                    nHashHeight = stateChannel.nChannelHeight;
                else
                    debug::warning(FUNCTION, "GET_ROUND: Could not get Hash channel state");

                stateChannel = stateBest;
                if(TAO::Ledger::GetLastState(stateChannel, 0))
                    nStakeHeight = stateChannel.nChannelHeight;
                else
                    debug::warning(FUNCTION, "GET_ROUND: Could not get Stake channel state");

                /* Channel-specific height for staleness check (used in auto-send below).
                 * Note: channel 0 (Stake) is already rejected above via the nChannel == 0 guard.
                 * Explicit three-way assignment here for clarity and future-proofing. */
                uint32_t nChannelHeight = 0;
                if(context.nChannel == 1)
                    nChannelHeight = nPrimeHeight;
                else if(context.nChannel == 2)
                    nChannelHeight = nHashHeight;
                /* else channel 0 stake (unreachable) remains 0 */

                debug::log(3, FUNCTION, "GET_ROUND: unified=", nUnifiedHeight,
                           " prime=", nPrimeHeight, " hash=", nHashHeight, " stake=", nStakeHeight);

                /* ═════════════════════════════════════════════════════════════════════════ */
                /* BUILD 16-BYTE RESPONSE — Full Height Picture                             */
                /* ═════════════════════════════════════════════════════════════════════════ */

                /* Response format (16 bytes, big-endian):
                 *   [0-3]   uint32_t nUnifiedHeight  - Current blockchain height (all channels)
                 *   [4-7]   uint32_t nPrimeHeight    - Last confirmed Prime channel height
                 *   [8-11]  uint32_t nHashHeight     - Last confirmed Hash channel height
                 *   [12-15] uint32_t nStakeHeight    - Last confirmed Stake channel height
                 *
                 * Total: 16 bytes
                 *
                 * This matches the legacy miner.cpp handle_get_round_stateless() format and
                 * gives NexusMiner the Full Height Picture needed to populate HeightTracker
                 * (unified_height, prime_height, hash_height, stake_height) without relying
                 * solely on KeepAlive ACKs for channel-height data.
                 */
                std::vector<uint8_t> vData;
                vData.reserve(16);

                /* Unified height [0-3] big-endian */
                vData.push_back((nUnifiedHeight >> 24) & 0xFF);
                vData.push_back((nUnifiedHeight >> 16) & 0xFF);
                vData.push_back((nUnifiedHeight >>  8) & 0xFF);
                vData.push_back((nUnifiedHeight >>  0) & 0xFF);

                /* Prime height [4-7] big-endian */
                vData.push_back((nPrimeHeight >> 24) & 0xFF);
                vData.push_back((nPrimeHeight >> 16) & 0xFF);
                vData.push_back((nPrimeHeight >>  8) & 0xFF);
                vData.push_back((nPrimeHeight >>  0) & 0xFF);

                /* Hash height [8-11] big-endian */
                vData.push_back((nHashHeight >> 24) & 0xFF);
                vData.push_back((nHashHeight >> 16) & 0xFF);
                vData.push_back((nHashHeight >>  8) & 0xFF);
                vData.push_back((nHashHeight >>  0) & 0xFF);

                /* Stake height [12-15] big-endian */
                vData.push_back((nStakeHeight >> 24) & 0xFF);
                vData.push_back((nStakeHeight >> 16) & 0xFF);
                vData.push_back((nStakeHeight >>  8) & 0xFF);
                vData.push_back((nStakeHeight >>  0) & 0xFF);

                /* CRITICAL VALIDATION: Ensure exactly 16 bytes */
                if(vData.size() != 16)
                {
                    debug::error(FUNCTION, "GET_ROUND: Response size mismatch!");
                    debug::error(FUNCTION, "   Expected: 16 bytes");
                    debug::error(FUNCTION, "   Got: ", vData.size(), " bytes");
                    debug::error(FUNCTION, "   This is a CRITICAL BUG - miner will receive malformed packet");
                    return true;  // Don't send malformed response
                }

                debug::log(3, FUNCTION, "✓ Sending NEW_ROUND (16 bytes): Unified=", nUnifiedHeight,
                           " Prime=", nPrimeHeight, " Hash=", nHashHeight, " Stake=", nStakeHeight);

                /* Diagnostic logging */
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "📤 STATELESS PORT (9323): SENDING NEW_ROUND RESPONSE");
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "   Thread ID:      ", std::this_thread::get_id());
                debug::log(2, "   To:             ", GetAddress().ToStringIP());
                debug::log(2, "   Opcode:         NEW_ROUND (204/0xCC)");
                debug::log(2, "   Response Data (16 bytes, big-endian):");
                debug::log(2, "      Unified Height:  ", nUnifiedHeight);
                debug::log(2, "      Prime Height:    ", nPrimeHeight);
                debug::log(2, "      Hash Height:     ", nHashHeight);
                debug::log(2, "      Stake Height:    ", nStakeHeight);
                debug::log(2, "   Packet Size:    16 bytes");
                debug::log(2, "   ⚠️  NOTE:");
                debug::log(2, "      This is the legacy polling flow response.");
                debug::log(2, "      Preferred flow is push notifications:");
                debug::log(2, "         MINER_READY → ", (context.nChannel == 1 ? "PRIME" : "HASH"), "_BLOCK_AVAILABLE");
                debug::log(2, "════════════════════════════════════════════════════════════");

                /* Send response */
                StatelessPacket response(StatelessOpcodes::NEW_ROUND);
                response.DATA = vData;
                response.LENGTH = static_cast<uint32_t>(vData.size());
                respond(response);
                
                /* ═════════════════════════════════════════════════════════════════════════ */
                /* GET_ROUND COMPATIBILITY: AUTO-SEND TEMPLATE                              */
                /* ═════════════════════════════════════════════════════════════════════════ */
                
                /* DESIGN RATIONALE:
                 * Legacy miners using GET_ROUND polling expect to receive BLOCK_DATA automatically
                 * when the height changes, without needing to explicitly request GET_BLOCK.
                 * 
                 * This compatibility behavior:
                 * 1. Sends NEW_ROUND first (already done above)
                 * 2. Checks if miner's CHANNEL height has changed (not unified height)
                 * 3. If changed: Auto-send BLOCK_DATA (like GET_BLOCK does)
                 * 4. If same: Skip template send (miner already has current template)
                 * 
                 * IMPORTANT: We compare channel-specific heights, not unified heights.
                 * A Hash block advancing unified height should NOT trigger a Prime template refresh.
                 * Only when the miner's own channel advances do we need a new template.
                 *
                 * This maintains backward compatibility with legacy mining software that relies
                 * on GET_ROUND polling to automatically deliver templates.
                 */
                
                /* Template staleness check: Guard 1 (StakeMinter pattern).
                 * fChannelHeightChanged catches normal block advances.
                 * fHashChanged catches same-height reorgs (hash advances without integer change).
                 * Either condition means the miner's current template is stale. */
                bool fChannelHeightChanged = (context.nLastTemplateChannelHeight != nChannelHeight);
                bool fTemplateStale = fChannelHeightChanged || fHashChanged;
                
                debug::log(2, "");
                debug::log(2, "   🔍 GET_ROUND GUARD-1 CHECK (StakeMinter pattern):");
                debug::log(2, "      Miner's last template channel height:  ", context.nLastTemplateChannelHeight);
                debug::log(2, "      Current channel height:                ", nChannelHeight, " (channel ", context.nChannel, ")");
                debug::log(2, "      Channel height changed:                ", (fChannelHeightChanged ? "YES" : "NO"));
                debug::log(2, "      hashLastBlock:   ", context.hashLastBlock.SubString());
                debug::log(2, "      hashCurrentBest: ", hashCurrentBest.SubString());
                debug::log(2, "      Hash changed (reorg):                  ", (fHashChanged ? "YES" : "NO"));
                debug::log(2, "      Template stale:                        ", (fTemplateStale ? "YES" : "NO"));
                
                if(fTemplateStale)
                {
                    debug::log(2, "");
                    if(fHashChanged)
                        debug::log(2, "   ✅ REORG DETECTED (hashBestChain changed) - AUTO-SENDING TEMPLATE");
                    else
                        debug::log(2, "   ✅ CHANNEL HEIGHT CHANGED - AUTO-SENDING TEMPLATE");
                    debug::log(2, "      This maintains compatibility with legacy miners");
                    debug::log(2, "      that expect GET_ROUND to automatically deliver templates.");
                    debug::log(2, "");
                    debug::log(2, "   📤 Creating new block template...");
                    
                    /* Create a new block template (same logic as GET_BLOCK handler) */
                    TAO::Ledger::Block* pBlock = new_block();
                    
                    if(!pBlock)
                    {
                        debug::error(FUNCTION, "   ❌ GET_ROUND auto-send: new_block() returned nullptr");
                        debug::error(FUNCTION, "      Template will not be sent - miner must use GET_BLOCK");
                    }
                    else
                    {
                        try {
                            /* Serialize block template (216 bytes for Tritium) */
                            std::vector<uint8_t> vBlockData = pBlock->Serialize();
                            
                            if(vBlockData.empty())
                            {
                                debug::error(FUNCTION, "   ❌ GET_ROUND auto-send: Serialization returned empty");
                            }
                            else
                            {
                                /* Build 12-byte metadata prefix + 216-byte block = 228 bytes */
                                uint32_t nBitsVal = pBlock->nBits;
                                std::vector<uint8_t> vPayload;
                                vPayload.reserve(12 + vBlockData.size());
                                vPayload.push_back((nUnifiedHeight >> 24) & 0xFF);
                                vPayload.push_back((nUnifiedHeight >> 16) & 0xFF);
                                vPayload.push_back((nUnifiedHeight >>  8) & 0xFF);
                                vPayload.push_back((nUnifiedHeight      ) & 0xFF);
                                vPayload.push_back((nChannelHeight >> 24) & 0xFF);
                                vPayload.push_back((nChannelHeight >> 16) & 0xFF);
                                vPayload.push_back((nChannelHeight >>  8) & 0xFF);
                                vPayload.push_back((nChannelHeight      ) & 0xFF);
                                vPayload.push_back((nBitsVal       >> 24) & 0xFF);
                                vPayload.push_back((nBitsVal       >> 16) & 0xFF);
                                vPayload.push_back((nBitsVal       >>  8) & 0xFF);
                                vPayload.push_back((nBitsVal            ) & 0xFF);
                                vPayload.insert(vPayload.end(), vBlockData.begin(), vBlockData.end());

                                /* Send BLOCK_DATA packet */
                                StatelessPacket blockPacket(OpcodeUtility::Stateless::BLOCK_DATA);
                                blockPacket.DATA   = vPayload;
                                blockPacket.LENGTH = static_cast<uint32_t>(vPayload.size());
                                respond(blockPacket);

                                debug::log(2, "   ✅ BLOCK_DATA AUTO-SENT!");
                                debug::log(2, "      Template size:    ", vPayload.size(), " bytes");
                                debug::log(2, "      Block height:     ", pBlock->nHeight);
                                debug::log(2, "      Block channel:    ", pBlock->nChannel);
                                debug::log(2, "      Merkle root:      ", pBlock->hashMerkleRoot.SubString());
                                debug::log(2, "      [nUnifiedHeight=", nUnifiedHeight,
                                           " nChannelHeight=", nChannelHeight,
                                           " nBits=", nBitsVal, "]");

                                /* Update statistics */
                                StatelessMinerManager::Get().IncrementTemplatesServed();
                            }
                        }
                        catch(const std::exception& e) {
                            debug::error(FUNCTION, "   ❌ GET_ROUND auto-send exception: ", e.what());
                        }
                    }
                }
                else
                {
                    debug::log(2, "");
                    debug::log(2, "   ℹ️  TEMPLATE NOT STALE - NO TEMPLATE SENT");
                    debug::log(2, "      Miner should continue mining current template.");
                    debug::log(2, "      If miner needs new template, use GET_BLOCK explicitly.");
                }
                
                debug::log(2, "════════════════════════════════════════════════════════════");
                
                /* Update context timestamp, height, and staleness anchors.
                 * Only update channel height / hashLastBlock if a new template was sent
                 * (prevents duplicate sends on the next GET_ROUND poll). */
                if(fTemplateStale)
                {
                    /* Template was (or was attempted to be) sent — advance both anchors */
                    context = context.WithTimestamp(runtime::unifiedtimestamp())
                                     .WithHeight(nUnifiedHeight)
                                     .WithLastTemplateChannelHeight(nChannelHeight)
                                     .WithHashLastBlock(hashCurrentBest);
                }
                else
                {
                    /* Update only timestamp, keep existing staleness anchors */
                    context = context.WithTimestamp(runtime::unifiedtimestamp());
                }
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);

                return true;
            }

            /* Handle MINER_READY - Subscribe to push notifications */
            if(PACKET.HEADER == MINER_READY)
            {
                debug::log(2, "📥 === MINER_READY REQUEST ===");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Authenticated: ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(0, "   Channel: ", context.nChannel);
                
                /* Validate authentication */
                if (!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "MINER_READY: authentication required");
                    debug::log(2, "📥 === MINER_READY: REJECTED (AUTH) ===");
                    return false;
                }
                
                /* CRITICAL: Only Prime (1) and Hash (2) support stateless mining */
                if (context.nChannel != 1 && context.nChannel != 2)
                {
                    debug::error(FUNCTION, "MINER_READY: invalid channel ", context.nChannel);
                    debug::error(FUNCTION, "  Valid: 1 (Prime), 2 (Hash)");
                    debug::error(FUNCTION, "  Stake (0) uses Proof-of-Stake, not mined");
                    debug::log(2, "📥 === MINER_READY: REJECTED (INVALID CHANNEL) ===");
                    return false;
                }

                /* Attempt lane-switch recovery based on live node context */
                context.strAddress = GetAddress().ToStringIP();
                auto optExisting = SessionRecoveryManager::Get().RecoverSessionByIdentity(context);
                if(optExisting.has_value())
                {
                    if(optExisting->fRewardBound && optExisting->hashRewardAddress != 0)
                        context = context.WithRewardAddress(optExisting->hashRewardAddress);

                    if(optExisting->fEncryptionReady && !optExisting->vChaCha20Key.empty())
                        context = context.WithChaChaKey(optExisting->vChaCha20Key);

                    if(!optExisting->vDisposablePubKey.empty())
                    {
                        {
                            std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                            if(optExisting->nSessionId != 0)
                                mapSessionKeys[optExisting->nSessionId] = optExisting->vDisposablePubKey;
                        }
                        /* Fix 2: Propagate recovered key into context so SaveSession re-persists it */
                        context = context.WithDisposableKey(optExisting->vDisposablePubKey,
                                                            optExisting->hashDisposableKeyID);
                    }
                    else
                    {
                        /* Fix 2: Fallback — try RestoreDisposableKey independently in case the
                         * recovery container lost the key. */
                        std::vector<uint8_t> vFallbackPubKey;
                        uint256_t hashFallbackKeyID;
                        if(context.hashKeyID != 0 &&
                           SessionRecoveryManager::Get().RestoreDisposableKey(
                               context.hashKeyID, vFallbackPubKey, hashFallbackKeyID) &&
                           !vFallbackPubKey.empty())
                        {
                            {
                                std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                                if(optExisting->nSessionId != 0)
                                    mapSessionKeys[optExisting->nSessionId] = vFallbackPubKey;
                            }
                            context = context.WithDisposableKey(vFallbackPubKey, hashFallbackKeyID);
                            debug::log(0, FUNCTION, "  Fallback: restored disposable Falcon key from dedicated key store");
                        }
                        else
                        {
                            debug::error(FUNCTION, "  ERROR: disposable Falcon key lost after recovery — block submissions will fail");
                            debug::error(FUNCTION, "  keyID=", FullHexOrUnset(context.hashKeyID));
                            debug::error(FUNCTION, "  Miner must re-authenticate to restore block submission capability");
                        }
                    }

                    debug::log(0, FUNCTION, "Session recovered from lane switch");
                    debug::log(0, FUNCTION, "  session state source: SessionRecoveryManager::RecoverSessionByIdentity");
                    debug::log(0, FUNCTION, "  recovered falcon key id: ", FullHexOrUnset(optExisting->hashKeyID));
                    debug::log(0, FUNCTION, "  recovered session genesis: ", FullHexOrUnset(optExisting->hashGenesis));
                    debug::log(0, FUNCTION, "  recovered reward hash: ", FullHexOrUnset(optExisting->hashRewardAddress));
                    debug::log(0, FUNCTION, "  recovered ChaCha20 key hash: ", FullHexOrUnset(optExisting->hashChaCha20Key));
                    debug::log(0, FUNCTION, "  recovered disposable Falcon key present: ", YesNo(!context.vDisposablePubKey.empty()));
                }
                
                /* Subscribe to notifications */
                context = context.WithSubscription(context.nChannel);
                
                /* Ensure encryption is properly set up before mining starts
                 * CRITICAL: ChaCha20 key should have been derived during authentication.
                 * If not present, derive it now from genesis hash. */
                if(context.hashGenesis != 0 && !context.fEncryptionReady)
                {
                    /* Derive ChaCha20 encryption key from genesis hash */
                    context = context.WithChaChaKey(LLC::MiningSessionKeys::DeriveChaCha20Key(context.hashGenesis));
                    
                    debug::log(0, FUNCTION, "✓ Derived ChaCha20 key on MINER_READY (8-bit)");
                    debug::log(0, "   Session genesis used for KDF: ", context.GenesisHex());
                    debug::log(0, "   Derived key fingerprint: ", KeyFingerprint(context.vChaChaKey));
                    debug::log(0, "   Encryption ready: YES");
                }
                
                debug::log(0, FUNCTION, "✓ Miner subscribed to ", 
                          context.strChannelName, " notifications");
                debug::log(0, "   Encryption ready: ", (context.fEncryptionReady ? "YES" : "NO"));
                debug::log(0, "   ChaCha key size: ", context.vChaChaKey.size(), " bytes");
                
                /* Send immediate notification with current state.
                 * Force-bypass the push throttle — miner explicitly re-subscribed and needs
                 * fresh work immediately regardless of when the previous push was sent.
                 * Also reset the AutoCoolDown so the recovery GET_BLOCK is served immediately. */
                {
                    LOCK(MUTEX);
                    // Reset violation state on clean re-subscription — same invariant as
                    // STATELESS_MINER_READY: fThrottleMode MUST NOT persist across reconnects.
                    m_rateLimit.ResetViolationState();
                    m_force_next_push = true;
                    // Reassign (not Reset()) — we want Ready() to return true immediately
                    // so the recovery GET_BLOCK is served without waiting 2 s.
                    // Reset() would START a new 2-second cooldown; reassignment
                    // restores the "never triggered" state where Ready() returns true.
                    m_get_block_cooldown = AutoCoolDown(std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS));
                }
                SendChannelNotification();

                /* Re-arm the bypass flag: SendChannelNotification() consumed m_force_next_push
                 * and updated m_last_template_push_time.  Without re-arming, SendStatelessTemplate()
                 * would see elapsed ~0 ms and be throttled, leaving the miner without a full
                 * template and keeping its recovery epoch alive. */
                {
                    LOCK(MUTEX);
                    m_force_next_push = true;
                }

                /* Update last template channel height and persist session BEFORE sending
                 * the template (write-ahead pattern): even if the connection drops after
                 * SaveSession but before the template arrives, the recovered session already
                 * carries the correct channel height, preventing stale-height throttling. */
                {
                    TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
                    TAO::Ledger::BlockState stateChannel = stateBest;
                    uint32_t nChannelHeight = 0;
                    if(TAO::Ledger::GetLastState(stateChannel, context.nChannel))
                        nChannelHeight = stateChannel.nChannelHeight;
                    context = context.WithLastTemplateChannelHeight(nChannelHeight);
                }

                /* Persist session and lane state for cross-lane recovery */
                if(context.fAuthenticated && context.hashKeyID != 0)
                {
                    SessionRecoveryManager::Get().SaveSession(context);
                    SessionRecoveryManager::Get().UpdateLane(context.hashKeyID, 1);
                    if(context.fEncryptionReady && !context.vChaChaKey.empty())
                    {
                        uint256_t hashKey(context.vChaChaKey);
                        SessionRecoveryManager::Get().SaveChaCha20State(context.hashKeyID, hashKey, 0);
                    }
                }

                /* Auto-send template so miner can resume mining immediately.
                 * This is critical for MINER_READY recovery from degraded mode -
                 * miners need both the push notification AND the actual template. */
                SendStatelessTemplate();
                debug::log(0, FUNCTION, "✓ Recovery template delivered via MINER_READY push — miner should resume mining");

                /* Update manager with COMPLETE context including encryption state */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                
                debug::log(2, "📥 === MINER_READY: SUCCESS ===");
                return true;
            }

            /* SESSION_STATUS (0xD0DB) — miner queries session/lane health on stateless port */
            if(PACKET.HEADER == OpcodeUtility::Stateless::SESSION_STATUS)
            {
                debug::log(2, FUNCTION, "SESSION_STATUS received from ", GetAddress().ToStringIP());

                SessionStatus::SessionStatusRequest req;
                if(!req.Parse(PACKET.DATA))
                {
                    debug::error(FUNCTION, "SESSION_STATUS: malformed payload (size=", PACKET.DATA.size(), ")");
                    StatelessPacket errResponse(OpcodeUtility::Stateless::SESSION_STATUS_ACK);
                    auto vAck = SessionStatus::BuildAckPayload(0u, 0u, 0u, 0u);
                    errResponse.DATA = vAck;
                    errResponse.LENGTH = static_cast<uint32_t>(vAck.size());
                    respond(errResponse);
                    return true;
                }

                /* Validate session ID matches this connection's context */
                if(req.session_id != context.nSessionId)
                {
                    debug::error(FUNCTION, "SESSION_STATUS: session_id mismatch (got=0x", std::hex,
                                 req.session_id, " expected=0x", context.nSessionId, std::dec, ")");
                    auto vAck = SessionStatus::BuildAckPayload(context.nSessionId, 0u, 0u, req.status_flags);
                    StatelessPacket mismatchResponse(OpcodeUtility::Stateless::SESSION_STATUS_ACK);
                    mismatchResponse.DATA = vAck;
                    mismatchResponse.LENGTH = static_cast<uint32_t>(vAck.size());
                    respond(mismatchResponse);
                    return true;
                }

                /* Build lane health flags — stateless lane: we are ON it + authenticated */
                uint32_t nLaneHealth = 0;
                nLaneHealth |= SessionStatus::LANE_PRIMARY_ALIVE;  // stateless lane alive
                nLaneHealth |= SessionStatus::LANE_AUTHENTICATED;  // session verified

                const uint32_t nUptime = static_cast<uint32_t>(context.GetSessionDuration(runtime::unifiedtimestamp()));
                auto vAck = SessionStatus::BuildAckPayload(req.session_id, nLaneHealth, nUptime, req.status_flags);
                StatelessPacket ackResponse(OpcodeUtility::Stateless::SESSION_STATUS_ACK);
                ackResponse.DATA = vAck;
                ackResponse.LENGTH = static_cast<uint32_t>(vAck.size());
                respond(ackResponse);

                debug::log(2, FUNCTION, "SESSION_STATUS_ACK sent: lane_health=0x", std::hex, nLaneHealth, std::dec);

                /* TWO-STEP RE-ARM INVARIANT (PR #375):
                 * SendChannelNotification() consumes m_force_next_push (sets it false)
                 * AND resets m_last_template_push_time to "now". Without re-arming
                 * m_force_next_push before SendStatelessTemplate(), elapsed = ~0 ms
                 * and the template push is throttled (TEMPLATE_PUSH_MIN_INTERVAL_MS).
                 * Always: set → SendChannelNotification → re-set → SendStatelessTemplate.
                 * See: MINER_READY handler for the canonical pattern. */
                if((req.status_flags & SessionStatus::MINER_DEGRADED) &&
                    context.fAuthenticated && (context.nChannel == 1 || context.nChannel == 2))
                {
                    debug::log(0, FUNCTION, "⚠️ Miner reports DEGRADED — forcing recovery template push via SESSION_STATUS");
                    {
                        LOCK(MUTEX);
                        m_force_next_push = true;
                        /* Reset cooldown so any pending GET_BLOCK from the miner is served
                         * immediately rather than being held for GET_BLOCK_COOLDOWN_SECONDS.
                         * Uses reassignment (not Reset()) to restore "never triggered" state
                         * where Ready() returns true immediately. */
                        m_get_block_cooldown = AutoCoolDown(std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS));
                    }
                    SendChannelNotification();
                    {
                        LOCK(MUTEX);
                        m_force_next_push = true;  // re-arm: SendChannelNotification() consumed the flag
                    }
                    SendStatelessTemplate();
                    debug::log(0, FUNCTION, "✓ Degraded-recovery template pushed — miner should exit DEGRADED");
                }

                return true;
            }

            /* PONG_DIAG — 64-byte diagnostic pong from miner */
            if(PACKET.HEADER == ColinDiagOpcodes::PONG_DIAG_STATELESS)
            {
                if(PACKET.DATA.size() >= 64)
                {
                    /* Forward pong payload to Colin Agent for RTT computation and reporting */
                    ColinMiningAgent::Get().on_pong_received(
                        context.hashGenesis != 0 ? context.hashGenesis.SubString(8) : "",
                        PACKET.DATA);
                }
                return true;
            }

            /* For all other packets (including KEEPALIVE_V2), route through StatelessMiner processor */
            ProcessResult result = StatelessMiner::ProcessPacket(context, PACKET);

            /* Update context if successful */
            if(result.fSuccess)
            {
                /* CRITICAL: Extract miner's Falcon pubkey BEFORE updating context
                 * The pubkey is needed for signature verification but will be cleared
                 * from the new context for security reasons (single-use, preserved in keyID).
                 * We must store it in mapSessionKeys while still available. */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && result.context.fAuthenticated)
                {
                    /* Extract pubkey from result context BEFORE it's cleared */
                    if(!result.context.vMinerPubKey.empty())
                    {
                        std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                        
                        /* Check for session ID collision (should be extremely rare) */
                        auto it = mapSessionKeys.find(result.context.nSessionId);
                        if(it != mapSessionKeys.end())
                        {
                            debug::warning(FUNCTION, "⚠ Session ID collision detected: 0x",
                                          std::hex, result.context.nSessionId, std::dec);
                            debug::warning(FUNCTION, "   Overwriting existing session key");
                            debug::warning(FUNCTION, "   This indicates either a key collision or logic error");
                        }
                        
                        mapSessionKeys[result.context.nSessionId] = result.context.vMinerPubKey;

                        debug::log(1, FUNCTION, "✓ Extracted and stored miner's Falcon pubkey for session 0x",
                                  std::hex, result.context.nSessionId, std::dec,
                                  " (", result.context.vMinerPubKey.size(), " bytes)");
                    }
                    else
                    {
                        debug::error(FUNCTION, "⚠ LOGIC ERROR: MINER_AUTH_RESPONSE succeeded but vMinerPubKey is empty");
                        debug::error(FUNCTION, "   Authentication should guarantee pubkey is present");
                        debug::error(FUNCTION, "   This indicates a bug in ProcessFalconResponse");
                        debug::error(FUNCTION, "   Signature verification will FAIL for this session (no pubkey stored)");
                    }
                }
                
                context = result.context;

                /* Derive ChaCha20 key from genesis using unified helper (same as legacy miner) */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && context.fAuthenticated)
                {
                    if(context.hashGenesis != 0 && !context.fEncryptionReady)
                    {
                        /* Derive ChaCha20 encryption key from genesis hash */
                        context = context.WithChaChaKey(LLC::MiningSessionKeys::DeriveChaCha20Key(context.hashGenesis));
                        
                        debug::log(0, FUNCTION, "✓ Derived ChaCha20 key from genesis for session 0x",
                                  std::hex, context.nSessionId, std::dec);
                        debug::log(0, "   Session genesis used for KDF: ", context.GenesisHex());
                        debug::log(0, "   Derived key fingerprint: ", KeyFingerprint(context.vChaChaKey));
                        debug::log(0, "   Encryption ready: YES");
                    }
                    else if(context.hashGenesis == 0)
                    {
                        debug::warning(FUNCTION, "⚠ Authentication succeeded but genesis hash is 0");
                        debug::warning(FUNCTION, "   ChaCha20 encryption will NOT be available");
                        debug::warning(FUNCTION, "   This may indicate incomplete Falcon authentication");
                    }
                }

                /* Register session in NodeSessionRegistry for cross-port session identity.
                 * This is the node-side outer wrapper that mirrors NodeSession on the miner side.
                 * One Falcon identity (hashKeyID) maps to one canonical nSessionId across both ports. */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && context.fAuthenticated && context.hashKeyID != 0)
                {
                    auto [canonicalSessionId, isNew] = NodeSessionRegistry::Get().RegisterOrRefresh(
                        context.hashKeyID,
                        context.hashGenesis,
                        context,
                        ProtocolLane::STATELESS
                    );

                    /* CRITICAL: If the registry returned a different nSessionId than what we derived,
                     * it means this miner has already authenticated on the other port.
                     * We MUST use the canonical ID to prevent session_id=0 bugs on reconnection. */
                    if(canonicalSessionId != context.nSessionId)
                    {
                        debug::log(0, FUNCTION, "⚠ Cross-port session recovery: Overriding derived sessionId ",
                                   context.nSessionId, " → ", canonicalSessionId,
                                   " (already registered on other port)");
                        context = context.WithSession(canonicalSessionId);
                    }

                    if(isNew)
                    {
                        debug::log(0, FUNCTION, "✓ Registered NEW session in NodeSessionRegistry: sessionId=",
                                   canonicalSessionId, " falcon_key_id=", FullHexOrUnset(context.hashKeyID));
                    }
                    else
                    {
                        debug::log(0, FUNCTION, "✓ Refreshed EXISTING session in NodeSessionRegistry: sessionId=",
                                   canonicalSessionId, " falcon_key_id=", FullHexOrUnset(context.hashKeyID));
                    }
                }

                /* Persist session and lane state for cross-lane recovery */
                if(context.fAuthenticated && context.hashKeyID != 0)
                {
                    /* CRITICAL (Fix 3): Ensure vDisposablePubKey is included in context before
                     * SaveSession so the recovery cache is created with the Falcon pubkey.
                     * SaveDisposableKey cannot be called before SaveSession (no session exists
                     * yet at that point), so we embed the key in context here instead. */
                    if(PACKET.HEADER == MINER_AUTH_RESPONSE &&
                       !context.vMinerPubKey.empty() &&
                       context.vDisposablePubKey.empty())
                    {
                        context = context.WithDisposableKey(context.vMinerPubKey,
                                                            LLC::SK256(context.vMinerPubKey));
                        debug::log(1, FUNCTION, "✓ Embedded disposable Falcon key in context for session recovery persistence");
                    }

                    SessionRecoveryManager::Get().SaveSession(context);
                    SessionRecoveryManager::Get().UpdateLane(context.hashKeyID, 1);
                    if(context.fEncryptionReady && !context.vChaChaKey.empty())
                    {
                        uint256_t hashKey(context.vChaChaKey);
                        SessionRecoveryManager::Get().SaveChaCha20State(context.hashKeyID, hashKey, 0);
                    }
                }

                /* Update manager with new context after successful packet processing */
                /* CRITICAL: This must be done AFTER ChaCha20 key derivation to ensure
                 * StatelessMinerManager has the complete encryption state */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);

                /* Log session registration for auth packets */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && context.fAuthenticated)
                {
                    debug::log(0, FUNCTION, "Session registered: address=", context.strAddress,
                               " sessionId=", context.nSessionId, " falcon_key_id=", FullHexOrUnset(context.hashKeyID));

                    /* Notify ChannelStateManager if this was a failover authentication.
                     * FailoverConnectionTracker::IsFailover() returns true when the CONNECT
                     * event found no recoverable session for this IP, indicating the miner
                     * has failed over to this node and performed a full fresh handshake. */
                    if(FailoverConnectionTracker::Get().IsFailover(context.strAddress))
                    {
                        ChannelStateManager::NotifyFailoverConnection(context.hashKeyID, context.strAddress);
                        /* Mark session as fresh auth so Colin report can show fresh_auth=true */
                        SessionRecoveryManager::Get().MarkFreshAuth(context.hashKeyID);
                        /* Clear the pending failover flag now that auth has completed */
                        FailoverConnectionTracker::Get().ClearConnection(context.strAddress);
                    }
                }
                
                /* Send response if present */
                if(!result.response.IsNull())
                {
                    /* StatelessMiner now builds responses with 16-bit opcodes directly (0xD0D0, etc.).
                     * Only legacy response opcodes (200-206) from other parts of the codebase
                     * need mirroring to 16-bit format for the stateless lane.
                     * 
                     * Valid ranges:
                     * - 200-206: Legacy responses (BLOCK_ACCEPTED, etc.) → SHOULD mirror to 16-bit
                     * - 0xD000+: Already 16-bit opcodes from StatelessMiner → Send as-is
                     * 
                     * NOTE: result.response is const, so we must create a copy before modifying.
                     */
                    if(result.response.HEADER < 256 && 
                       result.response.HEADER >= MiningConstants::MINING_OPCODE_MIN &&
                       result.response.HEADER < MiningConstants::STATELESS_MINING_OPCODE_MIN)
                    {
                        /* This is a legacy response opcode (200-206) that needs mirroring */
                        uint8_t nLegacyOpcode = static_cast<uint8_t>(result.response.HEADER);
                        StatelessPacket mirroredResponse = result.response;  // Create a copy
                        mirroredResponse.HEADER = StatelessOpcodes::Mirror(nLegacyOpcode);
                        debug::log(3, FUNCTION, "Mirrored legacy response opcode ", 
                                  uint32_t(nLegacyOpcode), " → 16-bit 0x", std::hex, mirroredResponse.HEADER, std::dec);
                        
                        respond(mirroredResponse);  // Send the modified copy
                    }
                    else
                    {
                        /* Send as-is - already 16-bit opcodes from StatelessMiner */
                        debug::log(3, FUNCTION, "Sending response with opcode 0x",
                                  std::hex, result.response.HEADER, std::dec);
                        respond(result.response);
                    }

                    /* After successful authentication, automatically send SESSION_START
                     * to advertise the node's session timeout configuration to the miner.
                     * This allows modern miners to auto-calculate keepalive intervals. */
                    if(PACKET.HEADER == MINER_AUTH_RESPONSE && result.fSuccess && context.fAuthenticated)
                    {
                        debug::log(0, FUNCTION, "Sending SESSION_START after successful authentication");

                        /* Build SESSION_START packet with session parameters */
                        StatelessPacket sessionStart(StatelessOpcodes::SESSION_START);
                        sessionStart.DATA.push_back(0x01); // Success

                        /* Add session ID (4 bytes, little-endian) */
                        uint32_t nSessionId = context.nSessionId;
                        sessionStart.DATA.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));

                        /* Add timeout (4 bytes, little-endian)
                         * Cast to uint32_t to avoid silent truncation if nSessionTimeout > 0xFFFFFFFF.
                         * NexusMiner parser expects 4 bytes LE. Session timeouts realistically fit in 32 bits
                         * (max ~136 years if in seconds, or ~49 days if in milliseconds). */
                        uint32_t nTimeout = static_cast<uint32_t>(context.nSessionTimeout);
                        sessionStart.DATA.push_back(static_cast<uint8_t>(nTimeout & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nTimeout >> 8) & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nTimeout >> 16) & 0xFF));
                        sessionStart.DATA.push_back(static_cast<uint8_t>((nTimeout >> 24) & 0xFF));

                        /* Add genesis hash if bound (32 bytes) */
                        if(context.hashGenesis != 0)
                        {
                            std::vector<uint8_t> vGenesis = context.hashGenesis.GetBytes();
                            sessionStart.DATA.insert(sessionStart.DATA.end(), vGenesis.begin(), vGenesis.end());
                        }

                        sessionStart.LENGTH = static_cast<uint32_t>(sessionStart.DATA.size());

                        debug::log(0, FUNCTION, "SESSION_START: sessionId=", nSessionId,
                                  " timeout=", nTimeout, "s session_genesis=", context.GenesisHex());

                        respond(sessionStart);
                    }
                }
            }
            else
            {
                /* Log error and send failure response to miner for graceful handling */
                debug::log(0, FUNCTION, "MinerLLP: Processing error: ", result.strError);
                
                /* Send generic error response based on packet type */
                /* This allows miner to handle errors gracefully instead of timing out */
                
                StatelessPacket errorResponse;
                
                /* Send appropriate error response based on what was requested */
                if(PACKET.HEADER == MINER_AUTH_INIT || PACKET.HEADER == MINER_AUTH_RESPONSE)
                {
                    /* Note: MINER_AUTH_RESULT is a local const initialized from STATELESS_AUTH_RESULT (0xD0D2)
                     * at line 609, so it's already the correct 16-bit opcode for the stateless lane.
                     * Unlike responses from StatelessMiner (which use 8-bit enum values and need mirroring),
                     * error responses built here use the local 16-bit consts directly. */
                    errorResponse.HEADER = MINER_AUTH_RESULT;  // Already 16-bit (0xD0D2)
                    errorResponse.DATA.push_back(0x00);  /* Failure status */
                    errorResponse.LENGTH = 1;
                    respond(errorResponse);
                    debug::log(2, FUNCTION, "Sent MINER_AUTH_RESULT error response");
                }
                else if(PACKET.HEADER == KEEPALIVE_V2)
                {
                    if(context.fAuthenticated && context.nSessionId != 0)
                    {
                        /* Use an all-zero fallback frame because the error path only needs
                         * a syntactically valid KEEPALIVE_V2 packet to trigger the normal
                         * ACK-building path; session identity and liveness come from context. */
                        const StatelessPacket fallbackKeepalive = BuildFallbackKeepaliveV2Packet();

                        const ProcessResult fallbackResult =
                            StatelessMiner::ProcessKeepaliveV2(context, fallbackKeepalive);

                        if(fallbackResult.fSuccess && fallbackResult.response.LENGTH > 0)
                        {
                            respond(fallbackResult.response);
                            debug::log(2, FUNCTION,
                                       "Sent KEEPALIVE_V2 fallback response after processing error");
                        }
                    }
                    else
                    {
                        errorResponse = BuildSessionExpiredResponse(context.nSessionId);
                        respond(errorResponse);
                        debug::log(2, FUNCTION,
                                   "Sent SESSION_EXPIRED fallback response after KEEPALIVE_V2 error");
                    }
                }
                
                /* For other packet types, connection will be closed gracefully */
                /* Don't force disconnect immediately - let miner handle the error */
                return true;  /* Return true to avoid force disconnect */
            }

            return true;
        }
        catch(const std::exception& e)
        {
            debug::log(0, FUNCTION, "MinerLLP: EXCEPTION in ProcessPacket from ",
                       GetAddress().ToStringIP(), " - what(): ", e.what());
            
            /* Send generic error response before disconnecting */
            /* This prevents node from locking up on unexpected errors */
            try
            {
                StatelessPacket errorResponse;
                errorResponse.HEADER = StatelessOpcodes::STATELESS_AUTH_RESULT;
                errorResponse.DATA.push_back(0x00);  /* Failure status */
                errorResponse.LENGTH = 1;
                respond(errorResponse);
            }
            catch(...)
            {
                /* Ignore errors when sending error response */
            }
            
            /* Return true to allow graceful disconnect instead of force disconnect */
            return true;
        }
    }


    /** Send a stateless packet response */
    void StatelessMinerConnection::respond(const StatelessPacket& packet)
    {
        /* Serialize and write the packet */
        WritePacket(packet);
    }


    /** Check if block exists in map */
    bool StatelessMinerConnection::find_block(const uint512_t& hashMerkleRoot)
    {
        /* Check that the block exists. */
        if(!mapBlocks.count(hashMerkleRoot))
        {
            debug::log(2, FUNCTION, "Block Not Found ", hashMerkleRoot.SubString());
            return false;
        }

        return true;
    }


    /** Create a new block */
    TAO::Ledger::Block* StatelessMinerConnection::new_block()
    {
        /* Snapshot context fields under MUTEX so that new_block() never races
         * with ProcessPacket() writers when called from the notification thread. */
        uint32_t nChannel_snap;
        uint32_t nSessionId_snap;
        bool     fAuthenticated_snap;
        bool     fRewardBound_snap;
        uint256_t hashRewardAddress_snap;
        uint256_t hashGenesis_snap;
        {
            LOCK(MUTEX);
            nChannel_snap         = context.nChannel;
            nSessionId_snap       = context.nSessionId;
            fAuthenticated_snap   = context.fAuthenticated;
            fRewardBound_snap     = context.fRewardBound;
            hashRewardAddress_snap = context.hashRewardAddress;
            hashGenesis_snap      = context.hashGenesis;
        }

        {
            std::unique_lock<std::mutex> lock(TEMPLATE_CREATE_MUTEX);
            if(m_template_create_in_flight)
            {
                debug::log(2, FUNCTION, "Template creation already in-flight for session ", nSessionId_snap,
                           "; waiting for existing result");
                while(m_template_create_in_flight && !config::fShutdown.load())
                    TEMPLATE_CREATE_CV.wait_for(lock, std::chrono::milliseconds(500));

                if(config::fShutdown.load())
                {
                    debug::log(1, FUNCTION, "Shutdown detected while waiting for template creation");
                    return nullptr;
                }

                /* If the waited-for template returned nullptr, retry once instead of propagating the failure.
                 * This prevents miners from receiving empty BLOCK_DATA when template creation fails transiently
                 * (e.g., during chain reorganization). */
                if(m_last_created_template == nullptr)
                {
                    debug::log(1, FUNCTION, "Waited template creation returned nullptr — retrying once");
                    /* Fall through to create our own template below */
                }
                else
                {
                    /* FIX #332: Validate cached template is not stale before returning it.
                     *
                     * PROBLEM: If the chain advanced while we waited for template creation,
                     * m_last_created_template may point to a template that's 4+ blocks old.
                     * This caused the "channel_height=2333128, channel_target=2333132 gap" bug
                     * where nodes served stale templates that were immediately rejected.
                     *
                     * ROOT CAUSE: After the GET_BLOCK deadlock fix (PR #331), if new_block()
                     * was called while holding MUTEX and hung, the node cached a stale template.
                     * Concurrent callers then received this stale template without staleness
                     * revalidation.
                     *
                     * SOLUTION: Look up the template metadata in mapBlocks and call IsStale()
                     * before returning the cached pointer. If stale, fall through to create
                     * a fresh template.
                     */
                    const uint512_t hashMerkleKey = m_last_created_template->hashMerkleRoot;
                    if(mapBlocks.count(hashMerkleKey))
                    {
                        const TemplateMetadata& meta = mapBlocks[hashMerkleKey];
                        if(meta.IsStale())
                        {
                            debug::log(1, FUNCTION, "Cached template is STALE — creating fresh template");
                            debug::log(1, FUNCTION, "   Cached merkle: ", hashMerkleKey.SubString());
                            debug::log(1, FUNCTION, "   Cached channel height: ", meta.nChannelHeight);
                            debug::log(1, FUNCTION, "   Cached age: ",
                                      (runtime::unifiedtimestamp() - meta.nCreationTime), "s");
                            /* Fall through to create a fresh template below */
                        }
                        else
                        {
                            debug::log(2, FUNCTION, "Cached template is still fresh — returning it");
                            return m_last_created_template;
                        }
                    }
                    else
                    {
                        /* Template was removed from mapBlocks (e.g., cleanup) — create fresh */
                        debug::log(1, FUNCTION, "Cached template no longer in mapBlocks — creating fresh");
                        /* Fall through to create a fresh template below */
                    }
                }
            }

            m_template_create_in_flight = true;
            m_last_created_template = nullptr;
        }

        auto finalize_template_creation = [this](TAO::Ledger::Block* pResult)
        {
            std::lock_guard<std::mutex> lock(TEMPLATE_CREATE_MUTEX);
            m_last_created_template = pResult;
            m_template_create_in_flight = false;
            TEMPLATE_CREATE_CV.notify_all();
        };

        struct TemplateCreationScope
        {
            decltype(finalize_template_creation)& finalize;
            TAO::Ledger::Block* result{nullptr};
            ~TemplateCreationScope()
            {
                finalize(result);
            }
        } creationScope{finalize_template_creation};

        /* Early exit if shutdown is in progress */
        if (config::fShutdown.load())
        {
            debug::log(1, FUNCTION, "Shutdown in progress; skipping template creation");
            return nullptr;
        }
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Request from ", GetAddress().ToStringIP(), " ===", ANSI_COLOR_RESET);
        
        /* Validate channel is set BEFORE creating template */
        if(nChannel_snap == 0)
        {
            debug::error(FUNCTION, "❌ Cannot create template: nChannel not set");
            debug::error(FUNCTION, "   Required: Miner must send SET_CHANNEL before GET_BLOCK");
            debug::error(FUNCTION, "   Current context.nChannel: ", nChannel_snap);
            return nullptr;
        }
        
        /* Validate channel value is valid (1=Prime, 2=Hash) */
        if(nChannel_snap != 1 && nChannel_snap != 2)
        {
            debug::error(FUNCTION, "❌ Invalid channel: ", nChannel_snap);
            debug::error(FUNCTION, "   Valid channels: 1 (Prime), 2 (Hash)");
            return nullptr;
        }
        
        debug::log(0, "   Channel validated: ", nChannel_snap, 
                   " (", MiningContext::ChannelName(nChannel_snap), ")");
        
        /* Get CURRENT blockchain height FIRST */
        uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Creating template ===", ANSI_COLOR_RESET);
        debug::log(0, "   Current blockchain height: ", nCurrentHeight);
        debug::log(0, "   Template will target height: ", nCurrentHeight + 1);
        
        /* Cleanup old templates using the new dedicated method.
         * Lock ordering: MUTEX protects all mapBlocks reads/writes;
         * TEMPLATE_CREATE_MUTEX (inner) is never held when MUTEX is acquired here. */
        {
            std::lock_guard<std::mutex> map_lk(MUTEX);
            CleanupStaleTemplates(nCurrentHeight);
        }
        
        /* Determine reward - same priority as miner.cpp */
        debug::log(0, "   Determining reward identity for block rewards...");
        uint256_t hashReward = 0;
        std::string strRewardSource = "not configured";
        const auto optRecovery = SessionRecoveryManager::Get().RecoverSessionByIdentity(
            context.hashKeyID,
            context.strAddress
        );
        const bool fRecoveryGenesisMatches = optRecovery.has_value() &&
            optRecovery->hashGenesis == hashGenesis_snap;
        
        if(fRewardBound_snap && hashRewardAddress_snap != 0) {
            hashReward = hashRewardAddress_snap;
            strRewardSource = "current connection context bound reward hash";
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "      Using bound reward hash: ", hashReward.GetHex(), ANSI_COLOR_RESET);
        }
        else if(hashGenesis_snap != 0) {
            hashReward = hashGenesis_snap;
            strRewardSource = "current connection context session genesis fallback";
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "      Using session genesis fallback hash: ", hashReward.GetHex(), ANSI_COLOR_RESET);
        }
        else {
            debug::error(FUNCTION, "No reward address available");
            debug::log(0, ANSI_COLOR_BRIGHT_RED, "   FAILED: No reward address", ANSI_COLOR_RESET);
            return nullptr;
        }

        debug::log(0, FUNCTION, "REWARD BINDING DIAGNOSTIC");
        debug::log(0, FUNCTION, "- miner reward string: NOT AVAILABLE (node does not receive miner reward string during template creation)");
        debug::log(0, FUNCTION, "- decoded reward register/account hash: ", fRewardBound_snap ? hashRewardAddress_snap.GetHex() : "NOT AVAILABLE");
        debug::log(0, FUNCTION, "- bound reward hash from cache/session: ", FullHexOrUnset(hashRewardAddress_snap));
        debug::log(0, FUNCTION, "- bound reward source: ", strRewardSource);
        debug::log(0, FUNCTION, "- session genesis used for ChaCha20 KDF: ", FullHexOrUnset(hashGenesis_snap));
        debug::log(0, FUNCTION, "- session recovery state available: ", YesNo(optRecovery.has_value()));
        debug::log(0, FUNCTION, "- recovered session genesis: ",
                   optRecovery.has_value() ? FullHexOrUnset(optRecovery->hashGenesis) : "NOT AVAILABLE");
        debug::log(0, FUNCTION, "- recovered session genesis matches current context: ", YesNo(fRecoveryGenesisMatches));
        debug::log(0, FUNCTION, "- reward hash == bound reward hash: ",
                   YesNo(!fRewardBound_snap || hashReward == hashRewardAddress_snap));
        debug::log(0, FUNCTION, "- consistency result: ",
                   PassFail((!optRecovery.has_value() || fRecoveryGenesisMatches) &&
                            (!fRewardBound_snap || hashReward == hashRewardAddress_snap)));

        if(optRecovery.has_value() && !fRecoveryGenesisMatches)
        {
            debug::warning(FUNCTION, "SESSION RECOVERY GENESIS MISMATCH during template creation: recovered_genesis=",
                           FullHexOrUnset(optRecovery->hashGenesis),
                           " current_context_genesis=", FullHexOrUnset(hashGenesis_snap));
        }
        
        debug::log(0, "   Block parameters:");
        debug::log(0, "      Channel: ", nChannel_snap);
        debug::log(0, "      Session ID: ", nSessionId_snap);
        debug::log(0, "      Falcon authenticated: ", fAuthenticated_snap ? "Yes" : "No");
        
        /* Ensure wallet is unlocked for mining (matches legacy Miner::new_block behavior).
         * CreateBlockForStatelessMining() requires Authentication::Unlocked(PinUnlock::MINING).
         * Legacy miners auto-unlock; stateless miners must do the same. */
        if(!TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
        {
            try
            {
                SecureString strPIN;
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
                debug::log(0, FUNCTION, "Wallet auto-unlocked for mining");
            }
            catch(const std::exception& e)
            {
                debug::error(FUNCTION, "Mining unlock failed: ", e.what());
                debug::error(FUNCTION, "Start node with -autologin=username:password or unlock mining manually");
                return nullptr;
            }
        }

        /* Prime channel optimization */
        const uint32_t nBitMask = config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;
        TAO::Ledger::TritiumBlock* pBlock = nullptr;
        
        /* Use simplified utility function */
        debug::log(0, "   Calling CreateBlockForStatelessMining...");
        uint32_t nAttempts = 0;
        while(true) {
            /* Check for shutdown during template creation loop */
            if (config::fShutdown.load())
            {
                debug::log(1, FUNCTION, "Shutdown detected during block creation; aborting");
                return nullptr;
            }
            
            ++nAttempts;
            const uint32_t extraNonce =
                nBlockIterator.fetch_add(1, std::memory_order_relaxed) + 1;
            pBlock = TAO::Ledger::CreateBlockForStatelessMining(
                nChannel_snap,
                extraNonce,
                hashReward
            );
            
            if(!pBlock) {
                debug::log(0, ANSI_COLOR_BRIGHT_RED, "   FAILED: CreateBlockForStatelessMining returned nullptr", ANSI_COLOR_RESET);
                return nullptr;
            }
            
            if(is_prime_mod(nBitMask, pBlock)) {
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   SUCCESS: Block created (attempts: ", nAttempts, ")", ANSI_COLOR_RESET);
                break;
            }
            
            delete pBlock;
            pBlock = nullptr;
        }
        
        /* PR #136: Use ChannelStateManager for fork-aware state management */
        uint64_t nCreationTime = runtime::unifiedtimestamp();
        
        /* Get channel manager for this channel */
        ChannelStateManager* pChannelMgr = GetChannelManager(nChannel_snap);
        if(!pChannelMgr)
        {
            debug::error(FUNCTION, "Failed to get channel manager for channel ", nChannel_snap);
            delete pBlock;
            return nullptr;
        }
        
        /* Sync with blockchain (detects forks automatically) */
        debug::log(0, "   📡 Syncing channel manager with blockchain...");
        if(!pChannelMgr->SyncWithBlockchain())
        {
            debug::error(FUNCTION, "Failed to sync channel manager with blockchain");
            delete pBlock;
            return nullptr;
        }
        
        /* Check for fork detection */
        if(pChannelMgr->IsForkDetected())
        {
            debug::warning(FUNCTION, "⚠ Fork detected during template creation!");
            debug::warning(FUNCTION, "   Blocks rolled back: ", pChannelMgr->GetBlocksRolledBack());
            debug::warning(FUNCTION, "   Clearing stale templates...");
            
            /* Clear templates first, then clear fork flag.
             * Lock ordering: MUTEX protects all mapBlocks writes in clear_map(). */
            {
                std::lock_guard<std::mutex> map_lk(MUTEX);
                clear_map();
            }
            
            /* Clear fork flag after successful cleanup */
            pChannelMgr->ClearForkFlag();
        }
        
        /* Get comprehensive height info from manager */
        HeightInfo info = pChannelMgr->GetHeightInfo();
        
        /* Log comprehensive state (PR #136: Enhanced diagnostics) */
        debug::log(0, "   ✓ Channel state synchronized:");
        debug::log(0, "      Channel: ", pChannelMgr->GetChannelName());
        debug::log(0, "      Unified blockchain height: ", info.nUnifiedHeight, " (current)");
        debug::log(0, "      ", pChannelMgr->GetChannelName(), " channel height: ", info.nChannelHeight, " (last block in channel)");
        debug::log(0, "      Template mining for unified height: ", info.nNextUnifiedHeight);
        debug::log(0, "      Template mining for ", pChannelMgr->GetChannelName(), " height: ", info.nNextChannelHeight);
        
        if(info.fForkDetected)
        {
            debug::log(0, "      ⚠ Fork recently detected (", info.nBlocksRolledBack, " blocks rolled back)");
        }
        
        /* Create metadata with heights from manager (PR #136) */
        /* Use nNextChannelHeight because template is mining for NEXT block in channel */
        debug::log(0, "   Creating template metadata:");
        debug::log(0, "      unified_current (ChainState best):  ", info.nUnifiedHeight);
        debug::log(0, "      unified_next    (best + 1):         ", info.nNextUnifiedHeight);
        debug::log(0, "      channel_current (last in channel):  ", info.nChannelHeight);
        debug::log(0, "      channel_target  (next in channel):  ", info.nNextChannelHeight, " = pBlock->nHeight=", pBlock->nHeight);
        debug::log(0, "      prev_hash       (template anchor):  ", pBlock->hashPrevBlock.SubString());
        debug::log(0, "      best_hash       (current tip):      ", info.hashCurrentBlock.SubString());
        /* Full hashPrevBlock hex (MSB-first via GetHex()) for cross-verification with miner's GetBytes()[0..7] log. */
        debug::log(2, FUNCTION, "hashPrevBlock FULL (MSB-first): ", pBlock->hashPrevBlock.GetHex());
        debug::log(2, FUNCTION, "hashPrevBlock SubString (LSB, legacy display): ", pBlock->hashPrevBlock.SubString());
        debug::log(2, FUNCTION, "hashBestChain SubString (current tip):         ", TAO::Ledger::ChainState::hashBestChain.load().SubString());
        debug::log(2, FUNCTION, "hashPrevBlock == hashBestChain: ", (pBlock->hashPrevBlock == TAO::Ledger::ChainState::hashBestChain.load()));
        
        /* Capture the merkle root key before the move — pBlock may be invalid after emplace */
        const uint512_t hashMerkleKey = pBlock->hashMerkleRoot;

        TemplateMetadata meta(pBlock, nCreationTime, info.nUnifiedHeight, info.nNextChannelHeight, 
                             hashMerkleKey, nChannel_snap,
                             TAO::Ledger::ChainState::hashBestChain.load());
        TAO::Ledger::Block* pStableBlock = nullptr;
        {
            /* Lock ordering: MUTEX protects all mapBlocks writes (and reads in ProcessPacket handlers).
             * TEMPLATE_CREATE_MUTEX is never held when this lock is acquired. */
            std::lock_guard<std::mutex> map_lk(MUTEX);
            auto result = mapBlocks.emplace(hashMerkleKey, std::move(meta));

            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Template stored in map with metadata", ANSI_COLOR_RESET);
            debug::log(0, "      Merkle root: ", hashMerkleKey.SubString());
            debug::log(0, "      Unified height (current):  ", info.nUnifiedHeight);
            debug::log(0, "      Channel height (target):   ", info.nNextChannelHeight, " (mining for next ", pChannelMgr->GetChannelName(), " block)");
            debug::log(0, "      Channel: ", pChannelMgr->GetChannelName());
            debug::log(0, "      Creation time: ", nCreationTime);
            debug::log(0, "      Templates in map: ", mapBlocks.size());

            /* ✅ ADD: Verify stored value matches what we intended */
            if(result.second)  // Successfully inserted
            {
                if(result.first->second.nChannelHeight != info.nNextChannelHeight)
                {
                    debug::error(FUNCTION, "❌ CRITICAL: Template stored with wrong nChannelHeight!");
                    debug::error(FUNCTION, "   Expected: ", info.nNextChannelHeight);
                    debug::error(FUNCTION, "   Got: ", result.first->second.nChannelHeight);
                }
                else
                {
                    debug::log(0, "   ✓ Verified: Template has correct nChannelHeight=", info.nNextChannelHeight);
                }
            }
            else
            {
                /* Duplicate key: another caller already stored a template for this merkle root.
                 * The newly created block (in meta) may have been destroyed by the failed emplace.
                 * Use the map key (result.first->first) to log — do NOT dereference pBlock here.
                 * Return the existing map entry's pointer which remains valid. */
                debug::log(1, FUNCTION, "⚠ Duplicate merkle root — returning existing map entry for ", result.first->first.SubString());
            }

            /* Return the stable pointer owned by the map entry, not the pre-move raw pointer.
             * If emplace succeeded, result.first->second.pBlock.get() == pBlock (same object).
             * If emplace failed (duplicate key), pBlock would be destroyed at end of scope;
             * result.first->second.pBlock.get() points to the existing valid template instead. */
            pStableBlock = result.first->second.pBlock.get();
        }
        creationScope.result = pStableBlock;

        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Complete ===", ANSI_COLOR_RESET);
        return pStableBlock;
    }


    /** Sign a block */
    bool StatelessMinerConnection::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot, const std::vector<uint8_t>& vOffsets)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📝 === SIGN_BLOCK: Updating template ===", ANSI_COLOR_RESET);
        debug::log(0, "   Requested merkle root: ", hashMerkleRoot.SubString());
        debug::log(0, "   Nonce: 0x", std::hex, nNonce, std::dec);
        
        /* ✅ NEW: Check if any templates exist first */
        if(mapBlocks.empty())
        {
            debug::error(FUNCTION, "════════════════════════════════════════");
            debug::error(FUNCTION, "   ❌ NO TEMPLATES AVAILABLE");
            debug::error(FUNCTION, "════════════════════════════════════════");
            debug::error(FUNCTION, "This means:");
            debug::error(FUNCTION, "  - Template expired (height changed)");
            debug::error(FUNCTION, "  - Template was never created");
            debug::error(FUNCTION, "  - Template cleanup removed it");
            debug::error(FUNCTION, "");
            debug::error(FUNCTION, "ACTION: Miner should request new template via GET_BLOCK");
            debug::error(FUNCTION, "════════════════════════════════════════");
            return false;
        }
        
        /* ✅ NEW: Look up template by merkle root */
        auto it = mapBlocks.find(hashMerkleRoot);
        if(it == mapBlocks.end())
        {
            debug::error(FUNCTION, "════════════════════════════════════════");
            debug::error(FUNCTION, "   ❌ MERKLE ROOT MISMATCH");
            debug::error(FUNCTION, "════════════════════════════════════════");
            debug::error(FUNCTION, "Requested merkle: ", hashMerkleRoot.SubString());
            debug::error(FUNCTION, "");
            debug::error(FUNCTION, "Known templates in map:");
            for(const auto& entry : mapBlocks)
            {
                const TemplateMetadata& meta = entry.second;
                uint64_t nAge = runtime::unifiedtimestamp() - meta.nCreationTime;
                debug::error(FUNCTION, "  ✓ ", entry.first.SubString());
                debug::error(FUNCTION, "    Height: ", meta.nHeight);
                debug::error(FUNCTION, "    Age: ", nAge, "s");
                debug::error(FUNCTION, "    Channel: ", meta.nChannel);
            }
            debug::error(FUNCTION, "");
            debug::error(FUNCTION, "POSSIBLE CAUSES:");
            debug::error(FUNCTION, "  1. Miner computed wrong merkle root (BUG in miner)");
            debug::error(FUNCTION, "  2. Miner submitting work from old template");
            debug::error(FUNCTION, "  3. Height changed between GET_BLOCK and SUBMIT_BLOCK");
            debug::error(FUNCTION, "  4. Miner's nonce caused merkle to change (overflow?)");
            debug::error(FUNCTION, "");
            debug::error(FUNCTION, "ACTION: Miner should poll GET_ROUND and request new template");
            debug::error(FUNCTION, "════════════════════════════════════════");
            return false;
        }
        
        /* ✅ NEW: Validate template is still valid */
        const TemplateMetadata& meta = it->second;
        
        if(meta.IsStale())
        {
            uint64_t nAge = runtime::unifiedtimestamp() - meta.nCreationTime;
            debug::error(FUNCTION, "❌ Template is too old (", nAge, "s)");
            debug::error(FUNCTION, "   Max age: ", LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS, "s");
            return false;
        }
        
        /* PR #134: Check channel-specific staleness using IsStale() */
        /* Note: IsStale() now checks both age and channel height */
        if(meta.IsStale())
        {
            uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();
            debug::error(FUNCTION, "❌ Template is STALE");
            debug::error(FUNCTION, "   Template unified height: ", meta.nHeight);
            debug::error(FUNCTION, "   Template channel height: ", meta.nChannelHeight);
            debug::error(FUNCTION, "   Current blockchain height: ", nCurrentHeight);
            debug::error(FUNCTION, "   Reason: Channel height changed or age exceeded");
            return false;
        }
        
        /* ✅ Template is valid - proceed with solved-candidate construction */
        TAO::Ledger::Block* pBaseBlock = meta.pBlock.get();
        if(!pBaseBlock)
        {
            debug::error(FUNCTION, "❌ Template has null block pointer!");
            return false;
        }

        /* If the block dynamically casts to a tritium block, validate the tritium block. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(pBaseBlock);
        if(pBlock)
        {
            debug::log(0, "   Merkle root: ", hashMerkleRoot.SubString());
            debug::log(0, "   Miner's nonce: 0x", std::hex, nNonce, std::dec);
            debug::log(0, "   Block height: ", pBlock->nHeight);
            debug::log(0, "   Block channel: ", pBlock->nChannel);

            /* Build a canonical solved candidate from the immutable template.
             *
             * For Prime (channel 1): use BuildSolvedPrimeCandidateFromTemplate which:
             *   - copies all consensus-critical fields from the original template
             *   - applies the miner's nNonce and vOffsets
             *   - preserves nTime (ProofHash for Prime excludes nTime)
             *   - clears vchBlockSig so FinalizeWalletSignatureForSolvedBlock can re-sign
             *
             * For Hash (channel 2): use BuildSolvedHashCandidateFromTemplate which:
             *   - copies all consensus-critical fields from the original template
             *   - applies the miner's nNonce
             *   - clears vOffsets (Hash channel invariant — no Cunningham chain)
             *   - preserves nTime (ProofHash for Hash also excludes nTime)
             *   - clears vchBlockSig so FinalizeWalletSignatureForSolvedBlock can re-sign
             *
             * Both helpers do NOT mutate the original template.  The solved candidate
             * is written back into the template slot so downstream validate_block() /
             * AcceptMinedBlock() operate on the fully-prepared signed block. */
            if(pBlock->nChannel == TAO::Ledger::CHANNEL::PRIME)
            {
                *pBlock = TAO::Ledger::BuildSolvedPrimeCandidateFromTemplate(*pBlock, nNonce, vOffsets);

                /* Structural validation of miner-submitted Prime offsets.
                 * The prior GetOffsets(GetPrime()) equivalence check has been removed:
                 * it returned empty vOffsets whenever GetPrime() was not itself prime,
                 * producing false rejections for valid Prime submissions.
                 * VerifySubmittedPrimeOffsets() does lightweight structural checks;
                 * the authoritative PoW gate remains VerifyWork() inside Check().
                 *
                 * The empty check guards the legacy-fallback path below: when the miner
                 * does not submit vOffsets (compact wrapper), we fall through to GetOffsets()
                 * rather than rejecting. Only non-empty submissions are validated here. */
                if(!pBlock->vOffsets.empty() &&
                   !TAO::Ledger::VerifySubmittedPrimeOffsets(*pBlock, pBlock->vOffsets))
                {
                    debug::error(FUNCTION, "Prime vOffsets structural validation failed — BLOCK_REJECTED");
                    return false;
                }

                /* Legacy fallback: derive offsets locally when the miner did not submit
                 * them (compact wrapper path).  Backwards-compatible with older miners. */
                if(pBlock->vOffsets.empty())
                    TAO::Ledger::GetOffsets(pBlock->GetPrime(), pBlock->vOffsets);

                debug::log(2, FUNCTION, "Prime channel: solved candidate built (nTime preserved, vOffsets applied)");
            }
            else if(pBlock->nChannel == TAO::Ledger::CHANNEL::HASH)
            {
                *pBlock = TAO::Ledger::BuildSolvedHashCandidateFromTemplate(*pBlock, nNonce);
                debug::log(2, FUNCTION, "Hash channel: solved candidate built (nTime preserved, vOffsets cleared)");
            }
            else
            {
                /* Unknown channel — apply nNonce directly and clear offsets as a safe fallback. */
                pBlock->nNonce = nNonce;
                pBlock->vOffsets.clear();
                pBlock->vchBlockSig.clear();
                debug::log(2, FUNCTION, "Unknown channel (", pBlock->nChannel, "): applied nNonce directly");
            }

            debug::log(0, "   ✅ Solved candidate ready");
            debug::log(0, "      Height: ", meta.nHeight);
            debug::log(0, "      Age: ", runtime::unifiedtimestamp() - meta.nCreationTime, "s");

            /* ============================================================
             * TRAINING WHEELS MODE: Comprehensive Diagnostic Logging
             * ============================================================ */

            if(pBlock->nChannel == 1)  // Prime channel
            {
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === PRIME CHANNEL DIAGNOSTIC (Training Wheels Mode) ===", ANSI_COLOR_RESET);

                /* hashPrime for diagnostic display.
                 * NOTE: vOffsets and verification were already handled by
                 * BuildSolvedPrimeCandidateFromTemplate + VerifySubmittedPrimeOffsets
                 * above.  This block is diagnostic-only. */
                uint1024_t hashPrime = pBlock->GetPrime();

                debug::log(0, "📊 PRIME BASE CALCULATION:");
                debug::log(0, "   ProofHash() = ", pBlock->ProofHash().ToString().substr(0, 64), "...");
                debug::log(0, "   nNonce      = 0x", std::hex, pBlock->nNonce, std::dec);
                debug::log(0, "   hashPrime   = ProofHash() + nNonce");
                debug::log(0, "   hashPrime   = ", hashPrime.ToString().substr(0, 64), "...");
                debug::log(0, "   (Full 1024-bit value shown in hex above)");

                /* NOTE: The standalone PrimeCheck() call that previously appeared here
                 * has been removed.  It ran the full Miller-Rabin + Fermat primality
                 * test for diagnostic purposes only (result was never used for accept/
                 * reject decisions) and produced a spurious first "PRIMECHECK DIAGNOSTIC"
                 * series in the logs before the consensus VerifyWork() checks, making
                 * it appear as if three separate Prime validations occurred per block.
                 * The authoritative primality gate is VerifyWork() inside block.Check(),
                 * which runs inside ValidateMinedBlock() further down this call stack. */

                /* Display Cunningham chain offsets (already validated above). */
                debug::log(0, "");
                debug::log(0, "🔗 CUNNINGHAM CHAIN OFFSETS:");
                debug::log(0, "   vOffsets.size() = ", pBlock->vOffsets.size());
                
                if(!pBlock->vOffsets.empty())
                {
                    debug::log(0, "   ", ANSI_COLOR_BRIGHT_GREEN, "✅ Offsets found:", ANSI_COLOR_RESET);
                    
                    /* Show first 10 offsets (or all if fewer) */
                    size_t numToShow = std::min(size_t(10), pBlock->vOffsets.size());
                    for(size_t i = 0; i < numToShow; ++i)
                    {
                        debug::log(0, "      vOffsets[", i, "] = ", static_cast<int>(pBlock->vOffsets[i]));
                    }
                    
                    if(pBlock->vOffsets.size() > 10)
                    {
                        debug::log(0, "      ... (", pBlock->vOffsets.size() - 10, " more offsets)");
                    }
                    
                    /* Calculate difficulty from offsets */
                    debug::log(0, "");
                    debug::log(0, "📈 DIFFICULTY CALCULATION:");
                    double nPrimeDifficulty = TAO::Ledger::GetPrimeDifficulty(hashPrime, pBlock->vOffsets, true);
                    double nRequiredDifficulty = TAO::Ledger::GetDifficulty(pBlock->nBits, 1);
                    
                    debug::log(0, "   Prime difficulty:    ", std::fixed, std::setprecision(6), nPrimeDifficulty);
                    debug::log(0, "   Required difficulty: ", std::fixed, std::setprecision(6), nRequiredDifficulty);
                    
                    if(nPrimeDifficulty >= nRequiredDifficulty)
                    {
                        debug::log(0, "   ", ANSI_COLOR_BRIGHT_GREEN, "✅ Difficulty meets requirement", ANSI_COLOR_RESET);
                    }
                    else
                    {
                        debug::log(0, "   ", ANSI_COLOR_BRIGHT_RED, "❌ Difficulty too low", ANSI_COLOR_RESET);
                        debug::log(0, "   Deficit: ", nRequiredDifficulty - nPrimeDifficulty);
                    }
                }
                else
                {
                    debug::log(0, "   ", ANSI_COLOR_BRIGHT_RED, "❌ NO OFFSETS FOUND", ANSI_COLOR_RESET);
                    debug::log(0, "   ⚠️  This means GetOffsets() found zero valid primes in the Cunningham chain");
                    debug::log(0, "   ⚠️  Expected: Array of offsets like [2, 6, 12, 66, 146, 32, 0]");
                    debug::log(0, "   ⚠️  Got: Empty array");
                    debug::log(0, "   ⚠️  Block will FAIL Check() validation");
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === END PRIME DIAGNOSTIC ===", ANSI_COLOR_RESET);
            }
            else if(pBlock->nChannel == 2)  // Hash channel
            {
                /* vOffsets was already cleared by BuildSolvedHashCandidateFromTemplate above.
                 * This diagnostic block logs the proof-of-work state for debugging. */
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === HASH CHANNEL DIAGNOSTIC (Training Wheels Mode) ===", ANSI_COLOR_RESET);

                /* Calculate proof hash */
                uint1024_t hashProof = pBlock->ProofHash();

                debug::log(0, "📊 HASH PROOF CALCULATION:");
                debug::log(0, "   ProofHash() = SK1024(block_header)");
                debug::log(0, "   hashProof   = ", hashProof.ToString().substr(0, 64), "...");
                debug::log(0, "   nNonce      = 0x", std::hex, pBlock->nNonce, std::dec);
                
                /* Get target from nBits */
                LLC::CBigNum bnTarget;
                bnTarget.SetCompact(pBlock->nBits);
                uint1024_t nTarget = bnTarget.getuint1024();
                
                debug::log(0, "");
                debug::log(0, "🎯 TARGET VALIDATION:");
                debug::log(0, "   nBits   = 0x", std::hex, pBlock->nBits, std::dec);
                debug::log(0, "   nTarget = ", nTarget.ToString().substr(0, 64), "...");
                
                /* Check if hash meets target */
                debug::log(0, "");
                debug::log(0, "🧪 PROOF-OF-WORK TEST:");
                debug::log(0, "   Checking: hashProof <= nTarget");
                
                if(hashProof <= nTarget)
                {
                    debug::log(0, "   Result: ", ANSI_COLOR_BRIGHT_GREEN, "✅ HASH MEETS TARGET", ANSI_COLOR_RESET);
                    
                    /* Calculate leading zeros for reference */
                    int nLeadingZeros = 0;
                    uint1024_t temp = hashProof;
                    while(temp > 0 && nLeadingZeros < 1024)
                    {
                        if(temp.high_bits(0x80000000))
                            break;
                        temp <<= 1;
                        ++nLeadingZeros;
                    }
                    
                    debug::log(0, "   Leading zeros: ", nLeadingZeros, " bits");
                }
                else
                {
                    debug::log(0, "   Result: ", ANSI_COLOR_BRIGHT_RED, "❌ HASH DOES NOT MEET TARGET", ANSI_COLOR_RESET);
                    debug::log(0, "   ⚠️  Mismatch: Miner validated this hash, but node disagrees!");
                    debug::log(0, "   ⚠️  Hash is too high (difficulty not met)");
                }
                
                /* Confirm vOffsets invariant (should already be empty from BuildSolvedHashCandidateFromTemplate). */
                if(!pBlock->vOffsets.empty())
                {
                    debug::error(FUNCTION, "Hash channel vOffsets not empty after BuildSolvedHashCandidateFromTemplate — clearing (invariant violation)");
                    pBlock->vOffsets.clear();
                }

                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === END HASH DIAGNOSTIC ===", ANSI_COLOR_RESET);
            }
            
            debug::log(0, "");
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Block prepared for validation", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📝 === SIGN_BLOCK: Complete ===", ANSI_COLOR_RESET);

            /* Generate the canonical block signature.
             * SignatureHash() covers nNonce and vOffsets; the template's prior
             * signature (produced at template creation with nNonce=1, vOffsets=empty)
             * is no longer valid after the miner's nNonce and vOffsets are applied.
             * FinalizeWalletSignatureForSolvedBlock() re-signs the block so that
             * TritiumBlock::Check() → VerifySignature() passes in ValidateMinedBlock().
             *
             * Architecture note: the Falcon signature authenticates the stateless session
             * transport (miner identity + payload integrity).  The wallet signature
             * (vchBlockSig) is the consensus-visible proof of authorised block production,
             * verifiable by any network peer without Stateless Node. */
            if(!TAO::Ledger::FinalizeWalletSignatureForSolvedBlock(*pBlock))
            {
                debug::error(FUNCTION, "FinalizeWalletSignatureForSolvedBlock failed — BLOCK_REJECTED");
                return false;
            }
            debug::log(2, FUNCTION, "Wallet signature applied to solved block");
            
            return true;
        }

        /* If we get here, the block is null or doesn't exist. */
        return debug::error(FUNCTION, "null block");
    }


    /** Validate a block */
    bool StatelessMinerConnection::validate_block(const uint512_t& hashMerkleRoot)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "✅ === VALIDATE_BLOCK: Final consensus validation ===", ANSI_COLOR_RESET);

        /* Safe map access to avoid creating null entry */
        auto it = mapBlocks.find(hashMerkleRoot);
        if(it == mapBlocks.end())
        {
            debug::error(FUNCTION, "Block not found in map");
            return false;
        }

        /* Get block from metadata */
        const TemplateMetadata& meta = it->second;
        if(!meta.pBlock)
        {
            debug::error(FUNCTION, "Block has null pointer in metadata");
            return false;
        }

        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock*>(meta.pBlock.get());
        if(pBlock)
        {
            debug::log(2, FUNCTION, "Tritium");
            pBlock->print();
            
            /* ============================================================
             * TRAINING WHEELS MODE: Pre-Check() Diagnostic
             * ============================================================ */
            
            debug::log(0, "");
            debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === PRE-CHECK() DIAGNOSTIC ===", ANSI_COLOR_RESET);
            debug::log(0, "📋 BLOCK STATE BEFORE Check():");
            debug::log(0, "   Height:      ", pBlock->nHeight);
            debug::log(0, "   Channel:     ", pBlock->nChannel);
            debug::log(0, "   nBits:       0x", std::hex, pBlock->nBits, std::dec);
            debug::log(0, "   nNonce:      0x", std::hex, pBlock->nNonce, std::dec);
            debug::log(0, "   Merkle root: ", pBlock->hashMerkleRoot.SubString());
            debug::log(0, "   vOffsets.size(): ", pBlock->vOffsets.size());
            
            if(pBlock->nChannel == 1 && !pBlock->vOffsets.empty())
            {
                debug::log(0, "   First 5 offsets:");
                for(size_t i = 0; i < std::min(size_t(5), pBlock->vOffsets.size()); ++i)
                {
                    debug::log(0, "      [", i, "] = ", static_cast<int>(pBlock->vOffsets[i]));
                }
            }
            
            debug::log(0, "");
            debug::log(0, "🧪 CALLING TritiumBlock::Check()...");
            
            /* Call canonical Check() validation */
            bool fCheckResult = pBlock->Check();
            
            debug::log(0, "");
            if(fCheckResult)
            {
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "✅ Check() PASSED", ANSI_COLOR_RESET);
                debug::log(0, "   All block validations successful:");
                debug::log(0, "   ✓ Block structure valid");
                debug::log(0, "   ✓ Producer transaction valid");
                debug::log(0, "   ✓ Proof-of-work valid");
                if(pBlock->nChannel == 1)
                    debug::log(0, "   ✓ Prime Cunningham chain valid");
                else if(pBlock->nChannel == 2)
                    debug::log(0, "   ✓ Hash meets target");
            }
            else
            {
                debug::log(0, ANSI_COLOR_BRIGHT_RED, "❌ Check() FAILED", ANSI_COLOR_RESET);
                debug::log(0, "   Block failed consensus validation");
                
                /* ✅ ADD: Enhanced Prime channel diagnostics */
                if(pBlock->nChannel == 1)  // Prime channel
                {
                    debug::error(FUNCTION, "════════════════════════════════════════");
                    debug::error(FUNCTION, "   PRIME CHANNEL VALIDATION FAILED");
                    debug::error(FUNCTION, "════════════════════════════════════════");
                    debug::error(FUNCTION, "Block details:");
                    debug::error(FUNCTION, "  Height: ", pBlock->nHeight);
                    debug::error(FUNCTION, "  Channel: 1 (Prime)");
                    debug::error(FUNCTION, "  Merkle: ", pBlock->hashMerkleRoot.SubString());
                    debug::error(FUNCTION, "  nNonce: ", pBlock->nNonce);
                    debug::error(FUNCTION, "");
                    debug::error(FUNCTION, "NOTE: PR #129 added Miller-Rabin primality test");
                    debug::error(FUNCTION, "If this is a recent build, Check() now uses");
                    debug::error(FUNCTION, "cryptographically secure primality validation.");
                    debug::error(FUNCTION, "");
                    debug::error(FUNCTION, "Common Prime mining failures:");
                    debug::error(FUNCTION, "  - Empty vOffsets (no valid Cunningham chain)");
                    debug::error(FUNCTION, "  - Base prime not actually prime");
                    debug::error(FUNCTION, "  - Chain length insufficient");
                    debug::error(FUNCTION, "════════════════════════════════════════");
                }
                else
                {
                    debug::log(0, "   Common failure reasons:");
                    debug::log(0, "   - Prime: Empty vOffsets or invalid Cunningham chain");
                    debug::log(0, "   - Hash: Proof hash doesn't meet target");
                    debug::log(0, "   - Producer transaction invalid");
                    debug::log(0, "   - Block structure malformed");
                }
                
                debug::error(FUNCTION, "Block failed Check() validation");
                debug::error(FUNCTION, "  Height: ", pBlock->nHeight);
                debug::error(FUNCTION, "  Channel: ", pBlock->nChannel);
                debug::error(FUNCTION, "  Merkle: ", pBlock->hashMerkleRoot.SubString());
                
                return false;
            }
            
            debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === END PRE-CHECK() DIAGNOSTIC ===", ANSI_COLOR_RESET);
            debug::log(0, "");

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
                debug::log(0, ANSI_COLOR_BRIGHT_RED, "❌ AcceptMinedBlock rejected block", ANSI_COLOR_RESET);
                debug::log(0, "   Reason: ", acceptanceResult.reason);
                debug::log(0, "   Status flags: 0x", std::hex, static_cast<int>(acceptanceResult.status), std::dec);
                return false;
            }

            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "✅ AcceptMinedBlock accepted block", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "✅ === VALIDATE_BLOCK: SUCCESS ===", ANSI_COLOR_RESET);
            return true;
        }

        debug::error(FUNCTION, "Unexpected non-Tritium block in metadata");
        return false;
    }


    /** Helper function used for prime channel modification rule in loop */
    bool StatelessMinerConnection::is_prime_mod(uint32_t nBitMask, TAO::Ledger::Block *pBlock)
    {
        /* Get the proof hash. */
        uint1024_t hashProof = pBlock->ProofHash();

        /* Get channel from context */
        uint32_t nChannel = context.nChannel;

        /* Skip if not prime channel or version less than 5 */
        if(nChannel != 1 || pBlock->nVersion < 5)
            return true;

        /* Exit loop when the block is above minimum prime origins and less than 1024-bit hashes */
        if(hashProof > TAO::Ledger::bnPrimeMinOrigins.getuint1024() && !hashProof.high_bits(nBitMask))
            return true;

        /* Otherwise keep looping. */
        return false;
    }


    /** Clear the blocks map */
    void StatelessMinerConnection::clear_map()
    {
        /* Clear the map - unique_ptr automatically deletes all blocks */
        mapBlocks.clear();
    }


    /** GetChannelManager (PR #136: Fork-Aware Channel State Management)
     * 
     *  Get the appropriate channel state manager for a given channel.
     */
    ChannelStateManager* StatelessMinerConnection::GetChannelManager(uint32_t nChannel)
    {
        switch(nChannel)
        {
            case 1:  // Prime channel
                return m_pPrimeState.get();
            
            case 2:  // Hash channel
                return m_pHashState.get();
            
            default:
                debug::error(FUNCTION, "Invalid channel: ", nChannel);
                return nullptr;
        }
    }


    /** CleanupStaleTemplates
     * 
     *  Remove templates that are no longer valid due to height changes or age.
     */
    void StatelessMinerConnection::CleanupStaleTemplates(uint32_t nCurrentHeight)
    {
        uint32_t nRemoved = 0;
        uint64_t nNow = runtime::unifiedtimestamp();
        /* Keep a short warm window (2 blocks) to survive brief bursts/reorgs without
         * dropping to zero templates, while still pruning old entries quickly. */
        static constexpr uint32_t TEMPLATE_RETENTION_BLOCKS = 2;
        std::map<uint32_t, std::pair<uint64_t, uint512_t>> latestByChannel;

        debug::log(2, FUNCTION, "🧹 Cleaning stale templates...");
        debug::log(2, FUNCTION, "   Current height: ", nCurrentHeight);
        debug::log(2, FUNCTION, "   Templates before cleanup: ", mapBlocks.size());

        /* Get current blockchain state to determine current channel heights */
        TAO::Ledger::BlockState stateCurrent = TAO::Ledger::ChainState::tStateBest.load();

        /* Cache current channel heights for each channel (0=Stake, 1=Prime, 2=Hash) */
        std::map<uint32_t, uint32_t> currentChannelHeights;
        for(uint32_t nChannel = 0; nChannel <= 2; ++nChannel)
        {
            TAO::Ledger::BlockState stateChannel = stateCurrent;
            if(TAO::Ledger::GetLastState(stateChannel, nChannel))
            {
                currentChannelHeights[nChannel] = stateChannel.nChannelHeight;
                debug::log(2, FUNCTION, "   Current ",
                          (nChannel == 0 ? "Stake" : nChannel == 1 ? "Prime" : "Hash"),
                          " channel height: ", stateChannel.nChannelHeight);
            }
        }

        for(const auto& entry : mapBlocks)
        {
            const TemplateMetadata& meta = entry.second;
            auto itLatest = latestByChannel.find(meta.nChannel);
            if(itLatest == latestByChannel.end() || meta.nCreationTime > itLatest->second.first)
                latestByChannel[meta.nChannel] = std::make_pair(meta.nCreationTime, entry.first);
        }

        for(auto it = mapBlocks.begin(); it != mapBlocks.end(); )
        {
            const TemplateMetadata& meta = it->second;
            const auto itLatest = latestByChannel.find(meta.nChannel);
            const bool fKeepLatest = (itLatest != latestByChannel.end() && itLatest->second.second == it->first);

            /* Keep at least one hot template per channel to avoid full cold regeneration bursts. */
            if(fKeepLatest)
            {
                ++it;
                continue;
            }

            /* Check staleness by CHANNEL HEIGHT, not unified height.
             * Template with nChannelHeight=N targets mining block N for that channel.
             * If blockchain's channel is now at height >= N, the template is stale.
             * Keep templates within TEMPLATE_RETENTION_BLOCKS of the current channel height. */
            bool fTooOldByBlocks = false;
            auto itCurrentHeight = currentChannelHeights.find(meta.nChannel);
            if(itCurrentHeight != currentChannelHeights.end())
            {
                uint32_t nCurrentChannelHeight = itCurrentHeight->second;
                /* Template targets nChannelHeight (next block in channel).
                 * If current channel is at nCurrentChannelHeight, and template targets
                 * nChannelHeight, template is stale if:
                 * nCurrentChannelHeight >= nChannelHeight + TEMPLATE_RETENTION_BLOCKS */
                if(nCurrentChannelHeight >= meta.nChannelHeight &&
                   (nCurrentChannelHeight - meta.nChannelHeight) >= TEMPLATE_RETENTION_BLOCKS)
                {
                    fTooOldByBlocks = true;
                }
            }

            const bool fTooOldByTime = (meta.GetAge(nNow) > LLP::FalconConstants::MAX_TEMPLATE_AGE_SECONDS);

            if(fTooOldByBlocks || fTooOldByTime)
            {
                uint64_t nAge = nNow - meta.nCreationTime;
                debug::log(2, FUNCTION, "   ❌ Removing stale template (retention window)");
                debug::log(2, FUNCTION, "      Unified height: ", meta.nHeight, " (current: ", nCurrentHeight, ")");

                auto itCurrent = currentChannelHeights.find(meta.nChannel);
                if(itCurrent != currentChannelHeights.end())
                {
                    debug::log(2, FUNCTION, "      Channel height: ", meta.nChannelHeight,
                              " (current: ", itCurrent->second, ", ", meta.GetChannelName(),
                              ", keep <= ", TEMPLATE_RETENTION_BLOCKS, " blocks old)");
                }
                else
                {
                    debug::log(2, FUNCTION, "      Channel height: ", meta.nChannelHeight,
                              " (", meta.GetChannelName(), ")");
                }

                debug::log(2, FUNCTION, "      Age: ", nAge, "s");
                debug::log(2, FUNCTION, "      Merkle: ", it->first.SubString());
                it = mapBlocks.erase(it);  // unique_ptr automatically deletes pBlock
                ++nRemoved;
                continue;
            }

            ++it;
        }

        debug::log(2, FUNCTION, "   ✅ Cleanup complete: ", nRemoved, " templates removed");
        debug::log(2, FUNCTION, "   Templates after cleanup: ", mapBlocks.size());
    }


    // ═══════════════════════════════════════════════════════════════════════
    // RATE LIMITING IMPLEMENTATION
    // ═══════════════════════════════════════════════════════════════════════

    std::string StatelessMinerConnection::GetBlockRateKey() const
    {
        return std::to_string(context.nSessionId) + "|" +
               std::to_string(context.nChannel) + "|" +
               GetAddress().ToStringIP();
    }

    void StatelessMinerConnection::SendGetBlockDataResponse(const std::vector<uint8_t>& vPayload, bool fAuthenticatedPath)
    {
        if(fAuthenticatedPath)
        {
            assert(!vPayload.empty() && "Authenticated GET_BLOCK cannot serialize empty BLOCK_DATA payload");
            if(vPayload.empty())
            {
                debug::error(FUNCTION, "Authenticated GET_BLOCK produced empty BLOCK_DATA payload; converting to INTERNAL_RETRY control response");
                SendGetBlockControlResponse(GetBlockPolicyReason::INTERNAL_RETRY,
                    MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS, true);
                return;
            }
        }

        StatelessPacket response(OpcodeUtility::Stateless::BLOCK_DATA);
        response.DATA = vPayload;
        response.LENGTH = static_cast<uint32_t>(vPayload.size());
        respond(response);

        const uint64_t nSent = ++g_get_block_blockdata_sent_total;
        debug::log(2, FUNCTION, "metric get_block_blockdata_sent_total=", nSent);
    }

    void StatelessMinerConnection::SendGetBlockControlResponse(GetBlockPolicyReason eReason, uint32_t nRetryAfterMs, bool fAuthenticatedPath)
    {
        const bool fLegacyEligible = fAuthenticatedPath && IsGetBlockRetryable(eReason);
        const bool fAllowLegacyEmpty = fLegacyEligible && AllowLegacyEmptyBlockData();

        if(fLegacyEligible && !fAllowLegacyEmpty)
        {
            const uint64_t nBlocked = ++g_get_block_legacy_empty_attempt_blocked_total;
            debug::log(1, FUNCTION, "metric get_block_legacy_empty_attempt_blocked_total=", nBlocked);
        }

        if(fAllowLegacyEmpty)
        {
            StatelessPacket response(OpcodeUtility::Stateless::BLOCK_DATA);
            response.LENGTH = 0;
            respond(response);

            debug::warning(FUNCTION,
                "GET_BLOCK legacy empty BLOCK_DATA enabled via -allow_legacy_empty_block_data=true "
                "[DEPRECATED: removal target 2026-06-30, follow-up PR pending] reason=",
                GetBlockPolicyReasonCode(eReason),
                " retry_after_ms=", nRetryAfterMs);
            return;
        }

        StatelessPacket response(OpcodeUtility::Stateless::BLOCK_REJECTED);
        response.DATA = BuildGetBlockControlPayload(eReason, nRetryAfterMs);
        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        if(eReason == GetBlockPolicyReason::UNAUTHENTICATED)
        {
            response.HEADER = OpcodeUtility::Stateless::AUTH_RESULT;
            response.DATA = std::vector<uint8_t>{0x00};
            response.LENGTH = 1;
        }
        else if(eReason == GetBlockPolicyReason::SESSION_INVALID)
        {
            response.HEADER = OpcodeUtility::Stateless::SESSION_EXPIRED;
            response.DATA = std::vector<uint8_t>{
                static_cast<uint8_t>((context.nSessionId >> 24) & 0xFF),
                static_cast<uint8_t>((context.nSessionId >> 16) & 0xFF),
                static_cast<uint8_t>((context.nSessionId >> 8) & 0xFF),
                static_cast<uint8_t>(context.nSessionId & 0xFF),
                static_cast<uint8_t>(eReason)
            };
            response.LENGTH = static_cast<uint32_t>(response.DATA.size());
        }

        respond(response);

        const uint64_t nReasonCount = IncrementControlCounter(eReason);
        debug::log(1, FUNCTION,
            "get_block_outcome=control reason=", GetBlockPolicyReasonCode(eReason),
            " retry_after_ms=", nRetryAfterMs,
            " auth=", (context.fAuthenticated ? 1 : 0),
            " session_id=", context.nSessionId,
            " peer=", GetAddress().ToStringIP());
        debug::log(1, FUNCTION,
            "metric get_block_control_response_total{reason=", GetBlockPolicyReasonCode(eReason),
            "}=", nReasonCount);
    }

    bool StatelessMinerConnection::CheckRateLimit(uint16_t nRequestType, uint32_t* pnRetryAfterMs)
    {
        auto now = std::chrono::steady_clock::now();
        if(pnRetryAfterMs)
            *pnRetryAfterMs = 0;
        
        // Reset counters every minute
        auto elapsedSinceReset = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_rateLimit.tLastCounterReset).count();
        if (elapsedSinceReset >= 60) {
            ResetMinuteCounters();
        }
        
        // Use stateless protocol constants (16-bit opcodes)
        using namespace StatelessOpcodes;
        const uint16_t GET_ROUND = STATELESS_GET_ROUND;
        const uint16_t GET_BLOCK = STATELESS_GET_BLOCK;
        const uint16_t SUBMIT_BLOCK = STATELESS_SUBMIT_BLOCK;
        
        // If in throttle mode, check minimum interval enforcement for non-GET_BLOCK requests.
        // For GET_BLOCK the rolling limiter is the authoritative gate: throttle mode must only
        // amplify genuine over-budget behaviour, never create an independent gate that blocks
        // an authenticated miner who is still within the rolling window.
        if (m_rateLimit.fThrottleMode && nRequestType != GET_BLOCK) {
            // Default to GET_ROUND timing; overridden below for SUBMIT_BLOCK.
            // GET_BLOCK is excluded from this gate (see condition above) — the
            // rolling limiter is its sole authoritative gate.
            auto lastRequestTime = m_rateLimit.tLastGetRound;
            if (nRequestType == SUBMIT_BLOCK) {
                lastRequestTime = m_rateLimit.tLastSubmitBlock;
            }
            
            auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastRequestTime).count();
            
            if (lastRequestTime.time_since_epoch().count() > 0 && 
                timeSinceLastRequest < MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS) {
                debug::log(1, FUNCTION, "⏳ Connection ", GetAddress().ToStringIP(), 
                    " is throttled - request rejected (too soon: ", timeSinceLastRequest, "ms < ",
                    MiningConstants::GET_BLOCK_THROTTLE_INTERVAL_MS, "ms)");
                RecordViolation("Throttle mode: request too soon");
                return false;
            }
        }
        
        switch (nRequestType) {
            case GET_ROUND:
            {
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "📥 RECEIVED GET_ROUND REQUEST");
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "   From:           ", GetAddress().ToStringIP());
                debug::log(2, "   Opcode:         GET_ROUND (133/0x85)");
                debug::log(2, "   Context:");
                debug::log(2, "      Authenticated:  ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(2, "      Channel:        ", context.nChannel, " (", 
                          context.strChannelName, ")");
                debug::log(2, "      Subscribed:     ", (context.fSubscribedToNotifications ? "YES" : "NO"));
                
                // Determine if this is legacy polling or fallback from failed notifications
                if(context.fSubscribedToNotifications)
                {
                    debug::log(2, "");
                    debug::log(2, "   ⚠️  POTENTIAL ISSUE DETECTED:");
                    debug::log(2, "      Miner is subscribed to push notifications but");
                    debug::log(2, "      is polling with GET_ROUND. This suggests:");
                    debug::log(2, "      1. Client didn't receive push notification, OR");
                    debug::log(2, "      2. Client timed out waiting for notification, OR");
                    debug::log(2, "      3. Client is using hybrid polling+push strategy");
                    debug::log(2, "");
                    debug::log(2, "   Expected flow should be:");
                    debug::log(2, "      Server → ", (context.nChannel == 1 ? "PRIME" : "HASH"), "_BLOCK_AVAILABLE");
                    debug::log(2, "      Client → GET_BLOCK");
                    debug::log(2, "");
                    debug::log(2, "   Fallback behavior (what's happening now):");
                    debug::log(2, "      Client → GET_ROUND (polling)");
                    debug::log(2, "      Server → NEW_ROUND (with heights)");
                }
                else
                {
                    debug::log(2, "");
                    debug::log(2, "   ℹ️  LEGACY POLLING MODE:");
                    debug::log(2, "      Miner is not subscribed to notifications.");
                    debug::log(2, "      Using traditional GET_ROUND polling flow.");
                    debug::log(2, "      This is expected for legacy clients.");
                }
                debug::log(2, "════════════════════════════════════════════════════════════");
                
                // Check minimum interval
                auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_rateLimit.tLastGetRound).count();
                
                if (m_rateLimit.tLastGetRound.time_since_epoch().count() > 0 && 
                    intervalMs < RateLimitConfig::MIN_GET_ROUND_INTERVAL_MS) 
                {
                    std::string reason = "GET_ROUND interval too short: " + 
                        std::to_string(intervalMs) + "ms < " + 
                        std::to_string(RateLimitConfig::MIN_GET_ROUND_INTERVAL_MS) + "ms minimum";
                    RecordViolation(reason);
                    return false;
                }
                
                // Check per-minute limit
                if (m_rateLimit.nGetRoundCount >= RateLimitConfig::MAX_GET_ROUND_PER_MINUTE) {
                    std::string reason = "GET_ROUND limit exceeded: " + 
                        std::to_string(m_rateLimit.nGetRoundCount) + "/" + 
                        std::to_string(RateLimitConfig::MAX_GET_ROUND_PER_MINUTE) + " per minute";
                    RecordViolation(reason);
                    return false;
                }
                
                // Request allowed - update tracking
                m_rateLimit.nGetRoundCount++;
                m_rateLimit.tLastGetRound = now;
                return true;
            }
            
            case GET_BLOCK:
            {
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "📥 RECEIVED GET_BLOCK REQUEST");
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "   From:           ", GetAddress().ToStringIP());
                debug::log(2, "   Opcode:         GET_BLOCK (129/0x81)");
                debug::log(2, "   Context:");
                debug::log(2, "      Authenticated:  ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(2, "      Channel:        ", context.nChannel, " (", 
                          context.strChannelName, ")");
                debug::log(2, "      Subscribed:     ", (context.fSubscribedToNotifications ? "YES" : "NO"));
                
                // Check if this is likely in response to a notification
                if(context.fSubscribedToNotifications && context.nLastNotificationTime > 0)
                {
                    uint64_t nCurrentTime = runtime::unifiedtimestamp();
                    uint64_t nTimeSinceNotification = 0;
                    
                    // Guard against clock adjustments or race conditions
                    if(nCurrentTime >= context.nLastNotificationTime)
                    {
                        nTimeSinceNotification = nCurrentTime - context.nLastNotificationTime;
                    }
                    else
                    {
                        // Clock adjustment detected - log warning
                        debug::log(1, FUNCTION, "⚠️ Clock adjustment detected: current time < last notification time");
                        debug::log(1, FUNCTION, "   This may indicate system clock was adjusted backwards");
                    }
                    
                    debug::log(2, "");
                    debug::log(2, "   ✅ NOTIFICATION FLOW DETECTED:");
                    debug::log(2, "      Time since last notification: ", nTimeSinceNotification, " seconds");
                    debug::log(2, "      This appears to be a response to:");
                    debug::log(2, "         ", (context.nChannel == 1 ? "PRIME_BLOCK_AVAILABLE" : "HASH_BLOCK_AVAILABLE"));
                    debug::log(2, "         (NEW_", (context.nChannel == 1 ? "PRIME" : "HASH"), "_AVAILABLE)");
                }
                else
                {
                    debug::log(2, "");
                    debug::log(2, "   ℹ️  POLLING MODE:");
                    debug::log(2, "      This request is NOT following a notification.");
                    debug::log(2, "      Client may be using legacy polling flow.");
                }
                debug::log(2, "════════════════════════════════════════════════════════════");

                /* SIM-LINK SESSION-SCOPED RATE LIMIT for GET_BLOCK
                 *
                 * The per-connection m_getBlockRollingLimiter has been replaced by a
                 * session-scoped limiter shared across both the legacy (8323) and stateless
                 * (9323) lanes.  A miner with simultaneous connections on both lanes shares
                 * one 20/60s budget, preventing rate-limit bypass via dual-connection.
                 *
                 * Key format: "session=N|combined"
                 */

                // Check for localhost bypass in DEBUG mode
                #ifdef ENABLE_DEBUG
                if (MiningConstants::DISABLE_LOCALHOST_RATE_LIMITING)
                {
                    std::string strIP = GetAddress().ToStringIP();
                    if (strIP == "127.0.0.1" || strIP == "::1" || strIP == "localhost")
                    {
                        debug::log(2, "🔓 DEBUG: Skipping rate limit for localhost connection");
                        m_rateLimit.nGetBlockCount++;
                        m_rateLimit.tLastGetBlock = now;
                        return true;
                    }
                }
                #endif

                /* Delegate to session-scoped limiter (shared 20/60s budget across both lanes) */
                auto pSessionLimiter = StatelessMinerManager::Get().GetSessionRateLimiter(context.nSessionId);
                const std::string strSessionKey = "session=" + std::to_string(context.nSessionId) + "|combined";
                uint32_t nRetryAfterMs = 0;
                std::size_t nCurrentInWindow = 0;
                if(!pSessionLimiter->Allow(strSessionKey, now, nRetryAfterMs, nCurrentInWindow))
                {
                    const std::string strReason =
                        "GET_BLOCK session rate limit exceeded: key=" + strSessionKey +
                        " count=" + std::to_string(nCurrentInWindow) + "/" +
                        std::to_string(RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE) +
                        " window=60s retry_after_ms=" + std::to_string(nRetryAfterMs) +
                        " (combined across all lanes)";
                    RecordViolation(strReason);
                    if(pnRetryAfterMs)
                        *pnRetryAfterMs = nRetryAfterMs;
                    debug::log(1, FUNCTION, "SIM-LINK GET_BLOCK session rate limit: key=", strSessionKey,
                        " count=", nCurrentInWindow, "/", RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE);
                    return false;
                }

                m_rateLimit.tLastGetBlock = now;
                debug::log(3, FUNCTION, "SIM-LINK GET_BLOCK session budget: key=", strSessionKey,
                    " count=", nCurrentInWindow, "/", RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE);
                return true;
            }
            
            case SUBMIT_BLOCK:
            {
                // LENIENT for SUBMIT_BLOCK - miners need to submit solutions!
                // Only check per-minute limit, allow fast submissions
                
                auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_rateLimit.tLastSubmitBlock).count();
                
                if (m_rateLimit.tLastSubmitBlock.time_since_epoch().count() > 0 && 
                    intervalMs < RateLimitConfig::MIN_SUBMIT_BLOCK_INTERVAL_MS) 
                {
                    // Very short interval - might be duplicate, just warn
                    debug::log(2, FUNCTION, "⚠️ Rapid SUBMIT_BLOCK from ", GetAddress().ToStringIP(),
                        " (", intervalMs, "ms interval) - allowing but logging");
                }
                
                if (m_rateLimit.nSubmitBlockCount >= RateLimitConfig::MAX_SUBMIT_BLOCK_PER_MINUTE) {
                    std::string reason = "SUBMIT_BLOCK limit exceeded: " + 
                        std::to_string(m_rateLimit.nSubmitBlockCount) + "/" + 
                        std::to_string(RateLimitConfig::MAX_SUBMIT_BLOCK_PER_MINUTE) + " per minute";
                    RecordViolation(reason);
                    return false;
                }
                
                // Request allowed
                m_rateLimit.nSubmitBlockCount++;
                m_rateLimit.tLastSubmitBlock = now;
                return true;
            }
            
            default:
                return true;  // Unknown request type - allow (don't break new features)
        }
    }

    void StatelessMinerConnection::RecordViolation(const std::string& strReason)
    {
        m_rateLimit.nViolationCount++;
        
        debug::warning(FUNCTION, "⚠️ Rate limit violation #", m_rateLimit.nViolationCount,
            " from ", GetAddress().ToStringIP(), ": ", strReason);
        
        // Graduated response based on violation count
        if (m_rateLimit.nViolationCount <= RateLimitConfig::VIOLATIONS_BEFORE_STRIKE) {
            // Violations 1-3: Warning only, no penalty
            debug::log(1, FUNCTION, "   → Warning issued (", m_rateLimit.nViolationCount, "/", 
                RateLimitConfig::VIOLATIONS_BEFORE_STRIKE, " before strike)");
        }
        else if (m_rateLimit.nViolationCount <= RateLimitConfig::VIOLATIONS_BEFORE_THROTTLE) {
            // Violations 4-6: Add strike
            m_rateLimit.nStrikeCount++;
            debug::log(1, FUNCTION, "   → Strike #", m_rateLimit.nStrikeCount, " recorded");
        }
        else if (m_rateLimit.nViolationCount <= RateLimitConfig::VIOLATIONS_BEFORE_DISCONNECT) {
            // Violations 7-10: Enable throttle mode
            if (!m_rateLimit.fThrottleMode) {
                m_rateLimit.fThrottleMode = true;
                debug::warning(FUNCTION, "   → ⏳ THROTTLE MODE enabled for ", GetAddress().ToStringIP());
                debug::warning(FUNCTION, "   → All requests will be delayed by ", 
                    RateLimitConfig::THROTTLE_DELAY_MS, "ms");
            }
        }
        else {
            // Violations 11+: Disconnect with cooldown
            debug::error(FUNCTION, "❌ Too many violations (", m_rateLimit.nViolationCount, 
                ") from ", GetAddress().ToStringIP());
            debug::error(FUNCTION, "   → Disconnecting with ", 
                RateLimitConfig::COOLDOWN_DURATION_SECONDS, " second cooldown");
            debug::error(FUNCTION, "   → This is AUTOMATED protection, not a ban");
            debug::error(FUNCTION, "   → Cooldown auto-expires - miner can reconnect after");
            
            // Add to auto-expiring cooldown list
            AutoCooldownManager::Get().AddCooldown(GetAddress(), RateLimitConfig::COOLDOWN_DURATION_SECONDS);
            
            // Disconnect the connection
            Disconnect();
        }
    }

    void StatelessMinerConnection::ResetMinuteCounters()
    {
        // Reset per-minute counters
        m_rateLimit.nGetRoundCount = 0;
        m_rateLimit.nGetBlockCount = 0;
        m_rateLimit.nSubmitBlockCount = 0;
        m_rateLimit.nSetChannelCount = 0;
        m_rateLimit.tLastCounterReset = std::chrono::steady_clock::now();
        
        // Gradually reduce violation count (forgiveness over time)
        // The if condition above prevents underflow
        if (m_rateLimit.nViolationCount > 0) {
            m_rateLimit.nViolationCount--;
            
            // Exit throttle mode if violations drop below threshold
            if (m_rateLimit.fThrottleMode && 
                m_rateLimit.nViolationCount < RateLimitConfig::VIOLATIONS_BEFORE_THROTTLE) 
            {
                m_rateLimit.fThrottleMode = false;
                debug::log(1, FUNCTION, "✅ Throttle mode disabled for ", GetAddress().ToStringIP(),
                    " - behavior improved");
            }
        }
        
        debug::log(3, FUNCTION, "Rate limit counters reset for ", GetAddress().ToStringIP());
    }


    /* GetContext - Accessor for mining context (used by server for notifications)
     *
     * Returns a copy of the mining context under the connection mutex to avoid
     * data races with writers (e.g. in ProcessPacket). Callers get a snapshot
     * of the state and cannot mutate the internal context directly.
     */
    MiningContext StatelessMinerConnection::GetContext()
    {
        std::lock_guard<std::mutex> lock(MUTEX);
        return context;
    }


    /* PrepareHeartbeatNotification - Reset per-connection rate-limit state for heartbeat push */
    void StatelessMinerConnection::PrepareHeartbeatNotification()
    {
        LOCK(MUTEX);
        /* Force the next SendChannelNotification() / SendStatelessTemplate() to bypass
         * the push throttle (same bypass used by STATELESS_MINER_READY re-subscription). */
        m_force_next_push = true;
        /* Reassign (not Reset()) so Ready() returns true immediately — restores the
         * "never triggered" state so the miner's first GET_BLOCK after the heartbeat
         * push is served without the 2-second cooldown delay. */
        m_get_block_cooldown = AutoCoolDown(std::chrono::seconds(MiningConstants::GET_BLOCK_COOLDOWN_SECONDS));
    }


    /* SendChannelNotification - Send push notification to subscribed miner */
    void StatelessMinerConnection::SendChannelNotification()
    {
        /* Early exit if shutdown is in progress */
        if (config::fShutdown.load())
        {
            debug::log(2, FUNCTION, "Shutdown in progress; skipping channel notification");
            return;
        }
        
        /* Thread-safe context access - use RAII lock guard */
        uint32_t nChannel;
        {
            LOCK(MUTEX);

            /* Push throttle — drop if a template was sent less than
             * TEMPLATE_PUSH_MIN_INTERVAL_MS ago (guards against fork-resolution bursts).
             * Re-subscription responses bypass via m_force_next_push. */
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

            /* Validate subscription state */
            if (!context.fSubscribedToNotifications)
                return;
            
            /* Validate channel (1=Prime, 2=Hash only) */
            if (context.nSubscribedChannel != 1 && context.nSubscribedChannel != 2)
            {
                debug::error(FUNCTION, "Invalid subscribed channel: ", context.nSubscribedChannel);
                return;
            }
            
            /* Copy channel for use outside lock */
            nChannel = context.nSubscribedChannel;
        }  // MUTEX automatically unlocked here
        
        /* Get blockchain state */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Get channel-specific state */
        TAO::Ledger::BlockState stateChannel = stateBest;
        if (!TAO::Ledger::GetLastState(stateChannel, nChannel))
        {
            debug::error(FUNCTION, "Failed to get channel state for channel ", nChannel);
            return;
        }
        
        /* Get difficulty */
        uint32_t nDifficulty = TAO::Ledger::GetNextTargetRequired(stateBest, nChannel);

        /* Keep the tip hash consistent with the loaded best-state snapshot. */
        const uint1024_t hashBestChain =
            PushNotificationBuilder::BestChainHashForNotification(stateBest);
        
        /* Build notification using unified builder */
        StatelessPacket notification = PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
            nChannel, ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty,
            hashBestChain);
        
        /* Log the notification details BEFORE sending for diagnostics */
        const std::string strOpcodeName = (nChannel == 1) ? 
            "PRIME_BLOCK_AVAILABLE (NEW_PRIME_AVAILABLE)" : 
            "HASH_BLOCK_AVAILABLE (NEW_HASH_AVAILABLE)";
        
        uint32_t nChannelHeight = stateChannel.nChannelHeight;
        
        debug::log(2, "════════════════════════════════════════════════════════════");
        debug::log(2, "📢 SENDING PUSH NOTIFICATION TO MINER");
        debug::log(2, "════════════════════════════════════════════════════════════");
        debug::log(2, "   Opcode:         ", strOpcodeName);
        debug::log(2, "   Opcode Value:   0x", std::hex, static_cast<uint32_t>(notification.HEADER), std::dec, " (", static_cast<uint32_t>(notification.HEADER), ")");
        debug::log(2, "   To Address:     ", GetAddress().ToStringIP());
        debug::log(2, "   Channel:        ", nChannel, " (", GetChannelName(nChannel), ")");
        debug::log(2, "   Payload:");
        debug::log(2, "      Unified Height:  ", stateBest.nHeight);
        debug::log(2, "      Channel Height:  ", nChannelHeight);
        debug::log(2, "      Difficulty (nBits): 0x", std::hex, nDifficulty, std::dec);
        debug::log(2, "      Difficulty (calc):  ", std::fixed, std::setprecision(6), 
                   TAO::Ledger::GetDifficulty(nDifficulty, nChannel));
        debug::log(2, "   Packet Size:    ", notification.LENGTH, " bytes");
        debug::log(0, FUNCTION, "[BLOCK CREATE] hashPrevBlock = ", hashBestChain.SubString(),
                   " (template anchor embedded in push notification, unified height ", stateBest.nHeight + 1, ")");
        debug::log(2, "");
        debug::log(2, "   ⚠️  EXPECTED CLIENT ACTION:");
        debug::log(2, "      Client should respond with GET_BLOCK (129/0x81)");
        debug::log(2, "      to fetch new mining template for this channel.");
        debug::log(2, "");
        debug::log(2, "   🔄 FALLBACK BEHAVIOR:");
        debug::log(2, "      If client times out or doesn't receive this,");
        debug::log(2, "      client may fall back to polling GET_ROUND (133/0x85).");
        debug::log(2, "════════════════════════════════════════════════════════════");

        /* Send to miner */
        respond(notification);

        /* Reset connection activity timer — the miner is alive and receiving push notifications.
         * Without this, the node would disconnect active miners during long Prime searches (~2-5 min)
         * because the push-notification protocol means miners don't send data back until they find a block. */
        this->Reset();

        /* Capture timestamp for accurate timing measurements
         * Using same timestamp both for updating context and for client timing calculations */
        uint64_t nNotificationTimestamp = runtime::unifiedtimestamp();
        
        /* Update statistics (thread-safe) */
        uint32_t nSessionId_snap;
        {
            LOCK(MUTEX);
            context = context.WithNotificationSent(nNotificationTimestamp);
            nSessionId_snap = context.nSessionId;
        }  // MUTEX automatically unlocked here
        
        debug::log(2, FUNCTION, "Sent ", GetChannelName(nChannel), 
                   " notification to ", GetAddress().ToStringIP(),
                   " session=", nSessionId_snap,
                   " (unified=", stateBest.nHeight, 
                   ", channelHeight=", nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");

        /* SIM-LINK: Prune stale session-scoped block templates now that the tip has
         * advanced.  Per-connection templates are pruned via CleanupStaleTemplates();
         * the session-scoped store needs its own prune call.  Non-fatal on session_id=0. */
        if(nSessionId_snap != 0)
        {
            StatelessMinerManager::Get().PruneSessionBlocks(nSessionId_snap);
        }
    }


    /* SendStatelessTemplate - Send complete template using 16-bit opcode 0xD081 = Mirror(129) */
    void StatelessMinerConnection::SendStatelessTemplate()
    {
        /* Protocol constants for stateless template push */
        static const size_t METADATA_SIZE = 12;              // Height (4) + channel height (4) + difficulty (4)
        static const size_t STATELESS_TEMPLATE_SIZE = 228;   // Total: metadata + block template
        
        /* Early exit if shutdown is in progress */
        if (config::fShutdown.load())
        {
            debug::log(2, FUNCTION, "Shutdown in progress; skipping stateless template");
            return;
        }
        
        /* Thread-safe context access */
        uint32_t nChannel;
        {
            LOCK(MUTEX);

            /* Push throttle — drop if a template was sent less than
             * TEMPLATE_PUSH_MIN_INTERVAL_MS ago (guards against fork-resolution bursts).
             * Re-subscription responses bypass via m_force_next_push.
             *
             * IMPORTANT — two-step re-arm when calling SendChannelNotification() then
             * SendStatelessTemplate() in sequence (e.g. MINER_READY and SESSION_STATUS
             * degraded-recovery handlers):
             *   1. Set m_force_next_push = true
             *   2. Call SendChannelNotification()   ← consumes flag, updates m_last_template_push_time
             *   3. Set m_force_next_push = true     ← re-arm: without this, elapsed ~0 ms here → throttled
             *   4. Call SendStatelessTemplate()
             * Omitting step 3 causes the template to be dropped silently, leaving the miner
             * in degraded mode with no work. */
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

            /* Validate channel (1=Prime, 2=Hash only) */
            if (context.nChannel != 1 && context.nChannel != 2)
            {
                debug::error(FUNCTION, "Invalid channel: ", context.nChannel);
                return;
            }
            
            /* Copy channel for use outside lock */
            nChannel = context.nChannel;
        }  // MUTEX automatically unlocked here
        
        debug::log(2, "════════════════════════════════════════════════════════════");
        debug::log(2, "📤 SENDING STATELESS TEMPLATE (0xD081)");
        debug::log(2, "════════════════════════════════════════════════════════════");
        debug::log(2, "   To Address:     ", GetAddress().ToStringIP());
        debug::log(2, "   Channel:        ", nChannel, " (", GetChannelName(nChannel), ")");
        
        /* Get blockchain state */
        TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Get channel-specific state */
        TAO::Ledger::BlockState stateChannel = stateBest;
        if (!TAO::Ledger::GetLastState(stateChannel, nChannel))
        {
            debug::error(FUNCTION, "Failed to get channel state for channel ", nChannel);
            debug::log(2, "════════════════════════════════════════════════════════════");
            return;
        }
        
        /* Get difficulty */
        uint32_t nDifficulty = TAO::Ledger::GetNextTargetRequired(stateBest, nChannel);
        
        /* Build canonical chain state snapshot for template serving (PR #316) */
        CanonicalChainState canonicalSnap = CanonicalChainState::from_chain_state(
            stateBest, stateChannel, nDifficulty);
        
        debug::log(2, FUNCTION, "Canonical snapshot: unified=", canonicalSnap.canonical_unified_height,
                   " channel=", canonicalSnap.canonical_channel_height,
                   " nBits=0x", std::hex, canonicalSnap.canonical_difficulty_nbits, std::dec,
                   " stale=", canonicalSnap.is_canonically_stale() ? "yes" : "no");
        
        /* Create new block template - note: new_block() stores in mapBlocks, so we don't own the pointer */
        TAO::Ledger::Block* pBlock = new_block();
        if (!pBlock)
        {
            debug::log(2, FUNCTION, "new_block() returned nullptr in push path — retrying once");
            pBlock = new_block();
        }
        if (!pBlock)
        {
            debug::error(FUNCTION, "Failed to create block template after retry");
            debug::log(2, "════════════════════════════════════════════════════════════");
            return;
        }
        
        /* Serialize block template (expected: 216 bytes for Tritium) */
        std::vector<uint8_t> vBlockData = pBlock->Serialize();
        if (vBlockData.empty() || vBlockData.size() != TRITIUM_BLOCK_SIZE)
        {
            debug::error(FUNCTION, "Invalid block serialization: ", vBlockData.size(), 
                        " bytes (expected ", TRITIUM_BLOCK_SIZE, ")");
            debug::log(2, "════════════════════════════════════════════════════════════");
            return;
        }
        
        /* Build 16-bit opcode packet (228 bytes total: 12 metadata + 216 template) */
        StatelessPacket notification(StatelessOpcodes::STATELESS_GET_BLOCK);  // 16-bit constructor
        notification.DATA.reserve(STATELESS_TEMPLATE_SIZE);  // Pre-allocate
        
        /* Add 12-byte metadata (big-endian) */
        // Unified height [0-3]
        notification.DATA.push_back((stateBest.nHeight >> 24) & 0xFF);
        notification.DATA.push_back((stateBest.nHeight >> 16) & 0xFF);
        notification.DATA.push_back((stateBest.nHeight >> 8) & 0xFF);
        notification.DATA.push_back((stateBest.nHeight >> 0) & 0xFF);
        
        // Channel height [4-7]
        uint32_t nChannelHeight = stateChannel.nChannelHeight;
        notification.DATA.push_back((nChannelHeight >> 24) & 0xFF);
        notification.DATA.push_back((nChannelHeight >> 16) & 0xFF);
        notification.DATA.push_back((nChannelHeight >> 8) & 0xFF);
        notification.DATA.push_back((nChannelHeight >> 0) & 0xFF);
        
        // Difficulty [8-11]
        notification.DATA.push_back((nDifficulty >> 24) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 16) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 8) & 0xFF);
        notification.DATA.push_back((nDifficulty >> 0) & 0xFF);
        
        /* Add 216-byte block template [12-227] */
        notification.DATA.insert(notification.DATA.end(), vBlockData.begin(), vBlockData.end());
        
        /* Set LENGTH field to match DATA size before serialization.
         * CRITICAL: Unlike error responses (which always have 1 byte of data),
         * this notification carries variable-size payload (228 bytes = 12 metadata + 216 template).
         * Must use DATA.size() instead of hardcoded value to ensure correct framing. */
        notification.LENGTH = static_cast<uint32_t>(notification.DATA.size());
        
        debug::log(2, "   Payload:");
        debug::log(2, "      Unified Height:  ", stateBest.nHeight);
        debug::log(2, "      Channel Height:  ", nChannelHeight);
        debug::log(2, "      Difficulty (nBits): 0x", std::hex, nDifficulty, std::dec);
        debug::log(2, "      Difficulty (calc):  ", std::fixed, std::setprecision(6), 
                   TAO::Ledger::GetDifficulty(nDifficulty, nChannel));
        debug::log(2, "      Block Hash:      ", pBlock->GetHash().SubString());
        debug::log(2, "      Merkle Root:     ", pBlock->hashMerkleRoot.SubString());
        debug::log(0, FUNCTION, "[BLOCK CREATE] hashPrevBlock = ", pBlock->hashPrevBlock.SubString(),
                   " (template anchor baked in, unified height ", pBlock->nHeight, ")");
        debug::log(2, FUNCTION, "[BLOCK CREATE] hashPrevBlock FULL (MSB-first): ", pBlock->hashPrevBlock.GetHex());
        debug::log(2, "   Total Size:     ", notification.DATA.size(), " bytes (", 
                   METADATA_SIZE, " meta + ", TRITIUM_BLOCK_SIZE, " template)");
        debug::log(2, "");
        debug::log(2, "   ⚡ STATELESS PROTOCOL:");
        debug::log(2, "      Miner can begin hashing immediately (no GET_BLOCK needed)");
        debug::log(2, "      Template pushed automatically on new blocks");
        debug::log(2, "════════════════════════════════════════════════════════════");
        
        /* Send to miner */
        respond(notification);
        
        /* Update statistics and store canonical snapshot in context. */
        uint64_t nNotificationTimestamp = runtime::unifiedtimestamp();
        uint32_t nSessionId_snap;
        {
            LOCK(MUTEX);
            context = context.WithNotificationSent(nNotificationTimestamp)
                             .WithCanonicalSnap(canonicalSnap);
            nSessionId_snap = context.nSessionId;
        }
        
        debug::log(2, FUNCTION, "Sent stateless template (0xD081) to ", GetAddress().ToStringIP(),
                   " session=", nSessionId_snap,
                   " (unified=", stateBest.nHeight, 
                   ", channel=", nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");
    }


    /* SendNodeShutdown - Notify miner of graceful node shutdown via NODE_SHUTDOWN (0xD0FF) */
    void StatelessMinerConnection::SendNodeShutdown(uint32_t nReasonCode)
    {
        if(!m_nodeShutdownNotification.MarkSent())
        {
            debug::log(1, FUNCTION, "NODE_SHUTDOWN already sent to ", GetAddress().ToStringIP(),
                       " - skipping duplicate");
            return;
        }

        /* Build NODE_SHUTDOWN packet: 4-byte reason code, big-endian */
        StatelessPacket packet(OpcodeUtility::Stateless::NODE_SHUTDOWN);
        packet.DATA.push_back(static_cast<uint8_t>((nReasonCode >> 24) & 0xFF));
        packet.DATA.push_back(static_cast<uint8_t>((nReasonCode >> 16) & 0xFF));
        packet.DATA.push_back(static_cast<uint8_t>((nReasonCode >>  8) & 0xFF));
        packet.DATA.push_back(static_cast<uint8_t>((nReasonCode >>  0) & 0xFF));
        packet.LENGTH = 4;

        debug::log(1, FUNCTION, "Sending NODE_SHUTDOWN (0xD0FF) to ", GetAddress().ToStringIP(),
                   " reason=", nReasonCode);
        WritePacket(packet);
        debug::log(1, FUNCTION, "Queued NODE_SHUTDOWN for ", GetAddress().ToStringIP(),
                   " buffered=", Buffered(), " bytes");
    }

} // namespace LLP
