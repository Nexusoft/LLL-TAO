using Test
using LLLTaoQTV

@testset "LLLTaoQTV" begin
    include("test_constants.jl")
    include("test_riscv_detect.jl")
    include("test_bucket.jl")
    include("test_encoding.jl")
    include("test_qtv.jl")
    include("test_parity.jl")
end
