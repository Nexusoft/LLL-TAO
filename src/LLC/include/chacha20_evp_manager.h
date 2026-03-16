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
            state.setSeenNonces.clear();
            state.deqNonceOrder.clear();
        }

        void RotateSession(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];
            Zeroize(state.vSessionKey);
            state.vSessionKey = vSessionKey;
            state.setSeenNonces.clear();
            state.deqNonceOrder.clear();
            ++state.nEpoch;
        }

        void TeardownSession(const uint32_t nSessionId)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
                return;

            Zeroize(it->second.vSessionKey);
            it->second.setSeenNonces.clear();
            it->second.deqNonceOrder.clear();
            mapSessions.erase(it);
        }

        bool EncryptPacket(
            const uint32_t nSessionId,
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

            std::vector<uint8_t> vNonce(LLP::NONCE_BYTES);
            uint64_t nRand1 = LLC::GetRand();
            uint64_t nRand2 = LLC::GetRand();
            std::memcpy(&vNonce[0], &nRand1, 8);
            std::memcpy(&vNonce[8], &nRand2, 4);

            const std::vector<uint8_t> vKey = ResolveSessionKey(nSessionId, vSessionKey);
            if(vKey.size() != 32 || vPlaintext.empty())
                return false;

            std::vector<uint8_t> vCiphertext;
            std::vector<uint8_t> vTag;
            if(!LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag, vAAD))
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
            return true;
        }

        bool DecryptPacket(
            const uint32_t nSessionId,
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

            const std::string strNonce(reinterpret_cast<const char*>(vEncrypted.data() + nNonceOffset), LLP::NONCE_BYTES);
            if(!TrackInboundNonce(nSessionId, strNonce))
                return false;

            const std::vector<uint8_t> vKey = ResolveSessionKey(nSessionId, vSessionKey);
            if(vKey.size() != 32)
                return false;

            std::vector<uint8_t> vNonce(vEncrypted.begin() + nNonceOffset, vEncrypted.begin() + nCiphertextOffset);
            std::vector<uint8_t> vTag(vEncrypted.begin() + nTagOffset, vEncrypted.end());
            std::vector<uint8_t> vCiphertext(vEncrypted.begin() + nCiphertextOffset, vEncrypted.begin() + nTagOffset);

            if(!LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vPlaintext, vAAD))
            {
                ForgetInboundNonce(nSessionId, strNonce);
                return false;
            }

            return true;
        }

    private:
        struct SessionState
        {
            std::vector<uint8_t> vSessionKey;
            std::unordered_set<std::string> setSeenNonces;
            std::deque<std::string> deqNonceOrder;
            uint64_t nEpoch = 0;
        };

        static constexpr size_t MAX_TRACKED_NONCES = 4096;

        mutable std::mutex MUTEX;
        std::unordered_map<uint32_t, SessionState> mapSessions;

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

        bool TrackInboundNonce(const uint32_t nSessionId, const std::string& strNonce)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            SessionState& state = mapSessions[nSessionId];

            if(state.setSeenNonces.find(strNonce) != state.setSeenNonces.end())
                return false;

            state.setSeenNonces.insert(strNonce);
            state.deqNonceOrder.push_back(strNonce);

            while(state.deqNonceOrder.size() > MAX_TRACKED_NONCES)
            {
                const std::string& oldest = state.deqNonceOrder.front();
                state.setSeenNonces.erase(oldest);
                state.deqNonceOrder.pop_front();
            }

            return true;
        }

        void ForgetInboundNonce(const uint32_t nSessionId, const std::string& strNonce)
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            auto it = mapSessions.find(nSessionId);
            if(it == mapSessions.end())
                return;

            it->second.setSeenNonces.erase(strNonce);
        }
    };
}

#endif
