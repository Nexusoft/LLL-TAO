/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <LLC/include/falcon_constants_v2.h>

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::size_t BUCKET_COUNT = 4;
    constexpr std::uint64_t FIXED_SEED = 0x0000000000001024ULL;
    constexpr std::size_t FIXTURE_MULTIPLIER = 73;
    constexpr std::size_t FIXTURE_OFFSET = 19;

    const std::array<int, BUCKET_COUNT> FIXTURE_INITIAL_PERMUTATION = {{4, 1, 3, 2}};
    const std::array<int, BUCKET_COUNT> FIXTURE_FINAL_PERMUTATION = {{4, 2, 1, 3}};
    const std::array<int, 6> FIXTURE_SWAP_TARGET_SLOTS = {{2, 2, 2, 3, 4, 2}};
    const std::string FIXTURE_MESSAGE = "laser quantum vector handoff";
    const std::string FIXTURE_ENCODED_HEX = "976ab0d8d02789fc12e9a9eda25396817e2336593430ebb0cc243834";
    const std::string FIXTURE_WORKING_VECTOR_DIGEST =
        "16697c4aedac647ba9dc76a7db751fcec2593a723e8af280c16645856a148577"
        "a38c8b71c0d8b56b76bfa9da45d22c2c752d9fbf40382d3b08a5a56f4b206b5b";

    struct Bucket
    {
        int id;
        std::size_t start_offset;
        std::size_t stop_offset;
        std::vector<std::uint8_t> payload;
        std::string tag;
    };

    struct QuantumTunnelVectorFixture
    {
        std::vector<Bucket> buckets;
        std::array<int, BUCKET_COUNT> permutation;
        std::vector<std::uint8_t> working_vector;
        std::uint64_t seed;
        int epoch;
    };

    std::vector<std::uint8_t> EncodeUInt64LittleEndian(std::uint64_t value)
    {
        std::vector<std::uint8_t> bytes(8, 0);

        for(std::size_t index = 0; index < bytes.size(); ++index)
            bytes[index] = static_cast<std::uint8_t>((value >> (8 * index)) & 0xffU);

        return bytes;
    }

    std::vector<std::uint8_t> Concat(const std::vector<std::vector<std::uint8_t>>& chunks)
    {
        std::size_t total_size = 0;
        for(const auto& chunk : chunks)
            total_size += chunk.size();

        std::vector<std::uint8_t> combined;
        combined.reserve(total_size);

        for(const auto& chunk : chunks)
            combined.insert(combined.end(), chunk.begin(), chunk.end());

        return combined;
    }

    std::vector<std::uint8_t> Sha512(const std::vector<std::uint8_t>& data)
    {
        std::vector<std::uint8_t> digest(SHA512_DIGEST_LENGTH, 0);
        SHA512(data.data(), data.size(), digest.data());
        return digest;
    }

    std::string HexEncode(const std::vector<std::uint8_t>& data)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');

        for(const auto byte : data)
            stream << std::setw(2) << static_cast<unsigned int>(byte);

        return stream.str();
    }

    std::vector<std::uint8_t> XorBytes(const std::vector<std::uint8_t>& payload, const std::vector<std::uint8_t>& keystream)
    {
        if(payload.size() != keystream.size())
            throw std::invalid_argument("payload and keystream lengths must match");

        std::vector<std::uint8_t> encoded(payload.size(), 0);

        for(std::size_t index = 0; index < payload.size(); ++index)
            encoded[index] = payload[index] ^ keystream[index];

        return encoded;
    }

    std::vector<std::uint8_t> DeriveBucketKeystream(std::size_t length_bytes, std::uint64_t seed, int epoch, int bucket_id, const std::string& tag)
    {
        const std::vector<std::uint8_t> domain_tag = {'F', 'a', 'l', 'c', 'o', 'n', 'Q', 'T', 'V'};
        std::vector<std::uint8_t> state = Sha512(
            Concat({
                domain_tag,
                EncodeUInt64LittleEndian(seed),
                EncodeUInt64LittleEndian(static_cast<std::uint64_t>(epoch)),
                EncodeUInt64LittleEndian(static_cast<std::uint64_t>(bucket_id)),
                std::vector<std::uint8_t>(tag.begin(), tag.end()),
            })
        );

        std::vector<std::uint8_t> stream;
        stream.reserve(length_bytes);

        for(std::uint64_t counter = 0; stream.size() < length_bytes; ++counter)
        {
            state = Sha512(Concat({state, EncodeUInt64LittleEndian(counter)}));
            stream.insert(stream.end(), state.begin(), state.end());
        }

        stream.resize(length_bytes);
        return stream;
    }

    std::vector<std::uint8_t> DeriveLaserKeystream(std::size_t length_bytes, const std::vector<std::uint8_t>& working_vector)
    {
        std::vector<std::uint8_t> state = Sha512(working_vector);
        std::vector<std::uint8_t> stream;
        stream.reserve(length_bytes);

        for(std::uint64_t counter = 0; stream.size() < length_bytes; ++counter)
        {
            state = Sha512(Concat({state, working_vector, EncodeUInt64LittleEndian(counter)}));
            stream.insert(stream.end(), state.begin(), state.end());
        }

        stream.resize(length_bytes);
        return stream;
    }

    std::vector<std::uint8_t> BuildFixturePrivkey()
    {
        std::vector<std::uint8_t> privkey(LLC::FalconSizes::FALCON1024_PRIVATE_KEY_SIZE, 0);

        for(std::size_t index = 0; index < privkey.size(); ++index)
            privkey[index] = static_cast<std::uint8_t>(((index * FIXTURE_MULTIPLIER) + FIXTURE_OFFSET) % 256);

        return privkey;
    }

    std::vector<Bucket> BuildBuckets(const std::vector<std::uint8_t>& privkey)
    {
        std::vector<Bucket> buckets;
        buckets.reserve(BUCKET_COUNT);

        const std::size_t base = privkey.size() / BUCKET_COUNT;
        const std::size_t remainder = privkey.size() % BUCKET_COUNT;

        std::size_t slice_start = 0;

        for(std::size_t bucket_index = 0; bucket_index < BUCKET_COUNT; ++bucket_index)
        {
            const std::size_t bucket_length = base + (bucket_index < remainder ? 1 : 0);
            const std::size_t slice_stop_exclusive = slice_start + bucket_length;

            Bucket bucket;
            bucket.id = static_cast<int>(bucket_index + 1);
            /* Mirror Julia's 1-based inclusive offset metadata while keeping C++ slices 0-based. */
            bucket.start_offset = slice_start + 1;
            bucket.stop_offset = slice_stop_exclusive;
            bucket.payload.assign(privkey.begin() + slice_start, privkey.begin() + slice_stop_exclusive);
            bucket.tag = HexEncode(Sha512(bucket.payload));

            buckets.push_back(std::move(bucket));
            slice_start = slice_stop_exclusive;
        }

        return buckets;
    }

    const Bucket& ActiveBucket(const QuantumTunnelVectorFixture& qtv)
    {
        return qtv.buckets.at(static_cast<std::size_t>(qtv.permutation[0] - 1));
    }

    void RefreshWorkingVector(QuantumTunnelVectorFixture& qtv)
    {
        const Bucket& bucket = ActiveBucket(qtv);
        qtv.working_vector = XorBytes(bucket.payload, DeriveBucketKeystream(bucket.payload.size(), qtv.seed, qtv.epoch, bucket.id, bucket.tag));
    }

    QuantumTunnelVectorFixture BuildFixtureQtv()
    {
        QuantumTunnelVectorFixture qtv;
        qtv.buckets = BuildBuckets(BuildFixturePrivkey());
        qtv.permutation = FIXTURE_INITIAL_PERMUTATION;
        qtv.seed = FIXED_SEED;
        qtv.epoch = 0;
        RefreshWorkingVector(qtv);
        return qtv;
    }

    void SwapActiveBucket(QuantumTunnelVectorFixture& qtv, int target_slot)
    {
        const std::size_t zero_based_target = static_cast<std::size_t>(target_slot - 1);
        std::swap(qtv.permutation[0], qtv.permutation[zero_based_target]);
        ++qtv.epoch;
        RefreshWorkingVector(qtv);
    }
}

TEST_CASE("Falcon QTV parity fixture matches Julia bucket partitioning", "[falcon][qtv][parity]")
{
    const auto qtv = BuildFixtureQtv();
    const auto privkey = BuildFixturePrivkey();

    REQUIRE(qtv.buckets.size() == BUCKET_COUNT);
    REQUIRE(qtv.buckets[0].start_offset == 1);
    REQUIRE(qtv.buckets[0].stop_offset == 577);
    REQUIRE(qtv.buckets[1].start_offset == 578);
    REQUIRE(qtv.buckets[1].stop_offset == 1153);
    REQUIRE(qtv.buckets[2].start_offset == 1154);
    REQUIRE(qtv.buckets[2].stop_offset == 1729);
    REQUIRE(qtv.buckets[3].start_offset == 1730);
    REQUIRE(qtv.buckets[3].stop_offset == 2305);
    REQUIRE(qtv.permutation == FIXTURE_INITIAL_PERMUTATION);

    std::size_t covered_bytes = 0;
    for(std::size_t index = 0; index < qtv.buckets.size(); ++index)
    {
        const auto& bucket = qtv.buckets[index];
        REQUIRE((bucket.stop_offset - bucket.start_offset + 1) == bucket.payload.size());
        covered_bytes += bucket.payload.size();

        if(index + 1 < qtv.buckets.size())
            REQUIRE(bucket.stop_offset + 1 == qtv.buckets[index + 1].start_offset);
    }

    REQUIRE(covered_bytes == privkey.size());
}

TEST_CASE("Falcon QTV parity fixture reproduces Julia digest and encoded tunnel", "[falcon][qtv][parity]")
{
    auto qtv = BuildFixtureQtv();

    for(const int slot : FIXTURE_SWAP_TARGET_SLOTS)
        SwapActiveBucket(qtv, slot);

    REQUIRE(qtv.epoch == 6);
    REQUIRE(qtv.permutation == FIXTURE_FINAL_PERMUTATION);
    REQUIRE(HexEncode(Sha512(qtv.working_vector)) == FIXTURE_WORKING_VECTOR_DIGEST);

    const std::vector<std::uint8_t> message(FIXTURE_MESSAGE.begin(), FIXTURE_MESSAGE.end());
    const auto encoded = XorBytes(message, DeriveLaserKeystream(message.size(), qtv.working_vector));

    REQUIRE(HexEncode(encoded) == FIXTURE_ENCODED_HEX);
}
