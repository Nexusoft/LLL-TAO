/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H
#define NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H

#include <string>

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/argon2.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** SignatureChain
         *
         *  Base Class for a Signature SignatureChain
         *
         *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
         *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
         *  to attack a signature chain by dictionary attack.
         *
         */
        class SignatureChain
        {

            /** Secure allocator to represent the username of this signature chain. **/
            SecureString strUsername;


            /** Secure allocater to represent the password of this signature chain. **/
            SecureString strPassword;

        public:


            /** Constructor to generate Keychain
             *
             * @param[in] strUsernameIn The username to seed the signature chain
             * @param[in] strPasswordIn The password to seed the signature chain
             **/
            SignatureChain(SecureString strUsernameIn, SecureString strPasswordIn)
            : strUsername(strUsernameIn.c_str())
            , strPassword(strPasswordIn.c_str())
            {

            }


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint256_t Genesis()
            {
                /* Generate the Secret Phrase */
                std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());

                // low-level API
                std::vector<uint8_t> vHash(32);
                std::vector<uint8_t> vSalt(16); //TODO: possibly make this your birthday (required in API)

                /* Create the hash context. */
                argon2_context context =
                {
                    /* Hash Return Value. */
                    &vHash[0],
                    32,

                    /* Password input data. */
                    &vUsername[0],
                    vUsername.size(),

                    /* The salt for usernames */
                    &vSalt[0],
                    vSalt.size(),

                    /* Optional secret data */
                    NULL, 0,

                    /* Optional associated data */
                    NULL, 0,

                    /* Computational Cost. */
                    9,

                    /* Memory Cost (64 MB). */
                    (1 << 16),

                    /* The number of threads and lanes */
                    1, 1,

                    /* Algorithm Version */
                    ARGON2_VERSION_13,

                    /* Custom memory allocation / deallocation functions. */
                    NULL, NULL,

                    /* By default only internal memory is cleared (pwd is not wiped) */
                    ARGON2_DEFAULT_FLAGS
                };

                /* Run the argon2 computation. */
                if(argon2i_ctx(&context) != ARGON2_OK)
                    return 0;

                /* Set the bytes for the key. */
                uint256_t hashKey;
                hashKey.SetBytes(vHash);

                return hashKey;
            }


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            static uint256_t Genesis(const SecureString strUsername)
            {
                /* Generate the Secret Phrase */
                std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());

                // low-level API
                std::vector<uint8_t> vHash(32);
                std::vector<uint8_t> vSalt(16); //TODO: possibly make this your birthday (required in API)

                /* Create the hash context. */
                argon2_context context =
                {
                    /* Hash Return Value. */
                    &vHash[0],
                    32,

                    /* Password input data. */
                    &vUsername[0],
                    vUsername.size(),

                    /* The salt for usernames */
                    &vSalt[0],
                    vSalt.size(),

                    /* Optional secret data */
                    NULL, 0,

                    /* Optional associated data */
                    NULL, 0,

                    /* Computational Cost. */
                    9,

                    /* Memory Cost (64 MB). */
                    (1 << 16),

                    /* The number of threads and lanes */
                    1, 1,

                    /* Algorithm Version */
                    ARGON2_VERSION_13,

                    /* Custom memory allocation / deallocation functions. */
                    NULL, NULL,

                    /* By default only internal memory is cleared (pwd is not wiped) */
                    ARGON2_DEFAULT_FLAGS
                };

                /* Run the argon2 computation. */
                if(argon2i_ctx(&context) != ARGON2_OK)
                    return 0;

                /* Set the bytes for the key. */
                uint256_t hashKey;
                hashKey.SetBytes(vHash);

                return hashKey;
            }


            /** Generate
             *
             *  This function is responsible for genearting the private key in the keychain of a specific account.
             *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use (Never Cached)
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(uint32_t nKeyID, SecureString strSecret)
            {
                /* Generate the Secret Phrase */
                std::vector<uint8_t> vPassword(strPassword.begin(), strPassword.end());
                vPassword.insert(vPassword.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

                /* Generate the secret data. */
                std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());
                vSecret.insert(vSecret.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

                // low-level API
                std::vector<uint8_t> hash(64);

                /* Create the hash context. */
                argon2_context context =
                {
                    /* Hash Return Value. */
                    &hash[0],
                    64,

                    /* Password input data. */
                    &vPassword[0],
                    vPassword.size(),

                    /* The secret phrase (PIN) as the salt. */
                    &vSecret[0],
                    vSecret.size(),

                    /* Optional secret data */
                    NULL, 0,

                    /* Optional associated data */
                    NULL, 0,

                    /* Computational Cost. */
                    9,

                    /* Memory Cost (64 MB). */
                    (1 << 16),

                    /* The number of threads and lanes */
                    1, 1,

                    /* Algorithm Version */
                    ARGON2_VERSION_13,

                    /* Custom memory allocation / deallocation functions. */
                    NULL, NULL,

                    /* By default only internal memory is cleared (pwd is not wiped) */
                    ARGON2_DEFAULT_FLAGS
                };

                /* Run the argon2 computation. */
                if(argon2i_ctx(&context) != ARGON2_OK)
                    return 0;

                /* Set the bytes for the key. */
                uint512_t hashKey;
                hashKey.SetBytes(hash);

                return hashKey;
            }
        };
    }
}

#endif
