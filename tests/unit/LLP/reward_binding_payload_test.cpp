/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(W.J.[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLP/include/reward_binding_payload.h>

#include <cstdint>
#include <string>
#include <vector>

namespace
{
    /* Build a 32-byte big-endian / display-order genesis whose first byte is the
     * UserType marker (0xa1).  Mirrors the layout produced by NexusMiner. */
    std::vector<uint8_t> SampleGenesisBytes()
    {
        std::vector<uint8_t> v(32, 0x00);
        v[0]  = 0xa1;       // UserType marker (matches GENESIS::UserType())
        v[31] = 0x42;       // arbitrary least-significant byte
        return v;
    }
}

using LLP::RewardBindingPayload;

TEST_CASE("RewardBindingPayload: legacy 32-byte payload (event-only)", "[reward_binding][shared]")
{
    /* A bare 32-byte payload is the legacy wire form shared by both lanes
     * before auto-credit existed.  The parser must accept it as event-only. */
    const std::vector<uint8_t> v = SampleGenesisBytes();

    uint256_t   hashGenesis = 0;
    std::string strAccountName = "preset-must-be-cleared";

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::Ok);

    /* The high byte of the uint256_t corresponds to the type-byte position. */
    REQUIRE(hashGenesis.GetType() == 0xa1);
    REQUIRE(strAccountName.empty());
}

TEST_CASE("RewardBindingPayload: extended payload with account-name (auto-credit)",
          "[reward_binding][shared]")
{
    std::vector<uint8_t> v = SampleGenesisBytes();
    const std::string    strName = "default";
    v.push_back(static_cast<uint8_t>(strName.size()));
    v.insert(v.end(), strName.begin(), strName.end());

    uint256_t   hashGenesis = 0;
    std::string strAccountName;

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::Ok);
    REQUIRE(hashGenesis.GetType() == 0xa1);
    REQUIRE(strAccountName == "default");
}

TEST_CASE("RewardBindingPayload: payload too short rejected", "[reward_binding][shared]")
{
    std::vector<uint8_t> v(31, 0x00);

    uint256_t   hashGenesis = 0xdead;
    std::string strAccountName = "should-clear";

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::TooShort);
    /* Outputs must be reset on every failure path. */
    REQUIRE(hashGenesis == uint256_t(0));
    REQUIRE(strAccountName.empty());
}

TEST_CASE("RewardBindingPayload: 33-byte extension marker without name rejected",
          "[reward_binding][shared]")
{
    /* 33 bytes is unambiguous: extension marker present but no room for a name. */
    std::vector<uint8_t> v = SampleGenesisBytes();
    v.push_back(0x05);          // claims 5 bytes follow but none do

    uint256_t   hashGenesis = 0;
    std::string strAccountName;

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::ExtensionTooShort);
}

TEST_CASE("RewardBindingPayload: zero name length is invalid", "[reward_binding][shared]")
{
    /* Zero-length name with a trailing byte must not parse — that wire form
     * is reserved (callers must use the bare 32-byte legacy payload instead). */
    std::vector<uint8_t> v = SampleGenesisBytes();
    v.push_back(0x00);
    v.push_back(0x00);

    uint256_t   hashGenesis = 0;
    std::string strAccountName;

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::NameLengthZero);
}

TEST_CASE("RewardBindingPayload: declared length / actual length mismatch rejected",
          "[reward_binding][shared]")
{
    std::vector<uint8_t> v = SampleGenesisBytes();
    v.push_back(0x05);                    // claims 5 name bytes
    const std::string strName = "ab";     // but only 2 follow
    v.insert(v.end(), strName.begin(), strName.end());

    uint256_t   hashGenesis = 0;
    std::string strAccountName;

    REQUIRE(RewardBindingPayload::ParsePayload(v, hashGenesis, strAccountName)
            == RewardBindingPayload::ParseResult::NameLengthMismatch);
}

TEST_CASE("RewardBindingPayload: result strings cover all enumerators",
          "[reward_binding][shared]")
{
    /* Guard against new ParseResult values being added without log mapping. */
    REQUIRE(std::string(RewardBindingPayload::ResultString(
                RewardBindingPayload::ParseResult::Ok)) == "Ok");
    REQUIRE(std::string(RewardBindingPayload::ResultString(
                RewardBindingPayload::ParseResult::TooShort)) == "TooShort");
    REQUIRE(std::string(RewardBindingPayload::ResultString(
                RewardBindingPayload::ParseResult::ExtensionTooShort)) == "ExtensionTooShort");
    REQUIRE(std::string(RewardBindingPayload::ResultString(
                RewardBindingPayload::ParseResult::NameLengthZero)) == "NameLengthZero");
    REQUIRE(std::string(RewardBindingPayload::ResultString(
                RewardBindingPayload::ParseResult::NameLengthMismatch)) == "NameLengthMismatch");
}
