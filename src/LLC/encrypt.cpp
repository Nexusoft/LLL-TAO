/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/encrypt.h>
#include <LLC/include/random.h>
#include <LLC/aes/aes.h>

#include <Util/include/debug.h>

#include <openssl/evp.h>
#include <cstring>


namespace LLC
{

    /* Encrypts the data using the AES 256 function and the specified symmetric key. */
    bool EncryptAES256(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vPlaintext, std::vector<uint8_t> &vCipherText)
    {
        /* Check the key length is correct alignment. */
        if(vKey.size() != AES_KEYLEN)
            return debug::error(FUNCTION, "Invalid key length");

        /* Check the incoming data is not empty */
        if(vPlaintext.empty())
            return debug::error(FUNCTION, "Plain text is empty");

        /* First copy our initialzation vector to ciphertext. */
        vCipherText =
            LLC::GetRand128().GetBytes(); //initialization vector is 16 bytes which is a multiple of block length

        /* Copy the plain text data into the ciphertext buffer to pass to the AES function */
        vCipherText.insert(vCipherText.end(), vPlaintext.begin(), vPlaintext.end());

        /* Number of bytes to pad so that the ciphertext is a multiple of AES_BLOCKLEN */
        const uint8_t nPadBytes =
            (AES_BLOCKLEN - ((vCipherText.size() + 1) % AES_BLOCKLEN)); //+1 to track our padding length

        /* Add our ciphertext padding now. */
        vCipherText.resize(vCipherText.size() + nPadBytes + 1, 0); //+1 for the padding size
        vCipherText.back() = nPadBytes; //track the extra padding required


        /* Do the encryption */
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, &vKey[0], &vCipherText[0]); //we store IV as first AES_BLOCKLEN bytes
        AES_CBC_encrypt_buffer(&ctx, &vCipherText[AES_BLOCKLEN], vCipherText.size() - AES_BLOCKLEN); //we want the IV as the first 16 bytes

        /* return true */
        return true;
    }


    /* Decrypts the data using the AES 256 function and the specified symmetric key. */
    bool DecryptAES256(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vCipherText, std::vector<uint8_t> &vPlainText)
    {
        /* Check the key length is a valid AES key. */
        if(vKey.size() != AES_KEYLEN)
            return debug::error(FUNCTION, "Invalid key length");

        /* Check size of the encrypted data vector.  This must be at least 32 bytes, 16 for the IV and 16 for the minimum
           AES block size */
        if(vCipherText.size() < 32)
            return debug::error(FUNCTION, "Encrypted data size too small");

        /* Ciphertext length must be a multiple of the AES block size (16) */
        if(vCipherText.size() % AES_BLOCKLEN != 0)
            return debug::error(FUNCTION, "Ciphertext needs to be multiple of AES blocklength");

        /* Copy over our initialization vector. */
        const std::vector<uint8_t> vIV =
            std::vector<uint8_t>(vCipherText.begin(), vCipherText.begin() + AES_BLOCKLEN);

        /* Copy our data over to our plaintext now. */
        vPlainText =
            std::vector<uint8_t>(vCipherText.begin() + AES_BLOCKLEN, vCipherText.end());

        /* Decrypt our raw data buffer now. */
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, &vKey[0], &vIV[0]); //we store IV as first AES_BLOCKLEN bytes
        AES_CBC_decrypt_buffer(&ctx, &vPlainText[0], vPlainText.size());

        /* Strip off the PKCS7 padding.  The number of bytes to remove is stored in the last byte of the decrypted data */
        const uint8_t nPadBytes =
            vPlainText.back();

        /* Ensure that the number of padding bytes is 16 or less.  If it is then we have not decrypted successfully */
        if(nPadBytes > 16)
            return debug::error(FUNCTION, "malformed decryption, incorrect padding byte value ", uint32_t(nPadBytes));

        /* Remove the trailing bytes. */
        vPlainText.erase(vPlainText.end() - nPadBytes, vPlainText.end());

        /* return true */
        return true;
    }


    /* Encrypts data using ChaCha20-Poly1305 AEAD cipher. */
    bool EncryptChaCha20Poly1305(
        const std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce,
        std::vector<uint8_t>& vCiphertext,
        std::vector<uint8_t>& vTag,
        const std::vector<uint8_t>& vAAD)
    {
        /* Validate key size (256 bits = 32 bytes) */
        if(vKey.size() != 32)
            return debug::error(FUNCTION, "ChaCha20-Poly1305 key must be 32 bytes, got ", vKey.size());

        /* Validate nonce size (96 bits = 12 bytes) */
        if(vNonce.size() != 12)
            return debug::error(FUNCTION, "ChaCha20-Poly1305 nonce must be 12 bytes, got ", vNonce.size());

        /* Check plaintext is not empty */
        if(vPlaintext.empty())
            return debug::error(FUNCTION, "Plaintext is empty");

        /* Create and initialize cipher context */
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
            return debug::error(FUNCTION, "Failed to create cipher context");

        /* Initialize encryption */
        if(EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, vKey.data(), vNonce.data()) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "Failed to initialize ChaCha20-Poly1305 encryption");
        }

        /* Set AAD if provided */
        if(!vAAD.empty())
        {
            int nLen = 0;
            if(EVP_EncryptUpdate(ctx, nullptr, &nLen, vAAD.data(), static_cast<int>(vAAD.size())) != 1)
            {
                EVP_CIPHER_CTX_free(ctx);
                return debug::error(FUNCTION, "Failed to set AAD");
            }
        }

        /* Prepare output buffer */
        vCiphertext.resize(vPlaintext.size());
        int nCiphertextLen = 0;

        /* Perform encryption */
        if(EVP_EncryptUpdate(ctx, vCiphertext.data(), &nCiphertextLen, 
                            vPlaintext.data(), static_cast<int>(vPlaintext.size())) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "ChaCha20-Poly1305 encryption failed");
        }

        /* Finalize encryption */
        int nFinalLen = 0;
        if(EVP_EncryptFinal_ex(ctx, vCiphertext.data() + nCiphertextLen, &nFinalLen) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "ChaCha20-Poly1305 encryption finalization failed");
        }

        /* Resize to actual ciphertext length */
        vCiphertext.resize(nCiphertextLen + nFinalLen);

        /* Get authentication tag (16 bytes) */
        vTag.resize(16);
        if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, vTag.data()) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "Failed to get authentication tag");
        }

        EVP_CIPHER_CTX_free(ctx);
        return true;
    }


    /* Decrypts data using ChaCha20-Poly1305 AEAD cipher. */
    bool DecryptChaCha20Poly1305(
        const std::vector<uint8_t>& vCiphertext,
        const std::vector<uint8_t>& vTag,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce,
        std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vAAD)
    {
        /* Validate key size (256 bits = 32 bytes) */
        if(vKey.size() != 32)
            return debug::error(FUNCTION, "ChaCha20-Poly1305 key must be 32 bytes, got ", vKey.size());

        /* Validate nonce size (96 bits = 12 bytes) */
        if(vNonce.size() != 12)
            return debug::error(FUNCTION, "ChaCha20-Poly1305 nonce must be 12 bytes, got ", vNonce.size());

        /* Validate tag size (128 bits = 16 bytes) */
        if(vTag.size() != 16)
            return debug::error(FUNCTION, "ChaCha20-Poly1305 tag must be 16 bytes, got ", vTag.size());

        /* Check ciphertext is not empty */
        if(vCiphertext.empty())
            return debug::error(FUNCTION, "Ciphertext is empty");

        /* Create and initialize cipher context */
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
            return debug::error(FUNCTION, "Failed to create cipher context");

        /* Initialize decryption */
        if(EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, vKey.data(), vNonce.data()) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "Failed to initialize ChaCha20-Poly1305 decryption");
        }

        /* Set AAD if provided */
        if(!vAAD.empty())
        {
            int nLen = 0;
            if(EVP_DecryptUpdate(ctx, nullptr, &nLen, vAAD.data(), static_cast<int>(vAAD.size())) != 1)
            {
                EVP_CIPHER_CTX_free(ctx);
                return debug::error(FUNCTION, "Failed to set AAD");
            }
        }

        /* Prepare output buffer */
        vPlaintext.resize(vCiphertext.size());
        int nPlaintextLen = 0;

        /* Perform decryption */
        if(EVP_DecryptUpdate(ctx, vPlaintext.data(), &nPlaintextLen, 
                            vCiphertext.data(), static_cast<int>(vCiphertext.size())) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "ChaCha20-Poly1305 decryption failed");
        }

        /* Set expected authentication tag */
        if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, 
                              const_cast<uint8_t*>(vTag.data())) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "Failed to set authentication tag");
        }

        /* Finalize decryption - this verifies the tag */
        int nFinalLen = 0;
        if(EVP_DecryptFinal_ex(ctx, vPlaintext.data() + nPlaintextLen, &nFinalLen) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            return debug::error(FUNCTION, "ChaCha20-Poly1305 authentication failed - tag mismatch");
        }

        /* Resize to actual plaintext length */
        vPlaintext.resize(nPlaintextLen + nFinalLen);

        EVP_CIPHER_CTX_free(ctx);
        return true;
    }
}
