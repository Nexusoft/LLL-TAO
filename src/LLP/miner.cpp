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
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/types/miner.h>
#include <LLP/templates/events.h>
#include <LLP/templates/ddos.h>
#include <LLP/types/tritium.h>


#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/process.h>
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
    , fStatelessMinerSession(false)
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
    , fStatelessMinerSession(false)
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
    , fStatelessMinerSession(false)
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

                    /* Debug output. */
                    debug::log(2, FUNCTION, "Sent Block ", hashBlock.SubString(), " to Worker.");
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
                    /* Allow localhost connections to proceed even without an API session */
                    if(GetAddress().ToStringIP() == "127.0.0.1")
                    {
                        /* Mark this as a stateless miner session for localhost */
                        fStatelessMinerSession.store(true);
                        
                        /* Log once at connection time with clear message */
                        debug::log(0, FUNCTION, "MinerLLP: Using stateless Miner session for localhost connection from ", 
                                   GetAddress().ToStringIP(), ":", GetAddress().GetPort(), 
                                   ". TAO API session not required.");
                        
                        /* Do not disconnect - allow localhost to continue */
                    }
                    else
                    {
                        /* Non-localhost connections require valid API session */
                        debug::warning(FUNCTION, "Miner Connection Failed: ", e.what(), " from ", GetAddress().ToStringIP(), ":", GetAddress().GetPort());
                        this->Disconnect();
                    }
                }

                return;
            }


            /* On Disconnect Event, Reduce the Connection Count for Daemon */
            case EVENTS::DISCONNECT:
            {
                /* Debut output. */
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
                debug::log(3, FUNCTION, "Disconnecting ", GetAddress().ToStringIP(), " (", strReason, ")");
                return;
            }
        }

    }


    /* Helper function to get packet name for logging */
    static std::string GetMinerPacketName(uint8_t header)
    {
        switch(header) {
            case 0:   return "BLOCK_DATA";
            case 1:   return "SUBMIT_BLOCK";
            case 2:   return "BLOCK_HEIGHT";
            case 3:   return "SET_CHANNEL";
            case 4:   return "BLOCK_REWARD";
            case 5:   return "SET_COINBASE";
            case 6:   return "GOOD_BLOCK";
            case 7:   return "ORPHAN_BLOCK";
            case 64:  return "CHECK_BLOCK";
            case 65:  return "SUBSCRIBE";
            case 129: return "GET_BLOCK";
            case 130: return "GET_HEIGHT";
            case 131: return "GET_REWARD";
            case 132: return "CLEAR_MAP";
            case 133: return "GET_ROUND";
            case 200: return "BLOCK_ACCEPTED";
            case 201: return "BLOCK_REJECTED";
            case 202: return "COINBASE_SET";
            case 203: return "COINBASE_FAIL";
            case 204: return "NEW_ROUND";
            case 205: return "OLD_ROUND";
            case 206: return "CHANNEL_ACK";
            case 207: return "MINER_AUTH_INIT";
            case 208: return "MINER_AUTH_CHALLENGE";
            case 209: return "MINER_AUTH_RESPONSE";
            case 210: return "MINER_AUTH_RESULT";
            case 211: return "SESSION_START";
            case 212: return "SESSION_KEEPALIVE";
            case 213: return "MINER_SET_REWARD";
            case 214: return "MINER_REWARD_RESULT";
            case 253: return "PING";
            case 254: return "CLOSE";
            default:  return "UNKNOWN";
        }
    }


    /* This function is necessary for a template LLP server. It handles a custom message system, interpreting from raw packets. */
    bool Miner::ProcessPacket()
    {
        try
        {
            /* Get the incoming packet. */
            Packet PACKET = this->INCOMING;

            /* Log entry with clear indication and packet details */
            debug::log(1, FUNCTION, "MinerLLP: ProcessPacket ENTRY from ", GetAddress().ToStringIP(),
                       " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                       " length=", PACKET.LENGTH);

            /* Top of function marker for diagnostics */
            debug::log(1, FUNCTION, "MinerLLP: !!! TOP OF FUNCTION after ENTRY for ", GetAddress().ToStringIP());

            /* Route to appropriate handler based on stateless flag and localhost IP */
            if(GetAddress().ToStringIP() == "127.0.0.1" && fStatelessMinerSession.load())
            {
                return ProcessPacketStateless(PACKET);
            }
            else
            {
                return ProcessPacketStateful(PACKET);
            }
        }
        catch(const std::exception& e)
        {
            debug::log(0, FUNCTION, "MinerLLP: EXCEPTION in ProcessPacket from ", GetAddress().ToStringIP(), 
                       " - what(): ", e.what());
            return debug::error(FUNCTION, "Exception in ProcessPacket");
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

        /* Evaluate the packet header to determine what to do. */
        switch(PACKET.HEADER)
        {
            /* Authentication: Miner sends Genesis + Falcon public key + label */
            case MINER_AUTH_INIT:
            {
                debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH_INIT from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH);

                /* Validate minimum packet size (32 for genesis + 2 + 2 = 36 bytes minimum) */
                if(PACKET.DATA.size() < 36)
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: packet too small");

                /* Extract genesis hash (32 bytes) */
                std::copy(PACKET.DATA.begin(), PACKET.DATA.begin() + 32, hashGenesis.begin());

                debug::log(0, FUNCTION, "ProcessMinerAuthInit : Genesis raw bytes: ", 
                           HexStr(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.begin() + 32)));
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : Genesis hex string: ", hashGenesis.ToString());
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : Genesis after SetHex: ", hashGenesis.ToString());
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : Genesis type byte: 0x", 
                           std::hex, static_cast<uint32_t>(hashGenesis.GetType()), std::dec);

                /* ═══════════════════════════════════════════════════════════════════════════
                 * DERIVE PRE-AUTH ChaCha20 KEY (genesis only - for decrypting wrapped pubkey)
                 * This key is derived BEFORE we have the nonce, using only domain + genesis.
                 * The miner uses this same derivation to wrap its public key.
                 * ═══════════════════════════════════════════════════════════════════════════ */
                static const std::string DOMAIN = "nexus-mining-chacha20-v1";
                
                std::vector<uint8_t> vPreAuthInput;
                vPreAuthInput.insert(vPreAuthInput.end(), DOMAIN.begin(), DOMAIN.end());
                vPreAuthInput.insert(vPreAuthInput.end(), hashGenesis.begin(), hashGenesis.end());
                // NOTE: NO NONCE - this is pre-auth key derivation for initial pubkey decryption
                
                uint256_t hashPreAuthKey = LLC::SK256(vPreAuthInput);
                std::vector<uint8_t> vPreAuthChaChaKey = hashPreAuthKey.GetBytes();

                debug::log(0, FUNCTION, "ProcessMinerAuthInit : ═══════════════════════════════════════════════════");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : PRE-AUTH KEY DERIVATION (genesis only):");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Domain: ", DOMAIN);
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Genesis: ", hashGenesis.ToString().substr(0, 32), "...");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Key (first 8 bytes): ", 
                           HexStr(std::vector<uint8_t>(vPreAuthChaChaKey.begin(), vPreAuthChaChaKey.begin() + 8)));
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : ═══════════════════════════════════════════════════");

                /* Parse pubkey_len (2 bytes, big-endian) */
                uint16_t nPubKeyLen = (static_cast<uint16_t>(PACKET.DATA[32]) << 8) | 
                                      static_cast<uint16_t>(PACKET.DATA[33]);

                /* Validate pubkey_len */
                if(nPubKeyLen == 0 || nPubKeyLen > 2048)
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: invalid pubkey_len ", nPubKeyLen);

                if(PACKET.DATA.size() < 34 + nPubKeyLen)
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: packet too small for pubkey");

                /* Extract (possibly encrypted) public key */
                std::vector<uint8_t> vReceivedPubKey(PACKET.DATA.begin() + 34, 
                                                      PACKET.DATA.begin() + 34 + nPubKeyLen);

                debug::log(0, FUNCTION, "ProcessMinerAuthInit : Received pubkey size: ", nPubKeyLen, " bytes");

                /* ═══════════════════════════════════════════════════════════════════════════
                 * CHECK IF PUBKEY IS CHACHA20 WRAPPED
                 * Wrapped pubkey format: nonce(12) + ciphertext(897) + tag(16) = 925 bytes
                 * Raw Falcon-512 pubkey: 897 bytes
                 * ═══════════════════════════════════════════════════════════════════════════ */
                const size_t FALCON512_RAW_PUBKEY_SIZE = 897;
                const size_t FALCON512_WRAPPED_PUBKEY_SIZE = 925;  // nonce(12) + ciphertext(897) + tag(16)
                
                if(nPubKeyLen == FALCON512_WRAPPED_PUBKEY_SIZE)
                {
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit : Pubkey is ChaCha20 wrapped (925 bytes)");
                    
                    /* Decrypt using pre-auth key */
                    std::vector<uint8_t> vDecryptedPubKey;
                    
                    /* Extract nonce (first 12 bytes) */
                    std::vector<uint8_t> vNonce(vReceivedPubKey.begin(), vReceivedPubKey.begin() + 12);
                    
                    /* Extract ciphertext (middle 897 bytes) */
                    std::vector<uint8_t> vCiphertext(vReceivedPubKey.begin() + 12, vReceivedPubKey.begin() + 12 + 897);
                    
                    /* Extract tag (last 16 bytes) */
                    std::vector<uint8_t> vTag(vReceivedPubKey.end() - 16, vReceivedPubKey.end());
                    
                    /* AAD for Falcon pubkey encryption */
                    std::vector<uint8_t> vAAD{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};
                    
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit : Decrypting wrapped pubkey:");
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Nonce (12 bytes): ", HexStr(vNonce));
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Ciphertext size: ", vCiphertext.size());
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Tag (16 bytes): ", HexStr(vTag));
                    
                    /* Decrypt with AAD */
                    if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vPreAuthChaChaKey, vNonce, vDecryptedPubKey, vAAD))
                    {
                        debug::error(FUNCTION, "ProcessMinerAuthInit : ChaCha20 decryption FAILED - genesis mismatch?");
                        debug::error(FUNCTION, "ProcessMinerAuthInit : Check that miner's genesis matches node's expected genesis");
                        std::vector<uint8_t> vFail(1, 0x00);
                        respond(MINER_AUTH_RESULT, vFail);
                        this->Disconnect();
                        return false;
                    }
                    
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit : ✓ ChaCha20 decryption SUCCESS");
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Decrypted pubkey size: ", vDecryptedPubKey.size());
                    
                    /* Validate decrypted pubkey size */
                    if(vDecryptedPubKey.size() != FALCON512_RAW_PUBKEY_SIZE)
                    {
                        debug::error(FUNCTION, "ProcessMinerAuthInit : Invalid decrypted pubkey size: ", 
                                    vDecryptedPubKey.size(), " (expected ", FALCON512_RAW_PUBKEY_SIZE, ")");
                        std::vector<uint8_t> vFail(1, 0x00);
                        respond(MINER_AUTH_RESULT, vFail);
                        this->Disconnect();
                        return false;
                    }
                    
                    vMinerPubKey = vDecryptedPubKey;
                }
                else if(nPubKeyLen == FALCON512_RAW_PUBKEY_SIZE)
                {
                    /* Plaintext pubkey - use as-is */
                    debug::log(0, FUNCTION, "ProcessMinerAuthInit : Pubkey is plaintext (897 bytes, no ChaCha20 wrapping)");
                    vMinerPubKey = vReceivedPubKey;
                }
                else
                {
                    /* Invalid pubkey size */
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: invalid pubkey size ", nPubKeyLen, 
                                       " (expected ", FALCON512_RAW_PUBKEY_SIZE, " or ", FALCON512_WRAPPED_PUBKEY_SIZE, ")");
                }

                /* Parse miner_id_len (2 bytes, big-endian) */
                size_t nMinerIdOffset = 34 + nPubKeyLen;
                uint16_t nMinerIdLen = (static_cast<uint16_t>(PACKET.DATA[nMinerIdOffset]) << 8) | 
                                       static_cast<uint16_t>(PACKET.DATA[nMinerIdOffset + 1]);

                /* Validate miner_id_len */
                if(nMinerIdLen > 256)
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: invalid miner_id_len ", nMinerIdLen);

                if(PACKET.DATA.size() < nMinerIdOffset + 2 + nMinerIdLen)
                    return debug::error(FUNCTION, "MINER_AUTH_INIT: packet too small for miner_id");

                /* Extract miner ID */
                if(nMinerIdLen > 0)
                    strMinerId.assign(PACKET.DATA.begin() + nMinerIdOffset + 2, 
                                     PACKET.DATA.begin() + nMinerIdOffset + 2 + nMinerIdLen);
                else
                    strMinerId = "<no-id>";

                /* Generate random nonce (32 bytes) for challenge */
                uint256_t nonce = LLC::GetRand256();
                vAuthNonce = nonce.GetBytes();

                /* ═══════════════════════════════════════════════════════════════════════════
                 * DERIVE ChaCha20 SESSION KEY (genesis only - shared secret)
                 * This key is derived from the genesis hash as a shared secret between miner and node.
                 * Both miner and node can derive the same key from the genesis hash alone.
                 * ═══════════════════════════════════════════════════════════════════════════ */
                std::vector<uint8_t> vKeyInput;
                vKeyInput.insert(vKeyInput.end(), DOMAIN.begin(), DOMAIN.end());
                vKeyInput.insert(vKeyInput.end(), hashGenesis.begin(), hashGenesis.end());
                
                uint256_t hashSessionKey = LLC::SK256(vKeyInput);
                vChaChaKey = hashSessionKey.GetBytes();
                fEncryptionReady = true;

                debug::log(0, FUNCTION, "ProcessMinerAuthInit : ═══════════════════════════════════════════════════");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : ChaCha20 SESSION KEY DERIVATION (genesis only):");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Domain: ", DOMAIN);
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Genesis: ", hashGenesis.ToString().substr(0, 32), "...");
                debug::log(0, FUNCTION, "ProcessMinerAuthInit :   Key (first 8 bytes): ", 
                           HexStr(std::vector<uint8_t>(vChaChaKey.begin(), vChaChaKey.begin() + 8)));
                debug::log(0, FUNCTION, "ProcessMinerAuthInit : ═══════════════════════════════════════════════════");

                /* Log the authentication init */
                debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH_INIT: genesis=", hashGenesis.ToString().substr(0, 16), 
                           "... miner=", strMinerId, " pubkey=", vMinerPubKey.size());

                /* Build MINER_AUTH_CHALLENGE response */
                std::vector<uint8_t> vResponse;
                uint16_t nNonceLen = static_cast<uint16_t>(vAuthNonce.size());
                vResponse.push_back(static_cast<uint8_t>(nNonceLen >> 8));
                vResponse.push_back(static_cast<uint8_t>(nNonceLen & 0xFF));
                vResponse.insert(vResponse.end(), vAuthNonce.begin(), vAuthNonce.end());

                /* Send challenge */
                respond(MINER_AUTH_CHALLENGE, vResponse);

                return true;
            }


            /* Authentication: Miner sends Falcon signature over nonce */
            case MINER_AUTH_RESPONSE:
            {
                debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH_RESPONSE from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH);

                /* Validate that we have nonce and pubkey */
                if(vAuthNonce.empty())
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: no nonce (MINER_AUTH_INIT not received)");
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                if(vMinerPubKey.empty())
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: no pubkey (MINER_AUTH_INIT not received)");
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                /* Validate minimum packet size (2 bytes for sig_len) */
                if(PACKET.DATA.size() < 2)
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: packet too small");
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                /* Parse sig_len (2 bytes, little-endian) */
                uint16_t nSigLen = static_cast<uint16_t>(PACKET.DATA[0]) |
                                   (static_cast<uint16_t>(PACKET.DATA[1]) << 8);

                /* Validate sig_len */
                if(nSigLen == 0 || nSigLen > FalconConstants::FALCON512_SIG_MAX_VALIDATION)
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: invalid sig_len ", nSigLen);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                if(PACKET.DATA.size() < 2 + nSigLen)
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: packet too small for signature");
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                /* Extract signature */
                std::vector<uint8_t> vSignature(PACKET.DATA.begin() + 2, PACKET.DATA.begin() + 2 + nSigLen);

                debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH_RESPONSE from ", GetAddress().ToStringIP(),
                           " sig_len=", nSigLen);

                /* Verify Falcon signature */
                LLC::FLKey flkey;
                if(!flkey.SetPubKey(vMinerPubKey))
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: invalid public key from ", GetAddress().ToStringIP());
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                if(!flkey.Verify(vAuthNonce, vSignature))
                {
                    debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH verification FAILED for miner_id=", 
                               strMinerId, " from ", GetAddress().ToStringIP());
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }

                /* Authentication succeeded */
                fMinerAuthenticated = true;
                
                /* Derive key ID from public key for session ID generation */
                FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
                if(!pAuth)
                {
                    debug::error(FUNCTION, "MINER_AUTH_RESPONSE: FalconAuth not available");
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    this->Disconnect();
                    return false;
                }
                
                uint256_t hashKeyID = pAuth->DeriveKeyId(vMinerPubKey);
                
                /* Derive session ID from key ID (lower 32 bits) */
                nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0) & 0xFFFFFFFF);
                
                debug::log(0, FUNCTION, "MinerLLP: MINER_AUTH success for miner_id=", strMinerId,
                           " keyID=", hashKeyID.SubString(), " sessionId=", nSessionId,
                           " from ", GetAddress().ToStringIP());

                /* ChaCha20 key was already derived in MINER_AUTH_INIT */
                debug::log(0, FUNCTION, "✓ ChaCha20 encryption ready (derived in MINER_AUTH_INIT)");

                /* Build success response with session ID */
                std::vector<uint8_t> vSuccess;
                vSuccess.push_back(0x01); // Success status
                
                // Append session ID (4 bytes, little-endian)
                vSuccess.push_back(nSessionId & 0xFF);
                vSuccess.push_back((nSessionId >> 8) & 0xFF);
                vSuccess.push_back((nSessionId >> 16) & 0xFF);
                vSuccess.push_back((nSessionId >> 24) & 0xFF);
                respond(MINER_AUTH_RESULT, vSuccess);

                return true;
            }


            /* Set the Mining Channel this Connection will Serve Blocks for. 
             *
             * PROTOCOL DESIGN NOTE:
             * This implementation supports backward compatibility with both old and new NexusMiner versions:
             * 
             * - Legacy format (NexusMiner v1.x): 4-byte little-endian payload using convert::bytes2uint()
             * - New format (NexusMiner v2.x+): 1-byte payload for efficiency and simplicity
             * 
             * The channel value indicates which Proof-of-Work algorithm to mine:
             * - Channel 1: Prime (prime number discovery)
             * - Channel 2: Hash (traditional hashing)
             * - Channel 0: Reserved for Proof of Stake (not valid for mining)
             * 
             * This backward compatibility ensures smooth transition as miners upgrade, preventing
             * protocol breakage during the rollout of the unified mining stack.
             */
            case SET_CHANNEL:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted SET_CHANNEL before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                /* Log when SET_CHANNEL case is hit for debugging */
                debug::log(0, FUNCTION, "MinerLLP: *** SET_CHANNEL CASE HIT (STATELESS) *** header=0x", 
                           std::hex, uint32_t(PACKET.HEADER), std::dec,
                           " length=", PACKET.LENGTH, " dataBytes=", PACKET.DATA.size());

                /* Parse channel in a backward-compatible way */
                uint32_t nChannelValue = 0;
                if(PACKET.DATA.size() == 1)
                {
                    /* Single-byte payload - interpret as channel value directly */
                    nChannelValue = static_cast<uint32_t>(PACKET.DATA[0]);
                    debug::log(2, FUNCTION, "SET_CHANNEL: parsed single-byte channel value = ", nChannelValue);
                }
                else if(PACKET.DATA.size() >= 4)
                {
                    /* 4-byte or larger payload - decode using existing method */
                    nChannelValue = convert::bytes2uint(PACKET.DATA);
                    debug::log(2, FUNCTION, "SET_CHANNEL: parsed multi-byte channel value = ", nChannelValue);
                }
                else
                {
                    /* Unexpected payload size */
                    return debug::error(FUNCTION, "SET_CHANNEL: unexpected payload size ", PACKET.DATA.size());
                }

                /* Store the parsed channel */
                nChannel = nChannelValue;

                switch (nChannel.load())
                {
                    case 1:
                        debug::log(0, FUNCTION, "Prime Channel Set for ", GetAddress().ToStringIP());
                        break;

                    case 2:
                        debug::log(0, FUNCTION, "Hash Channel Set for ", GetAddress().ToStringIP());
                        break;

                    /* Don't allow Mining LLP Requests for Proof of Stake, or any other Channel. */
                    default:
                        return debug::error(FUNCTION, "Invalid PoW Channel (", nChannel.load(), ")");
                }

                /* Add distinctive marker for easy grep in logs */
                debug::log(0, FUNCTION, "MinerLLP: ### CHANNEL_SET_MARKER (STATELESS) from ", GetAddress().ToStringIP(),
                           " channel=", nChannel.load());

                /* Do not send ACK in stateless mode yet - will be sent after subsequent commands */
                return true;
            }


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
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_ROUND before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                /* Flag indicating the current round is no longer valid or there is a new block */
                bool fNewRound = false;
                {
                    LOCK(MUTEX);
                    fNewRound = check_best_height();
                }

                /* If height was outdated, respond with old round, otherwise respond with a new round */
                if(fNewRound)
                    respond(NEW_ROUND);
                else
                    respond(OLD_ROUND);

                return true;
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
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted GET_BLOCK before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
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

                debug::log(2, FUNCTION, "GET_BLOCK request from ", GetAddress().ToStringIP());

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

                return true;
            }


            /* Submit a block using the merkle root as the key. */
            case SUBMIT_BLOCK:
            {
                /* Check authentication for stateless miners */
                if(!fMinerAuthenticated)
                {
                    debug::log(0, FUNCTION, "MinerLLP: Stateless miner attempted SUBMIT_BLOCK before authentication from ",
                               GetAddress().ToStringIP(), " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
                    std::vector<uint8_t> vFail(1, 0x00);
                    respond(MINER_AUTH_RESULT, vFail);
                    return debug::error(FUNCTION, "Authentication required for stateless miner commands");
                }

                debug::log(2, FUNCTION, "SUBMIT_BLOCK from ", GetAddress().ToStringIP(),
                           " size=", PACKET.DATA.size());

                /* Validate packet size using FalconConstants */
                const size_t MIN_SIZE = FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE;
                const size_t MAX_SIZE = FalconConstants::SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX;

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

                /* Get the merkle root (first 64 bytes). */
                hashMerkle.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE));

                /* Get the nonce (next 8 bytes) */
                nonce = convert::bytes2uint64(std::vector<uint8_t>(
                    PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE,
                    PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE));

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

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Generate an Accepted response. */
                respond(BLOCK_ACCEPTED);
                return true;
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


            /* Placeholder for SESSION_START - not fully implemented yet.
             *
             * PROTOCOL DESIGN NOTE (STATELESS MODE):
             * SESSION_START and SESSION_KEEPALIVE are defined in the protocol but not yet fully implemented.
             * These packet types are reserved for future session management features where:
             * - SESSION_START would initialize a persistent mining session with session ID
             * - SESSION_KEEPALIVE would maintain the session and detect disconnections
             * 
             * For now, these handlers acknowledge receipt without error to maintain forward compatibility.
             * This allows future NexusMiner versions to use these packets without breaking existing nodes.
             * 
             * Full implementation will come in a later PR when session management requirements are finalized.
             */
            case SESSION_START:
            {
                debug::log(0, FUNCTION, "MinerLLP: SESSION_START received from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH, " - placeholder handler (not fully implemented)");
                
                /* Validate packet has some data */
                if(PACKET.DATA.size() == 0)
                {
                    return debug::error(FUNCTION, "SESSION_START: empty packet from ", GetAddress().ToStringIP());
                }
                
                /* Log that we received the packet but haven't implemented full logic yet */
                debug::log(0, FUNCTION, "MinerLLP: SESSION_START recognized but full session management not implemented yet");
                
                /* For now, return true to acknowledge without error */
                return true;
            }


            /* Placeholder for SESSION_KEEPALIVE - not fully implemented yet. */
            case SESSION_KEEPALIVE:
            {
                debug::log(0, FUNCTION, "MinerLLP: SESSION_KEEPALIVE received from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH, " - placeholder handler (not fully implemented)");
                
                /* Log that we received the packet but haven't implemented full logic yet */
                debug::log(0, FUNCTION, "MinerLLP: SESSION_KEEPALIVE recognized but full session management not implemented yet");
                
                /* For now, return true to acknowledge without error */
                return true;
            }


            /* MINER_SET_REWARD: Miner sends reward address (encrypted) */
            case MINER_SET_REWARD:
            {
                debug::log(0, FUNCTION, "MinerLLP: MINER_SET_REWARD received from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH);

                /* Check authentication first */
                if(!fMinerAuthenticated)
                {
                    debug::error(FUNCTION, "MINER_SET_REWARD: not authenticated");
                    SendRewardResult(false, "Authentication required");
                    return false;
                }

                /* Check encryption is ready */
                if(!fEncryptionReady)
                {
                    debug::error(FUNCTION, "MINER_SET_REWARD: encryption not ready");
                    SendRewardResult(false, "Encryption not established");
                    return false;
                }

                /* Process the reward address */
                ProcessSetReward(PACKET.DATA);

                return true;
            }
        }

        /* Fallback for unknown commands - log and return error */
        debug::log(0, FUNCTION, "MinerLLP: COMMAND NOT FOUND (stateless) header=0x", std::hex, 
                   uint32_t(PACKET.HEADER), std::dec, " length=", PACKET.LENGTH);
        return debug::error(FUNCTION, "Command not found 0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
    }


    /* Handles packets from stateful miners with TAO API session. */
    bool Miner::ProcessPacketStateful(const Packet& PACKET)
    {
        /* Make sure the mining server has a connection. (skip check if running local testnet) */
        bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

        /* Total number of peer connections */
        uint16_t nConnections = (TRITIUM_SERVER ? TRITIUM_SERVER->GetConnectionCount() : 0);

        if(!fLocalTestnet && nConnections == 0)
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=NO_NETWORK fLocalTestnet=", fLocalTestnet, 
                       " nConnections=", nConnections, " from ", GetAddress().ToStringIP());
            return debug::error(FUNCTION, "No network connections.");
        }

        /* Special rule for testnet so we don't bloat the chain. */
        if(config::fTestNet.load() && TAO::Ledger::mempool.Size() == 0)
        {
            /* Log early exit for testnet empty mempool rule */
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=TESTNET_EMPTY_MEMPOOL from ", GetAddress().ToStringIP());
            
            /* Handle if on verbose=3. */
            if(config::nVerbose.load() >= 3)
                return debug::error(FUNCTION, "Cannot mine with no pending transactions for -testnet");

            return false;
        }

        /* No mining when synchronizing. */
        if(TAO::Ledger::ChainState::Synchronizing())
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=SYNCHRONIZING from ", GetAddress().ToStringIP());
            return debug::error(FUNCTION, "Cannot mine while ledger is synchronizing.");
        }

        /* No mining when wallet is locked */
        if(is_locked())
        {
            debug::log(0, FUNCTION, "MinerLLP: EARLY_EXIT reason=WALLET_LOCKED from ", GetAddress().ToStringIP());
            return debug::error(FUNCTION, "Cannot mine while wallet is locked.");
        }

        /* Obtain timelock and timestamp. */
        uint64_t nTimeLock = TAO::Ledger::CurrentBlockTimelock();
        uint64_t nTimeStamp = runtime::unifiedtimestamp();

        /* Print a message explaining how many minutes until timelock activation. */
        if(nTimeStamp < nTimeLock)
        {
            uint64_t nSeconds =  nTimeLock - nTimeStamp;

            if(nSeconds % 60 == 0)
                debug::log(0, FUNCTION, "Timelock ", TAO::Ledger::CurrentBlockVersion(), " activation in ", nSeconds / 60, " minutes. ");
        }

        /* Log that we've reached the switch statement (passed all early exit checks) */
        debug::log(0, FUNCTION, "MinerLLP: >>> REACHED_SWITCH with header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                   " length=", PACKET.LENGTH, " from ", GetAddress().ToStringIP());

        /* Evaluate the packet header to determine what to do. */
        switch(PACKET.HEADER)
        {
            /* Set the Mining Channel this Connection will Serve Blocks for. */
            case SET_CHANNEL:
            {
                /* Log when SET_CHANNEL case is hit for debugging */
                debug::log(0, FUNCTION, "*** SET_CHANNEL CASE HIT *** from ", GetAddress().ToStringIP(),
                           " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                           " length=", PACKET.LENGTH, " DATA.size()=", PACKET.DATA.size());

                /* Parse channel in a backward-compatible way */
                uint32_t nChannelValue = 0;
                if(PACKET.DATA.size() == 1)
                {
                    /* Single-byte payload - interpret as channel value directly */
                    nChannelValue = static_cast<uint32_t>(PACKET.DATA[0]);
                    debug::log(2, FUNCTION, "SET_CHANNEL: parsed single-byte channel value = ", nChannelValue);
                }
                else if(PACKET.DATA.size() >= 4)
                {
                    /* 4-byte or larger payload - decode using existing method */
                    nChannelValue = convert::bytes2uint(PACKET.DATA);
                    debug::log(2, FUNCTION, "SET_CHANNEL: parsed multi-byte channel value = ", nChannelValue);
                }
                else
                {
                    /* Unexpected payload size */
                    return debug::error(FUNCTION, "SET_CHANNEL: unexpected payload size ", PACKET.DATA.size(), 
                                      " from ", GetAddress().ToStringIP());
                }

                /* Store the parsed channel */
                nChannel = nChannelValue;

                switch (nChannel.load())
                {
                    case 1:
                    debug::log(0, FUNCTION, "Prime Channel Set for ", GetAddress().ToStringIP());
                    break;

                    case 2:
                    debug::log(0, FUNCTION, "Hash Channel Set for ", GetAddress().ToStringIP());
                    break;

                    /* Don't allow Mining LLP Requests for Proof of Stake, or any other Channel. */
                    default:
                    return debug::error(FUNCTION, "Invalid PoW Channel (", nChannel.load(), ") from ", GetAddress().ToStringIP());
                }
                
                /* Add distinctive marker for easy grep in logs */
                debug::log(0, FUNCTION, "MinerLLP: ### CHANNEL_SET_MARKER from ", GetAddress().ToStringIP(),
                           " channel=", nChannel.load());
                
                /* Send explicit ACK to client after successfully setting channel. */
                respond(CHANNEL_ACK);
                debug::log(2, FUNCTION, "Sent CHANNEL_ACK to ", GetAddress().ToStringIP());

                return true;
            }


            /* Return a Ping if Requested. */
            case PING:
            {
                debug::log(3, FUNCTION, "PING received from ", GetAddress().ToStringIP());
                respond(PING);
                return true;
            }


            /* Setup a coinbase reward for potentially many outputs. */
            case SET_COINBASE:
            {
                /* The maximum coinbase reward for a block. */
                uint64_t nMaxValue = TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::tStateBest.load(), nChannel.load(), 0);

                /* Make sure there is a coinbase reward. */
                if(nMaxValue == 0)
                {
                    respond(COINBASE_FAIL);
                    return debug::error(FUNCTION, "Invalid coinbase reward.");
                }

                /* Byte 0 is the number of records. */
                uint8_t nSize = PACKET.DATA[0];

                /* Bytes 1 - 8 is the Wallet Operator Fee for that Round. */
                uint64_t nWalletFee  = convert::bytes2uint64(PACKET.DATA, 1);

                /* Iterator offset for map deserialization. */
                uint32_t nIterator = 9;

                /* The map of outputs for this coinbase transaction. */
                std::map<std::string, uint64_t> vOutputs;

                /* Loop through every Record. */
                for(uint8_t nIndex = 0; nIndex < nSize; ++nIndex)
                {
                    /* Get the string length. */
                    uint32_t nLength = PACKET.DATA[nIterator];

                    /* Get the string address for coinbase output. */
                    std::vector<uint8_t> vAddress = std::vector<uint8_t>(
                                             PACKET.DATA.begin() + nIterator + 1,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength);

                    /* Get the value for the coinbase output. */
                    uint64_t nValue = convert::bytes2uint64(
                        std::vector<uint8_t>(PACKET.DATA.begin() + nIterator + 1 + nLength,
                                             PACKET.DATA.begin() + nIterator + 1 + nLength + 8));

                    /* Check value for coinbase output. */
                    if(nValue == 0 || nValue > nMaxValue)
                    {
                        respond(COINBASE_FAIL);
                        return debug::error(FUNCTION, "Invalid coinbase recipient reward.");
                    }

                    /* Get the string address. */
                    std::string strAddress = convert::bytes2string(vAddress);

                    /* Validate the address. Disconnect immediately if an invalid address is provided. */
                    uint256_t hashGenesis(strAddress);
                    if(!LLD::Ledger->HasFirst(hashGenesis))
                    {
                        respond(COINBASE_FAIL);
                        return debug::error(FUNCTION, "Invalid Tritium Address in Coinbase Tx: ", strAddress);
                    }

                    /* Add the transaction as an output. */
                    vOutputs[strAddress] = nValue;

                    /* Increment the iterator. */
                    nIterator += (nLength + 9);
                }

                /* Lock the coinbase transaction object. */
                LOCK(MUTEX);

                /* Update the coinbase transaction. */
                tCoinbaseTx = Legacy::Coinbase(vOutputs, nMaxValue, nWalletFee);

                /* Check the consistency of the coibase transaction. */
                if(!tCoinbaseTx.IsValid())
                {
                    tCoinbaseTx.Print();
                    tCoinbaseTx.SetNull();
                    respond(COINBASE_FAIL);
                    return debug::error(FUNCTION, "Invalid Coinbase Tx");
                }

                /* Send a coinbase set message. */
                respond(COINBASE_SET);

                /* Verbose output. */
                debug::log(2, FUNCTION, " Set Coinbase Reward of ", nMaxValue);
                if(config::GetArg("-verbose", 0 ) >= 3)
                    tCoinbaseTx.Print();

                return true;
            }


            /* Clear the Block Map if Requested by Client. */
            case CLEAR_MAP:
            {
                LOCK(MUTEX);
                clear_map();
                return true;
            }


            /* Respond to the miner with the new height. */
            case GET_HEIGHT:
            {
                {
                    /* Check the best height before responding. */
                    LOCK(MUTEX);
                    check_best_height();
                }

                debug::log(0, FUNCTION, "GET_HEIGHT request from ", GetAddress().ToStringIP(), " - responding with height ", nBestHeight + 1);

                /* Create the response packet and write. */
                respond(BLOCK_HEIGHT, convert::uint2bytes(nBestHeight + 1));

                return true;
            }


            /* Respond to a miner if it is a new round. */
            case GET_ROUND:
            {

                /* Flag indicating the current round is no longer valid or there is a new block */
                bool fNewRound = false;
                {
                    LOCK(MUTEX);
                    fNewRound = !check_round() || check_best_height();
                }

                /* If height was outdated, respond with old round, otherwise respond with a new round */
                if(fNewRound)
                    respond(NEW_ROUND);
                else
                    respond(OLD_ROUND);

                return true;
            }


            /* Respond with the block reward in a given round. */
            case GET_REWARD:
            {
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
                /* Don't allow mining llp requests for proof of stake channel */
                if(nChannel.load() == 0)
                    return debug::error(FUNCTION, "Cannot subscribe to Stake Channel from ", GetAddress().ToStringIP());

                /* Get the number of subscribed blocks. */
                nSubscribed = convert::bytes2uint(PACKET.DATA);

                /* Check for zero blocks. */
                if(nSubscribed.load() == 0)
                    return debug::error(FUNCTION, "No blocks subscribed from ", GetAddress().ToStringIP());

                /* Debug output. */
                debug::log(2, FUNCTION, "Subscribed to ", nSubscribed.load(), " Blocks from ", GetAddress().ToStringIP());
                return true;
            }


            /* Get a new block for the miner. */
            case GET_BLOCK:
            {
                debug::log(2, FUNCTION, "GET_BLOCK request from ", GetAddress().ToStringIP());

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

                return true;
            }


            /* Submit a block using the merkle root as the key. */
            case SUBMIT_BLOCK:
            {
                debug::log(2, FUNCTION, "SUBMIT_BLOCK from ", GetAddress().ToStringIP());

                uint512_t hashMerkle;
                uint64_t nonce = 0;

                /* Get the merkle root (first 64 bytes). */
                hashMerkle.SetBytes(std::vector<uint8_t>(PACKET.DATA.begin(), PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE));

                /* Get the nonce (next 8 bytes) */
                nonce = convert::bytes2uint64(std::vector<uint8_t>(
                    PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE,
                    PACKET.DATA.begin() + FalconConstants::MERKLE_ROOT_SIZE + FalconConstants::NONCE_SIZE));

                debug::log(3, FUNCTION, "Block merkle root: ", hashMerkle.SubString(), " nonce: ", nonce, " from ", GetAddress().ToStringIP());

                LOCK(MUTEX);

                /* Make sure the block was created by this mining server. */
                if(!find_block(hashMerkle))
                {
                    debug::log(2, FUNCTION, "Block not found in map from ", GetAddress().ToStringIP());
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in signing block. */
                if(!sign_block(nonce, hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Make sure there is no inconsistencies in validating block. */
                if(!validate_block(hashMerkle))
                {
                    respond(BLOCK_REJECTED);
                    return true;
                }

                /* Generate an Accepted response. */
                respond(BLOCK_ACCEPTED);
                return true;
            }


            /* Allows a client to check if a block is part of the main chain. */
            case CHECK_BLOCK:
            {
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


            /* Placeholder for SESSION_START - not fully implemented yet.
             *
             * PROTOCOL DESIGN NOTE (STATEFUL MODE):
             * SESSION_START and SESSION_KEEPALIVE are defined in the protocol but not yet fully implemented.
             * These packet types are reserved for future session management features where:
             * - SESSION_START would initialize a persistent mining session with session ID
             * - SESSION_KEEPALIVE would maintain the session and detect disconnections
             * 
             * For now, these handlers acknowledge receipt without error to maintain forward compatibility.
             * This allows future NexusMiner versions to use these packets without breaking existing nodes.
             * 
             * Full implementation will come in a later PR when session management requirements are finalized.
             */
            case SESSION_START:
            {
                debug::log(0, FUNCTION, "MinerLLP: SESSION_START received from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH, " - placeholder handler (not fully implemented)");
                
                /* Validate packet has some data */
                if(PACKET.DATA.size() == 0)
                {
                    return debug::error(FUNCTION, "SESSION_START: empty packet from ", GetAddress().ToStringIP());
                }
                
                /* Log that we received the packet but haven't implemented full logic yet */
                debug::log(0, FUNCTION, "MinerLLP: SESSION_START recognized but full session management not implemented yet");
                
                /* For now, return true to acknowledge without error */
                return true;
            }


            /* Placeholder for SESSION_KEEPALIVE - not fully implemented yet. */
            case SESSION_KEEPALIVE:
            {
                debug::log(0, FUNCTION, "MinerLLP: SESSION_KEEPALIVE received from ", GetAddress().ToStringIP(),
                           " length=", PACKET.LENGTH, " - placeholder handler (not fully implemented)");
                
                /* Log that we received the packet but haven't implemented full logic yet */
                debug::log(0, FUNCTION, "MinerLLP: SESSION_KEEPALIVE recognized but full session management not implemented yet");
                
                /* For now, return true to acknowledge without error */
                return true;
            }
        }

        /* Fallback for unknown commands - log and return error */
        debug::log(0, FUNCTION, "MinerLLP: COMMAND NOT FOUND from ", GetAddress().ToStringIP(),
                   " header=0x", std::hex, uint32_t(PACKET.HEADER), std::dec,
                   " length=", PACKET.LENGTH);
        return debug::error(FUNCTION, "Command not found 0x", std::hex, uint32_t(PACKET.HEADER), std::dec);
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
        /* Skip session-dependent checks for stateless miner sessions (localhost only). */
        if(fStatelessMinerSession.load())
            return true;

        /* Get the hash genesis. */
        const uint256_t hashGenesis = TAO::API::Authentication::Caller(); //no parameter goes to default session

        /* Read hashLast from hashGenesis' sigchain and also check mempool. */
        uint512_t hashLast;

        /* Check to see whether there are any new transactions in the mempool for the sig chain */
        if(TAO::Ledger::mempool.Has(hashGenesis))
        {
            /* Get the last hash of the last transaction created by the sig chain */
            LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL);

            /* Update nHashLast if it changed. */
            if(nHashLast != hashLast)
            {
                nHashLast = hashLast;

                clear_map();

                debug::log(2, FUNCTION, "Block producer will orphan new sig chain transactions, resetting blocks");

                return false;
            }
        }

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

            /* Skip session-dependent operations for stateless miner sessions (localhost only). */
            if(!fStatelessMinerSession.load())
            {
                /* Get our current genesis. */
                const uint256_t hashGenesis = TAO::API::Authentication::Caller(); //no parameter goes to default session

                //TODO: we want to pipe in the notifications processor event notifications

                /*

                // Wake up events processor and wait for a signal to guarantee added transactions won't orphan a mined block.
                if(TAO::API::Commands::Instance<TAO::API::Users>()->NOTIFICATIONS_PROCESSOR
                    && TAO::API::GetSessionManager().Has(0)
                    && TAO::API::GetSessionManager().Get(0, false).CanProcessNotifications())
                {
                    //Find the thread processing notifications for this user
                    TAO::API::NotificationsThread* pThread = TAO::API::Commands::Instance<TAO::API::Users>()->NOTIFICATIONS_PROCESSOR->FindThread(0);

                    if(pThread)
                    {
                        pThread->NotifyEvent();
                        WaitEvent();
                    }
                }

                */

                /* If we detected a block height change, update the cached last hash of the logged in sig chain.
                 * This is done AFTER the notifications processor has finished, in case it added new transactions to the mempool  */
                LLD::Ledger->ReadLast(hashGenesis, nHashLast, TAO::Ledger::FLAGS::MEMPOOL);
            }
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
        /* If the primemod flag is set, take the hash proof down to 1017-bit to maximize prime ratio as much as possible. */
        const uint32_t nBitMask =
            config::GetBoolArg(std::string("-primemod"), false) ? 0xFE000000 : 0x80000000;

        /* Unlock sigchain to create new block. */
        SecureString strPIN;
        RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));

        /* Get an instance of our credentials. */
        const auto& pCredentials =
            TAO::API::Authentication::Credentials();

        /* Allocate memory for the new block. */
        TAO::Ledger::TritiumBlock *pBlock = new TAO::Ledger::TritiumBlock();

        /* Determine reward address for stateless miners */
        uint256_t hashDynamicReward = 0;
        if(fStatelessMinerSession.load() && fRewardBound)
        {
            hashDynamicReward = hashRewardAddress;
            debug::log(0, FUNCTION, "Using reward address: ", hashDynamicReward.ToString().substr(0, 16), "...");
        }

        /* Create a new block and loop for prime channel if minimum bit target length isn't met */
        while(TAO::Ledger::CreateBlock(pCredentials, strPIN, nChannel.load(), *pBlock, ++nBlockIterator, &tCoinbaseTx, hashDynamicReward))
        {
            /* Break out of loop when block is ready for prime mod. */
            if(is_prime_mod(nBitMask, pBlock))
                break;
        }

        /* Output debug info and return the newly created block. */
        debug::log(2, FUNCTION, "Created new Tritium Block ", pBlock->ProofHash().SubString(), " nVersion=", pBlock->nVersion);
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


    /* Encrypts a payload using ChaCha20-Poly1305 AEAD cipher */
    std::vector<uint8_t> Miner::EncryptPayload(const std::vector<uint8_t>& vPlaintext)
    {
        /* Generate cryptographically secure random 12-byte nonce */
        std::vector<uint8_t> vNonce(12);
        
        /* Use secure random generator - get 96 bits (12 bytes) from two uint64_t values */
        uint64_t nRand1 = LLC::GetRand();
        uint64_t nRand2 = LLC::GetRand();
        
        std::memcpy(&vNonce[0], &nRand1, 8);
        std::memcpy(&vNonce[8], &nRand2, 4);

        /* Encrypt using ChaCha20-Poly1305 */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;

        if(!LLC::EncryptChaCha20Poly1305(vPlaintext, vChaChaKey, vNonce, vCiphertext, vTag))
        {
            debug::error(FUNCTION, "Failed to encrypt payload");
            return std::vector<uint8_t>();
        }

        /* Build response: nonce(12) + ciphertext + tag(16) */
        std::vector<uint8_t> vEncrypted;
        vEncrypted.insert(vEncrypted.end(), vNonce.begin(), vNonce.end());
        vEncrypted.insert(vEncrypted.end(), vCiphertext.begin(), vCiphertext.end());
        vEncrypted.insert(vEncrypted.end(), vTag.begin(), vTag.end());

        return vEncrypted;
    }


    /* Decrypts a payload using ChaCha20-Poly1305 AEAD cipher */
    bool Miner::DecryptPayload(const std::vector<uint8_t>& vEncrypted, std::vector<uint8_t>& vPlaintext)
    {
        /* Validate minimum size: nonce(12) + tag(16) = 28 bytes */
        if(vEncrypted.size() < 28)
        {
            debug::error(FUNCTION, "Encrypted payload too small: ", vEncrypted.size());
            return false;
        }

        /* Extract nonce (12 bytes) */
        std::vector<uint8_t> vNonce(vEncrypted.begin(), vEncrypted.begin() + 12);

        /* Extract tag (last 16 bytes) */
        std::vector<uint8_t> vTag(vEncrypted.end() - 16, vEncrypted.end());

        /* Extract ciphertext (middle portion) */
        std::vector<uint8_t> vCiphertext(vEncrypted.begin() + 12, vEncrypted.end() - 16);

        /* Decrypt */
        if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vChaChaKey, vNonce, vPlaintext))
        {
            debug::error(FUNCTION, "Failed to decrypt payload");
            return false;
        }

        return true;
    }


    /* Validates that a reward address exists on chain and is a valid NXS account */
    bool Miner::ValidateRewardAddress(const uint256_t& hashReward)
    {
        /* Check for zero address */
        if(hashReward == 0)
        {
            debug::error(FUNCTION, "Reward address cannot be zero");
            return false;
        }

        /* Check address exists on chain */
        TAO::Register::Object account;
        if(!LLD::Register->ReadObject(hashReward, account, TAO::Ledger::FLAGS::LOOKUP))
        {
            debug::error(FUNCTION, "Reward address not found on chain: ", hashReward.SubString());
            return false;
        }

        /* Parse the account object */
        if(!account.Parse())
        {
            debug::error(FUNCTION, "Failed to parse reward account object");
            return false;
        }

        /* Verify it's an ACCOUNT type (not TRUST, not TOKEN, etc.) */
        if(account.Standard() != TAO::Register::OBJECTS::ACCOUNT)
        {
            debug::error(FUNCTION, "Reward address is not an account type, got: ",
                        static_cast<uint32_t>(account.Standard()));
            return false;
        }

        /* Verify it's an NXS account (token = 0) */
        uint256_t hashToken = account.get<uint256_t>("token");
        if(hashToken != 0)
        {
            debug::error(FUNCTION, "Reward address is not an NXS account, token: ",
                        hashToken.SubString());
            return false;
        }

        debug::log(0, FUNCTION, "✓ Reward address validated: ", hashReward.ToString());
        debug::log(2, FUNCTION, "  Owner: ", account.hashOwner.SubString());

        return true;
    }


    /* Sends encrypted MINER_REWARD_RESULT packet to miner */
    void Miner::SendRewardResult(bool fSuccess, const std::string& strMessage)
    {
        /* Build the result packet */
        std::vector<uint8_t> vPayload;

        /* Status byte */
        vPayload.push_back(fSuccess ? 0x01 : 0x00);

        /* Message (only on failure) */
        if(!fSuccess && !strMessage.empty())
        {
            vPayload.push_back(static_cast<uint8_t>(strMessage.size()));
            vPayload.insert(vPayload.end(), strMessage.begin(), strMessage.end());
        }
        else
        {
            vPayload.push_back(0x00);  // No message
        }

        /* Encrypt and send */
        std::vector<uint8_t> vEncrypted = EncryptPayload(vPayload);

        respond(MINER_REWARD_RESULT, vEncrypted);

        if(fSuccess)
            debug::log(0, FUNCTION, "Sent MINER_REWARD_RESULT: SUCCESS");
        else
            debug::log(0, FUNCTION, "Sent MINER_REWARD_RESULT: FAILURE - ", strMessage);
    }


    /* Processes MINER_SET_REWARD packet from miner */
    void Miner::ProcessSetReward(const std::vector<uint8_t>& vPayload)
    {
        /* Decrypt the payload using established ChaCha20 key */
        std::vector<uint8_t> vDecrypted;
        if(!DecryptPayload(vPayload, vDecrypted))
        {
            debug::error(FUNCTION, "Failed to decrypt reward address payload");
            SendRewardResult(false, "Decryption failed");
            return;
        }

        /* Extract the reward address (32 bytes) */
        if(vDecrypted.size() < 32)
        {
            debug::error(FUNCTION, "Invalid reward address payload size: ", vDecrypted.size());
            SendRewardResult(false, "Invalid payload size");
            return;
        }

        uint256_t hashReward;
        std::copy(vDecrypted.begin(), vDecrypted.begin() + 32, hashReward.begin());

        debug::log(0, FUNCTION, "Received reward address (encrypted): ", hashReward.ToString());

        /* Validate the reward address */
        if(!ValidateRewardAddress(hashReward))
        {
            SendRewardResult(false, "Invalid reward address");
            return;
        }

        /* Cache the reward address for mining */
        hashRewardAddress = hashReward;
        fRewardBound = true;

        debug::log(0, FUNCTION, "✓ Reward address bound: ", hashReward.ToString());
        debug::log(0, FUNCTION, "Session cache updated:");
        debug::log(0, FUNCTION, "  Genesis (auth): ", hashGenesis.SubString());
        debug::log(0, FUNCTION, "  Reward address: ", hashReward.ToString());
        debug::log(0, FUNCTION, "  Encryption: ChaCha20 ready");

        /* Send success result (encrypted) */
        SendRewardResult(true, "");
    }

}
