/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CANONICAL_CHAIN_STATE_H
#define NEXUS_LLP_INCLUDE_CANONICAL_CHAIN_STATE_H

#include <LLC/types/uint1024.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/chainstate.h>

#include <chrono>
#include <cstdint>

namespace LLP
{

    /** CanonicalChainState
     *
     *  Point-in-time snapshot of the node's canonical chain state.
     *
     *  Captures the blockchain heights, difficulty targets, and previous block
     *  hash that the node considers authoritative at a given moment.  Used by
     *  the Stateless Block Utility and Colin Mining Agent for:
     *
     *  - Template validation: confirm a template is built on the canonical tip
     *  - Staleness detection: discard snapshots older than CANONICAL_STALE_SECONDS
     *  - Height-drift monitoring: measure how far the live chain has moved
     *    since this snapshot was taken
     *
     *  All fields use aggregate (brace) initialisation so the default-constructed
     *  state is all-zero / epoch, which is_initialized() reports as false.
     *
     *  THREAD SAFETY:
     *  ==============
     *  The struct itself is plain data (no mutex).  Callers that share a
     *  CanonicalChainState across threads should protect it externally,
     *  just as TemplateMetadata and HeightInfo are protected by their
     *  owning connection's mutex.
     *
     **/
    struct CanonicalChainState
    {
        /** Current unified blockchain height (increments for ALL channels) */
        uint32_t canonical_unified_height{0};

        /** Channel-specific height (increments only for the tracked channel) */
        uint32_t canonical_channel_height{0};

        /** Compact difficulty target (nBits) from the canonical tip */
        uint32_t canonical_difficulty_nbits{0};

        /** Channel-specific target value */
        uint32_t canonical_channel_target{0};

        /** Hash of the previous block on the canonical chain */
        uint1024_t canonical_hash_prev_block{};

        /** Steady-clock timestamp when this snapshot was captured */
        std::chrono::steady_clock::time_point canonical_received_at{};

        /* ═══════════════════════════════════════════════════════════════════ */
        /* CONSTANTS                                                          */
        /* ═══════════════════════════════════════════════════════════════════ */

        /** Maximum age (in seconds) before the snapshot is considered stale.
         *
         *  30 s is short enough to catch stale chain views during normal
         *  mining (block times ~50 s) but long enough to tolerate brief
         *  network jitter or burst fork-resolution events.
         */
        static constexpr uint32_t CANONICAL_STALE_SECONDS = 30;

        /* ═══════════════════════════════════════════════════════════════════ */
        /* QUERY METHODS                                                      */
        /* ═══════════════════════════════════════════════════════════════════ */

        /** is_initialized
         *
         *  Returns true if this snapshot has been populated with real data.
         *
         *  A default-constructed CanonicalChainState has canonical_unified_height == 0,
         *  which corresponds to the genesis block and is never a valid tip height
         *  once the chain has advanced past genesis.
         *
         *  @return true if the snapshot contains meaningful chain data.
         *
         **/
        bool is_initialized() const
        {
            return canonical_unified_height != 0;
        }


        /** is_canonically_stale
         *
         *  Returns true if this snapshot is too old to be trusted.
         *
         *  A snapshot is stale when:
         *    1. It was never initialised (is_initialized() == false), OR
         *    2. More than CANONICAL_STALE_SECONDS have elapsed since
         *       canonical_received_at.
         *
         *  @return true if the snapshot should be refreshed.
         *
         **/
        bool is_canonically_stale() const
        {
            if(!is_initialized())
                return true;

            const auto nElapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - canonical_received_at);

            return nElapsed.count() > static_cast<int64_t>(CANONICAL_STALE_SECONDS);
        }


        /** height_drift_from_canonical
         *
         *  Computes how far the live chain has moved since this snapshot.
         *
         *    drift = live_unified_height - canonical_unified_height
         *
         *  Positive: chain has advanced past the snapshot.
         *  Negative: chain has rolled back (fork / reorg).
         *  Zero:     chain is at the same height as the snapshot.
         *
         *  Uses ChainState::tStateBest (atomic load) for the live height.
         *
         *  @return Signed height drift (live − canonical).
         *
         **/
        int32_t height_drift_from_canonical() const
        {
            const TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();

            return static_cast<int32_t>(stateBest.nHeight)
                 - static_cast<int32_t>(canonical_unified_height);
        }


        /* ═══════════════════════════════════════════════════════════════════ */
        /* FACTORY METHODS                                                    */
        /* ═══════════════════════════════════════════════════════════════════ */

        /** from_chain_state
         *
         *  Factory: build a snapshot from a BlockState and channel state.
         *
         *  Captures all six fields in a single call so that
         *  SendStatelessTemplate() and the GET_BLOCK handler can
         *  construct a CanonicalChainState without duplicating the
         *  field-by-field assignment pattern.
         *
         *  @param[in] stateBest     Atomic snapshot of ChainState::tStateBest
         *  @param[in] stateChannel  Channel-specific state (via GetLastState)
         *  @param[in] nDifficulty   Compact difficulty target (nBits)
         *
         *  @return Fully-populated CanonicalChainState with
         *          canonical_received_at set to now().
         *
         **/
        static CanonicalChainState from_chain_state(
            const TAO::Ledger::BlockState& stateBest,
            const TAO::Ledger::BlockState& stateChannel,
            uint32_t nDifficulty)
        {
            CanonicalChainState snap;
            snap.canonical_unified_height   = stateBest.nHeight;
            snap.canonical_channel_height   = stateChannel.nChannelHeight;
            snap.canonical_difficulty_nbits = nDifficulty;
            snap.canonical_channel_target   = stateChannel.nChannelHeight;
            snap.canonical_hash_prev_block  = stateBest.GetHash();
            snap.canonical_received_at      = std::chrono::steady_clock::now();
            return snap;
        }
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_CANONICAL_CHAIN_STATE_H
