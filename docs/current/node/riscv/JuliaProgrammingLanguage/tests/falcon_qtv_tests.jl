using Test

include(joinpath(@__DIR__, "..", "crypto", "falcon_quantum_tunnel_vector.jl"))
using .FalconQuantumTunnelVector

const FIXED_SEED = 0x0000000000001024
const FIXTURE_PRIVKEY = UInt8[UInt8(mod((index * 73) + 19, 256)) for index in 0:(FALCON1024_PRIVATE_KEY_SIZE - 1)]

@testset "Falcon Quantum Tunnel Vector" begin
    @testset "partitioning and round-trip" begin
        qtv = build_qtv(FIXTURE_PRIVKEY; seed = FIXED_SEED)

        @test reconstruct_payload(qtv) == FIXTURE_PRIVKEY
        @test [entry.length for entry in bucket_manifest(qtv)] == [577, 576, 576, 576]
        @test [entry.start_offset for entry in bucket_manifest(qtv)] == [1, 578, 1154, 1730]
        @test [entry.stop_offset for entry in bucket_manifest(qtv)] == [577, 1153, 1729, 2305]
        @test decode_working_vector(qtv) == active_bucket(qtv).payload
        @test qtv.working_vector != active_bucket(qtv).payload
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
end
