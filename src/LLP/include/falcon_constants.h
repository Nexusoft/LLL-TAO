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
     **************************************************************************/
    
    /** Full Tritium block size (without signature or timestamp)
     *  Based on diagnostic data showing 216-byte Tritium blocks */
    static const size_t FULL_BLOCK_TRITIUM_SIZE = 216;
    
    /** Full Legacy block size (without signature or timestamp)
     *  Based on diagnostic data showing 220-byte Legacy blocks */
    static const size_t FULL_BLOCK_LEGACY_SIZE = 220;
    
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
     * GET_BLOCK / BLOCK_DATA Response Sizes (Node → Miner)
     * 
     * When miner sends GET_BLOCK(129), node responds with BLOCK_DATA(0)
     * containing a full serialized block (NexusMiner PR #65/#66).
     * 
     * Format: [Full Block Data]
     *   - Tritium: 216 bytes
     *   - Legacy:  220 bytes
     * 
     * The node sends ONLY the block data, no additional fields.
     **************************************************************************/
    
    /** Minimum BLOCK_DATA response size (Tritium block)
     *  Full Tritium block = 216 bytes */
    static const size_t BLOCK_DATA_RESPONSE_MIN = FULL_BLOCK_TRITIUM_SIZE;  // 216 bytes
    
    /** Maximum BLOCK_DATA response size (Legacy block)
     *  Full Legacy block = 220 bytes */
    static const size_t BLOCK_DATA_RESPONSE_MAX = FULL_BLOCK_LEGACY_SIZE;  // 220 bytes
    
    /** Validation range for BLOCK_DATA packets
     *  Must be between 216 bytes (Tritium) and 220 bytes (Legacy) */
    static const size_t BLOCK_DATA_MIN_VALIDATION = FULL_BLOCK_TRITIUM_SIZE;  // 216 bytes
    static const size_t BLOCK_DATA_MAX_VALIDATION = FULL_BLOCK_LEGACY_SIZE;   // 220 bytes

    /***************************************************************************
     * DEPRECATED: Legacy 64-byte Merkle Format Constants
     * 
     * ⚠️  WARNING: These constants are for the OLD protocol format that sent
     * only 64-byte merkle roots. NexusMiner PR #65/#66 changed the protocol
     * to send FULL BLOCKS (216/220 bytes). These constants remain for backward
     * compatibility with old miners but should NOT be used for new code.
     * 
     * Migration: Use SUBMIT_BLOCK_FULL_* constants instead (see below).
     **************************************************************************/
    
    /** DEPRECATED: Size of the message signed in old Submit Block wrapper
     *  merkle_root(64) + nonce(8) + timestamp(8) = 80 bytes
     *  NOTE: This is FIXED regardless of actual block size */
    static const size_t SUBMIT_BLOCK_MESSAGE_SIZE = 80;  // 64 + 8 + 8
    
    /** DEPRECATED: Minimum Submit Block wrapper size (without signature)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) = 82 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MIN = 82;
    
    /** DEPRECATED: Submit Block wrapper - LOCALHOST (no encryption)
     *  merkle(64) + nonce(8) + timestamp(8) + sig_len(2) + sig(809) = 891 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_MAX = 891;
    
    /** DEPRECATED: Submit Block wrapper - PUBLIC MINER (with ChaCha20 encryption)
     *  nonce(12) + encrypted_payload(891) + auth_tag(16) = 919 bytes */
    static const size_t SUBMIT_BLOCK_WRAPPER_ENCRYPTED_MAX = 919;

    /***************************************************************************
     * SUBMIT_BLOCK Full Block Format (Miner → Node) - NexusMiner PR #65/#66
     * 
     * ASCII Packet Format Diagram:
     * ┌────────────────────────────────────────────────────────────────────┐
     * │ SUBMIT_BLOCK with Full Block - No Signature                       │
     * ├────────────────────────────────────────────────────────────────────┤
     * │ Full Block (216/220 bytes) │ Timestamp (8 bytes)                  │
     * └────────────────────────────────────────────────────────────────────┘
     * 
     * ┌────────────────────────────────────────────────────────────────────┐
     * │ SUBMIT_BLOCK with Full Block - Wrapper Signature Only             │
     * ├────────────────────────────────────────────────────────────────────┤
     * │ Full Block (216/220) │ Timestamp (8) │ SigLen (2) │ Sig (~809)    │
     * └────────────────────────────────────────────────────────────────────┘
     * 
     * ┌────────────────────────────────────────────────────────────────────┐
     * │ SUBMIT_BLOCK with Full Block - Dual Signature                     │
     * ├────────────────────────────────────────────────────────────────────┤
     * │ Block │ Timestamp │ WrapSigLen │ WrapSig │ PhySigLen │ PhySig     │
     * │ (216) │    (8)    │     (2)    │  (~809) │    (2)    │  (~809)    │
     * └────────────────────────────────────────────────────────────────────┘
     **************************************************************************/
    
    /** SUBMIT_BLOCK minimum - Full block + timestamp (no signature)
     *  Tritium: 216 + 8 = 224 bytes
     *  Legacy:  220 + 8 = 228 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_MIN = FULL_BLOCK_TRITIUM_SIZE + TIMESTAMP_SIZE;  // 224 bytes
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_MIN = FULL_BLOCK_LEGACY_SIZE + TIMESTAMP_SIZE;    // 228 bytes
    
    /** SUBMIT_BLOCK with wrapper signature - LOCALHOST (no encryption)
     *  Components: Block + Timestamp + sig_len(2) + sig(809)
     *  Tritium: 216 + 8 + 2 + 809 = 1,035 bytes
     *  Legacy:  220 + 8 + 2 + 809 = 1,039 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX = FULL_BLOCK_TRITIUM_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX;  // 1,035 bytes
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX = FULL_BLOCK_LEGACY_SIZE + TIMESTAMP_SIZE + LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX;    // 1,039 bytes
    
    /** SUBMIT_BLOCK with wrapper signature - PUBLIC MINER (encrypted)
     *  Add ChaCha20 overhead: nonce(12) + auth_tag(16) = 28 bytes
     *  Tritium: 1,035 + 28 = 1,063 bytes
     *  Legacy:  1,039 + 28 = 1,067 bytes */
    static const size_t SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_MAX + CHACHA20_OVERHEAD;  // 1,063 bytes
    static const size_t SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_LEGACY_WRAPPER_MAX + CHACHA20_OVERHEAD;    // 1,067 bytes

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
     * DEPRECATED: Dual Signature - OLD Merkle Format (64-byte merkle root)
     * 
     * ⚠️  WARNING: These constants calculate dual signature sizes for the OLD
     * protocol format. They remain for backward compatibility only.
     * 
     * Migration: Use SUBMIT_BLOCK_FULL_DUAL_* constants below for new code.
     **************************************************************************/
    
    /** DEPRECATED: Submit block with BOTH signatures - LOCALHOST (no encryption)
     *  Disposable wrapper(891) + Physical signature overhead(811) = 1,702 bytes
     *  This is the maximum size when both Disposable Falcon wrapper AND 
     *  Physical Block Signature are used together on localhost. */
    static const size_t SUBMIT_BLOCK_DUAL_SIG_MAX = SUBMIT_BLOCK_WRAPPER_MAX + PHYSICAL_BLOCK_SIG_OVERHEAD;  // 1,702 bytes
    
    /** DEPRECATED: Submit block with BOTH signatures - PUBLIC MINER (with ChaCha20 encryption)
     *  Dual sig(1,702) + ChaCha20 overhead(28) = 1,730 bytes
     *  This is the maximum size when both signatures are used on public miners
     *  with encryption enabled. */
    static const size_t SUBMIT_BLOCK_DUAL_SIG_ENCRYPTED_MAX = SUBMIT_BLOCK_DUAL_SIG_MAX + CHACHA20_OVERHEAD;  // 1,730 bytes

    /***************************************************************************
     * Dual Signature - Full Block Format (NexusMiner PR #65/#66)
     * 
     * When both Wrapper Signature AND Physical Block Signature are used:
     * 
     * Calculation Formula (LOCALHOST - no encryption):
     *   TOTAL = BLOCK + TIMESTAMP + WRAPPER_SIG + PHYSICAL_SIG
     *   TOTAL = BLOCK + 8 + (2 + 809) + (2 + 809)
     *   TOTAL = BLOCK + 8 + 811 + 811
     *   TOTAL = BLOCK + 1630
     * 
     * Tritium: 216 + 1630 = 1,846 bytes
     * Legacy:  220 + 1630 = 1,850 bytes
     * 
     * With Encryption (PUBLIC MINER):
     *   Add ChaCha20 overhead: +28 bytes (nonce=12, auth_tag=16)
     *   Tritium: 1,846 + 28 = 1,874 bytes
     *   Legacy:  1,850 + 28 = 1,878 bytes
     **************************************************************************/
    
    /** Helper: Total overhead for dual signatures (wrapper + physical)
     *  Wrapper sig overhead:  2 + 809 = 811 bytes
     *  Physical sig overhead: 2 + 809 = 811 bytes
     *  Total:                 811 + 811 = 1,622 bytes */
    static const size_t DUAL_SIG_OVERHEAD = (LENGTH_FIELD_SIZE + FALCON512_SIG_ABSOLUTE_MAX) * 2;  // 1,622 bytes
    
    /** Helper: Total overhead for dual signatures + timestamp
     *  Dual sig overhead(1,622) + timestamp(8) = 1,630 bytes */
    static const size_t DUAL_SIG_TOTAL_OVERHEAD = DUAL_SIG_OVERHEAD + TIMESTAMP_SIZE;  // 1,630 bytes
    
    /** Submit Tritium block with BOTH signatures - LOCALHOST (no encryption)
     *  Block(216) + Timestamp(8) + WrapperSig(2+809) + PhysicalSig(2+809) = 1,846 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_MAX = FULL_BLOCK_TRITIUM_SIZE + DUAL_SIG_TOTAL_OVERHEAD;  // 1,846 bytes
    
    /** Submit Tritium block with BOTH signatures - PUBLIC MINER (encrypted)
     *  Dual sig(1,846) + ChaCha20 overhead(28) = 1,874 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_MAX + CHACHA20_OVERHEAD;  // 1,874 bytes
    
    /** Submit Legacy block with BOTH signatures - LOCALHOST (no encryption)
     *  Block(220) + Timestamp(8) + WrapperSig(2+809) + PhysicalSig(2+809) = 1,850 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_MAX = FULL_BLOCK_LEGACY_SIZE + DUAL_SIG_TOTAL_OVERHEAD;  // 1,850 bytes
    
    /** Submit Legacy block with BOTH signatures - PUBLIC MINER (encrypted)
     *  Dual sig(1,850) + ChaCha20 overhead(28) = 1,878 bytes */
    static const size_t SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_ENCRYPTED_MAX = SUBMIT_BLOCK_FULL_DUAL_SIG_LEGACY_MAX + CHACHA20_OVERHEAD;  // 1,878 bytes

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

} // namespace FalconConstants
} // namespace LLP

#endif
