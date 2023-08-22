/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2023

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/hash/SK.h>
#include <LLC/hash/macro.h>

#include <LLC/include/argon2.h>
#include <LLC/include/flkey.h>
#include <LLC/include/kyber.h>
#include <LLC/include/eckey.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/credentials.h>
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
        Credentials::Credentials(const Credentials& sigchain)
        : strUsername    (sigchain.strUsername)
        , strPassword    (sigchain.strPassword)
        , pairCache      (sigchain.pairCache)
        , mapCrypto      (sigchain.mapCrypto)
        , hashGenesis    (sigchain.hashGenesis)
        {
        }


        /** Move Constructor **/
        Credentials::Credentials(Credentials&& sigchain) noexcept
        : strUsername (std::move(sigchain.strUsername.c_str()))
        , strPassword (std::move(sigchain.strPassword.c_str()))
        , pairCache   (std::move(sigchain.pairCache))
        , mapCrypto   (std::move(sigchain.mapCrypto))
        , hashGenesis (std::move(sigchain.hashGenesis))
        {
        }


        /** Destructor. **/
        Credentials::~Credentials()
        {
        }


        /* Constructor to generate Keychain */
        Credentials::Credentials(const SecureString& strUsernameIn, const SecureString& strPasswordIn)
        : strUsername (strUsernameIn.c_str())
        , strPassword (strPasswordIn.c_str())
        , pairCache   (std::make_pair(std::numeric_limits<uint32_t>::max(), ""))
        , mapCrypto   ( )
        , hashGenesis (Credentials::Genesis(strUsernameIn))
        {
        }


        /* Equivilence operator. */
        bool Credentials::operator==(const Credentials& pCheck) const
        {
            return (strUsername == pCheck.strUsername && strPassword == pCheck.strPassword && hashGenesis == pCheck.hashGenesis);
        }


        /* Equivilence operator. */
        bool Credentials::operator!=(const Credentials& pCheck) const
        {
            return !(*this == pCheck);
        }


        /* This function is responsible for returning the genesis ID.*/
        uint256_t Credentials::Genesis() const
        {
            return hashGenesis;
        }


        /* This function is responsible for generating the genesis ID.*/
        uint256_t Credentials::Genesis(const SecureString& strUsername)
        {
            /* The Genesis hash to return */
            uint256_t hashGenesis;

            /* Check the local DB first */
            if(LLD::Local && LLD::Local->ReadFirst(strUsername, hashGenesis))
                return hashGenesis;

            /* Generate the Secret Phrase */
            std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());

            /* Argon2 hash the secret */
            hashGenesis = LLC::Argon2_256(vUsername, std::vector<uint8_t>(16), std::vector<uint8_t>(), 12, (1 << 16));
            hashGenesis.SetType(TAO::Ledger::GENESIS::UserType());

            /* Cache this username-genesis pair in the local db*/
            if(LLD::Local)
                LLD::Local->WriteFirst(strUsername, hashGenesis);

            return hashGenesis;
        }


        /* This function is responsible for genearting the private key in the keychain of a specific account. */
        uint512_t Credentials::Generate(const uint32_t nKeyID, const SecureString& strSecret, const bool fCache) const
        {
            /* Handle cache to stop exhaustive hash key generation. */
            if(fCache && nKeyID == pairCache.first)
            {
                /* Get the bytes from secure allocator. */
                const std::vector<uint8_t> vBytes =
                    std::vector<uint8_t>(pairCache.second.begin(), pairCache.second.end());

                /* Set the bytes of return value. */
                uint512_t hashKey;
                hashKey.SetBytes(vBytes);

                return hashKey;
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

            /* Argon2 hash the secret */
            const uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            /* Set the cache items. */
            if(fCache)
            {
                /* Grab our key's binary data. */
                const std::vector<uint8_t> vBytes = hashKey.GetBytes();

                /* Set our cache record now with it. */
                pairCache.first  = nKeyID;
                pairCache.second = SecureString(vBytes.begin(), vBytes.end());
            }

            return hashKey;
        }


        /* This function is responsible for generating the private key in the sigchain with a specific password and pin. */
        uint512_t Credentials::Generate(const uint32_t nKeyID, const SecureString& strPassword, const SecureString& strSecret) const
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


            /* Argon2 hash the secret */
            const uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));


            return hashKey;
        }


        /* This function is responsible for genearting the private key in the keychain of a specific account. */
        uint512_t Credentials::Generate(const std::string& strName, const uint32_t nKeyID, const SecureString& strSecret) const
        {
            /* Check if we have this key already. */
            if(mapCrypto.count(strName) && mapCrypto[strName].first == strSecret)
                return mapCrypto[strName].second;

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
            vSecret.insert(vSecret.end(), strName.begin(), strName.end());

            /* Argon2 hash the secret */
            const uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            /* Set our internal cache for quick access. */
            mapCrypto[strName] = std::make_pair(strSecret, hashKey);

            return hashKey;
        }


        /* This function version using far stronger argon2 hashing since the only data input is the seed phrase itself. */
        uint512_t Credentials::GenerateRecovery(const SecureString& strRecovery) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vSecret(strRecovery.begin(), strRecovery.end());

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vSecret);

            /* Set the key type */
            hashKey.SetType(TAO::Ledger::GENESIS::UserType());

            return hashKey;
        }


        /* This function generates a hash of a public key generated from random seed phrase. */
        uint256_t Credentials::SignatureKey(const std::string& strName, const SecureString& strSecret,
                                            const uint8_t nType, const uint32_t nKeyID) const
        {
            /* The public key bytes */
            std::vector<uint8_t> vchPubKey;

            /* Check if we have this key in our internal map. */
            uint512_t hashSecret = 0;

            /* Check if we have this key already. */
            if(mapCrypto.count(strName) && mapCrypto[strName].first == strSecret)
                hashSecret = mapCrypto[strName].second;
            else
                hashSecret = Generate(strName, nKeyID, strSecret);

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

                    /* Set the key bytes to return */
                    vchPubKey = key.GetPubKey();

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

                    /* Set the key bytes to return */
                    vchPubKey = key.GetPubKey();

                    break;
                }

                default:
                    throw debug::exception(FUNCTION, "unsupported key type ", uint32_t(nType));
            }

            /* Calculate the key hash. */
            return LLC::SK256(vchPubKey, nType);
        }


        /* This function generates a hash of a public key generated from random seed phrase using key-encapsulation mechanism. */
        uint256_t Credentials::CertificateKey(const std::string& strName, const SecureString& strSecret, const uint8_t nType, const uint32_t nKeyID) const
        {
            /* The public key bytes */
            std::vector<uint8_t> vPubKey (CRYPTO_PUBLICKEYBYTES, 0);
            std::vector<uint8_t> vPrivKey(CRYPTO_SECRETKEYBYTES, 0);

            /* Check if we have this key in our internal map. */
            uint512_t hashSecret = 0;

            /* Check if we have this key already. */
            if(mapCrypto.count(strName) && mapCrypto[strName].first == strSecret)
                hashSecret = mapCrypto[strName].second;
            else
                hashSecret = Generate(strName, nKeyID, strSecret);

            /* Get the secret from new key. */
            const std::vector<uint8_t> vSecret =
                hashSecret.GetBytes();

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the KYBER key encapsulation mechanism. */
                case KEM::KYBER:
                {
                    /* Generate our shared key using entropy from our secret hash. */
                    crypto_kem_keypair_from_secret(&vPubKey[0], &vPrivKey[0], &vSecret[0]);

                    break;
                }

                default:
                    throw debug::exception(FUNCTION, "unsupported key type ", uint32_t(nType));
            }

            /* Calculate the key hash. */
            return LLC::SK256(vPubKey, nType);
        }


        /* This function generates a hash of a public key generated from a recovery seed phrase. */
        uint256_t Credentials::RecoveryHash(const SecureString& strRecovery, const uint8_t nType) const
        {
            /* The hashed public key to return*/
            uint256_t hashRet = 0;

            /* Timer to track how long it takes to generate the recovery hash private key from the seed. */
            runtime::timer timer;
            timer.Reset();

            /* Get the private key. */
            uint512_t hashSecret = GenerateRecovery(strRecovery);

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

                    /* Calculate the hash of the public key. */
                    hashRet = LLC::SK256(key.GetPubKey());

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

                    /* Calculate the hash of the public key. */
                    hashRet = LLC::SK256(key.GetPubKey());

                    break;

                }
                default:
                    throw debug::exception(FUNCTION, "Unknown signature key type");

            }

            debug::log(0, FUNCTION, "Recovery Hash ", hashRet.SubString(), " generated in ", timer.Elapsed(), " seconds");

            return hashRet;
        }


        /* Returns the username for this sig chain */
        const SecureString& Credentials::UserName() const
        {
            return strUsername;
        }


        /* Returns the password for this sig chain */
        const SecureString& Credentials::Password() const
        {
            return strPassword;
        }


        /* Updates the password for this sigchain. */
        void Credentials::Update(const SecureString& strPasswordNew)
        {
            /* Clear our cached crypto keys. */
            ClearCache();

            /* Update our password now. */
            strPassword = strPasswordNew.c_str();
        }


        /** ClearCache
         *
         *  Clears all of the active crypto keys.
         *
         **/
        void Credentials::ClearCache()
        {
            /* Clear our cached crypto keys. */
            mapCrypto.clear();

            /* Reset our internal key cache too. */
            pairCache = std::make_pair(std::numeric_limits<uint32_t>::max(), "");
        }


        /* Special method for encrypting specific data types inside class. */
        void Credentials::Encrypt()
        {
            encrypt(strUsername);
            encrypt(strPassword);
            encrypt(pairCache);
            encrypt(hashGenesis);
        }
    }
}
