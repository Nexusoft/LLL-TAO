/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/falcon_handshake.h>
#include <LLP/include/node_cache.h>

#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>
#include <Util/include/hex.h>

#include <openssl/evp.h>
#include <cstring>

namespace LLP
{
namespace FalconHandshake
{
    /** HandshakeResult factory methods */
    HandshakeResult HandshakeResult::Success(const HandshakeData& data_, const uint256_t& keyId_)
    {
        HandshakeResult result;
        result.fSuccess = true;
        result.strError = "";
        result.data = data_;
        result.hashKeyID = keyId_;
        return result;
    }

    HandshakeResult HandshakeResult::Failure(const std::string& error)
    {
        HandshakeResult result;
        result.fSuccess = false;
        result.strError = error;
        result.data = HandshakeData();
        result.hashKeyID = 0;
        return result;
    }


    /** EncryptFalconPublicKey
     *
     *  Encrypt using ChaCha20 (OpenSSL EVP interface).
     *
     **/
    std::vector<uint8_t> EncryptFalconPublicKey(
        const std::vector<uint8_t>& vPubKey,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce
    )
    {
        /* Validate inputs */
        if(vPubKey.empty())
        {
            debug::error(FUNCTION, "Cannot encrypt empty public key");
            return std::vector<uint8_t>();
        }

        if(vKey.size() != 32)
        {
            debug::error(FUNCTION, "ChaCha20 key must be 32 bytes, got ", vKey.size());
            return std::vector<uint8_t>();
        }

        if(vNonce.size() != 12)
        {
            debug::error(FUNCTION, "ChaCha20 nonce must be 12 bytes, got ", vNonce.size());
            return std::vector<uint8_t>();
        }

        /* Prepare output buffer */
        std::vector<uint8_t> vCiphertext(vPubKey.size());

        /* Create and initialize context */
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
        {
            debug::error(FUNCTION, "Failed to create cipher context");
            return std::vector<uint8_t>();
        }

        /* Initialize encryption with ChaCha20 */
        if(EVP_EncryptInit_ex(ctx, EVP_chacha20(), nullptr, vKey.data(), vNonce.data()) != 1)
        {
            debug::error(FUNCTION, "Failed to initialize ChaCha20 encryption");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Perform encryption */
        int nCiphertextLen = 0;
        if(EVP_EncryptUpdate(ctx, vCiphertext.data(), &nCiphertextLen, 
                             vPubKey.data(), static_cast<int>(vPubKey.size())) != 1)
        {
            debug::error(FUNCTION, "ChaCha20 encryption failed");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Finalize encryption (no-op for ChaCha20, but required by API) */
        int nFinalLen = 0;
        if(EVP_EncryptFinal_ex(ctx, vCiphertext.data() + nCiphertextLen, &nFinalLen) != 1)
        {
            debug::error(FUNCTION, "ChaCha20 encryption finalization failed");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        /* Resize to actual ciphertext length */
        vCiphertext.resize(nCiphertextLen + nFinalLen);

        return vCiphertext;
    }


    /** DecryptFalconPublicKey
     *
     *  Decrypt using ChaCha20 (OpenSSL EVP interface).
     *
     **/
    std::vector<uint8_t> DecryptFalconPublicKey(
        const std::vector<uint8_t>& vEncrypted,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce
    )
    {
        /* Validate inputs */
        if(vEncrypted.empty())
        {
            debug::error(FUNCTION, "Cannot decrypt empty ciphertext");
            return std::vector<uint8_t>();
        }

        if(vKey.size() != 32)
        {
            debug::error(FUNCTION, "ChaCha20 key must be 32 bytes, got ", vKey.size());
            return std::vector<uint8_t>();
        }

        if(vNonce.size() != 12)
        {
            debug::error(FUNCTION, "ChaCha20 nonce must be 12 bytes, got ", vNonce.size());
            return std::vector<uint8_t>();
        }

        /* Prepare output buffer */
        std::vector<uint8_t> vPlaintext(vEncrypted.size());

        /* Create and initialize context */
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
        {
            debug::error(FUNCTION, "Failed to create cipher context");
            return std::vector<uint8_t>();
        }

        /* Initialize decryption with ChaCha20 */
        if(EVP_DecryptInit_ex(ctx, EVP_chacha20(), nullptr, vKey.data(), vNonce.data()) != 1)
        {
            debug::error(FUNCTION, "Failed to initialize ChaCha20 decryption");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Perform decryption */
        int nPlaintextLen = 0;
        if(EVP_DecryptUpdate(ctx, vPlaintext.data(), &nPlaintextLen, 
                             vEncrypted.data(), static_cast<int>(vEncrypted.size())) != 1)
        {
            debug::error(FUNCTION, "ChaCha20 decryption failed");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Finalize decryption */
        int nFinalLen = 0;
        if(EVP_DecryptFinal_ex(ctx, vPlaintext.data() + nPlaintextLen, &nFinalLen) != 1)
        {
            debug::error(FUNCTION, "ChaCha20 decryption finalization failed");
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        /* Resize to actual plaintext length */
        vPlaintext.resize(nPlaintextLen + nFinalLen);

        return vPlaintext;
    }


    /** GenerateSessionKey
     *
     *  Derive session key from genesis hash and miner public key.
     *
     **/
    uint256_t GenerateSessionKey(
        const uint256_t& hashGenesis,
        const std::vector<uint8_t>& vMinerPubKey,
        uint64_t nTimestamp
    )
    {
        /* Build session key derivation input */
        std::vector<uint8_t> vInput;

        /* Add genesis hash */
        std::vector<uint8_t> vGenesis = hashGenesis.GetBytes();
        vInput.insert(vInput.end(), vGenesis.begin(), vGenesis.end());

        /* Add miner public key */
        vInput.insert(vInput.end(), vMinerPubKey.begin(), vMinerPubKey.end());

        /* Add timestamp (rounded to hour for session stability) */
        uint64_t nRoundedTime = (nTimestamp / 3600) * 3600;
        for(size_t i = 0; i < 8; ++i)
        {
            vInput.push_back(static_cast<uint8_t>((nRoundedTime >> (i * 8)) & 0xFF));
        }

        /* Hash to derive session key */
        return LLC::SK256(vInput);
    }


    /** BuildHandshakePacket
     *
     *  Construct handshake packet with optional encryption.
     *
     **/
    std::vector<uint8_t> BuildHandshakePacket(
        const HandshakeConfig& config,
        const HandshakeData& data,
        const std::vector<uint8_t>& vEncryptionKey
    )
    {
        std::vector<uint8_t> vPacket;

        /* Determine if we should encrypt */
        bool fEncrypt = config.fEnableEncryption && !vEncryptionKey.empty();

        /* Add encryption flag (1 byte) */
        vPacket.push_back(fEncrypt ? 1 : 0);

        /* Prepare public key (encrypt if needed) */
        std::vector<uint8_t> vPubKeyData = data.vFalconPubKey;
        if(fEncrypt)
        {
            /* Generate random nonce */
            std::vector<uint8_t> vNonce = LLC::GetRand256().GetBytes();
            vNonce.resize(12);  // ChaCha20 uses 12-byte nonce

            /* Encrypt public key */
            vPubKeyData = EncryptFalconPublicKey(data.vFalconPubKey, vEncryptionKey, vNonce);
            if(vPubKeyData.empty())
            {
                debug::error(FUNCTION, "Failed to encrypt public key");
                return std::vector<uint8_t>();
            }

            /* Add nonce to packet (needed for decryption) */
            vPacket.insert(vPacket.end(), vNonce.begin(), vNonce.end());
        }

        /* Add public key length (2 bytes, big-endian) */
        uint16_t nPubKeyLen = static_cast<uint16_t>(vPubKeyData.size());
        vPacket.push_back((nPubKeyLen >> 8) & 0xFF);
        vPacket.push_back(nPubKeyLen & 0xFF);

        /* Add public key */
        vPacket.insert(vPacket.end(), vPubKeyData.begin(), vPubKeyData.end());

        /* Add genesis hash (32 bytes) */
        std::vector<uint8_t> vGenesis = data.hashGenesis.GetBytes();
        vPacket.insert(vPacket.end(), vGenesis.begin(), vGenesis.end());

        /* Add session key (32 bytes) */
        std::vector<uint8_t> vSessionKey = data.hashSessionKey.GetBytes();
        vPacket.insert(vPacket.end(), vSessionKey.begin(), vSessionKey.end());

        /* Add timestamp (8 bytes, little-endian) */
        for(size_t i = 0; i < 8; ++i)
        {
            vPacket.push_back(static_cast<uint8_t>((data.nTimestamp >> (i * 8)) & 0xFF));
        }

        /* Add miner ID length and data (optional) */
        uint16_t nMinerIDLen = static_cast<uint16_t>(data.strMinerID.size());
        vPacket.push_back((nMinerIDLen >> 8) & 0xFF);
        vPacket.push_back(nMinerIDLen & 0xFF);
        if(nMinerIDLen > 0)
        {
            vPacket.insert(vPacket.end(), data.strMinerID.begin(), data.strMinerID.end());
        }

        return vPacket;
    }


    /** ParseHandshakePacket
     *
     *  Parse and validate handshake packet.
     *
     **/
    HandshakeResult ParseHandshakePacket(
        const std::vector<uint8_t>& vPacket,
        const HandshakeConfig& config,
        const std::vector<uint8_t>& vEncryptionKey
    )
    {
        /* Minimum packet size check */
        if(vPacket.size() < 77)  // 1 + 2 + 32 + 32 + 8 + 2 = 77 bytes minimum
        {
            return HandshakeResult::Failure("Packet too small for handshake");
        }

        size_t nOffset = 0;
        HandshakeData data;

        /* Read encryption flag */
        bool fEncrypted = (vPacket[nOffset++] == 1);
        data.fEncrypted = fEncrypted;

        /* Read nonce if encrypted */
        std::vector<uint8_t> vNonce;
        if(fEncrypted)
        {
            if(vPacket.size() < nOffset + 12)
                return HandshakeResult::Failure("Packet too small for nonce");

            vNonce.assign(vPacket.begin() + nOffset, vPacket.begin() + nOffset + 12);
            nOffset += 12;
        }

        /* Read public key length */
        if(vPacket.size() < nOffset + 2)
            return HandshakeResult::Failure("Packet too small for pubkey length");

        uint16_t nPubKeyLen = (static_cast<uint16_t>(vPacket[nOffset]) << 8) 
                            | static_cast<uint16_t>(vPacket[nOffset + 1]);
        nOffset += 2;

        /* Read public key */
        if(vPacket.size() < nOffset + nPubKeyLen)
            return HandshakeResult::Failure("Packet too small for pubkey data");

        std::vector<uint8_t> vPubKeyData(vPacket.begin() + nOffset, 
                                         vPacket.begin() + nOffset + nPubKeyLen);
        nOffset += nPubKeyLen;

        /* Decrypt public key if encrypted */
        if(fEncrypted)
        {
            if(vEncryptionKey.empty())
                return HandshakeResult::Failure("Encrypted handshake but no key provided");

            data.vFalconPubKey = DecryptFalconPublicKey(vPubKeyData, vEncryptionKey, vNonce);
            if(data.vFalconPubKey.empty())
                return HandshakeResult::Failure("Failed to decrypt public key");
        }
        else
        {
            /* Check if encryption is required */
            if(config.fRequireEncryption)
                return HandshakeResult::Failure("Encryption required but handshake not encrypted");

            data.vFalconPubKey = vPubKeyData;
        }

        /* Read genesis hash */
        if(vPacket.size() < nOffset + 32)
            return HandshakeResult::Failure("Packet too small for genesis hash");

        data.hashGenesis.SetBytes(std::vector<uint8_t>(vPacket.begin() + nOffset, 
                                                        vPacket.begin() + nOffset + 32));
        nOffset += 32;

        /* Read session key */
        if(vPacket.size() < nOffset + 32)
            return HandshakeResult::Failure("Packet too small for session key");

        data.hashSessionKey.SetBytes(std::vector<uint8_t>(vPacket.begin() + nOffset, 
                                                           vPacket.begin() + nOffset + 32));
        nOffset += 32;

        /* Read timestamp */
        if(vPacket.size() < nOffset + 8)
            return HandshakeResult::Failure("Packet too small for timestamp");

        data.nTimestamp = 0;
        for(size_t i = 0; i < 8; ++i)
        {
            data.nTimestamp |= (static_cast<uint64_t>(vPacket[nOffset + i]) << (i * 8));
        }
        nOffset += 8;

        /* Read miner ID (optional) */
        if(vPacket.size() >= nOffset + 2)
        {
            uint16_t nMinerIDLen = (static_cast<uint16_t>(vPacket[nOffset]) << 8) 
                                 | static_cast<uint16_t>(vPacket[nOffset + 1]);
            nOffset += 2;

            if(nMinerIDLen > 0 && vPacket.size() >= nOffset + nMinerIDLen)
            {
                data.strMinerID.assign(vPacket.begin() + nOffset, 
                                       vPacket.begin() + nOffset + nMinerIDLen);
            }
        }

        /* Derive key ID from public key */
        uint256_t hashKeyID = LLC::SK256(data.vFalconPubKey);

        /* Validate handshake data */
        if(!ValidateHandshakeData(data))
            return HandshakeResult::Failure("Invalid handshake data");

        return HandshakeResult::Success(data, hashKeyID);
    }


    /** ValidateHandshakeData */
    bool ValidateHandshakeData(const HandshakeData& data)
    {
        /* Public key must be present */
        if(data.vFalconPubKey.empty())
            return false;

        /* Genesis hash must be set for authenticated mining */
        /* Zero genesis is only valid during initial key exchange, not for active mining */
        if(data.hashGenesis == 0)
        {
            debug::warning(FUNCTION, "Handshake with zero GenesisHash - only valid for initial setup");
            /* Allow for now but log warning - caller should verify before allowing mining */
        }

        /* Timestamp must be reasonable (within 1 hour for replay protection) */
        uint64_t nNow = runtime::unifiedtimestamp();
        if(data.nTimestamp > nNow + 3600 || data.nTimestamp < nNow - 3600)
        {
            debug::error(FUNCTION, "Handshake timestamp out of range: ", data.nTimestamp, 
                          " vs current ", nNow, " - rejecting for replay protection");
            return false;  // Reject handshakes with invalid timestamps
        }

        return true;
    }


    /** IsLocalhostHandshake */
    bool IsLocalhostHandshake(const std::string& strAddress)
    {
        return NodeCache::IsLocalhost(strAddress);
    }

} // namespace FalconHandshake
} // namespace LLP
