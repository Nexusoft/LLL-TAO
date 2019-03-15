/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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

namespace LLC
{


    /** FLKey
     *
     *  An encapsulated FALCON Key
     *  Falcon is a post-quantum lattice based signature scheme
     *
     *  It stands for Fast-Fourier Lattice-based Compact Signatures Over NTRU
     *  This class uses a LOG value of 9, for 512-bit keys. It's relative
     *  Classical security parameters are equivilent to RSA-2048.
     *
     *  It is considered a quantum resistant signature scheme and is a second
     *  Round candidate out of 9 other for the NIST post-quantum competetition:
     *
     *  https://csrc.nist.gov/Projects/Post-Quantum-Cryptography/Round-2-Submissions
     *
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


        /** Flag to Determine if the Key has been Compressed. **/
        bool fCompressedPubKey;


    public:

        FLKey();
        FLKey(const FLKey& b);
        ~FLKey()
        {
            
        }


        /** Assignment Operator **/
        FLKey& operator=(const FLKey& b);


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
        bool Sign(const std::vector<uint8_t>& vchData, std::vector<uint8_t>& vchSig) const;


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

    };
}
#endif
