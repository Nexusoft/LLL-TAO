@testset "parity" begin
    @testset "fixture_privkey" begin
        pk = fixture_privkey()
        @test pk isa Vector{UInt8}
        @test length(pk) == FALCON1024_PRIVKEY_BYTES

        # Verify formula: byte[i] = (i*73 + 19) % 256 for i in 0..2304
        expected = UInt8[UInt8(mod(i * 73 + 19, 256)) for i in 0:(FALCON1024_PRIVKEY_BYTES - 1)]
        @test pk == expected
    end

    @testset "make_parity_fixture" begin
        pk   = fixture_privkey()
        seed = UInt64(0x0000_0000_0000_1024)
        seq  = [2, 3, 4]

        fixture = make_parity_fixture(pk, seed, seq)

        @test fixture isa NamedTuple

        # Required fields
        @test haskey(fixture, :seed)
        @test haskey(fixture, :epoch)
        @test haskey(fixture, :initial_permutation)
        @test haskey(fixture, :final_permutation)
        @test haskey(fixture, :bucket_tags)
        @test haskey(fixture, :working_vector_hex)
        @test haskey(fixture, :swap_log)

        # Epoch must match number of swaps
        @test fixture.epoch == length(seq)

        # Permutations must be length 4
        @test length(fixture.initial_permutation) == BUCKET_COUNT
        @test length(fixture.final_permutation)   == BUCKET_COUNT

        # bucket_tags must be 4 non-empty strings
        @test length(fixture.bucket_tags) == BUCKET_COUNT
        for tag in fixture.bucket_tags
            @test tag isa String
            @test length(tag) == 128   # SHA-512 hex
        end

        # working_vector_hex must be a hex string
        @test fixture.working_vector_hex isa String
        @test !isempty(fixture.working_vector_hex)
        @test all(c -> c in "0123456789abcdef", fixture.working_vector_hex)

        # Swap log must have one entry per swap
        @test length(fixture.swap_log) == length(seq)
    end

    @testset "make_parity_fixture determinism" begin
        pk   = fixture_privkey()
        seed = UInt64(0x0000_0000_0000_1024)
        seq  = [2, 4]

        f1 = make_parity_fixture(pk, seed, seq)
        f2 = make_parity_fixture(pk, seed, seq)

        @test f1.working_vector_hex == f2.working_vector_hex
        @test f1.final_permutation  == f2.final_permutation
        @test f1.swap_log           == f2.swap_log
    end

    @testset "print_cpp_fixture" begin
        pk      = fixture_privkey()
        seed    = UInt64(0x0000_0000_0000_1024)
        fixture = make_parity_fixture(pk, seed, [2, 3])

        io     = IOBuffer()
        print_cpp_fixture(io, fixture)
        output = String(take!(io))

        # Output must be non-empty and contain C++ boilerplate
        @test !isempty(output)
        @test occursin("LLLTaoQTV parity fixture", output)
        @test occursin("FIXTURE_SEED", output)
        @test occursin("FIXTURE_EPOCH", output)
        @test occursin("FIXTURE_INITIAL_PERM", output)
        @test occursin("FIXTURE_FINAL_PERM", output)
        @test occursin("FIXTURE_WORKING_VECTOR_HEX", output)
    end
end
