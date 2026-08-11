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
    SECTION("Default constructor initializes current Falcon fields")
    {
        MiningContext ctx;

        REQUIRE(ctx.nFalconVersion == FalconVersion::FALCON_512);
        REQUIRE(ctx.fFalconVersionDetected == false);
        REQUIRE(ctx.fAuthenticated == false);
    }

    SECTION("WithFalconVersion updates version and preserves immutability")
    {
        MiningContext ctx;
        MiningContext updated = ctx.WithFalconVersion(FalconVersion::FALCON_1024);

        REQUIRE(updated.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(updated.fFalconVersionDetected == true);

        REQUIRE(ctx.nFalconVersion == FalconVersion::FALCON_512);
        REQUIRE(ctx.fFalconVersionDetected == false);
    }
}

TEST_CASE("Falcon Protocol - Version Detection from Public Key", "[protocol][falcon][integration]")
{
    SECTION("Falcon-512 public key is correctly detected")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        std::vector<uint8_t> pubkey = key.GetPubKey();

        FalconVersion detected;
        REQUIRE(FalconVerify::VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_512);
        REQUIRE(pubkey.size() == FalconConstants::Falcon512::PUBLIC_KEY_SIZE);
    }

    SECTION("Falcon-1024 public key is correctly detected")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        std::vector<uint8_t> pubkey = key.GetPubKey();

        FalconVersion detected;
        REQUIRE(FalconVerify::VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_1024);
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
        REQUIRE(FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_512)
                == FalconConstants::Falcon512::SIGNATURE_CT_SIZE);
        REQUIRE(FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_1024)
                == FalconConstants::Falcon1024::SIGNATURE_CT_SIZE);
    }
}

TEST_CASE("Falcon Protocol - Signature Verification", "[protocol][falcon][integration]")
{
    std::vector<uint8_t> message = {0xDE, 0xAD, 0xBE, 0xEF};

    SECTION("Falcon-512 signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        std::vector<uint8_t> pubkey = key.GetPubKey();

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        REQUIRE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_512));
    }

    SECTION("Falcon-1024 signature verifies correctly")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        std::vector<uint8_t> pubkey = key.GetPubKey();

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        REQUIRE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_1024));
    }

    SECTION("Wrong version rejects signature")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        std::vector<uint8_t> pubkey = key.GetPubKey();

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        REQUIRE_FALSE(FalconVerify::VerifySignature(pubkey, message, signature, FalconVersion::FALCON_1024));
    }
}

TEST_CASE("Falcon Protocol - Key Bonding Enforcement", "[protocol][falcon][integration]")
{
    std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};

    SECTION("Context Falcon version defines the expected signature size")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        MiningContext ctx = MiningContext()
            .WithFalconVersion(FalconVersion::FALCON_512)
            .WithAuth(true);

        size_t expectedSize = FalconConstants::GetSignatureCTSize(ctx.nFalconVersion);
        REQUIRE(expectedSize == FalconConstants::Falcon512::SIGNATURE_CT_SIZE);
        REQUIRE(sig512.size() == expectedSize);
        REQUIRE(sig1024.size() != expectedSize);
    }

    SECTION("Signature sizes distinguish Falcon-512 and Falcon-1024 sessions")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        REQUIRE(sig512.size() == FalconConstants::Falcon512::SIGNATURE_CT_SIZE);
        REQUIRE(sig1024.size() == FalconConstants::Falcon1024::SIGNATURE_CT_SIZE);
        REQUIRE(sig512.size() != sig1024.size());
    }
}

TEST_CASE("Falcon Protocol - Session Lifecycle", "[protocol][falcon][integration]")
{
    SECTION("Falcon version persists across context updates")
    {
        MiningContext ctx = MiningContext()
            .WithFalconVersion(FalconVersion::FALCON_1024)
            .WithAuth(true)
            .WithSession(2001);

        MiningContext updated = ctx
            .WithTimestamp(1000)
            .WithHeight(12345);

        REQUIRE(updated.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(updated.fFalconVersionDetected == true);
        REQUIRE(updated.fAuthenticated == true);
        REQUIRE(updated.nSessionId == 2001);
        REQUIRE(updated.nHeight == 12345);
    }

    SECTION("Falcon-512 and Falcon-1024 miners can coexist")
    {
        MiningContext ctx512 = MiningContext()
            .WithFalconVersion(FalconVersion::FALCON_512)
            .WithAuth(true)
            .WithSession(1001);

        MiningContext ctx1024 = MiningContext()
            .WithFalconVersion(FalconVersion::FALCON_1024)
            .WithAuth(true)
            .WithSession(1002);

        REQUIRE(ctx512.nFalconVersion == FalconVersion::FALCON_512);
        REQUIRE(ctx1024.nFalconVersion == FalconVersion::FALCON_1024);
        REQUIRE(ctx512.nSessionId != ctx1024.nSessionId);
    }
}

TEST_CASE("Falcon Protocol - Error Cases", "[protocol][falcon][integration]")
{
    SECTION("Unauthenticated session has no detected Falcon version")
    {
        MiningContext ctx;

        REQUIRE(ctx.fAuthenticated == false);
        REQUIRE(ctx.fFalconVersionDetected == false);
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

        REQUIRE(signature.size() == FalconConstants::Falcon512::SIGNATURE_CT_SIZE);
        REQUIRE(signature.size() != FalconConstants::GetSignatureCTSize(FalconVersion::FALCON_1024));
    }
}
