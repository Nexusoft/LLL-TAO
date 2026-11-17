# 🟣 Build Local Julia Library Binary

**Last Updated:** 2026-03-11

A local Julia library build is appropriate only when a narrow, well-tested interop boundary is needed. Document exported symbols, ABI assumptions, test fixtures, and cleanup steps before any downstream consumer is allowed to depend on the library.

For the Falcon QTV prototype, the safe handoff boundary is the deterministic fixture surface:

- `deterministic_laser_fixture(...)`
- `encode_laser_tunnel(...)`
- `decode_laser_tunnel(...)`
- `qtv_run_fixture(case_id)`
- `qtv_compare_parity(case_id)`

Those entry points keep the Julia/C++ exchange limited to byte vectors, swap-log tuples, and SHA-512 digests. Any local `PackageCompiler.create_library(...)` experiment should preserve that boundary so the C++ parity test can continue to replay the same vectors byte-for-byte.

The current repository shape for that boundary is:

- Julia exports in `julia/LLLTaoQTV/src/hooks.jl`
  - `qtv_fixture_case(case_id)`
  - `Base.@ccallable qtv_run_fixture(::Cint)`
  - `Base.@ccallable qtv_compare_parity(::Cint)`
- C++ adapters in `src/LLC/include/`
  - `qtv_julia_bridge.h`
  - `qtv_engine.h`

Keep those hooks research-only. Do not route production miner networking,
session ownership, or live submission paths through libjulia.
