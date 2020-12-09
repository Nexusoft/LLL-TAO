/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/encrypt.h>
#include <Common/include/random.h>
#include <LLC/aes/aes.h>

#include <Util/include/debug.h>

#include <cstring>


namespace LLC
{

    /* Encrypts the data using the AES 256 function and the specified symmetric key. */
    bool EncryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vchPlainText, std::vector<uint8_t> &vchEncrypted)
    {
        /* Check the key length.  NOTE that this should be double AES_KEYLEN as the incoming key is in hex */
        if(vchKey.size() != AES_KEYLEN)
            return debug::error(FUNCTION, "Invalid key length");

        /* Check the incoming data is not empty */
        if(vchPlainText.empty())
            return debug::error(FUNCTION, "Plain text is empty");

        /* Generate a random 128 bit Initialisation Vector */
        std::vector<uint8_t> vchIV = Common::GetRand128().GetBytes();
        
        /* Number of bytes to pad so that the ciphertext is a multiple of AES_BLOCKLEN */
        uint8_t nPadBytes = AES_BLOCKLEN - (vchPlainText.size() % AES_BLOCKLEN);

        /* The ciphertext length.  This needs to be a multiple of the AES block size (16) based on the plain text length*/
        uint32_t nCiphertextLen = vchPlainText.size() + nPadBytes;

        /* The ciphertext bytes */
        uint8_t* pCiphertext = (uint8_t*)malloc(nCiphertextLen);

        /* Copy the plain text data into the ciphertext buffer to pass to the AES function */
        std::copy(vchPlainText.begin(), vchPlainText.end(), pCiphertext);
        
        /* PKCS7 padding - set the last X bytes to the value X so that the total number of bytes is a multiple of the block size */
        memset(pCiphertext + vchPlainText.size(), nPadBytes, nPadBytes);

   
        /* Do the encryption */
        try
        {
            struct AES_ctx ctx;
            AES_init_ctx_iv(&ctx, &vchKey[0], &vchIV[0]);
            AES_CBC_encrypt_buffer(&ctx, pCiphertext, nCiphertextLen);
        }
        catch(...)
        {
            /* Cleanup */
            free(pCiphertext);

            return debug::error(FUNCTION, "Error encrypting data");
        }

        /* Reserve space in the vector for the IV, and ciphertext */
        vchEncrypted.reserve(vchIV.size() + nCiphertextLen);        

        /* Copy IV into the cipher text vector */
        std::copy(vchIV.begin(), vchIV.end(), std::back_inserter(vchEncrypted));
        
        /* Copy ciphertext into the cipher text vector */
        std::copy(pCiphertext, pCiphertext + nCiphertextLen, std::back_inserter(vchEncrypted));

        /* Cleanup */
        free(pCiphertext);

        /* return true */
        return true;

    }

    
    /* Decrypts the data using the AES 256 function and the specified symmetric key. */
    bool DecryptAES256(const std::vector<uint8_t>& vchKey, const std::vector<uint8_t>& vEncrypted, std::vector<uint8_t> &vPlainText)
    {
        /* Check the key length.  NOTE that this should be double AES_KEYLEN as the incoming key is in hex */
        if(vchKey.size() != AES_KEYLEN)
            return debug::error(FUNCTION, "Invalid key length");

        /* Check size of the encrypted data vector.  This must be at least 32 bytes, 16 for the IV and 16 for the minimum 
           AES block size */
        if(vEncrypted.size() < 32)
            return debug::error(FUNCTION, "Encrypted data size too small");

        /* 128 bit Initialisation Vector is the first 16 bytes of the cipher text bytes*/
        std::vector<uint8_t> vchIV(&vEncrypted[0], &vEncrypted[16]);

        /* Actual ciphertext length, which is vEncrypted length minus 16 bytes for the IV */
        uint32_t nCiphertextLen = vEncrypted.size() - 16;

        /* Ciphertext length must be a multiple of the AES block size (16) */
        if(nCiphertextLen % AES_BLOCKLEN > 0)
            return debug::error(FUNCTION, "Invalid ciphertext");

        /* Allocate buffer to receive the decrypted data */
        uint8_t *pDecrypted = (uint8_t*)malloc(nCiphertextLen);

        /* Copy the encrypted bytes into the buffer to be decrypted */
        std::copy(vEncrypted.begin() + 16, vEncrypted.end(), pDecrypted);
        
        /* Do the decryption */
        try
        {
            struct AES_ctx ctx;
            AES_init_ctx_iv(&ctx, &vchKey[0], &vchIV[0]);
            AES_CBC_decrypt_buffer(&ctx, pDecrypted, nCiphertextLen);
        }
        catch(...)
        {
            /* Cleanup */
            free(pDecrypted);

            return debug::error(FUNCTION, "Error decrypting data");
        }
        
        /* Strip off the PKCS7 padding.  The number of bytes to remove is stored in the last byte of the decrypted data */
        uint8_t nPadBytes = pDecrypted[nCiphertextLen-1];

        /* Ensure that the number of padding bytes is 16 or less.  If it is then we have not decrypted successfully */
        if(nPadBytes > 16)
            return debug::error(FUNCTION, "Error decrypting data");

        /* The length of the plain text data, which is the decrypted data minus the padding bytes */
        uint32_t nPlaintextLen = nCiphertextLen - nPadBytes;

        /* Reserve space in the vector for the plain text bytes  */
        vPlainText.reserve(nPlaintextLen);
        
        /* Copy decrypted bytes into the return vector */
        std::copy(pDecrypted, pDecrypted + nPlaintextLen, std::back_inserter(vPlainText));

        /* Cleanup */
        free(pDecrypted);
        
        /* return true */
        return true;

    }

}
