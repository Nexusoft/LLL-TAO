/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_TEMPLATE_CACHE_H
#define NEXUS_LLP_INCLUDE_TEMPLATE_CACHE_H

#include <cstdint>
#include <mutex>
#include <vector>

namespace LLP
{

    /** GlobalTemplateCache
     *
     *  Process-wide cache of the most recently created block template per channel.
     *
     *  Eliminates redundant new_block() calls that contend with P2P LLD I/O.
     *  When TryAttachBlockTemplate() creates a template via new_block(), it stores
     *  the serialized payload here.  GET_ROUND auto-send, GET_BLOCK handlers, and
     *  SendLegacyTemplate() check this cache first; if the unified height matches
     *  the current chain tip the cached payload is reused without hitting LLD again.
     *
     *  Thread-safe: each channel slot is protected by its own mutex.
     *  Channels: 0 = Proof-of-Stake, 1 = Prime, 2 = Hash.
     *
     **/
    class GlobalTemplateCache
    {
    public:

        /** CachedTemplate
         *
         *  Entry stored for one channel.
         *
         **/
        struct CachedTemplate
        {
            /** Wire-ready payload: 12-byte metadata + serialised block (228 bytes total). **/
            std::vector<uint8_t> vBlockData;

            /** nBits from the block that was used to build this template. **/
            uint32_t nBits{0};

            /** Unified chain height at which this template was created. **/
            uint32_t nUnifiedHeight{0};

            /** true if this slot holds a valid (non-stale) template. **/
            bool fValid{false};
        };


        /** Get
         *
         *  Returns the process-wide singleton.
         *
         **/
        static GlobalTemplateCache& Get();


        /** CacheTemplate
         *
         *  Store a freshly created template for the given channel.
         *  Overwrites any previous entry for that channel.
         *
         *  @param[in] nChannel        Mining channel (0, 1, or 2).
         *  @param[in] nUnifiedHeight  Current unified chain height.
         *  @param[in] vBlockData      Serialized block bytes (Block::Serialize() output).
         *  @param[in] nBits           Difficulty bits from the created block.
         *
         **/
        void CacheTemplate(uint32_t nChannel, uint32_t nUnifiedHeight,
                           const std::vector<uint8_t>& vBlockData,
                           uint32_t nBits);


        /** GetCachedTemplate
         *
         *  Retrieve a cached template for the given channel and unified height.
         *  Returns an invalid entry (fValid == false) on a cache miss.
         *
         *  A miss occurs when:
         *    - nChannel is out of range (> 2).
         *    - No template has been stored for this channel yet.
         *    - The stored template was created at a different unified height.
         *
         *  @param[in] nChannel        Mining channel (0, 1, or 2).
         *  @param[in] nUnifiedHeight  Expected unified height of a valid template.
         *
         *  @return CachedTemplate with fValid==true on hit, fValid==false on miss.
         *
         **/
        CachedTemplate GetCachedTemplate(uint32_t nChannel,
                                         uint32_t nUnifiedHeight) const;


        /** InvalidateChannel
         *
         *  Explicitly invalidate the cache for a single channel.
         *  Called when a reorg or fork is detected to prevent stale template reuse.
         *
         *  @param[in] nChannel  Channel to invalidate (0, 1, or 2).
         *
         **/
        void InvalidateChannel(uint32_t nChannel);


        /** InvalidateAll
         *
         *  Invalidate all cached templates (all channels).
         *  Called on chain reorganisations.
         *
         **/
        void InvalidateAll();


    private:

        GlobalTemplateCache() = default;
        GlobalTemplateCache(const GlobalTemplateCache&) = delete;
        GlobalTemplateCache& operator=(const GlobalTemplateCache&) = delete;

        /** Maximum channel index supported (PoS=0, Prime=1, Hash=2). **/
        static constexpr uint32_t MAX_CHANNEL = 2;

        /** Per-channel mutex — avoids unnecessary contention between Prime and Hash miners. **/
        mutable std::mutex m_mutex[MAX_CHANNEL + 1];

        /** Per-channel cached template entry. **/
        CachedTemplate m_cache[MAX_CHANNEL + 1];
    };

} // namespace LLP

#endif
