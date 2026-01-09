/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <LLP/include/version.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

#include <sstream>
#include <mutex>
#include <atomic>
#include <chrono>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    /* Create wallet-signed block for stateless mining */
    TritiumBlock* CreateBlockForStatelessMining(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress)
    {
        /* Validate input nChannel parameter (defense in depth) */
        if(nChannel == 0)
        {
            debug::error(FUNCTION, "❌ Invalid input: nChannel is 0");
            debug::error(FUNCTION, "   Caller must provide valid channel (1=Prime, 2=Hash)");
            return nullptr;
        }
        
        if(nChannel != 1 && nChannel != 2)
        {
            debug::error(FUNCTION, "❌ Invalid input: nChannel = ", nChannel);
            debug::error(FUNCTION, "   Valid channels: 1 (Prime), 2 (Hash)");
            return nullptr;
        }
        
        /* All blocks MUST be wallet-signed per Nexus consensus */
        if (!TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
        {
            debug::error(FUNCTION, "Mining not unlocked - use -unlock=mining or -autologin=username:password");
            debug::error(FUNCTION, "CRITICAL: Nexus consensus requires wallet-signed blocks");
            debug::error(FUNCTION, "Falcon authentication is for miner sessions, NOT block signing");
            return nullptr;
        }
        
        debug::log(1, FUNCTION, "Creating wallet-signed block (Nexus consensus requirement)");
        
        try {
            const uint256_t hashSession = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
            const auto& pCredentials = TAO::API::Authentication::Credentials(hashSession);
            
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));
            
            /* Get current chain state (SAME as normal node does) */
            const BlockState statePrev = ChainState::tStateBest.load();
            const uint32_t nChainHeight = ChainState::nBestHeight.load();
            
            /* ✅ ADD: Diagnostic logging */
            debug::log(0, FUNCTION, "=== CHAIN STATE DIAGNOSTIC ===");
            debug::log(0, FUNCTION, "  ChainState::nBestHeight: ", nChainHeight);
            debug::log(0, FUNCTION, "  statePrev.nHeight: ", statePrev.nHeight);
            debug::log(0, FUNCTION, "  statePrev.GetHash(): ", statePrev.GetHash().SubString());
            debug::log(0, FUNCTION, "  Synchronizing: ", ChainState::Synchronizing() ? "YES" : "NO");
            debug::log(0, FUNCTION, "  Template will be for height: ", statePrev.nHeight + 1);
            
            /* Verify chain state is valid before proceeding */
            if(!statePrev || statePrev.GetHash() == 0)
            {
                debug::error(FUNCTION, "Chain state not initialized - cannot create block template");
                debug::error(FUNCTION, "  Node may still be starting up or synchronizing");
                return nullptr;
            }
            
            /* Don't create blocks while synchronizing */
            if(ChainState::Synchronizing())
            {
                debug::error(FUNCTION, "Cannot create block templates while synchronizing");
                return nullptr;
            }
            
            /* ✅ ADD: Validate consistency */
            if(statePrev.nHeight != nChainHeight)
            {
                debug::error(FUNCTION, "❌ Chain state inconsistency detected!");
                debug::error(FUNCTION, "   statePrev.nHeight: ", statePrev.nHeight);
                debug::error(FUNCTION, "   nBestHeight: ", nChainHeight);
                debug::error(FUNCTION, "   This indicates a race condition or chain state corruption");
                return nullptr;
            }
            
            TritiumBlock* pBlock = new TritiumBlock();
            
            /* Initialize block with proper chain context BEFORE CreateBlock()
             * This ensures CreateBlock() has the correct context to populate the producer transaction.
             * Without this, the block would have default-initialized values (zeros), causing validation failures.
             * This matches the flow in normal nodes: AddBlockData() sets these fields. */
            pBlock->hashPrevBlock = statePrev.GetHash();
            pBlock->nChannel = nChannel;
            
            /* CRITICAL: Use channel-specific height, NOT unified height
             * 
             * For multi-channel mining:
             * - Unified height = 6.5M (total across all channels)
             * - Prime channel = 2.3M blocks
             * - Hash channel = 2.1M blocks  
             * - Stake channel = 2.1M blocks
             * 
             * The miner needs the CHANNEL height to:
             * 1. Know which block they're mining (Prime block 2302585, not unified 6537246)
             * 2. Compare with GET_ROUND response to detect staleness
             * 3. Set proper hashPrevBlock for the channel
             */
            BlockState stateChannel = statePrev;
            if(GetLastState(stateChannel, nChannel))
            {
                /* Template is for the NEXT block in this channel */
                pBlock->nHeight = stateChannel.nChannelHeight + 1;
                
                debug::log(2, FUNCTION, "✓ Creating template for channel ", static_cast<uint32_t>(nChannel),
                           " at channel height ", pBlock->nHeight);
                debug::log(2, FUNCTION, "   (Unified height: ", statePrev.nHeight, " - for reference only)");
            }
            else
            {
                /* Channel doesn't exist yet - mining first block in this channel
                 * GetLastState returns false and sets stateChannel to genesis (nChannelHeight = 1)
                 * So first block in channel has nChannelHeight = 2 */
                pBlock->nHeight = stateChannel.nChannelHeight + 1;
                debug::log(2, FUNCTION, "Mining first block in channel ", static_cast<uint32_t>(nChannel),
                           " at channel height ", pBlock->nHeight);
                debug::log(2, FUNCTION, "   (Genesis has nChannelHeight = ", stateChannel.nChannelHeight, ")");
            }
            
            /* Verify nChannel was set correctly */
            debug::log(2, FUNCTION, "✓ Block nChannel set to: ", pBlock->nChannel, 
                       " (", (nChannel == 1 ? "Prime" : nChannel == 2 ? "Hash" : "INVALID"), ")");
            
            if(pBlock->nChannel == 0)
            {
                debug::error(FUNCTION, "❌ CRITICAL: nChannel is 0 after assignment!");
                debug::error(FUNCTION, "   Input nChannel parameter: ", nChannel);
                debug::error(FUNCTION, "   This should never happen - investigate immediately");
                delete pBlock;
                return nullptr;
            }
            
            pBlock->nBits = GetNextTargetRequired(statePrev, nChannel, false);
            pBlock->nTime = std::max(
                statePrev.GetBlockTime() + 1, 
                runtime::unifiedtimestamp()
            );
            
            debug::log(2, FUNCTION, "Block initialized with chain state:");
            debug::log(2, FUNCTION, "  hashPrevBlock: ", pBlock->hashPrevBlock.SubString());
            debug::log(2, FUNCTION, "  nHeight (channel): ", pBlock->nHeight, " ← This is what miner sees");
            debug::log(2, FUNCTION, "  nChannel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  Unified height: ", statePrev.nHeight, " (reference only)");
            debug::log(2, FUNCTION, "  nBits: 0x", std::hex, pBlock->nBits, std::dec);
            debug::log(2, FUNCTION, "  nTime: ", pBlock->nTime);
            
            // CreateBlock() handles wallet signing per consensus requirements
            bool success = CreateBlock(
                pCredentials,
                strPIN,
                nChannel,
                *pBlock,
                nExtraNonce,
                nullptr,           // No coinbase recipients
                hashRewardAddress  // Route rewards to miner's address
            );
            
            if (!success) {
                delete pBlock;
                debug::error(FUNCTION, "CreateBlock failed");
                return nullptr;
            }
            
            /* DO NOT call Check() here - the block hasn't been mined yet.
             * Check() validates PoW which requires a valid nonce from the miner.
             * Validation happens in validate_block() AFTER miner submits solution. */
            
            /* Basic sanity check only - verify CreateBlock() produced valid output */
            if(pBlock->hashMerkleRoot == 0)
            {
                debug::error(FUNCTION, "CreateBlock() produced invalid merkle root");
                delete pBlock;
                return nullptr;
            }
            
            debug::log(2, FUNCTION, "✓ Wallet-signed block created successfully");
            debug::log(2, FUNCTION, "  Note: PoW validation deferred until miner submits nonce");
            debug::log(2, FUNCTION, "  Channel height: ", pBlock->nHeight, " ← This is what miner sees");
            debug::log(2, FUNCTION, "  Channel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  Unified height: ", statePrev.nHeight, " (reference only)");
            debug::log(2, FUNCTION, "  Reward address: ", hashRewardAddress.SubString());
            
            return pBlock;
        }
        catch (const std::exception& e) {
            debug::error(FUNCTION, "Block creation failed: ", e.what());
            return nullptr;
        }
    }


    /* ==================== TEMPLATE CACHE IMPLEMENTATION ==================== */

    using namespace TemplateConstants;
    
    /** Internal cache structure **/
    struct CachedBlockTemplate
    {
        std::vector<uint8_t> vSerializedBlock;
        uint32_t nUnifiedHeight;
        uint32_t nChannelHeight;
        uint32_t nChannel;
        uint1024_t hashPrevBlock;
        uint32_t nDifficulty;
        uint64_t nCreationTime;
        uint64_t nCreationDurationUs;
        bool fValid;
        mutable std::atomic<uint64_t> nServeCount;
        
        CachedBlockTemplate()
            : nUnifiedHeight(0), nChannelHeight(0), nChannel(0)
            , hashPrevBlock(0), nDifficulty(0)
            , nCreationTime(0), nCreationDurationUs(0)
            , fValid(false), nServeCount(0)
        {
            vSerializedBlock.reserve(TRITIUM_BLOCK_TEMPLATE_SIZE);
        }
        
        bool IsStale() const
        {
            if (!fValid) return true;
            uint64_t nAge = runtime::unifiedtimestamp() - nCreationTime;
            return (nAge > TEMPLATE_CACHE_MAX_AGE_SECONDS);
        }
        
        std::string ToString() const
        {
            std::stringstream ss;
            ss << "CachedTemplate(";
            ss << "channel=" << (nChannel == Channels::PRIME ? "Prime" : "Hash");
            ss << ", unified=" << nUnifiedHeight;
            ss << ", height=" << nChannelHeight;
            ss << ", age=" << (runtime::unifiedtimestamp() - nCreationTime) << "s";
            ss << ", served=" << nServeCount.load();
            ss << ", valid=" << (fValid ? "yes" : "no");
            ss << ")";
            return ss.str();
        }
    };
    
    /** Module-level cache storage **/
    namespace {
        CachedBlockTemplate g_primeCache;
        CachedBlockTemplate g_hashCache;
        std::mutex g_primeCacheMutex;
        std::mutex g_hashCacheMutex;
        
        std::atomic<uint64_t> g_nTotalCacheCreations{0};
        std::atomic<uint64_t> g_nTotalCacheServes{0};
        std::atomic<uint64_t> g_nTotalCacheHits{0};
        std::atomic<uint64_t> g_nTotalCacheMisses{0};
    }
    
    /** CreateTemplateCache **/
    bool CreateTemplateCache(uint32_t nChannel)
    {
        if (nChannel != Channels::PRIME && nChannel != Channels::HASH)
        {
            debug::error(FUNCTION, "Invalid channel: ", nChannel);
            return false;
        }
        
        std::string channel_name = (nChannel == Channels::PRIME) ? "Prime" : "Hash";
        InvalidateTemplateCache(nChannel);
        
        std::mutex& cacheMutex = (nChannel == Channels::PRIME) ? g_primeCacheMutex : g_hashCacheMutex;
        CachedBlockTemplate& cache = (nChannel == Channels::PRIME) ? g_primeCache : g_hashCache;
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        BlockState stateBest = ChainState::tStateBest.load();
        BlockState stateChannel = stateBest;
        
        if (!GetLastState(stateChannel, nChannel))
        {
            debug::error(FUNCTION, "Failed to get state for channel ", nChannel);
            return false;
        }
        
        /* Use existing CreateBlockForStatelessMining() */
        uint256_t dummyRewardAddress;
        TritiumBlock* pBlock = CreateBlockForStatelessMining(nChannel, 0, dummyRewardAddress);
        
        if (!pBlock)
        {
            debug::error(FUNCTION, "Failed to create block for channel ", nChannel);
            return false;
        }
        
        /* Serialize template */
        DataStream ssBlock(SER_NETWORK, LLP::PROTOCOL_VERSION);
        ssBlock << *pBlock;
        std::vector<uint8_t> vSerialized(ssBlock.begin(), ssBlock.end());
        
        uint32_t nDifficulty = GetNextTargetRequired(stateBest, nChannel, false);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        /* Populate cache */
        cache.vSerializedBlock = std::move(vSerialized);
        cache.nUnifiedHeight = stateBest.nHeight;
        cache.nChannelHeight = stateChannel.nChannelHeight + 1;
        cache.nChannel = nChannel;
        cache.hashPrevBlock = pBlock->hashPrevBlock;
        cache.nDifficulty = nDifficulty;
        cache.nCreationTime = runtime::unifiedtimestamp();
        cache.nCreationDurationUs = duration.count();
        cache.fValid = true;
        cache.nServeCount.store(0);
        
        delete pBlock;
        
        if (ENABLE_TEMPLATE_CACHE_STATISTICS)
            g_nTotalCacheCreations++;
        
        debug::log(CACHE_CREATION_LOG_LEVEL, FUNCTION, "✓ ", channel_name, " template cached:");
        debug::log(CACHE_CREATION_LOG_LEVEL, FUNCTION, "  Height:    ", cache.nChannelHeight);
        debug::log(CACHE_CREATION_LOG_LEVEL, FUNCTION, "  Size:      ", cache.vSerializedBlock.size(), " bytes");
        debug::log(CACHE_CREATION_LOG_LEVEL, FUNCTION, "  Created:   ", duration.count(), " μs");
        
        if (duration.count() / 1000 > TEMPLATE_CREATION_WARN_THRESHOLD_MS)
            debug::warning(FUNCTION, "⚠️  Template creation slow: ", duration.count() / 1000, " ms");
        
        return true;
    }
    
    /** GetCachedTemplate **/
    bool GetCachedTemplate(uint32_t nChannel, TritiumBlock& block)
    {
        std::vector<uint8_t> vTemplate;
        if (!GetCachedTemplateSerialized(nChannel, vTemplate))
            return false;
        
        try {
            DataStream ssBlock(vTemplate, SER_NETWORK, LLP::PROTOCOL_VERSION);
            ssBlock >> block;
            return true;
        }
        catch (const std::exception& e) {
            debug::error(FUNCTION, "Failed to deserialize: ", e.what());
            return false;
        }
    }
    
    /** GetCachedTemplateSerialized **/
    bool GetCachedTemplateSerialized(uint32_t nChannel, std::vector<uint8_t>& vTemplate)
    {
        if (nChannel != Channels::PRIME && nChannel != Channels::HASH)
            return false;
        
        std::mutex& cacheMutex = (nChannel == Channels::PRIME) ? g_primeCacheMutex : g_hashCacheMutex;
        const CachedBlockTemplate& cache = (nChannel == Channels::PRIME) ? g_primeCache : g_hashCache;
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        if (!cache.fValid || cache.IsStale())
        {
            if (ENABLE_TEMPLATE_CACHE_STATISTICS)
                g_nTotalCacheMisses++;
            return false;
        }
        
        vTemplate = cache.vSerializedBlock;
        
        if (ENABLE_TEMPLATE_CACHE_STATISTICS)
        {
            g_nTotalCacheHits++;
            g_nTotalCacheServes++;
            cache.nServeCount++;
        }
        
        debug::log(CACHE_HIT_LOG_LEVEL, FUNCTION, "✓ Cache hit: ", 
                  (nChannel == Channels::PRIME ? "Prime" : "Hash"));
        
        return true;
    }
    
    /** InvalidateTemplateCache **/
    void InvalidateTemplateCache(uint32_t nChannel)
    {
        if (nChannel != Channels::PRIME && nChannel != Channels::HASH)
            return;
        
        std::mutex& cacheMutex = (nChannel == Channels::PRIME) ? g_primeCacheMutex : g_hashCacheMutex;
        CachedBlockTemplate& cache = (nChannel == Channels::PRIME) ? g_primeCache : g_hashCache;
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        if (cache.fValid && ENABLE_TEMPLATE_CACHE_STATISTICS)
        {
            debug::log(CACHE_STATISTICS_LOG_LEVEL, FUNCTION, "📊 Cache stats: served ", cache.nServeCount.load());
        }
        
        cache.fValid = false;
    }
    
    /** IsTemplateCacheFresh **/
    bool IsTemplateCacheFresh(uint32_t nChannel)
    {
        if (nChannel != Channels::PRIME && nChannel != Channels::HASH)
            return false;
        
        std::mutex& cacheMutex = (nChannel == Channels::PRIME) ? g_primeCacheMutex : g_hashCacheMutex;
        const CachedBlockTemplate& cache = (nChannel == Channels::PRIME) ? g_primeCache : g_hashCache;
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        return (cache.fValid && !cache.IsStale());
    }
    
    /** GetTemplateCacheStatistics **/
    std::string GetTemplateCacheStatistics()
    {
        if (!ENABLE_TEMPLATE_CACHE_STATISTICS)
            return "Statistics disabled";
        
        std::lock_guard<std::mutex> lockPrime(g_primeCacheMutex);
        std::lock_guard<std::mutex> lockHash(g_hashCacheMutex);
        
        std::stringstream ss;
        ss << "\n══════════════════════════════════════════════════\n";
        ss << "  TEMPLATE CACHE STATISTICS\n";
        ss << "══════════════════════════════════════════════════\n";
        ss << "Caches Created:   " << g_nTotalCacheCreations.load() << "\n";
        ss << "Templates Served: " << g_nTotalCacheServes.load() << "\n";
        ss << "Cache Hits:       " << g_nTotalCacheHits.load() << "\n";
        ss << "Cache Misses:     " << g_nTotalCacheMisses.load() << "\n";
        ss << "Hit Rate:         " << (GetTemplateCacheEfficiency() * 100.0) << "%\n\n";
        ss << "PRIME:  " << g_primeCache.ToString() << "\n";
        ss << "HASH:   " << g_hashCache.ToString() << "\n";
        ss << "══════════════════════════════════════════════════\n";
        
        return ss.str();
    }
    
    /** GetTemplateCacheEfficiency **/
    double GetTemplateCacheEfficiency()
    {
        if (!ENABLE_TEMPLATE_CACHE_STATISTICS)
            return 0.0;
        
        uint64_t nHits = g_nTotalCacheHits.load();
        uint64_t nMisses = g_nTotalCacheMisses.load();
        uint64_t nTotal = nHits + nMisses;
        
        return (nTotal > 0) ? (static_cast<double>(nHits) / nTotal) : 0.0;
    }

}
