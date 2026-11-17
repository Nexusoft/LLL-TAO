@testset "bucket" begin
    pk = fixture_privkey()

    @testset "partition_privkey" begin
        buckets = partition_privkey(pk)

        @test buckets isa NTuple{4, FalconBucket}

        # IDs must be 1..4
        @test [b.id for b in buckets] == [1, 2, 3, 4]

        # Offsets must match BUCKET_OFFSETS
        for i in 1:BUCKET_COUNT
            @test buckets[i].start_offset == BUCKET_OFFSETS[i].start
            @test buckets[i].stop_offset  == BUCKET_OFFSETS[i].stop
        end

        # Payload lengths: first bucket gets the remainder byte
        @test length(buckets[1].payload) == 577
        for i in 2:BUCKET_COUNT
            @test length(buckets[i].payload) == 576
        end

        # Tags must be 128-char hex strings (SHA-512 = 64 bytes = 128 hex chars)
        for b in buckets
            @test length(b.tag) == 128
            @test all(c -> c in "0123456789abcdef", b.tag)
        end
    end

    @testset "lossless round-trip" begin
        buckets  = partition_privkey(pk)
        # Reassemble by ordering on start_offset
        ordered  = sort(collect(buckets); by = b -> b.start_offset)
        rebuilt  = vcat([b.payload for b in ordered]...)
        @test rebuilt == pk
    end

    @testset "tag correctness" begin
        buckets = partition_privkey(pk)
        for b in buckets
            @test bucket_sha512_tag(b.payload) == b.tag
        end
    end

    @testset "verify_bucket_tags" begin
        buckets = partition_privkey(pk)
        @test verify_bucket_tags(buckets) == true

        # Tamper with one byte — tag check must fail
        bad_payload = copy(buckets[2].payload)
        bad_payload[1] ⊻= 0xff
        tampered = (
            buckets[1],
            FalconBucket(2, buckets[2].start_offset, buckets[2].stop_offset, bad_payload, buckets[2].tag),
            buckets[3],
            buckets[4],
        )
        @test verify_bucket_tags(tampered) == false
    end

    @testset "bucket_manifest" begin
        buckets  = partition_privkey(pk)
        manifest = bucket_manifest(buckets)

        @test length(manifest) == 4
        @test [m.id for m in manifest]           == [1, 2, 3, 4]
        @test [m.start_offset for m in manifest] == [1, 578, 1154, 1730]
        @test [m.stop_offset for m in manifest]  == [577, 1153, 1729, 2305]
        @test [m.length for m in manifest]        == [577, 576, 576, 576]

        # Tags must be non-empty 128-char strings
        for m in manifest
            @test length(m.tag) == 128
        end
    end

    @testset "invalid input" begin
        @test_throws ArgumentError partition_privkey(zeros(UInt8, 100))
    end
end
