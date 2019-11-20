/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>
#include <LLC/hash/argon2.h>

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/genesis.h>

#include <TAO/Ledger/include/enum.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Copy Constructor */
        SignatureChain::SignatureChain(const SignatureChain& sigchain)
        : strUsername (sigchain.strUsername)
        , strPassword (sigchain.strPassword)
        , MUTEX       ( )
        , pairCache   (sigchain.pairCache)
        , hashGenesis (sigchain.hashGenesis)
        {
        }


        /** Move Constructor **/
        SignatureChain::SignatureChain(SignatureChain&& sigchain) noexcept
        : strUsername (std::move(sigchain.strUsername.c_str()))
        , strPassword (std::move(sigchain.strPassword.c_str()))
        , MUTEX       ( )
        , pairCache   (std::move(sigchain.pairCache))
        , hashGenesis (std::move(sigchain.hashGenesis))
        {
        }


        /** Destructor. **/
        SignatureChain::~SignatureChain()
        {
        }


        /* Constructor to generate Keychain */
        SignatureChain::SignatureChain(const SecureString& strUsernameIn, const SecureString& strPasswordIn)
        : strUsername (strUsernameIn.c_str())
        , strPassword (strPasswordIn.c_str())
        , MUTEX       ( )
        , pairCache   (std::make_pair(std::numeric_limits<uint32_t>::max(), ""))
        , hashGenesis (SignatureChain::Genesis(strUsernameIn))
        {
        }


        /* This function is responsible for returning the genesis ID.*/
        uint256_t SignatureChain::Genesis() const
        {
            return hashGenesis;
        }


        /* This function is responsible for generating the genesis ID.*/
        uint256_t SignatureChain::Genesis(const SecureString& strUsername)
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
                static_cast<uint32_t>(vUsername.size()),

                /* The salt for usernames */
                &vSalt[0],
                static_cast<uint32_t>(vSalt.size()),

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                12,

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
            int32_t nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

            /* Set the bytes for the key. */
            uint256_t hashKey;
            hashKey.SetBytes(vHash);
            hashKey.SetType(config::fTestNet.load() ? 0xa2 : 0xa1);

            return hashKey;
        }


        /*
         *  This function is responsible for genearting the private key in the keychain of a specific account.
         *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
         */
        uint512_t SignatureChain::Generate(const uint32_t nKeyID, const SecureString& strSecret, bool fCache) const
        {
            {
                LOCK(MUTEX);

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
            }

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

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
                static_cast<uint32_t>(vPassword.size()),

                /* Username and key ID as the salt. */
                &vUsername[0],
                static_cast<uint32_t>(vUsername.size()),

                /* The secret phrase as secret data. */
                &vSecret[0],
                static_cast<uint32_t>(vSecret.size()),

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                std::max(1u, uint32_t(config::GetArg("-argon2", 12))),

                /* Memory Cost (64 MB). */
                uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))),

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
            int nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

            /* Set the cache items. */
            {
                LOCK(MUTEX);

                if(fCache)
                {
                    pairCache.first  = nKeyID;
                    pairCache.second = SecureString(hash.begin(), hash.end());
                }
            }

            /* Set the bytes for the key. */
            uint512_t hashKey;
            hashKey.SetBytes(hash);

            return hashKey;
        }


        /* This function is responsible for generating the private key in the sigchain with a specific password and pin.
        *  This version should be used when changing the password and/or pin */
        uint512_t SignatureChain::Generate(const uint32_t nKeyID, const SecureString& strPassword, const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

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
                static_cast<uint32_t>(vPassword.size()),

                /* Username and key ID as the salt. */
                &vUsername[0],
                static_cast<uint32_t>(vUsername.size()),

                /* The secret phrase as secret data. */
                &vSecret[0],
                static_cast<uint32_t>(vSecret.size()),

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                std::max(1u, uint32_t(config::GetArg("-argon2", 12))),

                /* Memory Cost (64 MB). */
                uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))),

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
            int nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));


            /* Set the bytes for the key. */
            uint512_t hashKey;
            hashKey.SetBytes(hash);

            return hashKey;
        }


        /*
         *  This function is responsible for genearting the private key in the keychain of a specific account.
         *  The keychain is a series of keys seeded from a secret phrase and a PIN number.
         */
        uint512_t SignatureChain::Generate(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
            vUsername.insert(vUsername.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Set to minimum salt limits. */
            if(vUsername.size() < 8)
                vUsername.resize(8);

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vPassword(strPassword.begin(), strPassword.end());
            vPassword.insert(vPassword.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Generate the secret data. */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());
            vSecret.insert(vSecret.end(), (uint8_t*)&nKeyID, (uint8_t*)&nKeyID + sizeof(nKeyID));

            /* Seed secret data with the key type. */
            vSecret.insert(vSecret.end(), strType.begin(), strType.end());

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
                static_cast<uint32_t>(vPassword.size()),

                /* Username and key ID as the salt. */
                &vUsername[0],
                static_cast<uint32_t>(vUsername.size()),

                /* The secret phrase as secret data. */
                &vSecret[0],
                static_cast<uint32_t>(vSecret.size()),

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                std::max(1u, uint32_t(config::GetArg("-argon2", 12))),

                /* Memory Cost (64 MB). */
                uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))),

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
            int nRet = argon2id_ctx(&context);
            if(nRet != ARGON2_OK)
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "Argon2 failed with code ", nRet));

            /* Set the bytes for the key. */
            uint512_t hashKey;
            hashKey.SetBytes(hash);

            return hashKey;
        }


        /* This function generates a hash of a public key generated from random seed phrase. */
        uint256_t SignatureChain::KeyHash(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const
        {
            /* Get the private key. */
            uint512_t hashSecret = Generate(strType, nKeyID, strSecret);

            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        throw debug::exception(FUNCTION, "failed to set falcon secret key");

                    /* Calculate the next hash. */
                    uint256_t hashRet = LLC::SK256(key.GetPubKey());

                    /* Set the leading byte. */
                    hashRet.SetType(nType);

                    return hashRet;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        throw debug::exception(FUNCTION, "failed to set brainpool secret key");

                    /* Calculate the next hash. */
                    uint256_t hashRet = LLC::SK256(key.GetPubKey());

                    /* Set the leading byte. */
                    hashRet.SetType(nType);

                    return hashRet;
                }
            }

            return 0;
        }


        /* Returns the username for this sig chain */
        const SecureString& SignatureChain::UserName() const
        {
            return strUsername;
        }


        /* Special method for encrypting specific data types inside class. */
        void SignatureChain::Encrypt()
        {
            encrypt(strUsername);
            encrypt(strPassword);
            encrypt(pairCache.first);
            encrypt(pairCache.second);
            encrypt(hashGenesis);
        }


        /* Generates a signature for the data, using the specified crypto key type. */
        bool SignatureChain::Sign(const std::string& strType, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                  std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig ) const
        {

            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* The crypto register object */
            TAO::Register::Object crypto;

            /* Get the crypto register. This is needed so that we can determine the key type used to generate the public key */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw debug::exception(FUNCTION, "Could not sign - missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw debug::exception(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.CheckName(strType))
                throw debug::exception(FUNCTION, "Key type not found in crypto register: ", strType);

            /* Retrieve the pubic key hash from the crypto register */
            uint256_t hashPublic = crypto.get<uint256_t>(strType);

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = hashPublic.GetType();

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the FALCON signature scheme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret))
                        throw debug::exception(FUNCTION, "failed to set falcon secret key");

                    /* Generate the public key */
                    vchPubKey = key.GetPubKey();

                    /* Generate the signature */
                    if(!key.Sign(vchData, vchSig))
                        throw debug::exception(FUNCTION, "failed to sign data");

                    break;

                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!key.SetSecret(vchSecret, true))
                        throw debug::exception(FUNCTION, "failed to set brainpool secret key");

                    /* Generate the public key */
                    vchPubKey = key.GetPubKey();

                    /* Generate the signature */
                    if(!key.Sign(vchData, vchSig))
                        throw debug::exception(FUNCTION, "failed to sign data");

                    break;
                }
                default:
                {
                    throw debug::exception(FUNCTION, "unknown crypto signature scheme");
                }
            }

            /* Return success */
            return true;

        }
    }
}
