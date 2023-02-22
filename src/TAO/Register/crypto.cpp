/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "act the way you'd like to be and soon you'll be the way you act" - Leonard Cohen

____________________________________________________________________________________________*/

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <TAO/API/types/exception.h>

#include <TAO/Register/types/crypto.h>

#include <TAO/Ledger/include/enum.h>

/* Global TAO namespace. */
namespace TAO::Register
{
    /* Default constructor. */
    Crypto::Crypto()
    : Object    ()
    {
    }


    /* Copy Constructor. */
    Crypto::Crypto(const Crypto& rObject)
    : Object      (rObject)
    {
    }


    /* Move Constructor. */
    Crypto::Crypto(Crypto&& rObject) noexcept
    : Object      (std::move(rObject))
    {
    }


    /* Assignment operator overload */
    Crypto& Crypto::operator=(const Crypto& rObject)
    {
        vchState     = rObject.vchState;

        nVersion     = rObject.nVersion;
        nType        = rObject.nType;
        hashOwner    = rObject.hashOwner;
        nCreated     = rObject.nCreated;
        nModified    = rObject.nModified;
        hashChecksum = rObject.hashChecksum;

        nReadPos     = 0; //don't copy over read position
        mapData      = rObject.mapData;

        return *this;
    }


    /* Assignment operator overload */
    Crypto& Crypto::operator=(Crypto&& rObject) noexcept
    {
        vchState     = std::move(rObject.vchState);

        nVersion     = std::move(rObject.nVersion);
        nType        = std::move(rObject.nType);
        hashOwner    = std::move(rObject.hashOwner);
        nCreated     = std::move(rObject.nCreated);
        nModified    = std::move(rObject.nModified);
        hashChecksum = std::move(rObject.hashChecksum);

        nReadPos     = 0; //don't copy over read position
        mapData      = std::move(rObject.mapData);

        return *this;
    }


    /* Default Destructor */
    Crypto::~Crypto()
    {
    }


    /* Checks our credentials against a given generate key to make sure they match. */
    bool Crypto::CheckCredentials(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& strPIN)
    {
        /* Read the key type from crypto object register. */
        const uint256_t hashAuth =
            this->get<uint256_t>("auth");

        /* Check if the auth has is deactivated. */
        if(hashAuth == 0)
            throw TAO::API::Exception(-9, "auth key is disabled, please run crypto/create/auth to enable");

        /* Generate a key to check credentials against. */
        const uint256_t hashCheck =
            pCredentials->SignatureKey("auth", strPIN, hashAuth.GetType());

        /* Check for invalid authorization hash. */
        if(hashAuth != hashCheck)
            return false;

        return true;
    }


    /* Verify signature data to a pubkey and valid key hash inside crypto object register. */
    bool Crypto::VerifySignature(const std::string& strKey, const bytes_t& vData, const bytes_t& vPubKey, const bytes_t& vSig)
    {
        /* Check that the requested key is in the crypto register */
        if(!Check(strKey))
            return debug::error(FUNCTION, strKey, " type not found in crypto register");

        /* Check the authorization hash. */
        const uint256_t hashPubKey =
            get<uint256_t>(strKey);

        /* Check that the hashed public key exists in the register*/
        if(hashPubKey == 0)
            return debug::error(FUNCTION, strKey, " is disabled, run crypto/create/", strKey);

        /* Get the encryption key type from the hash of the public key */
        const uint8_t nType =
            hashPubKey.GetType();

        /* Grab hash of incoming pubkey and set its type. */
        const uint256_t hashCheck = LLC::SK256(vPubKey, nType);

        /* Check the public key to expected authorization key. */
        if(hashPubKey != hashCheck)
            return debug::error(FUNCTION, "public key hash mismatch");

        /* Switch based on signature type. */
        switch(nType)
        {
            /* Support for the FALCON signature scheeme. */
            case TAO::Ledger::SIGNATURE::FALCON:
            {
                /* Create the FL Key object. */
                LLC::FLKey key;

                /* Set the public key and verify. */
                key.SetPubKey(vPubKey);
                if(!key.Verify(vData, vSig))
                    return debug::error(FUNCTION, "Invalid transaction signature");

                break;
            }

            /* Support for the BRAINPOOL signature scheme. */
            case TAO::Ledger::SIGNATURE::BRAINPOOL:
            {
                /* Create EC Key object. */
                LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                /* Set the public key and verify. */
                key.SetPubKey(vPubKey);
                if(!key.Verify(vData, vSig))
                    return debug::error(FUNCTION, "Invalid transaction signature");

                break;
            }

            default:
                return debug::error(FUNCTION, "Unknown signature scheme");

        }

        return true;
    }


    /* Generate a signature from signature chain credentials with valid key hash. */
    bool Crypto::GenerateSignature(const std::string& strKey, const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                   const SecureString& strPIN, const bytes_t& vData, bytes_t &vPubKey, bytes_t &vSig)
    {
        /* Check that the requested key is in the crypto register */
        if(!Check(strKey))
            return debug::error(FUNCTION, strKey, " type not found in crypto register");

        /* Check the authorization hash. */
        const uint256_t hashPubKey =
            get<uint256_t>(strKey);

        /* Check that the hashed public key exists in the register*/
        if(hashPubKey == 0)
            return debug::error(FUNCTION, strKey, " is disabled, run crypto/create/", strKey);

        /* Get the encryption key type from the hash of the public key */
        const uint8_t nType =
            hashPubKey.GetType();

        /* Generate our secret key. */
        const uint512_t hashSecret =
            pCredentials->Generate(strKey, 0, strPIN);

        /* Get the secret from new key. */
        std::vector<uint8_t> vBytes = hashSecret.GetBytes();
        LLC::CSecret vSecret(vBytes.begin(), vBytes.end());

        /* Switch based on signature type. */
        switch(nType)
        {
            /* Support for the FALCON signature scheme. */
            case TAO::Ledger::SIGNATURE::FALCON:
            {
                /* Create the FL Key object. */
                LLC::FLKey key;

                /* Set the secret key. */
                if(!key.SetSecret(vSecret))
                    return debug::error(FUNCTION, "failed to set falcon secret key");

                /* Generate the public key */
                vPubKey = key.GetPubKey();

                /* Generate the signature */
                if(!key.Sign(vData, vSig))
                    return debug::error(FUNCTION, "failed to sign data");

                break;

            }

            /* Support for the BRAINPOOL signature scheme. */
            case TAO::Ledger::SIGNATURE::BRAINPOOL:
            {
                /* Create EC Key object. */
                LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                /* Set the secret key. */
                if(!key.SetSecret(vSecret, true))
                    return debug::error(FUNCTION, "failed to set brainpool secret key");

                /* Generate the public key */
                vPubKey = key.GetPubKey();

                /* Generate the signature */
                if(!key.Sign(vData, vSig))
                    return debug::error(FUNCTION, "failed to sign data");

                break;
            }

            default:
            {
                return debug::error(FUNCTION, "unknown crypto signature scheme");
            }
        }

        /* Calculate the key hash. */
        const uint256_t hashCheck =
            LLC::SK256(vPubKey, nType);

        /* Check the public key to expected authorization key. */
        if(hashPubKey != hashCheck)
            return debug::error(FUNCTION, "public key hash mismatch");

        return true;
    }
}
