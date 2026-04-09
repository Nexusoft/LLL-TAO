/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/template_cache.h>

#include <Util/include/debug.h>

namespace LLP
{

    /* GlobalTemplateCache singleton accessor. */
    GlobalTemplateCache& GlobalTemplateCache::Get()
    {
        static GlobalTemplateCache instance;
        return instance;
    }


    /* Store a freshly created template for the given channel. */
    void GlobalTemplateCache::CacheTemplate(const uint32_t nChannel,
                                             const uint32_t nUnifiedHeight,
                                             const std::vector<uint8_t>& vBlockData,
                                             const uint32_t nBits)
    {
        if(nChannel > MAX_CHANNEL)
        {
            debug::log(2, FUNCTION, "GlobalTemplateCache: channel ", nChannel,
                       " out of range (max ", MAX_CHANNEL, ") — not cached");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex[nChannel]);
            CachedTemplate& entry = m_cache[nChannel];
            entry.vBlockData     = vBlockData;
            entry.nBits          = nBits;
            entry.nUnifiedHeight = nUnifiedHeight;
            entry.fValid         = true;
        }

        debug::log(3, FUNCTION, "GlobalTemplateCache: stored channel=", nChannel,
                   " unified=", nUnifiedHeight,
                   " bits=", nBits,
                   " size=", vBlockData.size(), "B");
    }


    /* Retrieve a cached template for the given channel and height. */
    GlobalTemplateCache::CachedTemplate GlobalTemplateCache::GetCachedTemplate(
        const uint32_t nChannel, const uint32_t nUnifiedHeight) const
    {
        if(nChannel > MAX_CHANNEL)
            return CachedTemplate{};

        std::lock_guard<std::mutex> lock(m_mutex[nChannel]);
        const CachedTemplate& entry = m_cache[nChannel];

        if(!entry.fValid || entry.nUnifiedHeight != nUnifiedHeight)
            return CachedTemplate{};

        debug::log(3, FUNCTION, "GlobalTemplateCache: HIT channel=", nChannel,
                   " unified=", nUnifiedHeight,
                   " bits=", entry.nBits,
                   " size=", entry.vBlockData.size(), "B");

        return entry;
    }


    /* Invalidate the cache for a single channel. */
    void GlobalTemplateCache::InvalidateChannel(const uint32_t nChannel)
    {
        if(nChannel > MAX_CHANNEL)
            return;

        std::lock_guard<std::mutex> lock(m_mutex[nChannel]);
        m_cache[nChannel].fValid = false;
    }


    /* Invalidate all cached templates. */
    void GlobalTemplateCache::InvalidateAll()
    {
        for(uint32_t nChannel = 0; nChannel <= MAX_CHANNEL; ++nChannel)
        {
            std::lock_guard<std::mutex> lock(m_mutex[nChannel]);
            m_cache[nChannel].fValid = false;
        }
    }

} // namespace LLP
