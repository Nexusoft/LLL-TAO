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
     * DISPOSABLE FALCON ARCHITECTURE
     * 
     * The Nexus mining protocol uses Disposable Falcon signatures:
     * 
     * Disposable Falcon Wrapper - Signs fixed 80-byte message 
     *    (merkle + nonce + timestamp) for session authentication.
     *    NOT stored on blockchain. Always enabled for authenticated sessions.
     **************************************************************************/

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
    
    /** Typical maximum for VARIABLE-TIME signatures (not used by LLL-TAO) */
    static const size_t FALCON512_SIG_VARTIME_MAX = 752;
    
    /** Constant-Time Falcon-512 signature size (exact)
     *  Per Falcon spec: FALCON_SIG_CT_SIZE(logn=9) = 809 bytes
     *  LLL-TAO's FLKey::Sign() uses ct=1, producing exactly 809 bytes */
    static const size_t FALCON512_SIG_CT_SIZE = 809;
    
    /** Absolute maximum Falcon-512 signature size
     *  This is the CT size since LLL-TAO uses constant-time signing */
    static const size_t FALCON512_SIG_ABSOLUTE_MAX = 809;
    
    /** Maximum signature size for validation (with safety margin) */
    static const size_t FALCON512_SIG_MAX_VALIDATION = 2048;
    
    /** Common Falcon-512 signature sizes for dynamic block size detection
     *  When block size is unknown, we try these common signature sizes to
     *  work backwards from the packet structure. These represent typical
     *  signature sizes observed in practice, ordered by frequency. */
    static const size_t FALCON512_SIG_COMMON_SIZE_1 = 809;  // CT signature (most common)
    static const size_t FALCON512_SIG_COMMON_SIZE_2 = 800;  // Near-CT compression
    static const size_t FALCON512_SIG_COMMON_SIZE_3 = 750;  // Good compression
    static const size_t FALCON512_SIG_COMMON_SIZE_4 = 700;  // Better compression
    static const size_t FALCON512_SIG_COMMON_SIZE_5 = 666;  // Best compression (minimum)

    /***************************************************************************
     * Falcon-1024 Key Sizes (Fixed per Falcon Specification)
     **************************************************************************/
    
    /** Falcon-1024 public key size (fixed) */
    static const size_t FALCON1024_PUBKEY_SIZE = 1793;
    
    /** Falcon-1024 private key size (fixed) */
    static const size_t FALCON1024_PRIVKEY_SIZE = 2305;

    /***************************************************************************
     * Falcon-1024 Signature Sizes (Variable due to compression algorithm)
     **************************************************************************/
    
    /** Minimum Falcon-1024 signature size (typical lower bound) */
    static const size_t FALCON1024_SIG_MIN = 1280;
    
    /** Typical maximum for authentication signatures (address + timestamp) */
    static const size_t FALCON1024_SIG_AUTH_MAX = 1400;
    
    /** Constant-Time Falcon-1024 signature size (exact)
     *  Per Falcon spec: FALCON_SIG_CT_SIZE(logn=10) = 1577 bytes
     *  LLL-TAO's FLKey::Sign() uses ct=1, producing exactly 1577 bytes */
    static const size_t FALCON1024_SIG_CT_SIZE = 1577;
    
    /** Absolute maximum Falcon-1024 signature size
     *  This is the CT size since LLL-TAO uses constant-time signing */
    static const size_t FALCON1024_SIG_ABSOLUTE_MAX = 1577;
    
    /** Maximum signature size for validation (with safety margin) */
    static const size_t FALCON1024_SIG_MAX_VALIDATION = 2048;
    
    /** Common Falcon-1024 signature sizes for dynamic block size detection
     *  When block size is unknown, we try these common signature sizes to
     *  work backwards from the packet structure. These represent typical
     *  signature sizes observed in practice, ordered by frequency. */
    static const size_t FALCON1024_SIG_COMMON_SIZE_1 = 1577;  // CT signature (most common)
    static const size_t FALCON1024_SIG_COMMON_SIZE_2 = 1550;  // Near-CT compression
    static const size_t FALCON1024_SIG_COMMON_SIZE_3 = 1500;  // Good compression
    static const size_t FALCON1024_SIG_COMMON_SIZE_4 = 1400;  // Better compression
    static const size_t FALCON1024_SIG_COMMON_SIZE_5 = 1280;  // Best compression (minimum)

    /***************************************************************************
     * Version-Agnostic Aliases for Max/Min Constants
     * 
     * These aliases use the maximum values from both Falcon-512 and Falcon-1024
     * to ensure the node can accept packets from miners using either variant.
     * Logic gates should use these aliases instead of version-specific constants.
     **************************************************************************/

    /** Minimum signature size across all Falcon variants (use smallest) */
    static const size_t FALCON_SIG_MIN = FALCON512_SIG_MIN;  // 600 bytes (Falcon-512 has smaller min)

    /** Maximum signature size for validation across all Falcon variants (use largest) */
    static const size_t FALCON_SIG_MAX_VALIDATION = FALCON1024_SIG_MAX_VALIDATION;  // 2048 bytes (both use same)

    /** Absolute maximum signature size across all Falcon variants (use largest) */
    static const size_t FALCON_SIG_ABSOLUTE_MAX = FALCON1024_SIG_ABSOLUTE_MAX;  // 1577 bytes (Falcon-1024 CT)

    /** Constant-Time signature size for maximum variant (use largest) */
    static const size_t FALCON_SIG_CT_MAX = FALCON1024_SIG_CT_SIZE;  // 1577 bytes (Falcon-1024 CT)

    /** Constant-Time signature size for minimum variant (use smallest) */
    static const size_t FALCON_SIG_CT_MIN = FALCON512_SIG_CT_SIZE;  // 809 bytes (Falcon-512 CT)

    /***************************************************************************
     * Public Key Size Helper Functions
     **************************************************************************/

    /** is_valid_pubkey_size
     *
     *  Returns true if the given size is a valid Falcon public key size
     *  for either Falcon-512 (897 bytes) or Falcon-1024 (1793 bytes).
     *
     *  @param[in] nSize  Size of the public key in bytes
     *  @return true if valid for any supported Falcon variant
     *
     **/
    static inline bool is_valid_pubkey_size(size_t nSize)
    {
        return (nSize == FALCON512_PUBKEY_SIZE || nSize == FALCON1024_PUBKEY_SIZE);
    }

    /** get_falcon_version_from_pubkey_size
     *
     *  Infer the Falcon variant from a public key size.
     *  Returns 512 for Falcon-512, 1024 for Falcon-1024, 0 if unknown.
     *
     *  @param[in] nSize  Size of the public key in bytes
     *  @return 512, 1024, or 0 (unknown)
     *
     **/
    static inline int get_falcon_version_from_pubkey_size(size_t nSize)
    {
        if(nSize == FALCON512_PUBKEY_SIZE)  return 512;
        if(nSize == FALCON1024_PUBKEY_SIZE) return 1024;
        return 0;
    }

    /** is_valid_sig_size
     *
     *  Returns true if the given size is within the valid range for a
     *  Falcon signature (either variant).  Uses the version-agnostic
     *  FALCON_SIG_MIN and FALCON_SIG_MAX_VALIDATION constants.
     *
     *  @param[in] nSize  Size of the signature in bytes
     *  @return true if plausibly valid
     *
     **/
    static inline bool is_valid_sig_size(size_t nSize)
    {
        return (nSize >= FALCON_SIG_MIN && nSize <= FALCON_SIG_MAX_VALIDATION);
    }

    /***************************************************************************
     * ChaCha20-Poly1305 AEAD Encryption Constants
     **************************************************************************/
    
    /** ChaCha20-Poly1305 nonce size (prepended to ciphertext) */
    static const size_t CHACHA20_NONCE_SIZE = 12;
    
    /** ChaCha20-Poly1305 authentication tag size (appended to ciphertext) */
    static const size_t CHACHA20_AUTH_TAG_SIZE = 16;
    
    /** Total ChaCha20-Poly1305 overhead (nonce + auth tag) */
    static const size_t CHACHA20_OVERHEAD = CHACHA20_NONCE_SIZE + CHACHA20_AUTH_TAG_SIZE;

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
     * Full Block Sizes (NexusMiner Full Block Format)
     * 
     * Note: In the stateless mining protocol, miners submit block HEADERS only.
     * Transactions are managed exclusively by the node and are never sent by
     * the miner to the node.  The "full block" here refers to the serialized
     * block header fields (nVersion, hashPrevBlock, hashMerkleRoot, channel,
     * height, bits, nNonce) without any transaction data.
     *
     * Previous values were for empty blocks only (216 bytes for Tritium, 220
     * for Legacy).  The 2MB figure that appeared in earlier comments was an
     * incorrect beta assumption that has been removed.
     **************************************************************************/
    
    /** Full Tritium block header size (serialized, without transactions)
     *  Fields: nVersion(4) + hashPrevBlock(128) + hashMerkleRoot(64) +
     *          nChannel(4) + nHeight(4) + nBits(4) + nNonce(8) = 216 bytes */
    static const size_t FULL_BLOCK_TRITIUM_SIZE = 216;
    
    /** Minimum Tritium block size (empty block, coinbase only) */
    static const size_t FULL_BLOCK_TRITIUM_MIN = 216;
    
    /** Full Legacy block header size (serialized, without transactions)
     *  Fields: nVersion(4) + hashPrevBlock(128) + hashMerkleRoot(64) +
     *          nTime(4) + nBits(4) + nNonce(4) + nChannel(4) + nHeight(4) +
     *          nNonce64(8) = 220 bytes */
    static const size_t FULL_BLOCK_LEGACY_SIZE = 220;
    
    /** Minimum Legacy block size (empty block, coinbase only) */
    static const size_t FULL_BLOCK_LEGACY_MIN = 220;
    
    /** Merkle root offset in full block
     *  Located after: nVersion(4) + hashPrevBlock(128) = 132 bytes */
    static const size_t FULL_BLOCK_MERKLE_OFFSET = 132;

    /** Channel offset in serialized Tritium full-block bodies. */
    static const size_t FULL_BLOCK_TRITIUM_CHANNEL_OFFSET = 196;

    /** Height offset in serialized Tritium full-block bodies. */
    static const size_t FULL_BLOCK_TRITIUM_HEIGHT_OFFSET = 200;
    
    /** Nonce offset in Tritium full block
     *  Confirmed via diagnostic data at offset 200 */
    static const size_t FULL_BLOCK_TRITIUM_NONCE_OFFSET = 200;
    
    /** Nonce offset in Legacy full block
     *  Confirmed via diagnostic data at offset 204 */
    static const size_t FULL_BLOCK_LEGACY_NONCE_OFFSET = 204;
    
    /** Format detection threshold for SUBMIT_BLOCK packets
     *  Packets >= 200 bytes are treated as full block format,
     *  packets < 200 bytes are treated as legacy 64-byte merkle format.
     *  This threshold safely distinguishes between:
     *  - Legacy format max: ~919 bytes (64-byte merkle + nonce + signatures + encryption)
     *  - Full block min: 216 bytes (Tritium block without signature) */
    static const size_t SUBMIT_BLOCK_FORMAT_DETECTION_THRESHOLD = 200;
    
    /** Size margin for distinguishing Tritium vs Legacy full blocks
     *  Used to account for variable-length signatures when determining block type.
     *  Tritium blocks start at 216 bytes, Legacy at 220 bytes. */
    static const size_t FULL_BLOCK_TYPE_DETECTION_MARGIN = 100;

    /***************************************************************************
     * Submit Block Message (What Gets Signed - Fixed Size)
     **************************************************************************/
    
    /** Size of the message signed in Submit Block wrapper
     *  merkle_root(64) + nonce(8) + timestamp(8) = 80 bytes
     *  NOTE: This is FIXED regardless of actual block size */
    static const size_t SUBMIT_BLOCK_MESSAGE_SIZE = 80;  // 64 + 8 + 8

     /***************************************************************************
     * Submit Block Wrapper Sizes (Serialized Transmission)
     * 
     * In the stateless mining protocol, miners submit block HEADERS only.
     * The node holds all transactions; the miner never sends them.
     * The packet format is now identical for Prime and Hash channels:
     *   [block_header(var)] [timestamp(8)] [siglen(2)] [signature(var)]
     * Miners no longer transmit vOffsets on the wire.  The node always
     * derives vOffsets via GetOffsets(GetPrime(), vOffsets) after setting nNonce.
     **************************************************************************/
    
    /** Minimum Submit Block wrapper size (without signature)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) = 82 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MIN = 82;

    /** Prime offsets budget retained for backwards-compatibility sizing calculations.
     *  Miners no longer send vOffsets on the wire; this constant is kept so that
     *  SUBMIT_BLOCK_WRAPPER_MAX retains its original generous upper bound. */
    static const size_t SUBMIT_BLOCK_PRIME_OFFSETS_MAX = 256;
    
    /** Submit Block wrapper maximum (no encryption)
     *  Format: [block_header(max=220)] [timestamp(8)] [siglen(2)] [sig(max=1577)]
     *  Note: Miners submit block HEADERS only; transactions are NOT included.
     *  Note: vOffsets are no longer on the wire; SUBMIT_BLOCK_PRIME_OFFSETS_MAX
     *        is retained in this sum for a conservative upper bound.
     *  Max: FULL_BLOCK_LEGACY_SIZE(220) + SUBMIT_BLOCK_PRIME_OFFSETS_MAX(256)
     *       + TIMESTAMP_SIZE(8) + LENGTH_FIELD_SIZE(2) + FALCON1024_SIG_ABSOLUTE_MAX(1577) = 2063 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MAX =
        FULL_BLOCK_LEGACY_SIZE +          // 220 bytes (max block header, no transactions)
        SUBMIT_BLOCK_PRIME_OFFSETS_MAX +   // 256 bytes (conservative budget, no longer on wire)
        TIMESTAMP_SIZE +                   // 8 bytes
        LENGTH_FIELD_SIZE +                // 2 bytes
        FALCON1024_SIG_ABSOLUTE_MAX;       // 1577 bytes (Falcon-1024 CT, default)
    // = 2063 bytes
    
    /** Submit Block wrapper maximum (with ChaCha20-Poly1305 encryption)
     *  Adds ChaCha20-Poly1305 overhead: nonce(12) + auth_tag(16) = 28 bytes
     *  2063 + 28 = 2091 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX =
        SUBMIT_BLOCK_WRAPPER_MAX + CHACHA20_OVERHEAD;
    // = 2091 bytes


    /***************************************************************************
     * Full Block Format - Detailed Size Constants (For Reference)
     * 
     * These constants provide granular details for different block types and
     * signature scenarios in the full block format.
     **************************************************************************/

    /** Hash-channel Tritium wrapper signature (Falcon-512) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(809)]
     *  Calculation: 216 + 8 + 2 + 809 = 1035 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_MAX = 1035;

    /** Hash-channel Tritium wrapper signature (Falcon-512) - encrypted
     *  1035 + 28 = 1063 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_ENCRYPTED_MAX = 1063;

    /** Deprecated compatibility alias for the fixed-size Hash / zero-offset Tritium wrapper.
     *  New code should prefer the explicit HASH constant names above. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_MAX;

    /** Deprecated compatibility alias for the fixed-size encrypted Hash / zero-offset Tritium wrapper.
     *  New code should prefer the explicit HASH constant names above. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_ENCRYPTED_MAX;

    /** Prime-channel Tritium wrapper signature (Falcon-512) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(809)]  (same as Hash; no vOffsets on wire)
     *  Miners derive vOffsets on the node side via GetOffsets(GetPrime(), vOffsets). */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_PRIME_WRAPPER_MIN = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_MAX;

    /** Prime-channel Tritium wrapper signature (Falcon-512) - encrypted
     *  Same as Hash-channel encrypted size; no vOffsets on wire. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_PRIME_WRAPPER_ENCRYPTED_MIN = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_ENCRYPTED_MAX;

    /** Legacy wrapper signature (Falcon-512) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(809)]
     *  Calculation: 220 + 8 + 2 + 809 = 1039 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX = 1039;

    /** Legacy wrapper signature (Falcon-512) - encrypted
     *  1039 + 28 = 1067 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_ENCRYPTED_MAX = 1067;

    /***************************************************************************
     * Falcon-1024 Packet Size Constants
     * 
     * These constants define packet sizes when using Falcon-1024 signatures.
     * Falcon-1024 uses 1577-byte signatures vs Falcon-512's 809 bytes.
     **************************************************************************/

    /** Hash-channel Tritium wrapper signature (Falcon-1024) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)]
     *  Calculation: 216 + 8 + 2 + 1577 = 1803 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_MAX = 1803;

    /** Hash-channel Tritium wrapper signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 1803 + 28 = 1831 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_ENCRYPTED_MAX = 1831;

    /** Deprecated compatibility alias for the fixed-size Hash / zero-offset Tritium Falcon-1024 wrapper.
     *  New code should prefer the explicit HASH constant names above. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_FALCON1024_MAX = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_MAX;

    /** Deprecated compatibility alias for the fixed-size encrypted Hash / zero-offset Tritium Falcon-1024 wrapper.
     *  New code should prefer the explicit HASH constant names above. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_FALCON1024_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_ENCRYPTED_MAX;

    /** Prime-channel Tritium wrapper signature (Falcon-1024) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)]  (same as Hash; no vOffsets on wire)
     *  Miners derive vOffsets on the node side via GetOffsets(GetPrime(), vOffsets). */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_PRIME_WRAPPER_FALCON1024_MIN = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_MAX;

    /** Prime-channel Tritium wrapper signature (Falcon-1024) - encrypted
     *  Same as Hash-channel encrypted size; no vOffsets on wire. */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_PRIME_WRAPPER_FALCON1024_ENCRYPTED_MIN = SUBMIT_BLOCK_FULL_TRITIUM_HASH_WRAPPER_FALCON1024_ENCRYPTED_MAX;

    /** Legacy wrapper signature (Falcon-1024) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(1577)]
     *  Calculation: 220 + 8 + 2 + 1577 = 1807 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_FALCON1024_MAX = 1807;

    /** Legacy wrapper signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 1807 + 28 = 1835 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_FALCON1024_ENCRYPTED_MAX = 1835;

    /***************************************************************************
     * GET_BLOCK / BLOCK_DATA Response Sizes (Node \u2192 Miner)
     **************************************************************************/

    /** Minimum BLOCK_DATA response size (Tritium block) */
    static const size_t BLOCK_DATA_RESPONSE_MIN = 216;

    /** Maximum BLOCK_DATA response size (Legacy block) */
    static const size_t BLOCK_DATA_RESPONSE_MAX = 220;


    /***************************************************************************
     * Authentication Response Sizes - Falcon-512
     **************************************************************************/
    
    /** Auth response - LOCALHOST - Falcon-512
     *  pubkey_len(2) + pubkey(897) + timestamp(8) + sig_len(2) + sig(809) = 1718 bytes */
    static const size_t AUTH_RESPONSE_FALCON512_MAX = LENGTH_FIELD_SIZE + FALCON512_PUBKEY_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX;  // 1718 bytes
    
    /** Auth response - PUBLIC MINER (ChaCha20 wrapped) - Falcon-512
     *  1718 + 28 = 1746 bytes */
    static const size_t AUTH_RESPONSE_FALCON512_ENCRYPTED_MAX = AUTH_RESPONSE_FALCON512_MAX + CHACHA20_OVERHEAD;  // 1746 bytes

    /***************************************************************************
     * Authentication Response Sizes - Falcon-1024 (DEFAULT)
     **************************************************************************/

    /** Auth response - LOCALHOST - Falcon-1024
     *  pubkey_len(2) + pubkey(1793) + timestamp(8) + sig_len(2) + sig(1577) = 3382 bytes */
    static const size_t AUTH_RESPONSE_FALCON1024_MAX = 
        LENGTH_FIELD_SIZE + FALCON1024_PUBKEY_SIZE + TIMESTAMP_SIZE + 
        LENGTH_FIELD_SIZE + FALCON1024_SIG_ABSOLUTE_MAX;  // 3382 bytes

    /** Auth response - PUBLIC MINER (ChaCha20 wrapped) - Falcon-1024
     *  3382 + 28 = 3410 bytes */
    static const size_t AUTH_RESPONSE_FALCON1024_ENCRYPTED_MAX = 
        AUTH_RESPONSE_FALCON1024_MAX + CHACHA20_OVERHEAD;  // 3410 bytes

    /***************************************************************************
     * Version-Agnostic Authentication Response (use largest for buffers)
     **************************************************************************/
    
    /** Auth response - Use Falcon-1024 size for buffer allocation */
    static const size_t AUTH_RESPONSE_MAX = AUTH_RESPONSE_FALCON1024_MAX;  // 3382 bytes
    
    /** Auth response - PUBLIC MINER (encrypted) - Use Falcon-1024 size */
    static const size_t AUTH_RESPONSE_ENCRYPTED_MAX = AUTH_RESPONSE_FALCON1024_ENCRYPTED_MAX;  // 3410 bytes
    
    /** Auth response with optional GenesisHash binding
     *  Add 32 bytes for Tritium genesis hash */
    static const size_t AUTH_RESPONSE_WITH_GENESIS_MAX = AUTH_RESPONSE_ENCRYPTED_MAX + GENESIS_HASH_SIZE;  // 3442 bytes

    /***************************************************************************
     * MINER_AUTH_INIT Sizes - Falcon-512
     **************************************************************************/

    /** MINER_AUTH_INIT minimum size (no miner_id, no genesis) - Falcon-512
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2) = 901 bytes */
    static const size_t MINER_AUTH_INIT_FALCON512_MIN = 901;

    /** MINER_AUTH_INIT with genesis but empty miner_id string - Falcon-512
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2, value=0) + genesis(32) = 933 bytes */
    static const size_t MINER_AUTH_INIT_FALCON512_WITH_GENESIS = 933;

    /** MINER_AUTH_INIT maximum size (with miner_id and genesis) - Falcon-512
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2) + miner_id(256) + genesis(32) = 1189 bytes */
    static const size_t MINER_AUTH_INIT_FALCON512_MAX = 1189;

    /***************************************************************************
     * MINER_AUTH_INIT Sizes - Falcon-1024 (DEFAULT)
     **************************************************************************/

    /** MINER_AUTH_INIT minimum - Falcon-1024
     *  pubkey_len(2) + pubkey(1793) + miner_id_len(2) = 1797 bytes */
    static const size_t MINER_AUTH_INIT_FALCON1024_MIN = 1797;

    /** MINER_AUTH_INIT with genesis but empty miner_id string - Falcon-1024
     *  pubkey_len(2) + pubkey(1793) + miner_id_len(2, value=0) + genesis(32) = 1829 bytes */
    static const size_t MINER_AUTH_INIT_FALCON1024_WITH_GENESIS = 1829;

    /** MINER_AUTH_INIT maximum - Falcon-1024
     *  pubkey_len(2) + pubkey(1793) + miner_id_len(2) + miner_id(256) + genesis(32) = 2085 bytes */
    static const size_t MINER_AUTH_INIT_FALCON1024_MAX = 2085;

    /***************************************************************************
     * Version-Agnostic MINER_AUTH_INIT (use largest for buffer allocation)
     **************************************************************************/

    /** MINER_AUTH_INIT minimum size - Use Falcon-512 (smallest) */
    static const size_t MINER_AUTH_INIT_MIN = MINER_AUTH_INIT_FALCON512_MIN;  // 901 bytes

    /** MINER_AUTH_INIT with genesis - Use Falcon-512 (smallest) */
    static const size_t MINER_AUTH_INIT_WITH_GENESIS = MINER_AUTH_INIT_FALCON512_WITH_GENESIS;  // 933 bytes

    /** MINER_AUTH_INIT maximum size - Use Falcon-1024 (largest) for buffer allocation */
    static const size_t MINER_AUTH_INIT_MAX = MINER_AUTH_INIT_FALCON1024_MAX;  // 2085 bytes

    /***************************************************************************
     * Pool Announcement Signature Sizes
     **************************************************************************/

    /** Pool announcement signature - Falcon-512 */
    static const size_t POOL_ANNOUNCEMENT_SIG_FALCON512_MAX = FALCON512_SIG_ABSOLUTE_MAX;  // 809 bytes

    /** Pool announcement signature - Falcon-1024 (DEFAULT) */
    static const size_t POOL_ANNOUNCEMENT_SIG_FALCON1024_MAX = FALCON1024_SIG_ABSOLUTE_MAX;  // 1577 bytes

    /** Pool announcement signature - Version agnostic (use largest) */
    static const size_t POOL_ANNOUNCEMENT_SIG_MAX = FALCON1024_SIG_ABSOLUTE_MAX;  // 1577 bytes

    /***************************************************************************
     * Version-Agnostic Defaults for New Code
     **************************************************************************/

    /** Default Falcon version flag (true = Falcon-1024, false = Falcon-512) */
    static const bool USE_FALCON1024_DEFAULT = true;

    /** Default public key size (Falcon-1024) */
    static const size_t FALCON_PUBKEY_DEFAULT = FALCON1024_PUBKEY_SIZE;  // 1793 bytes

    /** Default signature size (Falcon-1024 CT) */
    static const size_t FALCON_SIG_CT_DEFAULT = FALCON1024_SIG_CT_SIZE;  // 1577 bytes

    /***************************************************************************
     * Mining Template Validity Constants
     * 
     * NOTE: These constants are defined for future implementation of 
     * timestamp-based template staleness checking. Currently, template 
     * validation is height-based only (templates are invalidated when 
     * blockchain height changes). Time-based validation can be added in 
     * future enhancements to provide additional staleness protection.
     **************************************************************************/

    /** Template timestamp tolerance in seconds
     *  Allow clock skew between node and miner to prevent false rejections.
     *  Example: Don't reject templates if miner's clock is 20 seconds behind node.
     *  This is NOT a warning threshold - it's a tolerance for clock synchronization. */
    static const uint64_t TEMPLATE_TIMESTAMP_TOLERANCE_SECONDS = 30;

    /** Maximum age of a mining template in seconds
     *  Hard cutoff for template validity - templates older than this are discarded.
     *  Reasoning:
     *    - Nexus Prime channel can take several minutes per solution (5-7 chain primes)
     *    - Template must remain valid long enough for miners to complete a solution
     *    - 600 seconds (10 minutes) provides adequate margin for Prime channel mining
     *    - Height-based staleness (IsStale()) is the primary freshness check;
     *      this is a safety net for edge cases (reorgs, network partitions, clock skew)
     *  Prevents miners from working on very stale templates. */
    static const uint64_t MAX_TEMPLATE_AGE_SECONDS = 600;

    /** Heartbeat template refresh threshold in seconds.
     *
     *  When no new block push notification has been sent for this many seconds, the
     *  node proactively re-pushes the current template to all subscribed miners.
     *  This prevents miners from entering degraded mode during legitimate dry spells,
     *  for example on the Prime channel which can routinely take 2–5+ minutes between
     *  solutions (5–7 chain primes).
     *
     *  Set to 480 s (8 min) — 120 s before the hard MAX_TEMPLATE_AGE_SECONDS (600 s)
     *  cutoff — so miners always receive a refreshed template well before their
     *  template-age timer expires. */
    static const uint64_t TEMPLATE_HEARTBEAT_REFRESH_SECONDS = 480;

} // namespace FalconConstants
} // namespace LLP

#endif
