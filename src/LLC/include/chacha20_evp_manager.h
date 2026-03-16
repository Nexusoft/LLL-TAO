/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_CHACHA20_EVP_MANAGER_H
#define NEXUS_LLC_INCLUDE_CHACHA20_EVP_MANAGER_H

#include <LLC/include/chacha20_helpers.h>
#include <LLC/include/encrypt.h>
#include <LLP/include/crypto_envelope.h>
#include <LLP/include/node_crypto_mode_selector.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LLC
{
    class ChaCha20EvpManager
    {
    public:
        struct MetricsSnapshot
        {
            uint64_t nEncryptOk = 0;
            uint64_t nDecryptOk = 0;
            uint64_t nDecryptAuthFail = 0;
            uint64_t nNonceReject = 0;
            uint64_t nStaleSessionDrop = 0;
        };

        static ChaCha20EvpManager& Instance()
        {
            static ChaCha20EvpManager instance;
            return instance;
        }

        void InitSession(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            Zeroize(state.vSessionKey);
            state.vSessionKey = vSessionKey;
            state.nEpoch = 0;
            state.nTxNonce = 0;
            state.nRxEpoch = 0;
            state.nRxCounter = 0;
            state.fHasRxNonce = false;
            debug::log(2, FUNCTION, "session_bound sid=", nSessionId, " gen=", state.nEpoch, " mode=", LLP::NodeCryptoModeString(CurrentMode()));
        }

        void RotateSession(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            Zeroize(state.vSessionKey);
            state.vSessionKey = vSessionKey;
            ++state.nEpoch;
            state.nTxNonce = 0;
            state.nRxEpoch = 0;
            state.nRxCounter = 0;
            state.fHasRxNonce = false;
            debug::log(2, FUNCTION, "rekey_done sid=", nSessionId, " gen=", state.nEpoch, " mode=", LLP::NodeCryptoModeString(CurrentMode()));
        }

        void TeardownSession(const uint32_t nSessionId)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
                return;

            Zeroize(it->second.vSessionKey);
            debug::log(2, FUNCTION, "session_teardown sid=", nSessionId, " gen=", it->second.nEpoch, " mode=", LLP::NodeCryptoModeString(CurrentMode()));
            mapSessions.erase(it);
        }

        uint64_t SessionGeneration(const uint32_t nSessionId) const
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
                return 0;

            return it->second.nEpoch;
        }

        MetricsSnapshot GetMetricsSnapshot() const
        {
            MetricsSnapshot snapshot;
            snapshot.nEncryptOk = nEncryptOk.load(std::memory_order_relaxed);
            snapshot.nDecryptOk = nDecryptOk.load(std::memory_order_relaxed);
            snapshot.nDecryptAuthFail = nDecryptAuthFail.load(std::memory_order_relaxed);
            snapshot.nNonceReject = nNonceReject.load(std::memory_order_relaxed);
            snapshot.nStaleSessionDrop = nStaleSessionDrop.load(std::memory_order_relaxed);
            return snapshot;
        }

        void ResetMetrics()
        {
            nEncryptOk.store(0, std::memory_order_relaxed);
            nDecryptOk.store(0, std::memory_order_relaxed);
            nDecryptAuthFail.store(0, std::memory_order_relaxed);
            nNonceReject.store(0, std::memory_order_relaxed);
            nStaleSessionDrop.store(0, std::memory_order_relaxed);
        }

        bool EncryptPacket(
            const uint32_t nSessionId,
            const uint16_t nMessageType,
            const std::vector<uint8_t>& vSessionKey,
            const std::vector<uint8_t>& vPlaintext,
            std::vector<uint8_t>& vEncrypted,
            const std::vector<uint8_t>& vAAD = std::vector<uint8_t>())
        {
            if(CurrentMode() != LLP::NodeCryptoMode::EVP)
            {
                vEncrypted = EncryptPayloadChaCha20(vPlaintext, vSessionKey, vAAD);
                return !vEncrypted.empty();
            }

            const LLP::CryptoPhase phase = LLP::ResolveCryptoPhaseFromSessionId(nSessionId);
            if(phase != LLP::CryptoPhase::PHASE_SESSION_BOUND)
            {
                debug::error(FUNCTION, "Reject encrypt in EVP mode: phase=", LLP::CryptoPhaseString(phase),
                             " sid=", nSessionId, " mode=evp");
                return false;
            }

            if(nMessageType == 0)
                return false;

            std::vector<uint8_t> vNonce = NextOutboundNonce(nSessionId);

            const std::vector<uint8_t> vKey = ResolveSessionKey(nSessionId, vSessionKey);
            if(vKey.size() != 32 || vPlaintext.empty())
                return false;

            const std::vector<uint8_t> vEffectiveAAD = LLP::BuildAadV1(
                nSessionId,
                nMessageType,
                static_cast<uint32_t>(vPlaintext.size()),
                vAAD);

            std::vector<uint8_t> vCiphertext;
            std::vector<uint8_t> vTag;
            if(!LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag, vEffectiveAAD))
                return false;

            static_assert(LLP::SESSION_ID_BYTES == sizeof(uint32_t), "Session ID wire size must remain 4 bytes");
            vEncrypted.clear();
            vEncrypted.reserve(LLP::EncryptedOverheadBytes(LLP::NodeCryptoMode::EVP) + vCiphertext.size());
            vEncrypted.push_back(LLP::ENVELOPE_WIRE_VERSION_V1);
            vEncrypted.push_back(LLP::ENVELOPE_FLAGS_DEFAULT);
            vEncrypted.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
            vEncrypted.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
            vEncrypted.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
            vEncrypted.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));
            vEncrypted.insert(vEncrypted.end(), vNonce.begin(), vNonce.end());
            vEncrypted.insert(vEncrypted.end(), vCiphertext.begin(), vCiphertext.end());
            vEncrypted.insert(vEncrypted.end(), vTag.begin(), vTag.end());
            nEncryptOk.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        bool DecryptPacket(
            const uint32_t nSessionId,
            const uint16_t nMessageType,
            const std::vector<uint8_t>& vSessionKey,
            const std::vector<uint8_t>& vEncrypted,
            std::vector<uint8_t>& vPlaintext,
            const std::vector<uint8_t>& vAAD = std::vector<uint8_t>())
        {
            if(CurrentMode() != LLP::NodeCryptoMode::EVP)
                return DecryptPayloadChaCha20(vEncrypted, vSessionKey, vPlaintext, vAAD);

            const LLP::CryptoPhase phase = LLP::ResolveCryptoPhaseFromSessionId(nSessionId);
            if(phase != LLP::CryptoPhase::PHASE_SESSION_BOUND)
            {
                debug::error(FUNCTION, "Reject decrypt in EVP mode: phase=", LLP::CryptoPhaseString(phase),
                             " sid=", nSessionId, " mode=evp");
                return false;
            }

            if(nMessageType == 0)
                return false;

            const size_t nMinBytes = LLP::MinEncryptedFrameBytes(LLP::NodeCryptoMode::EVP);
            if(vEncrypted.size() < nMinBytes)
            {
                debug::error(FUNCTION, "EVP frame too small: mode=evp frame=", vEncrypted.size(),
                             " expected_min=", nMinBytes, " sid=", nSessionId);
                return false;
            }

            const uint8_t nVersion = vEncrypted[0];
            if(nVersion != LLP::ENVELOPE_WIRE_VERSION_V1)
            {
                debug::error(FUNCTION, "EVP wire version mismatch: mode=evp version=", static_cast<uint32_t>(nVersion),
                             " expected=", static_cast<uint32_t>(LLP::ENVELOPE_WIRE_VERSION_V1), " sid=", nSessionId);
                return false;
            }

            const uint8_t nFlags = vEncrypted[1];
            if(nFlags != LLP::ENVELOPE_FLAGS_DEFAULT)
            {
                debug::error(FUNCTION, "EVP flags mismatch: mode=evp flags=", static_cast<uint32_t>(nFlags),
                             " expected=", static_cast<uint32_t>(LLP::ENVELOPE_FLAGS_DEFAULT), " sid=", nSessionId);
                return false;
            }

            const uint32_t nEnvelopeSessionId = DeserializeUint32LE(vEncrypted.data() + 2);

            if(nEnvelopeSessionId != nSessionId)
            {
                debug::error(FUNCTION, "EVP session mismatch: mode=evp frame=", vEncrypted.size(),
                             " sid=", nSessionId, " envelope_sid=", nEnvelopeSessionId,
                             " expected_min=", nMinBytes);
                return false;
            }

            const size_t nNonceOffset = LLP::WIRE_VERSION_BYTES + LLP::CRYPTO_FLAGS_BYTES + LLP::SESSION_ID_BYTES;
            const size_t nCiphertextOffset = nNonceOffset + LLP::NONCE_BYTES;
            const size_t nTagOffset = vEncrypted.size() - LLP::AEAD_TAG_BYTES;
            const std::array<uint8_t, LLP::NONCE_BYTES> nonceBytes = ReadNonce(vEncrypted.data() + nNonceOffset);
            const uint32_t nFrameEpoch = NonceEpoch(nonceBytes);
            if(!ValidateInboundEpoch(nSessionId, nFrameEpoch))
            {
                nStaleSessionDrop.fetch_add(1, std::memory_order_relaxed);
                debug::error(FUNCTION, "EVP stale_session_drop: sid=", nSessionId, " frame_gen=", nFrameEpoch,
                             " active_gen=", SessionGeneration(nSessionId), " mode=evp");
                return false;
            }

            if(!TrackInboundNonce(nSessionId, nonceBytes))
            {
                nNonceReject.fetch_add(1, std::memory_order_relaxed);
                debug::error(FUNCTION, "EVP nonce_reject: sid=", nSessionId, " gen=", SessionGeneration(nSessionId), " mode=evp");
                return false;
            }

            const std::vector<uint8_t> vKey = ResolveSessionKey(nSessionId, vSessionKey);
            if(vKey.size() != 32)
                return false;

            std::vector<uint8_t> vNonce(vEncrypted.begin() + nNonceOffset, vEncrypted.begin() + nCiphertextOffset);
            std::vector<uint8_t> vTag(vEncrypted.begin() + nTagOffset, vEncrypted.end());
            std::vector<uint8_t> vCiphertext(vEncrypted.begin() + nCiphertextOffset, vEncrypted.begin() + nTagOffset);
            const std::vector<uint8_t> vEffectiveAAD = LLP::BuildAadV1(
                nSessionId,
                nMessageType,
                static_cast<uint32_t>(vCiphertext.size()),
                vAAD);

            if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vPlaintext, vEffectiveAAD))
            {
                ForgetInboundNonce(nSessionId, nonceBytes);
                nDecryptAuthFail.fetch_add(1, std::memory_order_relaxed);
                return false;
            }

            nDecryptOk.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

    private:
        struct SessionState
        {
            std::vector<uint8_t> vSessionKey;
            uint64_t nEpoch = 0;
            uint64_t nTxNonce = 0;
            uint32_t nRxEpoch = 0;
            uint64_t nRxCounter = 0;
            bool fHasRxNonce = false;
        };

        mutable std::mutex MUTEX;
        std::unordered_map<uint32_t, SessionState> mapSessions;
        std::atomic<uint64_t> nEncryptOk{0};
        std::atomic<uint64_t> nDecryptOk{0};
        std::atomic<uint64_t> nDecryptAuthFail{0};
        std::atomic<uint64_t> nNonceReject{0};
        std::atomic<uint64_t> nStaleSessionDrop{0};

        ChaCha20EvpManager() = default;

        static LLP::NodeCryptoMode CurrentMode()
        {
            return LLP::GetNodeCryptoMode();
        }

        static void Zeroize(std::vector<uint8_t>& vData)
        {
            std::fill(vData.begin(), vData.end(), 0);
        }

        static uint32_t DeserializeUint32LE(const uint8_t* pData)
        {
            if(pData == nullptr)
                return 0;

            return static_cast<uint32_t>(pData[0]) |
                (static_cast<uint32_t>(pData[1]) << 8) |
                (static_cast<uint32_t>(pData[2]) << 16) |
                (static_cast<uint32_t>(pData[3]) << 24);
        }

        static void SerializeUint32LE(std::vector<uint8_t>& vOut, const uint32_t nValue)
        {
            vOut.push_back(static_cast<uint8_t>(nValue & 0xFF));
            vOut.push_back(static_cast<uint8_t>((nValue >> 8) & 0xFF));
            vOut.push_back(static_cast<uint8_t>((nValue >> 16) & 0xFF));
            vOut.push_back(static_cast<uint8_t>((nValue >> 24) & 0xFF));
        }

        static void SerializeUint64LE(std::vector<uint8_t>& vOut, const uint64_t nValue)
        {
            for(uint32_t i = 0; i < 8; ++i)
                vOut.push_back(static_cast<uint8_t>((nValue >> (i * 8)) & 0xFF));
        }

        static uint64_t DeserializeUint64LE(const uint8_t* pData)
        {
            uint64_t nValue = 0;
            for(uint32_t i = 0; i < 8; ++i)
                nValue |= (static_cast<uint64_t>(pData[i]) << (i * 8));
            return nValue;
        }

        static std::array<uint8_t, LLP::NONCE_BYTES> ReadNonce(const uint8_t* pData)
        {
            std::array<uint8_t, LLP::NONCE_BYTES> nonce{};
            std::memcpy(nonce.data(), pData, LLP::NONCE_BYTES);
            return nonce;
        }

        static uint32_t NonceEpoch(const std::array<uint8_t, LLP::NONCE_BYTES>& nonce)
        {
            return DeserializeUint32LE(nonce.data());
        }

        static uint64_t NonceCounter(const std::array<uint8_t, LLP::NONCE_BYTES>& nonce)
        {
            return DeserializeUint64LE(nonce.data() + sizeof(uint32_t));
        }

        std::vector<uint8_t> NextOutboundNonce(const uint32_t nSessionId)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            ++state.nTxNonce;

            std::vector<uint8_t> vNonce;
            vNonce.reserve(LLP::NONCE_BYTES);
            SerializeUint32LE(vNonce, static_cast<uint32_t>(state.nEpoch & 0xFFFFFFFFu));
            SerializeUint64LE(vNonce, state.nTxNonce);
            return vNonce;
        }

        std::vector<uint8_t> ResolveSessionKey(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
            {
                SessionState state;
                state.vSessionKey = vSessionKey;
                auto [insertedIt, fInserted] = mapSessions.emplace(nSessionId, std::move(state));
                (void)fInserted;
                return insertedIt->second.vSessionKey;
            }

            if(it->second.vSessionKey.empty() && !vSessionKey.empty())
                it->second.vSessionKey = vSessionKey;

            return it->second.vSessionKey;
        }

        bool TrackInboundNonce(const uint32_t nSessionId, const std::array<uint8_t, LLP::NONCE_BYTES>& nonce)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            const uint32_t nEpoch = NonceEpoch(nonce);
            const uint64_t nCounter = NonceCounter(nonce);

            if(state.fHasRxNonce)
            {
                if(nEpoch < state.nRxEpoch || (nEpoch == state.nRxEpoch && nCounter <= state.nRxCounter))
                    return false;
            }

            state.nRxEpoch = nEpoch;
            state.nRxCounter = nCounter;
            state.fHasRxNonce = true;
            return true;
        }

        bool ValidateInboundEpoch(const uint32_t nSessionId, const uint32_t nFrameEpoch)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            const uint32_t nActiveEpoch = static_cast<uint32_t>(state.nEpoch & 0xFFFFFFFFu);
            return nFrameEpoch == nActiveEpoch;
        }

        void ForgetInboundNonce(const uint32_t nSessionId, const std::array<uint8_t, LLP::NONCE_BYTES>&)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
                return;
        }
    };
}

#endif
