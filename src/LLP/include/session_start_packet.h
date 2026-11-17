/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_START_PACKET_H
#define NEXUS_LLP_INCLUDE_SESSION_START_PACKET_H

#include <cstdint>
#include <vector>

#include <LLC/types/uint1024.h>

namespace LLP
{

    /** SessionStartPacket
     *
     *  Shared utility for building SESSION_START wire-format payloads.
     *  Used by both the Legacy (miner.cpp, port 8323) and Stateless
     *  (stateless_miner_connection.cpp, port 9323) lanes as well as the
     *  pure-functional ProcessSessionStart handler in stateless_miner.cpp.
     *
     *  Wire format (Node → Miner):
     *    [0]       success         (uint8; 0x01 = success)
     *    [1..4]    session_id      (little-endian uint32)
     *    [5..8]    timeout_seconds (little-endian uint32)
     *    [9..40]   genesis_hash    (32 bytes; big-endian / display order, MSW-first;
     *                              byte[0] == type byte, identical to GetHex())
     *
     *  Total: 9 bytes without genesis, 41 bytes with genesis.
     *
     **/
    namespace SessionStartPacket
    {

        /** BuildPayload
         *
         *  Build the SESSION_START payload bytes from session parameters.
         *
         *  @param[in] nSessionId     Session identifier (little-endian on wire).
         *  @param[in] nTimeoutSec    Session timeout in seconds (little-endian on wire).
         *  @param[in] hashGenesis    Tritium genesis hash (appended if non-zero).
         *
         *  @return std::vector<uint8_t> containing the complete SESSION_START payload.
         *
         **/
        inline std::vector<uint8_t> BuildPayload(
            uint32_t nSessionId,
            uint64_t nTimeoutSec,
            const uint256_t& hashGenesis)
        {
            std::vector<uint8_t> vPayload;
            vPayload.reserve(41); // max: 1 + 4 + 4 + 32

            /* Success flag */
            vPayload.push_back(0x01);

            /* Session ID (4 bytes, little-endian) */
            vPayload.push_back(static_cast<uint8_t>(nSessionId & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nSessionId >> 8) & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nSessionId >> 16) & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nSessionId >> 24) & 0xFF));

            /* Timeout in seconds (4 bytes, little-endian).
             * Cast to uint32_t — session timeouts realistically fit in 32 bits
             * (max ~136 years in seconds).  Matches NexusMiner parser expectation. */
            const uint32_t nTimeout32 = static_cast<uint32_t>(nTimeoutSec);
            vPayload.push_back(static_cast<uint8_t>(nTimeout32 & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nTimeout32 >> 8) & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nTimeout32 >> 16) & 0xFF));
            vPayload.push_back(static_cast<uint8_t>((nTimeout32 >> 24) & 0xFF));

            /* Genesis hash (32 bytes, optional).
             *
             * NexusMiner parses the genesis field as a plain byte string in
             * big-endian / display order — byte[0] carries the type byte,
             * exactly as printed by GetHex().
             *
             * GetBytes() iterates the internal uint32_t array pn[0]…pn[WIDTH-1]
             * from least-significant word (pn[0]) to most-significant word
             * (pn[WIDTH-1]), with each 32-bit word serialised big-endian.  That
             * produces the *word-reversed* form of display order, which is the
             * source of the "reward_address GENESIS MISMATCH" error visible in
             * the miner log.
             *
             * Fix: reverse the 4-byte word groups after calling GetBytes() so
             * the wire bytes spell out the hash MSW-first, matching GetHex(). */
            if(hashGenesis != 0)
            {
                std::vector<uint8_t> vGenesis = hashGenesis.GetBytes();
                /* uint256_t is stored as eight 32-bit words; each word is
                 * serialised big-endian by GetBytes(), so reversing the word
                 * groups (4 bytes each) converts LSW-first to MSW-first. */
                constexpr size_t WORD_BYTES = 4;
                const size_t nWords = vGenesis.size() / WORD_BYTES;
                for(size_t lo = 0, hi = nWords - 1; lo < hi; ++lo, --hi)
                {
                    for(size_t b = 0; b < WORD_BYTES; ++b)
                    {
                        const uint8_t tmp              = vGenesis[lo * WORD_BYTES + b];
                        vGenesis[lo * WORD_BYTES + b]  = vGenesis[hi * WORD_BYTES + b];
                        vGenesis[hi * WORD_BYTES + b]  = tmp;
                    }
                }
                vPayload.insert(vPayload.end(), vGenesis.begin(), vGenesis.end());
            }

            return vPayload;
        }

    }  /* namespace SessionStartPacket */

}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_SESSION_START_PACKET_H */
