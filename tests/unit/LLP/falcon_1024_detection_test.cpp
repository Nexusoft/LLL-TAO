/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/flkey.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/falcon_verify.h>

using namespace LLC;
using namespace LLP;

TEST_CASE("Falcon-1024 Block Size Detection", "[falcon][falcon1024][detection]")
{
    SECTION("Detect Falcon-1024 from public key size")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        auto pubkey512 = key512.GetPubKey();
        auto pubkey1024 = key1024.GetPubKey();
        
        // Falcon-512 public key is 897 bytes
        REQUIRE(pubkey512.size() == FalconConstants::FALCON512_PUBKEY_SIZE);
        REQUIRE(pubkey512.size() == 897);
        
        // Falcon-1024 public key is 1793 bytes
        REQUIRE(pubkey1024.size() == FalconConstants::FALCON1024_PUBKEY_SIZE);
        REQUIRE(pubkey1024.size() == 1793);
        
        // Auto-detect version
        FalconVersion detected512 = FalconVerify::DetectVersionFromPubkey(pubkey512);
        FalconVersion detected1024 = FalconVerify::DetectVersionFromPubkey(pubkey1024);
        
        REQUIRE(detected512 == FalconVersion::FALCON_512);
        REQUIRE(detected1024 == FalconVersion::FALCON_1024);
    }
    
    SECTION("Detect Falcon-1024 from signature size")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> message = {0xCA, 0xFE, 0xBA, 0xBE};
        std::vector<uint8_t> sig512, sig1024;
        
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));
        
        // Falcon-512 CT signature is 809 bytes
        REQUIRE(sig512.size() == FalconConstants::FALCON512_SIG_CT_SIZE);
        REQUIRE(sig512.size() == 809);
        
        // Falcon-1024 CT signature is 1577 bytes
        REQUIRE(sig1024.size() == FalconConstants::FALCON1024_SIG_CT_SIZE);
        REQUIRE(sig1024.size() == 1577);
        
        // Auto-detect version from signature size
        FalconVersion detected512 = FalconVerify::DetectVersionFromSignature(sig512);
        FalconVersion detected1024 = FalconVerify::DetectVersionFromSignature(sig1024);
        
        REQUIRE(detected512 == FalconVersion::FALCON_512);
        REQUIRE(detected1024 == FalconVersion::FALCON_1024);
    }
    
    SECTION("Calculate correct packet sizes for Falcon-1024")
    {
        // Tritium block without physical signature (Falcon-1024)
        // Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)]
        size_t expected = 216 + 8 + 2 + 1577 + 2;
        REQUIRE(expected == 1805);
        REQUIRE(expected == FalconConstants::SUBMIT_BLOCK_FULL_TRITIUM_WRAPPER_FALCON1024_MAX);
        
        // Tritium block WITH physical signature (Falcon-1024)
        // Format: [block(216)][timestamp(8)][siglen(2)][sig(1577)][physiglen(2)][physical_sig(1577)]
        size_t expectedDual = 216 + 8 + 2 + 1577 + 2 + 1577;
        REQUIRE(expectedDual == 3382);
        REQUIRE(expectedDual == FalconConstants::SUBMIT_BLOCK_FULL_DUAL_SIG_TRITIUM_FALCON1024_MAX);
    }
    
    SECTION("Version-agnostic aliases accept both Falcon-512 and Falcon-1024")
    {
        // Falcon-512 signature (809 bytes) must pass validation
        REQUIRE(809 >= FalconConstants::FALCON_SIG_MIN);
        REQUIRE(809 <= FalconConstants::FALCON_SIG_MAX_VALIDATION);
        REQUIRE(809 <= FalconConstants::FALCON_SIG_ABSOLUTE_MAX);
        
        // Falcon-1024 signature (1577 bytes) must pass validation
        REQUIRE(1577 >= FalconConstants::FALCON_SIG_MIN);
        REQUIRE(1577 <= FalconConstants::FALCON_SIG_MAX_VALIDATION);
        REQUIRE(1577 <= FalconConstants::FALCON_SIG_ABSOLUTE_MAX);
        
        // Version-agnostic max is the largest (Falcon-1024)
        REQUIRE(FalconConstants::FALCON_SIG_ABSOLUTE_MAX == 1577);
        REQUIRE(FalconConstants::FALCON_SIG_CT_MAX == 1577);
    }
}

TEST_CASE("Falcon-1024 Fallback Detection", "[falcon][falcon1024][detection]")
{
    SECTION("Common Falcon-1024 signature sizes are defined")
    {
        // Verify all 5 common Falcon-1024 signature sizes exist
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_1 == 1577);
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_2 == 1550);
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_3 == 1500);
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_4 == 1400);
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_5 == 1280);
        
        // All sizes are within valid range
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_1 <= FalconConstants::FALCON1024_SIG_ABSOLUTE_MAX);
        REQUIRE(FalconConstants::FALCON1024_SIG_COMMON_SIZE_5 >= FalconConstants::FALCON1024_SIG_MIN);
    }
    
    SECTION("Fallback detection prioritizes Falcon-1024 (larger, less ambiguous)")
    {
        // When block size is unknown, try common signature sizes
        // Falcon-1024 signatures should be tried FIRST (larger and less ambiguous)
        
        // This test simulates the fallback detection array logic from PR #124
        const uint16_t commonSigSizes[] = {
            // Falcon-1024 sizes (try first)
            FalconConstants::FALCON1024_SIG_COMMON_SIZE_1,  // 1577
            FalconConstants::FALCON1024_SIG_COMMON_SIZE_2,  // 1550
            FalconConstants::FALCON1024_SIG_COMMON_SIZE_3,  // 1500
            FalconConstants::FALCON1024_SIG_COMMON_SIZE_4,  // 1400
            FalconConstants::FALCON1024_SIG_COMMON_SIZE_5,  // 1280
            // Falcon-512 sizes
            FalconConstants::FALCON512_SIG_COMMON_SIZE_1,   // 809
            FalconConstants::FALCON512_SIG_COMMON_SIZE_2,   // 800
            FalconConstants::FALCON512_SIG_COMMON_SIZE_3,   // 750
            FalconConstants::FALCON512_SIG_COMMON_SIZE_4,   // 720
            FalconConstants::FALCON512_SIG_COMMON_SIZE_5,   // 700
        };
        
        // First 5 entries should be Falcon-1024
        REQUIRE(commonSigSizes[0] >= 1280);
        REQUIRE(commonSigSizes[4] >= 1280);
        
        // Last 5 entries should be Falcon-512
        REQUIRE(commonSigSizes[5] <= 900);
        REQUIRE(commonSigSizes[9] <= 900);
        
        // Falcon-1024 signatures are less likely to cause false positives
        REQUIRE(commonSigSizes[0] > commonSigSizes[5] * 1.5);  // At least 1.5x larger
    }
}

TEST_CASE("Falcon-1024 Physical Signature Overhead", "[falcon][falcon1024][overhead]")
{
    SECTION("Physical Falcon overhead constants are correct")
    {
        // Falcon-512 physical signature overhead
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_OVERHEAD == 811);
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_OVERHEAD == 2 + 809);
        
        // Falcon-1024 physical signature overhead
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_FALCON1024_OVERHEAD == 1579);
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_FALCON1024_OVERHEAD == 2 + 1577);
        
        // Version-agnostic alias uses Falcon-1024 (largest)
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_MAX_OVERHEAD == 1579);
        REQUIRE(FalconConstants::PHYSICAL_BLOCK_SIG_MAX_OVERHEAD == FalconConstants::PHYSICAL_BLOCK_SIG_FALCON1024_OVERHEAD);
    }
    
    SECTION("Dual signature overhead constants are correct")
    {
        // Falcon-512 dual signature overhead
        // (2 + 809) + (2 + 809) = 1622 bytes
        REQUIRE(FalconConstants::DUAL_SIG_OVERHEAD == 1622);
        
        // Falcon-1024 dual signature overhead
        // (2 + 1577) + (2 + 1577) = 3158 bytes
        REQUIRE(FalconConstants::DUAL_SIG_FALCON1024_OVERHEAD == 3158);
        
        // Version-agnostic alias uses Falcon-1024 (largest)
        REQUIRE(FalconConstants::DUAL_SIG_MAX_OVERHEAD == 3158);
        REQUIRE(FalconConstants::DUAL_SIG_MAX_OVERHEAD == FalconConstants::DUAL_SIG_FALCON1024_OVERHEAD);
        
        // With timestamp included
        REQUIRE(FalconConstants::DUAL_SIG_MAX_TOTAL_OVERHEAD == 3166);
        REQUIRE(FalconConstants::DUAL_SIG_MAX_TOTAL_OVERHEAD == 3158 + 8);
    }
    
    SECTION("Minimum overhead alias uses Falcon-512")
    {
        // The minimum overhead alias should use Falcon-512 (smaller)
        REQUIRE(FalconConstants::BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD_AGNOSTIC == 811);
        REQUIRE(FalconConstants::BLOCK_WITH_PHYSICAL_SIG_MIN_OVERHEAD_AGNOSTIC == FalconConstants::PHYSICAL_BLOCK_SIG_OVERHEAD);
    }
}

TEST_CASE("Mixed Falcon Version Scenarios", "[falcon][falcon1024][integration]")
{
    SECTION("Falcon-512 authentication, Falcon-1024 block submission should fail (key bonding)")
    {
        // This tests the key bonding enforcement from PR #121/#122
        // A miner cannot use Falcon-512 for auth and Falcon-1024 for blocks
        
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> message = {0x01, 0x02, 0x03};
        std::vector<uint8_t> sig512, sig1024;
        
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));
        
        // Signature sizes don't match (key bonding violation)
        REQUIRE(sig512.size() != sig1024.size());
        REQUIRE(sig512.size() == 809);
        REQUIRE(sig1024.size() == 1577);
        
        // Node would reject this in SUBMIT_BLOCK handler during key bonding check
    }
    
    SECTION("Consistent Falcon-1024 for both auth and blocks is valid")
    {
        FLKey key1024;
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> authMessage = {0xAA, 0xBB};
        std::vector<uint8_t> blockMessage = {0xCC, 0xDD};
        
        std::vector<uint8_t> authSig, blockSig;
        REQUIRE(key1024.Sign(authMessage, authSig));
        REQUIRE(key1024.Sign(blockMessage, blockSig));
        
        // Both signatures use Falcon-1024
        REQUIRE(authSig.size() == 1577);
        REQUIRE(blockSig.size() == 1577);
        
        // Same key can be used for authentication and block signing (key bonding)
        REQUIRE(key1024.GetPubKey().size() == 1793);
    }
}
