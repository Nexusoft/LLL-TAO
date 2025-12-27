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

namespace LLP
{
    /* The block iterator to act as extra nonce. */
    std::atomic<uint32_t> StatelessMinerConnection::nBlockIterator(0);
    /** Default Constructor **/
    StatelessMinerConnection::StatelessMinerConnection()
    : Connection()
    , context()
    , MUTEX()
    {
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(const Socket& SOCKET_IN, DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(SOCKET_IN, DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    {
    }


    /** Constructor **/
    StatelessMinerConnection::StatelessMinerConnection(DDOS_Filter* DDOS_IN, bool fDDOSIn)
    : Connection(DDOS_IN, fDDOSIn)
    , context()
    , MUTEX()
    {
    }


    /** Default Destructor **/
    StatelessMinerConnection::~StatelessMinerConnection()
    {
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
                
                /* Store the new block in the memory map of recent blocks being worked on. */
                mapBlocks[pBlock->hashMerkleRoot] = pBlock;
                
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
                /* Check authentication */
                if(!context.fAuthenticated)
                {
                    debug::error(FUNCTION, "SUBMIT_BLOCK rejected - authentication required");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }
                
                /* Check channel is set */
                if(context.nChannel == 0)
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK before channel set");
                    return debug::error(FUNCTION, "Channel not set");
                }

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

                /* Check for optional Falcon signature in extended format */
                /* Format: [merkle_root (64)][nonce (8)][sig_len (2)][signature (sig_len)] */
                if(PACKET.DATA.size() >= MIN_SIZE + 2)
                {
                    /* Parse signature length (2 bytes, little-endian) */
                    size_t nSigPos = MIN_SIZE;
                    uint16_t nSigLen = static_cast<uint16_t>(PACKET.DATA[nSigPos]) |
                                       (static_cast<uint16_t>(PACKET.DATA[nSigPos + 1]) << 8);

                    /* Process signature if present and valid length */
                    if(nSigLen > 0 && PACKET.DATA.size() >= MIN_SIZE + 2 + nSigLen)
                    {
                        /* Extract signature */
                        std::vector<uint8_t> vSignature(
                            PACKET.DATA.begin() + MIN_SIZE + 2,
                            PACKET.DATA.begin() + MIN_SIZE + 2 + nSigLen);

                        /* Build message to verify: merkle_root || nonce */
                        std::vector<uint8_t> vMessage;
                        vMessage.insert(vMessage.end(), 
                            PACKET.DATA.begin(), 
                            PACKET.DATA.begin() + MIN_SIZE);

                        /* Log that signature was provided for audit trail.
                         * The miner is already authenticated via challenge-response,
                         * so this signature provides additional non-repudiation. */
                        debug::log(2, FUNCTION, "SUBMIT_BLOCK includes Falcon signature, len=", nSigLen,
                                   " sessionId=", context.nSessionId,
                                   " keyId=", context.hashKeyID.SubString());
                    }
                }

                /* Track block submission in manager */
                StatelessMinerManager::Get().IncrementBlocksSubmitted();

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
                {
                    debug::log(2, FUNCTION, "Block not found in map");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkle))
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK sign_block failed");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkle))
                {
                    debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK validate_block failed");
                    Packet response(BLOCK_REJECTED);
                    respond(response);
                    return true;
                }

                /* Block accepted - track in manager */
                StatelessMinerManager::Get().IncrementBlocksAccepted();

                /* Generate an Accepted response. */
                debug::log(0, FUNCTION, "MinerLLP: SUBMIT_BLOCK result=accepted merkle=", hashMerkle.SubString());
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
        
        debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== NEW_BLOCK: Complete ===", ANSI_COLOR_RESET);
        return pBlock;
    }


    /** Sign a block */
    bool StatelessMinerConnection::sign_block(uint64_t nNonce, const uint512_t& hashMerkleRoot)
    {
        TAO::Ledger::Block *pBaseBlock = mapBlocks[hashMerkleRoot];

        /* Update block with the nonce and time. */
        if(pBaseBlock)
            pBaseBlock->nNonce = nNonce;

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
