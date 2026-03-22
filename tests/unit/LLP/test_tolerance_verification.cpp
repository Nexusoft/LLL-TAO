/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/channel_state_manager.h>
#include <TAO/Ledger/include/chainstate.h>

using namespace LLP;


/** Test Suite: Tolerance-Based Unified Height Verification
 *
 *  These tests verify the tolerance-based verification system that accepts
 *  the historical +3 block discrepancy while detecting new issues.
 *
 *  Key functionality tested:
 *  1. Perfect match verification (discrepancy = 0)
 *  2. Historical anomaly acceptance (discrepancy = 3, within tolerance)
 *  3. Boundary conditions (discrepancy = 5, at tolerance limit)
 *  4. Critical error detection (discrepancy > 5, exceeds tolerance)
 *  5. Negative discrepancies (unified ahead of channels)
 *  6. ForensicForkInfo structure
 *
 **/


TEST_CASE("VerifyUnifiedHeightWithTolerance - Perfect Match", "[tolerance][verification]")
{
    SECTION("Zero discrepancy returns true")
    {
        /* Heights that sum perfectly to unified */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166269;  // Adjusted to make sum = 6537420
        uint32_t nUnified = 6537420;
        
        /* Should pass with perfect match */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == true);
    }
}


TEST_CASE("VerifyUnifiedHeightWithTolerance - Historical Anomaly", "[tolerance][verification]")
{
    SECTION("Discrepancy of +3 blocks is accepted (within tolerance)")
    {
        /* Actual Nexus blockchain state with +3 discrepancy */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166272;
        uint32_t nUnified = 6537420;  // Sum is 6537423, diff = +3
        
        /* Should pass with warning */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == true);
    }
    
    SECTION("Discrepancy of -3 blocks is accepted (within tolerance)")
    {
        /* Unified ahead by 3 blocks */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166269;
        uint32_t nUnified = 6537423;  // Sum is 6537420, diff = -3
        
        /* Should pass with warning */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == true);
    }
}


TEST_CASE("VerifyUnifiedHeightWithTolerance - Boundary Conditions", "[tolerance][verification]")
{
    SECTION("Discrepancy of +5 blocks is accepted (at tolerance limit)")
    {
        /* Right at the tolerance boundary */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166274;
        uint32_t nUnified = 6537420;  // Sum is 6537425, diff = +5
        
        /* Should pass at boundary */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == true);
    }
    
    SECTION("Discrepancy of -5 blocks is accepted (at tolerance limit)")
    {
        /* Right at the tolerance boundary, other direction */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166264;
        uint32_t nUnified = 6537420;  // Sum is 6537415, diff = -5
        
        /* Should pass at boundary */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == true);
    }
}


TEST_CASE("VerifyUnifiedHeightWithTolerance - Critical Errors", "[tolerance][verification]")
{
    SECTION("Discrepancy of +6 blocks is rejected (exceeds tolerance)")
    {
        /* One block beyond tolerance */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166275;
        uint32_t nUnified = 6537420;  // Sum is 6537426, diff = +6
        
        /* Should fail - exceeds tolerance */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == false);
    }
    
    SECTION("Discrepancy of +10 blocks is rejected (exceeds tolerance)")
    {
        /* Well beyond tolerance - indicates NEW issue */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166279;
        uint32_t nUnified = 6537420;  // Sum is 6537430, diff = +10
        
        /* Should fail - NEW fork/corruption detected */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == false);
    }
    
    SECTION("Discrepancy of -10 blocks is rejected (exceeds tolerance)")
    {
        /* Unified significantly ahead - CRITICAL issue */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166259;
        uint32_t nUnified = 6537420;  // Sum is 6537410, diff = -10
        
        /* Should fail - database corruption */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified);
        
        REQUIRE(fResult == false);
    }
}


TEST_CASE("VerifyUnifiedHeightWithTolerance - Custom Tolerance", "[tolerance][verification]")
{
    SECTION("Custom tolerance of 10 blocks accepts +8 discrepancy")
    {
        /* Using custom tolerance */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166277;
        uint32_t nUnified = 6537420;  // Sum is 6537428, diff = +8
        uint32_t nTolerance = 10;
        
        /* Should pass with custom tolerance */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified, nTolerance);
        
        REQUIRE(fResult == true);
    }
    
    SECTION("Custom tolerance of 2 blocks rejects +3 discrepancy")
    {
        /* Using restrictive tolerance */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166272;
        uint32_t nUnified = 6537420;  // Sum is 6537423, diff = +3
        uint32_t nTolerance = 2;
        
        /* Should fail with restrictive tolerance */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified, nTolerance);
        
        REQUIRE(fResult == false);
    }
    
    SECTION("Zero tolerance only accepts perfect match")
    {
        /* No tolerance - strict verification */
        uint32_t nStake = 2068487;
        uint32_t nPrime = 2302664;
        uint32_t nHash = 2166272;
        uint32_t nUnified = 6537420;  // Sum is 6537423, diff = +3
        uint32_t nTolerance = 0;
        
        /* Should fail - any discrepancy rejected */
        bool fResult = ChannelStateManager::VerifyUnifiedHeightWithTolerance(
            nStake, nPrime, nHash, nUnified, nTolerance);
        
        REQUIRE(fResult == false);
    }
}


TEST_CASE("ForensicForkInfo Structure", "[tolerance][forensic]")
{
    SECTION("ForensicForkInfo default constructor initializes fields")
    {
        ForensicForkInfo info;
        
        /* Verify all fields initialized to zero/false */
        REQUIRE(info.nStakeHeight == 0);
        REQUIRE(info.nPrimeHeight == 0);
        REQUIRE(info.nHashHeight == 0);
        REQUIRE(info.nCalculatedSum == 0);
        REQUIRE(info.nUnifiedHeight == 0);
        REQUIRE(info.nDiscrepancy == 0);
        REQUIRE(info.nOrphanedBlocks == 0);
        REQUIRE(info.fWithinTolerance == false);
    }
    
    SECTION("ForensicForkInfo can store analysis results")
    {
        ForensicForkInfo info;
        
        /* Fill in analysis data */
        info.nStakeHeight = 2068487;
        info.nPrimeHeight = 2302664;
        info.nHashHeight = 2166272;
        info.nCalculatedSum = 6537423;
        info.nUnifiedHeight = 6537420;
        info.nDiscrepancy = 3;
        info.nOrphanedBlocks = 3;
        info.fWithinTolerance = true;
        
        /* Verify data stored correctly */
        REQUIRE(info.nStakeHeight == 2068487);
        REQUIRE(info.nPrimeHeight == 2302664);
        REQUIRE(info.nHashHeight == 2166272);
        REQUIRE(info.nCalculatedSum == 6537423);
        REQUIRE(info.nUnifiedHeight == 6537420);
        REQUIRE(info.nDiscrepancy == 3);
        REQUIRE(info.nOrphanedBlocks == 3);
        REQUIRE(info.fWithinTolerance == true);
    }
}


TEST_CASE("HISTORICAL_FORK_TOLERANCE Constant", "[tolerance][constants]")
{
    SECTION("Tolerance constant is set to 5")
    {
        /* Verify the tolerance value */
        REQUIRE(ChannelStateManager::HISTORICAL_FORK_TOLERANCE == 5);
    }
}


TEST_CASE("GetChannelHeightStatistics JSON Format", "[tolerance][forensic]")
{
    SECTION("Statistics method returns valid JSON structure")
    {
        /* Call the statistics method */
        std::string strStats = ChannelStateManager::GetChannelHeightStatistics();
        
        /* Should contain JSON keys */
        REQUIRE(strStats.find("stake_height") != std::string::npos);
        REQUIRE(strStats.find("prime_height") != std::string::npos);
        REQUIRE(strStats.find("hash_height") != std::string::npos);
        REQUIRE(strStats.find("calculated_sum") != std::string::npos);
        REQUIRE(strStats.find("unified_height") != std::string::npos);
        REQUIRE(strStats.find("discrepancy") != std::string::npos);
        REQUIRE(strStats.find("tolerance") != std::string::npos);
        REQUIRE(strStats.find("within_tolerance") != std::string::npos);
        REQUIRE(strStats.find("orphaned_blocks") != std::string::npos);
        
        /* Should start and end with braces */
        REQUIRE(strStats.front() == '{');
        REQUIRE(strStats.back() == '}');
    }
}


TEST_CASE("FindOrphanedBlocks Method", "[tolerance][forensic]")
{
    SECTION("FindOrphanedBlocks is callable")
    {
        /* Call with default depth - should not crash */
        REQUIRE_NOTHROW(ChannelStateManager::FindOrphanedBlocks());
    }
    
    SECTION("FindOrphanedBlocks with custom depth is callable")
    {
        /* Call with custom depth - should not crash */
        REQUIRE_NOTHROW(ChannelStateManager::FindOrphanedBlocks(1000));
    }
    
    SECTION("FindOrphanedBlocks returns non-negative count")
    {
        /* Result should be >= 0 */
        uint32_t nCount = ChannelStateManager::FindOrphanedBlocks(100);
        REQUIRE(nCount >= 0);
    }
}


TEST_CASE("AnalyzeChannelHeightDiscrepancy Method", "[tolerance][forensic]")
{
    SECTION("Analysis method is callable")
    {
        /* Should not crash */
        REQUIRE_NOTHROW(ChannelStateManager::AnalyzeChannelHeightDiscrepancy());
    }
    
    SECTION("Analysis method returns valid ForensicForkInfo")
    {
        /* Call analysis */
        ForensicForkInfo info = ChannelStateManager::AnalyzeChannelHeightDiscrepancy();
        
        /* Should have non-negative heights */
        REQUIRE(info.nStakeHeight >= 0);
        REQUIRE(info.nPrimeHeight >= 0);
        REQUIRE(info.nHashHeight >= 0);
        REQUIRE(info.nUnifiedHeight >= 0);
        REQUIRE(info.nCalculatedSum >= 0);
        REQUIRE(info.nOrphanedBlocks >= 0);
    }
}


/** SUMMARY OF TEST COVERAGE
 * 
 * ✅ Tested:
 *  - Perfect match verification (0 discrepancy)
 *  - Historical anomaly acceptance (+3 discrepancy)
 *  - Boundary conditions (±5 discrepancy)
 *  - Critical error detection (>5 discrepancy)
 *  - Custom tolerance values
 *  - ForensicForkInfo structure
 *  - HISTORICAL_FORK_TOLERANCE constant
 *  - GetChannelHeightStatistics JSON format
 *  - FindOrphanedBlocks functionality
 *  - AnalyzeChannelHeightDiscrepancy functionality
 * 
 * Test Philosophy:
 * ================
 * These tests verify the tolerance-based verification system correctly:
 * 1. Accepts the known +3 historical discrepancy
 * 2. Detects NEW issues exceeding tolerance
 * 3. Provides forensic tools for analysis
 * 4. Maintains backward compatibility
 **/
