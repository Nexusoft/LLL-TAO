/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_CRYPTER_H
#define NEXUS_LEGACY_WALLET_CRYPTER_H

#include <vector>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

#include <Util/include/allocators.h> /* for SecureString typedef */

/* forward declaration */    
namespace LLC 
{
    class CSecret;
}

namespace Legacy 
{
    
    namespace Wallet
    {
        const uint32_t WALLET_CRYPTO_KEY_SIZE = 72;
        const uint32_t WALLET_CRYPTO_SALT_SIZE = 18;

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

        bool EncryptSecret(CKeyingMaterial& vMasterKey, const LLC::CSecret &vchPlaintext, const uint576_t& nIV, std::vector<uint8_t> &vchCiphertext);
        bool DecryptSecret(const CKeyingMaterial& vMasterKey, const std::vector<uint8_t> &vchCiphertext, const uint576_t& nIV, LLC::CSecret &vchPlaintext);

    }
}

#endif
