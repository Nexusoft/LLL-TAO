/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>
#include <LLP/include/disposable_falcon.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/genesis_constants.h>
#include <LLP/include/stateless_manager.h>

#include <LLC/include/random.h>
#include <LLC/include/flkey.h>
#include <LLC/include/encrypt.h>
#include <LLC/hash/SK.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/config.h>
#include <Util/include/hex.h>

#include <sstream>
#include <iomanip>
#include <algorithm>

namespace LLP
{
    /* Default session timeout in seconds for mining sessions.
     * This is the inactivity timeout - sessions expire if no keepalive
     * is received within this window. Different from session_recovery.cpp
     * which uses a longer 1-hour timeout for recovery purposes. */
    static const uint64_t DEFAULT_SESSION_TIMEOUT = 300;

    /* Default constructor */
    MiningContext::MiningContext()
    : nChannel(0)
    , nHeight(0)
    , nTimestamp(0)
    , strAddress("")
    , nProtocolVersion(0)
    , fAuthenticated(false)
    , nSessionId(0)
    , hashKeyID(0)
    , hashGenesis(0)
    , strUserName("")
    , vAuthNonce()
    , vMinerPubKey()
    , nSessionStart(0)
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nKeepaliveCount(0)
    {
    }

    /* Parameterized constructor */
    MiningContext::MiningContext(
        uint32_t nChannel_,
        uint32_t nHeight_,
        uint64_t nTimestamp_,
        const std::string& strAddress_,
        uint32_t nProtocolVersion_,
        bool fAuthenticated_,
        uint32_t nSessionId_,
        const uint256_t& hashKeyID_,
        const uint256_t& hashGenesis_
    )
    : nChannel(nChannel_)
    , nHeight(nHeight_)
    , nTimestamp(nTimestamp_)
    , strAddress(strAddress_)
    , nProtocolVersion(nProtocolVersion_)
    , fAuthenticated(fAuthenticated_)
    , nSessionId(nSessionId_)
    , hashKeyID(hashKeyID_)
    , hashGenesis(hashGenesis_)
    , strUserName("")
    , vAuthNonce()
    , vMinerPubKey()
    , nSessionStart(0)
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nKeepaliveCount(0)
    {
    }

    /* Immutable update methods */
    MiningContext MiningContext::WithChannel(uint32_t nChannel_) const
    {
        MiningContext c = *this;
        c.nChannel = nChannel_;
        return c;
    }

    MiningContext MiningContext::WithHeight(uint32_t nHeight_) const
    {
        MiningContext c = *this;
        c.nHeight = nHeight_;
        return c;
    }

    MiningContext MiningContext::WithTimestamp(uint64_t nTimestamp_) const
    {
        MiningContext c = *this;
        c.nTimestamp = nTimestamp_;
        return c;
    }

    MiningContext MiningContext::WithAuth(bool fAuthenticated_) const
    {
        MiningContext c = *this;
        c.fAuthenticated = fAuthenticated_;
        return c;
    }

    MiningContext MiningContext::WithSession(uint32_t nSessionId_) const
    {
        MiningContext c = *this;
        c.nSessionId = nSessionId_;
        return c;
    }

    MiningContext MiningContext::WithKeyId(const uint256_t& hashKeyID_) const
    {
        MiningContext c = *this;
        c.hashKeyID = hashKeyID_;
        return c;
    }

    MiningContext MiningContext::WithGenesis(const uint256_t& hashGenesis_) const
    {
        MiningContext c = *this;
        c.hashGenesis = hashGenesis_;
        return c;
    }

    MiningContext MiningContext::WithUserName(const std::string& strUserName_) const
    {
        MiningContext c = *this;
        c.strUserName = strUserName_;
        return c;
    }

    MiningContext MiningContext::WithNonce(const std::vector<uint8_t>& vNonce_) const
    {
        MiningContext c = *this;
        c.vAuthNonce = vNonce_;
        return c;
    }

    MiningContext MiningContext::WithPubKey(const std::vector<uint8_t>& vPubKey_) const
    {
        MiningContext c = *this;
        c.vMinerPubKey = vPubKey_;
        return c;
    }

    MiningContext MiningContext::WithSessionStart(uint64_t nSessionStart_) const
    {
        MiningContext c = *this;
        c.nSessionStart = nSessionStart_;
        return c;
    }

    MiningContext MiningContext::WithSessionTimeout(uint64_t nSessionTimeout_) const
    {
        MiningContext c = *this;
        c.nSessionTimeout = nSessionTimeout_;
        return c;
    }

    MiningContext MiningContext::WithKeepaliveCount(uint32_t nKeepaliveCount_) const
    {
        MiningContext c = *this;
        c.nKeepaliveCount = nKeepaliveCount_;
        return c;
    }

    uint256_t MiningContext::GetPayoutAddress() const
    {
        /* Return explicit genesis if set */
        if(hashGenesis != 0)
            return hashGenesis;

        /* Username-based addressing (trust userName:default system)
         * The caller is responsible for resolving the username to a genesis hash
         * using TAO::API::Names. This method returns 0 to indicate that
         * resolution is needed.
         *
         * Usage pattern:
         * 1. Check if hashGenesis is set directly
         * 2. If not, check strUserName and resolve via Names API
         * 3. Use Names::ResolveAddress(strUserName + ":default") to get genesis
         */
        if(!strUserName.empty())
        {
            debug::log(3, FUNCTION, "Username '", strUserName,
                       "' set - caller should resolve via Names API");
        }

        return uint256_t(0);
    }

    bool MiningContext::HasValidPayout() const
    {
        return hashGenesis != 0 || !strUserName.empty();
    }

    bool MiningContext::IsSessionExpired(uint64_t nNow) const
    {
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Session never started means it's "expired" */
        if(nSessionStart == 0)
            return true;

        /* Check if time since last activity exceeds timeout.
         * This is an activity-based timeout (like keepalive):
         * - nTimestamp is updated on each keepalive/activity
         * - Session expires if no activity within nSessionTimeout */
        return (nNow - nTimestamp) > nSessionTimeout;
    }

    uint64_t MiningContext::GetSessionDuration(uint64_t nNow) const
    {
        /* Get current time if not provided */
        if(nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* No session started */
        if(nSessionStart == 0)
            return 0;

        /* Return time since session start */
        return nNow - nSessionStart;
    }


    /* ProcessResult private constructor */
    ProcessResult::ProcessResult(
        const MiningContext& ctx_,
        bool fSuccess_,
        const std::string& error_,
        const Packet& resp_
    )
    : context(ctx_)
    , fSuccess(fSuccess_)
    , strError(error_)
    , response(resp_)
    {
    }

    /* Static factory methods */
    ProcessResult ProcessResult::Success(const MiningContext& ctx, const Packet& resp)
    {
        return ProcessResult(ctx, true, "", resp);
    }

    ProcessResult ProcessResult::Error(const MiningContext& ctx, const std::string& error)
    {
        return ProcessResult(ctx, false, error, Packet());
    }


    /* Packet type definitions - must match miner.h and NexusMiner Phase 2 protocol */
    enum : Packet::message_t
    {
        /* Data packets */
        BLOCK_DATA           = 0,
        SUBMIT_BLOCK         = 1,
        SET_CHANNEL          = 3,

        /* Request packets */
        GET_BLOCK            = 129,

        /* Response packets */
        BLOCK_ACCEPTED       = 200,
        BLOCK_REJECTED       = 201,
        CHANNEL_ACK          = 206,

        /* Authentication packets - Phase 2 Unified Hybrid Protocol */
        MINER_AUTH_INIT      = 207,  // miner -> node, sends Falcon pubkey + label
        MINER_AUTH_CHALLENGE = 208,  // node -> miner, sends random nonce
        MINER_AUTH_RESPONSE  = 209,  // miner -> node, sends Falcon signature over nonce
        MINER_AUTH_RESULT    = 210,  // node -> miner, indicates success/fail

        /* Session management packets */
        SESSION_START        = 211,
        SESSION_KEEPALIVE    = 212,
    };


    /* Main packet processing entry point */
    ProcessResult StatelessMiner::ProcessPacket(
        const MiningContext& context,
        const Packet& packet
    )
    {
        /* DEBUG: Log incoming packet details for Falcon handshake debugging */
        debug::log(3, FUNCTION, "ProcessPacket: ", packet.DebugString());

        /* DEBUG: Log packet bytes when verbose >= 5 */
        if(config::nVerbose >= 5)
        {
            DisposableFalcon::DebugLogPacket("ProcessPacket::incoming", packet.DATA, 5);
        }

        /* Route based on packet type */
        /* Note: GET_BLOCK and SUBMIT_BLOCK are handled in StatelessMinerConnection */
        /* due to their need for stateful block management */
        switch(packet.HEADER)
        {
            case MINER_AUTH_INIT:
                debug::log(2, FUNCTION, "Routing to ProcessMinerAuthInit");
                return ProcessMinerAuthInit(context, packet);

            case MINER_AUTH_RESPONSE:
                debug::log(2, FUNCTION, "Routing to ProcessFalconResponse");
                return ProcessFalconResponse(context, packet);

            case SESSION_START:
                debug::log(2, FUNCTION, "Routing to ProcessSessionStart");
                return ProcessSessionStart(context, packet);

            case SET_CHANNEL:
                debug::log(2, FUNCTION, "Routing to ProcessSetChannel");
                return ProcessSetChannel(context, packet);

            case SESSION_KEEPALIVE:
                debug::log(3, FUNCTION, "Routing to ProcessSessionKeepalive");
                return ProcessSessionKeepalive(context, packet);

            default:
                debug::log(1, FUNCTION, "Unknown miner opcode: ", uint32_t(packet.HEADER));
                return ProcessResult::Error(context, "Unknown packet type");
        }
    }


    /* Build message for Falcon signature verification */
    std::vector<uint8_t> StatelessMiner::BuildAuthMessage(const MiningContext& context)
    {
        /* For challenge-response authentication, use the stored nonce */
        if(!context.vAuthNonce.empty())
        {
            return context.vAuthNonce;
        }

        /* Fallback: Build message from address + timestamp (legacy mode) */
        std::vector<uint8_t> vMessage;

        /* Add address string */
        vMessage.insert(vMessage.end(), context.strAddress.begin(), context.strAddress.end());

        /* Add timestamp (8 bytes, little-endian) */
        uint64_t nTime = context.nTimestamp;
        for(int i = 0; i < 8; ++i)
        {
            vMessage.push_back(static_cast<uint8_t>(nTime & 0xFF));
            nTime >>= 8;
        }

        return vMessage;
    }


    /* Derive ChaCha20 session key from genesis hash */
    std::vector<uint8_t> StatelessMiner::DeriveChaCha20SessionKey(const uint256_t& hashGenesis)
    {
        /* Domain separation for security */
        static const std::string DOMAIN = "nexus-mining-chacha20-v1";
        
        std::vector<uint8_t> preimage;
        preimage.insert(preimage.end(), DOMAIN.begin(), DOMAIN.end());
        
        std::vector<uint8_t> genesis_bytes = hashGenesis.GetBytes();
        preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());
        
        /* SHA-256 → 32-byte key */
        uint256_t hashKey = LLC::SK256(preimage);
        return hashKey.GetBytes();
    }


    /* Process MINER_AUTH_INIT - first step of authentication handshake */
    ProcessResult StatelessMiner::ProcessMinerAuthInit(
        const MiningContext& context,
        const Packet& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_AUTH_INIT from ", context.strAddress);

        const std::vector<uint8_t>& vData = packet.DATA;

        /* DEBUG: Log incoming packet details for GetBytes() debugging */
        debug::log(2, FUNCTION, "MINER_AUTH_INIT packet: ", packet.DebugString());
        DisposableFalcon::DebugLogPacket("MINER_AUTH_INIT::data", vData, 3);

        /* Validate minimum packet size: genesis(32) + pubkey_len(2) + miner_id_len(2) = 36 */
        if(vData.size() < 36)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small, size=", vData.size(), " expected>=36");
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small");
        }

        size_t nPos = 0;

        /* ═══════════════════════════════════════════════════════════
         * STEP 1: Parse hashGenesis FIRST (32 bytes)
         * ═══════════════════════════════════════════════════════════ */
        uint256_t hashGenesis(0);
        std::vector<uint8_t> vGenesis(vData.begin(), vData.begin() + 32);
        
        /* DEBUG: Log raw genesis bytes received for troubleshooting */
        debug::log(0, FUNCTION, "Genesis raw bytes (", vGenesis.size(), "): ", HexStr(vGenesis));
        
        /* Try standard little-endian format first (as returned by GetBytes) */
        hashGenesis.SetBytes(vGenesis);
        
        /* Check if genesis has valid type byte
         * Some miner implementations send genesis in big-endian (hex string order)
         * where type byte is first, while GetBytes() returns little-endian where
         * type byte is last. We handle both formats for compatibility. */
        if(!GenesisConstants::IsValidGenesisType(hashGenesis))
        {
            /* Try reversed (big-endian hex string order) */
            debug::log(2, FUNCTION, "Genesis type byte invalid in little-endian format, trying big-endian");
            std::reverse(vGenesis.begin(), vGenesis.end());
            hashGenesis.SetBytes(vGenesis);
            
            if(GenesisConstants::IsValidGenesisType(hashGenesis))
            {
                debug::log(0, FUNCTION, "Genesis byte order corrected from big-endian to little-endian");
            }
            else
            {
                debug::log(0, FUNCTION, "WARNING: Genesis type byte invalid in both byte orders");
            }
        }
        
        nPos += 32;

        /* DEBUG: Log parsed genesis details */
        debug::log(0, FUNCTION, "Genesis parsed: ", hashGenesis.SubString());
        debug::log(0, FUNCTION, "Genesis full hex: ", hashGenesis.GetHex());
        {
            std::ostringstream oss;
            oss << "Genesis type byte: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<uint32_t>(hashGenesis.GetType());
            debug::log(0, FUNCTION, oss.str());
        }

        /* Derive ChaCha20 session key from genesis */
        std::vector<uint8_t> vSessionKey;
        bool fCanDecrypt = false;
        if(hashGenesis != 0)
        {
            vSessionKey = DeriveChaCha20SessionKey(hashGenesis);
            fCanDecrypt = true;
            debug::log(2, FUNCTION, "ChaCha20 key derived from genesis");
        }

        /* ═══════════════════════════════════════════════════════════
         * STEP 2: Parse pubkey_len (2 bytes, big-endian)
         * ═══════════════════════════════════════════════════════════ */
        DisposableFalcon::DebugLogDeserialize("pubkey_len", nPos, 2, vData.size());
        uint16_t nPubKeyLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                              static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: parsed pubkey_len=", nPubKeyLen);

        /* Detect wrapped vs unwrapped */
        bool fWrapped = (nPubKeyLen == 925);  // 897 + 12 + 16

        /* Validate pubkey_len */
        if(nPubKeyLen == 0 || nPubKeyLen > 2048)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: invalid pubkey_len=", nPubKeyLen);
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid pubkey_len");
        }

        if(nPos + nPubKeyLen + 2 > vData.size())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small for pubkey, need=",
                       nPos + nPubKeyLen + 2, " have=", vData.size());
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for pubkey");
        }

        /* Extract public key data */
        DisposableFalcon::DebugLogDeserialize("pubkey", nPos, nPubKeyLen, vData.size());
        std::vector<uint8_t> vPubKeyData(vData.begin() + nPos, vData.begin() + nPos + nPubKeyLen);
        nPos += nPubKeyLen;

        /* ═══════════════════════════════════════════════════════════
         * STEP 3: Unwrap if ChaCha20 encrypted
         * ═══════════════════════════════════════════════════════════ */
        std::vector<uint8_t> vPubKey;
        if(fWrapped)
        {
            if(!fCanDecrypt)
            {
                debug::log(0, FUNCTION, "Wrapped key but zero genesis");
                return ProcessResult::Error(context, "Wrapped key requires genesis");
            }

            /* ChaCha20-Poly1305 format: nonce(12) + ciphertext(897) + tag(16) */
            if(vPubKeyData.size() != 925)
            {
                debug::log(0, FUNCTION, "Invalid wrapped key size: ", vPubKeyData.size());
                return ProcessResult::Error(context, "Invalid wrapped key size");
            }

            std::vector<uint8_t> vNonce(vPubKeyData.begin(), vPubKeyData.begin() + 12);
            std::vector<uint8_t> vCiphertext(vPubKeyData.begin() + 12, vPubKeyData.end() - 16);
            std::vector<uint8_t> vTag(vPubKeyData.end() - 16, vPubKeyData.end());
            std::vector<uint8_t> vAAD{'F','A','L','C','O','N','_','P','U','B','K','E','Y'};

            if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vSessionKey, vNonce, vPubKey, vAAD))
            {
                debug::log(0, FUNCTION, "ChaCha20 decryption FAILED - genesis mismatch?");
                return ProcessResult::Error(context, "ChaCha20 decryption failed");
            }
            debug::log(0, FUNCTION, "✓ ChaCha20 unwrap SUCCESS");
        }
        else
        {
            vPubKey = vPubKeyData;
        }

        /* Validate pubkey size */
        if(vPubKey.size() != 897)
        {
            debug::log(0, FUNCTION, "Invalid pubkey size: ", vPubKey.size(), " (expected 897)");
            return ProcessResult::Error(context, "Invalid public key size");
        }

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: extracted pubkey, len=", vPubKey.size());

        /* ═══════════════════════════════════════════════════════════
         * STEP 4: Parse miner_id
         * ═══════════════════════════════════════════════════════════ */
        DisposableFalcon::DebugLogDeserialize("miner_id_len", nPos, 2, vData.size());
        uint16_t nMinerIdLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                               static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        debug::log(3, FUNCTION, "MINER_AUTH_INIT: parsed miner_id_len=", nMinerIdLen);

        /* Validate miner_id_len */
        if(nMinerIdLen > 256)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: invalid miner_id_len=", nMinerIdLen);
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid miner_id_len");
        }

        if(nPos + nMinerIdLen > vData.size())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_INIT: packet too small for miner_id, need=",
                       nPos + nMinerIdLen, " have=", vData.size());
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for miner_id");
        }

        /* Extract miner ID (for logging) */
        DisposableFalcon::DebugLogDeserialize("miner_id", nPos, nMinerIdLen, vData.size());
        std::string strMinerId = "<no-id>";
        if(nMinerIdLen > 0)
            strMinerId.assign(vData.begin() + nPos, vData.begin() + nPos + nMinerIdLen);

        /* Generate challenge nonce */
        std::vector<uint8_t> vAuthNonce = LLC::GetRand256().GetBytes();

        /* Log summary */
        debug::log(0, FUNCTION, "MINER_AUTH_INIT: genesis=", hashGenesis.SubString(),
                   " miner=", strMinerId, " pubkey=", vPubKey.size(), 
                   fWrapped ? " (unwrapped)" : "");

        /* Validate genesis binding if FalconAuth is available
         * Genesis validation is CRITICAL for reward routing.
         * If genesis is invalid or doesn't resolve to a valid account, 
         * authentication MUST fail to prevent mining with incorrect reward routing. */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        if(pAuth && hashGenesis != 0)
        {
            uint256_t hashKeyID = pAuth->DeriveKeyId(vPubKey);
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            
            /* If this key has a bound genesis, verify it matches */
            if(boundGenesis.has_value() && boundGenesis.value() != 0)
            {
                if(boundGenesis.value() != hashGenesis)
                {
                    debug::log(0, FUNCTION, "MINER_AUTH_INIT: genesis mismatch! claimed=", 
                               hashGenesis.SubString(), " bound=", boundGenesis.value().SubString());
                    return ProcessResult::Error(context, "Genesis mismatch with bound Falcon key");
                }
                debug::log(2, FUNCTION, "MINER_AUTH_INIT: genesis binding verified");
            }
            else
            {
                /* No existing binding - this is a new key */
                debug::log(0, FUNCTION, "MINER_AUTH_INIT: new key, genesis=", hashGenesis.SubString());
            }
        }

        /* Update context */
        MiningContext newContext = context
            .WithPubKey(vPubKey)
            .WithNonce(vAuthNonce)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Build challenge */
        Packet response(MINER_AUTH_CHALLENGE);
        uint16_t nNonceLen = static_cast<uint16_t>(vAuthNonce.size());
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen >> 8));
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen & 0xFF));
        response.DATA.insert(response.DATA.end(), vAuthNonce.begin(), vAuthNonce.end());
        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        /* DEBUG: Log response packet */
        debug::log(2, FUNCTION, "MINER_AUTH_CHALLENGE response: ", response.DebugString());

        debug::log(0, FUNCTION, "Sending MINER_AUTH_CHALLENGE nonce_len=", vAuthNonce.size());

        return ProcessResult::Success(newContext, response);
    }


    /* Process Falcon authentication response (MINER_AUTH_RESPONSE) */
    ProcessResult StatelessMiner::ProcessFalconResponse(
        const MiningContext& context,
        const Packet& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE from ", context.strAddress);

        /* DEBUG: Log incoming packet details */
        debug::log(2, FUNCTION, "MINER_AUTH_RESPONSE packet: ", packet.DebugString());
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::data", packet.DATA, 3);

        /* Validate that we have nonce and pubkey from MINER_AUTH_INIT */
        if(context.vAuthNonce.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no nonce (MINER_AUTH_INIT not received)");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        if(context.vMinerPubKey.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no pubkey (MINER_AUTH_INIT not received)");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate minimum packet size (2 bytes for sig_len) */
        if(vData.size() < 2)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small, size=", vData.size());
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Parse sig_len (2 bytes, little-endian to match NexusMiner protocol) */
        DisposableFalcon::DebugLogDeserialize("sig_len", 0, 2, vData.size());
        uint16_t nSigLen = static_cast<uint16_t>(vData[0]) |
                           (static_cast<uint16_t>(vData[1]) << 8);

        debug::log(3, FUNCTION, "MINER_AUTH_RESPONSE: parsed sig_len=", nSigLen);

        /* Validate sig_len */
        if(nSigLen == 0 || nSigLen > FalconConstants::FALCON512_SIG_MAX_VALIDATION)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid sig_len ", nSigLen);
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        if(vData.size() < 2 + nSigLen)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small for signature, need=",
                       2 + nSigLen, " have=", vData.size());
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Extract signature */
        DisposableFalcon::DebugLogDeserialize("signature", 2, nSigLen, vData.size());
        std::vector<uint8_t> vSignature(vData.begin() + 2, vData.begin() + 2 + nSigLen);

        debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE sig_len=", nSigLen);

        /* DEBUG: Log verification data */
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::pubkey", context.vMinerPubKey, 4);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::nonce", context.vAuthNonce, 4);
        DisposableFalcon::DebugLogPacket("MINER_AUTH_RESPONSE::signature", vSignature, 4);

        /* Verify Falcon signature using LLC::FLKey directly */
        LLC::FLKey flkey;
        if(!flkey.SetPubKey(context.vMinerPubKey))
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid public key, len=",
                       context.vMinerPubKey.size());
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        debug::log(3, FUNCTION, "MINER_AUTH_RESPONSE: verifying signature...");

        if(!flkey.Verify(context.vAuthNonce, vSignature))
        {
            debug::log(0, FUNCTION, "MINER_AUTH verification FAILED from ", context.strAddress);
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        debug::log(2, FUNCTION, "MINER_AUTH_RESPONSE: signature verified successfully");

        /* Authentication succeeded */
        /* Derive key ID from public key */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        uint256_t hashKeyID(0);
        if(pAuth)
            hashKeyID = pAuth->DeriveKeyId(context.vMinerPubKey);

        /* Derive session ID from key ID (lower 32 bits) */
        uint32_t nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0) & 0xFFFFFFFF);

        /* Use genesis from MINER_AUTH_INIT if provided, otherwise check binding */
        uint256_t hashGenesis = context.hashGenesis;  // Already set from INIT
        if(hashGenesis == 0 && pAuth)
        {
            /* Fallback: check for bound genesis */
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            if(boundGenesis.has_value())
                hashGenesis = boundGenesis.value();
        }

        debug::log(0, FUNCTION, "MINER_AUTH success: keyID=", hashKeyID.SubString(),
                   " genesis=", hashGenesis.SubString(), " sessionId=", nSessionId);

        /* Authentication succeeded - now resolve reward routing */
        uint256_t hashGenesisFinal = hashGenesis;

        /* If no genesis in context, check for bound genesis */
        if(hashGenesisFinal == 0 && pAuth)
        {
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            if(boundGenesis.has_value())
                hashGenesisFinal = boundGenesis.value();
        }

        /* Resolve and validate username:default account for reward routing
         * This is CRITICAL - mining cannot proceed without valid reward routing */
        TAO::Register::Address hashDefaultAccount(0);

        if(hashGenesisFinal != 0)
        {
            debug::log(0, FUNCTION, "");
            debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
            debug::log(0, FUNCTION, "       GENESIS RESOLUTION FOR REWARD ROUTING");
            debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
            debug::log(0, FUNCTION, "Genesis hash: ", hashGenesisFinal.ToString());
            
            /* Validate genesis and resolve/cache default account mapping
             * CRITICAL: This validation is now FATAL - if it fails, authentication is denied.
             * Previous behavior logged failure but allowed auth to proceed - this was incorrect
             * as it would allow mining without valid reward routing. */
            StatelessMinerManager& manager = StatelessMinerManager::Get();
            if(!manager.ValidateAndCacheGenesis(hashGenesisFinal, hashDefaultAccount))
            {
                debug::log(0, FUNCTION, "✗ Genesis validation or default account resolution FAILED");
                debug::log(0, FUNCTION, "  Genesis: ", hashGenesisFinal.ToString());
                debug::log(0, FUNCTION, "  User must ensure genesis is valid and has 'default' account");
                debug::log(0, FUNCTION, "  To create: finance/create/account name=default");
                debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
                
                /* FATAL ERROR: Cannot mine without valid reward routing */
                Packet response(MINER_AUTH_RESULT);
                response.DATA.push_back(0x00); // Failure
                response.LENGTH = 1;
                return ProcessResult::Success(context, response);
            }
            
            debug::log(0, FUNCTION, "✓ Genesis validation successful");
            debug::log(0, FUNCTION, "✓ Resolved default account: ", hashDefaultAccount.ToString());
            debug::log(0, FUNCTION, "✓ Genesis→Default mapping cached");
            debug::log(0, FUNCTION, "═══════════════════════════════════════════════════════════");
        }
        else
        {
            /* No genesis provided - cannot proceed with stateless mining */
            debug::log(0, FUNCTION, "ERROR: No genesis hash provided - cannot route rewards");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            response.LENGTH = 1;
            return ProcessResult::Success(context, response);
        }

        /* Log authentication success summary */
        debug::log(0, FUNCTION, "");
        debug::log(0, FUNCTION, "╔═══════════════════════════════════════════════════════════╗");
        debug::log(0, FUNCTION, "║         MINER AUTHENTICATION SUCCESSFUL                   ║");
        debug::log(0, FUNCTION, "╠═══════════════════════════════════════════════════════════╣");
        debug::log(0, FUNCTION, "║ Key ID:       ", hashKeyID.SubString());
        debug::log(0, FUNCTION, "║ Session ID:   ", nSessionId);
        debug::log(0, FUNCTION, "║ Genesis:      ", hashGenesisFinal.SubString());
        debug::log(0, FUNCTION, "║ Default Acct: ", hashDefaultAccount.SubString());
        debug::log(0, FUNCTION, "║ Reward Route: DYNAMIC (username:default)");
        debug::log(0, FUNCTION, "║ From:         ", context.strAddress);
        debug::log(0, FUNCTION, "╚═══════════════════════════════════════════════════════════╝");

        /* Update context with auth success.
         * Note: We clear the nonce and pubkey after successful authentication for security:
         * - The nonce was single-use and should not be reused
         * - The pubkey hash is preserved in hashKeyID for audit trails
         * - The full pubkey can be recovered from FalconAuth if needed */
        MiningContext newContext = context
            .WithAuth(true)
            .WithSession(nSessionId)
            .WithKeyId(hashKeyID)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithNonce(std::vector<uint8_t>())  // Clear single-use nonce
            .WithPubKey(std::vector<uint8_t>()); // Clear pubkey (hash preserved in keyID)

        /* Build success response */
        Packet response(MINER_AUTH_RESULT);
        response.DATA.push_back(0x01); // Success
        response.LENGTH = 1;

        /* DEBUG: Log response packet */
        debug::log(2, FUNCTION, "MINER_AUTH_RESULT response: ", response.DebugString());

        debug::log(0, FUNCTION, "MINER_AUTH success for key ", hashKeyID.SubString(),
                   " sessionId=", nSessionId, " from ", context.strAddress);

        return ProcessResult::Success(newContext, response);
    }


    /* Process session start */
    ProcessResult StatelessMiner::ProcessSessionStart(
        const MiningContext& context,
        const Packet& packet
    )
    {
        /* Phase 2: Require authentication before session start */
        if(!context.fAuthenticated)
            return ProcessResult::Error(context, "Not authenticated");

        const std::vector<uint8_t>& vData = packet.DATA;
        uint64_t nNow = runtime::unifiedtimestamp();

        /* Parse optional session parameters from packet.DATA */
        /* Format: [timeout (4 bytes, optional)] */
        uint64_t nRequestedTimeout = DEFAULT_SESSION_TIMEOUT;
        if(vData.size() >= 4)
        {
            /* Parse timeout as 4-byte little-endian */
            nRequestedTimeout = vData[0] | (vData[1] << 8) |
                               (vData[2] << 16) | (vData[3] << 24);

            /* Clamp timeout to reasonable range (60s to 3600s) */
            if(nRequestedTimeout < 60)
                nRequestedTimeout = 60;
            else if(nRequestedTimeout > 3600)
                nRequestedTimeout = 3600;
        }

        /* Initialize session timing */
        MiningContext newContext = context
            .WithTimestamp(nNow)
            .WithSessionStart(nNow)
            .WithSessionTimeout(nRequestedTimeout)
            .WithKeepaliveCount(0);

        debug::log(0, FUNCTION, "SESSION_START established for sessionId=", context.nSessionId,
                   " timeout=", nRequestedTimeout, "s from ", context.strAddress);

        /* After session is established, log detailed summary */
        debug::log(0, FUNCTION, "");
        debug::log(0, FUNCTION, "╔═══════════════════════════════════════════════════════════╗");
        debug::log(0, FUNCTION, "║           MINING SESSION ESTABLISHED                      ║");
        debug::log(0, FUNCTION, "╠═══════════════════════════════════════════════════════════╣");
        debug::log(0, FUNCTION, "║ Session ID:    ", context.nSessionId);
        debug::log(0, FUNCTION, "║ Timeout:       ", nRequestedTimeout, " seconds");
        debug::log(0, FUNCTION, "║ Genesis:       ", context.hashGenesis != 0 ? context.hashGenesis.SubString() : "NOT SET");
        debug::log(0, FUNCTION, "║ Miner:         ", context.strAddress);
        debug::log(0, FUNCTION, "╚═══════════════════════════════════════════════════════════╝");

        /* Log reward routing status */
        if(context.hashGenesis != 0)
        {
            TAO::Register::Address hashDefault = 
                StatelessMinerManager::Get().GetCachedDefaultAccount(context.hashGenesis);
            
            if(hashDefault != 0)
                debug::log(0, FUNCTION, "REWARDS → Dynamic: ", hashDefault.SubString());
            else
                debug::log(0, FUNCTION, "REWARDS → Genesis set but default not resolved yet");
        }
        else
        {
            debug::log(0, FUNCTION, "REWARDS → Static: Node's mining address");
        }

        /* Build acknowledgment response with session parameters */
        /* Response format: [success (1)][session_id (4)][timeout (4)][genesis (32)] */
        Packet response(SESSION_START);
        response.DATA.push_back(0x01); // Success

        /* Add session ID (4 bytes, little-endian) */
        uint32_t nSessionId = newContext.nSessionId;
        response.DATA.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));

        /* Add timeout (4 bytes, little-endian) */
        response.DATA.push_back(static_cast<uint8_t>(nRequestedTimeout & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRequestedTimeout >> 8) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRequestedTimeout >> 16) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRequestedTimeout >> 24) & 0xFF));

        /* Add genesis hash if bound (32 bytes) for GenesisHash reward mapping */
        if(newContext.hashGenesis != 0)
        {
            std::vector<uint8_t> vGenesis = newContext.hashGenesis.GetBytes();
            response.DATA.insert(response.DATA.end(), vGenesis.begin(), vGenesis.end());
        }

        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        return ProcessResult::Success(newContext, response);
    }


    /* Process set channel */
    ProcessResult StatelessMiner::ProcessSetChannel(
        const MiningContext& context,
        const Packet& packet
    )
    {
        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate payload size (1 byte or 4+ bytes for legacy compatibility) */
        if(vData.size() != 1 && vData.size() < 4)
            return ProcessResult::Error(context, "Invalid channel payload size");

        uint32_t nChannel = 0;

        /* Parse channel value */
        if(vData.size() == 1)
        {
            /* New format: single byte */
            nChannel = static_cast<uint32_t>(vData[0]);
        }
        else
        {
            /* Legacy format: 4-byte little-endian */
            nChannel = vData[0] | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
        }

        /* Validate channel (1=Prime, 2=Hash, 0=invalid) */
        if(nChannel != 1 && nChannel != 2)
            return ProcessResult::Error(context, "Invalid channel");

        /* Update context */
        MiningContext newContext = context
            .WithChannel(nChannel)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Build acknowledgment response */
        Packet response(CHANNEL_ACK);
        response.DATA.push_back(static_cast<uint8_t>(nChannel));

        debug::log(2, FUNCTION, "Channel set to ", nChannel);

        return ProcessResult::Success(newContext, response);
    }


    /* Process session keepalive */
    ProcessResult StatelessMiner::ProcessSessionKeepalive(
        const MiningContext& context,
        const Packet& packet
    )
    {
        /* Phase 2: Require authentication before keepalive */
        if(!context.fAuthenticated)
            return ProcessResult::Error(context, "Not authenticated");

        /* Check if session has been started */
        if(context.nSessionStart == 0)
            return ProcessResult::Error(context, "Session not started");

        uint64_t nNow = runtime::unifiedtimestamp();

        /* Check if session has expired (even for keepalive) */
        if(context.IsSessionExpired(nNow))
        {
            debug::log(0, FUNCTION, "SESSION_KEEPALIVE rejected - session expired for sessionId=",
                       context.nSessionId, " last_activity=", context.nTimestamp);
            return ProcessResult::Error(context, "Session expired");
        }

        /* Update timestamp and increment keepalive count */
        uint32_t nNewKeepaliveCount = context.nKeepaliveCount + 1;
        MiningContext newContext = context
            .WithTimestamp(nNow)
            .WithKeepaliveCount(nNewKeepaliveCount);

        /* Log at different verbosity levels based on keepalive frequency */
        uint32_t nLogLevel = (nNewKeepaliveCount % 10 == 0) ? 2 : 3;
        debug::log(nLogLevel, FUNCTION, "SESSION_KEEPALIVE from sessionId=", context.nSessionId,
                   " keepalive_count=", nNewKeepaliveCount,
                   " session_duration=", newContext.GetSessionDuration(nNow), "s");

        /* Build keepalive response with session status */
        /* Response format: [status (1)][remaining_timeout (4)] */
        Packet response(SESSION_KEEPALIVE);
        response.DATA.push_back(0x01); // Success/active

        /* Calculate remaining time before timeout */
        uint64_t nElapsed = nNow - context.nTimestamp;
        uint64_t nRemaining = (nElapsed < context.nSessionTimeout)
                            ? (context.nSessionTimeout - nElapsed)
                            : 0;

        /* Add remaining timeout (4 bytes, little-endian) */
        response.DATA.push_back(static_cast<uint8_t>(nRemaining & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRemaining >> 8) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRemaining >> 16) & 0xFF));
        response.DATA.push_back(static_cast<uint8_t>((nRemaining >> 24) & 0xFF));

        response.LENGTH = static_cast<uint32_t>(response.DATA.size());

        return ProcessResult::Success(newContext, response);
    }

} // namespace LLP
