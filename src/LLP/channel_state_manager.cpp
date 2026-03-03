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

#include <sstream>

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
        
        /* Use tolerance-based verification */
        return VerifyUnifiedHeightWithTolerance(nStake, nPrime, nHash, nUnified);
    }


    /* Static: Verify unified height with tolerance */
    bool ChannelStateManager::VerifyUnifiedHeightWithTolerance(
        uint32_t nStake, uint32_t nPrime, uint32_t nHash, uint32_t nUnified,
        uint32_t nTolerance)
    {
        /* Calculate expected unified height using uint64_t to prevent theoretical overflow */
        uint64_t nCalculated = static_cast<uint64_t>(nStake) + nPrime + nHash;
        
        /* Log verification at debug level */
        debug::log(2, FUNCTION, "═══ UNIFIED HEIGHT VERIFICATION (Height ", nUnified, ") ═══");
        debug::log(2, FUNCTION, "   Stake:      ", nStake);
        debug::log(2, FUNCTION, "   Prime:      ", nPrime);
        debug::log(2, FUNCTION, "   Hash:       ", nHash);
        debug::log(2, FUNCTION, "   Calculated: ", nCalculated);
        debug::log(2, FUNCTION, "   Actual:     ", nUnified);
        debug::log(2, FUNCTION, "   Tolerance:  ", nTolerance);
        
        /* Perfect match - no discrepancy */
        if(nCalculated == static_cast<uint64_t>(nUnified))
        {
            debug::log(2, FUNCTION, "✓ Unified height consistent (perfect match)");
            return true;
        }
        
        /* Calculate discrepancy */
        int64_t nDifference = static_cast<int64_t>(nCalculated) - static_cast<int64_t>(nUnified);
        uint32_t nAbsDifference = (nDifference >= 0) ? nDifference : -nDifference;
        
        /* Check if within tolerance */
        if(nAbsDifference <= nTolerance)
        {
            /* Within tolerance - log warning but accept */
            debug::log(0, "");
            debug::log(0, "═══════════════════════════════════════════");
            debug::log(0, "⚠ UNIFIED HEIGHT WITHIN TOLERANCE");
            debug::log(0, "═══════════════════════════════════════════");
            debug::log(0, "   Stake channel:      ", nStake);
            debug::log(0, "   Prime channel:      ", nPrime);
            debug::log(0, "   Hash channel:       ", nHash);
            debug::log(0, "   Calculated sum:     ", nCalculated);
            debug::log(0, "   Actual unified:     ", nUnified);
            debug::log(0, "   Difference:         ", nDifference);
            debug::log(0, "   Tolerance:          ", nTolerance);
            debug::log(0, "");
            debug::log(0, "   NOTE: This is expected from pre-Tritium fork resolution");
            debug::log(0, "   where orphaned blocks were preserved for network resilience.");
            debug::log(0, "   See docs/CHANNEL_HEIGHT_DISCREPANCY.md for details.");
            debug::log(0, "");
            debug::log(0, "✓ Unified height verification passed (within tolerance)");
            debug::log(0, "═══════════════════════════════════════════");
            debug::log(0, "");
            
            return true;
        }
        
        /* CRITICAL: Discrepancy exceeds tolerance - this is a NEW issue */
        debug::error("");
        debug::error("═══════════════════════════════════════════");
        debug::error("❌ CRITICAL: Unified height mismatch exceeds tolerance!");
        debug::error("═══════════════════════════════════════════");
        debug::error("   Stake channel:      ", nStake);
        debug::error("   Prime channel:      ", nPrime);
        debug::error("   Hash channel:       ", nHash);
        debug::error("   Calculated sum:     ", nCalculated);
        debug::error("   Actual unified:     ", nUnified);
        debug::error("   Difference:         ", nDifference);
        debug::error("   Tolerance:          ", nTolerance);
        debug::error("");
        debug::error("   This indicates a NEW fork or database corruption!");
        debug::error("");
        
        /* Diagnostic based on difference direction */
        if(nDifference > 0)
        {
            debug::error("   DIAGNOSIS: Channel heights are AHEAD of unified");
            debug::error("   Possible causes:");
            debug::error("     - Channel heights incorrectly incremented");
            debug::error("     - Unified height not updated during block processing");
            debug::error("     - Database corruption");
        }
        else
        {
            debug::error("   DIAGNOSIS: Unified height is AHEAD of channels");
            debug::error("   Possible causes:");
            debug::error("     - Block double-counted in unified height");
            debug::error("     - Channel height not incremented");
            debug::error("     - Incomplete rollback");
            debug::error("     - Database corruption");
        }
        
        debug::error("");
        debug::error("   RECOMMENDED ACTIONS:");
        debug::error("     1. Stop the node immediately");
        debug::error("     2. Run: ./nexus -forensicforks (analyze the issue)");
        debug::error("     3. Run: ./nexus -reindex (rebuild database)");
        debug::error("     4. If problem persists, run: ./nexus -rescan");
        debug::error("     5. Check for disk/hardware errors");
        debug::error("═══════════════════════════════════════════");
        debug::error("");
        
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


    /* Static: Find orphaned blocks */
    uint32_t ChannelStateManager::FindOrphanedBlocks(uint32_t nMaxDepth)
    {
        /* Get current best block */
        TAO::Ledger::BlockState tStateBest = TAO::Ledger::ChainState::tStateBest.load();
        uint32_t nCurrentHeight = tStateBest.nHeight;
        
        /* Limit scan depth */
        uint32_t nStartHeight = (nCurrentHeight > nMaxDepth) ? (nCurrentHeight - nMaxDepth) : 0;
        
        debug::log(0, FUNCTION, "Scanning for orphaned blocks from height ", nStartHeight, " to ", nCurrentHeight);
        
        uint32_t nOrphanedCount = 0;
        
        /* Scan backward through blockchain */
        for(uint32_t nHeight = nCurrentHeight; nHeight > nStartHeight; --nHeight)
        {
            /* Get block at this height */
            TAO::Ledger::BlockState state;
            if(!LLD::Ledger->ReadBlock(nHeight, state))
                continue;  // Skip if can't read
            
            /* Check if this is an orphaned block (not chain tip and hashNextBlock == 0) */
            if(nHeight < nCurrentHeight && state.hashNextBlock == 0)
            {
                ++nOrphanedCount;
                debug::log(0, "   Found orphaned block at height ", nHeight, 
                          " hash: ", state.GetHash().SubString());
            }
        }
        
        debug::log(0, FUNCTION, "Found ", nOrphanedCount, " orphaned blocks");
        
        return nOrphanedCount;
    }


    /* Static: Analyze channel height discrepancy */
    ForensicForkInfo ChannelStateManager::AnalyzeChannelHeightDiscrepancy()
    {
        ForensicForkInfo info;
        
        debug::log(0, "");
        debug::log(0, "═══════════════════════════════════════════════════════");
        debug::log(0, "       CHANNEL HEIGHT DISCREPANCY FORENSIC ANALYSIS      ");
        debug::log(0, "═══════════════════════════════════════════════════════");
        debug::log(0, "");
        
        /* Get all channel heights */
        if(!GetAllChannelHeights(info.nStakeHeight, info.nPrimeHeight, 
                                  info.nHashHeight, info.nUnifiedHeight))
        {
            debug::error(FUNCTION, "Failed to get channel heights for analysis");
            return info;
        }
        
        /* Calculate discrepancy */
        info.nCalculatedSum = static_cast<uint64_t>(info.nStakeHeight) + 
                              info.nPrimeHeight + info.nHashHeight;
        info.nDiscrepancy = static_cast<int64_t>(info.nCalculatedSum) - 
                           static_cast<int64_t>(info.nUnifiedHeight);
        
        uint32_t nAbsDiscrepancy = (info.nDiscrepancy >= 0) ? 
            info.nDiscrepancy : -info.nDiscrepancy;
        info.fWithinTolerance = (nAbsDiscrepancy <= HISTORICAL_FORK_TOLERANCE);
        
        /* Display current state */
        debug::log(0, "CURRENT BLOCKCHAIN STATE:");
        debug::log(0, "   Stake channel:      ", info.nStakeHeight);
        debug::log(0, "   Prime channel:      ", info.nPrimeHeight);
        debug::log(0, "   Hash channel:       ", info.nHashHeight);
        debug::log(0, "   ───────────────────────────────────");
        debug::log(0, "   Calculated sum:     ", info.nCalculatedSum);
        debug::log(0, "   Actual unified:     ", info.nUnifiedHeight);
        debug::log(0, "   Discrepancy:        ", info.nDiscrepancy);
        debug::log(0, "");
        
        /* Check tolerance */
        if(info.fWithinTolerance)
        {
            debug::log(0, "TOLERANCE CHECK: ✓ PASS");
            debug::log(0, "   Discrepancy (", nAbsDiscrepancy, ") within tolerance (", 
                      HISTORICAL_FORK_TOLERANCE, ")");
            debug::log(0, "   This is consistent with historical fork resolution.");
        }
        else
        {
            debug::error("TOLERANCE CHECK: ❌ FAIL");
            debug::error("   Discrepancy (", nAbsDiscrepancy, ") EXCEEDS tolerance (", 
                        HISTORICAL_FORK_TOLERANCE, ")");
            debug::error("   This indicates a NEW issue requiring investigation!");
        }
        debug::log(0, "");
        
        /* Scan for orphaned blocks */
        debug::log(0, "ORPHANED BLOCK SCAN:");
        info.nOrphanedBlocks = FindOrphanedBlocks(10000);
        debug::log(0, "");
        
        /* Provide analysis */
        debug::log(0, "ANALYSIS:");
        if(info.nDiscrepancy > 0)
        {
            debug::log(0, "   Channel heights are AHEAD of unified height by ", 
                      info.nDiscrepancy, " blocks");
            debug::log(0, "   This suggests orphaned blocks were counted in channels");
            debug::log(0, "   but excluded from unified height (expected behavior).");
        }
        else if(info.nDiscrepancy < 0)
        {
            debug::error("   Unified height is AHEAD of channel heights by ", 
                        -info.nDiscrepancy, " blocks");
            debug::error("   This is UNEXPECTED and may indicate database corruption!");
        }
        else
        {
            debug::log(0, "   Perfect match - no discrepancy detected.");
        }
        debug::log(0, "");
        
        /* Recommendations */
        debug::log(0, "RECOMMENDATIONS:");
        if(info.fWithinTolerance)
        {
            debug::log(0, "   ✓ No action required - blockchain state is consistent");
            debug::log(0, "   ✓ Discrepancy within expected historical tolerance");
            debug::log(0, "   See docs/CHANNEL_HEIGHT_DISCREPANCY.md for background");
        }
        else
        {
            debug::error("   ❌ IMMEDIATE ACTION REQUIRED:");
            debug::error("   1. Backup your blockchain data");
            debug::error("   2. Run: ./nexus -reindex");
            debug::error("   3. If issue persists, run: ./nexus -rescan");
            debug::error("   4. Report this issue with forensic logs");
        }
        
        debug::log(0, "");
        debug::log(0, "═══════════════════════════════════════════════════════");
        debug::log(0, "");
        
        return info;
    }


    /* Static: Get channel height statistics */
    std::string ChannelStateManager::GetChannelHeightStatistics()
    {
        /* Get all channel heights */
        uint32_t nStake = 0, nPrime = 0, nHash = 0, nUnified = 0;
        if(!GetAllChannelHeights(nStake, nPrime, nHash, nUnified))
        {
            return "{\"error\": \"Failed to get channel heights\"}";
        }
        
        /* Calculate discrepancy */
        uint64_t nCalculated = static_cast<uint64_t>(nStake) + nPrime + nHash;
        int64_t nDiscrepancy = static_cast<int64_t>(nCalculated) - 
                              static_cast<int64_t>(nUnified);
        uint32_t nAbsDiscrepancy = (nDiscrepancy >= 0) ? nDiscrepancy : -nDiscrepancy;
        bool fWithinTolerance = (nAbsDiscrepancy <= HISTORICAL_FORK_TOLERANCE);
        
        /* Find orphaned blocks */
        uint32_t nOrphaned = FindOrphanedBlocks(10000);
        
        /* Build JSON response */
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"stake_height\": " << nStake << ",\n";
        ss << "  \"prime_height\": " << nPrime << ",\n";
        ss << "  \"hash_height\": " << nHash << ",\n";
        ss << "  \"calculated_sum\": " << nCalculated << ",\n";
        ss << "  \"unified_height\": " << nUnified << ",\n";
        ss << "  \"discrepancy\": " << nDiscrepancy << ",\n";
        ss << "  \"abs_discrepancy\": " << nAbsDiscrepancy << ",\n";
        ss << "  \"tolerance\": " << HISTORICAL_FORK_TOLERANCE << ",\n";
        ss << "  \"within_tolerance\": " << (fWithinTolerance ? "true" : "false") << ",\n";
        ss << "  \"orphaned_blocks\": " << nOrphaned << "\n";
        ss << "}";
        
        return ss.str();
    }


    /* StakeStateManager constructor */
    StakeStateManager::StakeStateManager()
        : ChannelStateManager(0)  // Stake channel = 0
    {
    }


    /* Static failover connection counter — definition */
    std::atomic<uint32_t> ChannelStateManager::m_nFailoverConnections{0};


    /* Static: NotifyFailoverConnection */
    void ChannelStateManager::NotifyFailoverConnection(const uint256_t& hashKeyID, const std::string& strAddr)
    {
        uint32_t nCount = m_nFailoverConnections.fetch_add(1, std::memory_order_relaxed) + 1;

        debug::log(0, FUNCTION, "Failover authentication confirmed:");
        debug::log(0, "   addr=", strAddr);
        debug::log(0, "   keyID=", hashKeyID.SubString());
        debug::log(0, "   total failover auths since boot: ", nCount);
    }


    /* Static: GetFailoverCount */
    uint32_t ChannelStateManager::GetFailoverCount()
    {
        return m_nFailoverConnections.load(std::memory_order_relaxed);
    }

} // namespace LLP
