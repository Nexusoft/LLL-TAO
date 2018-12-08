/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_WALLET_CRYPTER_H
#define NEXUS_TAO_LEGACY_WALLET_CRYPTER_H

#include <vector>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

#include <Util/include/allocators.h>

namespace Legacy 
{
    
    const uint32_t WALLET_CRYPTO_KEY_SIZE = 72;
    const uint32_t WALLET_CRYPTO_SALT_SIZE = 18;

    /** CKeyingMaterial is type alias defining a byte vector with a custom, secure allocator. **/
    using CKeyingMaterial = std::vector<uint8_t, secure_allocator<uint8_t> >;

    /** @class CCrypter
     *
     * Encryption/decryption context with key information 
     *
     **/
    class CCrypter
    {
    private:
        /** Key used to perform encryption **/
        uint8_t chKey[WALLET_CRYPTO_KEY_SIZE];

        /** Initialization Vector (IV) used in combination with key to perform encryption **/
        uint8_t chIV[WALLET_CRYPTO_KEY_SIZE];

        /** Set to true when chKey and chIV have values **/
        bool fKeySet;


    public:
        /** Default constructor
         *
         *  Initializes fKeySet = false
         *
         **/
        CCrypter() : fKeySet(false)
        {
        }


        /* Because this class defines a destructor and manages memory with lock/unlock, it also needs copy implementations. */
        /* Move operations will also use copy, so they are not defined. */

        /** Copy constructor
         **/
        CCrypter(const CCrypter &c);


         /** Copy assignment operator
         **/
        CCrypter& operator= (const CCrypter &rhs);


        /** Destructor
         *
         * Clears the encryption key from memory
         *
         **/
       ~CCrypter();


        /** SetKey
         *
         *  Assign new encryption key (chKey and chIV) for encrypting/decrypting data.
         *
         *  @param[in] chNewKey New value for chKey. Must be of length WALLET_CRYPTO_KEY_SIZE
         *
         *  @param[in] chNewIV New value for chIV. Must be of length WALLET_CRYPTO_KEY_SIZE
         *
         *  @return true if new key context successfully assigned
         *
         **/
        bool SetKey(const CKeyingMaterial& chNewKey, const std::vector<uint8_t>& chNewIV);


        /** SetKeyFromPassphrase
         *
         *  Generate new encryption key (chKey and chIV) from a passphrase.
         *
         *  @param[in] strKeyData The passphrase to use
         *
         *  @param[in] chSalt A salt value to use for key generation. Must be of length WALLET_CRYPTO_SALT_SIZE
         *
         *  @param[in] nRounds The iteration count to use for key generation. Must be >= 1
         *
         *  @param[in] nDerivationMethod The method (message digest) used to derive the key value. Currently supports 0 (SHA-512)
         *
         *  @return true if new key context successfully assigned
         *
         **/
        bool SetKeyFromPassphrase(const SecureString &strKeyData, const std::vector<uint8_t>& chSalt, const uint32_t nRounds, const uint32_t nDerivationMethod);


        /** IsKeySet
         *
         *  Check if encryption key is assigned.
         *
         *  @return true if key assigned
         *
         **/
        inline bool IsKeySet() const
        {
            return fKeySet;
        }


        /** CleanKey
         *
         *  Clear the current encryption key from memory. 
         *
         **/
        void CleanKey();


        /** Encrypt
         *
         *  Encrypt a plain text value using the current encryption key settings.
         *  Must first set the encryption key or this method returns false.
         *
         *  @param[in] vchPlaintext The text to encrypt
         *
         *  @param[out] vchCiphertext The encrypted text resulting from encrypting vchPlaintext using chKey and chIV
         *
         *  @return true if the text was successfully encrypted
         *
         **/
        bool Encrypt(const CKeyingMaterial& vchPlaintext, std::vector<uint8_t> &vchCiphertext);


        /** Decrypt
         *
         *  Decrypt an encrypted text value using the current encryption key settings.
         *  Must first set the encryption key or this method returns false.
         *
         *  @param[in] vchCiphertext The encrypted text to decrypt
         *
         *  @param[out] vchPlaintext The text resulting from decrypting vchCiphertext
         *
         *  @return true if the text was successfully decrypted
         *
         **/
        bool Decrypt(const std::vector<uint8_t>& vchCiphertext, CKeyingMaterial& vchPlaintext);

    };


    /** @fn EncryptSecret
     *
     *  Function to encrypt a private key using a master key and IV pair. 
     *  Creates a CCrypter instance and assigns the key context to perform the actual encryption.
     *
     *  @param[in] vMasterKey The encryption key to use for encryption
     *
     *  @param[in] vchPlaintext The private key to encrypt
     *
     *  @param[in] nIV The IV value to use for encryption
     *
     *  @param[out] vchCiphertext The resulting encrypted private key
     *
     *  @return true if the private key was successfully encrypted
     *
     **/
    bool EncryptSecret(const CKeyingMaterial& vMasterKey, const LLC::CSecret &vchPlaintext, const uint576_t& nIV, std::vector<uint8_t> &vchCiphertext);


    /** @fn DecryptSecret
     *
     *  Function to encrypt a private key using a master key and IV pair. 
     *  Creates a CCrypter instance and assigns the key context to perform the actual decryption.
     *
     *  @param[in] vMasterKey The encryption key to use for encryption
     *
     *  @param[in] vchCiphertext The encrypted private key to decrypt
     *
     *  @param[in] nIV The IV value to use for decryption
     *
     *  @param[out] vchPlaintext The resulting decrypted private key
     *
     *  @return true if the private key was successfully decrypted
     *
     **/
    bool DecryptSecret(const CKeyingMaterial& vMasterKey, const std::vector<uint8_t> &vchCiphertext, const uint576_t& nIV, LLC::CSecret &vchPlaintext);

}

#endif
