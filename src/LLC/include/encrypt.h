/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2025

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <vector>
#include <LLC/types/base_uint.h>

namespace LLC
{

     /** Encrypt
     *
     *  Encrypts the data using the AES 256 function and the specified symmetric key. 
     *  .       
     *
     *  @param[in] vchKey The symmetric key to use for the encryption.
     *  @param[in] vchPlainText The plain text data bytes to be encrypted.
     *  @param[out] vchEncrypted  The encrypted data.
     *
     *  @return True if the data was encrypted successfully.
     *
     **/
    bool EncryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vchPlainText, std::vector<uint8_t> &vchEncrypted);


    /** Decrypt
     *
     *  Decrypts the data using the AES 256 function and the specified symmetric key.       
     *
     *  @param[in] vchKey The symmetric key to use for the encryption.
     *  @param[in] vchEncrypted The encrypted data to be decrypted.
     *  @param[out] vchPlainText The decrypted plain text data.
     *
     *  @return True if the data was decrypted successfully.
     *
     **/
    bool DecryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vchEncrypted, std::vector<uint8_t> &vchPlainText);


    /** EncryptChaCha20Poly1305
     *
     *  Encrypts data using ChaCha20-Poly1305 AEAD cipher.
     *  Provides both confidentiality and authentication.
     *
     *  @param[in] vPlaintext The plain text data to encrypt
     *  @param[in] vKey The 256-bit (32 byte) symmetric key
     *  @param[in] vNonce The 96-bit (12 byte) nonce (must be unique per key)
     *  @param[out] vCiphertext The encrypted data
     *  @param[out] vTag The 128-bit (16 byte) authentication tag
     *  @param[in] vAAD Additional Authenticated Data (optional)
     *
     *  @return True if encryption succeeded
     *
     **/
    bool EncryptChaCha20Poly1305(
        const std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce,
        std::vector<uint8_t>& vCiphertext,
        std::vector<uint8_t>& vTag,
        const std::vector<uint8_t>& vAAD = std::vector<uint8_t>()
    );


    /** DecryptChaCha20Poly1305
     *
     *  Decrypts data using ChaCha20-Poly1305 AEAD cipher.
     *  Verifies authentication tag before decryption.
     *
     *  @param[in] vCiphertext The encrypted data
     *  @param[in] vTag The 128-bit (16 byte) authentication tag
     *  @param[in] vKey The 256-bit (32 byte) symmetric key
     *  @param[in] vNonce The 96-bit (12 byte) nonce
     *  @param[out] vPlaintext The decrypted data
     *  @param[in] vAAD Additional Authenticated Data (must match encryption)
     *
     *  @return True if decryption and authentication succeeded
     *
     **/
    bool DecryptChaCha20Poly1305(
        const std::vector<uint8_t>& vCiphertext,
        const std::vector<uint8_t>& vTag,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce,
        std::vector<uint8_t>& vPlaintext,
        const std::vector<uint8_t>& vAAD = std::vector<uint8_t>()
    );


}
