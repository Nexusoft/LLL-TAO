/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_FLKEY_H
#define NEXUS_LLC_INCLUDE_FLKEY_H

#include <stdexcept>
#include <vector>

#include <LLC/types/uint1024.h>
#include <LLC/types/typedef.h>

#include <LLC/falcon/falcon.h>

namespace LLC
{
    /** Falcon Version Enumeration
     *
     *  Defines supported Falcon signature variants with different security levels.
     **/
    enum class FalconVersion : uint8_t
    {
        FALCON_512  = 1,  // logn=9, NIST Level 1, 128-bit quantum security
        FALCON_1024 = 2   // logn=10, NIST Level 5, 256-bit quantum security
    };


    /** Falcon Size Constants Namespace
     *
     *  Compile-time constants for Falcon-512 and Falcon-1024 key and signature sizes.
     **/
    namespace FalconSizes
    {
        // Falcon-512 (logn=9) sizes
        constexpr size_t FALCON512_PUBLIC_KEY_SIZE = 897;
        constexpr size_t FALCON512_PRIVATE_KEY_SIZE = 1281;
        constexpr size_t FALCON512_SIGNATURE_SIZE = 809;      // Constant-time size (default, ct=1)
        constexpr size_t FALCON512_SIGNATURE_CT_SIZE = 809;   // Constant-time size
        constexpr size_t FALCON512_SIGNATURE_COMPRESSED_MIN = 666;  // Minimum compressed size
        constexpr size_t FALCON512_SIGNATURE_COMPRESSED_AVG = 690;  // Average compressed size

        // Falcon-1024 (logn=10) sizes
        constexpr size_t FALCON1024_PUBLIC_KEY_SIZE = 1793;
        constexpr size_t FALCON1024_PRIVATE_KEY_SIZE = 2305;
        constexpr size_t FALCON1024_SIGNATURE_SIZE = 1577;    // Constant-time size (default, ct=1)
        constexpr size_t FALCON1024_SIGNATURE_CT_SIZE = 1577; // Constant-time size
        constexpr size_t FALCON1024_SIGNATURE_COMPRESSED_MIN = 1280;  // Minimum compressed size
        constexpr size_t FALCON1024_SIGNATURE_COMPRESSED_AVG = 1330;  // Average compressed size
    }


    /** FLKey
     *
     *  An encapsulated FALCON Key with support for both Falcon-512 and Falcon-1024
     *  Falcon is a post-quantum lattice based signature scheme
     *
     *  It stands for Fast-Fourier Lattice-based Compact Signatures Over NTRU
     *  
     *  Falcon-512 (logn=9): NIST Level 1, ~128-bit quantum security, RSA-2048 equivalent
     *  Falcon-1024 (logn=10): NIST Level 5, ~256-bit quantum security, RSA-4096 equivalent
     *
     *  It is considered a quantum resistant signature scheme and is a finalist in the
     *  NIST post-quantum cryptography standardization:
     *
     *  https://csrc.nist.gov/Projects/post-quantum-cryptography
     *
     **/
    class FLKey
    {
    protected:

        /* The contained falcon public key. */
        std::vector<uint8_t> vchPubKey;

        /* The contained falcon private key. */
        CPrivKey vchPrivKey;


        /** Flag to Determine if the Key has been set. **/
        bool fSet;


        /** FALCON context. **/
        shake256_context ctx;


        /** Falcon version (512 or 1024). **/
        FalconVersion nVersion;


    public:

        /** Default Constructor. **/
        FLKey();


        /** Copy Constructor. **/
        FLKey(const FLKey& b);


        /** Move Constructor. **/
        FLKey(FLKey&& b) noexcept;


        /** Copy Assignment Operator **/
        FLKey& operator=(const FLKey& b);


        /** Move Assignment Operator **/
        FLKey& operator=(FLKey&& b) noexcept;


        /** Default Destructor. **/
        ~FLKey();


        /** Comparison Operator **/
        bool operator==(const FLKey& b) const;


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
         *  Create a new key from the Falcon random PRNG seeds
         *
         *  @param[in] ver Falcon version (512 or 1024). Default: Falcon-1024
         *
         **/
        void MakeNewKey(FalconVersion ver = FalconVersion::FALCON_1024);


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
         *
         *  @return True if the key was successfully created.
         *
         **/
        bool SetSecret(const CSecret& vchSecret);


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
        bool SetPubKey(const std::vector<uint8_t>& vchPubKeyIn);


        /** GetPubKey
         *
         *  Returns the Public key in a byte vector.
         *
         *  @return The bytes of the public key in this keypair.
         *
         **/
        std::vector<uint8_t> GetPubKey() const;


        /** Sign
         *
         *  Signing Function.
         *
         *  Based on standard set of byte data as input of any length.
         *
         *  @param[in] vchData The input data to sign in bytes.
         *  @param[out] vchSig The output data of the signature.
         *
         *  @return True if the Signature was created successfully.
         *
         **/
        bool Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig);


        /** Verify
         *
         *  Signature Verification Function
         *
         *  Based on standard set of byte data as input of any length.
         *
         *  @param[in] vchData The input data to sign in bytes.
         *  @param[in] vchSig The signature to check.
         *
         *  @return True if the Signature was Verified as Valid
         *
         **/
        bool Verify(const std::vector<uint8_t>& vchData, const std::vector<uint8_t>& vchSig) const;


        /** IsValid
         *
         *  Check if a Key is valid based on a few parameters.
         *
         *  @return True if the Key is in a valid state.
         *
         **/
        bool IsValid() const;


        /** GetVersion
         *
         *  Get the Falcon version of this key.
         *
         *  @return FalconVersion (FALCON_512 or FALCON_1024)
         *
         **/
        FalconVersion GetVersion() const;


        /** GetPublicKeySize
         *
         *  Get the public key size for the current version.
         *
         *  @return Public key size in bytes
         *
         **/
        size_t GetPublicKeySize() const;


        /** GetPrivateKeySize
         *
         *  Get the private key size for the current version.
         *
         *  @return Private key size in bytes
         *
         **/
        size_t GetPrivateKeySize() const;


        /** GetSignatureSize
         *
         *  Get the typical signature size for the current version.
         *
         *  @return Signature size in bytes
         *
         **/
        size_t GetSignatureSize() const;


        /** ValidatePublicKey
         *
         *  Validate that the public key has the correct structure.
         *
         *  @return True if public key is valid
         *
         **/
        bool ValidatePublicKey() const;


        /** ValidatePrivateKey
         *
         *  Validate that the private key has the correct structure.
         *
         *  @return True if private key is valid
         *
         **/
        bool ValidatePrivateKey() const;


        /** Clear
         *
         *  Securely wipe all key material.
         *
         **/
        void Clear();


        /** DetectVersion (static)
         *
         *  Auto-detect Falcon version from key size.
         *
         *  @param[in] keySize Size of key in bytes
         *  @param[in] isPublicKey True if detecting public key, false for private key
         *
         *  @return Detected FalconVersion, or throws if size doesn't match any version
         *
         **/
        static FalconVersion DetectVersion(size_t keySize, bool isPublicKey = true);

    };
}
#endif
