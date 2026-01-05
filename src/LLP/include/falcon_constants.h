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
     * DUAL SIGNATURE ARCHITECTURE
     * 
     * The Nexus mining protocol uses two types of Falcon signatures:
     * 
     * 1. Disposable Falcon Wrapper - Signs fixed 80-byte message 
     *    (merkle + nonce + timestamp) for session authentication.
     *    NOT stored on blockchain. Always enabled for authenticated sessions.
     * 
     * 2. Physical Block Signature - Signs full block data + nonce for 
     *    permanent proof of authorship. STORED on blockchain.
     *    Enabled via NexusMiner config: "enable_block_signing": true
     *    This is the emergency backup system.
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
     * Full Block Sizes (NexusMiner PR #65/#66 Full Block Format)
     * 
     * Note: These values are based on diagnostic data from actual miner submissions.
     * The offsets below were confirmed through real-world testing of NexusMiner
     * PR #65 and PR #66 which send full serialized blocks instead of compact merkle roots.
     * 
     * UPDATED: Now supports blocks with transactions (up to 2MB)
     * Previous values were for empty blocks only (216 bytes for Tritium, 220 for Legacy)
     * Current values support maximum network block size with transactions (2MB)
     **************************************************************************/
    
    /** Full Tritium block size (without signature or timestamp)
     *  UPDATED: Now supports blocks with transactions (up to 2MB)
     *  Previous: 216 bytes (empty Tritium block)
     *  Current: 2MB (maximum network block size with transactions)
     *  
     *  NOTE: Block size varies:
     *  - Empty block (coinbase only): 216 bytes
     *  - Block with transactions: up to 2MB */
    static const size_t FULL_BLOCK_TRITIUM_SIZE = 2 * 1024 * 1024;  // Was: 216
    
    /** Minimum Tritium block size (empty block, coinbase only) */
    static const size_t FULL_BLOCK_TRITIUM_MIN = 216;
    
    /** Full Legacy block size (without signature or timestamp)
     *  UPDATED: Now supports blocks with transactions (up to 2MB)
     *  Previous: 220 bytes (empty Legacy block)
     *  Current: 2MB (maximum network block size with transactions) */
    static const size_t FULL_BLOCK_LEGACY_SIZE = 2 * 1024 * 1024;  // Was: 220
    
    /** Minimum Legacy block size (empty block, coinbase only) */
    static const size_t FULL_BLOCK_LEGACY_MIN = 220;
    
    /** Merkle root offset in full block
     *  Located after: nVersion(4) + hashPrevBlock(128) = 132 bytes */
    static const size_t FULL_BLOCK_MERKLE_OFFSET = 132;
    
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
     * Submit Block Wrapper Sizes (Serialized Transmission) - CT=809
     * 
     * UPDATED for full block format (NexusMiner PR #65/#66)
     **************************************************************************/
    
    /** Minimum Submit Block wrapper size (without signature)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) = 82 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MIN = 82;
    
    /** Submit Block wrapper - LOCALHOST (no encryption)
     *  
     *  Old value (merkle format): 891 bytes
     *    merkle(64) + nonce(8) + timestamp(8) + sig_len(2) + sig(809) = 891
     *  
     *  Previous value (full block format, no Physical Falcon field): 1,035 bytes
     *    block(216) + timestamp(8) + sig_len(2) + sig(809) = 1,035
     *  
     *  Current value (PR #122 - Physical Falcon support): 1,037 bytes (minimum with Falcon-512)
     *    block(216) + timestamp(8) + sig_len(2) + sig(809) + physiglen(2) = 1,037
     *    Note: Physical Falcon length field is ALWAYS present (2 bytes), even when disabled (value=0)
     *  
     *  UPDATED: Now supports 2MB blocks with transactions AND Falcon-1024
     *  Format: [block(2MB max)][timestamp(8)][sig_len(2)][signature(1577 max)][physiglen(2)][physical_sig(optional)]
     *  Calculation: 2,097,152 + 8 + 2 + 1577 + 2 = 2,098,741 bytes (Falcon-1024 without physical signature)
     *  Note: Using Falcon-1024 size as maximum since nodes must accept both Falcon-512 and Falcon-1024
     *  
     *  Updated in: NexusMiner PR #65/#66 (full block mining), PR #115 (2MB support), PR #122 (Physical Falcon), PR #122 (Falcon-1024)
     */
    static const size_t SUBMIT_BLOCK_WRAPPER_MAX = 2098741;  // Was 1035, then 2097971, then 2097973
    
    /** Submit Block wrapper - PUBLIC MINER (with ChaCha20 encryption)
     *  
     *  Old value (merkle format): 919 bytes
     *    nonce(12) + encrypted_payload(891) + auth_tag(16) = 919
     *  
     *  Previous value (full block format, no Physical Falcon field): 1,063 bytes
     *    nonce(12) + encrypted_payload(1,035) + auth_tag(16) = 1,063
     *  
     *  Current value (PR #122 - Physical Falcon support): 1,065 bytes (minimum with Falcon-512)
     *    nonce(12) + encrypted_payload(1,037) + auth_tag(16) = 1,065
     *    Note: Physical Falcon length field is ALWAYS present in encrypted payload
     *  
     *  UPDATED: Now supports 2MB blocks with transactions AND Falcon-1024
     *  Adds ChaCha20-Poly1305 overhead: nonce(12) + auth_tag(16) = 28 bytes
     *  Calculation: 2,098,741 + 28 = 2,098,769 bytes
     *  Note: Using Falcon-1024 size as maximum since nodes must accept both Falcon-512 and Falcon-1024
     *  
     *  Updated in: NexusMiner PR #65/#66 (full block mining), PR #115 (2MB support), PR #122 (Physical Falcon), PR #122 (Falcon-1024)
     */
    static const size_t SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX = 2098769;  // Was 1063, then 2097999, then 2098001

    /***************************************************************************
     * Physical Block Signature (Stored on Blockchain - Emergency Backup System) - CT=809
     **************************************************************************/
    
    /** Physical block signature - signs full block data + nonce
     *  This signature IS stored on the blockchain for permanent proof of authorship.
     *  Enabled via NexusMiner config: "enable_block_signing": true
     *  
     *  Unlike the Disposable Falcon wrapper (which signs a fixed 80-byte message),
     *  the Physical Block Signature signs the FULL block data which can be up to
     *  MAX_BLOCK_SIZE (2MB).
     *  
     *  Message format: [block_data (variable, up to MAX_BLOCK_SIZE)] + [nonce (8 bytes LE)]
     *  Signature: Falcon-512 (~600-809 bytes)
     *  
     *  This is the emergency backup system for proving block authorship when
     *  the Disposable Falcon session authentication is insufficient.
     */
    
    /** Minimum physical block signature size */
    static const size_t PHYSICAL_BLOCK_SIG_MIN = FALCON512_SIG_MIN;  // 600 bytes
    
    /** Maximum physical block signature size */
    static const size_t PHYSICAL_BLOCK_SIG_MAX = FALCON512_SIG_ABSOLUTE_MAX;  // 809 bytes
    
    /** Maximum message size for physical block signature
     *  block_data (up to 2MB) + nonce (8 bytes) 
     *  Note: References TAO::Ledger::MAX_BLOCK_SIZE from constants.h */
    static const size_t PHYSICAL_BLOCK_SIG_MESSAGE_MAX = (1024 * 1024 * 2) + NONCE_SIZE;  // 2MB + 8 bytes
    
    /** Physical block signature overhead added to block transmission
     *  sig_len(2) + signature(809) = 811 bytes max */
    static const size_t PHYSICAL_BLOCK_SIG_OVERHEAD = LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX;  // 811 bytes
    
    /** Minimum block submission size with physical signature overhead */
    static const size_t BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD = PHYSICAL_BLOCK_SIG_OVERHEAD;  // 811 bytes

    /***************************************************************************
     * Dual Signature Scenario (Both Disposable Wrapper + Physical Signature)
     * 
     * UPDATED for full block format (NexusMiner PR #65/#66) and Falcon-1024 support
     * Note: These use Falcon-1024 sizes as maximum to support both Falcon-512 and Falcon-1024
     **************************************************************************/
    
    /** Submit block with BOTH signatures (Falcon-512) - LOCALHOST (no encryption)
     *  
     *  Old value (merkle format): 1,702 bytes
     *    Disposable wrapper(891) + Physical signature overhead(811) = 1,702
     *  
     *  Current value (full block format, Falcon-512): 1,850 bytes (Legacy - largest)
     *    block(220) + timestamp(8) + sig_len(2) + sig(809) + sig_len(2) + sig(809) = 1,850
     *  
     *  Updated in: NexusMiner PR #65/#66 (full block mining)
     */
    static const size_t SUBMIT_BLOCK_DUAL_SIG_MAX = 1850;  // Falcon-512, was 1,702
    
    /** Submit block with BOTH signatures - PUBLIC MINER (with ChaCha20 encryption)
     *  
     *  Old value (merkle format): 1,730 bytes
     *    Dual sig(1,702) + ChaCha20 overhead(28) = 1,730
     *  
     *  Previous value (full block format, Falcon-512): 1,878 bytes (Legacy - largest)
     *    Dual sig(1,850) + ChaCha20 overhead(28) = 1,878
     *  
     *  UPDATED: Now supports 2MB blocks with transactions AND Falcon-1024
     *  Using Falcon-1024 sizes as maximum since nodes must accept both variants
     *  Format: [block(2MB)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)][physical_sig(1577)]
     *  Calculation: 2,097,152 + 8 + 2 + 1577 + 2 + 1577 + 28 = 2,100,346 bytes
     *  
     *  Updated in: NexusMiner PR #65/#66 (full block mining), PR #115 (2MB support), PR #122 (Falcon-1024)
     */
    static const size_t SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX = 2100346;  // Falcon-1024, was 1878, then 2098810

    /***************************************************************************
     * Full Block Format - Detailed Size Constants (Optional - For Reference)
     * 
     * These constants provide granular details for different block types and
     * signature scenarios in the full block format.
     * 
     * NOTE (PR #122): All sizes now include Physical Falcon length field (2 bytes)
     * which is ALWAYS present, even when Physical Falcon is disabled (value=0).
     * 
     * Falcon-512 constants are listed first, followed by Falcon-1024 constants.
     **************************************************************************/

    /** Tritium wrapper signature (Falcon-512) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(809)][physiglen(2)]
     *  Was 1035, now 1037 (added physiglen field) */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX = 1037;

    /** Tritium wrapper signature (Falcon-512) - encrypted
     *  Was 1063, now 1065 (added physiglen field in encrypted payload) */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_ENCRYPTED_MAX = 1065;

    /** Legacy wrapper signature (Falcon-512) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(809)][physiglen(2)]
     *  Was 1039, now 1041 (added physiglen field) */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX = 1041;

    /** Legacy wrapper signature (Falcon-512) - encrypted
     *  Was 1067, now 1069 (added physiglen field in encrypted payload) */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_ENCRYPTED_MAX = 1069;

    /** Tritium dual signature (Falcon-512) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(809)][physiglen(2)][physical_sig(809)]
     *  Was 1846, stays 1846 (already included physical sig, just formalized the physiglen field) */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_MAX = 1846;

    /** Tritium dual signature (Falcon-512) - encrypted
     *  Was 1874, stays 1874 (already included physical sig) */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_ENCRYPTED_MAX = 1874;

    /** Legacy dual signature (Falcon-512) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(809)][physiglen(2)][physical_sig(809)]
     *  Was 1850, stays 1850 (already included physical sig) */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_MAX = 1850;

    /** Legacy dual signature (Falcon-512) - encrypted
     *  Was 1878, stays 1878 (already included physical sig) */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_ENCRYPTED_MAX = 1878;

    /***************************************************************************
     * Falcon-1024 Packet Size Constants
     * 
     * These constants define packet sizes when using Falcon-1024 signatures.
     * Falcon-1024 uses 1577-byte signatures vs Falcon-512's 809 bytes.
     **************************************************************************/

    /** Tritium wrapper signature (Falcon-1024) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)]
     *  Calculation: 216 + 8 + 2 + 1577 + 2 = 1805 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_FALCON1024_MAX = 1805;

    /** Tritium wrapper signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 1805 + 28 = 1833 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_FALCON1024_ENCRYPTED_MAX = 1833;

    /** Legacy wrapper signature (Falcon-1024) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)]
     *  Calculation: 220 + 8 + 2 + 1577 + 2 = 1809 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_FALCON1024_MAX = 1809;

    /** Legacy wrapper signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 1809 + 28 = 1837 bytes */
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_FALCON1024_ENCRYPTED_MAX = 1837;

    /** Tritium dual signature (Falcon-1024) - localhost
     *  Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)][physical_sig(1577)]
     *  Calculation: 216 + 8 + 2 + 1577 + 2 + 1577 = 3382 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_FALCON1024_MAX = 3382;

    /** Tritium dual signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 3382 + 28 = 3410 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_FALCON1024_ENCRYPTED_MAX = 3410;

    /** Legacy dual signature (Falcon-1024) - localhost
     *  Format: [block(220)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)][physical_sig(1577)]
     *  Calculation: 220 + 8 + 2 + 1577 + 2 + 1577 = 3386 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_FALCON1024_MAX = 3386;

    /** Legacy dual signature (Falcon-1024) - encrypted
     *  Add ChaCha20 overhead: 3386 + 28 = 3414 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_FALCON1024_ENCRYPTED_MAX = 3414;

    /** Falcon-1024 physical block signature overhead
     *  sig_len(2) + signature(1577) = 1579 bytes */
    static const size_t PHYSICAL_BLOCK_SIG_FALCON1024_OVERHEAD = LENGTH_FIELD_SIZE + FALCON1024_SIG_ABSOLUTE_MAX;  // 1579 bytes

    /** Falcon-1024 dual signature overhead (wrapper + physical)
     *  (2 + 1577) + (2 + 1577) = 3158 bytes */
    static const size_t DUAL_SIG_FALCON1024_OVERHEAD = 3158;

    /** Falcon-1024 dual signature overhead + timestamp
     *  3158 + 8 = 3166 bytes */
    static const size_t DUAL_SIG_FALCON1024_TOTAL_OVERHEAD = 3166;

    /***************************************************************************
     * Version-Agnostic Overhead Aliases
     * 
     * These aliases use the maximum values from both Falcon-512 and Falcon-1024
     * to ensure the node can allocate sufficient buffers for packets from either variant.
     * Logic gates should use these aliases instead of version-specific constants.
     **************************************************************************/

    /** Physical block signature overhead - maximum across all Falcon variants
     *  Falcon-512: sig_len(2) + signature(809) = 811 bytes
     *  Falcon-1024: sig_len(2) + signature(1577) = 1579 bytes
     *  Use largest for buffer allocation */
    static const size_t PHYSICAL_BLOCK_SIG_MAX_OVERHEAD = PHYSICAL_BLOCK_SIG_FALCON1024_OVERHEAD;  // 1579 bytes

    /** Dual signature overhead - maximum across all Falcon variants
     *  Falcon-512: (2 + 809) + (2 + 809) = 1622 bytes
     *  Falcon-1024: (2 + 1577) + (2 + 1577) = 3158 bytes
     *  Use largest for buffer allocation */
    static const size_t DUAL_SIG_MAX_OVERHEAD = DUAL_SIG_FALCON1024_OVERHEAD;  // 3158 bytes

    /** Dual signature total overhead (with timestamp) - maximum across all Falcon variants
     *  Falcon-512: 1622 + 8 = 1630 bytes
     *  Falcon-1024: 3158 + 8 = 3166 bytes
     *  Use largest for buffer allocation */
    static const size_t DUAL_SIG_MAX_TOTAL_OVERHEAD = DUAL_SIG_FALCON1024_TOTAL_OVERHEAD;  // 3166 bytes

    /** Minimum block submission size with physical signature overhead - minimum across variants
     *  Use Falcon-512 minimum since it's smaller */
    static const size_t BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD_AGNOSTIC = PHYSICAL_BLOCK_SIG_OVERHEAD;  // 811 bytes

    /***************************************************************************
     * GET_BLOCK / BLOCK_DATA Response Sizes (Node → Miner)
     **************************************************************************/

    /** Minimum BLOCK_DATA response size (Tritium block) */
    static const size_t BLOCK_DATA_RESPONSE_MIN = 216;

    /** Maximum BLOCK_DATA response size (Legacy block) */
    static const size_t BLOCK_DATA_RESPONSE_MAX = 220;

    /***************************************************************************
     * Dual Signature Overhead Helpers
     **************************************************************************/

    /** Total overhead for dual signatures (wrapper + physical)
     *  (2 + 809) + (2 + 809) = 1,622 bytes */
    static const size_t DUAL_SIG_OVERHEAD = 1622;

    /** Total overhead for dual signatures + timestamp
     *  1,622 + 8 = 1,630 bytes */
    static const size_t DUAL_SIG_TOTAL_OVERHEAD = 1630;

    /***************************************************************************
     * Authentication Response Sizes - CT=809
     **************************************************************************/
    
    /** Auth response - LOCALHOST (no encryption on pubkey)
     *  pubkey_len(2) + pubkey(897) + timestamp(8) + sig_len(2) + sig(809) = 1718 bytes */
    static const size_t AUTH_RESPONSE_MAX = LENGTH_FIELD_SIZE + FALCON512_PUBKEY_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX;  // 1718 bytes
    
    /** Auth response - PUBLIC MINER (ChaCha20 wrapped pubkey)
     *  pubkey_len(2) + wrapped_pubkey(897+28) + timestamp(8) + sig_len(2) + sig(809) = 1746 bytes */
    static const size_t AUTH_RESPONSE_ENCRYPTED_MAX = AUTH_RESPONSE_MAX + CHACHA20_OVERHEAD;  // 1746 bytes
    
    /** Auth response with optional GenesisHash binding
     *  Add 32 bytes for Tritium genesis hash */
    static const size_t AUTH_RESPONSE_WITH_GENESIS_MAX = AUTH_RESPONSE_ENCRYPTED_MAX + GENESIS_HASH_SIZE;  // 1778 bytes

    /***************************************************************************
     * MINER_AUTH_INIT Sizes (Enhanced with hashGenesis)
     **************************************************************************/

    /** MINER_AUTH_INIT minimum size (no miner_id, no genesis)
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2) = 901 bytes */
    static const size_t MINER_AUTH_INIT_MIN = 901;

    /** MINER_AUTH_INIT with genesis but empty miner_id string
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2, value=0) + genesis(32) = 933 bytes */
    static const size_t MINER_AUTH_INIT_WITH_GENESIS = 933;

    /** MINER_AUTH_INIT maximum size (with miner_id and genesis)
     *  pubkey_len(2) + pubkey(897) + miner_id_len(2) + miner_id(256) + genesis(32) = 1189 bytes */
    static const size_t MINER_AUTH_INIT_MAX = 1189;

    /***************************************************************************
     * Mining Template Validity Constants
     * 
     * NOTE: These constants are defined for future implementation of 
     * timestamp-based template staleness checking. Currently, template 
     * validation is height-based only (templates are invalidated when 
     * blockchain height changes). Time-based validation can be added in 
     * future enhancements to provide additional staleness protection.
     **************************************************************************/

    /** Maximum age of a mining template in seconds
     *  Templates older than this should be discarded even if height hasn't changed.
     *  Prevents miners from working on very stale templates. */
    static const uint64_t MAX_TEMPLATE_AGE_SECONDS = 60;  // 1 minute

    /** Template staleness warning threshold
     *  Warn if template is this old but still valid by height.
     *  Helps detect slow template distribution. */
    static const uint64_t TEMPLATE_STALENESS_WARNING_SECONDS = 30;  // 30 seconds

} // namespace FalconConstants
} // namespace LLP

#endif
