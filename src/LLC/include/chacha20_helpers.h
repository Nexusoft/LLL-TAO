/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/include/encrypt.h>
#include <LLC/include/random.h>
#include <Util/include/debug.h>
#include <vector>
#include <cstring>

namespace LLC
{
    /** EncryptPayloadChaCha20
     *
     *  High-level wrapper for ChaCha20-Poly1305 encryption.
     *  Automatically generates nonce and packages result.
     *
     *  Use cases:
     *  - Mining protocol encryption (reward addresses, etc.)
     *  - Future: Distributed file system encryption
     *  - Future: Augmented contract data encryption
     *
     *  Format: nonce(12) + ciphertext + tag(16)
     *
     *  @param[in] vPlaintext The data to encrypt
     *  @param[in] vKey The 256-bit (32 byte) symmetric key
     *  @param[in] vAAD Additional Authenticated Data (optional, for domain separation)
     *
     *  @return Encrypted payload or empty vector on error
     *
     **/
    inline std::vector<uint8_t> EncryptPayloadChaCha20(
        const std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vAAD = std::vector<uint8_t>()
    )
    {
        /* Validate inputs */
        if(vPlaintext.empty())
        {
            debug::error(FUNCTION, "Cannot encrypt empty plaintext");
            return std::vector<uint8_t>();
        }

        if(vKey.size() != 32)
        {
            debug::error(FUNCTION, "Key must be 32 bytes, got ", vKey.size());
            return std::vector<uint8_t>();
        }

        /* Generate cryptographically secure random 12-byte nonce */
        std::vector<uint8_t> vNonce(12);
        
        uint64_t nRand1 = LLC::GetRand();
        uint64_t nRand2 = LLC::GetRand();
        
        std::memcpy(&vNonce[0], &nRand1, 8);
        std::memcpy(&vNonce[8], &nRand2, 4);

        /* Encrypt using ChaCha20-Poly1305 AEAD */
        std::vector<uint8_t> vCiphertext;
        std::vector<uint8_t> vTag;

        if(!LLC::EncryptChaCha20Poly1305(vPlaintext, vKey, vNonce, vCiphertext, vTag, vAAD))
        {
            debug::error(FUNCTION, "ChaCha20-Poly1305 encryption failed");
            return std::vector<uint8_t>();
        }

        /* Package: nonce(12) + ciphertext + tag(16) */
        std::vector<uint8_t> vEncrypted;
        vEncrypted.reserve(12 + vCiphertext.size() + 16);
        vEncrypted.insert(vEncrypted.end(), vNonce.begin(), vNonce.end());
        vEncrypted.insert(vEncrypted.end(), vCiphertext.begin(), vCiphertext.end());
        vEncrypted.insert(vEncrypted.end(), vTag.begin(), vTag.end());

        return vEncrypted;
    }


    /** DecryptPayloadChaCha20
     *
     *  High-level wrapper for ChaCha20-Poly1305 decryption.
     *  Automatically extracts nonce and tag from packaged payload.
     *
     *  @param[in] vEncrypted Packaged payload (nonce + ciphertext + tag)
     *  @param[in] vKey The 256-bit (32 byte) symmetric key
     *  @param[out] vPlaintext The decrypted data
     *  @param[in] vAAD Additional Authenticated Data (must match encryption)
     *
     *  @return True if decryption and authentication succeeded
     *
     **/
    inline bool DecryptPayloadChaCha20(
        const std::vector<uint8_t>& vEncrypted,
        const std::vector<uint8_t>& vKey,
        std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vAAD = std::vector<uint8_t>()
    )
    {
        /* Validate minimum size: nonce(12) + tag(16) = 28 bytes */
        if(vEncrypted.size() < 28)
        {
            debug::error(FUNCTION, "Encrypted payload too small: ", vEncrypted.size());
            return false;
        }

        if(vKey.size() != 32)
        {
            debug::error(FUNCTION, "Key must be 32 bytes, got ", vKey.size());
            return false;
        }

        /* Extract nonce (first 12 bytes) */
        std::vector<uint8_t> vNonce(vEncrypted.begin(), vEncrypted.begin() + 12);

        /* Extract tag (last 16 bytes) */
        std::vector<uint8_t> vTag(vEncrypted.end() - 16, vEncrypted.end());

        /* Extract ciphertext (middle portion) */
        std::vector<uint8_t> vCiphertext(vEncrypted.begin() + 12, vEncrypted.end() - 16);

        /* Decrypt and verify authentication tag */
        return LLC::DecryptChaCha20Poly1305(vCiphertext, vTag, vKey, vNonce, vPlaintext, vAAD);
    }

} // namespace LLC
