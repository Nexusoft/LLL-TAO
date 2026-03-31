/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_KEEPALIVE_V2_H
#define NEXUS_LLP_INCLUDE_KEEPALIVE_V2_H

#include <array>
#include <cstdint>
#include <vector>

namespace LLP
{

    /** KeepaliveV2
     *
     *  Common utility for SESSION_KEEPALIVE protocol handling.
     *  Used by both the Legacy (Miner.cpp) and Stateless (stateless_miner.cpp) lanes.
     *
     *  Unified Client → Node (len == 8, all big-endian):
     *    [0..3] session_id          (big-endian uint32)
     *    [4..7] hashPrevBlock_lo32  (big-endian uint32; low 32 bits of miner's
     *                                current prevHash, fork canary; 0 if no template)
     *
     *  Node → Client UNIFIED (len == 32):
     *    Used on BOTH ports — SESSION_KEEPALIVE (port 8323, 0xD4) and (port 9323, 0xD0D4).
     *    [0..3]   session_id          (big-endian uint32; session validation)
     *    [4..7]   hashPrevBlock_lo32  (big-endian uint32; echo of miner's fork canary)
     *    [8..11]  unified_height      (big-endian uint32)
     *    [12..15] hash_tip_lo32       (big-endian uint32; lo32 of node hashBestChain, fork cross-check)
     *    [16..19] prime_height        (big-endian uint32)
     *    [20..23] hash_height         (big-endian uint32)
     *    [24..27] stake_height        (big-endian uint32)
     *    [28..31] fork_score          (big-endian uint32; 0=healthy, >0=divergence magnitude)
     *
     *  nBits is NOT included — miner obtains difficulty from the 12-byte GET_BLOCK response.
     *
     **/
    namespace KeepaliveV2
    {

        /** KeepAliveV2AckFrame — 32-byte unified keepalive response (SESSION_KEEPALIVE on both ports)
         *
         *  Wire format:
         *    [0-3]   uint32_t  session_id          big-endian — session validation
         *    [4-7]   uint32_t  hashPrevBlock_lo32  big-endian — echo of miner's fork canary
         *    [8-11]  uint32_t  unified_height      big-endian
         *    [12-15] uint32_t  hash_tip_lo32       big-endian — lo32 of node hashBestChain
         *    [16-19] uint32_t  prime_height        big-endian
         *    [20-23] uint32_t  hash_height         big-endian
         *    [24-27] uint32_t  stake_height        big-endian
         *    [28-31] uint32_t  fork_score          big-endian — 0=healthy, >0=divergence
         *
         *  Used on BOTH ports:
         *    - Legacy SESSION_KEEPALIVE response (port 8323)
         *    - Stateless SESSION_KEEPALIVE response (port 9323, 0xD0D4)
         *
         *  nBits is NOT included — miner obtains difficulty from the 12-byte GET_BLOCK response.
         *  hashBestChain_prefix is NOT included — hash_tip_lo32 serves the fork-canary role.
         **/
        struct KeepAliveV2AckFrame
        {
            uint32_t session_id{0};         // [0-3]  BE — session validation
            uint32_t hashPrevBlock_lo32{0}; // [4-7]  BE — echo of miner's fork canary
            uint32_t unified_height{0};     // [8-11] BE
            uint32_t hash_tip_lo32{0};      // [12-15] BE — lo32 of node hashBestChain
            uint32_t prime_height{0};       // [16-19] BE
            uint32_t hash_height{0};        // [20-23] BE
            uint32_t stake_height{0};       // [24-27] BE
            uint32_t fork_score{0};         // [28-31] BE

            static constexpr uint32_t PAYLOAD_SIZE = 32;

            std::vector<uint8_t> Serialize() const
            {
                std::vector<uint8_t> v;
                v.reserve(32);

                // All fields big-endian (unified wire format)
                auto p32be = [&](uint32_t x) {
                    v.push_back((x >> 24) & 0xFF);
                    v.push_back((x >> 16) & 0xFF);
                    v.push_back((x >>  8) & 0xFF);
                    v.push_back( x        & 0xFF);
                };
                p32be(session_id);
                p32be(hashPrevBlock_lo32);
                p32be(unified_height);
                p32be(hash_tip_lo32);
                p32be(prime_height);
                p32be(hash_height);
                p32be(stake_height);
                p32be(fork_score);
                return v;
            }
        };


        /** BuildUnifiedResponse
         *
         *  Build the 32-byte unified keepalive response used on both legacy and stateless paths.
         *
         *  @param[in] nSessionId      Session identifier (big-endian in wire format)
         *  @param[in] nHashPrevLo32   Lo32 of miner's hashPrevBlock echo (from request)
         *  @param[in] nUnifiedHeight  Current unified best-chain height
         *  @param[in] nHashTipLo32    Lo32 of node's current hashBestChain
         *  @param[in] nPrimeHeight    Current Prime channel height
         *  @param[in] nHashHeight     Current Hash channel height
         *  @param[in] nStakeHeight    Current Stake channel height
         *  @param[in] nForkScore      Fork divergence score (0=healthy)
         *
         *  @return 32-byte serialized payload (all fields big-endian)
         **/
        inline std::vector<uint8_t> BuildUnifiedResponse(
            uint32_t nSessionId,
            uint32_t nHashPrevLo32,
            uint32_t nUnifiedHeight,
            uint32_t nHashTipLo32,
            uint32_t nPrimeHeight,
            uint32_t nHashHeight,
            uint32_t nStakeHeight,
            uint32_t nForkScore)
        {
            KeepAliveV2AckFrame frame;
            frame.session_id         = nSessionId;
            frame.hashPrevBlock_lo32 = nHashPrevLo32;
            frame.unified_height     = nUnifiedHeight;
            frame.hash_tip_lo32      = nHashTipLo32;
            frame.prime_height       = nPrimeHeight;
            frame.hash_height        = nHashHeight;
            frame.stake_height       = nStakeHeight;
            frame.fork_score         = nForkScore;
            return frame.Serialize();
        }


        /** ParsePayload
         *
         *  Parse an 8-byte SESSION_KEEPALIVE request payload (big-endian).
         *
         *  Wire format:
         *    [0..3] session_id          (big-endian uint32)
         *    [4..7] hashPrevBlock_lo32  (big-endian uint32; fork canary, 0 if no template)
         *
         *  @param[in]  data                   Raw payload bytes (must be exactly 8)
         *  @param[out] nSessionId             Session ID (bytes [0..3], big-endian)
         *  @param[out] prevblockSuffixBytes   Raw bytes [4..7] as-sent (fork canary)
         *
         *  @return true if the payload is valid (len >= 8), false otherwise
         *
         **/
        inline bool ParsePayload(
            const std::vector<uint8_t>& data,
            uint32_t& nSessionId,
            std::array<uint8_t, 4>& prevblockSuffixBytes)
        {
            prevblockSuffixBytes = {};

            if(data.size() < 8)
                return false;

            /* Parse session_id (big-endian) */
            nSessionId =
                (static_cast<uint32_t>(data[0]) << 24) |
                (static_cast<uint32_t>(data[1]) << 16) |
                (static_cast<uint32_t>(data[2]) << 8)  |
                 static_cast<uint32_t>(data[3]);

            /* Copy hashPrevBlock_lo32 as raw bytes (already big-endian on wire) */
            prevblockSuffixBytes[0] = data[4];
            prevblockSuffixBytes[1] = data[5];
            prevblockSuffixBytes[2] = data[6];
            prevblockSuffixBytes[3] = data[7];
            return true;
        }

    } /* namespace KeepaliveV2 */

} /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_KEEPALIVE_V2_H */
