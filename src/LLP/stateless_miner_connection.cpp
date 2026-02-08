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
#include <LLP/include/pool_discovery.h>
#include <LLP/include/opcode_utility.h>
#include <LLP/include/push_notification.h>
#include <LLP/include/mining_constants.h>
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

#include <chrono>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <thread>

namespace LLP
{
    /* Import opcode constants for stateless mining protocol */
    using LLP::OpcodeUtility::Opcodes::MINER_AUTH_INIT;
    using LLP::OpcodeUtility::Opcodes::MINER_AUTH_CHALLENGE;
    using LLP::OpcodeUtility::Opcodes::MINER_AUTH_RESPONSE;
    using LLP::OpcodeUtility::Opcodes::MINER_AUTH_RESULT;
    using LLP::OpcodeUtility::Opcodes::SESSION_START;
    using LLP::OpcodeUtility::Opcodes::SESSION_KEEPALIVE;

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
        std::string physicalStatus = context.fPhysicalFalconPresent ? "PRESENT" : "ABSENT";
        
        debug::log(0, FUNCTION, "   [Disposable: ", disposableVersion, 
                  ", Physical: ", physicalStatus, "]");
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


    /** Helper function to calculate offset after disposable signature in SUBMIT_BLOCK packet.
     *  This improves code readability and makes the offset calculation more maintainable.
     *  
     *  Packet format: [block][timestamp][sig_len][disposable_sig][physiglen?][physical_sig?]
     *  
     *  @param blockSize Size of the block data in bytes
     *  @param timestampSize Size of timestamp field (typically 8 bytes)
     *  @param lengthFieldSize Size of signature length field (typically 2 bytes)
     *  @param sigLen Length of the disposable signature
     *  @return Offset in bytes where Physical Falcon signature length field would start
     */
    inline static size_t GetDisposableSignatureEndOffset(size_t blockSize, size_t timestampSize, 
                                                          size_t lengthFieldSize, size_t sigLen)
    {
        return blockSize + timestampSize + lengthFieldSize + sigLen;
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

                /* Register with StatelessMinerManager for tracking */
                StatelessMinerManager::Get().UpdateMiner(strAddr, context, 1);

                return;
            }

            /* On Disconnect Event */
            case EVENTS::DISCONNECT:
            {
                /* Debug output. */
                uint32_t reason = LENGTH;
                std::string strReason;

                switch(reason)
                {
                    case DISCONNECT::TIMEOUT:
                        strReason = "DISCONNECT::TIMEOUT";
                        break;
                    case DISCONNECT::ERRORS:
                        strReason = "DISCONNECT::ERRORS";
                        break;
                    case DISCONNECT::DDOS:
                        strReason = "DISCONNECT::DDOS";
                        break;
                    case DISCONNECT::FORCE:
                        strReason = "DISCONNECT::FORCE";
                        break;
                    default:
                        strReason = "UNKNOWN";
                        break;
                }

                debug::log(0, FUNCTION, "MinerLLP: Disconnected from ", GetAddress().ToStringIP(),
                           " reason: ", strReason);

                /* Notify local pool if enabled */
                if(PoolDiscovery::IsLocalPoolEnabled() && context.fAuthenticated && context.hashGenesis != 0)
                {
                    PoolDiscovery::OnMinerDisconnected(context.hashGenesis);
                }

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
            /* Get the incoming packet. */
            StatelessPacket PACKET = this->INCOMING;
            /* Log entry */
            debug::log(1, FUNCTION, "MinerLLP: ProcessPacket from ", GetAddress().ToStringIP(),
                       " header=0x", std::hex, std::setw(4), std::setfill('0'),
                       uint32_t(PACKET.HEADER), std::dec,
                       " length=", PACKET.LENGTH);

            if(!StatelessOpcodes::IsStateless(PACKET.HEADER))
            {
                uint16_t nHeaderSwapped = static_cast<uint16_t>((PACKET.HEADER >> 8) | (PACKET.HEADER << 8));
                if(StatelessOpcodes::IsStateless(nHeaderSwapped))
                {
                    debug::error(FUNCTION, "Header endian mismatch detected: header=0x", std::hex, uint32_t(PACKET.HEADER),
                                 " swapped=0x", uint32_t(nHeaderSwapped), std::dec);
                    PACKET.HEADER = nHeaderSwapped;
                }
            }

            /* Validate opcode range - reject opcodes outside 0xD000-0xD0FF */
            const uint16_t originalHeader = PACKET.HEADER;
            if(!StatelessOpcodes::IsStateless(originalHeader))
            {
                const uint16_t swappedHeader =
                    static_cast<uint16_t>((originalHeader >> 8) | (originalHeader << 8));
                if(StatelessOpcodes::IsStateless(swappedHeader))
                {
                    PACKET.HEADER = swappedHeader;
                }
                else
                {
                    debug::error(FUNCTION, "Invalid stateless opcode: 0x", std::hex, uint32_t(originalHeader), std::dec);
                    debug::error(FUNCTION, "  Stateless opcodes must be in range 0xD000-0xD0FF");
                    debug::error(FUNCTION, "  Rejecting packet from ", GetAddress().ToStringIP());
                    return false;
                }
            }

            if(!StatelessOpcodes::IsStateless(PACKET.HEADER))
            {
                debug::error(FUNCTION, "Invalid stateless opcode: 0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                debug::error(FUNCTION, "  Stateless opcodes must be in range 0xD000-0xD0FF");
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

                /* Attempt lane-switch recovery based on address */
                auto optExisting = SessionRecoveryManager::Get().RecoverSessionByAddress(GetAddress().ToStringIP());
                if(optExisting.has_value())
                {
                    uint256_t hashRecoveredKey(0);
                    uint64_t nRecoveredNonce = 0;
                    if(SessionRecoveryManager::Get().RestoreChaCha20State(optExisting->hashKeyID, hashRecoveredKey, nRecoveredNonce)
                       && hashRecoveredKey != 0)
                    {
                        context = context.WithChaChaKey(hashRecoveredKey.GetBytes());
                    }

                    std::vector<uint8_t> vRecoveredPubKey;
                    uint256_t hashDisposableKeyID(0);
                    if(SessionRecoveryManager::Get().RestoreDisposableKey(optExisting->hashKeyID, vRecoveredPubKey, hashDisposableKeyID)
                       && !vRecoveredPubKey.empty())
                    {
                        std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                        if(optExisting->nSessionId != 0)
                            mapSessionKeys[optExisting->nSessionId] = vRecoveredPubKey;
                    }

                    debug::log(0, FUNCTION, "Session recovered from lane switch");
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
                    debug::log(0, "   Genesis: ", context.hashGenesis.SubString());
                    debug::log(0, "   Encryption ready: YES");
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
                          (context.nChannel == 1 ? "Prime" : "Hash"), " notifications (stateless protocol)");
                debug::log(0, "   Updated StatelessMinerManager with complete context");
                debug::log(0, "   Encryption ready: ", (context.fEncryptionReady ? "YES" : "NO"));
                debug::log(0, "   ChaCha key size: ", context.vChaChaKey.size(), " bytes");
                
                /* Send immediate template push using STATELESS_GET_BLOCK (0xD081 = Mirror(129)) */
                SendStatelessTemplate();

                debug::log(2, "📥 === STATELESS_MINER_READY: SUCCESS ===");
                return true;
            }

            /* ============================================================================
             * 16-BIT STATELESS OPCODE HANDLERS
             * ============================================================================ */

            /* All opcodes reference OpcodeUtility::Stateless (via StatelessOpcodes alias) */
            using namespace StatelessOpcodes;
            
            /* Block rejection reason codes (PR #122: Falcon Protocol Integration) */
            const uint8_t REJECT_PHYSICAL_SIGNATURE_FAILED = 0x10;  // Physical Falcon signature verification failed
            const uint8_t REJECT_KEY_BONDING_VIOLATION = 0x11;      // Key bonding violation (version mismatch)
            
            /* Tritium block serialization constants */
            constexpr uint32_t TRITIUM_BLOCK_SIZE = 216;        // Total size of serialized Tritium block template
            constexpr uint32_t TRITIUM_OFFSET_NCHANNEL = 196;   // Offset of nChannel field in serialized block
            constexpr uint32_t TRITIUM_OFFSET_NHEIGHT = 200;    // Offset of nHeight field in serialized block

            LOCK(MUTEX);

            /* Handle GET_BLOCK - requires authentication and channel */
            if(PACKET.HEADER == GET_BLOCK)
            {
                // AUTOMATED RATE LIMIT CHECK
                if (!CheckRateLimit(GET_BLOCK)) {
                    // Request rejected - violation already recorded
                    // Send empty response to indicate rate limited
                    debug::log(1, FUNCTION, "GET_BLOCK rate limited for ", GetAddress().ToStringIP());
                    StatelessPacket response(BLOCK_DATA);
                    response.LENGTH = 0;
                    respond(response);
                    return true;  // Handled (rejected)
                }
                
                debug::log(2, "📥 === GET_BLOCK REQUEST ===");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Authenticated: ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(0, "   Channel: ", context.nChannel);
                debug::log(0, "   Session ID: ", context.nSessionId);
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error("   ❌ Authentication required");
                    StatelessPacket response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    response.LENGTH = 1;
                    respond(response);
                    debug::log(2, "📥 === GET_BLOCK: REJECTED (AUTH) ===");
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error("   ❌ Channel not set");
                    debug::log(2, "📥 === GET_BLOCK: REJECTED (NO CHANNEL) ===");
                    return true;
                }
                
                debug::log(0, "   ✅ Validation passed");
                debug::log(0, "   Calling new_block()...");
                
                /* Create a new block */
                TAO::Ledger::Block* pBlock = new_block();
                
                /* Handle if the block failed to be created. */
                if(!pBlock)
                {
                    debug::error("   ❌ new_block() returned nullptr");
                    StatelessPacket response(BLOCK_DATA);
                    response.LENGTH = 0;
                    respond(response);
                    debug::log(2, "📥 === GET_BLOCK: FAILED (NO BLOCK) ===");
                    return true;
                }
                
                debug::log(0, "   ✅ Block created successfully");
                debug::log(0, "      Height: ", pBlock->nHeight);
                debug::log(0, "      Channel: ", pBlock->nChannel);
                debug::log(0, "      Merkle root: ", pBlock->hashMerkleRoot.SubString());
                
                /* Note: Block is already stored in mapBlocks by new_block() */
                
                try {
                    debug::log(0, "   Serializing block...");
                    
                    /* Before serialization */
                    debug::log(0, "   Block fields before serialization:");
                    debug::log(0, "      nChannel: ", pBlock->nChannel);
                    debug::log(0, "      nHeight: ", pBlock->nHeight);
                    debug::log(0, "      Merkle: ", pBlock->hashMerkleRoot.SubString());
                    
                    /* Use block's Serialize() method - returns 216-byte mining template */
                    std::vector<uint8_t> vData = pBlock->Serialize();
                    
                    if(vData.empty())
                    {
                        debug::error("   ❌ Serialization returned empty vector!");
                        StatelessPacket response(BLOCK_DATA);
                        response.LENGTH = 0;
                        respond(response);
                        debug::log(2, "📥 === GET_BLOCK: FAILED (EMPTY SERIALIZATION) ===");
                        return true;
                    }
                    
                    debug::log(0, "   ✅ Serialized! Size: ", vData.size(), " bytes");
                    
                    /* ARCHITECTURAL NOTE: nChannelHeight vs nChannel vs nHeight
                     * =========================================================
                     * 
                     * The 216-byte Tritium block template contains:
                     *   - nChannel (offset 196): Which channel this block is for (1=Prime, 2=Hash)
                     *   - nHeight (offset 200):  Unified blockchain height (all channels combined)
                     * 
                     * The TemplateMetadata structure ADDITIONALLY stores:
                     *   - nChannelHeight: Channel-specific height (NOT in serialized template)
                     * 
                     * Why nChannelHeight is not in the template:
                     *   - Block (216 bytes) = Minimal serializable data for mining
                     *   - BlockState (extended) = Block + chain state (calculated during Accept())
                     *   - nChannelHeight is calculated by Block::Accept() using GetLastState()
                     *   - TemplateMetadata stores it for staleness detection before Accept()
                     * 
                     * Flow:
                     *   1. Node creates Block with nChannel=1, nHeight=6536668
                     *   2. Node calculates nChannelHeight=2302369 for metadata
                     *   3. Node serializes Block (216 bytes, includes nChannel at offset 196)
                     *   4. Node stores TemplateMetadata with nChannelHeight=2302369
                     *   5. Miner receives 216-byte template, deserializes nChannel from offset 196
                     *   6. When miner submits, Block::Accept() recalculates nChannelHeight
                     */
                    
                    /* Verify both nChannel and nHeight in serialized template */
                    if(vData.size() >= TRITIUM_BLOCK_SIZE)
                    {
                        /* Tritium block serialization format (216 bytes):
                         *   [0-3]     nVersion (4 bytes)
                         *   [4-131]   hashPrevBlock (128 bytes)
                         *   [132-195] hashMerkleRoot (64 bytes)
                         *   [196-199] nChannel (4 bytes)      ← CORRECT offset
                         *   [200-203] nHeight (4 bytes)
                         *   [204-207] nBits (4 bytes)
                         *   [208-215] nNonce (8 bytes)
                         * Total: 216 bytes
                         */
                        
                        /* Extract nChannel at offset 196 */
                        uint32_t nChannelFromSerialized =
                            convert::bytes2uint(vData, TRITIUM_OFFSET_NCHANNEL);
                        
                        /* Extract nHeight at offset 200 */
                        uint32_t nHeightFromSerialized =
                            convert::bytes2uint(vData, TRITIUM_OFFSET_NHEIGHT);
                        
                        debug::log(0, "   Serialization verification:");
                        debug::log(0, "      Template fields:");
                        debug::log(0, "         nChannel: ", pBlock->nChannel, " → serialized: ", nChannelFromSerialized);
                        debug::log(0, "         nHeight:  ", pBlock->nHeight, " → serialized: ", nHeightFromSerialized);
                        debug::log(0, "      Metadata fields (not serialized in template):");
                        debug::log(0, "         nChannelHeight: (stored in TemplateMetadata for staleness detection)");
                        
                        /* Validate nChannel */
                        if(nChannelFromSerialized != pBlock->nChannel)
                        {
                            debug::error(FUNCTION, "❌ nChannel mismatch after serialization!");
                            debug::error(FUNCTION, "   Expected: ", pBlock->nChannel);
                            debug::error(FUNCTION, "   Got: ", nChannelFromSerialized);
                            
                            StatelessPacket response(BLOCK_DATA);
                            response.LENGTH = 0;
                            respond(response);
                            debug::log(2, "📥 === GET_BLOCK: FAILED (CHANNEL MISMATCH) ===");
                            return true;
                        }
                        
                        /* Validate nHeight */
                        if(nHeightFromSerialized != pBlock->nHeight)
                        {
                            debug::error(FUNCTION, "❌ nHeight mismatch after serialization!");
                            debug::error(FUNCTION, "   Expected: ", pBlock->nHeight);
                            debug::error(FUNCTION, "   Got: ", nHeightFromSerialized);
                            
                            StatelessPacket response(BLOCK_DATA);
                            response.LENGTH = 0;
                            respond(response);
                            debug::log(2, "📥 === GET_BLOCK: FAILED (HEIGHT MISMATCH) ===");
                            return true;
                        }
                        
                        debug::log(0, "   ✅ Serialization verified: nChannel and nHeight preserved correctly");
                    }
                    
                    /* Create response packet */
                    StatelessPacket response(BLOCK_DATA);
                    response.DATA = vData;
                    response.LENGTH = static_cast<uint32_t>(vData.size());  // ⭐ CRITICAL FIX!
                    
                    debug::log(0, "   📤 Sending BLOCK_DATA...");
                    debug::log(0, "      Packet header: ", (uint32_t)response.HEADER);
                    debug::log(0, "      Packet LENGTH field: ", response.LENGTH);
                    debug::log(0, "      Packet DATA size: ", response.DATA.size());
                    
                    /* Send the response */
                    respond(response);
                    
                    debug::log(0, "   ✅ Packet sent!");
                    debug::log(2, "📥 === GET_BLOCK: SUCCESS ===");
                    
                    /* Notify local pool of authenticated miner (if not already notified) */
                    if(PoolDiscovery::IsLocalPoolEnabled() && context.fAuthenticated && context.hashGenesis != 0)
                    {
                        /* Detect Falcon version from context if available */
                        bool fUsesFalcon1024 = false;
                        if(context.fFalconVersionDetected)
                        {
                            fUsesFalcon1024 = (context.nFalconVersion == LLC::FalconVersion::FALCON_1024);
                        }
                        
                        /* Notify pool (pool tracks unique miners internally) */
                        PoolDiscovery::OnMinerAuthenticated(
                            context.hashGenesis,
                            GetAddress().ToStringIP(),
                            context.nChannel,
                            fUsesFalcon1024
                        );
                    }
                    
                    /* Update context timestamp and height */
                    context = context.WithTimestamp(runtime::unifiedtimestamp())
                                     .WithHeight(pBlock->nHeight);
                    
                    /* Update manager with new context after template served */
                    StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                    StatelessMinerManager::Get().IncrementTemplatesServed();
                    
                    return true;
                }
                catch(const std::exception& e) {
                    debug::error("   ❌ Serialization exception: ", e.what());
                    debug::log(2, "📥 === GET_BLOCK: EXCEPTION ===");
                    
                    StatelessPacket response(BLOCK_DATA);
                    response.LENGTH = 0;
                    respond(response);
                    
                    return true;
                }
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
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    return true;  // Handled (rejected)
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK received ===", ANSI_COLOR_RESET);
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "❌ Authentication required");
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (AUTH) ===", ANSI_COLOR_RESET);
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "❌ Channel not set");
                    StatelessPacket response(BLOCK_REJECTED);
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
                    
                    StatelessPacket response(BLOCK_REJECTED);
                    response.DATA.push_back(0x0C);  // Reason: Encryption required
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 encryption required) ===", ANSI_COLOR_RESET);
                    return true;
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
                           " rewardAddress=", context.hashRewardAddress.ToString().substr(0, 16), "...");

                /* Validate packet size using FalconConstants */
                /* Minimum: merkle(64) + nonce(8) = 72 bytes (legacy format) */
                const size_t MIN_SIZE = FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE;
                
                /* Maximum: full block dual-signature with ChaCha20 encryption = 1,878 bytes (Legacy - largest) */
                const size_t MAX_SIZE = FalconConstants::SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX;

                if(PACKET.DATA.size() < MIN_SIZE)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK packet too small: ", 
                               PACKET.DATA.size(), " < ", MIN_SIZE);
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                if(PACKET.DATA.size() > MAX_SIZE)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK packet too large: ",
                               PACKET.DATA.size(), " > ", MAX_SIZE);
                    StatelessPacket response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                uint512_t hashMerkle;
                uint64_t nonce = 0;
                bool fFalconVerified = false;
                std::unique_ptr<LLP::DisposableFalcon::IDisposableFalconWrapper> pFalconWrapper;

                /* Check for Falcon-signed format: [merkle][nonce][timestamp][sig_len][signature] */
                /* Minimum for Falcon format: 64 + 8 + 8 + 2 = 82 bytes */
                const size_t FALCON_MIN_SIZE = FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN;

                if(PACKET.DATA.size() >= FALCON_MIN_SIZE)
                {
                    /* Create disposable Falcon wrapper for this submission */
                    pFalconWrapper = LLP::DisposableFalcon::Create();
                    if(!pFalconWrapper)
                    {
                        debug::error(FUNCTION, "❌ CRITICAL: Falcon wrapper not initialized");
                        debug::error(FUNCTION, "   Cannot verify Falcon signatures - check constructor logs");
                        debug::error(FUNCTION, "   Packet size suggests Falcon format but wrapper unavailable");
                        debug::error(FUNCTION, "   This should not happen in production - investigate immediately");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "   ⚠️  SECURITY WARNING: rejecting Falcon packet (wrapper unavailable)");
                        debug::error(FUNCTION, "   ⚠️  In production, reject Falcon packets until wrapper is fixed");
                        debug::error(FUNCTION, "   ⚠️  Consider blocking submissions until wrapper is fixed");

                        StatelessPacket response(BLOCK_REJECTED);
                        response.DATA.push_back(0xFF);  // Reason: Internal error
                        response.LENGTH = 1;
                        respond(response);

                        pFalconWrapper.reset();
                        return true;
                    }
                    else
                    {
                        debug::log(2, FUNCTION, "✓ Disposable Falcon wrapper created for SUBMIT_BLOCK");
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
                        debug::log(0, "   Size: ", vSessionPubKey.size(), " bytes (expected: 897)");
                        if(!vSessionPubKey.empty() && vSessionPubKey.size() >= 16)
                        {
                            debug::log(0, "   First 16 bytes (hex): ");
                            debug::log(0, "      ", FormatHexDump(vSessionPubKey, 16));
                        }
                        
                        if(vSessionPubKey.empty())
                        {
                            debug::error(FUNCTION, "❌ No Falcon pubkey stored for session");
                            debug::error(FUNCTION, "   Session may have expired or never authenticated properly");
                            
                            StatelessPacket response(BLOCK_REJECTED);
                            response.DATA.push_back(0x0D);  // Reason: No session key
                            response.LENGTH = 1;
                            respond(response);
                            
                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (No Falcon session key) ===", ANSI_COLOR_RESET);
                            pFalconWrapper.reset();
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
                                    debug::error(FUNCTION, "❌ ChaCha20 decryption FAILED");
                                    debug::error(FUNCTION, "   Possible causes:");
                                    debug::error(FUNCTION, "   - Corrupted ciphertext during transmission");
                                    debug::error(FUNCTION, "   - Wrong decryption key (session key mismatch)");
                                    debug::error(FUNCTION, "   - Authentication tag verification failed");
                                    
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0B);  // Reason: ChaCha20 decryption failure
                                    response.LENGTH = 1;
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 decryption failed) ===", ANSI_COLOR_RESET);
                                    pFalconWrapper.reset();
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
                                
                                /* STEP 2: Extract from DECRYPTED data */
                                debug::log(0, "📊 DATA EXTRACTION:");
                                
                                /* Extract merkle root (64 bytes at offset 132) */
                                uint512_t hashMerkleFromBlock;
                                hashMerkleFromBlock.SetBytes(std::vector<uint8_t>(
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE
                                ));
                                
                                debug::log(0, "   Merkle root:");
                                debug::log(0, "      Offset: ", FalconConstants::FULL_BLOCK_MERKLE_OFFSET);
                                debug::log(0, "      Value: ", hashMerkleFromBlock.SubString());
                                
                                /* Extract nonce with BOTH endianness for debugging */
                                debug::log(0, "   Nonce extraction:");
                                debug::log(0, "      Offset: ", FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET);
                                
                                /* Show raw bytes */
                                std::vector<uint8_t> nonceBytes(
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET,
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE
                                );
                                debug::log(0, "      Raw bytes [200-207]: ", FormatHexDump(nonceBytes));
                                
                                /* Extract as BIG-endian (what convert::bytes2uint64 does) */
                                uint64_t nonce_be = convert::bytes2uint64(std::vector<uint8_t>(
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET,
                                    decryptedData.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE
                                ));
                                
                                /* Extract as LITTLE-endian (Falcon protocol standard) */
                                uint64_t nonce_le = bytes_to_uint64_le(decryptedData, FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET);
                                
                                debug::log(0, "      BIG-endian interpretation:    0x", std::hex, nonce_be, std::dec);
                                debug::log(0, "      LITTLE-endian interpretation: 0x", std::hex, nonce_le, std::dec);
                                debug::log(0, "      Using: LITTLE-ENDIAN (Falcon protocol standard)");
                                
                                /* Use little-endian value (correct for Falcon protocol) */
                                uint64_t nonceFromBlock = nonce_le;
                                
                                /* Reconstruct signed data to match what miner signed: [full_block][timestamp] */
                                /* Miner signs: [block][timestamp] */
                                /* This is simpler, more secure (authenticates entire block), and matches miner behavior */
                                
                                /* Format: [block(variable)][timestamp(8)][sig_len(2)][signature(variable)][physiglen(2)][physical_sig(optional)] */
                                /* Read sig_len to determine where signature starts */
                                /* Minimum size: timestamp(8) + sig_len(2) + physiglen(2) = 12 bytes */
                                const size_t MIN_METADATA_SIZE = FalconConstants::TIMESTAMP_SIZE + 
                                                                 FalconConstants::LENGTH_FIELD_SIZE +  // sig_len field
                                                                 FalconConstants::LENGTH_FIELD_SIZE;   // physiglen field
                                
                                if(decryptedData.size() < MIN_METADATA_SIZE) {
                                    debug::error(FUNCTION, "❌ Decrypted payload too small (need at least ", MIN_METADATA_SIZE, " bytes)");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    respond(response);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                                
                                /* Determine block size dynamically */
                                /* Format: [block(B)][timestamp(8)][sig_len(2)][signature(S)][physiglen(2)][physical_sig(P)] */
                                /* Strategy: Work backwards from the end to find sig_len and validate packet structure */
                                /* We know: totalSize = B + 8 + 2 + S + 2 + P */
                                /* Therefore: B = totalSize - 8 - 2 - S - 2 - P */
                                
                                /* Read sig_len from different potential positions and validate */
                                /* Most blocks are 216 or 220 bytes, but can be up to 2MB */
                                size_t blockSize = 0;
                                uint16_t sigLen = 0;
                                bool fFoundValidSize = false;
                                
                                /* Try common block sizes first for efficiency: 216 (Tritium) and 220 (Legacy) */
                                const size_t commonBlockSizes[] = {
                                    FalconConstants::FULL_BLOCK_TRITIUM_MIN,
                                    FalconConstants::FULL_BLOCK_LEGACY_MIN
                                };
                                
                                for(size_t testBlockSize : commonBlockSizes)
                                {
                                    /* Check minimum size including physiglen field */
                                    if(decryptedData.size() < testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                              FalconConstants::LENGTH_FIELD_SIZE + 
                                                              FalconConstants::FALCON_SIG_MIN +
                                                              FalconConstants::LENGTH_FIELD_SIZE)  // Physical Falcon length field
                                        continue;  // Too small for this block size
                                    
                                    /* Read sig_len at this position */
                                    uint16_t testSigLen = static_cast<uint16_t>(decryptedData[testBlockSize + FalconConstants::TIMESTAMP_SIZE]) |
                                                          (static_cast<uint16_t>(decryptedData[testBlockSize + FalconConstants::TIMESTAMP_SIZE + 1]) << 8);
                                    
                                    /* Check if this gives a valid signature length and total size 
                                     * Format: [block][timestamp(8)][siglen(2)][sig(N)][physiglen(2)][physical_sig(M)]
                                     * Must account for Physical Falcon length field (2 bytes) which is always present */
                                    size_t expectedSizeWithPhysigLen = testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                                       FalconConstants::LENGTH_FIELD_SIZE + testSigLen + 
                                                                       FalconConstants::LENGTH_FIELD_SIZE;  // Physical Falcon length field
                                    
                                    if(testSigLen >= FalconConstants::FALCON_SIG_MIN && 
                                       testSigLen <= FalconConstants::FALCON_SIG_MAX_VALIDATION &&
                                       decryptedData.size() >= expectedSizeWithPhysigLen)  // >= to allow optional physical sig
                                    {
                                        blockSize = testBlockSize;
                                        sigLen = testSigLen;
                                        fFoundValidSize = true;
                                        break;
                                    }
                                }
                                
                                /* If common sizes didn't work, try computing from signature size */
                                /* This handles blocks with transactions (larger than 220 bytes) */
                                if(!fFoundValidSize && decryptedData.size() >= MIN_METADATA_SIZE + FalconConstants::FALCON_SIG_MIN)
                                {
                                    /* Try reading sig_len from various positions, working backwards from end
                                     * Format: [block(B)][timestamp(8)][siglen(2)][sig(S)][physiglen(2)][physical_sig(P)]
                                     * Total = B + 8 + 2 + S + 2 + P
                                     * We try common values for S and check if the packet structure makes sense
                                     * Include both Falcon-512 and Falcon-1024 signature sizes */
                                    const uint16_t commonSigSizes[] = {
                                        // Falcon-1024 sizes (try first, as they're larger and less ambiguous)
                                        FalconConstants::FALCON1024_SIG_COMMON_SIZE_1,
                                        FalconConstants::FALCON1024_SIG_COMMON_SIZE_2,
                                        FalconConstants::FALCON1024_SIG_COMMON_SIZE_3,
                                        FalconConstants::FALCON1024_SIG_COMMON_SIZE_4,
                                        FalconConstants::FALCON1024_SIG_COMMON_SIZE_5,
                                        // Falcon-512 sizes
                                        FalconConstants::FALCON512_SIG_COMMON_SIZE_1,
                                        FalconConstants::FALCON512_SIG_COMMON_SIZE_2,
                                        FalconConstants::FALCON512_SIG_COMMON_SIZE_3,
                                        FalconConstants::FALCON512_SIG_COMMON_SIZE_4,
                                        FalconConstants::FALCON512_SIG_COMMON_SIZE_5
                                    };
                                    
                                    for(uint16_t testSigLen : commonSigSizes)
                                    {
                                        /* Minimum size check: block + timestamp + siglen + sig + physiglen */
                                        if(decryptedData.size() < FalconConstants::TIMESTAMP_SIZE + 
                                                                  FalconConstants::LENGTH_FIELD_SIZE + testSigLen +
                                                                  FalconConstants::LENGTH_FIELD_SIZE)  // Physical Falcon length field
                                            continue;
                                        
                                        /* Calculate block size assuming no physical signature (P = 0)
                                         * Format: [block(B)][timestamp(8)][siglen(2)][sig(S)][physiglen(2)][physical_sig(0)]
                                         * testBlockSize = Total - timestamp - siglen_field - sig - physiglen_field */
                                        size_t testBlockSize = decryptedData.size() - FalconConstants::TIMESTAMP_SIZE - 
                                                               FalconConstants::LENGTH_FIELD_SIZE -  // disposable sig_len field
                                                               testSigLen -
                                                               FalconConstants::LENGTH_FIELD_SIZE;   // physical sig_len field
                                        
                                        /* Read physical signature length to check if physical signature is present */
                                        size_t physSigLenOffset = testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                                 FalconConstants::LENGTH_FIELD_SIZE +  // disposable sig_len field
                                                                 testSigLen;
                                        
                                        if(physSigLenOffset + FalconConstants::LENGTH_FIELD_SIZE > decryptedData.size())
                                            continue;  // Not enough data for physiglen field
                                        
                                        uint16_t nPhysicalSigLen = static_cast<uint16_t>(decryptedData[physSigLenOffset]) |
                                                                   (static_cast<uint16_t>(decryptedData[physSigLenOffset + 1]) << 8);
                                        
                                        /* If physical signature is present, adjust the expected total size */
                                        size_t expectedTotalSize = testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                                  FalconConstants::LENGTH_FIELD_SIZE + testSigLen +
                                                                  FalconConstants::LENGTH_FIELD_SIZE + nPhysicalSigLen;
                                        
                                        /* Check if total size matches (accounting for optional physical signature) */
                                        if(decryptedData.size() != expectedTotalSize)
                                            continue;  // Size mismatch
                                        
                                        /* Read actual sig_len at position [block + timestamp] to validate our assumption */
                                        uint16_t actualSigLen = static_cast<uint16_t>(decryptedData[testBlockSize + FalconConstants::TIMESTAMP_SIZE]) |
                                                                (static_cast<uint16_t>(decryptedData[testBlockSize + FalconConstants::TIMESTAMP_SIZE + 1]) << 8);
                                        
                                        /* Check if the actual sig_len matches our test */
                                        if(actualSigLen == testSigLen &&
                                           actualSigLen >= FalconConstants::FALCON_SIG_MIN &&
                                           actualSigLen <= FalconConstants::FALCON_SIG_MAX_VALIDATION)
                                        {
                                            blockSize = testBlockSize;
                                            sigLen = actualSigLen;
                                            fFoundValidSize = true;
                                            break;
                                        }
                                    }
                                }
                                
                                if(!fFoundValidSize)
                                {
                                    debug::error(FUNCTION, "❌ Could not determine block size from packet structure");
                                    debug::error(FUNCTION, "   Decrypted size: ", decryptedData.size(), " bytes");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    respond(response);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                                
                                debug::log(0, "📝 SIGNATURE VERIFICATION:");
                                debug::log(0, "   Block size: ", blockSize, " bytes");
                                debug::log(0, "   Signature length: ", sigLen, " bytes");
                                debug::log(0, "   Total packet size: ", decryptedData.size(), " bytes");
                                debug::log(0, "   Format: [block][timestamp(", FalconConstants::TIMESTAMP_SIZE, 
                                          ")][sig_len(", FalconConstants::LENGTH_FIELD_SIZE, ")][signature][physiglen(2)][physical_sig(optional)]");
                                
                                /* Verify we have enough data (should always pass given size detection above)
                                 * Format: [block][timestamp(8)][siglen(2)][sig(N)][physiglen(2)][physical_sig(M)]
                                 * Minimum size must include Physical Falcon length field */
                                const size_t expectedMinSize = blockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                               FalconConstants::LENGTH_FIELD_SIZE + sigLen +
                                                               FalconConstants::LENGTH_FIELD_SIZE;  // Physical Falcon length field
                                if(decryptedData.size() < expectedMinSize)
                                {
                                    debug::error(FUNCTION, "❌ Internal error: Invalid size after detection");
                                    debug::error(FUNCTION, "   Expected at least: ", expectedMinSize, " Got: ", decryptedData.size());
                                    debug::error(FUNCTION, "   This should not happen - please report this bug");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    respond(response);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                                
                                /* Extract timestamp */
                                uint64_t nTimestamp = bytes_to_uint64_le(decryptedData, blockSize);
                                
                                /* Build message that was signed: [block][timestamp] */
                                std::vector<uint8_t> vMessage;
                                vMessage.insert(vMessage.end(),
                                                decryptedData.begin(),
                                                decryptedData.begin() + blockSize + FalconConstants::TIMESTAMP_SIZE);
                                
                                /* Extract disposable signature (skip block + timestamp + sig_len, take exactly sigLen bytes)
                                 * Format: [block][timestamp(8)][siglen(2)][sig(N)][physiglen(2)][physical_sig(M)]
                                 * We extract only the disposable signature (N bytes), not the physical signature part */
                                std::vector<uint8_t> vSignature(
                                    decryptedData.begin() + blockSize + FalconConstants::TIMESTAMP_SIZE + FalconConstants::LENGTH_FIELD_SIZE,
                                    decryptedData.begin() + blockSize + FalconConstants::TIMESTAMP_SIZE + FalconConstants::LENGTH_FIELD_SIZE + sigLen
                                );
                                
                                debug::log(0, "   Signed message: [block(" + std::to_string(blockSize) + ")][timestamp(8)]");
                                debug::log(0, "   Total message bytes: ", vMessage.size());
                                debug::log(0, "   Signature bytes: ", vSignature.size());
                                debug::log(0, "   ✅ Matches miner's signing format");
                                debug::log(0, "");
                                debug::log(0, "🔐 FALCON SIGNATURE VERIFICATION:");
                                
                                /* Set up Falcon key for verification */
                                LLC::FLKey verifyKey;
                                if(!verifyKey.SetPubKey(vSessionPubKey))
                                {
                                    debug::error(FUNCTION, "❌ Failed to set public key for verification");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    respond(response);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                                
                                /* Verify Falcon signature directly */
                                if(!verifyKey.Verify(vMessage, vSignature))
                                {
                                    debug::error(FUNCTION, "❌ Falcon signature verification FAILED");
                                    debug::error(FUNCTION, "   Possible causes:");
                                    debug::error(FUNCTION, "   - Signature was signed with different key");
                                    debug::error(FUNCTION, "   - Message format mismatch");
                                    debug::error(FUNCTION, "   - Corrupted signature data");
                                    
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0C);  // Reason: Signature verification failed
                                    response.LENGTH = 1;
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Signature verification failed) ===", ANSI_COLOR_RESET);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                                
                                debug::log(0, "   Status: ✅ VERIFIED");
                                debug::log(2, FUNCTION, "✅ Disposable ", 
                                          DetectedFalconVersionString(context.fFalconVersionDetected, context.nFalconVersion),
                                          " signature verified (", vSignature.size(), " bytes)");
                                debug::log(0, "   Timestamp: ", nTimestamp);
                                debug::log(0, "   Merkle: ", hashMerkleFromBlock.SubString());
                                debug::log(0, "   Nonce: 0x", std::hex, nonceFromBlock, std::dec);
                                debug::log(2, "════════════════════════════════════════════════════════");
                                
                                /* Signature verified successfully */
                                /* The signature verified the entire block + timestamp */
                                /* This authenticates the full block content, not just merkle + nonce */
                                
                                /* Use the verified values from block extraction */
                                hashMerkle = hashMerkleFromBlock;
                                nonce = nonceFromBlock;
                                fFalconVerified = true;
                                
                                /* CHECK FOR OPTIONAL PHYSICAL FALCON SIGNATURE (PR #122)
                                 * Format after disposable sig: [physiglen(2)][physical_sig(var)]
                                 * Physical signature is OPTIONAL for backward compatibility */
                                size_t offsetAfterDisposable = GetDisposableSignatureEndOffset(
                                    blockSize, 
                                    FalconConstants::TIMESTAMP_SIZE, 
                                    FalconConstants::LENGTH_FIELD_SIZE, 
                                    sigLen
                                );
                                
                                bool fHasPhysical = false;
                                std::vector<uint8_t> vchPhysicalSignature;
                                
                                if(decryptedData.size() > offsetAfterDisposable + FalconConstants::LENGTH_FIELD_SIZE)
                                {
                                    /* Read physical signature length */
                                    uint16_t nPhysicalSigLen = static_cast<uint16_t>(decryptedData[offsetAfterDisposable]) |
                                                               (static_cast<uint16_t>(decryptedData[offsetAfterDisposable + 1]) << 8);
                                    
                                    if(nPhysicalSigLen > 0 && 
                                       decryptedData.size() >= offsetAfterDisposable + FalconConstants::LENGTH_FIELD_SIZE + nPhysicalSigLen)
                                    {
                                        /* Extract Physical Falcon signature */
                                        vchPhysicalSignature.assign(
                                            decryptedData.begin() + offsetAfterDisposable + FalconConstants::LENGTH_FIELD_SIZE,
                                            decryptedData.begin() + offsetAfterDisposable + FalconConstants::LENGTH_FIELD_SIZE + nPhysicalSigLen
                                        );
                                        
                                        debug::log(2, FUNCTION, "Physical Falcon signature present (", nPhysicalSigLen, " bytes)");
                                        
                                        /* Verify Physical Falcon signature using same key (key bonding) */
                                        if(!LLP::FalconVerify::VerifyPhysicalFalconSignature(vSessionPubKey, vMessage, vchPhysicalSignature))
                                        {
                                            debug::error(FUNCTION, "❌ Physical Falcon signature verification FAILED");
                                            debug::error(FUNCTION, "   Key bonding requires same key for both signatures");
                                            
                                            StatelessPacket response(BLOCK_REJECTED);
                                            response.DATA.push_back(REJECT_PHYSICAL_SIGNATURE_FAILED);
                                            response.LENGTH = 1;
                                            respond(response);
                                            
                                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Physical signature failed) ===", ANSI_COLOR_RESET);
                                            pFalconWrapper.reset();
                                            return true;
                                        }
                                        
                                        /* Validate size matches session version (key bonding enforcement) */
                                        if(!context.fFalconVersionDetected)
                                        {
                                            debug::error(FUNCTION, "❌ No Falcon version detected for session");
                                            StatelessPacket response(BLOCK_REJECTED);
                                            respond(response);
                                            pFalconWrapper.reset();
                                            return true;
                                        }
                                        
                                        size_t expectedPhysicalSize = LLC::FalconConstants::GetSignatureCTSize(context.nFalconVersion);
                                        if(vchPhysicalSignature.size() != expectedPhysicalSize)
                                        {
                                            debug::error(FUNCTION, "❌ Physical signature size mismatch (key bonding violation)");
                                            debug::error(FUNCTION, "   Expected: ", expectedPhysicalSize, " (Falcon-",
                                                       (context.nFalconVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024"), ")");
                                            debug::error(FUNCTION, "   Got: ", vchPhysicalSignature.size());
                                            
                                            StatelessPacket response(BLOCK_REJECTED);
                                            response.DATA.push_back(REJECT_KEY_BONDING_VIOLATION);
                                            response.LENGTH = 1;
                                            respond(response);
                                            
                                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Key bonding violation) ===", ANSI_COLOR_RESET);
                                            pFalconWrapper.reset();
                                            return true;
                                        }
                                        
                                        fHasPhysical = true;
                                        
                                        debug::log(1, FUNCTION, "✅ Physical ", 
                                                  DetectedFalconVersionString(true, context.nFalconVersion),
                                                  " signature verified (", vchPhysicalSignature.size(), " bytes)");
                                    }
                                }
                                
                                if(!fHasPhysical)
                                {
                                    debug::log(2, FUNCTION, "No Physical Falcon signature (backward compatible)");
                                }
                                
                                /* Store Physical signature in context if present
                                 * 
                                 * NOTE (PR #122): Currently, Physical Falcon signatures are VERIFIED
                                 * and stored in the session context for auditing purposes, but are
                                 * NOT written to the blockchain. This is intentional for this PR.
                                 * 
                                 * Future Enhancement: Blockchain storage of Physical Falcon signatures
                                 * will be added in a subsequent PR to provide permanent proof of
                                 * block authorship. This will require:
                                 * 1. Block structure changes to add vchPhysicalSignature field
                                 * 2. Serialization/deserialization updates
                                 * 3. Consensus rule updates for signature validation
                                 * 4. Database schema changes
                                 * 
                                 * Current behavior: Physical signature is verified for immediate
                                 * security and key bonding enforcement, then discarded after
                                 * block acceptance. The disposable signature remains the primary
                                 * authentication mechanism.
                                 */
                                if(fHasPhysical)
                                {
                                    context = context.WithPhysicalSignature(vchPhysicalSignature);
                                }
                                
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
                                    
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x08);  // Reason: stale timestamp
                                    response.LENGTH = 1;
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Stale Falcon signature) ===", ANSI_COLOR_RESET);
                                    pFalconWrapper.reset();
                                    return true;
                                }
                            }
                            else
                            {
                                /* Fallback: legacy Falcon wrapper [merkle][nonce][timestamp][sig_len][signature] */
                                std::vector<uint8_t> decryptedData;
                                if(!LLC::DecryptPayloadChaCha20(PACKET.DATA, context.vChaChaKey, decryptedData))
                                {
                                    debug::error(FUNCTION, "❌ ChaCha20 decryption FAILED (legacy wrapper)");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0B);  // Reason: ChaCha20 decryption failure
                                    response.LENGTH = 1;
                                    respond(response);
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 decryption failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                auto falconWrapper = LLP::DisposableFalcon::Create();
                                if(!falconWrapper)
                                {
                                    debug::error(FUNCTION, "❌ Failed to create disposable Falcon wrapper");
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0xFF);  // Reason: Internal error
                                    response.LENGTH = 1;
                                    respond(response);
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Internal error) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                auto unwrapResult = falconWrapper->UnwrapWorkSubmission(decryptedData, vSessionPubKey);
                                if(!unwrapResult.fSuccess || !unwrapResult.submission.fSigned)
                                {
                                    debug::error(FUNCTION, "❌ Falcon wrapper verification failed: ", unwrapResult.strError);
                                    StatelessPacket response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0C);  // Reason: Signature verification failed
                                    response.LENGTH = 1;
                                    respond(response);
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Signature verification failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }

                                hashMerkle = unwrapResult.submission.hashMerkleRoot;
                                nonce = unwrapResult.submission.nNonce;
                                fFalconVerified = true;

                                debug::log(2, FUNCTION, "✅ Disposable Falcon wrapper verified legacy submission");
                            }
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
                    
                    StatelessPacket response(BLOCK_REJECTED);
                    response.DATA.push_back(0x0F);  // Reason: Packet too small
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Packet too small) ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
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
                    
                    StatelessPacket response(BLOCK_REJECTED);
                    response.DATA.push_back(0xFF);  // Reason: Internal error
                    response.LENGTH = 1;
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Internal error) ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                /* Continue with existing template lookup and validation... */
                StatelessMinerManager::Get().IncrementBlocksSubmitted();

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
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
                    pFalconWrapper.reset();
                    return true;
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Found original template (wallet-signed)", ANSI_COLOR_RESET);

                /* Get iterator to block template for processing */
                auto it = mapBlocks.find(hashMerkle);
                if(it == mapBlocks.end())
                {
                    debug::error(FUNCTION, "❌ Template lookup failed (race condition)");
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (template lookup failed) ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkle))
                {
                    debug::error(FUNCTION, "❌ sign_block failed (nonce update failed)");
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (sign_block failed) ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                TAO::Ledger::TritiumBlock* pTritium =
                    dynamic_cast<TAO::Ledger::TritiumBlock*>(it->second.pBlock.get());
                if(!pTritium)
                {
                    debug::error(FUNCTION, "❌ invalid block type (expected TritiumBlock)");
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (invalid block type) ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                TAO::Ledger::BlockValidationResult validationResult =
                    TAO::Ledger::ValidateMinedBlock(*pTritium);
                if(!validationResult.valid)
                {
                    debug::error(FUNCTION, "❌ ValidateMinedBlock failed: ", validationResult.reason);
                    
                    /* Notify local pool if enabled */
                    if(PoolDiscovery::IsLocalPoolEnabled() && context.hashGenesis != 0)
                    {
                        PoolDiscovery::OnBlockSubmitted(context.hashGenesis, false);
                    }
                    
                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (", validationResult.reason, ") ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                TAO::Ledger::BlockAcceptanceResult acceptanceResult =
                    TAO::Ledger::AcceptMinedBlock(*pTritium);
                if(!acceptanceResult.accepted)
                {
                    debug::error(FUNCTION, "❌ AcceptMinedBlock failed: ", acceptanceResult.reason);

                    /* Notify local pool if enabled */
                    if(PoolDiscovery::IsLocalPoolEnabled() && context.hashGenesis != 0)
                    {
                        PoolDiscovery::OnBlockSubmitted(context.hashGenesis, false);
                    }

                    StatelessPacket response(STATELESS_BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (", acceptanceResult.reason, ") ===", ANSI_COLOR_RESET);
                    pFalconWrapper.reset();
                    return true;
                }

                /* Block accepted - track in manager */
                StatelessMinerManager::Get().IncrementBlocksAccepted();

                /* Notify local pool if enabled */
                if(PoolDiscovery::IsLocalPoolEnabled() && context.hashGenesis != 0)
                {
                    PoolDiscovery::OnBlockSubmitted(context.hashGenesis, true);
                    
                    /* Calculate reward for block found notification */
                    uint64_t nReward = 0;
                    auto it = mapBlocks.find(hashMerkle);
                    if(it != mapBlocks.end() && it->second.pBlock)
                    {
                        /* Get previous block state for reward calculation */
                        TAO::Ledger::BlockState statePrev = TAO::Ledger::ChainState::tStateBest.load();
                        
                        /* Get block reward (in NXS base units) */
                        nReward = TAO::Ledger::GetCoinbaseReward(
                            statePrev, 
                            it->second.pBlock->nChannel, 
                            0);  // nType = 0 for mining rewards
                        
                        PoolDiscovery::OnBlockFound(context.hashGenesis, nReward);
                    }
                }

                /* Generate an Accepted response. */
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✅ Block accepted by network!", ANSI_COLOR_RESET);
                debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK result=accepted merkle=", hashMerkle.SubString(),
                           " channel=", acceptanceResult.nChannel, " height=", acceptanceResult.nHeight);
                
                /* Log signature configuration (PR #122) */
                LogFalconSignatureInfo(context);
                
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK: SUCCESS ===", ANSI_COLOR_RESET);
                
                /* Get block for detailed logging (reuse iterator from earlier) */
                if(it != mapBlocks.end() && it->second.pBlock)
                {
                    TAO::Ledger::Block *pBlock = it->second.pBlock.get();
                    debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   🎉 Block ", pBlock->nHeight, " accepted by Nexus network", ANSI_COLOR_RESET);
                    debug::log(0, "   Miner: ", GetAddress().ToStringIP());
                    debug::log(0, "   Channel: ", pBlock->nChannel, " (", (pBlock->nChannel == 1 ? "Prime" : "Hash"), ")");
                }
                
                StatelessPacket response(STATELESS_BLOCK_ACCEPTED);
                respond(response);
                pFalconWrapper.reset();
                debug::log(2, FUNCTION, "✓ Disposable Falcon wrapper discarded after SUBMIT_BLOCK");

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
            /* FIXED SCHEMA: Returns unified height + channel-specific height + difficulty (12 bytes total) */
            if(PACKET.HEADER == GET_ROUND)
            {
                // AUTOMATED RATE LIMIT CHECK
                if (!CheckRateLimit(GET_ROUND)) {
                    // Request rejected - violation already recorded
                    // Send OLD_ROUND response to indicate rate limited
                    debug::log(1, FUNCTION, "GET_ROUND rate limited for ", GetAddress().ToStringIP());
                    StatelessPacket response(OLD_ROUND);
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
                    StatelessPacket response(OLD_ROUND);
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
                    StatelessPacket response(OLD_ROUND);
                    response.LENGTH = 0;
                    respond(response);
                    return true;
                }

                /* ═════════════════════════════════════════════════════════════════════════ */
                /* FIXED SCHEMA: GET_ROUND/NEW_ROUND with channel-specific height           */
                /* ═════════════════════════════════════════════════════════════════════════ */
                
                /* Get current blockchain state */
                TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
                
                /* Validate blockchain is ready */
                if(stateBest.nHeight == 0)
                {
                    debug::error(FUNCTION, "GET_ROUND: Blockchain not initialized");
                    return true;  // Don't send response
                }
                
                uint32_t nUnifiedHeight = stateBest.nHeight;
                debug::log(3, FUNCTION, "GET_ROUND: Unified height: ", nUnifiedHeight);
                
                /* Get channel-specific height using GetLastState() */
                TAO::Ledger::BlockState stateChannel = stateBest;
                uint32_t nChannelHeight = 0;
                
                if(TAO::Ledger::GetLastState(stateChannel, context.nChannel))
                {
                    nChannelHeight = stateChannel.nChannelHeight;
                    debug::log(3, FUNCTION, "GET_ROUND: Channel ", context.nChannel, " height: ", nChannelHeight);
                }
                else
                {
                    debug::warning(FUNCTION, "GET_ROUND: Could not get channel ", context.nChannel, " state");
                    /* Continue with height 0 - channel may not exist yet (first block) */
                }
                
                /* Calculate next difficulty for this channel
                 * GetNextTargetRequired(state, channel, fDebug)
                 * - fDebug=false: Suppress debug logging for production use
                 */
                uint32_t nDifficulty = TAO::Ledger::GetNextTargetRequired(stateBest, context.nChannel, false);
                debug::log(3, FUNCTION, "GET_ROUND: Channel ", context.nChannel, " next difficulty: 0x", std::hex, nDifficulty, std::dec);
                
                /* ═════════════════════════════════════════════════════════════════════════ */
                /* BUILD FIXED RESPONSE (12 bytes total)                                    */
                /* ═════════════════════════════════════════════════════════════════════════ */
                
                /* Response format (FIXED SCHEMA):
                 *   [0-3]   uint32_t nUnifiedHeight  - Current blockchain height (all channels)
                 *   [4-7]   uint32_t nChannelHeight  - Last block height in miner's channel
                 *   [8-11]  uint32_t nDifficulty     - Next target difficulty for miner's channel
                 * 
                 * Total: 12 bytes
                 * 
                 * RATIONALE:
                 * - Miner needs unified height for block template
                 * - Miner needs channel height to validate Block::Accept() rule:
                 *     if(statePrev.nChannelHeight + 1 != nChannelHeight) → REJECT
                 * - Miner needs difficulty for block template nBits field
                 * 
                 * This eliminates FALSE OLD_ROUND rejections caused by missing channel height.
                 */
                std::vector<uint8_t> vData;
                vData.reserve(12);
                
                /* Pack unified height (4 bytes, little-endian) */
                vData.push_back((nUnifiedHeight >> 0) & 0xFF);
                vData.push_back((nUnifiedHeight >> 8) & 0xFF);
                vData.push_back((nUnifiedHeight >> 16) & 0xFF);
                vData.push_back((nUnifiedHeight >> 24) & 0xFF);
                
                /* Pack channel height (4 bytes, little-endian) */
                vData.push_back((nChannelHeight >> 0) & 0xFF);
                vData.push_back((nChannelHeight >> 8) & 0xFF);
                vData.push_back((nChannelHeight >> 16) & 0xFF);
                vData.push_back((nChannelHeight >> 24) & 0xFF);
                
                /* Pack difficulty (4 bytes, little-endian) */
                vData.push_back((nDifficulty >> 0) & 0xFF);
                vData.push_back((nDifficulty >> 8) & 0xFF);
                vData.push_back((nDifficulty >> 16) & 0xFF);
                vData.push_back((nDifficulty >> 24) & 0xFF);
                
                /* CRITICAL VALIDATION: Ensure exactly 12 bytes */
                if(vData.size() != 12)
                {
                    debug::error(FUNCTION, "GET_ROUND: Response size mismatch!");
                    debug::error(FUNCTION, "   Expected: 12 bytes");
                    debug::error(FUNCTION, "   Got: ", vData.size(), " bytes");
                    debug::error(FUNCTION, "   This is a CRITICAL BUG - miner will receive malformed packet");
                    return true;  // Don't send malformed response
                }
                
                /* ═════════════════════════════════════════════════════════════════════════ */
                /* SEND NEW_ROUND WITH CHANNEL-SPECIFIC HEIGHTS                             */
                /* ═════════════════════════════════════════════════════════════════════════ */
                
                /* DESIGN: We send NEW_ROUND with 12 bytes containing:
                 *   - Unified height (for block template nHeight)
                 *   - Channel height (for block template nChannelHeight)
                 *   - Difficulty (for block template nBits)
                 * 
                 * This provides ALL information the miner needs to:
                 * 1. Create a valid block template with correct heights
                 * 2. Validate that channel height matches blockchain state
                 * 3. Set correct difficulty in block header
                 * 
                 * This eliminates FALSE OLD_ROUND rejections from Block::Accept():
                 *   if(statePrev.nChannelHeight + 1 != nChannelHeight) → REJECT
                 */
                
                debug::log(3, FUNCTION, "✓ Sending NEW_ROUND (12 bytes): Unified=", nUnifiedHeight,
                           " Channel=", nChannelHeight, " Difficulty=0x", std::hex, nDifficulty, std::dec,
                           " (", std::fixed, std::setprecision(6), TAO::Ledger::GetDifficulty(nDifficulty, context.nChannel), ")");
                
                /* Enhanced diagnostic logging */
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "📤 SENDING NEW_ROUND RESPONSE");
                debug::log(2, "════════════════════════════════════════════════════════════");
                debug::log(2, "   To:             ", GetAddress().ToStringIP());
                debug::log(2, "   Opcode:         NEW_ROUND (204/0xCC)");
                debug::log(2, "   Response Data:");
                debug::log(2, "      Unified Height:  ", nUnifiedHeight);
                debug::log(2, "      Channel Height:  ", nChannelHeight, " (channel ", context.nChannel, ")");
                debug::log(2, "      Difficulty (nBits): 0x", std::hex, nDifficulty, std::dec);
                debug::log(2, "      Difficulty (calc):  ", std::fixed, std::setprecision(6), 
                           TAO::Ledger::GetDifficulty(nDifficulty, context.nChannel));
                debug::log(2, "   Packet Size:    12 bytes");
                debug::log(2, "");
                debug::log(2, "   ⚠️  NOTE:");
                debug::log(2, "      This is the legacy polling flow response.");
                debug::log(2, "      Preferred flow is push notifications:");
                debug::log(2, "         MINER_READY → ", (context.nChannel == 1 ? "PRIME" : "HASH"), "_BLOCK_AVAILABLE");
                debug::log(2, "         Then client issues GET_BLOCK automatically.");
                debug::log(2, "════════════════════════════════════════════════════════════");
                
                /* Send response */
                StatelessPacket response(NEW_ROUND);
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
                 * 2. Checks if miner's last known height differs from current height
                 * 3. If changed: Auto-send BLOCK_DATA (like GET_BLOCK does)
                 * 4. If same: Skip template send (miner already has current template)
                 * 
                 * This maintains backward compatibility with legacy mining software that relies
                 * on GET_ROUND polling to automatically deliver templates.
                 */
                
                bool fHeightChanged = (context.nHeight != nUnifiedHeight);
                
                debug::log(2, "");
                debug::log(2, "   🔍 GET_ROUND TEMPLATE AUTO-SEND CHECK:");
                debug::log(2, "      Miner's last height:  ", context.nHeight);
                debug::log(2, "      Current height:       ", nUnifiedHeight);
                debug::log(2, "      Height changed:       ", (fHeightChanged ? "YES" : "NO"));
                
                if(fHeightChanged)
                {
                    debug::log(2, "");
                    debug::log(2, "   ✅ HEIGHT CHANGED - AUTO-SENDING TEMPLATE");
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
                                /* Send BLOCK_DATA packet */
                                StatelessPacket blockPacket(BLOCK_DATA);
                                blockPacket.DATA = vBlockData;
                                blockPacket.LENGTH = static_cast<uint32_t>(vBlockData.size());
                                respond(blockPacket);
                                
                                debug::log(2, "   ✅ BLOCK_DATA AUTO-SENT!");
                                debug::log(2, "      Template size:    ", vBlockData.size(), " bytes");
                                debug::log(2, "      Block height:     ", pBlock->nHeight);
                                debug::log(2, "      Block channel:    ", pBlock->nChannel);
                                debug::log(2, "      Merkle root:      ", pBlock->hashMerkleRoot.SubString());
                                
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
                    debug::log(2, "   ℹ️  HEIGHT UNCHANGED - NO TEMPLATE SENT");
                    debug::log(2, "      Miner should continue mining current template.");
                    debug::log(2, "      If miner needs new template, use GET_BLOCK explicitly.");
                }
                
                debug::log(2, "════════════════════════════════════════════════════════════");
                
                /* Update context timestamp and height 
                 * Note: Height is only updated if template was sent to prevent duplicate sends.
                 * If no template was sent (height unchanged), we keep the old height so next
                 * GET_ROUND at a new height will trigger template send.
                 * 
                 * NOTE: We do NOT call CleanupStaleTemplates() here because:
                 * 1. It's already called when creating new templates (NEW_BLOCK handler)
                 * 2. Templates have age-based automatic expiration via TemplateMetadata::IsStale()
                 * 3. Calling cleanup on every GET_ROUND poll (every 5-10s) would be excessive
                 * 4. Template cleanup should be driven by template creation, not polling
                 */
                if(fHeightChanged)
                {
                    /* Update height only if we sent a template */
                    context = context.WithTimestamp(runtime::unifiedtimestamp())
                                     .WithHeight(nUnifiedHeight);
                }
                else
                {
                    /* Update only timestamp, keep existing height */
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

                /* Attempt lane-switch recovery based on address */
                auto optExisting = SessionRecoveryManager::Get().RecoverSessionByAddress(GetAddress().ToStringIP());
                if(optExisting.has_value())
                {
                    uint256_t hashRecoveredKey(0);
                    uint64_t nRecoveredNonce = 0;
                    if(SessionRecoveryManager::Get().RestoreChaCha20State(optExisting->hashKeyID, hashRecoveredKey, nRecoveredNonce)
                       && hashRecoveredKey != 0)
                    {
                        context = context.WithChaChaKey(hashRecoveredKey.GetBytes());
                    }

                    std::vector<uint8_t> vRecoveredPubKey;
                    uint256_t hashDisposableKeyID(0);
                    if(SessionRecoveryManager::Get().RestoreDisposableKey(optExisting->hashKeyID, vRecoveredPubKey, hashDisposableKeyID)
                       && !vRecoveredPubKey.empty())
                    {
                        std::lock_guard<std::mutex> lock(SESSION_MUTEX);
                        if(optExisting->nSessionId != 0)
                            mapSessionKeys[optExisting->nSessionId] = vRecoveredPubKey;
                    }

                    debug::log(0, FUNCTION, "Session recovered from lane switch");
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
                    debug::log(0, "   Genesis: ", context.hashGenesis.SubString());
                    debug::log(0, "   Encryption ready: YES");
                }
                
                debug::log(0, FUNCTION, "✓ Miner subscribed to ", 
                          (context.nChannel == 1 ? "Prime" : "Hash"), " notifications");
                debug::log(0, "   Encryption ready: ", (context.fEncryptionReady ? "YES" : "NO"));
                debug::log(0, "   ChaCha key size: ", context.vChaChaKey.size(), " bytes");
                
                /* Send immediate notification with current state */
                SendChannelNotification();
                
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

                /* Update manager with COMPLETE context including encryption state */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                
                debug::log(2, "📥 === MINER_READY: SUCCESS ===");
                return true;
            }

            /* For all other packets, route through StatelessMiner processor */
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

                        if(result.context.hashKeyID != 0)
                        {
                            SessionRecoveryManager::Get().SaveDisposableKey(
                                result.context.hashKeyID,
                                result.context.vMinerPubKey,
                                LLC::SK256(result.context.vMinerPubKey)
                            );
                        }
                        
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
                        debug::log(0, "   Genesis: ", context.hashGenesis.SubString());
                        debug::log(0, "   Encryption ready: YES");
                    }
                    else if(context.hashGenesis == 0)
                    {
                        debug::warning(FUNCTION, "⚠ Authentication succeeded but genesis hash is 0");
                        debug::warning(FUNCTION, "   ChaCha20 encryption will NOT be available");
                        debug::warning(FUNCTION, "   This may indicate incomplete Falcon authentication");
                    }
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

                /* Update manager with new context after successful packet processing */
                /* CRITICAL: This must be done AFTER ChaCha20 key derivation to ensure
                 * StatelessMinerManager has the complete encryption state */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context, 1);
                
                /* Log session registration for auth packets */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && context.fAuthenticated)
                {
                    debug::log(0, FUNCTION, "Session registered: address=", context.strAddress,
                               " sessionId=", context.nSessionId, " keyID=", context.hashKeyID.SubString());
                }
                
                /* Send response if present */
                if(!result.response.IsNull())
                {
                    /* Mirror 8-bit response opcodes to 16-bit for stateless lane.
                     * StatelessMiner builds responses with 8-bit opcodes (e.g., MINER_AUTH_CHALLENGE = 208),
                     * but the stateless lane expects 16-bit mirror-mapped opcodes (0xD0D0).
                     * 
                     * FIX: Only mirror legacy response opcodes (200-206) that need mirroring.
                     * DO NOT mirror stateless-specific opcodes (207-218) - they're already protocol-specific.
                     * DO NOT mirror already-mirrored 16-bit opcodes (0xD000+ range).
                     * 
                     * Valid ranges:
                     * - 200-206: Legacy responses (BLOCK_ACCEPTED, etc.) → SHOULD mirror
                     * - 207-218: Stateless-specific (AUTH, PUSH) → SKIP mirroring
                     * - 0xD000+: Already mirrored 16-bit opcodes → SKIP mirroring
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
                        /* Send original - either already stateless-specific (207-218) or 16-bit */
                        if(result.response.HEADER >= MiningConstants::STATELESS_MINING_OPCODE_MIN &&
                           result.response.HEADER <= MiningConstants::STATELESS_MINING_OPCODE_MAX)
                        {
                            debug::log(3, FUNCTION, "Sending stateless-specific opcode ", 
                                      uint32_t(result.response.HEADER), " without mirroring");
                        }
                        respond(result.response);  // Send original if no mirroring needed
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
        /* Early exit if shutdown is in progress */
        if (config::fShutdown.load())
        {
            debug::log(1, FUNCTION, "Shutdown in progress; skipping template creation");
            return nullptr;
        }
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Request from ", GetAddress().ToStringIP(), " ===", ANSI_COLOR_RESET);
        
        /* Validate channel is set BEFORE creating template */
        if(context.nChannel == 0)
        {
            debug::error(FUNCTION, "❌ Cannot create template: nChannel not set");
            debug::error(FUNCTION, "   Required: Miner must send SET_CHANNEL before GET_BLOCK");
            debug::error(FUNCTION, "   Current context.nChannel: ", context.nChannel);
            return nullptr;
        }
        
        /* Validate channel value is valid (1=Prime, 2=Hash) */
        if(context.nChannel != 1 && context.nChannel != 2)
        {
            debug::error(FUNCTION, "❌ Invalid channel: ", context.nChannel);
            debug::error(FUNCTION, "   Valid channels: 1 (Prime), 2 (Hash)");
            return nullptr;
        }
        
        debug::log(0, "   Channel validated: ", context.nChannel, 
                   " (", (context.nChannel == 1 ? "Prime" : "Hash"), ")");
        
        /* Get CURRENT blockchain height FIRST */
        uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Creating template ===", ANSI_COLOR_RESET);
        debug::log(0, "   Current blockchain height: ", nCurrentHeight);
        debug::log(0, "   Template will target height: ", nCurrentHeight + 1);
        
        /* Cleanup old templates using the new dedicated method */
        CleanupStaleTemplates(nCurrentHeight);
        
        /* Determine reward - same priority as miner.cpp */
        debug::log(0, "   Determining reward address...");
        uint256_t hashReward = 0;
        
        if(context.fRewardBound && context.hashRewardAddress != 0) {
            hashReward = context.hashRewardAddress;
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "      Using bound reward address: ", hashReward.SubString(), ANSI_COLOR_RESET);
        }
        else if(context.hashGenesis != 0) {
            hashReward = context.hashGenesis;
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "      Using genesis hash: ", hashReward.SubString(), ANSI_COLOR_RESET);
        }
        else {
            debug::error(FUNCTION, "No reward address available");
            debug::log(0, ANSI_COLOR_BRIGHT_RED, "   FAILED: No reward address", ANSI_COLOR_RESET);
            return nullptr;
        }
        
        debug::log(0, "   Block parameters:");
        debug::log(0, "      Channel: ", context.nChannel);
        debug::log(0, "      Session ID: ", context.nSessionId);
        debug::log(0, "      Falcon authenticated: ", context.fAuthenticated ? "Yes" : "No");
        
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
                context.nChannel,
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
        
        /* ✅ ADD: AFTER block is created, verify height is still valid */
        if(pBlock)
        {
            uint32_t nHeightNow = TAO::Ledger::ChainState::nBestHeight.load();
            if(pBlock->nHeight != nHeightNow + 1)
            {
                debug::error(FUNCTION, "❌ Template became stale during creation!");
                debug::error(FUNCTION, "   Created for height: ", pBlock->nHeight);
                debug::error(FUNCTION, "   Blockchain now at: ", nHeightNow);
                debug::error(FUNCTION, "   Template is INVALID - discarding");
                delete pBlock;
                return nullptr;
            }
            
            debug::log(0, "   ✓ Template validated at height ", pBlock->nHeight);
            debug::log(0, "   Valid until height change");
        }
        
        /* PR #136: Use ChannelStateManager for fork-aware state management */
        uint64_t nCreationTime = runtime::unifiedtimestamp();
        
        /* Get channel manager for this channel */
        ChannelStateManager* pChannelMgr = GetChannelManager(context.nChannel);
        if(!pChannelMgr)
        {
            debug::error(FUNCTION, "Failed to get channel manager for channel ", context.nChannel);
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
            
            /* Clear templates first, then clear fork flag */
            clear_map();
            
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
        debug::log(0, "      Unified height (current):  ", info.nUnifiedHeight);
        debug::log(0, "      Unified height (template): ", pBlock->nHeight, " = ", info.nNextUnifiedHeight);
        debug::log(0, "      Channel height (current):  ", info.nChannelHeight);
        debug::log(0, "      Channel height (template): ", info.nNextChannelHeight, " ← CORRECT");
        
        TemplateMetadata meta(pBlock, nCreationTime, pBlock->nHeight, info.nNextChannelHeight, 
                             pBlock->hashMerkleRoot, context.nChannel);
        auto result = mapBlocks.emplace(pBlock->hashMerkleRoot, std::move(meta));
        
        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Template stored in map with metadata", ANSI_COLOR_RESET);
        debug::log(0, "      Merkle root: ", pBlock->hashMerkleRoot.SubString());
        debug::log(0, "      Unified height: ", pBlock->nHeight);
        debug::log(0, "      Channel height: ", info.nNextChannelHeight, " (mining for next ", pChannelMgr->GetChannelName(), " block)");
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
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Complete ===", ANSI_COLOR_RESET);
        return pBlock;
    }


    /** Sign a block */
    bool StatelessMinerConnection::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
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
        
        /* ✅ Template is valid - proceed with update */
        TAO::Ledger::Block* pBaseBlock = meta.pBlock.get();
        if(!pBaseBlock)
        {
            debug::error(FUNCTION, "❌ Template has null block pointer!");
            return false;
        }
        
        pBaseBlock->nNonce = nNonce;
        
        debug::log(0, "   ✅ Template updated successfully");
        debug::log(0, "      Height: ", meta.nHeight);
        debug::log(0, "      Age: ", runtime::unifiedtimestamp() - meta.nCreationTime, "s");

        /* If the block dynamically casts to a tritium block, validate the tritium block. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock *>(pBaseBlock);
        if(pBlock)
        {
            debug::log(0, "   Merkle root: ", hashMerkleRoot.SubString());
            debug::log(0, "   Miner's nonce: 0x", std::hex, nNonce, std::dec);
            debug::log(0, "   Block height: ", pBlock->nHeight);
            debug::log(0, "   Block channel: ", pBlock->nChannel);
            
            /* Update the block's timestamp. */
            pBlock->UpdateTime();
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Timestamp updated", ANSI_COLOR_RESET);

            /* ============================================================
             * TRAINING WHEELS MODE: Comprehensive Diagnostic Logging
             * ============================================================ */
            
            if(pBlock->nChannel == 1)  // Prime channel
            {
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === PRIME CHANNEL DIAGNOSTIC (Training Wheels Mode) ===", ANSI_COLOR_RESET);
                
                /* Calculate hashPrime (same calculation miner did) */
                uint1024_t hashPrime = pBlock->GetPrime();
                
                debug::log(0, "📊 PRIME BASE CALCULATION:");
                debug::log(0, "   ProofHash() = ", pBlock->ProofHash().ToString().substr(0, 64), "...");
                debug::log(0, "   nNonce      = 0x", std::hex, pBlock->nNonce, std::dec);
                debug::log(0, "   hashPrime   = ProofHash() + nNonce");
                debug::log(0, "   hashPrime   = ", hashPrime.ToString().substr(0, 64), "...");
                debug::log(0, "   (Full 1024-bit value shown in hex above)");
                
                /* Test if base is prime using node's PrimeCheck */
                debug::log(0, "");
                debug::log(0, "🧪 PRIME VALIDATION TEST:");
                debug::log(0, "   Calling TAO::Ledger::PrimeCheck(hashPrime)...");
                
                bool isPrimeBase = TAO::Ledger::PrimeCheck(hashPrime);
                
                if(isPrimeBase)
                {
                    debug::log(0, "   Result: ", ANSI_COLOR_BRIGHT_GREEN, "✅ BASE IS PRIME", ANSI_COLOR_RESET);
                }
                else
                {
                    debug::log(0, "   Result: ", ANSI_COLOR_BRIGHT_RED, "❌ BASE IS NOT PRIME", ANSI_COLOR_RESET);
                    debug::log(0, "   ⚠️  Mismatch: Miner validated this as prime, but node disagrees!");
                    debug::log(0, "   ⚠️  This indicates either:");
                    debug::log(0, "      - Different PrimeCheck implementations (miner vs node)");
                    debug::log(0, "      - Different hashPrime calculation (endianness? encoding?)");
                    debug::log(0, "      - Nonce corruption during transmission");
                }
                
                /* Calculate Cunningham chain offsets */
                debug::log(0, "");
                debug::log(0, "🔗 CUNNINGHAM CHAIN CALCULATION:");
                debug::log(0, "   Calling TAO::Ledger::GetOffsets(hashPrime, vOffsets)...");
                debug::log(0, "   Before GetOffsets: vOffsets.size() = ", pBlock->vOffsets.size());
                
                TAO::Ledger::GetOffsets(hashPrime, pBlock->vOffsets);
                
                debug::log(0, "   After GetOffsets: vOffsets.size() = ", pBlock->vOffsets.size());
                
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
                
                /* Ensure no prime offsets for hash channel */
                if(!pBlock->vOffsets.empty())
                {
                    debug::log(0, "");
                    debug::log(0, "   ⚠️  WARNING: Hash channel has ", pBlock->vOffsets.size(), " offsets (should be empty)");
                    debug::log(0, "   Clearing invalid offsets...");
                    pBlock->vOffsets.clear();
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "🔬 === END HASH DIAGNOSTIC ===", ANSI_COLOR_RESET);
            }
            
            debug::log(0, "");
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Block prepared for validation", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📝 === SIGN_BLOCK: Complete ===", ANSI_COLOR_RESET);
            
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
        
        debug::log(2, FUNCTION, "🧹 Cleaning stale templates...");
        debug::log(2, FUNCTION, "   Current height: ", nCurrentHeight);
        debug::log(2, FUNCTION, "   Templates before cleanup: ", mapBlocks.size());
        
        for(auto it = mapBlocks.begin(); it != mapBlocks.end(); )
        {
            const TemplateMetadata& meta = it->second;
            
            /* Check age-based staleness */
            if(meta.IsStale(nNow))
            {
                uint64_t nAge = nNow - meta.nCreationTime;
                debug::log(2, FUNCTION, "   ❌ Removing template (age: ", nAge, "s)");
                debug::log(2, FUNCTION, "      Merkle: ", it->first.SubString());
                it = mapBlocks.erase(it);  // unique_ptr automatically deletes pBlock
                ++nRemoved;
                continue;
            }
            
            /* PR #134: Check staleness using IsStale() (checks both age and channel height) */
            if(meta.IsStale())
            {
                debug::log(2, FUNCTION, "   ❌ Removing stale template");
                debug::log(2, FUNCTION, "      Unified height: ", meta.nHeight, " (current: ", nCurrentHeight, ")");
                debug::log(2, FUNCTION, "      Channel height: ", meta.nChannelHeight, " (", meta.GetChannelName(), ")");
                debug::log(2, FUNCTION, "      Age: ", meta.GetAge(), "s");
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

    bool StatelessMinerConnection::CheckRateLimit(uint16_t nRequestType)
    {
        auto now = std::chrono::steady_clock::now();
        
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
        
        // If in throttle mode, check minimum interval enforcement (non-blocking)
        // Instead of blocking with sleep, we reject requests that come too soon
        if (m_rateLimit.fThrottleMode) {
            auto lastRequestTime = m_rateLimit.tLastGetRound;
            if (nRequestType == GET_BLOCK) {
                lastRequestTime = m_rateLimit.tLastGetBlock;
            } else if (nRequestType == SUBMIT_BLOCK) {
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
                          (context.nChannel == 1 ? "Prime" : context.nChannel == 2 ? "Hash" : "Unknown"), ")");
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
                          (context.nChannel == 1 ? "Prime" : context.nChannel == 2 ? "Hash" : "Unknown"), ")");
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
                
                // Check minimum interval
                auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_rateLimit.tLastGetBlock).count();
                
                if (m_rateLimit.tLastGetBlock.time_since_epoch().count() > 0 && 
                    intervalMs < MiningConstants::GET_BLOCK_MIN_INTERVAL_MS) 
                {
                    std::string reason = "GET_BLOCK interval too short: " + 
                        std::to_string(intervalMs) + "ms < " + 
                        std::to_string(MiningConstants::GET_BLOCK_MIN_INTERVAL_MS) + "ms minimum";
                    RecordViolation(reason);
                    return false;
                }
                
                // Check per-minute limit
                if (m_rateLimit.nGetBlockCount >= RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE) {
                    std::string reason = "GET_BLOCK limit exceeded: " + 
                        std::to_string(m_rateLimit.nGetBlockCount) + "/" + 
                        std::to_string(RateLimitConfig::MAX_GET_BLOCK_PER_MINUTE) + " per minute";
                    RecordViolation(reason);
                    return false;
                }
                
                // Request allowed
                m_rateLimit.nGetBlockCount++;
                m_rateLimit.tLastGetBlock = now;
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
        
        /* Notify local pool metrics if enabled */
        if(PoolDiscovery::IsLocalPoolEnabled())
        {
            PoolDiscovery::IncrementRateLimitViolations();
        }
        
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
        
        /* Build notification using unified builder */
        StatelessPacket notification = PushNotificationBuilder::BuildChannelNotification<StatelessPacket>(
            nChannel, ProtocolLane::STATELESS, stateBest, stateChannel, nDifficulty);
        
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
        
        /* Capture timestamp for accurate timing measurements
         * Using same timestamp both for updating context and for client timing calculations */
        uint64_t nNotificationTimestamp = runtime::unifiedtimestamp();
        
        /* Update statistics (thread-safe) */
        {
            LOCK(MUTEX);
            context = context.WithNotificationSent(nNotificationTimestamp);
        }  // MUTEX automatically unlocked here
        
        debug::log(2, FUNCTION, "Sent ", GetChannelName(nChannel), 
                   " notification to ", GetAddress().ToStringIP(),
                   " (unified=", stateBest.nHeight, 
                   ", channelHeight=", nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");
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
        
        /* Create new block template - note: new_block() stores in mapBlocks, so we don't own the pointer */
        TAO::Ledger::Block* pBlock = new_block();
        if (!pBlock)
        {
            debug::error(FUNCTION, "Failed to create block template");
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
        debug::log(2, "   Total Size:     ", notification.DATA.size(), " bytes (", 
                   METADATA_SIZE, " meta + ", TRITIUM_BLOCK_SIZE, " template)");
        debug::log(2, "");
        debug::log(2, "   ⚡ STATELESS PROTOCOL:");
        debug::log(2, "      Miner can begin hashing immediately (no GET_BLOCK needed)");
        debug::log(2, "      Template pushed automatically on new blocks");
        debug::log(2, "════════════════════════════════════════════════════════════");
        
        /* Send to miner */
        respond(notification);
        
        /* Update statistics */
        uint64_t nNotificationTimestamp = runtime::unifiedtimestamp();
        {
            LOCK(MUTEX);
            context = context.WithNotificationSent(nNotificationTimestamp);
        }
        
        debug::log(2, FUNCTION, "Sent stateless template (0xD081) to ", GetAddress().ToStringIP(),
                   " (unified=", stateBest.nHeight, 
                   ", channel=", nChannelHeight,
                   ", diff=", std::hex, nDifficulty, std::dec, ")");
    }

} // namespace LLP
