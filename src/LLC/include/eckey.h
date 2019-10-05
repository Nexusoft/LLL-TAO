/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_ECKEY_H
#define NEXUS_LLC_INCLUDE_ECKEY_H

#include <vector>

#include <LLC/types/uint1024.h>
#include <LLC/types/typedef.h>

typedef struct ec_key_st EC_KEY;


namespace LLC
{

    enum
    {
        SECT_571_R1 = 0,
        BRAINPOOL_P512_T1 = 1,
    };


    /** ECKey
     *
     *  An encapsulated OpenSSL Elliptic Curve key (public and/or private)
     *
     **/
    class ECKey
    {
    protected:
        /** The OpenSSL key object. **/
        EC_KEY* pkey;


        /** Flag to Determine if the Key has been set. **/
        bool fSet;


        /** Flag to Determine if the Key has been Compressed. **/
        bool fCompressedPubKey;


        /** The curve type implemented. **/
        int32_t nCurveID;


        /** The size of the curve finite field in bytes (Secret). **/
        uint32_t nKeySize;

    public:

        ECKey();
        ECKey(const ECKey& b);
        ECKey(const uint32_t nID, const uint32_t nKeySizeIn = 72);
        ~ECKey();


        /** Assignment Operator **/
        ECKey& operator=(const ECKey& b);


        /** Comparison Operator **/
        bool operator==(const ECKey& b) const;


        /** Set a public key in Compression form **/
        void SetCompressedPubKey();


        /** Reset internal key data. **/
        void Reset();


        /** IsNull
         *
         *  @return True if the key is in nullptr state, false otherwise.
         *
         **/
        bool IsNull() const;


        /** IsCompressed
         *
         *  Flag to determine if the key is in compressed form.
         *
         *  @return True if the key is compressed, false otherwise.
         *
         **/
        bool IsCompressed() const;


        /** MakeNewKey
         *
         *  Create a new key from OpenSSL Library Pseudo-Random Generator.
         *
         *  @param[in] fCompressed Flag whether to make key in compressed form.
         *
         **/
        void MakeNewKey(bool fCompressed);


        /** SetPrivKey
         *
         *  Set the key from full private key. (including secret)
         *
         *  @param[in] vchPrivKey The key data in byte code in secure allocator.
         *
         *  @return True if was set correctly, false otherwise.
         *
         **/
        bool SetPrivKey(const CPrivKey& vchPrivKey);


        /** SetSecret
         *
         *  Set the secret phrase / key used in the private key.
         *
         *  @param[in] vchSecret the secret phrase in byte code in secure allocator.
         *  @param[in] fCompressed flag whether key is compressed or not.
         *
         *  @return True if the key was successfully created.
         *
         **/
        bool SetSecret(const CSecret& vchSecret, bool fCompressed = false);


        /** GetSecret
         *
         *  Obtain the secret key used in the private key.
         *
         *  @param[in] fCompressed Flag if the key is in compressed form.
         *
         *  @return the secret phrase in the secure allocator.
         *
         **/
        CSecret GetSecret(bool &fCompressed) const;


        /** GetPrivKey
         *
         *  Obtain the private key and all associated data.
         *
         *  @param[in] fCompressed Flag if the key is in compressed form.
         *
         *  @return the secret phrase in the secure allocator.
         *
         **/
        CPrivKey GetPrivKey() const;


        /** SetPubKey
         *
         *  Returns true on the setting of a public key.
         *
         *  @param[in] vchPubKey The public key to set.
         *
         *  @return True if the key was set properly.
         *
         **/
        bool SetPubKey(const std::vector<uint8_t>& vchPubKey);


        /** GetPubKey
         *
         *  Returns the Public key in a byte vector.
         *
         *  @return The bytes of the public key in this keypair.
         *
         **/
        std::vector<uint8_t> GetPubKey() const;


        /** Encoding
         *
         *  Nexus sepcific strict DER rules.
         *
         *  Fixed length 135 bytes.
         *  Byte 0         - 0x30 - Header Byte
         *  Byte 1         - Length without header byte
         *  Byte 2         - Length without header or 0, 1, and 2
         *  Byte 3         - 0x20 Integer Flag
         *  Byte 4         - R length byte
         *  Byte 5 to 68   - R data
         *  Byte 69        - 0x20 Integer Flag
         *  Byte 70        - S length byte
         *  Byte 71 to 134 - S data
         *
         *  R length fixed to 64 Bytes
         *  S length fixed to 64 Bytes
         *
         *  Use strict encoding for easy handling of ledger level scripts.
         *
         *  @param[in] vchSig The DER encoded signature to check.
         *
         *  @return True if encoding check passed.
         *
         **/
        bool Encoding(const std::vector<uint8_t>& vchSig) const;


        /** Sign
         *
         *  Tritium Signing Function.
         *
         *  Based on standard set of byte data as input of any length. Checks for DER encoding.
         *
         *  @param[in] vchData The input data to sign in bytes.
         *  @param[out] vchSig The output data of the signature.
         *
         *  @return True if the Signature was created successfully.
         *
         **/
        bool Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig) const;


        /** Verify
         *
         *  Tritium Signature Verification Function
         *
         *  Based on standard set of byte data as input of any length. Checks for DER encoding.
         *
         *  @param[in] vchData The input data to sign in bytes.
         *  @param[in] vchSig The signature to check.
         *
         *  @return True if the Signature was Verified as Valid
         *
         **/
        bool Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const;


        /** Sign
         *
         *  Legacy Signing Function
         *
         *  Based on a 1024 bit hash and internal output length passed through type.
         *
         *  @param[in] hash The input hash to be signed.
         *  @param[out] vchSig The output signature data.
         *  @param[in] nBits The total bits to use from param hash.
         *
         *  @return True if the Signature was created successfully.
         *
         **/
        bool Sign(const uint1024_t& hash, std::vector<uint8_t>& vchSig, const uint32_t nBits) const;


        /** SignCompact
         *
         *  Legacy Signing Function
         *  create a compact signature (65 bytes), which allows reconstructing the used public key
         *  The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
         *  The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
         *                   0x1D = second key with even y, 0x1E = second key with odd
         *
         *  @param[in] hash The input hash to be signed.
         *  @param[out] vchSig The output signature data.
         *
         *  @return True if the Signature was created successfully.
         *
         **/
        bool SignCompact(uint256_t hash, std::vector<uint8_t>& vchSig);


        /** SetCompactSignature
         *
         *  Reconstruct public key from a compact signature.
         *  This is only slightly more CPU intensive than just verifying it.
         *  If this function succeeds, the recovered public key is guaranteed to be valid
         *  (the signature is a valid signature of the given data for that key).
         *
         *  @param[in] hash The input compact signature hash.
         *  @param[out] vchSig The output public key data.
         *
         *  @return True if the Signature was set.
         *
         **/
        bool SetCompactSignature(uint256_t hash, const std::vector<uint8_t>& vchSig);


        /** Verify
         *
         *  Legacy Verifying Function.
         *
         *  Based on a 1024 bit hash and internal output length passed through type.
         *
         *  @param[in] hash The input hash to be signed.
         *  @param[in] vchSig The output signature data.
         *  @param[in] nBits The total bits to use from param hash.
         *
         *  @return True if the Signature was verified successfully.
         *
         **/
        bool Verify(const uint1024_t& hash, const std::vector<uint8_t>& vchSig, const uint32_t nBits) const;


        /** IsValid
         *
         *  Check if a Key is valid based on a few parameters.
         *
         *  @return True if the Key is in a valid state.
         *
         **/
        bool IsValid() const;

    };
}
#endif
