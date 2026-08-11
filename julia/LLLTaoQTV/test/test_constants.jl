@testset "constants" begin
    # Falcon-1024 sizes must match src/LLP/include/falcon_constants.h
    @test FALCON1024_PRIVKEY_BYTES == 2305
    @test FALCON1024_PUBKEY_BYTES  == 1793
    @test FALCON1024_SIG_BYTES     == 1577
    @test BUCKET_COUNT             == 4

    # Bucket offsets must tile exactly over the private key
    @test BUCKET_OFFSETS[1].start == 1
    @test BUCKET_OFFSETS[4].stop  == FALCON1024_PRIVKEY_BYTES

    # Verify contiguous coverage with no gaps or overlaps
    for i in 1:(BUCKET_COUNT - 1)
        @test BUCKET_OFFSETS[i].stop + 1 == BUCKET_OFFSETS[i + 1].start
    end

    # First bucket absorbs the remainder: 2305 % 4 == 1 extra byte
    @test (BUCKET_OFFSETS[1].stop - BUCKET_OFFSETS[1].start + 1) == 577
    for i in 2:BUCKET_COUNT
        @test (BUCKET_OFFSETS[i].stop - BUCKET_OFFSETS[i].start + 1) == 576
    end

    # DOMAIN_TAG_BYTES must be non-empty
    @test !isempty(DOMAIN_TAG_BYTES)
    @test eltype(DOMAIN_TAG_BYTES) == UInt8

    # DEFAULT_SEED must be a UInt64
    @test DEFAULT_SEED isa UInt64
    @test DEFAULT_SEED > 0
end
