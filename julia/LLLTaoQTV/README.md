# LLLTaoQTV

A local Julia library package wrapping the Falcon-1024 Quantum Tunnel Vector
(QTV) research primitive in LLL-TAO, with built-in RISC-V optimization hooks.

> **Research boundary:** This is lab-code only. No C++ production files are
> modified. The Falcon-1024 C++ node paths in `src/LLP/` and `src/LLC/` remain
> untouched.

---

## Purpose

`LLLTaoQTV` provides a pure-Julia, installable package for experimenting with
the QTV key-management primitive described in PR #376. It:

- Partitions a 2305-byte Falcon-1024 private key into 4 cryptographically
  tagged `FalconBucket`s
- Maintains a `QuantumTunnelVector` (QTV) that rotates the active bucket via
  seeded, reproducible swap rounds
- Derives XOR-based working vectors via SHA-512 keystream chaining
- Detects RISC-V CPU features at load time and activates optimized dispatch
  paths when the V vector extension is present
- Generates C++ parity fixtures for cross-language round-trip validation

---

## RISC-V Optimization Hooks

At library load time `__init__()` calls `detect_riscv_capabilities()`, which:

1. Checks `Sys.ARCH` for `:riscv64` / `:riscv32`
2. On Linux RISC-V, reads `/proc/cpuinfo` and parses `isa:` lines to detect the
   V extension, Zba, and Zbb bit-manipulation extensions
3. Stores results in the module-level constant `RISCV::RiscvCapabilities`

When `RISCV.is_riscv == true`:

| Hot path | Optimization |
|----------|-------------|
| XOR (`dispatch_xor`) | `@simd @inbounds` loop with cache-line-aligned tmp buffer (VLEN=512 stripe) |
| SHA-512 chaining (`dispatch_sha512_chain`) | Input buffer aligned to 128-byte boundary |
| Working-buffer allocation (`aligned_allocate`) | Rounded up to `RISCV.cache_line_bytes` multiple |

**Zero overhead on non-RISC-V:** all dispatch paths fall through to plain Julia
when `RISCV.is_riscv == false`. No external RISC-V packages are required.

---

## Installation

From a Julia REPL pointed at this directory:

```julia
julia --project=. -e 'using Pkg; Pkg.instantiate()'
```

All dependencies (`SHA`, `Random`, `Dates`, `Test`) are Julia standard-library
packages and require no external downloads.

---

## Usage

```julia
julia --project=.

using LLLTaoQTV

# Build a QTV from a fixture private key
pk  = fixture_privkey()              # 2305-byte deterministic fixture
qtv = build_qtv(pk; seed=DEFAULT_SEED)

println("Initial permutation: ", qtv.permutation)

# Run 3 swap rounds
run_swap_rounds!(qtv, 3)
println("After 3 swaps, epoch=", qtv.epoch)

# Decode the working vector back to the active bucket payload
ab      = active_bucket(qtv)
decoded = decode_working_vector(qtv.working_vector, ab, qtv.seed, qtv.epoch)
@assert decoded == ab.payload

# Reconstruct the full private key
@assert reconstruct_payload(qtv) == pk

# Print RISC-V capability report
print_riscv_capabilities()

# Show swap log
println(swap_log_summary(qtv))
```

---

## Running Tests

```bash
cd julia/LLLTaoQTV
make test
```

Or directly via Julia:

```bash
julia --project=. -e 'using Pkg; Pkg.test()'
```

All tests pass on x86 by exercising non-RISC-V fallback code paths.

---

## Building a System Image

Requires [PackageCompiler.jl](https://github.com/JuliaLang/PackageCompiler.jl):

```bash
julia --project=. -e 'using Pkg; Pkg.add("PackageCompiler")'
make sysimage
```

This produces `LLLTaoQTV.so`. Load it with:

```bash
julia --sysimage LLLTaoQTV.so --project=. -e 'using LLLTaoQTV'
```

---

## C++ Parity Fixture Workflow

Generate a parity fixture from Julia and embed it in a C++ test:

```julia
using LLLTaoQTV

pk      = fixture_privkey()
seed    = UInt64(0x0000_0000_0000_1024)
fixture = make_parity_fixture(pk, seed, [2, 3, 4])   # explicit swap sequence

# Print C++ initializers to stdout
print_cpp_fixture(stdout, fixture)
```

Copy the printed constants into a new test file under `src/LLP/tests/` and use
them to validate that the C++ QTV re-implementation produces identical results.

---

## File Tree

```
julia/LLLTaoQTV/
├── Project.toml          ← package metadata (name, uuid, deps, compat)
├── Manifest.toml         ← locked stdlib dependency manifest
├── Makefile              ← make test / make precompile / make sysimage
├── README.md             ← this file
├── src/
│   ├── LLLTaoQTV.jl     ← top-level module, re-exports everything
│   ├── constants.jl      ← Falcon-1024 sizes, bucket offsets, domain tag
│   ├── riscv_detect.jl   ← RiscvCapabilities, detect_riscv_capabilities()
│   ├── riscv_hooks.jl    ← RISC-V optimized XOR + SHA-512 dispatch
│   ├── bucket.jl         ← FalconBucket, partition_privkey, verify_bucket_tags
│   ├── encoding.jl       ← derive_keystream, encode_bucket, decode_working_vector
│   ├── qtv.jl            ← QuantumTunnelVector, build_qtv, swap engine
│   └── parity.jl         ← fixture_privkey, make_parity_fixture, print_cpp_fixture
└── test/
    ├── runtests.jl       ← @testset orchestrator
    ├── test_constants.jl
    ├── test_riscv_detect.jl
    ├── test_bucket.jl
    ├── test_encoding.jl
    ├── test_qtv.jl
    └── test_parity.jl
```

---

## Key Constants (must match `src/LLP/include/falcon_constants.h`)

| Constant | Value |
|----------|-------|
| `FALCON1024_PRIVKEY_BYTES` | 2305 |
| `FALCON1024_PUBKEY_BYTES`  | 1793 |
| `FALCON1024_SIG_BYTES`     | 1577 |
| `BUCKET_COUNT`             | 4    |
