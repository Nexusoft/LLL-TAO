# Test Strategy — First-Block Acceptance & Refactor Safety

**Section:** Node Architecture  
**Version:** LLL-TAO 5.1.0+ (post-PR #361 / #362 / #363)  
**Last Updated:** 2026-03-10

---

## Overview

This document describes the test strategy needed to complete the C++ refactor series safely and to verify that the first mined block is accepted end-to-end.

Tests are grouped into five areas:

1. [First-Mined-Block Acceptance Harness](#1-first-mined-block-acceptance-harness)
2. [Two-Miner / Multi-Miner Collision Tests](#2-two-miner--multi-miner-collision-tests)
3. [Reconnect and Recovery Conflict Scenarios](#3-reconnect-and-recovery-conflict-scenarios)
4. [Packet-Ingress Preflight Validation Checks](#4-packet-ingress-preflight-validation-checks)
5. [Cross-Architecture / Serialisation Stability](#5-cross-architecture--serialisation-stability)

---

## 1. First-Mined-Block Acceptance Harness

**Goal:** Prove the full path from miner connect to node-accepted first block with no cheats or stubs.

**Diagram:** [Diagram 10](../../diagrams/upgrade-path/10-first-mined-block-acceptance.md)

### Harness Flow

```
  ┌─────────────────────────────────────────────────────────────┐
  │  Test Harness ("first_block_acceptance_test")               │
  │                                                             │
  │  Setup                                                      │
  │  ─────                                                      │
  │  • Start node in testnet mode (in-process or loopback)      │
  │  • Generate Falcon-1024 key pair for test miner             │
  │  • Prepare reward address (known, decodable)                │
  │                                                             │
  │  Step 1 — MINER_AUTH                                        │
  │    Send AUTH with Falcon sig over genesis hash + session_id  │
  │    Assert: AUTH_RESPONSE received, session ID in response   │
  │    Assert: container created in NodeSessionRegistry         │
  │    Assert: ValidateConsistency() == true                    │
  │                                                             │
  │  Step 2 — MINER_SET_REWARD                                  │
  │    Encrypt reward hash under ChaCha20 from container        │
  │    Assert: REWARD_SET_OK received                           │
  │    Assert: container.fRewardBound == true                   │
  │    Assert: container.vRewardHash == expected 32 bytes       │
  │                                                             │
  │  Step 3 — GET_BLOCK / NEW_BLOCK                             │
  │    Assert: template received with correct channel/height    │
  │    Assert: template nonce field is writable                 │
  │                                                             │
  │  Step 4 — Solve Block (test mode: trivial difficulty)       │
  │    Solve PoW in test mode                                   │
  │                                                             │
  │  Step 5 — SUBMIT_BLOCK                                      │
  │    Send submission with solved nonce                        │
  │    Assert: BLOCK_ACCEPTED received (not BLOCK_REJECTED)     │
  │    Assert: reward address applied to coinbase               │
  │    Assert: no ChaCha20 tag mismatch in node logs            │
  │                                                             │
  │  Teardown                                                   │
  │    Assert: no leaked containers in NodeSessionRegistry      │
  │    Assert: no leaked recovery snapshots                     │
  └─────────────────────────────────────────────────────────────┘
```

### Preconditions Required Before This Test Can Pass

| Condition | Source PR / Roadmap Item |
|---|---|
| Authoritative container created at `MINER_AUTH` | PR #361 |
| Reward hash stored in container at `MINER_SET_REWARD` | PR #361 |
| Decrypt key sourced from container (not stale local copy) | R-09 |
| `ValidateConsistency()` called before `SUBMIT_BLOCK` | R-02 |
| Coinbase applies `container.vRewardHash` | PR #363 |

---

## 2. Two-Miner / Multi-Miner Collision Tests

**Goal:** Prove that two or more miners cannot bleed state into each other's containers.

**Diagram:** [Diagram 8](../../diagrams/upgrade-path/08-multi-miner-collision-tests.md)

### Test Cases

#### TC-2.1 — Two Miners, Different Genesis Hashes
- Connect Miner A and Miner B simultaneously.
- Each has a different Falcon key and different genesis hash.
- Each sends `MINER_SET_REWARD` with a different reward address.
- **Assert:** `NodeSessionRegistry` contains two distinct containers.
- **Assert:** Reward hash in A's container ≠ reward hash in B's container.
- **Assert:** Submitting from A does not reference B's reward.

#### TC-2.2 — Same Remote Address, Two Miners (NAT Scenario)
- Connect Miner A and Miner B from the same IP:port (simulated).
- Both have different Falcon keys.
- **Assert:** Falcon key ID is the disambiguating index; address collision is harmless.
- **Assert:** Each miner receives its own `MINER_AUTH_RESPONSE`.

#### TC-2.3 — Reconnect Collision (Same Falcon Key, Two Connections)
- Connect Miner A.  Disconnect without cleanup.  Connect again with same Falcon key.
- **Assert:** New connection recovers the old container's reward binding.
- **Assert:** Old container is retired (evicted from `NodeSessionRegistry`).
- **Assert:** `nSessionID` in new container differs from old.

#### TC-2.4 — 1000-Miner Scale Test
- Launch 1000 virtual miners in sequence; each authenticates and sets reward.
- **Assert:** No lock contention (no deadlock; completes within timeout).
- **Assert:** No memory leak (container count returns to zero after all disconnect).
- **Assert:** `NodeSessionRegistry` is empty after teardown.

---

## 3. Reconnect and Recovery Conflict Scenarios

**Goal:** Prove the recovery merge model is safe and deterministic.

**Diagram:** [Diagram 11](../../diagrams/upgrade-path/11-session-conflict-resolver.md)

### Test Cases

#### TC-3.1 — Clean Reconnect, Reward Restored
- Miner authenticates and binds reward.  TCP connection drops.  Miner reconnects.
- **Assert:** Container created with `fRewardBound = true` and matching `vRewardHash`.
- **Assert:** Reconnect accounting log line emitted.
- **Assert:** Miner does not need to re-send `MINER_SET_REWARD`.

#### TC-3.2 — Reconnect with Stale Snapshot (Expired)
- Set `recovery.ttl = 1s` in test config.  Miner disconnects.  Wait 2s.  Reconnect.
- **Assert:** Snapshot discarded (TTL exceeded).
- **Assert:** Fresh container created; miner must re-bind reward.

#### TC-3.3 — Snapshot Falcon Mismatch (Tampered)
- Store a snapshot with Falcon key ID `A`.  Reconnect with Falcon key ID `B`.
- **Assert:** Snapshot discarded (Falcon mismatch).
- **Assert:** No state from snapshot transferred to new container.

#### TC-3.4 — Conflict: Two Snapshots, One Falcon Key ID
- Force two snapshots with the same Falcon key ID into the recovery manager.
- **Assert:** `SessionConflictResolver` detects conflict and logs it.
- **Assert:** Deterministic resolution applied (newer `nLastActivity` wins).

---

## 4. Packet-Ingress Preflight Validation Checks

**Goal:** Prove the preflight gate blocks invalid packets before handler code runs.

**Diagram:** [Diagram 14](../../diagrams/upgrade-path/14-packet-ingress-preflight.md)

### Test Cases

#### TC-4.1 — `SUBMIT_BLOCK` Before `MINER_AUTH`
- Send `SUBMIT_BLOCK` on a fresh connection with no auth.
- **Assert:** Preflight rejects; `BLOCK_REJECTED` or `AUTH_REQUIRED` returned.
- **Assert:** Handler body never executes (verify with mock/counter).

#### TC-4.2 — `MINER_SET_REWARD` Before Authenticated
- Send `MINER_SET_REWARD` with a plausible but non-authenticated session ID.
- **Assert:** Preflight rejects; packet dropped.

#### TC-4.3 — `SUBMIT_BLOCK` Before Reward Bound
- Authenticate successfully but skip `MINER_SET_REWARD`.
- Send `SUBMIT_BLOCK`.
- **Assert:** Preflight rejects (`REWARD_NOT_BOUND` reason).

#### TC-4.4 — Corrupted `ValidateConsistency` Mid-Session
- Inject a corrupt container (zero `nSessionID`) via test hook.
- Send any packet.
- **Assert:** Preflight catches `ValidateConsistency()` failure and drops packet.
- **Assert:** Connection is not terminated (miner can re-authenticate).

---

## 5. Cross-Architecture / Serialisation Stability

**Goal:** Prove that block templates, session packets, and recovery snapshots round-trip identically on all supported architectures.

**Diagram:** [RISC-V Endianness Notes](../node/riscv/endianness-serialization.md)

### Test Cases

#### TC-5.1 — Session Packet Byte Order (x86 vs RISC-V)
- Serialise a `MinerSessionContainer` snapshot on x86.
- Deserialise it in a RISC-V cross-compiled test binary.
- **Assert:** All numeric fields decode to the same value.
- **Assert:** `ValidateConsistency()` passes after deserialisation.

#### TC-5.2 — Block Template Field Layout
- Generate a 216-byte Tritium template on the node.
- Parse `nChannel@196`, `nHeight@200`, `nBits@204`, `nNonce@208` offsets in a RISC-V test.
- **Assert:** Parsed values match the values used to build the template.

#### TC-5.3 — Genesis Hash Endianness
- Produce a `hashGenesis` from a known Falcon public key on x86.
- Reproduce on RISC-V cross-compile.
- **Assert:** Byte arrays are identical.

#### TC-5.4 — ChaCha20 Encrypt / Decrypt Round-Trip
- Encrypt a known 32-byte reward hash on x86 with a known 32-byte key.
- Decrypt in a RISC-V cross-compiled binary.
- **Assert:** Decrypted bytes == original plaintext.
- **Assert:** Poly1305 tag matches (no architecture-induced tag mismatch).

---

## Test Infrastructure Notes

### Existing Test Files (Extend These)

| File | Purpose |
|---|---|
| `tests/unit/LLP/session_recovery.cpp` | Recovery manager unit tests |
| `tests/unit/LLP/node_session_registry_tests.cpp` | Registry index unit tests |
| `tests/unit/LLP/node_integration_hardening.cpp` | Fork/rejection/reauth integration tests |
| `tests/unit/LLP/test_stateless_miner_crypto.cpp` | ChaCha20 / Falcon crypto unit tests |

### New Test Files Needed

| File | Tests |
|---|---|
| `tests/unit/LLP/first_block_acceptance_test.cpp` | Section 1 harness |
| `tests/unit/LLP/multi_miner_collision_tests.cpp` | Section 2 (TC-2.1 – TC-2.4) |
| `tests/unit/LLP/recovery_conflict_tests.cpp` | Section 3 (TC-3.1 – TC-3.4) |
| `tests/unit/LLP/preflight_gate_tests.cpp` | Section 4 (TC-4.1 – TC-4.4) |
| `tests/unit/LLP/cross_arch_serialisation_tests.cpp` | Section 5 (TC-5.1 – TC-5.4) |

---

## Related Pages

- [Session Container Architecture](session-container-architecture.md)
- [Recovery Merge Model](recovery-merge-model.md)
- [Roadmap & Upgrade Path](roadmap-upgrade-path.md)
- [RISC-V Diagnostics](riscv/diagnostics.md)
- [Diagrams Index](../../diagrams/upgrade-path/README.md)
