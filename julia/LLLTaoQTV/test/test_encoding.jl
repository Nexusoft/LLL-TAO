@testset "encoding" begin
    pk   = fixture_privkey()
    seed = UInt64(0x0000_0000_0000_1024)

    @testset "derive_keystream" begin
        ks = derive_keystream(64, seed, 0, 1, "test_tag")
        @test ks isa Vector{UInt8}
        @test length(ks) == 64

        # Exact length is respected (not rounded up)
        ks37 = derive_keystream(37, seed, 0, 1, "test_tag")
        @test length(ks37) == 37

        # Zero length returns empty
        @test derive_keystream(0, seed, 0, 1, "test_tag") == UInt8[]

        # Deterministic: same inputs → same output
        @test derive_keystream(64, seed, 0, 1, "test_tag") == ks

        # Different epoch → different keystream
        @test derive_keystream(64, seed, 1, 1, "test_tag") != ks

        # Different bucket_id → different keystream
        @test derive_keystream(64, seed, 0, 2, "test_tag") != ks

        # Different tag → different keystream
        @test derive_keystream(64, seed, 0, 1, "other_tag") != ks
    end

    @testset "encode_bucket / decode_working_vector round-trip" begin
        buckets = partition_privkey(pk)
        b       = buckets[1]
        epoch   = 3

        encoded = encode_bucket(b, seed, epoch)
        @test length(encoded) == length(b.payload)
        @test encoded != b.payload   # encoding must change the bytes

        decoded = decode_working_vector(encoded, b, seed, epoch)
        @test decoded == b.payload   # decode must recover original

        # Wrong epoch → wrong decode
        wrong_decoded = decode_working_vector(encoded, b, seed, epoch + 1)
        @test wrong_decoded != b.payload
    end

    @testset "determinism across separate calls" begin
        buckets = partition_privkey(pk)
        b       = buckets[2]

        enc1 = encode_bucket(b, seed, 7)
        enc2 = encode_bucket(b, seed, 7)
        @test enc1 == enc2
    end

    @testset "different epochs produce different encodings" begin
        buckets = partition_privkey(pk)
        b       = buckets[1]

        enc0 = encode_bucket(b, seed, 0)
        enc1 = encode_bucket(b, seed, 1)
        @test enc0 != enc1
    end

    @testset "RISC-V dispatch_xor exercises fallback path on x86" begin
        a = UInt8[1, 2, 3, 4]
        b = UInt8[5, 6, 7, 8]
        result = dispatch_xor(a, b)
        @test result == xor.(a, b)
        @test result isa Vector{UInt8}
    end

    @testset "xor_bytes_riscv fallback on non-RISC-V" begin
        # On x86, RISCV.has_v_ext == false → uses xor.() fallback
        a = rand(UInt8, 128)
        b = rand(UInt8, 128)
        @test xor_bytes_riscv(a, b) == xor.(a, b)
    end

    @testset "aligned_allocate returns correct size on non-RISC-V" begin
        # On non-RISC-V, returns exactly n_bytes
        buf = aligned_allocate(100)
        @test length(buf) == 100
        @test eltype(buf) == UInt8
    end
end
