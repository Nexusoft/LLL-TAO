using Test

include(joinpath(@__DIR__, "..", "crypto", "falcon_quantum_tunnel_vector.jl"))
using .FalconQuantumTunnelVector

const FIXED_SEED = 0x0000000000001024
const FIXTURE_MULTIPLIER = 73
const FIXTURE_OFFSET = 19
const FIXTURE_PRIVKEY = UInt8[UInt8(mod((index * FIXTURE_MULTIPLIER) + FIXTURE_OFFSET, 256)) for index in 0:(FALCON1024_PRIVATE_KEY_SIZE - 1)]
const FIXTURE_MESSAGE = collect(codeunits("laser quantum vector handoff"))
const FIXTURE_ENCODED_HEX = "976ab0d8d02789fc12e9a9eda25396817e2336593430ebb0cc243834"
const FIXTURE_WORKING_VECTOR_DIGEST = "16697c4aedac647ba9dc76a7db751fcec2593a723e8af280c16645856a148577a38c8b71c0d8b56b76bfa9da45d22c2c752d9fbf40382d3b08a5a56f4b206b5b"
const FIXTURE_INITIAL_PERMUTATION = [4, 1, 3, 2]
const FIXTURE_FINAL_PERMUTATION = [4, 2, 1, 3]
const FIXTURE_SWAP_LOG = [
    (epoch = 1, from_bucket = 4, to_bucket = 1, slot = 2, permutation = [1, 4, 3, 2]),
    (epoch = 2, from_bucket = 1, to_bucket = 4, slot = 2, permutation = [4, 1, 3, 2]),
    (epoch = 3, from_bucket = 4, to_bucket = 1, slot = 2, permutation = [1, 4, 3, 2]),
    (epoch = 4, from_bucket = 1, to_bucket = 3, slot = 3, permutation = [3, 4, 1, 2]),
    (epoch = 5, from_bucket = 3, to_bucket = 2, slot = 4, permutation = [2, 4, 1, 3]),
    (epoch = 6, from_bucket = 2, to_bucket = 4, slot = 2, permutation = [4, 2, 1, 3]),
]

@testset "Falcon Quantum Tunnel Vector" begin
    @testset "partitioning and round-trip" begin
        qtv = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)

        @test reconstruct_payload(qtv) == FIXTURE_PRIVKEY
        @test [entry.length for entry in bucket_manifest(qtv)] == [577, 576, 576, 576]
        @test [entry.start_offset for entry in bucket_manifest(qtv)] == [1, 578, 1154, 1730]
        @test [entry.stop_offset for entry in bucket_manifest(qtv)] == [577, 1153, 1729, 2305]
        @test decode_working_vector(qtv) == active_bucket(qtv).payload
        @test qtv.working_vector != active_bucket(qtv).payload
        @test verify_bucket_integrity(qtv)
    end

    @testset "reproducible bucket manifest" begin
        left = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)
        right = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)

        @test left.permutation == right.permutation
        @test bucket_manifest(left) == bucket_manifest(right)
        @test left.working_vector == right.working_vector
    end

    @testset "seeded swap sequence stays reproducible" begin
        left = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)
        right = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)

        run_swap_rounds!(left, 6)
        run_swap_rounds!(right, 6)

        @test left.epoch == 6
        @test left.swap_log == right.swap_log
        @test left.permutation == right.permutation
        @test left.working_vector == right.working_vector
        @test decode_working_vector(left) == active_bucket(left).payload
        @test length(left.swap_log) == 6
        @test verify_bucket_integrity(left)
    end

    @testset "manual swap preserves append-only epoching" begin
        qtv = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED, epoch = 11)
        event = swap_active_bucket!(qtv; target_slot = 4)

        @test event.epoch == 12
        @test event.slot == 4
        @test length(qtv.swap_log) == 1
        @test qtv.swap_log[1] == event
        @test decode_working_vector(qtv) == active_bucket(qtv).payload
    end

    @testset "laser tunnel xor is self-inverse for matching qtv state" begin
        qtv = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)
        random_swap_sequence!(qtv, 6)

        encoded = encode_laser_tunnel(FIXTURE_MESSAGE, qtv)

        @test decode_laser_tunnel(encoded, qtv) == FIXTURE_MESSAGE

        qtv_mismatch = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)
        random_swap_sequence!(qtv_mismatch, 5)

        @test decode_laser_tunnel(encoded, qtv_mismatch) != FIXTURE_MESSAGE
    end

    @testset "deterministic fixture records parity handoff values" begin
        fixture = deterministic_laser_fixture(FIXTURE_PRIVKEY, FIXTURE_MESSAGE; seed = FIXED_SEED, n_swaps = 6)

        @test fixture.initial_permutation == FIXTURE_INITIAL_PERMUTATION
        @test fixture.final_permutation == FIXTURE_FINAL_PERMUTATION
        @test fixture.encoded_hex == FIXTURE_ENCODED_HEX
        @test fixture.working_vector_digest == FIXTURE_WORKING_VECTOR_DIGEST
        @test fixture.swap_log == FIXTURE_SWAP_LOG
        @test fixture.epoch == 6
    end
end
