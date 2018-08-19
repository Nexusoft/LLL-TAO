/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
            
            (c) Copyright The Nexus Developers 2014 - 2018
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <map>

#include <openssl/ecdsa.h>

#include "include/key.h"

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
    
    // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
    // recid selects which key is recovered
    // if check is nonzero, additional checks are performed
    int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const uint8_t *msg, int msglen, int recid, int check)
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
        if (!BN_copy(x, order)) { ret=-3; goto err; }
        
        if (!BN_mul_word(x, i)) { ret=-4; goto err; }
        
        if (!BN_add(x, x, ecsig->r)) { ret=-5; goto err; }
        
        field = BN_CTX_get(ctx);
        
        if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-6; goto err; }
        
        if (BN_cmp(x, field) >= 0) { ret=-7; goto err; }
        
        if ((R = EC_POINT_new(group)) == NULL) { ret = -8; goto err; }
        
        if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=-9; goto err; }
        
        if (check)
        {
            if ((O = EC_POINT_new(group)) == NULL) { ret = -10; goto err; }
            if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-11; goto err; }
            if (!EC_POINT_is_at_infinity(group, O)) { ret = -12; goto err; }
        }
        
        if ((Q = EC_POINT_new(group)) == NULL) { ret = -13; goto err; }
        
        n = EC_GROUP_get_degree(group);
        e = BN_CTX_get(ctx);
        
        if (!BN_bin2bn(msg, msglen, e)) { ret=-14; goto err; }
        
        if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
        
        zero = BN_CTX_get(ctx);
        if (!BN_zero(zero)) { ret=-15; goto err; }
        
        if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-16; goto err; }
        
        rr = BN_CTX_get(ctx);
        if (!BN_mod_inverse(rr, ecsig->r, order, ctx)) { ret=-17; goto err; }
        
        sor = BN_CTX_get(ctx);
        if (!BN_mod_mul(sor, ecsig->s, rr, order, ctx)) { ret=-18; goto err; }
        
        eor = BN_CTX_get(ctx);
        
        if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-19; goto err; }
        if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-20; goto err; }
        if (!EC_KEY_set_public_key(eckey, Q)) { ret=-21; goto err; }

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


    void CKey::SetCompressedPubKey()
    {
        EC_KEY_set_conv_form(pkey, POINT_CONVERSION_COMPRESSED);
        fCompressedPubKey = true;
    }
    

    void CKey::Reset()
    {
        fCompressedPubKey = false;
        pkey = EC_KEY_new_by_curve_name(nCurveID);
        if (pkey == NULL)
            throw key_error("CKey::CKey() : EC_KEY_new_by_curve_name failed");
        fSet = false;
    }
    

    CKey::CKey()
    {
        /* Set the Default Curve ID as sect571r1 */
        nCurveID   = NID_sect571r1;
        nKeySize = 72;
        
        /* Reset the Current Key. */
        Reset();
    }
    
    
    CKey::CKey(const int nID, const int nKeySizeIn = 72)
    {
        /* Set the Curve Type. */
        nCurveID   = nID;
        nKeySize = nKeySizeIn;
        
        /* Reset the Current Key. */
        Reset();
    }
    

    CKey::CKey(const CKey& b)
    {
        pkey = EC_KEY_dup(b.pkey);
        if (pkey == NULL)
            throw key_error("CKey::CKey(const CKey&) : EC_KEY_dup failed");
        
        nCurveID   = b.nCurveID;
        nKeySize = b.nKeySize;
        fSet = b.fSet;
    }
    

    CKey& CKey::operator=(const CKey& b)
    {
        if (!EC_KEY_copy(pkey, b.pkey))
            throw key_error("CKey::operator=(const CKey&) : EC_KEY_copy failed");
        fSet = b.fSet;
        return (*this);
    }
    
    
    bool CKey::operator==(const CKey& b) const
    {
        return (fSet == b.fSet && nCurveID == b.nCurveID && nKeySize == b.nKeySize && GetPrivKey() == b.GetPrivKey());
    }
    

    CKey::~CKey()
    {
        EC_KEY_free(pkey);
    }
    

    bool CKey::IsNull() const
    {
        return !fSet;
    }
    

    bool CKey::IsCompressed() const
    {
        return fCompressedPubKey;
    }
    

    void CKey::MakeNewKey(bool fCompressed)
    {
        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(nCurveID);
        
        if (!EC_KEY_generate_key(pkey))
            throw key_error("CKey::MakeNewKey() : EC_KEY_generate_key failed");
        
        if (fCompressed)
            SetCompressedPubKey();
        fSet = true;
    }
    

    bool CKey::SetPrivKey(const CPrivKey& vchPrivKey)
    {
        const uint8_t* pbegin = &vchPrivKey[0];
        if (!d2i_ECPrivateKey(&pkey, &pbegin, vchPrivKey.size()))
            return false;
        
        fSet = true;
        return true;
    }
    

    bool CKey::SetSecret(const CSecret& vchSecret, bool fCompressed)
    {
        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(nCurveID);
        
        if (pkey == NULL)
            throw key_error("CKey::SetSecret() : EC_KEY_new_by_curve_name failed");
        
        if (vchSecret.size() != nKeySize)
            throw key_error("CKey::SetSecret() : secret key size mismatch");
        
        BIGNUM *bn = BN_bin2bn(&vchSecret[0], nKeySize, BN_new());
        if (bn == NULL)
            throw key_error("CKey::SetSecret() : BN_bin2bn failed");
        
        if (!EC_KEY_regenerate_key(pkey, bn))
        {
            BN_clear_free(bn);
            throw key_error("CKey::SetSecret() : EC_KEY_regenerate_key failed");
        }
        
        BN_clear_free(bn);
        
        fSet = true;
        if (fCompressed || fCompressedPubKey)
            SetCompressedPubKey();
        
        return true;
    }
    

    CSecret CKey::GetSecret(bool &fCompressed) const
    {
        CSecret vchRet;
        vchRet.resize(nKeySize);
        
        const BIGNUM *bn = EC_KEY_get0_private_key(pkey);
        int nBytes = BN_num_bytes(bn);
        if (bn == NULL)
            throw key_error("CKey::GetSecret() : EC_KEY_get0_private_key failed");
        
        int n = BN_bn2bin(bn, &vchRet[nKeySize - nBytes]);
        if (n != nBytes)
            throw key_error("CKey::GetSecret(): BN_bn2bin failed");
        
        fCompressed = fCompressedPubKey;
        return vchRet;
    }
    

    CPrivKey CKey::GetPrivKey() const
    {
        int nSize = i2d_ECPrivateKey(pkey, NULL);
        if (!nSize)
            throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey failed");
        
        CPrivKey vchPrivKey(nSize, 0);
        uint8_t* pbegin = &vchPrivKey[0];
        if (i2d_ECPrivateKey(pkey, &pbegin) != nSize)
            throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey returned unexpected size");
        
        return vchPrivKey;
    }
    

    bool CKey::SetPubKey(const std::vector<uint8_t>& vchPubKey)
    {
        const uint8_t* pbegin = &vchPubKey[0];
        if (!o2i_ECPublicKey(&pkey, &pbegin, vchPubKey.size()))
            return false;
        
        fSet = true;
        if (vchPubKey.size() >= 33)
            SetCompressedPubKey();
        
        return true;
    }
    

    std::vector<uint8_t> CKey::GetPubKey() const
    {
        int nSize = i2o_ECPublicKey(pkey, NULL);
        if (!nSize)
            throw key_error("CKey::GetPubKey() : i2o_ECPublicKey failed");
        
        std::vector<uint8_t> vchPubKey(nSize, 0);
        uint8_t* pbegin = &vchPubKey[0];
        if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
            throw key_error("CKey::GetPubKey() : i2o_ECPublicKey returned unexpected size");
        
        return vchPubKey;
    }
    
    
    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool CKey::SignCompact(uint256_t hash, std::vector<uint8_t>& vchSig)
    {
        bool fOk = false;
        ECDSA_SIG *sig = ECDSA_do_sign((uint8_t*)&hash, sizeof(hash), pkey);
        if (sig==NULL)
            return false;
        vchSig.clear();
        vchSig.resize(65,0);
        
        int nBitsR = BN_num_bits(sig->r);
        int nBitsS = BN_num_bits(sig->s);
        
        printf("R %i, S %i\n", nBitsR, nBitsS);
        if (nBitsR <= 256 && nBitsS <= 256)
        {
            int nRecId = -1;
            for (int i=0; i < 4; i++)
            {
                CKey keyRec;
                keyRec.fSet = true;
                if (fCompressedPubKey)
                    keyRec.SetCompressedPubKey();
                
                int nID = ECDSA_SIG_recover_key_GFp(keyRec.pkey, sig, (uint8_t*)&hash, sizeof(hash), i, 1);
                if (nID == 1)
                {
                    if (keyRec.GetPubKey() == this->GetPubKey())
                    {
                        nRecId = i;
                        break;
                    }
                }
                else 
                    printf("Key Error %i\n", nID);
            }

            if (nRecId == -1)
            {
                throw key_error("CKey::SignCompact() : unable to construct recoverable key");
            }
            
            vchSig[0] = nRecId + 27 + (fCompressedPubKey ? 4 : 0);
            BN_bn2bin(sig->r,&vchSig[33 - (nBitsR+7)/8]);
            BN_bn2bin(sig->s,&vchSig[65 - (nBitsS+7)/8]);
            fOk = true;
        }
        ECDSA_SIG_free(sig);
        return fOk;
    }

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool CKey::SetCompactSignature(uint256_t hash, const std::vector<uint8_t>& vchSig)
    {
        if (vchSig.size() != 145)
            return false;
        int nV = vchSig[0];
        if (nV<27 || nV>=35)
            return false;
        ECDSA_SIG *sig = ECDSA_SIG_new();
        BN_bin2bn(&vchSig[1],72,sig->r);
        BN_bin2bn(&vchSig[73],72,sig->s);

        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(NID_sect571r1);
        if (nV >= 31)
        {
            SetCompressedPubKey();
            nV -= 4;
        }
        if (ECDSA_SIG_recover_key_GFp(pkey, sig, (uint8_t*)&hash, sizeof(hash), nV - 27, 0) == 1)
        {
            fSet = true;
            ECDSA_SIG_free(sig);
            return true;
        }
        return false;
    }
    
    bool static IsEncodingDER(const std::vector<uint8_t> & sig)
    {
        // Format: 0x30 [total-length] 0x02 [R-length] [R] 0x02 [S-length] [S] [sighash]
        // * total-length: 1-byte length descriptor of everything that follows,
        //   excluding the sighash byte.
        // * R-length: 1-byte length descriptor of the R value that follows.
        // * R: arbitrary-length big-endian encoded R value. It must use the shortest
        //   possible encoding for a positive integers (which means no null bytes at
        //   the start, except a single one when the next byte has its highest bit set).
        // * S-length: 1-byte length descriptor of the S value that follows.
        // * S: arbitrary-length big-endian encoded S value. The same rules apply.
        // * sighash: 1-byte value indicating what data is hashed (not part of the DER
        //   signature)

        // Minimum and maximum size constraints.
        if (sig.size() < 9) return false;
        if (sig.size() > 73) return false;

        // A signature is of type 0x30 (compound).
        if (sig[0] != 0x30) return false;

        // Make sure the length covers the entire signature.
        if (sig[1] != sig.size() - 3) return false;

        // Extract the length of the R element.
        uint32_t lenR = sig[3];

        // Make sure the length of the S element is still inside the signature.
        if (5 + lenR >= sig.size()) return false;

        // Extract the length of the S element.
        uint32_t lenS = sig[5 + lenR];

        // Verify that the length of the signature matches the sum of the length
        // of the elements.
        if ((size_t)(lenR + lenS + 7) != sig.size()) return false;
    
        // Check whether the R element is an integer.
        if (sig[2] != 0x02) return false;

        // Zero-length integers are not allowed for R.
        if (lenR == 0) return false;

        // Negative numbers are not allowed for R.
        if (sig[4] & 0x80) return false;

        // Null bytes at the start of R are not allowed, unless R would
        // otherwise be interpreted as a negative number.
        if (lenR > 1 && (sig[4] == 0x00) && !(sig[5] & 0x80)) return false;

        // Check whether the S element is an integer.
        if (sig[lenR + 4] != 0x02) return false;

        // Zero-length integers are not allowed for S.
        if (lenS == 0) return false;

        // Negative numbers are not allowed for S.
        if (sig[lenR + 6] & 0x80) return false;

        // Null bytes at the start of S are not allowed, unless S would otherwise be
        // interpreted as a negative number.
        if (lenS > 1 && (sig[lenR + 6] == 0x00) && !(sig[lenR + 7] & 0x80)) return false;

        return true;
    }
    
    
    bool CKey::Sign(uint1024_t hash, std::vector<uint8_t>& vchSig, int nBits)
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
    
    
    bool CKey::Verify(uint1024_t hash, const std::vector<uint8_t>& vchSig, int nBits)
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
    

    bool CKey::Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig)
    {
        uint32_t nSize = ECDSA_size(pkey);
        vchSig.resize(nSize); // Make sure it is big enough

        /* Attempt the ECDSA Signing Operation. */
        if(ECDSA_sign(0, &vchData[0], vchData.size(), &vchSig[0], &nSize, pkey) != 1)
        {
            vchSig.clear();
            return false;
        }
        
        vchSig.resize(nSize);
        if(!IsEncodingDER(vchSig))
        {
            vchSig.clear();
            return false;
        }
        
        return true;
    }
    

    bool CKey::Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig)
    {
        return IsEncodingDER(vchSig) && 
              (ECDSA_verify(0, &vchData[0], vchData.size(), &vchSig[0], vchSig.size(), pkey) == 1);
    }
    

    
    bool CKey::IsValid()
    {
        if (!fSet)
            return false;

        bool fCompr;
        CSecret secret = GetSecret(fCompr);
        CKey key2;
        key2.SetSecret(secret, fCompr);
        return GetPubKey() == key2.GetPubKey();
    }
}
