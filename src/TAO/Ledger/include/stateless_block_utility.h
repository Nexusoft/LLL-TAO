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

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/include/constants/template.h>

#include <string>
#include <vector>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** CreateBlockForStatelessMining
         *
         *  Create wallet-signed block for stateless mining.
         *
         *  IMPORTANT: "Stateless" refers to MINER state (no blockchain required),
         *  NOT block signing. All blocks MUST be wallet-signed per Nexus consensus.
         *
         *  Architecture:
         *  - Falcon authentication: Miner session security
         *  - Wallet signing: Block consensus requirement (Autologin required)
         *
         *  Node requirements:
         *  - Must have wallet unlocked via -autologin=username:password
         *  - Miners authenticate with Falcon keys (separate from wallet)
         *  - Rewards routed to hashRewardAddress (can be miner's address)
         *
         *  @param[in] nChannel The mining channel (1 = Prime, 2 = Hash)
         *  @param[in] nExtraNonce Extra nonce for block iteration
         *  @param[in] hashRewardAddress Miner's reward recipient address
         *
         *  @return Pointer to wallet-signed block, or nullptr on failure
         *
         **/
        TritiumBlock* CreateBlockForStatelessMining(
            const uint32_t nChannel,
            const uint64_t nExtraNonce,
            const uint256_t& hashRewardAddress);

        /** CreateTemplateCache
         *
         *  Create and cache template for specific channel.
         *  Called from BlockState::SetBest() after block validation.
         *  
         *  @param[in] nChannel Channel to cache (1=Prime, 2=Hash)
         *  @return True if cache created successfully
         **/
        bool CreateTemplateCache(uint32_t nChannel);
        
        /** GetCachedTemplate
         *
         *  Retrieve cached template (fast path).
         *  
         *  @param[in] nChannel Channel to retrieve
         *  @param[out] block Deserialized block template (if cache hit)
         *  @return True if cache hit
         **/
        bool GetCachedTemplate(uint32_t nChannel, TritiumBlock& block);
        
        /** GetCachedTemplateSerialized
         *
         *  Retrieve cached template as serialized bytes (fastest).
         *  
         *  @param[in] nChannel Channel to retrieve
         *  @param[out] vTemplate Serialized template bytes
         *  @return True if cache hit
         **/
        bool GetCachedTemplateSerialized(uint32_t nChannel, std::vector<uint8_t>& vTemplate);
        
        /** InvalidateTemplateCache
         *
         *  Mark cache as invalid (blockchain advanced).
         **/
        void InvalidateTemplateCache(uint32_t nChannel);
        
        /** IsTemplateCacheFresh
         *
         *  Check if cache is valid and not stale.
         **/
        bool IsTemplateCacheFresh(uint32_t nChannel);
        
        /** GetTemplateCacheStatistics
         *
         *  Get cache performance statistics (formatted string).
         **/
        std::string GetTemplateCacheStatistics();
        
        /** GetTemplateCacheEfficiency
         *
         *  Calculate cache hit rate (0.0-1.0).
         **/
        double GetTemplateCacheEfficiency();

    }
}

#endif
