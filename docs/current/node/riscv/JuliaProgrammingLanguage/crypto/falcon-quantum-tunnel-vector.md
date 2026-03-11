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
- `decode_working_vector(qtv)` — invert the working-vector encoding
- `reconstruct_payload(qtv)` — rebuild the original payload in bucket order
- `bucket_manifest(qtv)` — inspect bucket boundaries, lengths, and tags

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

These tests are intended to be run locally with Julia:

```bash
julia docs/current/node/riscv/JuliaProgrammingLanguage/tests/falcon_qtv_tests.jl
```

---

## Research Boundary

This model is deliberately limited to deterministic structural experiments. It does **not** replace Falcon signing, Nexus key handling, or any production encryption path. Promotion into C++ would require explicit parity tests, code review, and security analysis beyond this lab prototype.
