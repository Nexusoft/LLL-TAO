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
#include <LLP/templates/events.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/prime.h>
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

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/convert.h>
#include <Util/include/config.h>

#include <chrono>

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
                debug::log(0, "   From: ", GetAddress().ToStringIP());
                debug::log(0, "   Session ID: 0x", std::hex, context.nSessionId, std::dec);
                debug::log(0, "   Packet size: ", PACKET.DATA.size(), " bytes");
                
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

                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Authentication valid", ANSI_COLOR_RESET);
                debug::log(0, "   Channel: ", context.nChannel);
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
                        debug::log(0, "   🔐 Attempting Falcon signature verification...");
                        
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
                        
                        if(vSessionPubKey.empty())
                        {
                            debug::log(0, ANSI_COLOR_YELLOW, "   ⚠ No Falcon pubkey stored for session - treating as legacy format", ANSI_COLOR_RESET);
                        }
                        else
                        {
                            /* Packet format: [full_block(216)][timestamp(8)][sig_len(2)][signature] */
                            /* The miner signed: [merkle(64)][nonce(8)][timestamp(8)] */
                            /* UnwrapWorkSubmission expects: [merkle(64)][nonce(8)][timestamp(8)][sig_len(2)][signature] */
                            
                            /* Extract merkle root and nonce from the full block */
                            if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_TRITIUM_SIZE + 
                                                     FalconConstants::TIMESTAMP_SIZE + 
                                                     FalconConstants::LENGTH_FIELD_SIZE)
                            {
                                debug::log(2, FUNCTION, "   Extracting merkle and nonce from full block format");
                                
                                /* Extract merkle root (64 bytes at offset 132) */
                                uint512_t hashMerkleFromBlock;
                                hashMerkleFromBlock.SetBytes(std::vector<uint8_t>(
                                    PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
                                    PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE
                                ));
                                
                                /* Extract nonce (8 bytes at offset 200 for Tritium) */
                                uint64_t nonceFromBlock = convert::bytes2uint64(std::vector<uint8_t>(
                                    PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET,
                                    PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE
                                ));
                                
                                debug::log(2, FUNCTION, "   Extracted merkle: ", hashMerkleFromBlock.SubString());
                                debug::log(2, FUNCTION, "   Extracted nonce: 0x", std::hex, nonceFromBlock, std::dec);
                                
                                /* Reconstruct the signed data that UnwrapWorkSubmission expects */
                                /* Format: [merkle(64)][nonce(8)][timestamp(8)][sig_len(2)][signature] */
                                std::vector<uint8_t> signedData;
                                
                                /* Add merkle root (64 bytes) */
                                std::vector<uint8_t> merkleBytes = hashMerkleFromBlock.GetBytes();
                                signedData.insert(signedData.end(), merkleBytes.begin(), merkleBytes.end());
                                
                                /* Add nonce (8 bytes, little-endian)
                                 * NOTE: Manual byte extraction is required because Falcon protocol uses little-endian,
                                 * while convert::uint2bytes64() uses big-endian. This matches SignedWorkSubmission::Deserialize() */
                                for(size_t i = 0; i < FalconConstants::NONCE_SIZE; ++i)
                                {
                                    signedData.push_back((nonceFromBlock >> (i * 8)) & 0xFF);
                                }
                                
                                /* Add timestamp + sig_len + signature (everything after the full block) */
                                signedData.insert(signedData.end(),
                                                PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_TRITIUM_SIZE,
                                                PACKET.DATA.end());
                                
                                debug::log(2, FUNCTION, "   Reconstructed signed data: ", signedData.size(), " bytes");
                                
                                /* Now unwrap with the correct format */
                                auto result = m_pFalconWrapper->UnwrapWorkSubmission(signedData, vSessionPubKey);
                                
                                if(result.fSuccess)
                                {
                                    debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✅ Falcon signature VERIFIED", ANSI_COLOR_RESET);
                                    debug::log(0, "      Key ID: ", result.hashKeyID.SubString());
                                    debug::log(0, "      Timestamp: ", result.submission.nTimestamp);
                                    debug::log(0, "      Merkle: ", result.submission.hashMerkleRoot.SubString());
                                    debug::log(0, "      Nonce: 0x", std::hex, result.submission.nNonce, std::dec);
                                    
                                    /* Verify merkle and nonce match what we extracted */
                                    if(result.submission.hashMerkleRoot != hashMerkleFromBlock)
                                    {
                                        debug::error(FUNCTION, "❌ Merkle root mismatch after signature verification!");
                                        debug::error(FUNCTION, "   From block:     ", hashMerkleFromBlock.SubString());
                                        debug::error(FUNCTION, "   From signature: ", result.submission.hashMerkleRoot.SubString());
                                        
                                        Packet response(BLOCK_REJECTED);
                                        response.DATA.push_back(0x0A);  // Data corruption
                                        respond(response);
                                        return true;
                                    }
                                    
                                    if(result.submission.nNonce != nonceFromBlock)
                                    {
                                        debug::error(FUNCTION, "❌ Nonce mismatch after signature verification!");
                                        debug::error(FUNCTION, "   From block:     0x", std::hex, nonceFromBlock, std::dec);
                                        debug::error(FUNCTION, "   From signature: 0x", std::hex, result.submission.nNonce, std::dec);
                                        
                                        Packet response(BLOCK_REJECTED);
                                        response.DATA.push_back(0x0A);  // Data corruption
                                        respond(response);
                                        return true;
                                    }
                                    
                                    /* SECURITY: Verify signature key matches authenticated session key */
                                    /* This prevents key substitution attacks where an attacker might try */
                                    /* to submit work signed with a different key than the authenticated one */
                                    /* NOTE: Current comparison is not constant-time, which could theoretically */
                                    /* leak information via timing attacks. Consider using constant-time comparison */
                                    /* if this becomes a concern (low priority since keyID is public). */
                                    if(result.hashKeyID != context.hashKeyID)
                                    {
                                        debug::error(FUNCTION, "❌ Falcon key ID mismatch - SECURITY VIOLATION");
                                        debug::error(FUNCTION, "   Signature key ID: ", result.hashKeyID.SubString());
                                        debug::error(FUNCTION, "   Session key ID:   ", context.hashKeyID.SubString());
                                        debug::error(FUNCTION, "   This prevents key substitution attacks");
                                        
                                        Packet response(BLOCK_REJECTED);
                                        response.DATA.push_back(0x09);  // Reason: key ID mismatch
                                        respond(response);
                                        
                                        debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Key ID mismatch) ===", ANSI_COLOR_RESET);
                                        return true;
                                    }
                                    
                                    debug::log(2, FUNCTION, "   ✓ Merkle and nonce validated (match block data)");
                                    debug::log(2, FUNCTION, "   ✓ Key ID validated (matches session key)");
                                    
                                    /* Signature verified! Use the verified values */
                                    hashMerkle = result.submission.hashMerkleRoot;
                                    nonce = result.submission.nNonce;
                                    fFalconVerified = true;
                                    
                                    /* Check timestamp freshness (replay protection) */
                                    uint64_t nCurrentTime = std::chrono::duration_cast<std::chrono::seconds>(
                                        std::chrono::system_clock::now().time_since_epoch()).count();
                                    
                                    int64_t nTimeDiff = std::abs(static_cast<int64_t>(nCurrentTime) - 
                                                                static_cast<int64_t>(result.submission.nTimestamp));
                                    
                                    /* Allow 30 second clock skew */
                                    if(nTimeDiff > 30)
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
                                    /* Signature verification failed */
                                    debug::error(FUNCTION, "❌ Falcon signature verification FAILED: ", result.strError);
                                    
                                    Packet response(BLOCK_REJECTED);
                                    response.DATA.push_back(0x07);  // Reason: invalid signature
                                    respond(response);
                                    
                                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "📥 === SUBMIT_BLOCK: REJECTED (Invalid Falcon signature) ===", ANSI_COLOR_RESET);
                                    return true;
                                }
                            }
                            else
                            {
                                debug::error(FUNCTION, "Packet too small for Falcon-signed full block format");
                            }
                        }
                    }
                }

                /* If Falcon verification didn't happen, extract using legacy format */
                if(!fFalconVerified)
                {
                    debug::log(0, "   📝 Processing as legacy format (no Falcon signature)");
                    
                    /* Detect format based on packet size:
                     * - Legacy 64-byte merkle format: 72-919 bytes
                     * - Full block format (Tritium): 216+ bytes
                     * - Full block format (Legacy): 220+ bytes
                     * 
                     * Detection strategy:
                     * If packet size >= SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD (200 bytes), assume full block format.
                     * This threshold is chosen because:
                     * - Legacy format max is ~919 bytes (with encryption)
                     * - Full block min is 216 bytes (Tritium without signature)
                     * - 200 bytes is a safe threshold that distinguishes the two
                     */
                    if(PACKET.DATA.size() >= FalconConstants::SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD)
                    {
                        fFullBlockFormat = true;
                        
                        /* Extract merkle root from offset 132 (after version + hashPrevBlock) */
                        if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE)
                        {
                            hashMerkle.SetBytes(std::vector<uint8_t>(
                                PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
                                PACKET.DATA.begin() + FalconConstants::FULL_BLOCK_MERKLE_OFFSET + FalconConstants::MERKLE_ROOT_SIZE));
                            
                            /* Extract nonce - determine offset based on block type (Tritium vs Legacy) */
                            if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET + FalconConstants::NONCE_SIZE)
                            {
                                /* Default to Tritium offset (200) */
                                size_t nonceOffset = FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET;
                                
                                /* Determine if this is Legacy based on size.
                                 * Legacy blocks are 220 bytes base, Tritium are 216 bytes base.
                                 * If packet size is in the range [220, 220+margin), it's likely Legacy.
                                 * Otherwise, assume Tritium (which is more common).
                                 * 
                                 * Note: This heuristic works because:
                                 * 1. Tritium packets start at 216 bytes, Legacy at 220 bytes
                                 * 2. With signatures, both grow beyond 220, but Legacy grows from 220
                                 * 3. The 100-byte margin gives us a detection window [220, 320)
                                 * 4. Tritium packets in this range would have started from 216, not 220
                                 * 
                                 * Edge case: If miner behavior changes to use different signature sizes,
                                 * this heuristic may need adjustment. The offsets are only 4 bytes apart,
                                 * so worst case is reading the nonce from wrong offset (will fail validation). */
                                if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_LEGACY_SIZE && 
                                   PACKET.DATA.size() < FalconConstants::FULL_BLOCK_LEGACY_SIZE + FalconConstants::FULL_BLOCK_TYPE_DETECTION_MARGIN)
                                {
                                    /* Likely Legacy format, use offset 204 */
                                    if(PACKET.DATA.size() >= FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET + FalconConstants::NONCE_SIZE)
                                    {
                                        nonceOffset = FalconConstants::FULL_BLOCK_LEGACY_NONCE_OFFSET;
                                    }
                                }
                                
                                nonce = convert::bytes2uint64(std::vector<uint8_t>(
                                    PACKET.DATA.begin() + nonceOffset,
                                    PACKET.DATA.begin() + nonceOffset + FalconConstants::NONCE_SIZE));
                                
                                debug::log(2, FUNCTION, "SUBMIT_BLOCK: Full block format detected, ",
                                           "merkle at offset 132, nonce at offset ", nonceOffset);
                            }
                            else
                            {
                                debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK full block format but packet too small for nonce");
                                Packet response(BLOCK_REJECTED);
                                respond(response);
                                return true;
                            }
                        }
                        else
                        {
                            debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK full block format but packet too small for merkle");
                            Packet response(BLOCK_REJECTED);
                            respond(response);
                            return true;
                        }
                    }
                    else
                    {
                        /* Legacy 64-byte merkle format */
                        fFullBlockFormat = false;
                        
                        /* Get the merkle root (first 64 bytes). */
                        hashMerkle.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE));

                        /* Get the nonce (next 8 bytes) */
                        nonce = convert::bytes2uint64(std::vector<uint8_t>(
                            PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE,
                            PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE));
                        
                        debug::log(3, FUNCTION, "SUBMIT_BLOCK: Legacy 64-byte merkle format");
                    }

                    debug::log(3, FUNCTION, "Block merkle root: ", hashMerkle.SubString(), " nonce: ", nonce,
                               " format: ", fFullBlockFormat ? "full_block" : "merkle_only");

                    debug::log(0, "   Extracted merkle root: ", hashMerkle.SubString());
                    debug::log(0, "   Extracted nonce: 0x", std::hex, nonce, std::dec);
                    debug::log(0, "   Format: ", fFullBlockFormat ? "full_block" : "merkle_only");
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

            /* Calculate prime offsets before validation. */
            TAO::Ledger::GetOffsets(pBlock->GetPrime(), pBlock->vOffsets);
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Prime offsets calculated", ANSI_COLOR_RESET);

            /* CRITICAL: DO NOT re-sign the block! 
             * The block was already wallet-signed during creation (new_block).
             * Re-signing here would overwrite the wallet signature and cause 
             * "sign block failed" errors because the miner's nonce is already set.
             * 
             * Per PR #104: Blocks MUST be wallet-signed during creation and 
             * that signature MUST be preserved when the miner submits a solution.
             * 
             * What we do here:
             *   1. Update nonce (already done above)
             *   2. Update timestamp (done above)
             *   3. Calculate prime offsets (done above)
             *   4. PRESERVE the original wallet signature (do nothing)
             */
            
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "   ✓ Wallet signature preserved (no re-signing)", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "📝 === SIGN_BLOCK: Complete ===", ANSI_COLOR_RESET);
            
            return true;
        }

        /* If we get here, the block is null or doesn't exist. */
        return debug::error(FUNCTION, "null block");
    }


    /** Validate a block */
    bool StatelessMinerConnection::validate_block(const uint512_t& hashMerkleRoot)
    {
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

            /* Check if the block is stale. */
            if(pBlock->hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
                return debug::error(FUNCTION, "submitted block is stale");

            /* Unlock sigchain to create new block. */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));

            /* Process the block and relay to network if it gets accepted into main chain. */
            uint8_t nStatus = 0;
            TAO::Ledger::Process(*pBlock, nStatus);

            /* Check the statues. */
            if(!(nStatus & TAO::Ledger::PROCESS::ACCEPTED))
                return false;

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
