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
     *  Common utility for KEEPALIVE v2 protocol handling.
     *  Used by both the Legacy (Miner.cpp) and Stateless (stateless_miner.cpp) lanes.
     *
     *  Protocol versions (distinguished by payload length):
     *
     *  v1 Client → Node (len == 4):
     *    [0..3] session_id (little-endian uint32)
     *
     *  v2 Client → Node (len == 8):
     *    [0..3] session_id            (little-endian uint32)
     *    [4..7] miner_prevblock_suffix (little-endian uint32; last 4 bytes of template
     *                                   hashPrevBlock, or 0 if no template)
     *
     *  v1 Node → Client:
     *    Legacy lane   (len == 4): [0..3] session expiry seconds (little-endian uint32)
     *    Stateless lane (len == 5): [0] status byte (0x01 = active) +
     *                               [1..4] remaining_timeout (little-endian uint32)
     *
     *  v2 Node → Client UNIFIED (len == 32):
     *    Used on BOTH ports — SESSION_KEEPALIVE (port 8323) and KEEPALIVE_V2_ACK (port 9323, 0xD101).
     *    [0..3]   session_id          (little-endian uint32; session validation)
     *    [4..7]   hashPrevBlock_lo32  (big-endian uint32; echo of miner's fork canary, 0 on legacy path)
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

        /** KeepAliveV2Frame — 8-byte frame parsed from Miner → Node request (KEEPALIVE_V2)
         *
         *  Wire format (big-endian):
         *    [0-3]  uint32_t  sequence            Monotonic miner keepalive counter
         *    [4-7]  uint32_t  hashPrevBlock_lo32  Low 32 bits of miner's current prevHash (fork canary)
         **/
        struct KeepAliveV2Frame
        {
            uint32_t sequence{0};
            uint32_t hashPrevBlock_lo32{0};

            static constexpr uint32_t PAYLOAD_SIZE = 8;

            bool Parse(const std::vector<uint8_t>& data)
            {
                if(data.size() < 8) return false;
                auto r32 = [&](int o) -> uint32_t {
                    return (uint32_t(data[o  ]) << 24) | (uint32_t(data[o+1]) << 16)
                          |(uint32_t(data[o+2]) <<  8) |  uint32_t(data[o+3]);
                };
                sequence           = r32(0);
                hashPrevBlock_lo32 = r32(4);
                return true;
            }
        };


        /** KeepAliveV2AckFrame — 32-byte unified keepalive response (both SESSION_KEEPALIVE and KEEPALIVE_V2_ACK)
         *
         *  Wire format:
         *    [0-3]   uint32_t  session_id          little-endian — session validation
         *    [4-7]   uint32_t  hashPrevBlock_lo32  big-endian — echo of miner's fork canary
         *    [8-11]  uint32_t  unified_height      big-endian
         *    [12-15] uint32_t  hash_tip_lo32       big-endian — lo32 of node hashBestChain
         *    [16-19] uint32_t  prime_height        big-endian
         *    [20-23] uint32_t  hash_height         big-endian
         *    [24-27] uint32_t  stake_height        big-endian
         *    [28-31] uint32_t  fork_score          big-endian — 0=healthy, >0=divergence
         *
         *  Used on BOTH ports:
         *    - Legacy SESSION_KEEPALIVE response (port 8323)   replaces BuildBestCurrentResponse()
         *    - Stateless KEEPALIVE_V2_ACK (port 9323, 0xD101) replaces the old sequence-based format
         *
         *  nBits is NOT included — miner obtains difficulty from the 12-byte GET_BLOCK response.
         *  hashBestChain_prefix is NOT included — hash_tip_lo32 serves the fork-canary role.
         **/
        struct KeepAliveV2AckFrame
        {
            uint32_t session_id{0};         // [0-3]  LE — session validation
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

                // session_id: little-endian (matches legacy SESSION_KEEPALIVE encoding)
                v.push_back(static_cast<uint8_t>( session_id        & 0xFF));
                v.push_back(static_cast<uint8_t>((session_id >>  8) & 0xFF));
                v.push_back(static_cast<uint8_t>((session_id >> 16) & 0xFF));
                v.push_back(static_cast<uint8_t>((session_id >> 24) & 0xFF));

                // remaining fields: big-endian
                auto p32be = [&](uint32_t x) {
                    v.push_back((x >> 24) & 0xFF);
                    v.push_back((x >> 16) & 0xFF);
                    v.push_back((x >>  8) & 0xFF);
                    v.push_back( x        & 0xFF);
                };
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
         *  Replaces BuildBestCurrentResponse() which is now deleted.
         *
         *  @param[in] nSessionId      Session identifier (little-endian in wire format)
         *  @param[in] nHashPrevLo32   Lo32 of miner's hashPrevBlock echo (from request, 0 if legacy path)
         *  @param[in] nUnifiedHeight  Current unified best-chain height
         *  @param[in] nHashTipLo32    Lo32 of node's current hashBestChain
         *  @param[in] nPrimeHeight    Current Prime channel height
         *  @param[in] nHashHeight     Current Hash channel height
         *  @param[in] nStakeHeight    Current Stake channel height
         *  @param[in] nForkScore      Fork divergence score (0=healthy; use 0 on legacy path)
         *
         *  @return 32-byte serialized payload
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
         *  Parse a keepalive payload to extract session_id and (for v2) miner_prevblock_suffix.
         *
         *  @param[in]  data                   Raw payload bytes
         *  @param[out] nSessionId             Session ID (bytes [0..3], little-endian)
         *  @param[out] prevblockSuffixBytes   Raw bytes [4..7] as-sent (zeroed if payload is v1)
         *
         *  @return true if the payload is v2 (len >= 8), false if v1 (len >= 4 but < 8)
         *          Callers should reject if data.size() < 4.
         *
         **/
        inline bool ParsePayload(
            const std::vector<uint8_t>& data,
            uint32_t& nSessionId,
            std::array<uint8_t, 4>& prevblockSuffixBytes)
        {
            prevblockSuffixBytes = {};

            if(data.size() < 4)
                return false;

            /* Parse session_id (little-endian) */
            nSessionId =
                static_cast<uint32_t>(data[0])        |
                (static_cast<uint32_t>(data[1]) << 8)  |
                (static_cast<uint32_t>(data[2]) << 16) |
                (static_cast<uint32_t>(data[3]) << 24);

            if(data.size() >= 8)
            {
                /* v2: copy miner_prevblock_suffix as raw bytes (no endian conversion) */
                prevblockSuffixBytes[0] = data[4];
                prevblockSuffixBytes[1] = data[5];
                prevblockSuffixBytes[2] = data[6];
                prevblockSuffixBytes[3] = data[7];
                return true;
            }

            return false; /* v1 */
        }

    } /* namespace KeepaliveV2 */

} /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_KEEPALIVE_V2_H */
