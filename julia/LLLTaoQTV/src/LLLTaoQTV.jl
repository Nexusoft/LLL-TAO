"""
    LLLTaoQTV

Local Julia library wrapping the Falcon-1024 Quantum Tunnel Vector (QTV)
research primitive in LLL-TAO.

At load time, RISC-V CPU features are detected and stored in `RISCV`.
When running on a RISC-V hart with the V extension, hot XOR and SHA-512
chaining paths are routed through SIMD-annotated, cache-aligned code.
On x86/ARM/other hosts, all dispatch paths fall through to plain Julia
with zero overhead.
"""
module LLLTaoQTV

using Dates
using Random: MersenneTwister, randperm, rand
using SHA

include("constants.jl")
include("riscv_detect.jl")
include("riscv_hooks.jl")
include("bucket.jl")
include("encoding.jl")
include("qtv.jl")
include("parity.jl")
include("hooks.jl")

function __init__()
    global RISCV = detect_riscv_capabilities()
    if RISCV.is_riscv
        @info "[LLLTaoQTV] RISC-V hart detected — V-ext optimized paths active" caps=RISCV
    end
end

export FALCON1024_PRIVKEY_BYTES, FALCON1024_PUBKEY_BYTES, FALCON1024_SIG_BYTES,
       BUCKET_COUNT, BUCKET_OFFSETS, DOMAIN_TAG_BYTES, DEFAULT_SEED,
       RiscvCapabilities, RISCV, detect_riscv_capabilities, print_riscv_capabilities,
       FalconBucket, partition_privkey, bucket_manifest, verify_bucket_tags,
       bucket_sha512_tag,
       SwapEvent, QuantumTunnelVector,
       build_qtv, active_bucket, active_bucket_id,
       swap_active_bucket!, run_swap_rounds!, reconstruct_payload, swap_log_summary,
       encode_bucket, decode_working_vector, derive_keystream,
        xor_bytes_riscv, aligned_allocate, dispatch_xor, dispatch_sha512_chain,
        sha512_chain_riscv,
        fixture_privkey, make_parity_fixture, print_cpp_fixture,
        QTV_HOOK_STATUS_OK, QTV_HOOK_STATUS_ERROR, QTV_HOOK_STATUS_INVALID_CASE,
        QTV_HOOK_STATUS_PARITY_MISMATCH, QTV_HOOK_CASE_BASELINE, QTV_HOOK_CASE_PARITY,
        qtv_fixture_case, qtv_run_fixture, qtv_compare_parity

end # module LLLTaoQTV
