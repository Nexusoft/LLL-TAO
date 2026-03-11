# 🟣 Falcon Quantum Tunnel Vector (QTV)

**Section:** Node Architecture → RISC-V → Julia Programming Language → Crypto  
**Last Updated:** 2026-03-11

---

## Overview

The Falcon Quantum Tunnel Vector (QTV) is a Julia research primitive for exploring how a Falcon-1024 private-key payload can be segmented into four near-equal buckets, permuted deterministically, and swapped through a single active working vector. It is designed as a lab model for post-quantum data-channel experiments and not as a production cryptographic construction.

This research page tracks four artifacts:

1. [`crypto/falcon_quantum_tunnel_vector.jl`](falcon_quantum_tunnel_vector.jl) — core prototype
2. [`../tests/falcon_qtv_tests.jl`](../tests/falcon_qtv_tests.jl) — deterministic correctness tests
3. [`../diagrams/09-falcon-quantum-tunnel-vector.txt`](../diagrams/09-falcon-quantum-tunnel-vector.txt) — screenshot-friendly diagram
4. This documentation page — architecture rationale and usage notes

The production C++ implementation continues to use the repository's Falcon-1024 size constants (`1793`-byte public key and `2305`-byte private key) from `src/LLC/include/flkey.h` and `src/LLP/include/falcon_constants.h`.

---

## Key Design Decisions Explained

| Decision | Rationale |
|---|---|
| 4 buckets, not 8 or 16 | Falcon-1024's 2305-byte private key divides cleanly into 4 near-equal segments. More buckets would create sub-256-byte slices, losing meaningful key entropy per segment. |
| SHA-512 per-bucket tag | Matches NexusMiner's Falcon security level and provides deterministic integrity metadata for each immutable bucket. |
| XOR + SHA-512 chaining keystream | Fully invertible, deterministic, and trivial to reproduce for parity testing. It is a research encoding model, not a production cipher. |
| Seeded `MersenneTwister` | Produces reproducible swap sequences so Julia fixtures can be re-run and compared byte-for-byte. |
| Append-only swap log with epoch | Keeps swap history auditable and monotonic. Every swap event records the epoch, source bucket, destination bucket, and resulting permutation. |
| `QuantumTunnelVector` is mutable, `FalconBucket` is immutable | Buckets never change once partitioned. Only the active working vector, permutation state, and swap log evolve over time. |
| `reconstruct_payload` as round-trip guard | The first correctness gate is always `reconstruct_payload(build_qtv(privkey)) == privkey`, proving the partition step is lossless before swap testing begins. |

---

## Concept Architecture

See [`../diagrams/09-falcon-quantum-tunnel-vector.txt`](../diagrams/09-falcon-quantum-tunnel-vector.txt) for the screenshot-friendly diagram. The model is intentionally simple:

1. Partition a 2305-byte Falcon-1024 private-key payload into 4 immutable buckets.
2. SHA-512 each bucket to create a deterministic bucket tag.
3. Seed a `MersenneTwister` to create the initial permutation and future swap choices.
4. Keep one active slot mapped into a working vector.
5. Encode that working vector with `bucket_payload XOR SHA-512-keystream`.
6. Append each swap to a monotonic log for replay and parity testing.

---

## Julia Prototype Surface

The prototype exports a small surface area:

- `build_qtv(privkey; seed, epoch)` — partition payload and initialise the working vector
- `swap_active_bucket!(qtv; target_slot)` — swap a new bucket into the active slot
- `run_swap_rounds!(qtv, rounds)` — execute seeded swap rounds
- `random_swap_sequence!(qtv, rounds)` — explicit seeded swap-sequence alias for fixture generation
- `decode_working_vector(qtv)` — invert the working-vector encoding
- `encode_laser_tunnel(message, qtv)` / `decode_laser_tunnel(message, qtv)` — XOR self-inverse message tunnel keyed by the active working vector
- `reconstruct_payload(qtv)` — rebuild the original payload in bucket order
- `bucket_manifest(qtv)` — inspect bucket boundaries, lengths, and tags
- `swap_log_manifest(qtv)` — serialise swap history into C++-friendly tuples
- `verify_bucket_integrity(qtv)` — recompute SHA-512 bucket tags and assert they still match
- `working_vector_digest(qtv)` — record the current working-vector SHA-512 digest for parity
- `deterministic_laser_fixture(privkey, message; seed, n_swaps)` — package `encoded`, `swap_log`, and `working_vector_digest` for cross-language handoff

Example:

```julia
include("falcon_quantum_tunnel_vector.jl")
using .FalconQuantumTunnelVector

privkey = UInt8[UInt8(mod((i * 73) + 19, 256)) for i in 0:(FALCON1024_PRIVATE_KEY_SIZE - 1)]
qtv = build_qtv(privkey; seed = 0x1024)

reconstruct_payload(qtv) == privkey
run_swap_rounds!(qtv, 6)
decode_working_vector(qtv) == active_bucket(qtv).payload
```

---

## Deterministic Test Expectations

The paired test file [`../tests/falcon_qtv_tests.jl`](../tests/falcon_qtv_tests.jl) enforces the research contract:

- partitioning is lossless
- bucket boundaries are fixed and reproducible
- identical seeds produce identical permutations and swap logs
- manual swaps advance epochs monotonically
- the working vector remains invertible after every swap
- bucket SHA-512 tags remain valid after swap replay
- laser-tunnel XOR decode only succeeds when the receiver reproduces the same QTV state
- the fixed parity fixture remains byte-for-byte stable

These tests are intended to be run locally with Julia:

```bash
julia docs/current/node/riscv/JuliaProgrammingLanguage/tests/falcon_qtv_tests.jl
```

---

## Laser Quantum Data Encoding

The working vector can also drive a simple message tunnel for parity experiments:

```julia
encoded = encode_laser_tunnel(message, qtv)
decoded = decode_laser_tunnel(encoded, qtv)
```

The keystream is produced by repeated SHA-512 chaining over the current working vector. Because the tunnel is XOR-based, `decode_laser_tunnel` is intentionally the same operation as `encode_laser_tunnel`; correctness depends on the receiver recreating the identical bucket permutation and epoch state before decoding.

### Invariants

| Invariant | Enforcement |
|---|---|
| Bucket SHA-512 tags verify | `verify_bucket_integrity(qtv)` |
| Full round-trip fidelity | `reconstruct_payload(qtv) == original_privkey` |
| Deterministic from seed | `random_swap_sequence!` uses `MersenneTwister(seed)` |
| Monotonic epoch | swap log epochs strictly increase |
| XOR self-inverse | `decode_laser_tunnel == encode_laser_tunnel` with the same QTV state |

---

## Fixed Parity Fixture

The deterministic fixture used by the Julia tests and the C++ parity test is:

- `privkey_payload[i] = UInt8(mod((i * 73) + 19, 256))` for `i in 0:2304`
- `seed = 0x0000000000001024`
- `n_swaps = 6`
- `message = "laser quantum vector handoff"`

Recorded outputs:

- `initial_permutation = [4, 1, 3, 2]`
- `final_permutation = [4, 2, 1, 3]`
- `encoded = 976ab0d8d02789fc12e9a9eda25396817e2336593430ebb0cc243834`
- `working_vector_digest = 16697c4aedac647ba9dc76a7db751fcec2593a723e8af280c16645856a148577a38c8b71c0d8b56b76bfa9da45d22c2c752d9fbf40382d3b08a5a56f4b206b5b`
- `swap_slots = [2, 2, 2, 3, 4, 2]`

The Julia fixture is recorded in [`../tests/falcon_qtv_tests.jl`](../tests/falcon_qtv_tests.jl), and the matching C++ replay test lives at [`../../../../../../tests/unit/LLC/falcon_qtv_parity_test.cpp`](../../../../../../tests/unit/LLC/falcon_qtv_parity_test.cpp).

---

## C++ Parity and Benchmark Gate

Before a Julia-side result informs production C++, three artifacts are required:

1. a fixed fixture (`encoded`, `swap_log`, `working_vector_digest`)
2. a C++ parity replay that reproduces the bucket partitioning and digest byte-for-byte
3. a benchmark note for the same fixture payload size

Current local baseline for the fixed 28-byte message above:

| Runtime | Command | Result |
|---|---|---|
| Julia 1.12.5 | `julia docs/current/node/riscv/JuliaProgrammingLanguage/tests/falcon_qtv_tests.jl` + a 200,000-iteration `encode_laser_tunnel` loop | ~8,357 ns / call |
| C++17 (`-O3`, OpenSSL SHA-512) | local replay of the same fixture logic | ~3,067 ns / call |

Measurement notes:

- both runtimes used the same fixed 28-byte message and the same 200,000-iteration loop
- Julia timing used `@elapsed`; the C++ timing used `std::chrono::steady_clock`
- local hardware snapshot: `x86_64`, 4 visible cores, `AMD EPYC 7763 64-Core Processor`

These timings are reference notes only. They are intentionally non-normative and should be re-measured whenever the tunnel algorithm, compiler flags, or payload size changes.

---

## Research Boundary

This model is deliberately limited to deterministic structural experiments. It does **not** replace Falcon signing, Nexus key handling, or any production encryption path. Promotion into C++ would require explicit parity tests, code review, and security analysis beyond this lab prototype.
