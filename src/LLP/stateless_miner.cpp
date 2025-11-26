/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>

#include <LLC/include/random.h>
#include <LLC/include/flkey.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
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
    , vAuthNonce()
    , vMinerPubKey()
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
    , vAuthNonce()
    , vMinerPubKey()
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
        /* Route based on packet type */
        /* Note: GET_BLOCK and SUBMIT_BLOCK are handled in StatelessMinerConnection */
        /* due to their need for stateful block management */
        switch(packet.HEADER)
        {
            case MINER_AUTH_INIT:
                return ProcessMinerAuthInit(context, packet);

            case MINER_AUTH_RESPONSE:
                return ProcessFalconResponse(context, packet);

            case SESSION_START:
                return ProcessSessionStart(context, packet);

            case SET_CHANNEL:
                return ProcessSetChannel(context, packet);

            case SESSION_KEEPALIVE:
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


    /* Process MINER_AUTH_INIT - first step of authentication handshake */
    ProcessResult StatelessMiner::ProcessMinerAuthInit(
        const MiningContext& context,
        const Packet& packet
    )
    {
        debug::log(0, FUNCTION, "MINER_AUTH_INIT from ", context.strAddress);

        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate minimum packet size (2 + 2 = 4 bytes for lengths) */
        if(vData.size() < 4)
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small");

        size_t nPos = 0;

        /* Parse pubkey_len (2 bytes, big-endian to match miner.cpp) */
        uint16_t nPubKeyLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                              static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        /* Validate pubkey_len */
        if(nPubKeyLen == 0 || nPubKeyLen > 2048)
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid pubkey_len");

        if(nPos + nPubKeyLen + 2 > vData.size())
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for pubkey");

        /* Extract public key */
        std::vector<uint8_t> vPubKey(vData.begin() + nPos, vData.begin() + nPos + nPubKeyLen);
        nPos += nPubKeyLen;

        /* Parse miner_id_len (2 bytes, big-endian) */
        uint16_t nMinerIdLen = (static_cast<uint16_t>(vData[nPos]) << 8) |
                               static_cast<uint16_t>(vData[nPos + 1]);
        nPos += 2;

        /* Validate miner_id_len */
        if(nMinerIdLen > 256)
            return ProcessResult::Error(context, "MINER_AUTH_INIT: invalid miner_id_len");

        if(nPos + nMinerIdLen > vData.size())
            return ProcessResult::Error(context, "MINER_AUTH_INIT: packet too small for miner_id");

        /* Extract miner ID (for logging) */
        std::string strMinerId;
        if(nMinerIdLen > 0)
            strMinerId.assign(vData.begin() + nPos, vData.begin() + nPos + nMinerIdLen);
        else
            strMinerId = "<no-id>";

        /* Generate random nonce (32 bytes) */
        uint256_t nonce = LLC::GetRand256();
        std::vector<uint8_t> vAuthNonce = nonce.GetBytes();

        debug::log(0, FUNCTION, "MINER_AUTH_INIT from ", context.strAddress,
                   " miner_id=", strMinerId, " pubkey_len=", nPubKeyLen);

        /* Update context with pubkey and nonce */
        MiningContext newContext = context
            .WithPubKey(vPubKey)
            .WithNonce(vAuthNonce)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Build MINER_AUTH_CHALLENGE response */
        Packet response(MINER_AUTH_CHALLENGE);
        uint16_t nNonceLen = static_cast<uint16_t>(vAuthNonce.size());
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen >> 8));
        response.DATA.push_back(static_cast<uint8_t>(nNonceLen & 0xFF));
        response.DATA.insert(response.DATA.end(), vAuthNonce.begin(), vAuthNonce.end());

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

        /* Validate that we have nonce and pubkey from MINER_AUTH_INIT */
        if(context.vAuthNonce.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no nonce (MINER_AUTH_INIT not received)");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        if(context.vMinerPubKey.empty())
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: no pubkey (MINER_AUTH_INIT not received)");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        const std::vector<uint8_t>& vData = packet.DATA;

        /* Validate minimum packet size (2 bytes for sig_len) */
        if(vData.size() < 2)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        /* Parse sig_len (2 bytes, big-endian to match miner.cpp) */
        uint16_t nSigLen = (static_cast<uint16_t>(vData[0]) << 8) |
                           static_cast<uint16_t>(vData[1]);

        /* Validate sig_len */
        if(nSigLen == 0 || nSigLen > 2048)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid sig_len ", nSigLen);
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        if(vData.size() < 2 + nSigLen)
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: packet too small for signature");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        /* Extract signature */
        std::vector<uint8_t> vSignature(vData.begin() + 2, vData.begin() + 2 + nSigLen);

        debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE sig_len=", nSigLen);

        /* Verify Falcon signature using LLC::FLKey directly */
        LLC::FLKey flkey;
        if(!flkey.SetPubKey(context.vMinerPubKey))
        {
            debug::log(0, FUNCTION, "MINER_AUTH_RESPONSE: invalid public key");
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        if(!flkey.Verify(context.vAuthNonce, vSignature))
        {
            debug::log(0, FUNCTION, "MINER_AUTH verification FAILED from ", context.strAddress);
            Packet response(MINER_AUTH_RESULT);
            response.DATA.push_back(0x00); // Failure
            return ProcessResult::Success(context, response);
        }

        /* Authentication succeeded */
        /* Derive key ID from public key */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        uint256_t hashKeyID(0);
        if(pAuth)
            hashKeyID = pAuth->DeriveKeyId(context.vMinerPubKey);

        /* Derive session ID from key ID (lower 32 bits) */
        uint32_t nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0) & 0xFFFFFFFF);

        /* Check for bound genesis */
        uint256_t hashGenesis(0);
        if(pAuth)
        {
            std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
            if(boundGenesis.has_value())
                hashGenesis = boundGenesis.value();
        }

        /* Update context with auth success */
        MiningContext newContext = context
            .WithAuth(true)
            .WithSession(nSessionId)
            .WithKeyId(hashKeyID)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp())
            .WithNonce(std::vector<uint8_t>())  // Clear nonce after successful auth
            .WithPubKey(std::vector<uint8_t>()); // Clear pubkey after successful auth

        /* Build success response */
        Packet response(MINER_AUTH_RESULT);
        response.DATA.push_back(0x01); // Success

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

        /* Session start is now just a session negotiation step */
        /* Parse any session parameters from packet.DATA if needed */

        /* For now, just acknowledge and update timestamp */
        MiningContext newContext = context.WithTimestamp(runtime::unifiedtimestamp());

        /* Build acknowledgment response */
        Packet response(SESSION_START);
        response.DATA.push_back(1); // Success

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

        debug::log(3, FUNCTION, "SESSION_KEEPALIVE from sessionId=", context.nSessionId);

        /* Update timestamp to keep session alive */
        MiningContext newContext = context.WithTimestamp(runtime::unifiedtimestamp());

        /* No response packet needed for keepalive */
        return ProcessResult::Success(newContext, Packet());
    }

} // namespace LLP
