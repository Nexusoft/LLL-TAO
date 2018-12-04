/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <map>

#include <openssl/ecdsa.h>

#include <LLC/include/key.h>

#include <Util/include/debug.h>
#include <Util/include/hex.h>
#include <Util/include/mutex.h>

namespace LLC
{

    // Generate a private key from just the secret parameter
    int EC_KEY_regenerate_key(EC_KEY *eckey, BIGNUM *priv_key)
    {
        int ok = 0;
        BN_CTX *ctx = NULL;
        EC_POINT *pub_key = NULL;

        if (!eckey) return 0;

        const EC_GROUP *group = EC_KEY_get0_group(eckey);

        if ((ctx = BN_CTX_new()) == NULL)
            goto err;

        pub_key = EC_POINT_new(group);

        if (pub_key == NULL)
            goto err;

        if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx))
            goto err;

        EC_KEY_set_private_key(eckey, priv_key);
        EC_KEY_set_public_key(eckey, pub_key);

        ok = 1;

    err:

        if (pub_key)
            EC_POINT_free(pub_key);
        if (ctx != NULL)
            BN_CTX_free(ctx);

        return(ok);
    }


    /* Consturctor with default curve type. */
    ECKey::ECKey()
    {
        /* Set the Default Curve ID as sect571r1 */
        nCurveID   = NID_sect571r1;
        nKeySize = 72;

        /* Reset the Current Key. */
        Reset();
    }


    /* Constructor from a new curve type. */
    ECKey::ECKey(const int nID, const int nKeySizeIn = 72)
    {
        /* Set the Curve Type. */
        nCurveID   = nID;
        nKeySize = nKeySizeIn;

        /* Reset the Current Key. */
        Reset();
    }


    /* Constructor from a ECKey object. */
    ECKey::ECKey(const ECKey& b)
    {
        pkey = EC_KEY_dup(b.pkey);
        if (pkey == NULL)
            throw key_error("ECKey::ECKey(const ECKey&) : EC_KEY_dup failed");

        nCurveID   = b.nCurveID;
        nKeySize = b.nKeySize;
        fSet = b.fSet;
    }


    /* Key destructor */
    ECKey::~ECKey()
    {
        EC_KEY_free(pkey);
    }


    /* Assignment Operator */
    ECKey& ECKey::operator=(const ECKey& b)
    {
        if (!EC_KEY_copy(pkey, b.pkey))
            throw key_error("ECKey::operator=(const ECKey&) : EC_KEY_copy failed");
        fSet = b.fSet;
        return (*this);
    }


    /* Comparison Operator */
    bool ECKey::operator==(const ECKey& b) const
    {
        return (fSet == b.fSet && nCurveID == b.nCurveID && nKeySize == b.nKeySize && GetPrivKey() == b.GetPrivKey());
    }


    /* Set a public key in Compression form */
    void ECKey::SetCompressedPubKey()
    {
        EC_KEY_set_conv_form(pkey, POINT_CONVERSION_COMPRESSED);
        fCompressedPubKey = true;
    }


    /* Reset internal key data. */
    void ECKey::Reset()
    {
        fCompressedPubKey = false;
        pkey = EC_KEY_new_by_curve_name(nCurveID);
        if (pkey == NULL)
            throw key_error("ECKey::ECKey() : EC_KEY_new_by_curve_name failed");
        fSet = false;
    }


    /* Determines if key is in null state */
    bool ECKey::IsNull() const
    {
        return !fSet;
    }


    /* Flag to determine if the key is in compressed form */
    bool ECKey::IsCompressed() const
    {
        return fCompressedPubKey;
    }


    /* Create a new key from OpenSSL Library Pseudo-Random Generator */
    void ECKey::MakeNewKey(bool fCompressed)
    {
        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(nCurveID);

        if (!EC_KEY_generate_key(pkey))
            throw key_error("ECKey::MakeNewKey() : EC_KEY_generate_key failed");

        if (fCompressed)
            SetCompressedPubKey();
        fSet = true;
    }


    /* Set the key from full private key (including secret) */
    bool ECKey::SetPrivKey(const CPrivKey& vchPrivKey)
    {
        const uint8_t* pbegin = &vchPrivKey[0];
        if (!d2i_ECPrivateKey(&pkey, &pbegin, vchPrivKey.size()))
            return false;

        fSet = true;
        return true;
    }


    /* Set the secret phrase / key used in the private key. */
    bool ECKey::SetSecret(const CSecret& vchSecret, bool fCompressed)
    {
        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(nCurveID);

        if (pkey == NULL)
            throw key_error("ECKey::SetSecret() : EC_KEY_new_by_curve_name failed");

        if (vchSecret.size() != nKeySize)
            throw key_error("ECKey::SetSecret() : secret key size mismatch");

        BIGNUM *bn = BN_bin2bn(&vchSecret[0], nKeySize, BN_new());
        if (bn == NULL)
            throw key_error("ECKey::SetSecret() : BN_bin2bn failed");

        if (!EC_KEY_regenerate_key(pkey, bn))
        {
            BN_clear_free(bn);
            throw key_error("ECKey::SetSecret() : EC_KEY_regenerate_key failed");
        }

        BN_clear_free(bn);

        fSet = true;
        if (fCompressed || fCompressedPubKey)
            SetCompressedPubKey();

        return true;
    }


    /* Obtain the secret key used in the private key. */
    CSecret ECKey::GetSecret(bool &fCompressed) const
    {
        CSecret vchRet;
        vchRet.resize(nKeySize);

        const BIGNUM *bn = EC_KEY_get0_private_key(pkey);
        int nBytes = BN_num_bytes(bn);
        if (bn == NULL)
            throw key_error("ECKey::GetSecret() : EC_KEY_get0_private_key failed");

        int n = BN_bn2bin(bn, &vchRet[nKeySize - nBytes]);
        if (n != nBytes)
            throw key_error("ECKey::GetSecret(): BN_bn2bin failed");

        fCompressed = fCompressedPubKey;
        return vchRet;
    }


    /* Obtain the private key and all associated data */
    CPrivKey ECKey::GetPrivKey() const
    {
        int nSize = i2d_ECPrivateKey(pkey, NULL);
        if (!nSize)
            throw key_error("ECKey::GetPrivKey() : i2d_ECPrivateKey failed");

        CPrivKey vchPrivKey(nSize, 0);
        uint8_t* pbegin = &vchPrivKey[0];
        if (i2d_ECPrivateKey(pkey, &pbegin) != nSize)
            throw key_error("ECKey::GetPrivKey() : i2d_ECPrivateKey returned unexpected size");

        return vchPrivKey;
    }


    /* Returns true on the setting of a public key */
    bool ECKey::SetPubKey(const std::vector<uint8_t>& vchPubKey)
    {
        const uint8_t* pbegin = &vchPubKey[0];
        if (!o2i_ECPublicKey(&pkey, &pbegin, vchPubKey.size()))
            return false;

        fSet = true;
        if (vchPubKey.size() >= 33)
            SetCompressedPubKey();

        return true;
    }


    /* Returns the Public key in a byte vector */
    std::vector<uint8_t> ECKey::GetPubKey() const
    {
        int nSize = i2o_ECPublicKey(pkey, NULL);
        if (!nSize)
            throw key_error("ECKey::GetPubKey() : i2o_ECPublicKey failed");

        std::vector<uint8_t> vchPubKey(nSize, 0);
        uint8_t* pbegin = &vchPubKey[0];
        if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
            throw key_error("ECKey::GetPubKey() : i2o_ECPublicKey returned unexpected size");

        return vchPubKey;
    }


    /* Nexus sepcific strict DER rules. */
    bool ECKey::Encoding(const std::vector<uint8_t> vchSig)
    {
        /* Check the signature length. Strict encoding requires no more than 135 bytes. */
        if (vchSig.size() != 135) return false;

        /* Leading byte needs to be 0x30 (compound) */
        if (vchSig[0] != 0x30) return false;

        /* Ensure length is within range of first length indicator. */
        if (vchSig[1] != vchSig.size() - 6)
            return debug::error("length mismatch byte 1 %u %u", vchSig[1], vchSig.size());

        /* Ensure length is within range of second length indicator. */
        if (vchSig[2] != vchSig.size() - 3)
            return debug::error("length mismatch byte 2 %u - %u", vchSig[2], vchSig.size());

        /* Byte 3 needs to indicate integer value for R (0x02) */
        if (vchSig[3] != 0x02)
            return debug::error("R is not an integer");

        /* Byte 4 is length of R value. Extract it. */
        uint32_t lenR = vchSig[4];

        /* Enforce strict rule of 64 byte R value. */
        if(lenR != nKeySize)
            return false;

        /* Make sure R value is not negative (0x80). */
        if (vchSig[5] & 0x80)
            return debug::error("negatives not allowed for R");

        /* No Null value to pad R, unless interpreted as negative. */
        if ((vchSig[5] == 0x00) && !(vchSig[6] & 0x80))
            return debug::error("no null bytes at start of R");

        /* Ensure S is flagged as integer (0x02) */
        if (vchSig[5 + lenR] != 0x02)
            return debug::error("S is not an integer");

        /* Extract the length of S. */
        uint32_t lenS = vchSig[6 + lenR];

        /* Enforce strict rule of 64 byte S value. */
        if(lenS != nKeySize)
            return false;

        /* Make sure S value is not negative (0x80) */
        if (vchSig[lenR + 7] & 0x80)
            return debug::error("no negatives allowed for S");

        /* No Null value to pad S, unless interpreted as negative. */
        if ((vchSig[lenR + 7] == 0x00) && !(vchSig[lenR + 8] & 0x80))
            return debug::error("no null bytes at start of S");

        return true;
    }


    /* Based on standard set of byte data as input of any length. Checks for DER encoding */
    bool ECKey::Sign(const std::vector<uint8_t> vchData, std::vector<uint8_t>& vchSig)
    {
        uint32_t nSize = ECDSA_size(pkey);
        vchSig.resize(nSize); // Make sure it is big enough

        /* Attempt the ECDSA Signing Operation. */
        if(ECDSA_sign(0, &vchData[0], vchData.size(), &vchSig[0], &nSize, pkey) != 1)
        {
            vchSig.clear();
            return debug::error("Failed to Sign");
        }

        vchSig.resize(nSize);
        if(!Encoding(vchSig))
            return Sign(vchData, vchSig);

        return true;
    }


    /* Tritium Signature Verification Function */
    bool ECKey::Verify(const std::vector<uint8_t> vchData, const std::vector<uint8_t>& vchSig)
    {
        return Encoding(vchSig) &&
            (ECDSA_verify(0, &vchData[0], vchData.size(), &vchSig[0], vchSig.size(), pkey) == 1);
    }


    /* Legacy Signing Function */
    bool ECKey::Sign(uint1024_t hash, std::vector<uint8_t>& vchSig, int nBits)
    {
        uint32_t nSize = ECDSA_size(pkey);
        vchSig.resize(nSize); // Make sure it is big enough

        bool fSuccess = false;
        if(nBits == 256)
        {
            uint256_t hash256 = hash.getuint256();
            fSuccess = (ECDSA_sign(0, (uint8_t*)&hash256, sizeof(hash256), &vchSig[0], &nSize, pkey) == 1);
        }
        else if(nBits == 512)
        {
            uint512_t hash512 = hash.getuint512();
            fSuccess = (ECDSA_sign(0, (uint8_t*)&hash512, sizeof(hash512), &vchSig[0], &nSize, pkey) == 1);
        }
        else
            fSuccess = (ECDSA_sign(0, (uint8_t*)&hash, sizeof(hash), &vchSig[0], &nSize, pkey) == 1);

        if(!fSuccess)
        {
            vchSig.clear();
            return false;
        }

        vchSig.resize(nSize); // Shrink to fit actual size
        return true;
    }


    /* Legacy Verifying Function.*/
    bool ECKey::Verify(uint1024_t hash, const std::vector<uint8_t>& vchSig, int nBits)
    {
        bool fSuccess = false;
        if(nBits == 256)
        {
            uint256_t hash256 = hash.getuint256();
            fSuccess = (ECDSA_verify(0, (uint8_t*)&hash256, sizeof(hash256), &vchSig[0], vchSig.size(), pkey) == 1);
        }
        else if(nBits == 512)
        {
            uint512_t hash512 = hash.getuint512();
            fSuccess = (ECDSA_verify(0, (uint8_t*)&hash512, sizeof(hash512), &vchSig[0], vchSig.size(), pkey) == 1);
        }
        else
            fSuccess = (ECDSA_verify(0, (uint8_t*)&hash, sizeof(hash), &vchSig[0], vchSig.size(), pkey) == 1);

        return fSuccess;
    }


    /* Check if a Key is valid based on a few parameters*/
    bool ECKey::IsValid()
    {
        if (!fSet)
            return false;

        bool fCompr;
        CSecret secret = GetSecret(fCompr);
        ECKey key2;
        key2.SetSecret(secret, fCompr);
        return GetPubKey() == key2.GetPubKey();
    }



}
