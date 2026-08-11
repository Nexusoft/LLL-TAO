@testset "riscv_detect" begin
    # detect_riscv_capabilities() must run without error on any platform
    caps = detect_riscv_capabilities()

    @test caps isa RiscvCapabilities

    # All fields must be populated
    @test caps.is_riscv isa Bool
    @test caps.has_v_ext isa Bool
    @test caps.has_zba isa Bool
    @test caps.has_zbb isa Bool
    @test caps.hart_count isa Int
    @test caps.xlen isa Int
    @test caps.cache_line_bytes isa Int
    @test caps.detected_at isa String

    # Numeric fields must be sensible
    @test caps.hart_count >= 1
    @test caps.xlen in (32, 64)
    @test caps.cache_line_bytes >= 16

    # Timestamp must be non-empty
    @test !isempty(caps.detected_at)

    # On x86/ARM CI the platform is not RISC-V
    @test caps.is_riscv == false
    @test caps.has_v_ext == false
    @test caps.has_zba == false
    @test caps.has_zbb == false

    # The module-level RISCV constant must also be populated after __init__
    @test RISCV isa RiscvCapabilities
    @test RISCV.is_riscv == false

    # print_riscv_capabilities must produce non-empty output without error
    buf = IOBuffer()
    print_riscv_capabilities(buf)
    output = String(take!(buf))
    @test !isempty(output)
    @test occursin("is_riscv", output)
    @test occursin("hart_count", output)
end
