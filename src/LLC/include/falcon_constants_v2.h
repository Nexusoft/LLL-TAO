/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_INCLUDE_FALCON_CONSTANTS_V2_H
#define NEXUS_LLC_INCLUDE_FALCON_CONSTANTS_V2_H

#include <cstddef>
#include <cstdint>
#include <LLC/include/flkey.h>

namespace LLC
{
namespace FalconConstants
{
    /***************************************************************************
     * Version-Agnostic Constants for Falcon-512 and Falcon-1024
     * 
     * This header provides runtime size getters that work with both Falcon
     * variants, enabling seamless protocol operation in "stealth mode" where
     * nodes accept both Falcon-512 and Falcon-1024 signatures.
     **************************************************************************/

    /** Namespace for Falcon-512 specific constants **/
    namespace Falcon512
    {
        constexpr size_t PUBLIC_KEY_SIZE = FalconSizes::FALCON512_PUBLIC_KEY_SIZE;
        constexpr size_t PRIVATE_KEY_SIZE = FalconSizes::FALCON512_PRIVATE_KEY_SIZE;
        constexpr size_t SIGNATURE_SIZE = FalconSizes::FALCON512_SIGNATURE_SIZE;  // CT default
        constexpr size_t SIGNATURE_CT_SIZE = FalconSizes::FALCON512_SIGNATURE_CT_SIZE;
    }

    /** Namespace for Falcon-1024 specific constants **/
    namespace Falcon1024
    {
        constexpr size_t PUBLIC_KEY_SIZE = FalconSizes::FALCON1024_PUBLIC_KEY_SIZE;
        constexpr size_t PRIVATE_KEY_SIZE = FalconSizes::FALCON1024_PRIVATE_KEY_SIZE;
        constexpr size_t SIGNATURE_SIZE = FalconSizes::FALCON1024_SIGNATURE_SIZE;  // CT default
        constexpr size_t SIGNATURE_CT_SIZE = FalconSizes::FALCON1024_SIGNATURE_CT_SIZE;
    }

    /** ChaCha20-Poly1305 AEAD Encryption Constants **/
    constexpr size_t CHACHA20_NONCE_SIZE = 12;
    constexpr size_t CHACHA20_AUTH_TAG_SIZE = 16;
    constexpr size_t CHACHA20_OVERHEAD = CHACHA20_NONCE_SIZE + CHACHA20_AUTH_TAG_SIZE;

    /** Protocol Field Sizes **/
    constexpr size_t VERSION_FIELD_SIZE = 1;
    constexpr size_t TIMESTAMP_SIZE = 8;
    constexpr size_t LENGTH_FIELD_SIZE = 2;

    /***************************************************************************
     * Runtime Size Getters
     * 
     * These functions return the appropriate size based on the Falcon version.
     **************************************************************************/

    /** GetPublicKeySize
     *
     *  Get public key size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *
     *  @return Public key size in bytes
     *
     **/
    inline size_t GetPublicKeySize(FalconVersion version)
    {
        return (version == FalconVersion::FALCON_1024) 
            ? Falcon1024::PUBLIC_KEY_SIZE 
            : Falcon512::PUBLIC_KEY_SIZE;
    }

    /** GetPrivateKeySize
     *
     *  Get private key size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *
     *  @return Private key size in bytes
     *
     **/
    inline size_t GetPrivateKeySize(FalconVersion version)
    {
        return (version == FalconVersion::FALCON_1024) 
            ? Falcon1024::PRIVATE_KEY_SIZE 
            : Falcon512::PRIVATE_KEY_SIZE;
    }

    /** GetSignatureSize
     *
     *  Get typical signature size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *
     *  @return Signature size in bytes
     *
     **/
    inline size_t GetSignatureSize(FalconVersion version)
    {
        return (version == FalconVersion::FALCON_1024) 
            ? Falcon1024::SIGNATURE_SIZE 
            : Falcon512::SIGNATURE_SIZE;
    }

    /** GetSignatureCTSize
     *
     *  Get constant-time signature size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *
     *  @return Constant-time signature size in bytes
     *
     **/
    inline size_t GetSignatureCTSize(FalconVersion version)
    {
        return (version == FalconVersion::FALCON_1024) 
            ? Falcon1024::SIGNATURE_CT_SIZE 
            : Falcon512::SIGNATURE_CT_SIZE;
    }

    /***************************************************************************
     * Authentication Message Sizes
     * 
     * Format: [version(1)] [pubkey(var)] [challenge(var)] [signature(var)] [timestamp(8)]
     **************************************************************************/

    /** GetAuthPlaintextSize
     *
     *  Get authentication plaintext size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *  @param[in] challengeSize Size of challenge in bytes
     *
     *  @return Plaintext size in bytes
     *
     **/
    inline size_t GetAuthPlaintextSize(FalconVersion version, size_t challengeSize)
    {
        return VERSION_FIELD_SIZE + 
               GetPublicKeySize(version) + 
               challengeSize + 
               GetSignatureCTSize(version) + 
               TIMESTAMP_SIZE;
    }

    /** GetAuthEncryptedSize
     *
     *  Get authentication encrypted message size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *  @param[in] challengeSize Size of challenge in bytes
     *
     *  @return Encrypted size in bytes (plaintext + tag)
     *
     **/
    inline size_t GetAuthEncryptedSize(FalconVersion version, size_t challengeSize)
    {
        return GetAuthPlaintextSize(version, challengeSize) + CHACHA20_AUTH_TAG_SIZE;
    }

    /***************************************************************************
     * Submit Block Message Sizes
     * 
     * Format: [header(var)] [nonce(8)] [timestamp(8)] [signature(var)]
     **************************************************************************/

    /** GetSubmitBlockPlaintextSize
     *
     *  Get submit block plaintext size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *  @param[in] blockHeaderSize Size of block header in bytes
     *
     *  @return Plaintext size in bytes
     *
     **/
    inline size_t GetSubmitBlockPlaintextSize(FalconVersion version, size_t blockHeaderSize)
    {
        return blockHeaderSize + 
               8 +  // nonce
               TIMESTAMP_SIZE + 
               LENGTH_FIELD_SIZE +
               GetSignatureCTSize(version);
    }

    /** GetSubmitBlockEncryptedSize
     *
     *  Get submit block encrypted message size for a given Falcon version.
     *
     *  @param[in] version Falcon version
     *  @param[in] blockHeaderSize Size of block header in bytes
     *
     *  @return Encrypted size in bytes (plaintext + tag)
     *
     **/
    inline size_t GetSubmitBlockEncryptedSize(FalconVersion version, size_t blockHeaderSize)
    {
        return GetSubmitBlockPlaintextSize(version, blockHeaderSize) + CHACHA20_AUTH_TAG_SIZE;
    }

    /***************************************************************************
     * Maximum Buffer Sizes
     * 
     * Use Falcon-1024 (largest variant) for maximum buffer allocation.
     **************************************************************************/

    /** Maximum authentication plaintext size (with 64-byte challenge) */
    constexpr size_t MAX_AUTH_PLAINTEXT_SIZE = 
        VERSION_FIELD_SIZE + 
        Falcon1024::PUBLIC_KEY_SIZE + 
        64 +  // Max challenge size
        Falcon1024::SIGNATURE_CT_SIZE + 
        TIMESTAMP_SIZE;

    /** Maximum authentication encrypted size */
    constexpr size_t MAX_AUTH_ENCRYPTED_SIZE = MAX_AUTH_PLAINTEXT_SIZE + CHACHA20_AUTH_TAG_SIZE;

    /** Maximum submit block plaintext size (2MB block) */
    constexpr size_t MAX_SUBMIT_BLOCK_PLAINTEXT_SIZE = 
        (2 * 1024 * 1024) +  // Max block size
        8 +  // nonce
        TIMESTAMP_SIZE + 
        LENGTH_FIELD_SIZE +
        Falcon1024::SIGNATURE_CT_SIZE;

    /** Maximum submit block encrypted size */
    constexpr size_t MAX_SUBMIT_BLOCK_ENCRYPTED_SIZE = 
        MAX_SUBMIT_BLOCK_PLAINTEXT_SIZE + CHACHA20_AUTH_TAG_SIZE;

} // namespace FalconConstants
} // namespace LLC

#endif
