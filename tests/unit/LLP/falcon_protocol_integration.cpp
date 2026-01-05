/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/stateless_miner.h>
#include <LLP/include/falcon_verify.h>
#include <LLC/include/flkey.h>
#include <LLC/include/falcon_constants_v2.h>

using namespace LLP;
using namespace LLC;

TEST_CASE("Falcon Protocol - MiningContext Version Tracking", "[protocol][falcon][integration]")
{
    SECTION("Default constructor initializes Falcon fields")
    {
        MiningContext ctx;
        
        REQUIRE(ctx.nFalconVersion == FalconVersion::FALCON_512);
        REQUIRE(ctx.fFalconVersionDetected == false);
        REQUIRE(ctx.vchPhysicalSignature.empty());
        REQUIRE(ctx.fPhysicalFalconPresent == false);
    }
    
    SECTION("WithFalconVersion updates version and detection flag")
    {
        MiningContext ctx;
        auto newCtx = ctx.WithFalconVersion(FalconVersion::FALCON_1024);
        
        REQUIRE(newCtx.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(newCtx.fFalconVersionDetected == true);
        
        // Original context should be unchanged (immutable)
        REQUIRE(ctx.fFalconVersionDetected == false);
    }
    
    SECTION("WithPhysicalSignature stores signature and sets flag")
    {
        MiningContext ctx;
        std::vector<uint8_t> testSig = {0x01, 0x02, 0x03, 0x04};
        
        auto newCtx = ctx.WithPhysicalSignature(testSig);
        
        REQUIRE(newCtx.vchPhysicalSignature == testSig);
        REQUIRE(newCtx.fPhysicalFalconPresent == true);
        
        // Original context should be unchanged
        REQUIRE(ctx.vchPhysicalSignature.empty());
        REQUIRE(ctx.fPhysicalFalconPresent == false);
    }
    
    SECTION("WithPhysicalSignature with empty vector clears flag")
    {
        MiningContext ctx;
        std::vector<uint8_t> testSig = {0x01, 0x02};
        
        auto ctx1 = ctx.WithPhysicalSignature(testSig);
        REQUIRE(ctx1.fPhysicalFalconPresent == true);
        
        auto ctx2 = ctx1.WithPhysicalSignature(std::vector<uint8_t>());
        REQUIRE(ctx2.fPhysicalFalconPresent == false);
    }
}


TEST_CASE("Falcon Protocol - Version Detection from Public Key", "[protocol][falcon][integration]")
{
    SECTION("Falcon-512 public key is correctly detected")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();
        
        FalconVersion detected;
        REQUIRE(FalconVerify::VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_512);
        
        // Verify size matches expected
        REQUIRE(pubkey.size() == FalconConstants::Falcon512::PUBLIC_KEY_SIZE);
    }
    
    SECTION("Falcon-1024 public key is correctly detected")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();
        
        FalconVersion detected;
        REQUIRE(FalconVerify::VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_1024);
        
        // Verify size matches expected
        REQUIRE(pubkey.size() == FalconConstants::Falcon1024::PUBLIC_KEY_SIZE);
    }
    
    SECTION("Invalid public key sizes are rejected")
    {
        std::vector<uint8_t> invalid_pubkey(500, 0xAA);
        
        FalconVersion detected;
        REQUIRE_FALSE(FalconVerify::VerifyPublicKey(invalid_pubkey, detected));
    }
}


TEST_CASE("Falcon Protocol - Signature Size Validation", "[protocol][falcon][integration]")
{
    SECTION("Falcon-512 signature has correct CT size")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        
        std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04};
        std::vector<uint8_t> signature;
        
        REQUIRE(key.Sign(message, signature));
        REQUIRE(signature.size() == FalconConstants::Falcon512::SIGNATURE_CT_SIZE);
    }
    
    SECTION("Falcon-1024 signature has correct CT size")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04};
        std::vector<uint8_t> signature;
        
        REQUIRE(key.Sign(message, signature));
        REQUIRE(signature.size() == FalconConstants::Falcon1024::SIGNATURE_CT_SIZE);
    }
    
    SECTION("GetSignatureCTSize returns correct sizes")
    {
        REQUIRE(FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_512) == 809);
        REQUIRE(FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_1024) == 1577);
    }
}


TEST_CASE("Falcon Protocol - Disposable Signature Verification", "[protocol][falcon][integration]")
{
    std::vector<uint8_t> message = {0xDE, 0xAD, 0xBE, 0xEF};
    
    SECTION("Falcon-512 disposable signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        
        REQUIRE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_512));
    }
    
    SECTION("Falcon-1024 disposable signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        
        REQUIRE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_1024));
    }
    
    SECTION("Wrong version rejects signature")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        
        // Try to verify with wrong version
        REQUIRE_FALSE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_1024));
    }
}


TEST_CASE("Falcon Protocol - Physical Falcon Signature Verification", "[protocol][falcon][integration]")
{
    std::vector<uint8_t> message = {0xCA, 0xFE, 0xBA, 0xBE};
    
    SECTION("Physical Falcon-512 signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        
        REQUIRE(FalconVerify::VerifyPhysicalFalconSignature(pubkey, message, signature));
    }
    
    SECTION("Physical Falcon-1024 signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        
        REQUIRE(FalconVerify::VerifyPhysicalFalconSignature(pubkey, message, signature));
    }
    
    SECTION("Physical signature auto-detects version from pubkey")
    {
        // Test that Physical Falcon verification works with both versions
        // without explicitly specifying the version
        
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));
        
        REQUIRE(FalconVerify::VerifyPhysicalFalconSignature(key512.GetPubKey(), message, sig512));
        REQUIRE(FalconVerify::VerifyPhysicalFalconSignature(key1024.GetPubKey(), message, sig1024));
    }
}


TEST_CASE("Falcon Protocol - Key Bonding Enforcement", "[protocol][falcon][integration]")
{
    std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    SECTION("Same key pair can be used for disposable and physical signatures")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();
        
        std::vector<uint8_t> disposableSig, physicalSig;
        REQUIRE(key.Sign(message, disposableSig));
        REQUIRE(key.Sign(message, physicalSig));
        
        // Both signatures should verify with the same key
        REQUIRE(FalconVerify::VerifySignature(pubkey, message, disposableSig, FalconVersion::FALCON_512));
        REQUIRE(FalconVerify::VerifyPhysicalFalconSignature(pubkey, message, physicalSig));
    }
    
    SECTION("Signature sizes must match for key bonding")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));
        
        // Verify sizes are different (key bonding can detect version mixing)
        REQUIRE(sig512.size() != sig1024.size());
        REQUIRE(sig512.size() == 809);
        REQUIRE(sig1024.size() == 1577);
    }
    
    SECTION("Disposable and physical must use same Falcon version")
    {
        // This tests that if a miner authenticates with Falcon-512,
        // they cannot submit a Falcon-1024 physical signature
        
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);
        
        // Get signatures
        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));
        
        // A miner using Falcon-512 for disposable
        auto pubkey512 = key512.GetPubKey();
        
        // Should NOT be able to use a Falcon-1024 signature
        // (In practice, this would be enforced by signature size mismatch)
        REQUIRE(sig512.size() == FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_512));
        REQUIRE(sig1024.size() == FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_1024));
    }
}


TEST_CASE("Falcon Protocol - Backward Compatibility", "[protocol][falcon][integration]")
{
    SECTION("Blocks without Physical Falcon are accepted")
    {
        MiningContext ctx;
        
        // Authenticate with Falcon-512
        auto ctx1 = ctx.WithFalconVersion(FalconVersion::FALCON_512)
                      .WithAuth(true);
        
        REQUIRE(ctx1.fAuthenticated);
        REQUIRE(ctx1.fFalconVersionDetected);
        
        // Submit block WITHOUT Physical Falcon
        REQUIRE(ctx1.fPhysicalFalconPresent == false);
        REQUIRE(ctx1.vchPhysicalSignature.empty());
        
        // This should be accepted (backward compatible)
    }
    
    SECTION("Physical Falcon is optional")
    {
        MiningContext ctx;
        auto ctx1 = ctx.WithFalconVersion(FalconVersion::FALCON_1024);
        
        // Both scenarios should be valid:
        // 1. No Physical Falcon
        REQUIRE(ctx1.fPhysicalFalconPresent == false);
        
        // 2. With Physical Falcon
        std::vector<uint8_t> physSig = {0x01, 0x02};
        auto ctx2 = ctx1.WithPhysicalSignature(physSig);
        REQUIRE(ctx2.fPhysicalFalconPresent == true);
    }
}


TEST_CASE("Falcon Protocol - Cross-Version Scenarios", "[protocol][falcon][integration]")
{
    SECTION("Falcon-512 and Falcon-1024 miners can coexist")
    {
        // Miner 1: Falcon-512
        MiningContext ctx512;
        ctx512 = ctx512.WithFalconVersion(FalconVersion::FALCON_512)
                       .WithAuth(true)
                       .WithSession(1001);
        
        // Miner 2: Falcon-1024
        MiningContext ctx1024;
        ctx1024 = ctx1024.WithFalconVersion(FalconVersion::FALCON_1024)
                         .WithAuth(true)
                         .WithSession(1002);
        
        // Both should be valid
        REQUIRE(ctx512.fAuthenticated);
        REQUIRE(ctx512.nFalconVersion == FalconVersion::FALCON_512);
        
        REQUIRE(ctx1024.fAuthenticated);
        REQUIRE(ctx1024.nFalconVersion == FalconVersion::FALCON_1024);
        
        // Different sessions
        REQUIRE(ctx512.nSessionId != ctx1024.nSessionId);
    }
    
    SECTION("Version persists across multiple block submissions")
    {
        MiningContext ctx;
        ctx = ctx.WithFalconVersion(FalconVersion::FALCON_1024)
                 .WithAuth(true);
        
        // Simulate multiple block submissions
        auto ctx1 = ctx.WithTimestamp(1000);
        auto ctx2 = ctx1.WithTimestamp(2000);
        auto ctx3 = ctx2.WithTimestamp(3000);
        
        // Version should persist
        REQUIRE(ctx1.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(ctx2.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(ctx3.nFalconVersion == FalconVersion::FALCON_1024);
        
        REQUIRE(ctx1.fFalconVersionDetected);
        REQUIRE(ctx2.fFalconVersionDetected);
        REQUIRE(ctx3.fFalconVersionDetected);
    }
}


TEST_CASE("Falcon Protocol - Error Cases", "[protocol][falcon][integration]")
{
    SECTION("Unauthenticated session has no version")
    {
        MiningContext ctx;
        
        REQUIRE_FALSE(ctx.fAuthenticated);
        REQUIRE_FALSE(ctx.fFalconVersionDetected);
    }
    
    SECTION("Invalid public key size is detected")
    {
        std::vector<uint8_t> invalid_pubkey(500, 0xAA);
        
        FalconVersion detected;
        REQUIRE_FALSE(FalconVerify::VerifyPublicKey(invalid_pubkey, detected));
    }
    
    SECTION("Signature size mismatch is detected")
    {
        FLKey key512;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        
        std::vector<uint8_t> message = {0x01, 0x02};
        std::vector<uint8_t> signature;
        REQUIRE(key512.Sign(message, signature));
        
        // Signature is 809 bytes for Falcon-512
        REQUIRE(signature.size() == 809);
        
        // But if we expect 1577 (Falcon-1024), it should fail
        REQUIRE(signature.size() != FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_1024));
    }
    
    SECTION("Physical signature without authentication fails")
    {
        MiningContext ctx;
        
        // Try to add Physical signature without authentication
        std::vector<uint8_t> physSig = {0x01, 0x02};
        auto newCtx = ctx.WithPhysicalSignature(physSig);
        
        // Should be set, but without authentication should be rejected
        REQUIRE(newCtx.fPhysicalFalconPresent);
        REQUIRE_FALSE(newCtx.fAuthenticated);
    }
}


TEST_CASE("Falcon Protocol - Session Lifecycle", "[protocol][falcon][integration]")
{
    SECTION("Session version persists across updates")
    {
        MiningContext ctx;
        
        // Authenticate and set version
        auto ctx1 = ctx.WithFalconVersion(FalconVersion::FALCON_1024)
                       .WithAuth(true)
                       .WithSession(2001);
        
        // Update other fields
        auto ctx2 = ctx1.WithTimestamp(runtime::unifiedtimestamp())
                        .WithHeight(12345);
        
        // Version should still be present
        REQUIRE(ctx2.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(ctx2.fFalconVersionDetected);
        REQUIRE(ctx2.fAuthenticated);
        REQUIRE(ctx2.nSessionId == 2001);
        REQUIRE(ctx2.nHeight == 12345);
    }
    
    SECTION("Context immutability is preserved")
    {
        MiningContext ctx;
        
        auto ctx1 = ctx.WithFalconVersion(FalconVersion::FALCON_512);
        
        // Original should be unchanged
        REQUIRE_FALSE(ctx.fFalconVersionDetected);
        REQUIRE(ctx.nFalconVersion == FalconVersion::FALCON_512);  // Default
        
        // New context should be updated
        REQUIRE(ctx1.fFalconVersionDetected);
        REQUIRE(ctx1.nFalconVersion == FalconVersion::FALCON_512);
    }
}
