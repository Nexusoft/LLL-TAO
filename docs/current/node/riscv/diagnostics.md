# Diagnostics & Testing Notes (RISC-V)

**Section:** Node Architecture → RISC-V  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-10

---

## Overview

This page documents the diagnostics available for verifying correct node operation on RISC-V hardware and describes the RISC-V-specific considerations for the test strategy.

---

## Runtime Diagnostics

### Session Diagnostics Log Block

On every `MINER_AUTH` and `SUBMIT_BLOCK`, the node emits a structured diagnostics block.  On RISC-V the output should be identical to x86; any difference indicates a serialisation or endianness bug.

Example (stateless lane auth):

```
[DIAG][AUTH] miner session established
  session_id          = 0x1A2B3C4D
  genesis_hash        = 9f3a...c1e2  (32 bytes, LE hex)
  falcon_key_id       = sha256:ab12...  (fingerprint)
  chacha20_key_hash   = sha256:cd34...  (fingerprint)
  reward_bound        = NO
  lane                = STATELESS
  remote_addr         = 192.168.1.100
  arch_selfcheck      = PASS
```

The `arch_selfcheck` field is set by a compile-time self-test that verifies the byte layout of a known-value `MinerSessionContainer` matches the expected serialised form.  If this field shows `FAIL`, there is an architecture-specific serialisation bug.

### Enabling Maximum Verbosity

```bash
./nexus -verbose=5 -mining -testnet 2>&1 | grep -E '\[DIAG\]|\[RECOVERY\]|\[CRYPTO\]'
```

### Reconnect Accounting Log

After a successful recovery merge, the node logs:

```
[RECOVERY] miner reconnected
  falcon_key_id   = sha256:ab12...
  genesis_hash    = 9f3a...c1e2
  old_session_id  = 0x0A0B0C0D
  new_session_id  = 0x1A2B3C4D
  reward_restored = YES
  lane            = STATELESS
  recovery_key    = FALCON
```

On RISC-V, verify that `old_session_id` and `new_session_id` are decoded with correct little-endian byte order (i.e., the same 4 bytes produce the same `uint32_t` value as on x86).

---

## RISC-V-Specific Verification Steps

### Step 1 — Compile and Check for Warnings

```bash
make -j$(nproc) 2>&1 | grep -E 'warning:|error:'
```

Any `-Wstrict-aliasing` or `-Wuninitialized` warning on RISC-V but not on x86 indicates potentially undefined behaviour that may cause different results across architectures.

### Step 2 — Run Session Serialisation Self-Test

```bash
./nexus -testnet -selftest=session_serialisation
```

This runs a compile-time-seeded serialisation round-trip and prints `PASS` or `FAIL`.  It should pass on any architecture.

### Step 3 — ChaCha20 Known-Answer Test (KAT)

```bash
./nexus -testnet -selftest=chacha20_kat
```

Runs a ChaCha20-Poly1305 encrypt/decrypt cycle against known test vectors from RFC 8439.  Any architecture that produces a tag mismatch on the KAT has a broken crypto backend.

### Step 4 — Falcon-1024 Sign/Verify Round-Trip

```bash
./nexus -testnet -selftest=falcon_roundtrip
```

Generates a fresh Falcon-1024 key pair, signs a known message, and verifies the signature.  Should pass on any architecture.

### Step 5 — Block Template Offset Check

```bash
./nexus -testnet -selftest=template_offsets
```

Generates a dummy Tritium template, reads back `nChannel`, `nHeight`, `nBits`, `nNonce` from their expected byte offsets, and confirms they match the values that were written.

---

## Cross-Compile CI Integration

The recommended CI configuration for RISC-V cross-compile testing:

```yaml
# .github/workflows/riscv-cross-build.yml (example)
jobs:
  riscv-cross:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install cross-toolchain
        run: |
          sudo apt-get install -y gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
              qemu-user-static
      - name: Cross-compile
        run: |
          CC=riscv64-linux-gnu-gcc CXX=riscv64-linux-gnu-g++ make -j$(nproc)
      - name: Run self-tests under QEMU
        run: |
          qemu-riscv64-static ./nexus -testnet -selftest=session_serialisation
          qemu-riscv64-static ./nexus -testnet -selftest=chacha20_kat
          qemu-riscv64-static ./nexus -testnet -selftest=falcon_roundtrip
          qemu-riscv64-static ./nexus -testnet -selftest=template_offsets
```

---

## Common RISC-V Issues and Fixes

| Symptom | Likely Cause | Fix |
|---|---|---|
| `arch_selfcheck = FAIL` in session log | Struct layout differs; compiler padding inserted | Add `static_assert` on struct sizes; use explicit offsets |
| ChaCha20 tag mismatch on RISC-V only | Wrong endianness in nonce construction | Verify nonce bytes are written with `WriteLE32`, not a plain cast |
| `nHeight` reads as 0 on RISC-V | Misaligned read from template buffer | Use `memcpy` into a local `uint32_t`, never a direct cast |
| Falcon verification fails on RISC-V | liboqs not compiled for RV64 | Rebuild liboqs with `cmake -DCMAKE_SYSTEM_PROCESSOR=riscv64` |
| Node crashes on startup (RISC-V) | `-march` flag includes extension not supported by CPU | Build with `-march=rv64gc` (baseline) rather than `-march=rv64gcv` |

---

## Diagnostics Helper Functions

The `LLP::Diagnostics` namespace (see `src/LLP/include/stateless_miner.h`) provides the following helpers used in session log output:

| Helper | Purpose |
|---|---|
| `FullHexOrUnset(uint256_t)` | Print genesis hash as hex, or `<unset>` if null |
| `KeyFingerprint(bytes)` | Print `sha256:<first-8-hex>` fingerprint of a key |
| `YesNo(bool)` | Print `YES` or `NO` |
| `PassFail(bool)` | Print `PASS` or `FAIL` |

These helpers produce the same output on x86 and RISC-V because they operate on byte arrays with no integer-endianness dependence.

---

## Related Pages

- [Endianness & Serialisation](endianness-serialization.md)
- [Atomic Operations & Locking](atomic-locking.md)
- [RISC-V Overview](index.md)
- [Test Strategy](../test-strategy.md)
- [Session Container Architecture](../session-container-architecture.md)
