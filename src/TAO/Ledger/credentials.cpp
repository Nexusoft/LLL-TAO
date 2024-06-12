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
        : strUsername (sigchain.strUsername)
        , strPassword (sigchain.strPassword)
        , MUTEX       ( )
        , pairCache   (sigchain.pairCache)
        , hashGenesis (sigchain.hashGenesis)
        {
        }


        /** Move Constructor **/
        Credentials::Credentials(Credentials&& sigchain) noexcept
        : strUsername (std::move(sigchain.strUsername.c_str()))
        , strPassword (std::move(sigchain.strPassword.c_str()))
        , MUTEX       ( )
        , pairCache   (std::move(sigchain.pairCache))
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
        , MUTEX       ( )
        , pairCache   (std::make_pair(std::numeric_limits<uint32_t>::max(), ""))
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


        /* Set's the current cached key manually in case it was generated externally from this object. */
        void Credentials::SetCache(const uint512_t& hashSecret, const uint32_t nKeyID)
        {
            {
                LOCK(MUTEX);

                /* Handle cache to stop exhaustive hash key generation. */
                if(nKeyID == pairCache.first)
                {
                    /* Grab our key's binary data. */
                    const std::vector<uint8_t> vBytes = hashSecret.GetBytes();

                    /* Set our cache record now with it. */
                    pairCache.first  = nKeyID;
                    pairCache.second = SecureString(vBytes.begin(), vBytes.end());
                }
            }
        }


        /* This function is responsible for genearting the private key in the keychain of a specific account. */
        uint512_t Credentials::Generate(const uint32_t nKeyID, const SecureString& strSecret, const bool fCache) const
        {
            {
                LOCK(MUTEX);

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
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            /* Set the cache items. */
            {
                LOCK(MUTEX);

                if(fCache)
                {
                    /* Grab our key's binary data. */
                    const std::vector<uint8_t> vBytes = hashKey.GetBytes();

                    /* Set our cache record now with it. */
                    pairCache.first  = nKeyID;
                    pairCache.second = SecureString(vBytes.begin(), vBytes.end());
                }
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
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));


            return hashKey;
        }


        /* This function is responsible for genearting the private key in the keychain of a specific account. */
        uint512_t Credentials::Generate(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret) const
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

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vPassword, vUsername, vSecret,
                            std::max(1u, uint32_t(config::GetArg("-argon2", 12))),
                            uint32_t(1 << std::max(4u, uint32_t(config::GetArg("-argon2_memory", 16)))));

            return hashKey;
        }


        /* This function version using far stronger argon2 hashing since the only data input is the seed phrase itself. */
        uint512_t Credentials::Generate(const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());

            // low-level API
            std::vector<uint8_t> vHash(32);
            std::vector<uint8_t> vSalt(16);

            /* Create the hash context. */
            argon2_context context =
            {
                /* Hash Return Value. */
                &vHash[0],
                32,

                /* Password input data. */
                &vSecret[0],
                static_cast<uint32_t>(vSecret.size()),

                /* The salt for usernames */
                &vSalt[0],
                static_cast<uint32_t>(vSalt.size()),

                /* Optional secret data */
                NULL, 0,

                /* Optional associated data */
                NULL, 0,

                /* Computational Cost. */
                64,

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
            hashKey.SetType(TAO::Ledger::GENESIS::UserType());

            return hashKey;
        }


        /* This function is to remain backwards compatible if recovery was created after 5.0.6 and before 5.1.1. */
        uint512_t Credentials::GenerateDeprecated(const SecureString& strSecret) const
        {
            /* Generate the Secret Phrase */
            std::vector<uint8_t> vSecret(strSecret.begin(), strSecret.end());

            /* Argon2 hash the secret */
            uint512_t hashKey = LLC::Argon2_512(vSecret);

            /* Set the key type */
            hashKey.SetType(TAO::Ledger::GENESIS::UserType());

            return hashKey;
        }


        /* This function is to remain backwards compatible if recovery was created after 5.0.6 and before 5.1.1 */
        uint256_t Credentials::RecoveryDeprecated(const SecureString& strRecovery, const uint8_t nType) const
        {
            /* The hashed public key to return*/
            uint256_t hashRet = 0;

            /* Timer to track how long it takes to generate the recovery hash private key from the seed. */
            runtime::timer timer;
            timer.Reset();

            /* Get the private key. */
            uint512_t hashSecret = GenerateDeprecated(strRecovery);

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

            debug::log(0, FUNCTION, "Recovery Hash (DEPRECATED) ", hashRet.SubString(), " generated in ", timer.Elapsed(), " seconds");

            return hashRet;
        }


        /* This function generates a public key generated from random seed phrase. */
        std::vector<uint8_t> Credentials::Key(const std::string& strType, const uint32_t nKeyID,
                                                 const SecureString& strSecret, const uint8_t nType) const
        {
            /* The public key bytes */
            std::vector<uint8_t> vchPubKey;

            /* Get the private key. */
            const uint512_t hashSecret =
                Generate(strType, nKeyID, strSecret);

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

            /* return the public key */
            return vchPubKey;
        }

        /* This function generates a hash of a public key generated from random seed phrase. */
        uint256_t Credentials::KeyHash(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const
        {
            /* Generate the public key */
            const std::vector<uint8_t> vchPubKey =
                Key(strType, nKeyID, strSecret, nType);

            /* Calculate the key hash. */
            uint256_t hashRet =
                LLC::SK256(vchPubKey);

            /* Set the leading byte. */
            hashRet.SetType(nType);
            return hashRet;
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
            uint512_t hashSecret = Generate(strRecovery);

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
            strPassword = strPasswordNew.c_str();
        }


        /* Special method for encrypting specific data types inside class. */
        void Credentials::Encrypt()
        {
            encrypt(strUsername);
            encrypt(strPassword);
            encrypt(pairCache);
            encrypt(hashGenesis);
        }


        /* Generates a signature for the data, using the specified crypto key from the crypto object register. */
        bool Credentials::Sign(const std::string& strKey, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                  std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const
        {
            /* The crypto register object */
            TAO::Register::Object crypto;

            /* Get the crypto register. This is needed so that we can determine the key type used to generate the public key */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                return debug::error(FUNCTION, "Could not sign - missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                return debug::error(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.Check(strKey))
                return debug::error(FUNCTION, "Key type not found in crypto register: ", strKey);

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = crypto.get<uint256_t>(strKey).GetType();

            /* call the Sign method with the retrieved type */
            return Sign(nType, vchData, hashSecret, vchPubKey, vchSig);
        }


        /* Generates a signature for the data, using the specified crypto key type. */
        bool Credentials::Sign(const uint8_t& nKeyType, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                  std::vector<uint8_t> &vchPubKey, std::vector<uint8_t> &vchSig) const
        {
            /* Get the secret from new key. */
            std::vector<uint8_t> vBytes = hashSecret.GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nKeyType)
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


        /* Verifies a signature for the data, as well as verifying that the hashed public key matches the
        *  specified key from the crypto object register */
        bool Credentials::Verify(const uint256_t hashGenesis, const std::string& strKey, const std::vector<uint8_t>& vchData,
                    const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchSig)
        {
            /* Derive the object register address. */
            TAO::Register::Address hashCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Get the crypto register. */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::LOOKUP))
                return debug::error(FUNCTION, "Missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                return debug::drop(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.Check(strKey))
                return debug::error(FUNCTION, "Key type not found in crypto register: ", strKey);

            /* Check the authorization hash. */
            uint256_t hashCheck = crypto.get<uint256_t>(strKey);

            /* Check that the hashed public key exists in the register*/
            if(hashCheck == 0)
                return debug::error(FUNCTION, "Public key hash not found in crypto register: ", strKey);

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = hashCheck.GetType();

            /* Grab hash of incoming pubkey and set its type. */
            uint256_t hashPubKey = LLC::SK256(vchPubKey);
            hashPubKey.SetType(nType);

            /* Check the public key to expected authorization key. */
            if(hashPubKey != hashCheck)
                return debug::error(FUNCTION, "Invalid public key");

            /* Switch based on signature type. */
            switch(nType)
            {
                /* Support for the FALCON signature scheeme. */
                case TAO::Ledger::SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(vchData, vchSig))
                        return debug::error(FUNCTION, "Invalid transaction signature");

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the public key and verify. */
                    key.SetPubKey(vchPubKey);
                    if(!key.Verify(vchData, vchSig))
                        return debug::error(FUNCTION, "Invalid transaction signature");

                    break;
                }

                default:
                    return debug::error(FUNCTION, "Unknown signature scheme");

            }

            /* Verified! */
            return true;
        }

    }
}
