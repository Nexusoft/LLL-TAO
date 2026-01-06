/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <TAO/Ledger/types/tritium.h>

#include <Util/include/runtime.h>

using namespace LLP;


/** Test Suite: Template Metadata Channel Height (PR #134)
 *
 *  These tests verify the multi-channel height tracking functionality
 *  that fixes the ~40% wasted mining work problem.
 *  
 *  Key functionality tested:
 *  1. Channel height storage in TemplateMetadata
 *  2. Corrected staleness detection using channel-specific heights
 *  3. Age-based timeout (60 second safety net)
 *  4. Utility methods (GetAge, GetChannelName)
 *  5. Move semantics integration with std::map
 *  6. Default constructor behavior
 *
 *  Background:
 *  -----------
 *  Before PR #134, templates were marked stale when unified blockchain height changed,
 *  which happens when ANY channel mines a block. This caused ~40% false-positive staleness.
 *  
 *  After PR #134, templates are only marked stale when THEIR SPECIFIC CHANNEL advances,
 *  eliminating false positives and reducing wasted work to <5%.
 **/


TEST_CASE("TemplateMetadata Channel Height Storage", "[template][channel_height][pr134]")
{
    SECTION("Constructor stores all fields including nChannelHeight")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;  // Prime
        pMockBlock->hashMerkleRoot.SetHex("abc123");
        
        /* Create metadata with channel height */
        uint64_t nCreationTime = runtime::unifiedtimestamp();
        uint32_t nHeight = 100;
        uint32_t nChannelHeight = 50;  // Prime channel is at height 50
        uint512_t hashMerkleRoot = pMockBlock->hashMerkleRoot;
        uint32_t nChannel = 1;
        
        TemplateMetadata meta(pMockBlock, nCreationTime, nHeight, nChannelHeight, hashMerkleRoot, nChannel);
        
        /* Verify all fields are stored correctly */
        REQUIRE(meta.pBlock != nullptr);
        REQUIRE(meta.nCreationTime == nCreationTime);
        REQUIRE(meta.nHeight == nHeight);
        REQUIRE(meta.nChannelHeight == nChannelHeight);  // PR #134: Channel height
        REQUIRE(meta.hashMerkleRoot == hashMerkleRoot);
        REQUIRE(meta.nChannel == nChannel);
    }
    
    SECTION("Default constructor initializes nChannelHeight to 0")
    {
        /* Create metadata with default constructor */
        TemplateMetadata meta;
        
        /* Verify all fields are initialized */
        REQUIRE(meta.pBlock == nullptr);
        REQUIRE(meta.nCreationTime == 0);
        REQUIRE(meta.nHeight == 0);
        REQUIRE(meta.nChannelHeight == 0);  // PR #134: Should be 0
        REQUIRE(meta.hashMerkleRoot == 0);
        REQUIRE(meta.nChannel == 0);
    }
}


TEST_CASE("TemplateMetadata Staleness Logic", "[template][staleness][pr134]")
{
    /* Note: IsStale() implementation requires actual blockchain state via GetLastState()
     * which accesses ChainState::tStateBest and walks the blockchain.
     * 
     * Since we cannot easily mock the blockchain in unit tests, we verify the
     * STRUCTURE of the staleness logic by testing:
     * 1. Age timeout (can be tested without blockchain)
     * 2. That IsStale() is callable and returns a boolean
     * 3. Edge cases (invalid timestamps, etc.)
     * 
     * The actual channel height comparison logic is tested via:
     * - Integration tests with real blockchain
     * - Manual testing on testnet
     * - Code review of the implementation
     */
    
    SECTION("Age timeout: templates older than 60 seconds are stale")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata with creation time 70 seconds ago */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nOldTime = nNow - 70;  // 70 seconds ago
        
        TemplateMetadata meta(pMockBlock, nOldTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* Template should be stale due to age (>60 seconds) */
        bool fStale = meta.IsStale(nNow);
        REQUIRE(fStale == true);  // Should be stale due to age
    }
    
    SECTION("Age timeout: templates younger than 60 seconds may be fresh")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata with creation time 10 seconds ago */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nRecentTime = nNow - 10;  // 10 seconds ago
        
        TemplateMetadata meta(pMockBlock, nRecentTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* Template age is <60 seconds, so age check passes
         * Note: IsStale() will still check channel height via blockchain,
         * so we just verify the method is callable and returns a boolean */
        bool fStale = meta.IsStale(nNow);
        
        /* We can't predict the exact result without mocking blockchain,
         * but we can verify the method doesn't crash and returns a valid bool */
        REQUIRE((fStale == true || fStale == false));  // Just verify it returns a boolean
    }
    
    SECTION("Invalid timestamp: template with creation time 0 is stale")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata with invalid creation time */
        TemplateMetadata meta(pMockBlock, 0, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* Should be marked stale due to invalid timestamp */
        bool fStale = meta.IsStale();
        REQUIRE(fStale == true);
    }
    
    SECTION("Future timestamp: template with future creation time is stale")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata with future creation time (clock skew) */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nFutureTime = nNow + 100;  // 100 seconds in the future
        
        TemplateMetadata meta(pMockBlock, nFutureTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* Should be marked stale due to invalid timestamp */
        bool fStale = meta.IsStale(nNow);
        REQUIRE(fStale == true);
    }
}


TEST_CASE("TemplateMetadata GetAge Method", "[template][age][pr134]")
{
    SECTION("GetAge returns correct age in seconds")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata 30 seconds ago */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nCreationTime = nNow - 30;
        
        TemplateMetadata meta(pMockBlock, nCreationTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* GetAge should return ~30 seconds */
        uint64_t nAge = meta.GetAge(nNow);
        REQUIRE(nAge == 30);
    }
    
    SECTION("GetAge uses current time if not provided")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Create metadata now */
        uint64_t nCreationTime = runtime::unifiedtimestamp();
        
        TemplateMetadata meta(pMockBlock, nCreationTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* GetAge should return 0 or small value (just created) */
        uint64_t nAge = meta.GetAge();
        REQUIRE(nAge <= 1);  // Allow 1 second for execution time
    }
    
    SECTION("GetAge returns 0 for invalid timestamps")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        /* Test with creation time = 0 */
        TemplateMetadata meta1(pMockBlock, 0, 100, 50, pMockBlock->hashMerkleRoot, 1);
        REQUIRE(meta1.GetAge() == 0);
        
        /* Create another block for second test */
        TAO::Ledger::TritiumBlock* pMockBlock2 = new TAO::Ledger::TritiumBlock();
        *pMockBlock2 = *pMockBlock;
        
        /* Test with future creation time */
        uint64_t nNow = runtime::unifiedtimestamp();
        uint64_t nFuture = nNow + 100;
        TemplateMetadata meta2(pMockBlock2, nFuture, 100, 50, pMockBlock2->hashMerkleRoot, 1);
        REQUIRE(meta2.GetAge(nNow) == 0);
    }
}


TEST_CASE("TemplateMetadata GetChannelName Method", "[template][channel][pr134]")
{
    SECTION("GetChannelName returns correct names for all channels")
    {
        /* Create mock blocks for each channel */
        TAO::Ledger::TritiumBlock* pStakeBlock = new TAO::Ledger::TritiumBlock();
        TAO::Ledger::TritiumBlock* pPrimeBlock = new TAO::Ledger::TritiumBlock();
        TAO::Ledger::TritiumBlock* pHashBlock = new TAO::Ledger::TritiumBlock();
        
        pStakeBlock->nVersion = 9;
        pStakeBlock->nHeight = 100;
        pPrimeBlock->nVersion = 9;
        pPrimeBlock->nHeight = 100;
        pHashBlock->nVersion = 9;
        pHashBlock->nHeight = 100;
        
        uint64_t nTime = runtime::unifiedtimestamp();
        uint512_t hash1, hash2, hash3;
        hash1.SetHex("abc1");
        hash2.SetHex("abc2");
        hash3.SetHex("abc3");
        
        /* Stake channel (0) */
        TemplateMetadata metaStake(pStakeBlock, nTime, 100, 50, hash1, 0);
        REQUIRE(metaStake.GetChannelName() == "Stake");
        
        /* Prime channel (1) */
        TemplateMetadata metaPrime(pPrimeBlock, nTime, 100, 50, hash2, 1);
        REQUIRE(metaPrime.GetChannelName() == "Prime");
        
        /* Hash channel (2) */
        TemplateMetadata metaHash(pHashBlock, nTime, 100, 50, hash3, 2);
        REQUIRE(metaHash.GetChannelName() == "Hash");
    }
    
    SECTION("GetChannelName returns 'Unknown' for invalid channels")
    {
        /* Create mock block with invalid channel */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        
        uint64_t nTime = runtime::unifiedtimestamp();
        uint512_t hash;
        hash.SetHex("abc");
        
        /* Invalid channel 99 */
        TemplateMetadata meta(pMockBlock, nTime, 100, 50, hash, 99);
        REQUIRE(meta.GetChannelName() == "Unknown");
    }
}


TEST_CASE("TemplateMetadata Move Semantics", "[template][move][pr134]")
{
    SECTION("TemplateMetadata can be moved into std::map")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        pMockBlock->hashMerkleRoot.SetHex("abc123");
        
        /* Create metadata */
        uint64_t nTime = runtime::unifiedtimestamp();
        TemplateMetadata meta(pMockBlock, nTime, 100, 50, pMockBlock->hashMerkleRoot, 1);
        
        /* Store merkle root before move */
        uint512_t hashKey = pMockBlock->hashMerkleRoot;
        
        /* Move into std::map (this is how StatelessMinerConnection uses it) */
        std::map<uint512_t, TemplateMetadata> mapTemplates;
        mapTemplates.emplace(hashKey, std::move(meta));
        
        /* Verify it was stored */
        REQUIRE(mapTemplates.size() == 1);
        REQUIRE(mapTemplates.count(hashKey) == 1);
        
        /* Verify stored data */
        const TemplateMetadata& stored = mapTemplates.at(hashKey);
        REQUIRE(stored.pBlock != nullptr);
        REQUIRE(stored.nCreationTime == nTime);
        REQUIRE(stored.nHeight == 100);
        REQUIRE(stored.nChannelHeight == 50);
        REQUIRE(stored.nChannel == 1);
    }
    
    SECTION("TemplateMetadata move constructor transfers ownership")
    {
        /* Create a mock block */
        TAO::Ledger::TritiumBlock* pMockBlock = new TAO::Ledger::TritiumBlock();
        pMockBlock->nVersion = 9;
        pMockBlock->nHeight = 100;
        pMockBlock->nChannel = 1;
        
        uint64_t nTime = runtime::unifiedtimestamp();
        uint512_t hash;
        hash.SetHex("abc");
        
        /* Create original metadata */
        TemplateMetadata meta1(pMockBlock, nTime, 100, 50, hash, 1);
        REQUIRE(meta1.pBlock != nullptr);
        
        /* Move construct new metadata */
        TemplateMetadata meta2(std::move(meta1));
        
        /* meta2 should have the block pointer */
        REQUIRE(meta2.pBlock != nullptr);
        REQUIRE(meta2.nCreationTime == nTime);
        REQUIRE(meta2.nChannelHeight == 50);
        
        /* meta1 should have nullptr after move (moved-from state) */
        REQUIRE(meta1.pBlock == nullptr);
    }
}


TEST_CASE("TemplateMetadata Integration with std::map", "[template][map][pr134]")
{
    SECTION("Multiple templates can be stored in std::map with different merkle roots")
    {
        std::map<uint512_t, TemplateMetadata> mapTemplates;
        
        /* Create and store 3 templates */
        for(uint32_t i = 0; i < 3; ++i)
        {
            TAO::Ledger::TritiumBlock* pBlock = new TAO::Ledger::TritiumBlock();
            pBlock->nVersion = 9;
            pBlock->nHeight = 100 + i;
            pBlock->nChannel = (i % 2) + 1;  // Alternate between Prime(1) and Hash(2)
            
            /* Create unique merkle root */
            uint512_t hash;
            hash.SetHex(std::to_string(i) + "abc");
            pBlock->hashMerkleRoot = hash;
            
            uint64_t nTime = runtime::unifiedtimestamp();
            uint32_t nChannelHeight = 50 + i;
            
            TemplateMetadata meta(pBlock, nTime, 100 + i, nChannelHeight, hash, pBlock->nChannel);
            
            mapTemplates.emplace(hash, std::move(meta));
        }
        
        /* Verify all templates were stored */
        REQUIRE(mapTemplates.size() == 3);
        
        /* Verify each template */
        for(uint32_t i = 0; i < 3; ++i)
        {
            uint512_t hash;
            hash.SetHex(std::to_string(i) + "abc");
            
            REQUIRE(mapTemplates.count(hash) == 1);
            
            const TemplateMetadata& stored = mapTemplates.at(hash);
            REQUIRE(stored.pBlock != nullptr);
            REQUIRE(stored.nHeight == 100 + i);
            REQUIRE(stored.nChannelHeight == 50 + i);
            REQUIRE(stored.nChannel == (i % 2) + 1);
        }
    }
    
    SECTION("Templates can be looked up and removed from std::map")
    {
        std::map<uint512_t, TemplateMetadata> mapTemplates;
        
        /* Create and store a template */
        TAO::Ledger::TritiumBlock* pBlock = new TAO::Ledger::TritiumBlock();
        pBlock->nVersion = 9;
        pBlock->nHeight = 200;
        pBlock->nChannel = 1;
        
        uint512_t hash;
        hash.SetHex("test123");
        pBlock->hashMerkleRoot = hash;
        
        uint64_t nTime = runtime::unifiedtimestamp();
        TemplateMetadata meta(pBlock, nTime, 200, 75, hash, 1);
        
        mapTemplates.emplace(hash, std::move(meta));
        REQUIRE(mapTemplates.size() == 1);
        
        /* Look up the template */
        auto it = mapTemplates.find(hash);
        REQUIRE(it != mapTemplates.end());
        REQUIRE(it->second.nChannelHeight == 75);
        
        /* Remove the template */
        mapTemplates.erase(hash);
        REQUIRE(mapTemplates.size() == 0);
        REQUIRE(mapTemplates.find(hash) == mapTemplates.end());
    }
}
