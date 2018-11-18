/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_MASTERKEY_H
#define NEXUS_LEGACY_WALLET_MASTERKEY_H

#include <vector>

#include <LLC/types/uint1024.h>

#include <Util/templates/serialize.h>

namespace Legacy
{
    
    namespace Wallet
    {
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

    }
}

#endif
