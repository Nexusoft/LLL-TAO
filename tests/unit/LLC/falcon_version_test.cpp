/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/flkey.h>
#include <LLC/include/falcon_constants_v2.h>

using namespace LLC;

TEST_CASE("Falcon Version Support - Key Generation", "[falcon][version]")
{
    SECTION("Generate Falcon-512 key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        REQUIRE(key.IsValid());
        REQUIRE(key.GetVersion() == FalconVersion::FALCON_512);
        REQUIRE(key.GetPublicKeySize() == FalconSizes::FALCON512_PUBLIC_KEY_SIZE);
        REQUIRE(key.GetPrivateKeySize() == FalconSizes::FALCON512_PRIVATE_KEY_SIZE);
        REQUIRE(key.GetPubKey().size() == FalconSizes::FALCON512_PUBLIC_KEY_SIZE);
        REQUIRE(key.GetPrivKey().size() == FalconSizes::FALCON512_PRIVATE_KEY_SIZE);
    }

    SECTION("Generate Falcon-1024 key")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        REQUIRE(key.IsValid());
        REQUIRE(key.GetVersion() == FalconVersion::FALCON_1024);
        REQUIRE(key.GetPublicKeySize() == FalconSizes::FALCON1024_PUBLIC_KEY_SIZE);
        REQUIRE(key.GetPrivateKeySize() == FalconSizes::FALCON1024_PRIVATE_KEY_SIZE);
        REQUIRE(key.GetPubKey().size() == FalconSizes::FALCON1024_PUBLIC_KEY_SIZE);
        REQUIRE(key.GetPrivKey().size() == FalconSizes::FALCON1024_PRIVATE_KEY_SIZE);
    }

    SECTION("Default key generation is Falcon-512")
    {
        FLKey key;
        key.MakeNewKey();  // No parameter - should default to Falcon-512

        REQUIRE(key.IsValid());
        REQUIRE(key.GetVersion() == FalconVersion::FALCON_512);
    }
}


TEST_CASE("Falcon Version Support - Signing and Verification", "[falcon][version]")
{
    std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};

    SECTION("Falcon-512 sign and verify")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        REQUIRE(!signature.empty());
        
        // Falcon-512 CT signatures are exactly 809 bytes (ct=1 flag)
        REQUIRE(signature.size() == 809);

        REQUIRE(key.Verify(message, signature));
    }

    SECTION("Falcon-1024 sign and verify")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> signature;
        REQUIRE(key.Sign(message, signature));
        REQUIRE(!signature.empty());
        
        // Falcon-1024 CT signatures are exactly 1577 bytes (ct=1 flag)
        REQUIRE(signature.size() == 1577);

        REQUIRE(key.Verify(message, signature));
    }

    SECTION("Cross-version verification fails correctly")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        std::vector<uint8_t> sig512, sig1024;
        REQUIRE(key512.Sign(message, sig512));
        REQUIRE(key1024.Sign(message, sig1024));

        // Signature from one key shouldn't verify with another key
        REQUIRE_FALSE(key512.Verify(message, sig1024));
        REQUIRE_FALSE(key1024.Verify(message, sig512));
    }
}


TEST_CASE("Falcon Version Support - Version Detection", "[falcon][version]")
{
    SECTION("Detect version from public key size")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        auto pubkey512 = key512.GetPubKey();
        auto pubkey1024 = key1024.GetPubKey();

        REQUIRE(FLKey::DetectVersion(pubkey512.size(), true) == FalconVersion::FALCON_512);
        REQUIRE(FLKey::DetectVersion(pubkey1024.size(), true) == FalconVersion::FALCON_1024);
    }

    SECTION("Detect version from private key size")
    {
        FLKey key512, key1024;
        key512.MakeNewKey(FalconVersion::FALCON_512);
        key1024.MakeNewKey(FalconVersion::FALCON_1024);

        auto privkey512 = key512.GetPrivKey();
        auto privkey1024 = key1024.GetPrivKey();

        REQUIRE(FLKey::DetectVersion(privkey512.size(), false) == FalconVersion::FALCON_512);
        REQUIRE(FLKey::DetectVersion(privkey1024.size(), false) == FalconVersion::FALCON_1024);
    }

    SECTION("Invalid key sizes throw exception")
    {
        REQUIRE_THROWS(FLKey::DetectVersion(100, true));
        REQUIRE_THROWS(FLKey::DetectVersion(500, true));
        REQUIRE_THROWS(FLKey::DetectVersion(2000, true));
    }
}


TEST_CASE("Falcon Version Support - Key Import/Export", "[falcon][version]")
{
    std::vector<uint8_t> message = {0xAA, 0xBB, 0xCC};

    SECTION("Export and import Falcon-512 key")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_512);

        auto pubkey = key1.GetPubKey();
        auto privkey = key1.GetPrivKey();

        FLKey key2;
        REQUIRE(key2.SetPubKey(pubkey));
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_512);

        FLKey key3;
        REQUIRE(key3.SetPrivKey(privkey));
        REQUIRE(key3.GetVersion() == FalconVersion::FALCON_512);

        // Verify signature works with imported keys
        std::vector<uint8_t> signature;
        REQUIRE(key3.Sign(message, signature));
        REQUIRE(key2.Verify(message, signature));
    }

    SECTION("Export and import Falcon-1024 key")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_1024);

        auto pubkey = key1.GetPubKey();
        auto privkey = key1.GetPrivKey();

        FLKey key2;
        REQUIRE(key2.SetPubKey(pubkey));
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_1024);

        FLKey key3;
        REQUIRE(key3.SetPrivKey(privkey));
        REQUIRE(key3.GetVersion() == FalconVersion::FALCON_1024);

        // Verify signature works with imported keys
        std::vector<uint8_t> signature;
        REQUIRE(key3.Sign(message, signature));
        REQUIRE(key2.Verify(message, signature));
    }
}


TEST_CASE("Falcon Version Support - Key Validation", "[falcon][version]")
{
    SECTION("Validate Falcon-512 keys")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_512);

        REQUIRE(key.ValidatePublicKey());
        REQUIRE(key.ValidatePrivateKey());
    }

    SECTION("Validate Falcon-1024 keys")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        REQUIRE(key.ValidatePublicKey());
        REQUIRE(key.ValidatePrivateKey());
    }

    SECTION("Empty keys fail validation")
    {
        FLKey key;
        REQUIRE_FALSE(key.ValidatePublicKey());
        REQUIRE_FALSE(key.ValidatePrivateKey());
    }
}


TEST_CASE("Falcon Version Support - Clear and Reset", "[falcon][version]")
{
    SECTION("Clear securely wipes key material")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        REQUIRE(key.IsValid());
        REQUIRE(key.GetVersion() == FalconVersion::FALCON_1024);

        key.Clear();

        REQUIRE_FALSE(key.IsValid());
        REQUIRE(key.GetPubKey().empty());
        REQUIRE(key.GetPrivKey().empty());
        // After clear, version should reset to default
        REQUIRE(key.GetVersion() == FalconVersion::FALCON_512);
    }

    SECTION("Reset clears all key data")
    {
        FLKey key;
        key.MakeNewKey(FalconVersion::FALCON_1024);

        REQUIRE(key.IsValid());
        key.Reset();

        REQUIRE_FALSE(key.IsValid());
        REQUIRE(key.IsNull());
    }
}


TEST_CASE("Falcon Constants V2 - Runtime Size Getters", "[falcon][constants]")
{
    using namespace FalconConstants;

    SECTION("Get sizes for Falcon-512")
    {
        REQUIRE(GetPublicKeySize(FalconVersion::FALCON_512) == 897);
        REQUIRE(GetPrivateKeySize(FalconVersion::FALCON_512) == 1281);
        REQUIRE(GetSignatureSize(FalconVersion::FALCON_512) == 809);  // CT default
        REQUIRE(GetSignatureCTSize(FalconVersion::FALCON_512) == 809);
    }

    SECTION("Get sizes for Falcon-1024")
    {
        REQUIRE(GetPublicKeySize(FalconVersion::FALCON_1024) == 1793);
        REQUIRE(GetPrivateKeySize(FalconVersion::FALCON_1024) == 2305);
        REQUIRE(GetSignatureSize(FalconVersion::FALCON_1024) == 1577);  // CT default
        REQUIRE(GetSignatureCTSize(FalconVersion::FALCON_1024) == 1577);
    }

    SECTION("Auth message sizes vary by version")
    {
        size_t challenge32 = 32;

        size_t auth512 = GetAuthPlaintextSize(FalconVersion::FALCON_512, challenge32);
        size_t auth1024 = GetAuthPlaintextSize(FalconVersion::FALCON_1024, challenge32);

        REQUIRE(auth1024 > auth512);
        // Difference should be: (1793-897) + (1577-809) = 896 + 768 = 1664 bytes
        REQUIRE((auth1024 - auth512) == 1664);
    }
}


TEST_CASE("Falcon Version Support - Copy and Move Semantics", "[falcon][version]")
{
    std::vector<uint8_t> message = {0x11, 0x22, 0x33};

    SECTION("Copy constructor preserves version")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_1024);

        FLKey key2(key1);
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_1024);
        REQUIRE(key2.IsValid());

        std::vector<uint8_t> sig;
        REQUIRE(key2.Sign(message, sig));
        REQUIRE(key1.Verify(message, sig));
    }

    SECTION("Copy assignment preserves version")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_1024);

        FLKey key2;
        key2 = key1;
        
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_1024);
        REQUIRE(key2.IsValid());
    }

    SECTION("Move constructor preserves version")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_1024);

        FLKey key2(std::move(key1));
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_1024);
        REQUIRE(key2.IsValid());
    }

    SECTION("Move assignment preserves version")
    {
        FLKey key1;
        key1.MakeNewKey(FalconVersion::FALCON_1024);

        FLKey key2;
        key2 = std::move(key1);
        
        REQUIRE(key2.GetVersion() == FalconVersion::FALCON_1024);
        REQUIRE(key2.IsValid());
    }
}
