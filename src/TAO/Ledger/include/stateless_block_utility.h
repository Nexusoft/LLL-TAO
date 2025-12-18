/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H
#define NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/credentials.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** MiningMode
         *
         *  Enum representing different block creation modes for stateless mining.
         *
         **/
        enum class MiningMode
        {
            DAEMON_STATELESS,      // Mode 1: No credentials, miner signs producer
            INTERFACE_SESSION,     // Mode 2: Has credentials, node signs producer
            UNAVAILABLE           // Neither mode available (error condition)
        };


        /** DetectMiningMode
         *
         *  Intelligently detects which mining mode is available based on node state.
         *
         *  Checks for Mode 2 first (fastest/easiest):
         *  - Does SESSION::DEFAULT exist?
         *  - Is it unlocked for mining?
         *
         *  Falls back to Mode 1 (stateless daemon):
         *  - Mode 1 is always available from daemon perspective
         *  - It's the miner's responsibility to provide signed producers
         *
         *  @return The detected mining mode
         *
         **/
        MiningMode DetectMiningMode();


        /** CreateBlockForStatelessMining
         *
         *  Unified interface for creating Tritium blocks in stateless mining.
         *
         *  Auto-detects which mode is available and uses appropriate flow:
         *  - Mode 2 (INTERFACE_SESSION): Uses node credentials to sign producer
         *  - Mode 1 (DAEMON_STATELESS): Uses miner-provided pre-signed producer
         *
         *  @param[in] nChannel The mining channel (1 = Prime, 2 = Hash)
         *  @param[in] nExtraNonce Extra nonce for block iteration
         *  @param[in] hashRewardAddress Miner's reward recipient address
         *  @param[in] pPreSignedProducer Optional pre-signed producer (Mode 1 only)
         *
         *  @return Pointer to created block, or nullptr on failure
         *
         **/
        TritiumBlock* CreateBlockForStatelessMining(
            const uint32_t nChannel,
            const uint64_t nExtraNonce,
            const uint256_t& hashRewardAddress,
            const Transaction* pPreSignedProducer = nullptr);


        /** ValidateMinerProducer
         *
         *  Validates a miner-signed producer transaction for Mode 1.
         *
         *  Performs comprehensive validation:
         *  - Signature verification against expected genesis
         *  - Producer structure validation
         *  - Sequence number validity
         *  - Timestamp freshness check
         *  - Replay attack prevention
         *  - Reward address matching
         *
         *  @param[in] producer The producer transaction to validate
         *  @param[in] hashExpectedGenesis Expected genesis from Falcon auth
         *  @param[in] hashBoundRewardAddress Bound reward address from MINER_SET_REWARD
         *
         *  @return True if producer is valid, false otherwise
         *
         **/
        bool ValidateMinerProducer(
            const Transaction& producer,
            const uint256_t& hashExpectedGenesis,
            const uint256_t& hashBoundRewardAddress);


        /** CreateProducerTemplate
         *
         *  Creates template data that a miner needs to build a producer transaction.
         *
         *  This is used in Mode 1 two-phase flow:
         *  1. Miner requests template (this function)
         *  2. Miner creates producer locally using template data
         *  3. Miner signs producer with their credentials
         *  4. Miner submits signed producer to daemon
         *
         *  Template includes:
         *  - Previous block hash
         *  - Current block height
         *  - Block version
         *  - Suggested timestamp
         *  - Channel information
         *
         *  @param[in] nChannel The mining channel
         *  @param[out] hashPrevBlock Previous block hash
         *  @param[out] nHeight Current block height
         *  @param[out] nVersion Block version
         *  @param[out] nTimestamp Suggested timestamp
         *
         *  @return True if template created successfully
         *
         **/
        bool CreateProducerTemplate(
            const uint32_t nChannel,
            uint1024_t& hashPrevBlock,
            uint32_t& nHeight,
            uint32_t& nVersion,
            uint64_t& nTimestamp);

    }
}

#endif
