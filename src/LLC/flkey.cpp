/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <stdexcept>
#include <vector>

#include <LLC/types/uint1024.h>
#include <LLC/types/typedef.h>

#include <LLC/include/flkey.h>

#include <LLC/falcon/falcon.h>

namespace LLC
{
    /* The default constructor. */
    FLKey::FLKey()
    : vchPubKey   ( )
    , vchPrivKey  ( )
    , fSet        (false)
    , fCompressed (false)
    {

    }

    /** Copy Constructor. **/
    FLKey::FLKey(const FLKey& b)
    : vchPubKey   (b.vchPubKey)
    , vchPrivKey  (b.vchPrivKey)
    , fSet        (b.fSet)
    , fCompressed (b.fCompressed)
    {
    }


    /** Move Constructor. **/
    FLKey::FLKey(FLKey&& b) noexcept
    : vchPubKey   (std::move(b.vchPubKey))
    , vchPrivKey  (std::move(b.vchPrivKey))
    , fSet        (std::move(b.fSet))
    , fCompressed (std::move(b.fCompressed))
    {
    }


    /** Copy Assignment Operator **/
    FLKey& FLKey::operator=(const FLKey& b)
    {
        vchPubKey   = b.vchPubKey;
        vchPrivKey  = b.vchPrivKey;
        fSet        = b.fSet;
        fCompressed = b.fCompressed;

        return *this;
    }


    /** Move Assignment Operator **/
    FLKey& FLKey::operator=(FLKey&& b) noexcept
    {
        vchPubKey   = std::move(b.vchPubKey);
        vchPrivKey  = std::move(b.vchPrivKey);
        fSet        = std::move(b.fSet);
        fCompressed = std::move(b.fCompressed);

        return *this;
    }

    /** Default Destructor. **/
    FLKey::~FLKey()
    {
    }


    /* Comparison Operator */
    bool FLKey::operator==(const FLKey& b) const
    {
        return (vchPubKey == b.vchPubKey && vchPrivKey == b.vchPrivKey);
    }


    /* Reset internal key data. */
    void FLKey::Reset()
    {
        fCompressed = false;

        vchPubKey.clear();
        vchPrivKey.clear();

        fSet = false;
    }


    /* Determine if the key is in nullptr state, false otherwise. */
    bool FLKey::IsNull() const
    {
        return !fSet;
    }


    /* Flag to determine if the key is in compressed form. */
    bool FLKey::IsCompressed() const
    {
        return fCompressed;
    }


    /* Create a new key from the Falcon random PRNG seeds */
    void FLKey::MakeNewKey(bool fCompressedIn)
    {
        /* Flag if the key is compressed. */
        fCompressed = fCompressedIn;

        /* Create a new keygen pointer. */
        falcon_keygen* keygen = falcon_keygen_new(9, 0); //NOTE: using log(9) for 512-bit Log(10) is for 1024-bit

        /* Get the maximum key sizes (to return actual size on keygen). */
        size_t nPrivKeySize = falcon_keygen_max_privkey_size(keygen);
        size_t nPubKeySize  = falcon_keygen_max_pubkey_size(keygen);

        /* Resize the allocators to expected sizes. */
        vchPubKey.resize(nPubKeySize);
        vchPrivKey.resize(nPrivKeySize);

        /* Attempt to create the key. */
        if(falcon_keygen_make(keygen, fCompressed ? FALCON_COMP_STATIC : FALCON_COMP_NONE,
            &vchPrivKey[0], &nPrivKeySize, &vchPubKey[0], &nPubKeySize) != 1)
        {
            Reset();

            /* Free the memory. */
            falcon_keygen_free(keygen);

            return;
        }

        /* Resize to the actual returned sizes. */
        vchPrivKey.resize(nPrivKeySize);
        vchPubKey.resize(nPubKeySize);

        /* Show key as successfully set. */
        fSet = true;

        /* Free the memory. */
        falcon_keygen_free(keygen);
    }


    /* Set the secret phrase / key used in the private key. */
    bool FLKey::SetSecret(const CSecret& vchSecret, bool fCompressedIn)
    {
        /* Flag if the key is compressed. */
        fCompressed = fCompressedIn;

        /* Create a new keygen pointer. */
        falcon_keygen* keygen = falcon_keygen_new(9, 0); //NOTE: using log(9) for 512-bit Log(10) is for 1024-bit

        /* Set the key's seed from secret phrase. */
        falcon_keygen_set_seed(keygen, &vchSecret[0], vchSecret.size(), 1);

        /* Get the maximum key sizes (to return actual size on keygen). */
        size_t nPrivKeySize = falcon_keygen_max_privkey_size(keygen);
        size_t nPubKeySize  = falcon_keygen_max_pubkey_size(keygen);

        /* Resize the allocators to expected sizes. */
        vchPubKey.resize(nPubKeySize);
        vchPrivKey.resize(nPrivKeySize);

        /* Attempt to create the key. */
        if(falcon_keygen_make(keygen, fCompressed ? FALCON_COMP_STATIC : FALCON_COMP_NONE,
            &vchPrivKey[0], &nPrivKeySize, &vchPubKey[0], &nPubKeySize) != 1)
        {
            Reset();

            /* Free the memory. */
            falcon_keygen_free(keygen);

            return false;
        }

        /* Resize to the actual returned sizes. */
        vchPrivKey.resize(nPrivKeySize);
        vchPubKey.resize(nPubKeySize);

        /* Show key as successfully set. */
        fSet = true;

        /* Free the memory. */
        falcon_keygen_free(keygen);

        return true;
    }


    /* Obtain the private key and all associated data. */
    CPrivKey FLKey::GetPrivKey() const
    {
        return vchPrivKey;
    }


    /* Returns true on the setting of a public key. */
    bool FLKey::SetPubKey(const std::vector<uint8_t>& vchPubKeyIn)
    {
        /* Set the binary data. */
        vchPubKey = vchPubKeyIn;

        /* Set key as active. */
        fSet = true;

        return true;
    }


    /* Returns the Public key in a byte vector. */
    std::vector<uint8_t> FLKey::GetPubKey() const
    {
        return vchPubKey;
    }


    /* Based on standard set of byte data as input of any length. */
    bool FLKey::Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig) const
    {
        /* Check for null or no private key. */
        if(!fSet || vchPrivKey.empty())
            return false;

        /* Clear the signature data and resize. */
        vchSig.clear();
        vchSig.resize(1025); //NOTE: this is for log(9) for 512-bit (2n + 1)

        /* Create new signer context. */
        falcon_sign* signer = falcon_sign_new();

        /* Set the private key from stored private key. */
        if(falcon_sign_set_private_key(signer, &vchPrivKey[0], vchPrivKey.size()) != 1)
        {
            /* Free the context. */
            falcon_sign_free(signer);

            return false;
        }

        /* Randomly Generate the random number for the signature. */
        std::vector<uint8_t> vchRandom(40);
        if(falcon_sign_start(signer, &vchRandom[0]) != 1)
        {
            /* Free the context. */
            falcon_sign_free(signer);

            return false;
        }

        /* Load in the data to be signed. */
        falcon_sign_update(signer, &vchData[0], vchData.size());

        /* Get the new signature size. */
        int32_t nSize = falcon_sign_generate(signer, &vchSig[0], vchSig.size(),
            fCompressed ? FALCON_COMP_STATIC : FALCON_COMP_NONE);

        /* Check for errors. */
        if(nSize < 1)
        {
            /* Free the context. */
            falcon_sign_free(signer);

            return false;
        }

        /* Resize the signature data. */
        vchSig.resize(nSize);

        /* Add the random number to the end. */
        vchSig.insert(vchSig.end(), vchRandom.begin(), vchRandom.end());

        /* Free the context. */
        falcon_sign_free(signer);

        return true;
    }


    /* Signature Verification Function */
    bool FLKey::Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const
    {
        /* Check for null or no public key. */
        if(!fSet || vchPubKey.empty())
            return false;

        /* Create the verifier context. */
        falcon_vrfy* verifier = falcon_vrfy_new();

        /* Set the public key from internal data. */
        if(falcon_vrfy_set_public_key(verifier, &vchPubKey[0], vchPubKey.size()) != 1)
        {
            /* Free the context. */
            falcon_vrfy_free(verifier);

            return false;
        }

        /* Find the signature data (removing the random number). */
        uint32_t nSize = vchSig.size() - 40;

        /* Update the random number that's appened to end of signature. */
        falcon_vrfy_start(verifier, &vchSig[nSize], 40);

        /* Update the data that was signed. */
        falcon_vrfy_update(verifier, &vchData[0], vchData.size());

        /* Check for a positive result. */
        if(falcon_vrfy_verify(verifier, &vchSig[0], nSize) != 1)
        {
            /* Free the context. */
            falcon_vrfy_free(verifier);

            return false;
        }

        /* Free the context. */
        falcon_vrfy_free(verifier);

        return true;
    }


    /* Check if a Key is valid based on a few parameters. */
    bool FLKey::IsValid() const
    {
        if(!fSet)
            return false;

        return (!vchPubKey.empty() || !vchPrivKey.empty());
    }
}
