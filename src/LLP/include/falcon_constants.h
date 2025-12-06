/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_FALCON_CONSTANTS_H
#define NEXUS_LLP_INCLUDE_FALCON_CONSTANTS_H

#include <cstddef>
#include <cstdint>

namespace LLP
{
namespace FalconConstants
{
    /***************************************************************************
     * Falcon-512 Key Sizes (Fixed per Falcon Specification)
     **************************************************************************/
    
    /** Falcon-512 public key size (fixed) */
    static const size_t FALCON512_PUBKEY_SIZE = 897;
    
    /** Falcon-512 private key size (fixed) */
    static const size_t FALCON512_PRIVKEY_SIZE = 1281;

    /***************************************************************************
     * Falcon-512 Signature Sizes (Variable due to compression algorithm)
     **************************************************************************/
    
    /** Minimum Falcon-512 signature size (typical lower bound) */
    static const size_t FALCON512_SIG_MIN = 600;
    
    /** Typical maximum for authentication signatures (address + timestamp) */
    static const size_t FALCON512_SIG_AUTH_MAX = 700;
    
    /** Absolute maximum Falcon-512 signature size
     *  Per Falcon spec: FALCON_SIG_VARTIME_MAXSIZE(logn=9) = 752 bytes
     *  This covers ALL possible Falcon-512 signatures regardless of message */
    static const size_t FALCON512_SIG_ABSOLUTE_MAX = 752;
    
    /** Maximum signature size for validation (with safety margin) */
    static const size_t FALCON512_SIG_MAX_VALIDATION = 2048;

    /***************************************************************************
     * ChaCha20-Poly1305 AEAD Encryption Constants
     **************************************************************************/
    
    /** ChaCha20-Poly1305 nonce size (prepended to ciphertext) */
    static const size_t CHACHA20_NONCE_SIZE = 12;
    
    /** ChaCha20-Poly1305 authentication tag size (appended to ciphertext) */
    static const size_t CHACHA20_AUTH_TAG_SIZE = 16;
    
    /** Total ChaCha20-Poly1305 overhead (nonce + auth tag) */
    static const size_t CHACHA20_OVERHEAD = 28;  // 12 + 16

    /***************************************************************************
     * Protocol Field Sizes
     **************************************************************************/
    
    /** Merkle root size (uint512_t) */
    static const size_t MERKLE_ROOT_SIZE = 64;
    
    /** Nonce size (uint64_t, little-endian) */
    static const size_t NONCE_SIZE = 8;
    
    /** Timestamp size (uint64_t, little-endian) */
    static const size_t TIMESTAMP_SIZE = 8;
    
    /** Length field size for variable-length fields (2 bytes, little-endian) */
    static const size_t LENGTH_FIELD_SIZE = 2;
    
    /** Tritium GenesisHash size (uint256_t) */
    static const size_t GENESIS_HASH_SIZE = 32;

    /***************************************************************************
     * Submit Block Message (What Gets Signed - Fixed Size)
     **************************************************************************/
    
    /** Size of the message signed in Submit Block wrapper
     *  merkle_root(64) + nonce(8) + timestamp(8) = 80 bytes
     *  NOTE: This is FIXED regardless of actual block size */
    static const size_t SUBMIT_BLOCK_MESSAGE_SIZE = 80;  // 64 + 8 + 8

    /***************************************************************************
     * Submit Block Wrapper Sizes (Serialized Transmission)
     **************************************************************************/
    
    /** Minimum Submit Block wrapper size (without signature)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) = 82 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MIN = 82;
    
    /** Submit Block wrapper - LOCALHOST (no encryption)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) + sig(752) = 834 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MAX = 834;
    
    /** Submit Block wrapper - PUBLIC MINER (with ChaCha20 encryption)
     *  nonce(12) + encrypted_payload(834) + auth_tag(16) = 862 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX = 862;

    /***************************************************************************
     * Authentication Response Sizes
     **************************************************************************/
    
    /** Auth response - LOCALHOST (no encryption on pubkey)
     *  pubkey_len(2) + pubkey(897) + timestamp(8) + sig_len(2) + sig(752) = 1661 bytes */
    static const size_t AUTH_RESPONSE_MAX = 1661;
    
    /** Auth response - PUBLIC MINER (ChaCha20 wrapped pubkey)
     *  pubkey_len(2) + wrapped_pubkey(897+28) + timestamp(8) + sig_len(2) + sig(752) = 1689 bytes */
    static const size_t AUTH_RESPONSE_ENCRYPTED_MAX = 1689;
    
    /** Auth response with optional GenesisHash binding
     *  Add 32 bytes for Tritium genesis hash */
    static const size_t AUTH_RESPONSE_WITH_GENESIS_MAX = 1721;

} // namespace FalconConstants
} // namespace LLP

#endif
