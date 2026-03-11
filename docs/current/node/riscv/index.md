# RISC-V Node Deployment — Overview

**Section:** Node Architecture → RISC-V  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-11

---

## Purpose

This section documents RISC-V-specific considerations for building, running, and debugging the Nexus LLL-TAO node.  The node is designed to compile and run without source-code changes on any RISC-V RV64GC system.

---

## Pages in This Section

| Document | Purpose |
|---|---|
| **[Endianness & Serialisation](endianness-serialization.md)** | Byte-order invariants; how packet fields are laid out |
| **[Atomic Operations & Locking](atomic-locking.md)** | Memory model differences on RISC-V; mutex / atomic best practices |
| **[Diagnostics & Testing Notes](diagnostics.md)** | How to verify correct operation on RISC-V hardware |
| **[🟣 Julia Programming Language](JuliaProgrammingLanguage/README.md)** | Research lab for Julia tooling, interop, tests, diagrams, and local binaries |

---

## Architecture Summary

Nexus LLL-TAO targets **RV64GC** (64-bit, IMAFD base + Compressed).  Optional vector extensions (RVV) may be used by OpenSSL for hardware-accelerated cryptography but are never required by node source code.

```
┌───────────────────────────────────────────────────────────────┐
│  RISC-V RV64GC Node Build                                     │
│                                                               │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │  C++17 Source (architecture-agnostic)                   │  │
│  │  src/LLP/  src/TAO/  src/LLC/  src/Legacy/              │  │
│  └─────────────────────────────────────────────────────────┘  │
│                        │                                      │
│            ┌───────────┴───────────┐                          │
│            ▼                       ▼                          │
│  ┌──────────────────┐   ┌──────────────────────────────────┐  │
│  │  OpenSSL 1.1.1+  │   │  Boost 1.71+ / BerkeleyDB 5.3   │  │
│  │  (RV64 backend   │   │  (portable, no arch-specific     │  │
│  │   with optional  │   │   optimisations required)        │  │
│  │   Zbkb/Zbkc/RVV) │   │                                  │  │
│  └──────────────────┘   └──────────────────────────────────┘  │
│                        │                                      │
│            ┌───────────┴───────────┐                          │
│            ▼                       ▼                          │
│  ┌──────────────────┐   ┌──────────────────────────────────┐  │
│  │  Falcon-1024     │   │  ChaCha20-Poly1305               │  │
│  │  (via LLC /      │   │  (via OpenSSL EVP or built-in    │  │
│  │   liboqs)        │   │   constant-time impl)            │  │
│  └──────────────────┘   └──────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

---

## Build Quick-Start (RISC-V cross-compile on x86 host)

```bash
# Install cross-toolchain on Ubuntu 22.04 host
sudo apt-get install -y gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
    libboost-all-dev:riscv64 libssl-dev:riscv64 libdb5.3++-dev:riscv64

# Cross-compile the node
export CC=riscv64-linux-gnu-gcc
export CXX=riscv64-linux-gnu-g++
export CROSS_COMPILE=riscv64-linux-gnu-
make -j$(nproc)
```

For native builds on a RISC-V board (e.g. StarFive VisionFive 2, SiFive HiFive Unmatched):

```bash
# Same as x86 — no source changes needed
sudo apt-get install -y build-essential libboost-all-dev libssl-dev libdb5.3++-dev
make -j$(nproc)
```

See [docs/build-linux.md](../../../build-linux.md) for full dependency list.  
See [docs/riscv-build-guide.md](../../../riscv-build-guide.md) for RISC-V-specific compiler flag guidance.

---

## Key Portability Guarantees

| Feature | Guarantee |
|---|---|
| Byte order | All protocol integers use explicit little-endian serialisation; no `htonl` / `ntohl` |
| Alignment | No packed structs relying on architecture alignment; all fields explicitly offset |
| Atomics | `std::atomic` throughout; no platform intrinsics in application code |
| Crypto | Delegated entirely to OpenSSL / liboqs; RISC-V backends activated at runtime |
| Float / double | Not used in consensus-critical code; all mining arithmetic is integer |

---

## Hardware Tested

| Board | CPU | Status |
|---|---|---|
| SiFive HiFive Unmatched | U74 (RV64GC) | Build tested |
| StarFive VisionFive 2 | JH7110 (RV64GC) | Build tested |
| QEMU virt (RV64GC) | Emulated | CI cross-compile tested |

---

## Related Pages

- [Endianness & Serialisation](endianness-serialization.md)
- [Atomic Operations & Locking](atomic-locking.md)
- [Diagnostics & Testing Notes](diagnostics.md)
- [🟣 Julia Programming Language](JuliaProgrammingLanguage/README.md)
- [RISC-V Design (top-level)](../../../riscv-design.md)
- [RISC-V Build Guide](../../../riscv-build-guide.md)
- [Node Architecture Index](../index.md)
