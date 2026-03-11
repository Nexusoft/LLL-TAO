@testset "hooks" begin
    @testset "qtv_fixture_case" begin
        baseline = qtv_fixture_case(QTV_HOOK_CASE_BASELINE)
        parity   = qtv_fixture_case(QTV_HOOK_CASE_PARITY)

        @test baseline.seed == UInt64(0x0000_0000_0000_1024)
        @test isempty(baseline.swap_sequence)
        @test baseline.expected_final_permutation === nothing

        @test parity.swap_sequence == [2, 2, 2, 3, 4, 2]
        @test parity.expected_final_permutation == [4, 2, 1, 3]
        @test length(parity.expected_working_vector_digest) == 128

        @test_throws ArgumentError qtv_fixture_case(-1)
    end

    @testset "qtv_run_fixture" begin
        @test qtv_run_fixture(QTV_HOOK_CASE_BASELINE) == QTV_HOOK_STATUS_OK
        @test qtv_run_fixture(QTV_HOOK_CASE_PARITY) == QTV_HOOK_STATUS_OK
        @test qtv_run_fixture(99) == QTV_HOOK_STATUS_INVALID_CASE
    end

    @testset "qtv_compare_parity" begin
        @test qtv_compare_parity(QTV_HOOK_CASE_PARITY) == QTV_HOOK_STATUS_PARITY_MISMATCH
        @test qtv_compare_parity(99) == QTV_HOOK_STATUS_INVALID_CASE
    end
end
