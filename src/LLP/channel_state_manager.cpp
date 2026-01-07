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
    namespace
    {
        /** GetStakeManager
         *
         *  Returns singleton StakeStateManager instance.
         *  Thread-safe initialization guaranteed by C++11.
         *
         **/
        StakeStateManager& GetStakeManager()
        {
            static StakeStateManager manager;
            return manager;
        }


        /** GetPrimeManager
         *
         *  Returns singleton PrimeStateManager instance.
         *  Thread-safe initialization guaranteed by C++11.
         *
         **/
        PrimeStateManager& GetPrimeManager()
        {
            static PrimeStateManager manager;
            return manager;
        }


        /** GetHashManager
         *
         *  Returns singleton HashStateManager instance.
         *  Thread-safe initialization guaranteed by C++11.
         *
         **/
        HashStateManager& GetHashManager()
        {
            static HashStateManager manager;
            return manager;
        }
    }


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
         * Use cached channel height which was just updated by SyncWithBlockchain() above.
         * While tStateBest is loaded twice (once in Sync, once here), the blockchain state
         * is atomic and both reads occur in quick succession, ensuring temporal consistency.
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


    /* Static: Get all channel heights */
    bool ChannelStateManager::GetAllChannelHeights(uint32_t& nStake, uint32_t& nPrime, uint32_t& nHash, uint32_t& nUnified)
    {
        /* Sync all managers with blockchain */
        GetStakeManager().SyncWithBlockchain();
        GetPrimeManager().SyncWithBlockchain();
        GetHashManager().SyncWithBlockchain();
        
        /* Get heights from existing infrastructure */
        nStake = GetStakeManager().GetChannelHeight();
        nPrime = GetPrimeManager().GetChannelHeight();
        nHash = GetHashManager().GetChannelHeight();
        
        /* Get unified height from blockchain state (all managers see same unified height) */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        nUnified = tStateBest.nHeight;
        
        return true;
    }


    /* Static: Verify all channel heights sum to unified */
    bool ChannelStateManager::VerifyAllChannels(uint32_t nCheckInterval)
    {
        /* Get current unified height for interval check */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        uint32_t nUnifiedHeight = tStateBest.nHeight;
        
        /* Skip during initial sync */
        if(TAO::Ledger::ChainState::Synchronizing())
            return true;
        
        /* Skip for genesis block */
        if(nUnifiedHeight == 0)
            return true;
        
        /* Only check at specified interval (default every 10 blocks) */
        if(nCheckInterval > 0 && (nUnifiedHeight % nCheckInterval) != 0)
            return true;
        
        /* Get all channel heights using existing managers */
        uint32_t nStake = 0, nPrime = 0, nHash = 0, nUnified = 0;
        if(!GetAllChannelHeights(nStake, nPrime, nHash, nUnified))
        {
            debug::error(FUNCTION, "Failed to get channel heights");
            return false;
        }
        
        /* Calculate expected unified height using uint64_t to prevent theoretical overflow */
        uint64_t nCalculated = static_cast<uint64_t>(nStake) + nPrime + nHash;
        
        /* Log verification at debug level */
        debug::log(2, FUNCTION, "═══ UNIFIED HEIGHT VERIFICATION (Height ", nUnified, ") ═══");
        debug::log(2, FUNCTION, "   Stake:      ", nStake);
        debug::log(2, FUNCTION, "   Prime:      ", nPrime);
        debug::log(2, FUNCTION, "   Hash:       ", nHash);
        debug::log(2, FUNCTION, "   Calculated: ", nCalculated);
        debug::log(2, FUNCTION, "   Actual:     ", nUnified);
        
        /* Check for match */
        if(nCalculated == static_cast<uint64_t>(nUnified))
        {
            debug::log(2, FUNCTION, "✓ Unified height consistent");
            return true;
        }
        
        /* MISMATCH DETECTED - Use existing fork detection mechanism */
        int64_t nDiff = static_cast<int64_t>(nUnified) - static_cast<int64_t>(nCalculated);
        
        debug::error("═══════════════════════════════════════════");
        debug::error("❌ UNIFIED HEIGHT MISMATCH DETECTED!");
        debug::error("═══════════════════════════════════════════");
        debug::error("   Expected (Stake+Prime+Hash): ", nCalculated);
        debug::error("   Actual unified height:       ", nUnified);
        debug::error("   Difference:                  ", nDiff);
        debug::error("");
        debug::error("   Stake channel:  ", nStake);
        debug::error("   Prime channel:  ", nPrime);
        debug::error("   Hash channel:   ", nHash);
        debug::error("");
        
        /* Diagnostic based on difference direction */
        if(nDiff > 0)
        {
            debug::error("   DIAGNOSIS: Unified height is AHEAD");
            debug::error("   Possible causes:");
            debug::error("     - Block double-counted in unified height");
            debug::error("     - Channel height not incremented");
            debug::error("     - Database corruption");
        }
        else
        {
            debug::error("   DIAGNOSIS: Unified height is BEHIND");
            debug::error("   Possible causes:");
            debug::error("     - Channel block not reflected in unified");
            debug::error("     - Incomplete rollback");
            debug::error("     - Channel on fork");
        }
        
        debug::error("");
        debug::error("   RECOMMENDED ACTIONS:");
        debug::error("     1. Stop the node immediately");
        debug::error("     2. Run: ./nexus -reindex");
        debug::error("     3. If problem persists, run: ./nexus -rescan");
        debug::error("     4. Check for disk/hardware errors");
        debug::error("═══════════════════════════════════════════");
        
        /* Trigger fork detection on all managers using existing callback mechanism */
        GetStakeManager().m_fForkDetected.store(true);
        GetPrimeManager().m_fForkDetected.store(true);
        GetHashManager().m_fForkDetected.store(true);
        
        /* Call existing OnForkDetected() which triggers user callbacks */
        GetStakeManager().OnForkDetected();
        GetPrimeManager().OnForkDetected();
        GetHashManager().OnForkDetected();
        
        return false;
    }


    /* StakeStateManager constructor */
    StakeStateManager::StakeStateManager()
        : ChannelStateManager(0)  // Stake channel = 0
    {
    }

} // namespace LLP
