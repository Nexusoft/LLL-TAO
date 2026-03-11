@testset "qtv" begin
    pk        = fixture_privkey()
    seed      = UInt64(0x0000_0000_0000_1024)

    @testset "build_qtv" begin
        qtv = build_qtv(pk; seed=seed)

        @test qtv isa QuantumTunnelVector
        @test length(qtv.permutation) == BUCKET_COUNT
        @test qtv.active_slot == 1
        @test qtv.epoch == 0
        @test isempty(qtv.swap_log)
        @test !isempty(qtv.working_vector)
        @test length(qtv.working_vector) == length(active_bucket(qtv).payload)
    end

    @testset "reconstruct_payload" begin
        qtv = build_qtv(pk; seed=seed)
        @test reconstruct_payload(qtv) == pk
    end

    @testset "active_bucket / active_bucket_id" begin
        qtv = build_qtv(pk; seed=seed)
        id  = active_bucket_id(qtv)
        @test 1 <= id <= BUCKET_COUNT
        @test active_bucket(qtv).id == id
    end

    @testset "working_vector decode" begin
        qtv  = build_qtv(pk; seed=seed)
        ab   = active_bucket(qtv)
        wv   = qtv.working_vector
        decoded = decode_working_vector(wv, ab, qtv.seed, qtv.epoch)
        @test decoded == ab.payload
        # Working vector differs from raw payload (it is XOR-encoded)
        @test wv != ab.payload
    end

    @testset "swap_active_bucket!" begin
        qtv = build_qtv(pk; seed=seed, epoch=5)

        ev = swap_active_bucket!(qtv; target_slot=3)

        @test ev isa SwapEvent
        @test ev.epoch == 6
        @test ev.slot  == 3
        @test length(qtv.swap_log) == 1
        @test qtv.swap_log[1] === ev
        @test qtv.epoch == 6

        # Working vector decodes back to active bucket payload
        decoded = decode_working_vector(qtv.working_vector, active_bucket(qtv), qtv.seed, qtv.epoch)
        @test decoded == active_bucket(qtv).payload
    end

    @testset "epoch monotonicity" begin
        qtv = build_qtv(pk; seed=seed)
        prev_epoch = qtv.epoch
        for _ in 1:6
            swap_active_bucket!(qtv)
            @test qtv.epoch == prev_epoch + 1
            prev_epoch = qtv.epoch
        end
        @test qtv.epoch == 6
        @test length(qtv.swap_log) == 6
    end

    @testset "run_swap_rounds!" begin
        left  = build_qtv(pk; seed=seed)
        right = build_qtv(pk; seed=seed)

        run_swap_rounds!(left, 6)
        run_swap_rounds!(right, 6)

        @test left.epoch == 6
        @test left.permutation == right.permutation
        @test left.working_vector == right.working_vector
        @test left.swap_log == right.swap_log
    end

    @testset "reproducibility" begin
        qtv1 = build_qtv(pk; seed=seed)
        qtv2 = build_qtv(pk; seed=seed)

        @test qtv1.permutation == qtv2.permutation
        @test qtv1.working_vector == qtv2.working_vector
    end

    @testset "swap_log integrity" begin
        qtv = build_qtv(pk; seed=seed)
        run_swap_rounds!(qtv, 4)

        # Epochs in log must be 1..4 in order
        @test [ev.epoch for ev in qtv.swap_log] == 1:4
        # from_bucket and to_bucket must be valid ids
        for ev in qtv.swap_log
            @test 1 <= ev.from_bucket <= BUCKET_COUNT
            @test 1 <= ev.to_bucket   <= BUCKET_COUNT
            @test ev.from_bucket != ev.to_bucket
            @test length(ev.permutation) == BUCKET_COUNT
        end
    end

    @testset "swap_log_summary" begin
        qtv = build_qtv(pk; seed=seed)
        run_swap_rounds!(qtv, 2)
        summary = swap_log_summary(qtv)
        @test summary isa String
        @test !isempty(summary)
        @test occursin("epoch=2", summary)
    end

    @testset "invalid input" begin
        @test_throws ArgumentError build_qtv(zeros(UInt8, 100); seed=seed)
        qtv = build_qtv(pk; seed=seed)
        # Cannot swap to same slot
        @test_throws ArgumentError swap_active_bucket!(qtv; target_slot=qtv.active_slot)
        # Out of range slot
        @test_throws ArgumentError swap_active_bucket!(qtv; target_slot=0)
        @test_throws ArgumentError swap_active_bucket!(qtv; target_slot=BUCKET_COUNT + 1)
    end
end
