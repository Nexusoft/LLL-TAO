/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/argon2.h>

#include <TAO/Ledger/types/sigchain.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* This function is responsible for generating the genesis ID.*/
        uint256_t SignatureChain::Genesis()
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
                5,

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


        /* This function is responsible for generating the genesis ID.*/
        uint256_t SignatureChain::Genesis(const SecureString strUsername)
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
                5,

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


        /*
         *  This function is responsible for genearting the private key in the keychain of a specific account.
         *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
         */
        uint512_t SignatureChain::Generate(uint32_t nKeyID, SecureString strSecret, bool fCache)
        {
            /* Handle cache to stop exhaustive hash key generation. */
            if(fCache && nKeyID == pairCache.first)
            {
                /* Create the hash key object. */
                uint512_t hashKey;

                /* Get the bytes from secure allocator. */
                std::vector<uint8_t> vBytes(pairCache.second.begin(), pairCache.second.end());

                /* Set the bytes of return value. */
                hashKey.SetBytes(vBytes);

                return hashKey;
            }

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
                5,

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

            /* Set the cache items. */
            if(fCache)
            {
                pairCache.first  = nKeyID;
                pairCache.second = SecureString(hash.begin(), hash.end());
            }

            /* Set the bytes for the key. */
            uint512_t hashKey;
            hashKey.SetBytes(hash);

            return hashKey;
        }
    }
}
