# Roadmap & Upgrade Path

**Section:** Node Architecture  
**Version:** LLL-TAO 5.1.0+ (post-PR #361 / #362 / #363)  
**Last Updated:** 2026-03-10

---

## Overview

This document lists the remaining C++ refactor items needed to bring the stateless mining node to a fully consistent, production-hardened state.  Items are ordered by dependency and impact.

Each item corresponds to one or more [upgrade-path diagrams](../../diagrams/upgrade-path/README.md).

---

## Recently Merged (Baseline)

| PR | Title | Impact |
|---|---|---|
| #361 | Containerise stateless miner session recovery state | Authoritative per-session container; reward binding persisted in recovery cache |
| #362 | Node session registry: multi-index lookup | `NodeSessionRegistry` with session-ID and Falcon-key-ID indexes |
| #363 | ValidateConsistency & reconnect accounting | `ValidateConsistency()` method; structured reconnect log lines |

---

## Remaining Refactor Items

### Priority 1 — Correctness Critical

#### R-01 · Shared `SessionBinding` Value Object
**Status:** Not started  
**Diagram:** [01](../../diagrams/upgrade-path/01-shared-session-binding.md)

Replace the ad-hoc tuple `(nSessionID, hashGenesis, strFalconKeyID)` with a single immutable `SessionBinding` value object.  Callers that compare session identity must use `SessionBinding::Matches()` so comparison logic is centralised and cannot drift.

**Acceptance:** No raw field-by-field session-identity comparisons remain outside `SessionBinding`.

---

#### R-02 · Canonical `ValidateConsistency` Call Sites
**Status:** Partial (method exists; not all call sites added yet)  
**Diagram:** [02](../../diagrams/upgrade-path/02-canonical-validate-consistency.md)

Ensure `ValidateConsistency()` is called at every security boundary:

- End of `MINER_AUTH` handler ✅ (PR #363)
- Start of `MINER_SET_REWARD` handler — **missing**
- Start of `SUBMIT_BLOCK` handler — **missing**
- Recovery merge path ✅ (PR #363)

**Acceptance:** All four call sites present; CI integration test verifies each path returns `AUTH_FAILED` / packet rejection on deliberate corruption.

---

#### R-09 · Canonical `CryptoContext` Accessor
**Status:** Not started  
**Diagram:** [09](../../diagrams/upgrade-path/09-canonical-crypto-context.md)

Introduce `SessionCryptoContext` as a scoped accessor that returns `(session_id, chacha20_key, key_fingerprint, genesis_hash)` atomically from the container under a read lock.  Every encrypt/decrypt call must go through this accessor — direct member field access (`m_chacha20_session_key`) must be removed.

**Acceptance:** `grep -r 'm_chacha20_session_key'` returns zero hits outside the accessor implementation.

---

### Priority 2 — Safety & Robustness

#### R-03 · Stronger State Machines / Enums
**Status:** Not started  
**Diagram:** [03](../../diagrams/upgrade-path/03-stronger-state-machines.md)

Replace scattered boolean flags (`fAuthenticated`, `fRewardBound`, `fChannelSet`) with a `MinerSessionState` enum class that enforces valid transitions:

```
CONNECTED → AUTHENTICATED → REWARD_BOUND → CHANNEL_SET → MINING
```

Illegal transitions (e.g. `SUBMIT_BLOCK` before `REWARD_BOUND`) are rejected without needing manual flag checks.

---

#### R-06 · Scoped Update Guard / Staged Merge Model
**Status:** Not started  
**Diagram:** [06](../../diagrams/upgrade-path/06-scoped-update-guard.md)

Wrap multi-field container updates in a scoped guard (`ScopedContainerUpdate`) that holds a write lock and rolls back on scope-exit if the update was not explicitly committed.  Prevents partial-write races during concurrent reconnects.

---

#### R-07 · Reward Address Semantics Clarification
**Status:** Not started  
**Diagram:** [07](../../diagrams/upgrade-path/07-reward-address-semantics.md)

Document and enforce in code the distinction between:
- `rewardAddressString` — the human-readable Nexus address sent by the miner
- `rewardHash` — the 32-byte decoded form stored in the container and applied to coinbase

Add an assertion that `rewardHash` is always derived from `rewardAddressString` by the same decode path so the two never diverge.

---

#### R-14 · Packet-Ingress Preflight Gate
**Status:** Not started  
**Diagram:** [14](../../diagrams/upgrade-path/14-packet-ingress-preflight.md)

Add a `PreflightCheck(opcode, container)` function called before every opcode handler.  It validates:
1. Container exists for the session ID in the packet header.
2. Container state machine allows this opcode (e.g. `SUBMIT_BLOCK` requires `MINING` state).
3. `ValidateConsistency()` passes.

If any check fails, the packet is dropped with a structured log line — the handler is never entered.

---

### Priority 3 — Scaling & Reliability

#### R-04 · Live Container vs Recovery Snapshot Separation
**Status:** Partial (container exists; snapshot serialisation ad hoc)  
**Diagram:** [04](../../diagrams/upgrade-path/04-live-container-vs-recovery-snapshot.md)

Define a `RecoverableSnapshot` struct (POD / JSON-serialisable) that is always derived from the live container.  The recovery manager stores and restores `RecoverableSnapshot`, never the live container directly.

---

#### R-05 · Identity-First Recovery Keys (Strengthen)
**Status:** Partial (Falcon key used as primary; address as fallback)  
**Diagram:** [05](../../diagrams/upgrade-path/05-identity-first-recovery-keys.md)

Deprecate address-based recovery fallback for stateless-lane connections.  For legacy lane keep address-based recovery (required by protocol), but document it as weaker.  Add a configurable flag `recovery.require_falcon_identity = true` that disables the address fallback entirely.

---

#### R-08 · Multi-Miner Collision Tests
**Status:** Not started  
**Diagram:** [08](../../diagrams/upgrade-path/08-multi-miner-collision-tests.md)

Add unit and integration tests for:
- Two miners with different genesis hashes and reward hashes — no state bleed.
- Same Falcon key, two TCP connections — newer supersedes older.
- Same remote address, two miners — disambiguation by Falcon key ID.
- 1000-miner scale test — no lock contention, no memory leak.

---

#### R-11 · `SessionConflictResolver`
**Status:** Not started  
**Diagram:** [11](../../diagrams/upgrade-path/11-session-conflict-resolver.md)

Dedicated component that detects and resolves conflicts when multiple recovery snapshots claim the same Falcon key ID.

---

#### R-12 · Fast vs Full Validation Modes
**Status:** Not started  
**Diagram:** [12](../../diagrams/upgrade-path/12-fast-vs-full-validation.md)

Add a configurable fast-path validation mode for `SUBMIT_BLOCK` that skips full PoW verification for blocks from a trusted, authenticated session, falling back to full validation only when the block is novel.

---

#### R-13 · Per-Session Event Journal / Ring Buffer
**Status:** Not started  
**Diagram:** [13](../../diagrams/upgrade-path/13-per-session-event-journal.md)

Attach a fixed-size ring-buffer event journal to each container.  Events include: auth, reward bind, keepalive, submit, recovery, reconnect.  Dump the last N events on any validation failure for instant root-cause visibility.

---

### Priority 4 — Future Architecture

#### R-10 · First-Mined-Block Acceptance Harness
**Status:** Not started  
**Diagram:** [10](../../diagrams/upgrade-path/10-first-mined-block-acceptance.md)

See [Test Strategy](test-strategy.md) — this harness is the final integration gate.

---

#### R-15 · Strong Semantic ID Wrapper Types
**Status:** Not started  
**Diagram:** [15](../../diagrams/upgrade-path/15-strong-semantic-id-types.md)

Replace raw `uint32_t session_id`, `std::string falcon_id`, `uint256_t genesis_hash` with named wrapper types (`SessionId`, `FalconKeyId`, `GenesisHash`) that prevent accidental field transposition and enable overloaded comparison operators.

---

## Roadmap Summary Table

| ID | Item | Priority | Diagram | Status |
|---|---|---|---|---|
| R-01 | Shared `SessionBinding` value object | 1 | [01](../../diagrams/upgrade-path/01-shared-session-binding.md) | Not started |
| R-02 | Canonical `ValidateConsistency` call sites | 1 | [02](../../diagrams/upgrade-path/02-canonical-validate-consistency.md) | Partial |
| R-03 | Stronger state machines / enums | 2 | [03](../../diagrams/upgrade-path/03-stronger-state-machines.md) | Not started |
| R-04 | Live container vs recovery snapshot | 3 | [04](../../diagrams/upgrade-path/04-live-container-vs-recovery-snapshot.md) | Partial |
| R-05 | Identity-first recovery (strengthen) | 3 | [05](../../diagrams/upgrade-path/05-identity-first-recovery-keys.md) | Partial |
| R-06 | Scoped update guard / staged merge | 2 | [06](../../diagrams/upgrade-path/06-scoped-update-guard.md) | Not started |
| R-07 | Reward address semantics | 2 | [07](../../diagrams/upgrade-path/07-reward-address-semantics.md) | Not started |
| R-08 | Multi-miner collision tests | 3 | [08](../../diagrams/upgrade-path/08-multi-miner-collision-tests.md) | Not started |
| R-09 | Canonical `CryptoContext` accessor | 1 | [09](../../diagrams/upgrade-path/09-canonical-crypto-context.md) | Not started |
| R-10 | First-mined-block acceptance harness | 4 | [10](../../diagrams/upgrade-path/10-first-mined-block-acceptance.md) | Not started |
| R-11 | `SessionConflictResolver` | 3 | [11](../../diagrams/upgrade-path/11-session-conflict-resolver.md) | Not started |
| R-12 | Fast vs full validation | 3 | [12](../../diagrams/upgrade-path/12-fast-vs-full-validation.md) | Not started |
| R-13 | Per-session event journal | 3 | [13](../../diagrams/upgrade-path/13-per-session-event-journal.md) | Not started |
| R-14 | Packet-ingress preflight gate | 2 | [14](../../diagrams/upgrade-path/14-packet-ingress-preflight.md) | Not started |
| R-15 | Strong semantic ID wrapper types | 4 | [15](../../diagrams/upgrade-path/15-strong-semantic-id-types.md) | Not started |

---

## Related Pages

- [Session Container Architecture](session-container-architecture.md)
- [Recovery Merge Model](recovery-merge-model.md)
- [Test Strategy](test-strategy.md)
- [Upgrade-Path Diagrams Index](../../diagrams/upgrade-path/README.md)
