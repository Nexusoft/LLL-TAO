/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CRYPTO_ENVELOPE_H
#define NEXUS_LLP_INCLUDE_CRYPTO_ENVELOPE_H

#include <LLP/include/node_crypto_mode_selector.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace LLP
{
    static constexpr size_t SESSION_ID_BYTES = sizeof(uint32_t);
    /* Session ID is serialized in little-endian byte order on wire (v1). */
    static constexpr size_t NONCE_BYTES = 12;
    static constexpr size_t AEAD_TAG_BYTES = 16;
    static constexpr size_t WIRE_VERSION_BYTES = 1;
    static constexpr size_t CRYPTO_FLAGS_BYTES = 1;

    static constexpr uint8_t ENVELOPE_WIRE_VERSION_V1 = 0x01;
    /* Envelope flags byte for future capabilities negotiation (compression/options bits). */
    static constexpr uint8_t ENVELOPE_FLAGS_DEFAULT   = 0x00;
    static constexpr size_t LEGACY_STALE_FRAME_LIMIT_BYTES = (2u * 1024u * 1024u);
    static constexpr size_t AAD_MESSAGE_TYPE_BYTES = sizeof(uint16_t);
    static constexpr size_t AAD_PAYLOAD_LENGTH_BYTES = sizeof(uint32_t);

    enum class CryptoPhase : uint8_t
    {
        PHASE_PRE_AUTH = 0,
        PHASE_SESSION_BOUND
    };

    constexpr const char* CryptoPhaseString(const CryptoPhase phase)
    {
        return phase == CryptoPhase::PHASE_SESSION_BOUND ? "PHASE_SESSION_BOUND" : "PHASE_PRE_AUTH";
    }

    constexpr CryptoPhase ResolveCryptoPhase(const bool fAuthenticated, const uint32_t nSessionId)
    {
        return (fAuthenticated && nSessionId != 0) ? CryptoPhase::PHASE_SESSION_BOUND : CryptoPhase::PHASE_PRE_AUTH;
    }

    constexpr CryptoPhase ResolveCryptoPhaseFromSessionId(const uint32_t nSessionId)
    {
        return nSessionId != 0 ? CryptoPhase::PHASE_SESSION_BOUND : CryptoPhase::PHASE_PRE_AUTH;
    }

    constexpr size_t EncryptedOverheadBytes(const NodeCryptoMode mode)
    {
        if(mode == NodeCryptoMode::EVP)
            return WIRE_VERSION_BYTES + CRYPTO_FLAGS_BYTES + SESSION_ID_BYTES + NONCE_BYTES + AEAD_TAG_BYTES;

        return NONCE_BYTES + AEAD_TAG_BYTES;
    }

    constexpr size_t MinEncryptedFrameBytes(const NodeCryptoMode mode)
    {
        return EncryptedOverheadBytes(mode);
    }

    constexpr size_t MaxEncryptedPayloadBytes(const NodeCryptoMode mode, const size_t nMaxPlaintextBytes)
    {
        if(nMaxPlaintextBytes > (std::numeric_limits<size_t>::max() - EncryptedOverheadBytes(mode)))
            return 0;

        return nMaxPlaintextBytes + EncryptedOverheadBytes(mode);
    }

    inline std::vector<uint8_t> BuildAadV1(
        const uint32_t nSessionId,
        const uint16_t nMessageType,
        const uint32_t nPayloadLength,
        const std::vector<uint8_t>& vContext = std::vector<uint8_t>())
    {
        std::vector<uint8_t> vAAD;
        vAAD.reserve(WIRE_VERSION_BYTES + SESSION_ID_BYTES + AAD_MESSAGE_TYPE_BYTES + AAD_PAYLOAD_LENGTH_BYTES + vContext.size());

        vAAD.push_back(ENVELOPE_WIRE_VERSION_V1);
        vAAD.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>(nMessageType & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nMessageType >> 8) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>(nPayloadLength & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nPayloadLength >> 8) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nPayloadLength >> 16) & 0xFF));
        vAAD.push_back(static_cast<uint8_t>((nPayloadLength >> 24) & 0xFF));
        vAAD.insert(vAAD.end(), vContext.begin(), vContext.end());
        return vAAD;
    }
}

#endif
