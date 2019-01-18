/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <map>

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

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
        BN_CTX *ctx = nullptr;
        EC_POINT *pub_key = nullptr;

        if (!eckey) return 0;

        const EC_GROUP *group = EC_KEY_get0_group(eckey);

        if ((ctx = BN_CTX_new()) == nullptr)
            goto err;

        pub_key = EC_POINT_new(group);

        if (pub_key == nullptr)
            goto err;

        if (!EC_POINT_mul(group, pub_key, priv_key, nullptr, nullptr, ctx))
            goto err;

        EC_KEY_set_private_key(eckey, priv_key);
        EC_KEY_set_public_key(eckey, pub_key);

        ok = 1;

    err:

        if (pub_key)
            EC_POINT_free(pub_key);
        if (ctx != nullptr)
            BN_CTX_free(ctx);

        return(ok);
    }

    // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
    // recid selects which key is recovered
    // if check is nonzero, additional checks are performed
    int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, BIGNUM* sig_r, BIGNUM* sig_s, const unsigned char *msg, int msglen, int recid, int check)
    {
        if (!eckey) return 0;

        int ret = 0;
        BN_CTX *ctx = NULL;

        BIGNUM *x = NULL;
        BIGNUM *e = NULL;
        BIGNUM *order = NULL;
        BIGNUM *sor = NULL;
        BIGNUM *eor = NULL;
        BIGNUM *field = NULL;
        EC_POINT *R = NULL;
        EC_POINT *O = NULL;
        EC_POINT *Q = NULL;
        BIGNUM *rr = NULL;
        BIGNUM *zero = NULL;
        int n = 0;
        int i = recid / 2;

        const EC_GROUP *group = EC_KEY_get0_group(eckey);
        if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
        BN_CTX_start(ctx);
        order = BN_CTX_get(ctx);
        if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
        x = BN_CTX_get(ctx);
        if (!BN_copy(x, order)) { ret=-1; goto err; }
        if (!BN_mul_word(x, i)) { ret=-1; goto err; }
        if (!BN_add(x, x, sig_r)) { ret=-1; goto err; }
        field = BN_CTX_get(ctx);
        if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-2; goto err; }
        if (BN_cmp(x, field) >= 0) { ret=0; goto err; }
        if ((R = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=0; goto err; }
        if (check)
        {
            if ((O = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
            if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-2; goto err; }
            if (!EC_POINT_is_at_infinity(group, O)) { ret = 0; goto err; }
        }
        if ((Q = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        n = EC_GROUP_get_degree(group);
        e = BN_CTX_get(ctx);
        if (!BN_bin2bn(msg, msglen, e)) { ret=-1; goto err; }
        if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
        zero = BN_CTX_get(ctx);
        if (!BN_zero(zero)) { ret=-1; goto err; }
        if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-1; goto err; }
        rr = BN_CTX_get(ctx);
        if (!BN_mod_inverse(rr, sig_r, order, ctx)) { ret=-1; goto err; }
        sor = BN_CTX_get(ctx);
        if (!BN_mod_mul(sor, sig_s, rr, order, ctx)) { ret=-1; goto err; }
        eor = BN_CTX_get(ctx);
        if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-1; goto err; }
        if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-2; goto err; }
        if (!EC_KEY_set_public_key(eckey, Q)) { ret=-2; goto err; }

        ret = 1;

    err:
        if (ctx) {
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
        }
        if (R != NULL) EC_POINT_free(R);
        if (O != NULL) EC_POINT_free(O);
        if (Q != NULL) EC_POINT_free(Q);
        return ret;
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
        if (pkey == nullptr)
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
        if (pkey == nullptr)
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

        if (pkey == nullptr)
            throw key_error("ECKey::SetSecret() : EC_KEY_new_by_curve_name failed");

        if (vchSecret.size() != nKeySize)
            throw key_error("ECKey::SetSecret() : secret key size mismatch");

        BIGNUM *bn = BN_bin2bn(&vchSecret[0], nKeySize, BN_new());
        if (bn == nullptr)
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
        if (bn == nullptr)
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
        int nSize = i2d_ECPrivateKey(pkey, nullptr);
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
        int nSize = i2o_ECPublicKey(pkey, nullptr);
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
            return debug::error("length mismatch byte 1 ", vchSig[1], " ", vchSig.size());

        /* Ensure length is within range of second length indicator. */
        if (vchSig[2] != vchSig.size() - 3)
            return debug::error("length mismatch byte 2 ", vchSig[2], " - ", vchSig.size());

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

    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool ECKey::SignCompact(uint256_t hash, std::vector<unsigned char>& vchSig)
    {
        bool fOk = false;
        ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
        if (sig == nullptr)
            throw key_error("CKey::SignCompact() : Failed to make signature");

        vchSig.clear();
        vchSig.resize(145,0);

        const BIGNUM *sig_r = nullptr;
        const BIGNUM *sig_s = nullptr;
        int nBitsR = 0;
        int nBitsS = 0;

    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
        ECDSA_SIG_get0(sig, &sig_r, &sig_s);

        nBitsR = BN_num_bits(sig_r);
        nBitsS = BN_num_bits(sig_s);
    #else
        sig_r = sig->r;
        sig_s = sig->s;

        nBitsR = BN_num_bits(sig->r);
        nBitsS = BN_num_bits(sig->s);
    #endif


        if (nBitsR <= 571 && nBitsS <= 571)
        {
            int nRecId = -1;
            for (int i=0; i < 9; ++i)
            {
                ECKey keyRec;
                keyRec.fSet = true;
                if (fCompressedPubKey)
                    keyRec.SetCompressedPubKey();

                if (ECDSA_SIG_recover_key_GFp(keyRec.pkey, const_cast<BIGNUM *>(sig_r), const_cast<BIGNUM *>(sig_s), (unsigned char*)&hash, sizeof(hash), i, 1) == 1)
                {
                    if (keyRec.GetPubKey() == this->GetPubKey())
                    {
                        nRecId = i;
                        break;
                    }
                }
            }

            if (nRecId == -1)
            {
                ECDSA_SIG_free(sig);
                throw key_error("CKey::SignCompact() : unable to construct recoverable key");
            }

            vchSig[0] = nRecId+27+(fCompressedPubKey ? 4 : 0);

            BN_bn2bin(sig_r, &vchSig[73-(nBitsR+7)/8]);
            BN_bn2bin(sig_s, &vchSig[145-(nBitsS+7)/8]);

            fOk = true;
        }
        ECDSA_SIG_free(sig);

        return fOk;
    }

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool ECKey::SetCompactSignature(uint256_t hash, const std::vector<unsigned char>& vchSig)
    {
        if (vchSig.size() != 145)
            return false;
        int nV = vchSig[0];
        if (nV<27 || nV>=35)
            return false;
        ECDSA_SIG* sig = ECDSA_SIG_new();
        if (nullptr == sig)
            return false;

        #if OPENSSL_VERSION_NUMBER > 0x10100000L
            // sig_r and sig_s will be deallocated by ECDSA_SIG_free(sig)
            BIGNUM* sig_r = BN_bin2bn(&vchSig[1], 72, BN_new());
            BIGNUM* sig_s = BN_bin2bn(&vchSig[73], 72, BN_new());

            if ((sig_r == nullptr) || (sig_s == nullptr))
            {
                ECDSA_SIG_free(sig);
                throw key_error("CKey::VerifyCompact() : r or s can't be null");
            }

            // set r and s values, this transfers ownership to the ECDSA_SIG object
            ECDSA_SIG_set0(sig, sig_r, sig_s);
        #else
            BN_bin2bn(&vchSig[1], 72, sig->r);
            BN_bin2bn(&vchSig[73], 72, sig->s);
        #endif

        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(NID_sect571r1);
        if (nV >= 31)
        {
            SetCompressedPubKey();
            nV -= 4;
        }

        #if OPENSSL_VERSION_NUMBER > 0x10100000L
        if (ECDSA_SIG_recover_key_GFp(pkey, sig_r, sig_s, (unsigned char*)&hash, sizeof(hash), nV - 27, 0) == 1)
        {
            fSet = true;
            ECDSA_SIG_free(sig);
            return true;
        }
        #else
        if (ECDSA_SIG_recover_key_GFp(pkey, sig->r, sig->s, (unsigned char*)&hash, sizeof(hash), nV - 27, 0) == 1)
        {
            fSet = true;
            ECDSA_SIG_free(sig);
            return true;
        }
        #endif

        ECDSA_SIG_free(sig);
        return false;
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
