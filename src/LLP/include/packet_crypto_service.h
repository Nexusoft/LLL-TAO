/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_PACKET_CRYPTO_SERVICE_H
#define NEXUS_LLP_INCLUDE_PACKET_CRYPTO_SERVICE_H

#include <LLC/include/chacha20_evp_manager.h>

#include <cstdint>
#include <vector>

namespace LLP
{
    class PacketCryptoService
    {
    public:
        using MetricsSnapshot = LLC::ChaCha20EvpManager::MetricsSnapshot;

        static bool Encode(
            const uint32_t nSessionId,
            const uint16_t nMessageType,
            const std::vector<uint8_t>& vSessionKey,
            const std::vector<uint8_t>& vPlaintext,
            std::vector<uint8_t>& vEncrypted,
            const std::vector<uint8_t>& vAAD = std::vector<uint8_t>())
        {
            return LLC::ChaCha20EvpManager::Instance().EncryptPacket(
                nSessionId, nMessageType, vSessionKey, vPlaintext, vEncrypted, vAAD);
        }

        static bool Decode(
            const uint32_t nSessionId,
            const uint16_t nMessageType,
            const std::vector<uint8_t>& vSessionKey,
            const std::vector<uint8_t>& vEncrypted,
            std::vector<uint8_t>& vPlaintext,
            const std::vector<uint8_t>& vAAD = std::vector<uint8_t>())
        {
            return LLC::ChaCha20EvpManager::Instance().DecryptPacket(
                nSessionId, nMessageType, vSessionKey, vEncrypted, vPlaintext, vAAD);
        }

        static MetricsSnapshot Metrics()
        {
            return LLC::ChaCha20EvpManager::Instance().GetMetricsSnapshot();
        }

        static void ResetMetrics()
        {
            LLC::ChaCha20EvpManager::Instance().ResetMetrics();
        }
    };
}

#endif
