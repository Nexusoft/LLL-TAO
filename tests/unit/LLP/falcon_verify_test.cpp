/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/falcon_verify.h>
#include <LLC/include/flkey.h>

using namespace LLP::FalconVerify;
using namespace LLC;

TEST_CASE("Falcon Verify - Public Key Validation", "[falcon][verify]")
{
    SECTION("Verify valid Falcon-512 public key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();

        REQUIRE(VerifyPublicKey512(pubkey));
    }

    SECTION("Verify valid Falcon-1024 public key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();

        REQUIRE(VerifyPublicKey1024(pubkey));
    }

    SECTION("Falcon-512 key fails Falcon-1024 validation")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();

        REQUIRE_FALSE(VerifyPublicKey1024(pubkey));
    }

    SECTION("Falcon-1024 key fails Falcon-512 validation")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();

        REQUIRE_FALSE(VerifyPublicKey512(pubkey));
    }

    SECTION("Invalid size fails validation")
    {
        std::vector<uint8_t> invalid_pubkey(500, 0xAA);
        
        REQUIRE_FALSE(VerifyPublicKey512(invalid_pubkey));
        REQUIRE_FALSE(VerifyPublicKey1024(invalid_pubkey));
    }

    SECTION("Empty key fails validation")
    {
        std::vector<uint8_t> empty;
        
        REQUIRE_FALSE(VerifyPublicKey512(empty));
        REQUIRE_FALSE(VerifyPublicKey1024(empty));
    }
}


TEST_CASE("Falcon Verify - Auto-Detection", "[falcon][verify]")
{
    SECTION("Auto-detect Falcon-512 from public key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);
        auto pubkey = key.GetPubKey();

        FalconVersion detected;
        REQUIRE(VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_512);
    }

    SECTION("Auto-detect Falcon-1024 from public key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);
        auto pubkey = key.GetPubKey();

        FalconVersion detected;
        REQUIRE(VerifyPublicKey(pubkey, detected));
        REQUIRE(detected == FalconVersion::FALCON_1024);
    }

    SECTION("DetectVersionFromPubkey works correctly")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        REQUIRE(DetectVersionFromPubkey(key512.GetPubKey()) == FalconVersion::FALCON_512);
        REQUIRE(DetectVersionFromPubkey(key1024.GetPubKey()) == FalconVersion::FALCON_1024);
    }

    SECTION("DetectVersionFromSignature works correctly")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> message = {0x01, 0x02, 0x03};
        std::vector<uint8_t> sig512, sig1024;

        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        REQUIRE(DetectVersionFromSignature(sig512) == FalconVersion::FALCON_512);
        REQUIRE(DetectVersionFromSignature(sig1024) == FalconVersion::FALCON_1024);
    }

    SECTION("Invalid signature size throws exception")
    {
        std::vector<uint8_t> invalid_sig(100, 0xFF);
        REQUIRE_THROWS(DetectVersionFromSignature(invalid_sig));
    }
}


TEST_CASE("Falcon Verify - Signature Verification", "[falcon][verify]")
{
    std::vector<uint8_t> message = {0xDE, 0xAD, 0xBE, 0xEF};

    SECTION("Verify Falcon-512 signature")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        REQUIRE(VerifySignature(key.GetPubKey(), message, signature, FalconVersion::FALCON_512));
    }

    SECTION("Verify Falcon-1024 signature")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        REQUIRE(VerifySignature(key.GetPubKey(), message, signature, FalconVersion::FALCON_1024));
    }

    SECTION("Wrong message fails verification")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        std::vector<uint8_t> wrong_message = {0xFF, 0xFF, 0xFF, 0xFF};
        REQUIRE_FALSE(VerifySignature(key.GetPubKey(), wrong_message, signature, FalconVersion::FALCON_512));
    }

    SECTION("Wrong version fails verification")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        // Try to verify with wrong version
        REQUIRE_FALSE(VerifySignature(key.GetPubKey(), message, signature, FalconVersion::FALCON_1024));
    }

    SECTION("Corrupted signature fails verification")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));

        // Corrupt the signature
        if(!signature.empty())
            signature[signature.size() / 2] ^= 0xFF;

        REQUIRE_FALSE(VerifySignature(key.GetPubKey(), message, signature, FalconVersion::FALCON_512));
    }
}


TEST_CASE("Falcon Verify - Cross-Version Tests", "[falcon][verify]")
{
    std::vector<uint8_t> message = {0xCA, 0xFE, 0xBA, 0xBE};

    SECTION("Falcon-512 and Falcon-1024 signatures are distinct")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        // Each signature should only verify with its own key
        REQUIRE(VerifySignature(key512.GetPubKey(), message, sig512, FalconVersion::FALCON_512));
        REQUIRE(VerifySignature(key1024.GetPubKey(), message, sig1024, FalconVersion::FALCON_1024));

        // Cross-verification should fail
        REQUIRE_FALSE(VerifySignature(key512.GetPubKey(), message, sig1024, FalconVersion::FALCON_512));
        REQUIRE_FALSE(VerifySignature(key1024.GetPubKey(), message, sig512, FalconVersion::FALCON_1024));
    }

    SECTION("Auto-detection allows stealth mode operation")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        // Node can auto-detect version and verify both
        FalconVersion detected512, detected1024;
        REQUIRE(VerifyPublicKey(key512.GetPubKey(), detected512));
        REQUIRE(VerifyPublicKey(key1024.GetPubKey(), detected1024));

        REQUIRE(detected512 == FalconVersion::FALCON_512);
        REQUIRE(detected1024 == FalconVersion::FALCON_1024);

        REQUIRE(VerifySignature(key512.GetPubKey(), message, sig512, detected512));
        REQUIRE(VerifySignature(key1024.GetPubKey(), message, sig1024, detected1024));
    }
}


TEST_CASE("Falcon Verify - Multiple Messages", "[falcon][verify]")
{
    SECTION("Multiple signatures with same key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> msg1 = {0x01};
        std::vector<uint8_t> msg2 = {0x02};
        std::vector<uint8_t> msg3 = {0x03};

        std::vector<uint8_t> sig1, sig2, sig3;
        REQUIRE(key.Sign(msg1, sig1));
        REQUIRE(key.Sign(msg2, sig2));
        REQUIRE(key.Sign(msg3, sig3));

        // Each signature should verify with its corresponding message
        REQUIRE(VerifySignature(key.GetPubKey(), msg1, sig1, FalconVersion::FALCON_1024));
        REQUIRE(VerifySignature(key.GetPubKey(), msg2, sig2, FalconVersion::FALCON_1024));
        REQUIRE(VerifySignature(key.GetPubKey(), msg3, sig3, FalconVersion::FALCON_1024));

        // But not with wrong messages
        REQUIRE_FALSE(VerifySignature(key.GetPubKey(), msg1, sig2, FalconVersion::FALCON_1024));
        REQUIRE_FALSE(VerifySignature(key.GetPubKey(), msg2, sig3, FalconVersion::FALCON_1024));
    }
}


TEST_CASE("Falcon Verify - Edge Cases", "[falcon][verify]")
{
    SECTION("Empty message can be signed and verified")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> empty_message;
        std::vector<uint8_t> signature;

        REQUIRE(key.Sign(empty_message, signature));
        REQUIRE(VerifySignature(key.GetPubKey(), empty_message, signature, FalconVersion::FALCON_512));
    }

    SECTION("Large message can be signed and verified")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> large_message(10000, 0x42);
        std::vector<uint8_t> signature;

        REQUIRE(key.Sign(large_message, signature));
        REQUIRE(VerifySignature(key.GetPubKey(), large_message, signature, FalconVersion::FALCON_1024));
    }
}
