/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_INCLUDE_KEY_H
#define NEXUS_LLC_INCLUDE_KEY_H

#include <stdexcept>
#include <vector>

#include <LLC/types/uint1024.h>
#include <Util/include/allocators.h>

#include <openssl/ec.h> // for EC_KEY definition
#include <openssl/obj_mac.h>

namespace LLC
{

    /** Key Runtime Error Wrapper. **/
    class key_error : public std::runtime_error
    {
    public:
        explicit key_error(const std::string& str) : std::runtime_error(str) {}
    };


    /** CPrivKey is a serialized private key, with all parameters included **/
    typedef std::vector<uint8_t, secure_allocator<uint8_t> > CPrivKey;


    /** CSecret is a serialization of just the secret parameter **/
    typedef std::vector<uint8_t, secure_allocator<uint8_t> > CSecret;


    /** An encapsulated OpenSSL Elliptic Curve key (public and/or private) **/
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
        int nCurveID;


        /** The size of the curve finite field in bytes (Secret). **/
        uint32_t nKeySize;

    public:

        ECKey();
        ECKey(const ECKey& b);
        ECKey(const int nID, const int nKeySizeIn);
        ~ECKey();


        /** Assignment Operator **/
        ECKey& operator=(const ECKey& b);


        /** Comparison Operator **/
        bool operator==(const ECKey& b) const;


        /** Set a public key in Compression form **/
        void SetCompressedPubKey();


        /** Reset internal key data. **/
        void Reset();


        /** Is Null
         *
         *  @return True if the key is in NULL state
         **/
        bool IsNull() const;


        /** Is Compressed
         *
         *  Flag to determine if the key is in compressed form
         *
         *  @return True if the key is compressed
         **/
        bool IsCompressed() const;


        /** Make New Key
         *
         *  Create a new key from OpenSSL Library Pseudo-Random Generator
         *
         *  @param[in] fCompressed Flag whether to make key in compressed form
         **/
        void MakeNewKey(bool fCompressed);


        /** Set Priv Key
         *
         *  Set the key from full private key (including secret)
         *
         *  @param[in] vchPrivKey The key data in byte code in secure allocator
         *
         *  @return True if was set correctly
         **/
        bool SetPrivKey(const CPrivKey& vchPrivKey);


        /** Set Secret
         *
         *  Set the secret phrase / key used in the private key.
         *
         *  @param[in] vchSecret the secret phrase in byte code in secure allocator
         *  @param[in] fCompressed flag whether key is compressed or not
         *
         *  @return True if the key was successfully created
         **/
        bool SetSecret(const CSecret& vchSecret, bool fCompressed = false);


        /** Get Secret
         *
         *  Obtain the secret key used in the private key.
         *
         *  @param[in] fCompressed Flag if the key is in compressed from
         *
         *  @return the secret phrase in the secure allocator.
         **/
        CSecret GetSecret(bool &fCompressed) const;


        /** Get Private Key
         *
         *  Obtain the private key and all associated data
         *
         *  @param[in] fCompressed Flag if the key is in compressed from
         *
         *  @return the secret phrase in the secure allocator.
         **/
        CPrivKey GetPrivKey() const;


        /** Set Public Key
         *
         *  Returns true on the setting of a public key
         *
         *  @param[in] vchPubKey The public key to set
         *
         *  @return True if the key was set properly.
         **/
        bool SetPubKey(const std::vector<uint8_t>& vchPubKey);


        /** Get Public Key
         *
         *  Returns the Public key in a byte vector
         *
         *  @return The bytes of the public key in this keypair
         **/
        std::vector<uint8_t> GetPubKey() const;


        /** Nexus sepcific strict DER rules.
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
         *  @param[in] vchSig The DER encoded signature to check
         *
         *  @return true if encoding check passed.
         *
         **/
        bool Encoding(const std::vector<uint8_t> vchSig);


        /** Tritium Signing Function.
         *
         *  Based on standard set of byte data as input of any length. Checks for DER encoding
         *
         *  @param[in] vchData The input data to sign in bytes
         *  @param[out] vchSig The output data of the signature
         *
         *  @return True if the Signature was created successfully
         **/
        bool Sign(const std::vector<uint8_t> vchData, std::vector<uint8_t>& vchSig);


        /** Tritium Signature Verification Function
         *
         *  Based on standard set of byte data as input of any length. Checks for DER encoding
         *
         *  @param[in] vchData The input data to sign in bytes
         *  @param[in] vchSig The signature to check
         *
         *  @return True if the Signature was Verified as Valid
         **/
        bool Verify(const std::vector<uint8_t> vchData, const std::vector<uint8_t>& vchSig);


        /** Legacy Signing Function
         *
         *  Based on a 1024 bit hash and internal output length passed through type
         *
         *  @param[in] hash The input hash to be signed
         *  @param[out] vchSig The output signature data
         *  @param[in] nBits The total bits to use from param hash
         *
         *  @return True if the Signature was created successfully
         **/
        bool Sign(uint1024_t hash, std::vector<uint8_t>& vchSig, int nBits);


        /** Legacy Verifying Function.
         *
         *  Based on a 1024 bit hash and internal output length passed through type
         *
         *  @param[in] hash The input hash to be signed
         *  @param[in] vchSig The output signature data
         *  @param[in] nBits The total bits to use from param hash
         *
         *  @return True if the Signature was verified successfully
         **/
        bool Verify(uint1024_t hash, const std::vector<uint8_t>& vchSig, int nBits);


        /** Check if a Key is valid based on a few parameters
         *
         *  @return True if the Key is in a valid state
         **/
        bool IsValid();

    };
}
#endif
