/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_CHANNEL_STATE_MANAGER_H
#define NEXUS_LLP_INCLUDE_CHANNEL_STATE_MANAGER_H

#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <LLC/types/uint1024.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <functional>
#include <mutex>

namespace LLP
{
    /** HeightInfo
     *
     *  Comprehensive height information for a mining channel.
     *  Includes unified blockchain height, channel-specific height,
     *  fork detection status, and diagnostic data.
     *
     *  This structure provides all information needed for:
     *  - Template validation
     *  - Fork detection
     *  - Staleness checking
     *  - Diagnostic logging
     *
     **/
    struct HeightInfo
    {
        /** Current unified blockchain height (all channels) */
        uint32_t nUnifiedHeight;
        
        /** Current channel-specific height */
        uint32_t nChannelHeight;
        
        /** Expected next unified height for new block */
        uint32_t nNextUnifiedHeight;
        
        /** Expected next channel height for new block */
        uint32_t nNextChannelHeight;
        
        /** Previous unified height (for fork detection) */
        uint32_t nPrevUnifiedHeight;
        
        /** Fork detected flag (unified height regressed) */
        bool fForkDetected;
        
        /** Number of blocks rolled back (if fork detected) */
        int32_t nBlocksRolledBack;
        
        /** Current best block hash */
        uint1024_t hashCurrentBlock;
        
        /** Previous block hash (for validation) */
        uint1024_t hashPrevBlock;
        
        /** Mining channel (1=Prime, 2=Hash, 0=Stake) */
        uint32_t nChannel;
        
        /** Default constructor - initializes all fields to zero/false */
        HeightInfo()
            : nUnifiedHeight(0)
            , nChannelHeight(0)
            , nNextUnifiedHeight(0)
            , nNextChannelHeight(0)
            , nPrevUnifiedHeight(0)
            , fForkDetected(false)
            , nBlocksRolledBack(0)
            , hashCurrentBlock(0)
            , hashPrevBlock(0)
            , nChannel(0)
        {
        }
    };


    /** ChannelStateManager
     *
     *  Fork-aware channel state manager for stateless mining.
     *  
     *  Integrates with Nexus's Block validation pipeline (Block::Check() → Block::Accept() → BlockState)
     *  to provide comprehensive mining template validation including:
     *  
     *  1. Fork Detection:
     *     - Tracks unified height regression (blockchain rollbacks)
     *     - Detects when previous unified height > current unified height
     *     - Automatically clears invalid templates on fork
     *  
     *  2. Dual Height Validation:
     *     - Validates unified height (mirrors Block::Accept)
     *     - Validates channel height (mirrors Block::Accept)
     *     - Ensures templates match blockchain consensus rules
     *  
     *  3. Checkpoint Descendant Checking:
     *     - Validates templates are descendants of hardened checkpoints
     *     - Uses same IsDescendant() logic as Block::Accept()
     *  
     *  4. BlockState Integration:
     *     - Uses ChainState::tStateBest for atomic blockchain state access
     *     - Leverages GetLastState() for channel-specific heights
     *     - Integrates with existing validation infrastructure
     *  
     *  ARCHITECTURE:
     *  =============
     *  
     *  Nexus Block Processing Pipeline:
     *    Block (minimal, serializable)
     *      ↓
     *    Block::Check()  ← Basic validation
     *      ↓
     *    Block::Accept()  ← Chain continuity validation
     *      ↓  (Creates BlockState from validated Block)
     *    BlockState (adds chain state, including nChannelHeight)
     *      ↓
     *    ChannelStateManager (wraps BlockState for mining)
     *  
     *  Block::Accept() validates:
     *    ├─ statePrev.nHeight + 1 == nHeight  (unified height)
     *    ├─ statePrev.nChannelHeight + 1 == nChannelHeight  (channel height)
     *    ├─ GetBlockTime() > statePrev.GetBlockTime()  (timestamp)
     *    ├─ nBits == GetNextTargetRequired(statePrev, channel)  (difficulty)
     *    └─ IsDescendant(statePrev)  (checkpoint descendant)
     *  
     *  ChannelStateManager mirrors this:
     *    ├─ ValidateUnifiedHeight()  ← Block::Accept() logic
     *    ├─ ValidateChannelHeight()  ← Block::Accept() logic
     *    ├─ DetectFork()  ← New capability
     *    └─ CheckDescendant()  ← Block::Accept() logic
     *  
     *  THREAD SAFETY:
     *  =============
     *  - All BlockState accesses use atomic operations
     *  - ChainState::tStateBest.load() is atomic
     *  - Atomic member variables use std::atomic<>
     *  - Cache access protected by mutex (m_cacheMutex)
     *  - Safe to call from multiple threads
     *  
     *  PERFORMANCE:
     *  ===========
     *  - GetLastState() is O(1) average (~3 blocks)
     *  - Cache validity period: 1 second
     *  - Fork detection adds <1ms per sync
     *  - Minimal overhead per template
     *  - Mutex lock only held during cache read/write operations
     *  
     *  USAGE:
     *  =====
     *  
     *    // Create manager for Prime channel
     *    PrimeStateManager primeMgr;
     *    
     *    // Sync with blockchain (detects forks)
     *    primeMgr.SyncWithBlockchain();
     *    
     *    // Get comprehensive state
     *    HeightInfo info = primeMgr.GetHeightInfo();
     *    
     *    // Validate template heights
     *    bool valid = primeMgr.ValidateTemplateHeights(
     *        info.nNextUnifiedHeight,
     *        info.nNextChannelHeight
     *    );
     *    
     *    // Check checkpoint descendant
     *    bool isDescendant = primeMgr.CheckDescendant(hashPrevBlock);
     *    
     *    // Log diagnostic info
     *    primeMgr.LogHeightInfo();
     *  
     **/
    class ChannelStateManager
    {
    public:
        /** Fork detected flag - set when height regression detected **/
        std::atomic<bool> m_fForkDetected;
        
    protected:
        /** Mining channel (1=Prime, 2=Hash, 0=Stake) */
        uint32_t m_nChannel;
        
        /** Last known unified height (for fork detection) */
        std::atomic<uint32_t> m_nLastUnifiedHeight;
        
        /** Number of blocks rolled back */
        std::atomic<int32_t> m_nBlocksRolledBack;
        
        /** Cached channel state (BlockState from last sync) */
        TAO::Ledger::BlockState m_stateCache;
        
        /** Mutex for thread-safe cache access */
        mutable std::mutex m_cacheMutex;
        
        /** Last cache update timestamp */
        std::atomic<uint64_t> m_nLastCacheTime;
        
        /** Cache validity period (1 second) */
        static constexpr uint64_t CACHE_VALIDITY_SECONDS = 1;
        
        /** Fork callback function (called when fork detected) */
        std::function<void()> m_forkCallback;

    public:
        /** Constructor
         *
         *  @param nChannel Mining channel (1=Prime, 2=Hash, 0=Stake)
         *
         **/
        ChannelStateManager(uint32_t nChannel);
        
        /** Virtual destructor for polymorphism */
        virtual ~ChannelStateManager() = default;
        
        /** SyncWithBlockchain
         *
         *  Synchronize manager state with current blockchain state.
         *  
         *  This method:
         *  1. Loads current best block (atomic)
         *  2. Detects forks by comparing unified heights
         *  3. Updates channel state via GetLastState()
         *  4. Triggers fork callback if fork detected
         *  5. Caches state for performance
         *  
         *  FORK DETECTION:
         *  ==============
         *  A fork is detected when:
         *    current_unified_height < previous_unified_height
         *  
         *  This indicates a blockchain rollback/reorganization.
         *  
         *  When fork detected:
         *    - Sets m_fForkDetected = true
         *    - Calculates m_nBlocksRolledBack
         *    - Logs fork information
         *    - Triggers OnForkDetected() callback
         *  
         *  THREAD SAFETY:
         *  =============
         *  - Uses atomic operations for all state access
         *  - ChainState::tStateBest.load() is atomic
         *  - Cache updates protected by mutex
         *  - Safe to call from multiple threads
         *  
         *  @return true if sync succeeded, false on error
         *
         **/
        bool SyncWithBlockchain();
        
        /** ValidateTemplateHeights
         *
         *  Validate template heights using Block::Accept() logic.
         *  
         *  This method mirrors the height validation in Block::Accept():
         *    - statePrev.nHeight + 1 == nHeight  (unified)
         *    - statePrev.nChannelHeight + 1 == nChannelHeight  (channel)
         *  
         *  Templates are valid if:
         *    nUnified == blockchain_unified_height + 1
         *    nChannel == blockchain_channel_height + 1
         *  
         *  RATIONALE:
         *  =========
         *  Templates represent the NEXT block to mine.
         *  If blockchain is at height N, template should be for height N+1.
         *  
         *  FORK DETECTION:
         *  ==============
         *  If unified height doesn't match, either:
         *    - Blockchain advanced (template stale)
         *    - Fork occurred (unified height regressed)
         *  
         *  CHANNEL DETECTION:
         *  =================
         *  If channel height doesn't match:
         *    - Another block mined in this channel (template stale)
         *  
         *  @param nUnified Template unified height
         *  @param nChannel Template channel height
         *  @return true if heights valid, false if stale/fork
         *
         **/
        bool ValidateTemplateHeights(uint32_t nUnified, uint32_t nChannel);
        
        /** CheckDescendant
         *
         *  Validate template is descendant of hardened checkpoints.
         *  
         *  This method mirrors the checkpoint validation in Block::Accept():
         *    if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
         *        return error;
         *  
         *  Uses TAO::Ledger::IsDescendant() to check if previous block
         *  is a descendant of the last hardened checkpoint.
         *  
         *  WHEN TO CHECK:
         *  =============
         *  Only check when NOT synchronizing (same as Block::Accept).
         *  During initial sync, checkpoint checks are relaxed.
         *  
         *  @param hashPrevBlock Previous block hash from template
         *  @return true if valid descendant or synchronizing, false otherwise
         *
         **/
        bool CheckDescendant(const uint1024_t& hashPrevBlock);
        
        /** GetHeightInfo
         *
         *  Get comprehensive height information for diagnostics.
         *  
         *  Returns HeightInfo struct containing:
         *    - Current unified and channel heights
         *    - Expected next heights for templates
         *    - Fork detection status
         *    - Rollback count
         *    - Block hashes for validation
         *  
         *  Useful for:
         *    - Diagnostic logging
         *    - Template validation
         *    - Fork detection reporting
         *  
         *  @return HeightInfo struct with all height data
         *
         **/
        HeightInfo GetHeightInfo();
        
        /** GetUnifiedHeight
         *
         *  Get current unified blockchain height.
         *  
         *  @return Current unified height from last sync
         *
         **/
        uint32_t GetUnifiedHeight() const;
        
        /** GetChannelHeight
         *
         *  Get current channel-specific height.
         *  
         *  @return Current channel height from last sync
         *
         **/
        uint32_t GetChannelHeight() const;
        
        /** GetNextUnifiedHeight
         *
         *  Get expected next unified height for new block.
         *  
         *  @return Current unified height + 1
         *
         **/
        uint32_t GetNextUnifiedHeight() const;
        
        /** GetNextChannelHeight
         *
         *  Get expected next channel height for new block.
         *  
         *  @return Current channel height + 1
         *
         **/
        uint32_t GetNextChannelHeight() const;
        
        /** GetChannel
         *
         *  Get the channel this manager is tracking.
         *  
         *  @return Channel number (1=Prime, 2=Hash, 0=Stake)
         *
         **/
        uint32_t GetChannel() const;
        
        /** IsForkDetected
         *
         *  Check if a fork has been detected since last sync.
         *  
         *  @return true if fork detected, false otherwise
         *
         **/
        bool IsForkDetected() const;
        
        /** GetBlocksRolledBack
         *
         *  Get number of blocks rolled back in detected fork.
         *  
         *  @return Number of blocks rolled back (0 if no fork)
         *
         **/
        int32_t GetBlocksRolledBack() const;
        
        /** ClearForkFlag
         *
         *  Clear the fork detected flag after handling.
         *  
         *  Call this after processing fork (e.g., clearing templates).
         *
         **/
        void ClearForkFlag();
        
        /** SetForkCallback
         *
         *  Set callback function to be called when fork detected.
         *  
         *  Callback is invoked by OnForkDetected() when fork is detected
         *  during SyncWithBlockchain().
         *  
         *  Typical use: Clear template cache on fork.
         *  
         *  @param callback Function to call on fork (can be lambda)
         *
         **/
        void SetForkCallback(std::function<void()> callback);
        
        /** LogHeightInfo
         *
         *  Log comprehensive height information for diagnostics.
         *  
         *  Logs:
         *    - Channel name
         *    - Current unified and channel heights
         *    - Expected next heights
         *    - Fork detection status
         *    - Rollback information
         *  
         *  Useful for debugging template validation issues.
         *
         **/
        void LogHeightInfo();
        
        /** GetChannelName
         *
         *  Get human-readable channel name.
         *  
         *  @return "Prime", "Hash", or "Stake"
         *
         **/
        std::string GetChannelName() const;

    public:
        /** Fork detection flag - set when height regression detected **/
        std::atomic<bool> m_fForkDetected;
        
        /** OnForkDetected
         *
         *  Called when fork is detected during sync.
         *  
         *  Default implementation:
         *    - Logs fork information
         *    - Calls fork callback (if set)
         *  
         *  Derived classes can override for custom fork handling.
         *
         **/
        virtual void OnForkDetected();
        
        /** VerifyAllChannels
         *
         *  Static method to verify unified height equals sum of all channel heights.
         *  Uses existing PrimeStateManager, HashStateManager, and new StakeStateManager.
         *  
         *  Formula: nUnified = nStake + nPrime + nHash
         *  
         *  On mismatch, triggers fork callbacks on all channel managers.
         *  
         *  @param[in] nCheckInterval Only verify every N blocks (0 = always check)
         *  
         *  @return true if consistent or skipped, false if mismatch detected
         *
         **/
        static bool VerifyAllChannels(uint32_t nCheckInterval = 10);
        
        
        /** GetAllChannelHeights
         *
         *  Static method to get heights for all three channels using existing managers.
         *  
         *  @param[out] nStake Stake channel height (channel 0)
         *  @param[out] nPrime Prime channel height (channel 1)  
         *  @param[out] nHash Hash channel height (channel 2)
         *  @param[out] nUnified Current unified blockchain height
         *  
         *  @return true if all channels retrieved successfully
         *
         **/
        static bool GetAllChannelHeights(uint32_t& nStake, uint32_t& nPrime, uint32_t& nHash, uint32_t& nUnified);

    protected:
        
        /** IsCacheValid
         *
         *  Check if cached state is still valid.
         *  
         *  Cache expires after CACHE_VALIDITY_SECONDS (1 second).
         *  
         *  @return true if cache valid, false if expired
         *
         **/
        bool IsCacheValid() const;
        
        /** UpdateCache
         *
         *  Update cached state from blockchain.
         *  
         *  @param state Current blockchain state
         *
         **/
        void UpdateCache(const TAO::Ledger::BlockState& state);
    };


    /** PrimeStateManager
     *
     *  Channel state manager for Prime mining (channel 1).
     *  
     *  Prime channel uses CPU mining via Fermat prime cluster discovery.
     *  
     *  Usage:
     *    PrimeStateManager primeMgr;
     *    primeMgr.SyncWithBlockchain();
     *    uint32_t nChannelHeight = primeMgr.GetChannelHeight();
     *  
     **/
    class PrimeStateManager : public ChannelStateManager
    {
    public:
        /** Constructor - initializes for Prime channel (1) */
        PrimeStateManager();
        
        /** Virtual destructor */
        virtual ~PrimeStateManager() = default;
    };


    /** HashStateManager
     *
     *  Channel state manager for Hash mining (channel 2).
     *  
     *  Hash channel uses GPU/FPGA mining via SHA3 hashing.
     *  
     *  Usage:
     *    HashStateManager hashMgr;
     *    hashMgr.SyncWithBlockchain();
     *    uint32_t nChannelHeight = hashMgr.GetChannelHeight();
     *  
     **/
    class HashStateManager : public ChannelStateManager
    {
    public:
        /** Constructor - initializes for Hash channel (2) */
        HashStateManager();
        
        /** Virtual destructor */
        virtual ~HashStateManager() = default;
    };


    /** StakeStateManager
     *
     *  Specialized ChannelStateManager for Stake channel (channel 0).
     *  Completes the set: Stake(0), Prime(1), Hash(2).
     *
     **/
    class StakeStateManager : public ChannelStateManager
    {
    public:
        /** Default constructor - initializes for Stake channel (0) **/
        StakeStateManager();
        
        /** Virtual destructor */
        virtual ~StakeStateManager() = default;
    };

} // namespace LLP

#endif // NEXUS_LLP_INCLUDE_CHANNEL_STATE_MANAGER_H
