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

namespace LLC
{
    /* The default constructor. */
    FLKey::FLKey()
    : vchPubKey   ( )
    , vchPrivKey  ( )
    , fSet        (false)
    , fCompressed (false)
    , ctx         ( )
    {

    }

    /** Copy Constructor. **/
    FLKey::FLKey(const FLKey& b)
    : vchPubKey   (b.vchPubKey)
    , vchPrivKey  (b.vchPrivKey)
    , fSet        (b.fSet)
    , fCompressed (b.fCompressed)
    , ctx         (b.ctx)
    {
    }


    /** Move Constructor. **/
    FLKey::FLKey(FLKey&& b) noexcept
    : vchPubKey   (std::move(b.vchPubKey))
    , vchPrivKey  (std::move(b.vchPrivKey))
    , fSet        (std::move(b.fSet))
    , fCompressed (std::move(b.fCompressed))
    , ctx         (std::move(b.ctx))
    {
    }


    /** Copy Assignment Operator **/
    FLKey& FLKey::operator=(const FLKey& b)
    {
        vchPubKey   = b.vchPubKey;
        vchPrivKey  = b.vchPrivKey;
        fSet        = b.fSet;
        fCompressed = b.fCompressed;
        ctx         = b.ctx;

        return *this;
    }


    /** Move Assignment Operator **/
    FLKey& FLKey::operator=(FLKey&& b) noexcept
    {
        vchPubKey   = std::move(b.vchPubKey);
        vchPrivKey  = std::move(b.vchPrivKey);
        fSet        = std::move(b.fSet);
        fCompressed = std::move(b.fCompressed);
        ctx         = std::move(b.ctx);

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

        /* Generate random seed from system. */
        if(shake256_init_prng_from_system(&ctx))
        {
            Reset();
            return;
        }

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
            return;
        }

        /* Show key as successfully set. */
        fSet = true;
    }


    /* Set the secret phrase / key used in the private key. */
    bool FLKey::SetSecret(const CSecret& vchSecret, bool fCompressedIn)
    {
        /* Flag if the key is compressed. */
        fCompressed = fCompressedIn;

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

        /* Clear the signature data and resize. */
        vchSig.clear();
        vchSig.resize(1025); //NOTE: this is for log(9) for 512-bit (2n + 1)

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_SIGNDYN(9), 0);

        /* Create the signed message. */
        size_t nSize = vchSig.size();
        if(falcon_sign_dyn(&ctx, &vchSig[0], &nSize, &vchPrivKey[0], vchPrivKey.size(), &vchData[0], vchData.size(), 1, &vchTemp[0], vchTemp.size()))
            return false;

        /* Resize the signature data. */
        vchSig.resize(nSize);

        return true;
    }


    /* Signature Verification Function */
    bool FLKey::Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const
    {
        /* Check for null or no public key. */
        if(!fSet || vchPubKey.empty())
            return false;

        /* Create temp memory. */
        std::vector<uint8_t> vchTemp(FALCON_TMPSIZE_VERIFY(9), 0);

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
}
