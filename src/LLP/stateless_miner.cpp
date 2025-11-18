/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_auth.h>

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
    {
    }

    /* Immutable update methods */
    MiningContext MiningContext::WithChannel(uint32_t nChannel_) const
    {
        return MiningContext(
            nChannel_, nHeight, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId, hashKeyID, hashGenesis
        );
    }

    MiningContext MiningContext::WithHeight(uint32_t nHeight_) const
    {
        return MiningContext(
            nChannel, nHeight_, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId, hashKeyID, hashGenesis
        );
    }

    MiningContext MiningContext::WithTimestamp(uint64_t nTimestamp_) const
    {
        return MiningContext(
            nChannel, nHeight, nTimestamp_, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId, hashKeyID, hashGenesis
        );
    }

    MiningContext MiningContext::WithAuth(bool fAuthenticated_) const
    {
        return MiningContext(
            nChannel, nHeight, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated_, nSessionId, hashKeyID, hashGenesis
        );
    }

    MiningContext MiningContext::WithSession(uint32_t nSessionId_) const
    {
        return MiningContext(
            nChannel, nHeight, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId_, hashKeyID, hashGenesis
        );
    }

    MiningContext MiningContext::WithKeyId(const uint256_t& hashKeyID_) const
    {
        return MiningContext(
            nChannel, nHeight, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId, hashKeyID_, hashGenesis
        );
    }

    MiningContext MiningContext::WithGenesis(const uint256_t& hashGenesis_) const
    {
        return MiningContext(
            nChannel, nHeight, nTimestamp, strAddress, nProtocolVersion,
            fAuthenticated, nSessionId, hashKeyID, hashGenesis_
        );
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


    /* Packet type definitions - must match miner.h */
    enum : Packet::message_t
    {
        SET_CHANNEL          = 3,
        MINER_AUTH_RESPONSE  = 209,
        SESSION_START        = 211,
        CHANNEL_ACK          = 206,
    };


    /* Main packet processing entry point */
    ProcessResult StatelessMiner::ProcessPacket(
        const MiningContext& context,
        const Packet& packet
    )
    {
        /* Route based on packet type */
        switch(packet.HEADER)
        {
            case MINER_AUTH_RESPONSE:
                return ProcessFalconAuthResponse(context, packet);

            case SESSION_START:
                return ProcessSessionStart(context, packet);

            case SET_CHANNEL:
                return ProcessSetChannel(context, packet);

            default:
                return ProcessResult::Error(context, "Unknown packet type");
        }
    }


    /* Build message for Falcon signature verification */
    std::vector<uint8_t> StatelessMiner::BuildAuthMessage(const MiningContext& context)
    {
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

        /* TODO: Add challenge nonce for enhanced security */

        return vMessage;
    }


    /* Process Falcon authentication response */
    ProcessResult StatelessMiner::ProcessFalconAuthResponse(
        const MiningContext& context,
        const Packet& packet
    )
    {
        /* Get Falcon auth instance */
        FalconAuth::IFalconAuth* pAuth = FalconAuth::Get();
        if(!pAuth)
            return ProcessResult::Error(context, "Falcon auth not initialized");

        /* Parse packet data */
        const std::vector<uint8_t>& vData = packet.DATA;

        /* Minimum size check: pubkey_len(2) + sig_len(2) + data */
        if(vData.size() < 4)
            return ProcessResult::Error(context, "Invalid auth packet size");

        size_t nPos = 0;

        /* Read public key length (2 bytes, little-endian) */
        uint16_t nPubkeyLen = vData[nPos] | (vData[nPos + 1] << 8);
        nPos += 2;

        if(nPos + nPubkeyLen > vData.size())
            return ProcessResult::Error(context, "Invalid public key length");

        /* Extract public key */
        std::vector<uint8_t> vPubkey(vData.begin() + nPos, vData.begin() + nPos + nPubkeyLen);
        nPos += nPubkeyLen;

        /* Read signature length (2 bytes, little-endian) */
        if(nPos + 2 > vData.size())
            return ProcessResult::Error(context, "Invalid packet format");

        uint16_t nSigLen = vData[nPos] | (vData[nPos + 1] << 8);
        nPos += 2;

        if(nPos + nSigLen > vData.size())
            return ProcessResult::Error(context, "Invalid signature length");

        /* Extract signature */
        std::vector<uint8_t> vSignature(vData.begin() + nPos, vData.begin() + nPos + nSigLen);
        nPos += nSigLen;

        /* Optional: Extract Tritium genesis hash (32 bytes) */
        uint256_t hashGenesis(0);
        if(nPos + 32 <= vData.size())
        {
            hashGenesis.SetBytes(std::vector<uint8_t>(vData.begin() + nPos, vData.begin() + nPos + 32));
            nPos += 32;
        }

        /* Build message for verification */
        std::vector<uint8_t> vMessage = BuildAuthMessage(context);

        /* Verify signature */
        FalconAuth::VerifyResult result = pAuth->Verify(vPubkey, vMessage, vSignature);
        if(!result.fValid)
        {
            debug::log(0, FUNCTION, "Falcon verification failed: ", result.strError);
            return ProcessResult::Error(context, "Falcon verification failed: " + result.strError);
        }

        /* Derive key ID */
        uint256_t hashKeyID = result.keyId;

        /* Check for bound genesis */
        std::optional<uint256_t> boundGenesis = pAuth->GetBoundGenesis(hashKeyID);
        if(boundGenesis.has_value())
        {
            /* Use bound genesis instead of packet-provided one */
            hashGenesis = boundGenesis.value();
        }

        /* Derive session ID from key ID (lower 32 bits) */
        uint32_t nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0) & 0xFFFFFFFF);

        /* Update context with auth success */
        MiningContext newContext = context
            .WithAuth(true)
            .WithSession(nSessionId)
            .WithKeyId(hashKeyID)
            .WithGenesis(hashGenesis)
            .WithTimestamp(runtime::unifiedtimestamp());

        /* Build success response */
        Packet response(MINER_AUTH_RESPONSE);
        
        /* Response format: 1 byte status + 4 bytes session ID */
        response.DATA.push_back(1); // Success
        response.DATA.push_back(nSessionId & 0xFF);
        response.DATA.push_back((nSessionId >> 8) & 0xFF);
        response.DATA.push_back((nSessionId >> 16) & 0xFF);
        response.DATA.push_back((nSessionId >> 24) & 0xFF);

        debug::log(0, FUNCTION, "Falcon auth succeeded for key ", hashKeyID.SubString());

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

} // namespace LLP
