/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/types/stateless_miner_connection.h>
#include <LLP/include/stateless_miner.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/falcon_verify.h>
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

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/convert.h>
#include <Util/include/config.h>

#include <chrono>
#include <limits>
#include <algorithm>
#include <iomanip>

namespace LLP
{
    /* The block iterator to act as extra nonce. */
    std::atomic<uint32_t> StatelessMinerConnection::nBlockIterator(0);
    /** Default Constructor **/
    StatelessMinerConnection::StatelessMinerConnection()
    : Connection()
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    {
        /* Create disposable Falcon wrapper instance */
        m_pFalconWrapper = LLP::DisposableFalcon::Create();
        
        if (!m_pFalconWrapper)
        {
            debug::error(FUNCTION, "Failed to create DisposableFalconWrapper");
        }
        else
        {
            debug::log(2, FUNCTION, "✓ Disposable Falcon wrapper initialized");
        }
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(SOCKET_IN, DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    {
        /* Create disposable Falcon wrapper instance */
        m_pFalconWrapper = LLP::DisposableFalcon::Create();
        
        if (!m_pFalconWrapper)
        {
            debug::error(FUNCTION, "Failed to create DisposableFalconWrapper");
        }
        else
        {
            debug::log(2, FUNCTION, "✓ Disposable Falcon wrapper initialized");
        }
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    , mapBlocks()
    , mapSessionKeys()
    , SESSION_MUTEX()
    {
        /* Create disposable Falcon wrapper instance */
        m_pFalconWrapper = LLP::DisposableFalcon::Create();
        
        if (!m_pFalconWrapper)
        {
            debug::error(FUNCTION, "Failed to create DisposableFalconWrapper");
        }
        else
        {
            debug::log(2, FUNCTION, "✓ Disposable Falcon wrapper initialized");
        }
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
                    Packet PACKET = this->INCOMING;
                    debug::log(2, FUNCTION, "MinerLLP: HEADER from ", GetAddress().ToStringIP(),
                               " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
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
                StatelessMinerManager::Get().UpdateMiner(strAddr, context);

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
            Packet PACKET = this->INCOMING;

            /* Log entry */
            debug::log(1, FUNCTION, "MinerLLP: ProcessPacket from ", GetAddress().ToStringIP(),
                       " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                       " length=", PACKET.LENGTH);

            /* Handle block-related packets that require stateful block management */
            /* These are handled directly here instead of through StatelessMiner */
            const uint8_t GET_BLOCK = 129;
            const uint8_t GET_HEIGHT = 130;
            const uint8_t GET_REWARD = 131;
            const uint8_t GET_ROUND = 133;
            const uint8_t SUBMIT_BLOCK = 1;
            const uint8_t BLOCK_DATA = 0;
            const uint8_t BLOCK_HEIGHT = 2;
            const uint8_t BLOCK_REWARD = 4;
            const uint8_t BLOCK_ACCEPTED = 200;
            const uint8_t BLOCK_REJECTED = 201;
            const uint8_t NEW_ROUND = 204;
            const uint8_t OLD_ROUND = 205;
            
            /* Authentication packet types */
            const uint8_t MINER_AUTH_INIT = 207;
            const uint8_t MINER_AUTH_RESPONSE = 209;
            const uint8_t MINER_AUTH_RESULT = 210;
            
            /* Block rejection reason codes (PR #122: Falcon Protocol Integration) */
            const uint8_t REJECT_PHYSICAL_SIGNATURE_FAILED = 0x10;  // Physical Falcon signature verification failed
            const uint8_t REJECT_KEY_BONDING_VIOLATION = 0x11;      // Key bonding violation (version mismatch)

            LOCK(MUTEX);

            /* Handle GET_BLOCK - requires authentication and channel */
            if(PACKET.HEADER == GET_BLOCK)
            {
                debug::log(0, "📥 === GET_BLOCK REQUEST ===");
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Authenticated: ", (context.fAuthenticated ? "YES" : "NO"));
                debug::log(0, "   Channel: ", context.nChannel);
                debug::log(0, "   Session ID: ", context.nSessionId);
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error("   ❌ Authentication required");
                    Packet response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    response.LENGTH = 1;
                    respond(response);
                    debug::log(0, "📥 === GET_BLOCK: REJECTED (AUTH) ===");
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error("   ❌ Channel not set");
                    debug::log(0, "📥 === GET_BLOCK: REJECTED (NO CHANNEL) ===");
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
                    Packet response(BLOCK_DATA);
                    response.LENGTH = 0;
                    respond(response);
                    debug::log(0, "📥 === GET_BLOCK: FAILED (NO BLOCK) ===");
                    return true;
                }
                
                debug::log(0, "   ✅ Block created successfully");
                debug::log(0, "      Height: ", pBlock->nHeight);
                debug::log(0, "      Channel: ", pBlock->nChannel);
                debug::log(0, "      Merkle root: ", pBlock->hashMerkleRoot.SubString());
                
                /* Note: Block is already stored in mapBlocks by new_block() */
                
                try {
                    debug::log(0, "   Serializing block...");
                    
                    /* Use block's Serialize() method - returns 216-byte mining template */
                    std::vector<uint8_t> vData = pBlock->Serialize();
                    
                    if(vData.empty())
                    {
                        debug::error("   ❌ Serialization returned empty vector!");
                        Packet response(BLOCK_DATA);
                        response.LENGTH = 0;
                        respond(response);
                        debug::log(0, "📥 === GET_BLOCK: FAILED (EMPTY SERIALIZATION) ===");
                        return true;
                    }
                    
                    debug::log(0, "   ✅ Serialized! Size: ", vData.size(), " bytes");
                    
                    /* Create response packet */
                    Packet response(BLOCK_DATA);
                    response.DATA = vData;
                    response.LENGTH = static_cast<uint32_t>(vData.size());  // ⭐ CRITICAL FIX!
                    
                    debug::log(0, "   📤 Sending BLOCK_DATA...");
                    debug::log(0, "      Packet header: ", (uint32_t)response.HEADER);
                    debug::log(0, "      Packet LENGTH field: ", response.LENGTH);
                    debug::log(0, "      Packet DATA size: ", response.DATA.size());
                    
                    /* Send the response */
                    respond(response);
                    
                    debug::log(0, "   ✅ Packet sent!");
                    debug::log(0, "📥 === GET_BLOCK: SUCCESS ===");
                    
                    /* Update context timestamp and height */
                    context = context.WithTimestamp(runtime::unifiedtimestamp())
                                     .WithHeight(pBlock->nHeight);
                    
                    /* Update manager with new context after template served */
                    StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);
                    StatelessMinerManager::Get().IncrementTemplatesServed();
                    
                    return true;
                }
                catch(const std::exception& e) {
                    debug::error("   ❌ Serialization exception: ", e.what());
                    debug::log(0, "📥 === GET_BLOCK: EXCEPTION ===");
                    
                    Packet response(BLOCK_DATA);
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
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK received ===", ANSI_COLOR_RESET);
                
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "❌ Authentication required");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (AUTH) ===", ANSI_COLOR_RESET);
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "❌ Channel not set");
                    Packet response(BLOCK_REJECTED);
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
                    
                    Packet response(BLOCK_REJECTED);
                    response.DATA.push_back(0x0C);  // Reason: Encryption required
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (ChaCha20 encryption required) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* Training Wheels Diagnostic Mode */
                debug::log(0, "════════════════════════════════════════════════════════");
                debug::log(0, "🚀 SUBMIT_BLOCK DIAGNOSTIC (Training Wheels Mode)");
                debug::log(0, "════════════════════════════════════════════════════════");
                
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
                
                debug::log(0, "════════════════════════════════════════════════════════");
                
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
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                if(PACKET.DATA.size() > MAX_SIZE)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK packet too large: ",
                               PACKET.DATA.size(), " > ", MAX_SIZE);
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                uint512_t hashMerkle;
                uint64_t nonce = 0;
                bool fFullBlockFormat = false;
                bool fFalconVerified = false;

                /* Check for Falcon-signed format: [merkle][nonce][timestamp][sig_len][signature] */
                /* Minimum for Falcon format: 64 + 8 + 8 + 2 = 82 bytes */
                const size_t FALCON_MIN_SIZE = FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN;

                if(PACKET.DATA.size() >= FALCON_MIN_SIZE)
                {
                    /* Check if Falcon wrapper is available */
                    if(!m_pFalconWrapper)
                    {
                        debug::error(FUNCTION, "❌ CRITICAL: Falcon wrapper not initialized");
                        debug::error(FUNCTION, "   Cannot verify Falcon signatures - check constructor logs");
                        debug::error(FUNCTION, "   Packet size suggests Falcon format but wrapper unavailable");
                        debug::error(FUNCTION, "   This should not happen in production - investigate immediately");
                        debug::error(FUNCTION, "");
                        debug::error(FUNCTION, "   ⚠️  SECURITY WARNING: Falling back to legacy format (INSECURE)");
                        debug::error(FUNCTION, "   ⚠️  In production, this node should reject Falcon packets");
                        debug::error(FUNCTION, "   ⚠️  Consider blocking submissions until wrapper is fixed");
                        
                        /* Fall through to legacy format processing */
                        /* NOTE: This fallback is for compatibility/debugging only. */
                        /* Production nodes should reject packets if wrapper fails. */
                    }
                    else
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
                            
                            Packet response(BLOCK_REJECTED);
                            response.DATA.push_back(0x0D);  // Reason: No session key
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
                                    debug::error(FUNCTION, "❌ ChaCha20 decryption FAILED");
                                    debug::error(FUNCTION, "   Possible causes:");
                                    debug::error(FUNCTION, "   - Corrupted ciphertext during transmission");
                                    debug::error(FUNCTION, "   - Wrong decryption key (session key mismatch)");
                                    debug::error(FUNCTION, "   - Authentication tag verification failed");
                                    
                                    Packet response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0B);  // Reason: ChaCha20 decryption failure
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
                                
                                /* Format: [block(variable)][timestamp(8)][sig_len(2)][signature(variable)] */
                                /* Read sig_len to determine where signature starts */
                                /* Minimum size: timestamp(8) + sig_len(2) = 10 bytes */
                                const size_t MIN_METADATA_SIZE = FalconConstants::TIMESTAMP_SIZE + FalconConstants::LENGTH_FIELD_SIZE;
                                
                                if(decryptedData.size() < MIN_METADATA_SIZE) {
                                    debug::error(FUNCTION, "❌ Decrypted payload too small (need at least ", MIN_METADATA_SIZE, " bytes)");
                                    Packet response(BLOCK_REJECTED);
                                    respond(response);
                                    return true;
                                }
                                
                                /* Determine block size dynamically */
                                /* Format: [block(variable)][timestamp(8)][sig_len(2)][signature(variable)] */
                                /* Strategy: Work backwards from the end to find sig_len */
                                /* We know: totalSize = blockSize + timestamp + sig_len_field + sigLen */
                                /* Therefore: blockSize = totalSize - timestamp - sig_len_field - sigLen */
                                
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
                                    if(decryptedData.size() < testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                              FalconConstants::LENGTH_FIELD_SIZE + 
                                                              FalconConstants::FALCON512_SIG_MIN)
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
                                    
                                    if(testSigLen >= FalconConstants::FALCON512_SIG_MIN && 
                                       testSigLen <= FalconConstants::FALCON512_SIG_MAX_VALIDATION &&
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
                                if(!fFoundValidSize && decryptedData.size() >= MIN_METADATA_SIZE + FalconConstants::FALCON512_SIG_MIN)
                                {
                                    /* Try reading sig_len from various positions, working backwards from end
                                     * Format: [block(B)][timestamp(8)][siglen(2)][sig(S)][physiglen(2)][physical_sig(P)]
                                     * Total = B + 8 + 2 + S + 2 + P
                                     * We try common values for S and check if the packet structure makes sense */
                                    const uint16_t commonSigSizes[] = {
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
                                         * testBlockSize = Total - timestamp - disposable_siglen_field - sig - physical_siglen_field */
                                        const size_t disposableSigLenFieldSize = FalconConstants::LENGTH_FIELD_SIZE;
                                        const size_t physicalSigLenFieldSize = FalconConstants::LENGTH_FIELD_SIZE;
                                        
                                        size_t testBlockSize = decryptedData.size() - FalconConstants::TIMESTAMP_SIZE - 
                                                               disposableSigLenFieldSize - testSigLen -
                                                               physicalSigLenFieldSize;
                                        
                                        /* Read physical signature length to check if physical signature is present */
                                        size_t physSigLenOffset = testBlockSize + FalconConstants::TIMESTAMP_SIZE + 
                                                                 disposableSigLenFieldSize + testSigLen;
                                        
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
                                           actualSigLen >= FalconConstants::FALCON512_SIG_MIN &&
                                           actualSigLen <= FalconConstants::FALCON512_SIG_MAX_VALIDATION)
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
                                    Packet response(BLOCK_REJECTED);
                                    respond(response);
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
                                    Packet response(BLOCK_REJECTED);
                                    respond(response);
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
                                    Packet response(BLOCK_REJECTED);
                                    respond(response);
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
                                    
                                    Packet response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x0C);  // Reason: Signature verification failed
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Signature verification failed) ===", ANSI_COLOR_RESET);
                                    return true;
                                }
                                
                                debug::log(0, "   Status: ✅ VERIFIED");
                                debug::log(0, "   Timestamp: ", nTimestamp);
                                debug::log(0, "   Merkle: ", hashMerkleFromBlock.SubString());
                                debug::log(0, "   Nonce: 0x", std::hex, nonceFromBlock, std::dec);
                                debug::log(0, "════════════════════════════════════════════════════════");
                                
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
                                            
                                            Packet response(BLOCK_REJECTED);
                                            response.DATA.push_back(REJECT_PHYSICAL_SIGNATURE_FAILED);
                                            respond(response);
                                            
                                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Physical signature failed) ===", ANSI_COLOR_RESET);
                                            return true;
                                        }
                                        
                                        /* Validate size matches session version (key bonding enforcement) */
                                        if(!context.fFalconVersionDetected)
                                        {
                                            debug::error(FUNCTION, "❌ No Falcon version detected for session");
                                            Packet response(BLOCK_REJECTED);
                                            respond(response);
                                            return true;
                                        }
                                        
                                        size_t expectedPhysicalSize = LLC::FalconConstants::GetSignatureCTSize(context.nFalconVersion);
                                        if(vchPhysicalSignature.size() != expectedPhysicalSize)
                                        {
                                            debug::error(FUNCTION, "❌ Physical signature size mismatch (key bonding violation)");
                                            debug::error(FUNCTION, "   Expected: ", expectedPhysicalSize, " (Falcon-",
                                                       (context.nFalconVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024"), ")");
                                            debug::error(FUNCTION, "   Got: ", vchPhysicalSignature.size());
                                            
                                            Packet response(BLOCK_REJECTED);
                                            response.DATA.push_back(REJECT_KEY_BONDING_VIOLATION);
                                            respond(response);
                                            
                                            debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Key bonding violation) ===", ANSI_COLOR_RESET);
                                            return true;
                                        }
                                        
                                        fHasPhysical = true;
                                        
                                        debug::log(1, FUNCTION, "Physical Falcon-",
                                                  (context.nFalconVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024"),
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
                                    
                                    Packet response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x08);  // Reason: stale timestamp
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Stale Falcon signature) ===", ANSI_COLOR_RESET);
                                    return true;
                                }
                            }
                            else
                            {
                                /* Packet too small for Falcon-signed full block format */
                                debug::error(FUNCTION, "❌ Packet too small for Falcon-signed full block format");
                                debug::error(FUNCTION, "   Expected at least: ", 
                                           FalconConstants::FULL_BLOCK_TRITIUM_MIN + 
                                           FalconConstants::TIMESTAMP_SIZE + 
                                           FalconConstants::LENGTH_FIELD_SIZE, " bytes");
                                debug::error(FUNCTION, "   Got: ", PACKET.DATA.size(), " bytes");
                                
                                Packet response(BLOCK_REJECTED);
                                response.DATA.push_back(0x0E);  // Reason: Invalid packet size
                                respond(response);
                                
                                debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Invalid packet size) ===", ANSI_COLOR_RESET);
                                return true;
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
                    
                    Packet response(BLOCK_REJECTED);
                    response.DATA.push_back(0x0F);  // Reason: Packet too small
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
                    
                    Packet response(BLOCK_REJECTED);
                    response.DATA.push_back(0xFF);  // Reason: Internal error
                    respond(response);
                    
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Internal error) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* Continue with existing template lookup and validation... */
                StatelessMinerManager::Get().IncrementBlocksSubmitted();

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
                {
                    debug::error(FUNCTION, "❌ Template not found for merkle root: ", hashMerkle.SubString());
                    debug::error(FUNCTION, "   This can happen if:");
                    debug::error(FUNCTION, "   - Template expired (height changed)");
                    debug::error(FUNCTION, "   - Miner computed wrong merkle root");
                    debug::error(FUNCTION, "   - Miner mining stale template");
                    
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Unknown template) ===", ANSI_COLOR_RESET);
                    return true;
                }
                
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Found original template (wallet-signed)", ANSI_COLOR_RESET);

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkle))
                {
                    debug::error(FUNCTION, "❌ sign_block failed (nonce update failed)");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (sign_block failed) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkle))
                {
                    debug::error(FUNCTION, "❌ validate_block failed (network rejected or stale)");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (validate_block failed) ===", ANSI_COLOR_RESET);
                    return true;
                }

                /* Block accepted - track in manager */
                StatelessMinerManager::Get().IncrementBlocksAccepted();

                /* Generate an Accepted response. */
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✅ Block accepted by network!", ANSI_COLOR_RESET);
                debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK result=accepted merkle=", hashMerkle.SubString());
                
                /* Log signature configuration (PR #122) */
                debug::log(0, FUNCTION, "   [Disposable: Falcon-", 
                          (context.fFalconVersionDetected ? 
                           (context.nFalconVersion == LLC::FalconVersion::FALCON_512 ? "512" : "1024") : "Unknown"),
                          ", Physical: ", (context.fPhysicalFalconPresent ? "PRESENT" : "ABSENT"), "]");
                
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📥 === SUBMIT_BLOCK: SUCCESS ===", ANSI_COLOR_RESET);
                
                /* Get block for detailed logging (safe access since we know it exists) */
                auto it = mapBlocks.find(hashMerkle);
                if(it != mapBlocks.end() && it->second)
                {
                    TAO::Ledger::Block *pBlock = it->second;
                    debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   🎉 Block ", pBlock->nHeight, " accepted by Nexus network", ANSI_COLOR_RESET);
                    debug::log(0, "   Miner: ", GetAddress().ToStringIP());
                    debug::log(0, "   Channel: ", pBlock->nChannel, " (", (pBlock->nChannel == 1 ? "Prime" : "Hash"), ")");
                }
                
                Packet response(BLOCK_ACCEPTED);
                respond(response);

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp());

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);

                return true;
            }

            /* Handle GET_HEIGHT - requires authentication */
            if(PACKET.HEADER == GET_HEIGHT)
            {
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "GET_HEIGHT rejected - authentication required");
                    Packet response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    respond(response);
                    return true;
                }

                /* Get current blockchain height */
                uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();

                debug::log(2, FUNCTION, "GET_HEIGHT request from ", GetAddress().ToStringIP(),
                           " sessionId=", context.nSessionId,
                           " - responding with height ", nCurrentHeight + 1);

                /* Create the response packet with height (next block to mine, 4-byte little-endian) */
                Packet response(BLOCK_HEIGHT);
                response.DATA = convert::uint2bytes(nCurrentHeight + 1);
                response.LENGTH = static_cast<uint32_t>(response.DATA.size());
                
                respond(response);

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp())
                                 .WithHeight(nCurrentHeight);

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);

                return true;
            }

            /* Handle GET_REWARD - requires authentication and channel */
            if(PACKET.HEADER == GET_REWARD)
            {
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "GET_REWARD rejected - authentication required");
                    Packet response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    respond(response);
                    return true;
                }

                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::error(FUNCTION, "GET_REWARD rejected - channel not set");
                    debug::error(FUNCTION, "  Required flow: Auth → MINER_SET_REWARD → SET_CHANNEL → GET_REWARD");
                    
                    /* Send error response */
                    Packet response(BLOCK_REJECTED);
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
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                debug::log(2, FUNCTION, "Sending Coinbase Reward of ", nReward);

                /* Create the response packet with reward (8-byte little-endian) */
                Packet response(BLOCK_REWARD);
                response.DATA = convert::uint2bytes64(nReward);
                response.LENGTH = static_cast<uint32_t>(response.DATA.size());
                
                respond(response);

                /* Update context timestamp */
                context = context.WithTimestamp(runtime::unifiedtimestamp());

                /* Update manager with new context */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);

                return true;
            }

            /* Handle GET_ROUND - requires authentication */
            if(PACKET.HEADER == GET_ROUND)
            {
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "GET_ROUND rejected - authentication required");
                    Packet response(MINER_AUTH_RESULT);
                    response.DATA.push_back(0x00);  // Failure
                    respond(response);
                    return true;
                }

                debug::log(2, FUNCTION, "GET_ROUND request from ", GetAddress().ToStringIP(),
                           " sessionId=", context.nSessionId);

                /* Check if blockchain height has changed (indicating new round) */
                uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();
                bool fNewRound = (nCurrentHeight != context.nHeight);

                /* Respond with appropriate round status */
                Packet response;
                if(fNewRound)
                {
                    response.HEADER = NEW_ROUND;
                    debug::log(2, FUNCTION, "Responding NEW_ROUND (height changed from ",
                               context.nHeight, " to ", nCurrentHeight, ")");
                }
                else
                {
                    response.HEADER = OLD_ROUND;
                    debug::log(3, FUNCTION, "Responding OLD_ROUND (height still ", nCurrentHeight, ")");
                }
                
                response.LENGTH = 0;
                respond(response);

                /* Update context with current height if changed */
                if(fNewRound)
                {
                    context = context.WithHeight(nCurrentHeight)
                                     .WithTimestamp(runtime::unifiedtimestamp());
                    StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);
                }

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

                /* Update manager with new context after successful packet processing */
                StatelessMinerManager::Get().UpdateMiner(context.strAddress, context);
                
                /* Log session registration for auth packets */
                if(PACKET.HEADER == MINER_AUTH_RESPONSE && context.fAuthenticated)
                {
                    debug::log(0, FUNCTION, "Session registered: address=", context.strAddress,
                               " sessionId=", context.nSessionId, " keyID=", context.hashKeyID.SubString());
                }
                
                /* Send response if present */
                if(!result.response.IsNull())
                {
                    respond(result.response);
                }
            }
            else
            {
                /* Log error and send failure response to miner for graceful handling */
                debug::log(0, FUNCTION, "MinerLLP: Processing error: ", result.strError);
                
                /* Send generic error response based on packet type */
                /* This allows miner to handle errors gracefully instead of timing out */
                
                Packet errorResponse;
                
                /* Send appropriate error response based on what was requested */
                if(PACKET.HEADER == MINER_AUTH_INIT || PACKET.HEADER == MINER_AUTH_RESPONSE)
                {
                    errorResponse.HEADER = MINER_AUTH_RESULT;
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
                Packet errorResponse;
                errorResponse. HEADER = 210;  // MINER_AUTH_RESULT
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


    /** Send a packet response */
    void StatelessMinerConnection::respond(const Packet& packet)
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
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Request from ", GetAddress().ToStringIP(), " ===", ANSI_COLOR_RESET);
        
        /* Get current height for cleanup */
        uint32_t nCurrentHeight = TAO::Ledger::ChainState::nBestHeight.load();
        
        /* Cleanup old templates when height advances */
        debug::log(2, FUNCTION, "Cleaning up stale templates (current height: ", nCurrentHeight, ")");
        for (auto it = mapBlocks.begin(); it != mapBlocks.end(); )
        {
            /* Check for null pointer before accessing */
            if (it->second && it->second->nHeight < nCurrentHeight)
            {
                debug::log(2, FUNCTION, "   Removing stale template at height ", it->second->nHeight,
                          " (merkle: ", it->first.SubString(), ")");
                delete it->second;
                it = mapBlocks.erase(it);
            }
            else
            {
                ++it;
            }
        }
        debug::log(2, FUNCTION, "   Templates remaining in map: ", mapBlocks.size());
        
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
            ++nAttempts;
            pBlock = TAO::Ledger::CreateBlockForStatelessMining(
                context.nChannel,
                ++nBlockIterator,
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
        
        /* Store new template in map (wallet signature is already set by CreateBlockForStatelessMining) */
        mapBlocks[pBlock->hashMerkleRoot] = pBlock;
        debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Template stored in map", ANSI_COLOR_RESET);
        debug::log(0, "      Merkle root: ", pBlock->hashMerkleRoot.SubString());
        debug::log(0, "      Height: ", pBlock->nHeight);
        debug::log(0, "      Templates in map: ", mapBlocks.size());
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Complete ===", ANSI_COLOR_RESET);
        return pBlock;
    }


    /** Sign a block */
    bool StatelessMinerConnection::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
    {
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📝 === SIGN_BLOCK: Updating template with miner's nonce ===", ANSI_COLOR_RESET);
        
        /* Safe map access to avoid creating null entry */
        auto it = mapBlocks.find(hashMerkleRoot);
        if(it == mapBlocks.end() || !it->second)
        {
            return debug::error(FUNCTION, "Block not found in map for merkle root: ", hashMerkleRoot.SubString());
        }
        
        TAO::Ledger::Block *pBaseBlock = it->second;

        /* Update block with the nonce and time. */
        pBaseBlock->nNonce = nNonce;

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
        if(it == mapBlocks.end() || !it->second)
        {
            debug::error(FUNCTION, "Block not found in map");
            return false;
        }
        
        /* If the block dynamically casts to a tritium block, validate the tritium block. */
        TAO::Ledger::TritiumBlock *pBlock = dynamic_cast<TAO::Ledger::TritiumBlock*>(it->second);
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
                debug::log(0, "   Common failure reasons:");
                debug::log(0, "   - Prime: Empty vOffsets or invalid Cunningham chain");
                debug::log(0, "   - Hash: Proof hash doesn't meet target");
                debug::log(0, "   - Producer transaction invalid");
                debug::log(0, "   - Block structure malformed");
                
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

            /* Check if the block is stale. */
            if(pBlock->hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
                return debug::error(FUNCTION, "submitted block is stale");

            /* Unlock sigchain to create new block. */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));

            /* Process the block and relay to network if it gets accepted into main chain. */
            debug::log(0, "");
            debug::log(0, "🌐 CALLING TAO::Ledger::Process()...");
            debug::log(0, "   This will:");
            debug::log(0, "   1. Validate against full consensus rules");
            debug::log(0, "   2. Add block to blockchain database");
            debug::log(0, "   3. Relay block to network peers");
            debug::log(0, "");
            
            uint8_t nStatus = 0;
            TAO::Ledger::Process(*pBlock, nStatus);

            /* Check the statues. */
            if(!(nStatus & TAO::Ledger::PROCESS::ACCEPTED))
            {
                debug::log(0, ANSI_COLOR_BRIGHT_RED, "❌ Process() REJECTED BLOCK", ANSI_COLOR_RESET);
                debug::log(0, "   Status flags: 0x", std::hex, static_cast<int>(nStatus), std::dec);
                return false;
            }
            
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "✅ Process() ACCEPTED BLOCK", ANSI_COLOR_RESET);
            debug::log(0, "   Block added to blockchain");
            debug::log(0, "   Block relayed to network");
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "✅ === VALIDATE_BLOCK: SUCCESS ===", ANSI_COLOR_RESET);

            return true;
        }

        /* If we get here, the block is null or doesn't exist. */
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
        /* Delete all the blocks in the map. */
        for(auto& pair : mapBlocks)
            delete pair.second;

        /* Clear the map. */
        mapBlocks.clear();
    }

} // namespace LLP
