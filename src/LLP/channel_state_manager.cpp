/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/channel_state_manager.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>
#include <LLD/include/global.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    /* Constructor */
    ChannelStateManager::ChannelStateManager(uint32_t nChannel)
        : m_nChannel(nChannel)
        , m_nLastUnifiedHeight(0)
        , m_fForkDetected(false)
        , m_nBlocksRolledBack(0)
        , m_stateCache()
        , m_nLastCacheTime(0)
        , m_forkCallback(nullptr)
    {
    }


    /* SyncWithBlockchain */
    bool ChannelStateManager::SyncWithBlockchain()
    {
        /* Get current best block state atomically */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Get current unified height */
        uint32_t nCurrentUnified = tStateBest.nHeight;
        uint32_t nPreviousUnified = m_nLastUnifiedHeight.load();
        
        /* FORK DETECTION: Check for unified height regression */
        if(nPreviousUnified > 0 && nCurrentUnified < nPreviousUnified)
        {
            /* Fork detected - unified height regressed (blockchain rollback) */
            m_fForkDetected.store(true);
            m_nBlocksRolledBack.store(nPreviousUnified - nCurrentUnified);
            
            /* Log fork detection */
            debug::log(0, "⚠ FORK DETECTED in ", GetChannelName(), " channel manager!");
            debug::log(0, "   Previous unified height: ", nPreviousUnified);
            debug::log(0, "   Current unified height:  ", nCurrentUnified);
            debug::log(0, "   Blocks rolled back:      ", m_nBlocksRolledBack.load());
            
            /* Trigger fork callback if set */
            OnForkDetected();
        }
        
        /* Update last known unified height */
        m_nLastUnifiedHeight.store(nCurrentUnified);
        
        /* Get channel-specific state using GetLastState() */
        TAO::Ledger::BlockState stateCurrent = tStateBest;
        
        /* GetLastState modifies stateCurrent in-place to contain the last block
         * in the specified channel (walks backward through blockchain) */
        if(!TAO::Ledger::GetLastState(stateCurrent, m_nChannel))
        {
            /* GetLastState failed - might be genesis or invalid channel */
            if(nCurrentUnified == 0)
            {
                /* Genesis block - this is OK */
                {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                    m_stateCache = tStateBest;
                    m_stateCache.nChannelHeight = 0;
                }
                m_nLastCacheTime.store(runtime::unifiedtimestamp());
                return true;
            }
            
            /* Non-genesis failure - log warning but don't fail */
            debug::warning(FUNCTION, "GetLastState failed for channel ", m_nChannel);
            debug::warning(FUNCTION, "   This might be the first block in this channel");
            
            /* Cache best state with channel height 0 */
            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                m_stateCache = tStateBest;
                m_stateCache.nChannelHeight = 0;
            }
            m_nLastCacheTime.store(runtime::unifiedtimestamp());
            return true;
        }
        
        /* Update cache with channel state */
        UpdateCache(stateCurrent);
        
        return true;
    }


    /* ValidateTemplateHeights */
    bool ChannelStateManager::ValidateTemplateHeights(uint32_t nUnified, uint32_t nChannel)
    {
        /* Always sync to ensure unified and channel heights are from the same blockchain snapshot
         * This prevents inconsistencies where we validate against fresh unified height
         * but stale cached channel height (could be up to 1 second old) */
        if(!SyncWithBlockchain())
            return false;
        
        /* Get current best block for unified height */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* VALIDATION 1: Check unified height (Block::Accept logic)
         * Template should be for next block:
         *   blockchain at height N → template for height N+1
         */
        if(nUnified != tStateBest.nHeight + 1)
        {
            /* Unified height mismatch - either fork or stale */
            debug::log(2, FUNCTION, "Unified height mismatch for ", GetChannelName());
            debug::log(2, "   Template unified:  ", nUnified);
            debug::log(2, "   Expected unified:  ", tStateBest.nHeight + 1);
            return false;
        }
        
        /* VALIDATION 2: Check channel height (Block::Accept logic)
         * Template should be for next channel block:
         *   channel at height M → template for height M+1
         * 
         * Use cached channel height which was synced from the same tStateBest snapshot above
         */
        uint32_t nCachedChannelHeight;
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            nCachedChannelHeight = m_stateCache.nChannelHeight;
        }
        
        if(nChannel != nCachedChannelHeight + 1)
        {
            /* Channel height mismatch - another block mined in this channel */
            debug::log(2, FUNCTION, "Channel height mismatch for ", GetChannelName());
            debug::log(2, "   Template channel:  ", nChannel);
            debug::log(2, "   Expected channel:  ", nCachedChannelHeight + 1);
            return false;
        }
        
        /* Both heights valid - template is fresh */
        return true;
    }


    /* CheckDescendant */
    bool ChannelStateManager::CheckDescendant(const uint1024_t& hashPrevBlock)
    {
        /* Read previous block state */
        TAO::Ledger::BlockState statePrev;
        if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
        {
            debug::error(FUNCTION, "Failed to read previous block state");
            return false;
        }
        
        /* Check if descendant of hardened checkpoint
         * Same logic as Block::Accept():
         *   if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
         *       return error;
         */
        bool fSynchronizing = TAO::Ledger::ChainState::Synchronizing();
        if(!fSynchronizing && !TAO::Ledger::IsDescendant(statePrev))
        {
            debug::error(FUNCTION, "Template not descendant of hardened checkpoint");
            return false;
        }
        
        return true;
    }


    /* GetHeightInfo */
    HeightInfo ChannelStateManager::GetHeightInfo()
    {
        /* Sync if cache invalid */
        if(!IsCacheValid())
            SyncWithBlockchain();
        
        /* Build HeightInfo struct */
        HeightInfo info;
        
        /* Get current best block */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        
        /* Fill in height information with mutex protection for cache access */
        uint32_t nCachedChannelHeight;
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            nCachedChannelHeight = m_stateCache.nChannelHeight;
        }
        
        info.nUnifiedHeight = tStateBest.nHeight;
        info.nChannelHeight = nCachedChannelHeight;
        info.nNextUnifiedHeight = tStateBest.nHeight + 1;
        info.nNextChannelHeight = nCachedChannelHeight + 1;
        info.nPrevUnifiedHeight = m_nLastUnifiedHeight.load();
        info.fForkDetected = m_fForkDetected.load();
        info.nBlocksRolledBack = m_nBlocksRolledBack.load();
        info.hashCurrentBlock = tStateBest.GetHash();
        info.hashPrevBlock = tStateBest.hashPrevBlock;
        info.nChannel = m_nChannel;
        
        return info;
    }


    /* GetUnifiedHeight */
    uint32_t ChannelStateManager::GetUnifiedHeight() const
    {
        return m_nLastUnifiedHeight.load();
    }


    /* GetChannelHeight */
    uint32_t ChannelStateManager::GetChannelHeight() const
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        return m_stateCache.nChannelHeight;
    }


    /* GetNextUnifiedHeight */
    uint32_t ChannelStateManager::GetNextUnifiedHeight() const
    {
        return m_nLastUnifiedHeight.load() + 1;
    }


    /* GetNextChannelHeight */
    uint32_t ChannelStateManager::GetNextChannelHeight() const
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        return m_stateCache.nChannelHeight + 1;
    }


    /* GetChannel */
    uint32_t ChannelStateManager::GetChannel() const
    {
        return m_nChannel;
    }


    /* IsForkDetected */
    bool ChannelStateManager::IsForkDetected() const
    {
        return m_fForkDetected.load();
    }


    /* GetBlocksRolledBack */
    int32_t ChannelStateManager::GetBlocksRolledBack() const
    {
        return m_nBlocksRolledBack.load();
    }


    /* ClearForkFlag */
    void ChannelStateManager::ClearForkFlag()
    {
        m_fForkDetected.store(false);
        m_nBlocksRolledBack.store(0);
    }


    /* SetForkCallback */
    void ChannelStateManager::SetForkCallback(std::function<void()> callback)
    {
        m_forkCallback = callback;
    }


    /* LogHeightInfo */
    void ChannelStateManager::LogHeightInfo()
    {
        HeightInfo info = GetHeightInfo();
        
        debug::log(0, "=== ", GetChannelName(), " Channel State ===");
        debug::log(0, "   Current unified height:  ", info.nUnifiedHeight);
        debug::log(0, "   Current channel height:  ", info.nChannelHeight);
        debug::log(0, "   Next unified height:     ", info.nNextUnifiedHeight);
        debug::log(0, "   Next channel height:     ", info.nNextChannelHeight);
        debug::log(0, "   Previous unified height: ", info.nPrevUnifiedHeight);
        
        if(info.fForkDetected)
        {
            debug::log(0, "   ⚠ FORK DETECTED!");
            debug::log(0, "   Blocks rolled back:      ", info.nBlocksRolledBack);
        }
        else
        {
            debug::log(0, "   Fork status:             None detected");
        }
        
        debug::log(0, "   Current block hash:      ", info.hashCurrentBlock.SubString());
        debug::log(0, "   Previous block hash:     ", info.hashPrevBlock.SubString());
    }


    /* GetChannelName */
    std::string ChannelStateManager::GetChannelName() const
    {
        switch(m_nChannel)
        {
            case 0:  return "Stake";
            case 1:  return "Prime";
            case 2:  return "Hash";
            default: return "Unknown";
        }
    }


    /* OnForkDetected (protected) */
    void ChannelStateManager::OnForkDetected()
    {
        /* Log fork detection */
        debug::log(0, FUNCTION, "Fork detected - ", m_nBlocksRolledBack.load(), " blocks rolled back");
        
        /* Call user callback if set */
        if(m_forkCallback)
        {
            debug::log(2, FUNCTION, "Calling fork callback for ", GetChannelName(), " channel");
            m_forkCallback();
        }
    }


    /* IsCacheValid (protected) */
    bool ChannelStateManager::IsCacheValid() const
    {
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCacheTime = m_nLastCacheTime.load();
        
        /* Cache is invalid if never set or expired */
        if(nCacheTime == 0)
            return false;
        
        /* Check if cache expired (older than CACHE_VALIDITY_SECONDS) */
        if(nNow > nCacheTime && (nNow - nCacheTime) > CACHE_VALIDITY_SECONDS)
            return false;
        
        return true;
    }


    /* UpdateCache (protected) */
    void ChannelStateManager::UpdateCache(const TAO::Ledger::BlockState& state)
    {
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_stateCache = state;
        }
        m_nLastCacheTime.store(runtime::unifiedtimestamp());
    }


    /* PrimeStateManager constructor */
    PrimeStateManager::PrimeStateManager()
        : ChannelStateManager(1)  // Prime channel = 1
    {
    }


    /* HashStateManager constructor */
    HashStateManager::HashStateManager()
        : ChannelStateManager(2)  // Hash channel = 2
    {
    }

} // namespace LLP
