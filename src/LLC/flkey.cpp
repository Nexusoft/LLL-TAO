/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <stdexcept>
#include <vector>

#include <LLC/types/uint1024.h>
#include <LLC/types/typedef.h>

#include <LLC/include/flkey.h>

namespace LLC
{
    /* The default constructor. */
    FLKey::FLKey()
    : vchPubKey   ( )
    , vchPrivKey  ( )
    , fSet        (false)
    , ctx         ( )
    , nVersion    (FalconVersion::FALCON_1024)
    {

    }

    /** Copy Constructor. **/
    FLKey::FLKey(const FLKey& b)
    : vchPubKey   (b.vchPubKey)
    , vchPrivKey  (b.vchPrivKey)
    , fSet        (b.fSet)
    , ctx         (b.ctx)
    , nVersion    (b.nVersion)
    {
    }


    /** Move Constructor. **/
    FLKey::FLKey(FLKey&& b) noexcept
    : vchPubKey   (std::move(b.vchPubKey))
    , vchPrivKey  (std::move(b.vchPrivKey))
    , fSet        (std::move(b.fSet))
    , ctx         (std::move(b.ctx))
    , nVersion    (std::move(b.nVersion))
    {
    }


    /** Copy Assignment Operator **/
    FLKey& FLKey::operator=(const FLKey& b)
    {
        vchPubKey   = b.vchPubKey;
        vchPrivKey  = b.vchPrivKey;
        fSet        = b.fSet;
        ctx         = b.ctx;
        nVersion    = b.nVersion;

        return *this;
    }


    /** Move Assignment Operator **/
    FLKey& FLKey::operator=(FLKey&& b) noexcept
    {
        vchPubKey   = std::move(b.vchPubKey);
        vchPrivKey  = std::move(b.vchPrivKey);
        fSet        = std::move(b.fSet);
        ctx         = std::move(b.ctx);
        nVersion    = std::move(b.nVersion);

        return *this;
    }

    /** Default Destructor. **/
    FLKey::~FLKey()
    {
    }


    /* Comparison Operator */
    bool FLKey::operator==(const FLKey& b) const
    {
        return (vchPubKey == b.vchPubKey);
    }


    /* Reset internal key data. */
    void FLKey::Reset()
    {
        vchPubKey.clear();
        vchPrivKey.clear();

        fSet = false;
        nVersion = FalconVersion::FALCON_1024;
    }


    /* Determine if the key is in nullptr state, false otherwise. */
    bool FLKey::IsNull() const
    {
        return !fSet;
    }


    /* Create a new key from the Falcon random PRNG seeds */
    void FLKey::MakeNewKey(FalconVersion ver)
    {
        /* Store the version */
        nVersion = ver;

        /* Determine logn based on version */
        unsigned int logn = (ver == FalconVersion::FALCON_1024) ? 10 : 9;

        /* Generate random seed from system. */
        if(shake256_init_prng_from_system(&ctx))
        {
            Reset();
            return;
        }

        /* Resize the allocators to expected sizes. */
        vchPubKey.resize(FALCON_PUBKEY_SIZE(logn));
        vchPrivKey.resize(FALCON_PRIVKEY_SIZE(logn));

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_KEYGEN(logn), 0);

        /* Generate the falcon key. */
        if(falcon_keygen_make(&ctx, logn,
            &vchPrivKey[0], vchPrivKey.size(),
            &vchPubKey[0],  vchPubKey.size(),
            &vchTemp[0],     vchTemp.size()))
        {
            Reset();
            return;
        }

        /* Show key as successfully set. */
        fSet = true;
    }


    /* Set the key from full private key. */
    bool FLKey::SetPrivKey(const CPrivKey& vchPrivKeyIn)
    {
        /* Auto-detect version from private key size */
        try
        {
            nVersion = DetectVersion(vchPrivKeyIn.size(), false);
        }
        catch(const std::exception& e)
        {
            Reset();
            return false;
        }

        /* Determine logn based on detected version */
        unsigned int logn = (nVersion == FalconVersion::FALCON_1024) ? 10 : 9;

        /* Validate input size */
        if(vchPrivKeyIn.size() != FALCON_PRIVKEY_SIZE(logn))
        {
            Reset();
            return false;
        }

        /* Set the private key. */
        vchPrivKey = vchPrivKeyIn;

        /* Derive the public key from the private key. */
        vchPubKey.resize(FALCON_PUBKEY_SIZE(logn));

        /* Create temp memory for public key extraction. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_KEYGEN(logn), 0);

        /* Extract public key from private key.
         * Note: Falcon private keys contain the public key internally,
         * so we can extract it using falcon's public key extraction.
         * For now, we'll use falcon_make_public which derives the public key. */
        if(falcon_make_public(&vchPubKey[0], vchPubKey.size(),
                              &vchPrivKey[0], vchPrivKey.size(),
                              &vchTemp[0], vchTemp.size()))
        {
            Reset();
            return false;
        }

        /* Show key as successfully set. */
        fSet = true;

        return true;
    }


    /* Set the secret phrase / key used in the private key. */
    bool FLKey::SetSecret(const CSecret& vchSecret)
    {
        /* Create the shake256 context. */
        shake256_init_prng_from_seed(&ctx, &vchSecret[0], vchSecret.size());

        /* Resize the allocators to expected sizes. */
        vchPubKey.resize(FALCON_PUBKEY_SIZE(9));
        vchPrivKey.resize(FALCON_PRIVKEY_SIZE(9));

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_KEYGEN(9), 0);

        /* Generate the falcon key. */
        if(falcon_keygen_make(&ctx, 9,
            &vchPrivKey[0], vchPrivKey.size(),
            &vchPubKey[0],  vchPubKey.size(),
            &vchTemp[0],     vchTemp.size()))
        {
            Reset();
            return false;
        }

        /* Show key as successfully set. */
        fSet = true;

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
        /* Auto-detect version from public key size */
        try
        {
            nVersion = DetectVersion(vchPubKeyIn.size(), true);
        }
        catch(const std::exception& e)
        {
            return false;
        }

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
    bool FLKey::Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig)
    {
        /* Check for null or no private key. */
        if(!fSet || vchPrivKey.empty())
            return false;

        /* Determine logn based on version */
        unsigned int logn = (nVersion == FalconVersion::FALCON_1024) ? 10 : 9;

        /* Clear the signature data and resize to maximum possible size. */
        vchSig.clear();
        vchSig.resize((logn == 10) ? 2049 : 1025); // (2n + 1) for dynamic signing

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_SIGNDYN(logn), 0);

        /* Create the signed message. */
        size_t nSize = vchSig.size();
        if(falcon_sign_dyn(&ctx, &vchSig[0], &nSize, &vchPrivKey[0], vchPrivKey.size(), &vchData[0], vchData.size(), 1, &vchTemp[0], vchTemp.size()))
            return false;

        /* Resize the signature data to actual size. */
        vchSig.resize(nSize);

        return true;
    }


    /* Signature Verification Function */
    bool FLKey::Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const
    {
        /* Check for null or no public key. */
        if(!fSet || vchPubKey.empty())
            return false;

        /* Determine logn based on version */
        unsigned int logn = (nVersion == FalconVersion::FALCON_1024) ? 10 : 9;

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_VERIFY(logn), 0);

        /* Verify the signed message. */
        if(falcon_verify(&vchSig[0], vchSig.size(), &vchPubKey[0], vchPubKey.size(), &vchData[0], vchData.size(), &vchTemp[0], vchTemp.size()))
            return false;

        return true;
    }


    /* Check if a Key is valid based on a few parameters. */
    bool FLKey::IsValid() const
    {
        if(!fSet)
            return false;

        return (!vchPubKey.empty() || !vchPrivKey.empty());
    }


    /* Get the Falcon version of this key. */
    FalconVersion FLKey::GetVersion() const
    {
        return nVersion;
    }


    /* Get the public key size for the current version. */
    size_t FLKey::GetPublicKeySize() const
    {
        return (nVersion == FalconVersion::FALCON_1024) 
            ? FalconSizes::FALCON1024_PUBLIC_KEY_SIZE 
            : FalconSizes::FALCON512_PUBLIC_KEY_SIZE;
    }


    /* Get the private key size for the current version. */
    size_t FLKey::GetPrivateKeySize() const
    {
        return (nVersion == FalconVersion::FALCON_1024) 
            ? FalconSizes::FALCON1024_PRIVATE_KEY_SIZE 
            : FalconSizes::FALCON512_PRIVATE_KEY_SIZE;
    }


    /* Get the typical signature size for the current version. */
    size_t FLKey::GetSignatureSize() const
    {
        return (nVersion == FalconVersion::FALCON_1024) 
            ? FalconSizes::FALCON1024_SIGNATURE_SIZE 
            : FalconSizes::FALCON512_SIGNATURE_SIZE;
    }


    /* Validate that the public key has the correct structure. */
    bool FLKey::ValidatePublicKey() const
    {
        if(vchPubKey.empty())
            return false;

        /* Check size matches expected size for version */
        size_t expected = GetPublicKeySize();
        if(vchPubKey.size() != expected)
            return false;

        /* Check header byte (first byte should match version encoding) */
        if(vchPubKey[0] != 0x00 && vchPubKey[0] != 0x01)
            return false;

        return true;
    }


    /* Validate that the private key has the correct structure. */
    bool FLKey::ValidatePrivateKey() const
    {
        if(vchPrivKey.empty())
            return false;

        /* Check size matches expected size for version */
        size_t expected = GetPrivateKeySize();
        if(vchPrivKey.size() != expected)
            return false;

        return true;
    }


    /* Securely wipe all key material. */
    void FLKey::Clear()
    {
        /* Securely wipe key data */
        if(!vchPubKey.empty())
        {
            std::fill(vchPubKey.begin(), vchPubKey.end(), 0);
            vchPubKey.clear();
        }

        if(!vchPrivKey.empty())
        {
            std::fill(vchPrivKey.begin(), vchPrivKey.end(), 0);
            vchPrivKey.clear();
        }

        /* Reset context */
        std::memset(&ctx, 0, sizeof(ctx));

        fSet = false;
        nVersion = FalconVersion::FALCON_512;
    }


    /* Auto-detect Falcon version from key size (static). */
    FalconVersion FLKey::DetectVersion(size_t keySize, bool isPublicKey)
    {
        if(isPublicKey)
        {
            if(keySize == FalconSizes::FALCON512_PUBLIC_KEY_SIZE)
                return FalconVersion::FALCON_512;
            else if(keySize == FalconSizes::FALCON1024_PUBLIC_KEY_SIZE)
                return FalconVersion::FALCON_1024;
            else
                throw std::invalid_argument("Invalid public key size: " + std::to_string(keySize));
        }
        else
        {
            if(keySize == FalconSizes::FALCON512_PRIVATE_KEY_SIZE)
                return FalconVersion::FALCON_512;
            else if(keySize == FalconSizes::FALCON1024_PRIVATE_KEY_SIZE)
                return FalconVersion::FALCON_1024;
            else
                throw std::invalid_argument("Invalid private key size: " + std::to_string(keySize));
        }
    }
}
