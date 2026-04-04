/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_ROUND_STATE_UTILITY_H
#define NEXUS_LLP_INCLUDE_ROUND_STATE_UTILITY_H

#include <cstdint>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/convert.h>
#include <Util/include/debug.h>

namespace LLP
{

    /** RoundStateUtility
     *
     *  Shared utility for GET_ROUND / NEW_ROUND / OLD_ROUND wire-format payloads
     *  and chain-state snapshot capture.  Used by both the Legacy (miner.cpp,
     *  port 8323) and Stateless (stateless_miner_connection.cpp, port 9323) lanes.
     *
     *  Design:
     *  - Stateless pure functions: no internal mutexes, no side effects.
     *  - Callers take context snapshots under their own MUTEX before calling.
     *  - Uses convert::uint2bytes() for all big-endian serialization (BUG 2 fix).
     *  - CaptureHeights() tightly-scopes both atomic reads to minimize TOCTOU (BUG 4 fix).
     *
     **/
    namespace RoundStateUtility
    {

        /** ChainHeightSnapshot
         *
         *  Immutable snapshot of all channel heights at a point in time.
         *  Captured from ChainState atomics in a single tightly-scoped operation.
         *
         *  Wire format (16 bytes, all big-endian):
         *    [0-3]   nUnifiedHeight   — Current blockchain height (all channels)
         *    [4-7]   nPrimeHeight     — Last confirmed Prime channel height
         *    [8-11]  nHashHeight      — Last confirmed Hash channel height
         *    [12-15] nStakeHeight     — Last confirmed Stake channel height
         *
         **/
        struct ChainHeightSnapshot
        {
            uint32_t    nUnifiedHeight{0};
            uint32_t    nPrimeHeight{0};
            uint32_t    nHashHeight{0};
            uint32_t    nStakeHeight{0};
            uint1024_t  hashBestChain;
            bool        fValid{false};   // false if blockchain not initialized
        };


        /** CaptureHeights
         *
         *  Capture a consistent snapshot of all channel heights from ChainState.
         *  Uses raw GetLastState() calls (not ChannelStateManager cache) for
         *  maximum accuracy — GET_ROUND is our backup GET_BLOCK system if PUSH
         *  goes down, so up-to-date data matters.
         *
         *  Both tStateBest and hashBestChain are loaded back-to-back to minimize
         *  the temporal inconsistency window (BUG 4 mitigation).  Note: these are
         *  two separate atomic loads, so a chain advance between them is theoretically
         *  possible but extremely unlikely in practice (microsecond window).  True
         *  atomic consistency would require a single combined atomic structure which
         *  is beyond the scope of this utility.
         *
         *  @return ChainHeightSnapshot with all channel heights and validity flag.
         *
         **/
        inline ChainHeightSnapshot CaptureHeights()
        {
            ChainHeightSnapshot snap;

            /* Tightly-scoped atomic reads to minimize temporal inconsistency.
             * Both reads happen back-to-back with no intervening logic. */
            TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
            snap.hashBestChain = TAO::Ledger::ChainState::hashBestChain.load();

            /* Check blockchain readiness */
            if(stateBest.nHeight == 0)
            {
                snap.fValid = false;
                return snap;
            }

            snap.nUnifiedHeight = stateBest.nHeight;
            snap.fValid = true;

            /* Get all three channel heights using raw GetLastState() calls.
             * Reuse a single BlockState to reduce memory allocation. */
            TAO::Ledger::BlockState stateChannel = stateBest;

            /* Prime channel (1) */
            stateChannel = stateBest;
            if(TAO::Ledger::GetLastState(stateChannel, 1))
                snap.nPrimeHeight = stateChannel.nChannelHeight;

            /* Hash channel (2) */
            stateChannel = stateBest;
            if(TAO::Ledger::GetLastState(stateChannel, 2))
                snap.nHashHeight = stateChannel.nChannelHeight;

            /* Stake channel (0) */
            stateChannel = stateBest;
            if(TAO::Ledger::GetLastState(stateChannel, 0))
                snap.nStakeHeight = stateChannel.nChannelHeight;

            return snap;
        }


        /** GetChannelHeight
         *
         *  Get the channel-specific height for a given mining channel.
         *
         *  @param[in] snap      The height snapshot.
         *  @param[in] nChannel  The mining channel (1=Prime, 2=Hash).
         *
         *  @return The channel-specific height, or 0 if channel is invalid.
         *
         **/
        inline uint32_t GetChannelHeight(const ChainHeightSnapshot& snap, uint32_t nChannel)
        {
            if(nChannel == 1)
                return snap.nPrimeHeight;
            else if(nChannel == 2)
                return snap.nHashHeight;
            return 0;
        }


        /** SerializeRoundResponse
         *
         *  Build the 16-byte NEW_ROUND response payload using convert::uint2bytes()
         *  for big-endian serialization.  Replaces manual bit-shifting (BUG 2 fix).
         *
         *  Format: [Unified(4)][Prime(4)][Hash(4)][Stake(4)] = 16 bytes total.
         *
         *  @param[in] snap The height snapshot to serialize.
         *
         *  @return std::vector<uint8_t> containing the 16-byte NEW_ROUND payload.
         *
         **/
        inline std::vector<uint8_t> SerializeRoundResponse(const ChainHeightSnapshot& snap)
        {
            std::vector<uint8_t> vResponse;
            vResponse.reserve(16);

            /* Use convert::uint2bytes() for consistent big-endian encoding.
             * This matches the existing legacy lane implementation and replaces
             * the manual bit-shifting in the stateless lane. */
            std::vector<uint8_t> vUnified = convert::uint2bytes(snap.nUnifiedHeight);
            std::vector<uint8_t> vPrime   = convert::uint2bytes(snap.nPrimeHeight);
            std::vector<uint8_t> vHash    = convert::uint2bytes(snap.nHashHeight);
            std::vector<uint8_t> vStake   = convert::uint2bytes(snap.nStakeHeight);

            vResponse.insert(vResponse.end(), vUnified.begin(), vUnified.end());
            vResponse.insert(vResponse.end(), vPrime.begin(), vPrime.end());
            vResponse.insert(vResponse.end(), vHash.begin(), vHash.end());
            vResponse.insert(vResponse.end(), vStake.begin(), vStake.end());

            return vResponse;
        }


        /** SerializeTemplateMetadata
         *
         *  Build the 12-byte template metadata prefix that accompanies BLOCK_DATA
         *  auto-sends from GET_ROUND.  Uses convert::uint2bytes() for consistency.
         *
         *  Format: [UnifiedHeight(4)][ChannelHeight(4)][nBits(4)] = 12 bytes.
         *
         *  @param[in] nUnifiedHeight  The current blockchain unified height.
         *  @param[in] nChannelHeight  The miner's channel-specific height.
         *  @param[in] nBits           The difficulty target (nBits field).
         *
         *  @return std::vector<uint8_t> containing the 12-byte metadata prefix.
         *
         **/
        inline std::vector<uint8_t> SerializeTemplateMetadata(
            uint32_t nUnifiedHeight,
            uint32_t nChannelHeight,
            uint32_t nBits)
        {
            std::vector<uint8_t> vMetadata;
            vMetadata.reserve(12);

            std::vector<uint8_t> vUnified = convert::uint2bytes(nUnifiedHeight);
            std::vector<uint8_t> vChannel = convert::uint2bytes(nChannelHeight);
            std::vector<uint8_t> vDiff    = convert::uint2bytes(nBits);

            vMetadata.insert(vMetadata.end(), vUnified.begin(), vUnified.end());
            vMetadata.insert(vMetadata.end(), vChannel.begin(), vChannel.end());
            vMetadata.insert(vMetadata.end(), vDiff.begin(), vDiff.end());

            return vMetadata;
        }


        /** IsChannelStale
         *
         *  Check if the miner's template is stale based on channel-specific height.
         *
         *  CRITICAL: Uses channel-specific height (not unified height).
         *  Only triggers refresh when the miner's OWN channel advances.
         *  Using unified height would trigger unnecessary refreshes when OTHER
         *  channels mine blocks, causing ~40% wasted mining work.
         *
         *  @param[in] nChannel              The miner's mining channel (1=Prime, 2=Hash).
         *  @param[in] nLastTemplateHeight   Height when last template was sent.
         *  @param[in] snap                  Current chain height snapshot.
         *
         *  @return True if the template is stale and needs refreshing.
         *
         **/
        inline bool IsChannelStale(
            uint32_t nChannel,
            uint32_t nLastTemplateHeight,
            const ChainHeightSnapshot& snap)
        {
            return (nLastTemplateHeight != GetChannelHeight(snap, nChannel));
        }


        /** IsReorgDetected
         *
         *  Detect chain reorg by comparing hashBestChain snapshots.
         *  Mirrors the StakeMinter:534 pattern — catches same-height reorgs
         *  that channel height comparison alone would miss.
         *
         *  Skips the check if no template has been pushed yet (hashLastBlock == 0).
         *
         *  @param[in] hashLastBlock  The hashBestChain when last template was sent.
         *  @param[in] snap           Current chain height snapshot.
         *
         *  @return True if a reorg was detected.
         *
         **/
        inline bool IsReorgDetected(
            const uint1024_t& hashLastBlock,
            const ChainHeightSnapshot& snap)
        {
            return (hashLastBlock != uint1024_t(0) &&
                    hashLastBlock != snap.hashBestChain);
        }

    }  /* namespace RoundStateUtility */

}  /* namespace LLP */

#endif /* NEXUS_LLP_INCLUDE_ROUND_STATE_UTILITY_H */
