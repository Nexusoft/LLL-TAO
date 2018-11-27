/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef __CRYPTER_H__
#define __CRYPTER_H__

#include "../util/allocators.h" /* for SecureString */
#include "key.h"
#include "../util/serialize.h"

namespace Legacy
{
    const uint32_t WALLET_CRYPTO_KEY_SIZE = 72;
    const uint32_t WALLET_CRYPTO_SALT_SIZE = 18;

    /**
    Private key encryption is done based on a CMasterKey,
    which holds a salt and random encryption key.

    CMasterKeys are encrypted using AES-256-CBC using a key
    derived using derivation method nDerivationMethod
    (0 == EVP_sha512()) and derivation iterations nDeriveIterations.
    vchOtherDerivationParameters is provided for alternative algorithms
    which may require more parameters (such as scrypt).

    Wallet Private Keys are then encrypted using AES-256-CBC
    with the SK576 of the public key as the IV, and the
    master key's key as the encryption key (see keystore.[ch]).
    */

    /** Master key for wallet encryption */
    class CMasterKey
    {
    public:
        std::vector<uint8_t> vchCryptedKey;
        std::vector<uint8_t> vchSalt;
        // 0 = EVP_sha512()
        // 1 = scrypt()
        uint32_t nDerivationMethod;
        uint32_t nDeriveIterations;
        // Use this for more parameters to key derivation,
        // such as the various parameters to scrypt
        std::vector<uint8_t> vchOtherDerivationParameters;

        IMPLEMENT_SERIALIZE
        (
            READWRITE(vchCryptedKey);
            READWRITE(vchSalt);
            READWRITE(nDerivationMethod);
            READWRITE(nDeriveIterations);
            READWRITE(vchOtherDerivationParameters);
        )
        CMasterKey()
        {
            // 25000 rounds is just under 0.1 seconds on a 1.86 GHz Pentium M
            // ie slightly lower than the lowest hardware we need bother supporting
            nDeriveIterations = 25000;
            nDerivationMethod = 0;
            vchOtherDerivationParameters = std::vector<uint8_t>(0);
        }
    };

    typedef std::vector<uint8_t, secure_allocator<uint8_t> > CKeyingMaterial;

    /** Encryption/decryption context with key information */
    class CCrypter
    {
    private:
        uint8_t chKey[WALLET_CRYPTO_KEY_SIZE];
        uint8_t chIV[WALLET_CRYPTO_KEY_SIZE];
        bool fKeySet;

    public:
        bool SetKeyFromPassphrase(const SecureString &strKeyData, const std::vector<uint8_t>& chSalt, const uint32_t nRounds, const uint32_t nDerivationMethod);
        bool Encrypt(const CKeyingMaterial& vchPlaintext, std::vector<uint8_t> &vchCiphertext);
        bool Decrypt(const std::vector<uint8_t>& vchCiphertext, CKeyingMaterial& vchPlaintext);
        bool SetKey(const CKeyingMaterial& chNewKey, const std::vector<uint8_t>& chNewIV);

        void CleanKey()
        {
            memset(&chKey, 0, sizeof chKey);
            memset(&chIV, 0, sizeof chIV);
            munlock(&chKey, sizeof chKey);
            munlock(&chIV, sizeof chIV);
            fKeySet = false;
        }

        CCrypter()
        {
            fKeySet = false;
        }

        ~CCrypter()
        {
            CleanKey();
        }
    };

    bool EncryptSecret(CKeyingMaterial& vMasterKey, const CSecret &vchPlaintext, const uint576_t& nIV, std::vector<uint8_t> &vchCiphertext);
    bool DecryptSecret(const CKeyingMaterial& vMasterKey, const std::vector<uint8_t> &vchCiphertext, const uint576_t& nIV, CSecret &vchPlaintext);

}
#endif
