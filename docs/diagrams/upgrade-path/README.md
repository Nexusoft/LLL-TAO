# Upgrade-Path Diagrams — Index

**Section:** Diagrams  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-10

This directory contains 15 thick-box ASCII upgrade-path diagrams for the stateless mining node C++ refactor series.  Each diagram is screenshot-friendly and suitable for PNG conversion.

---

## Diagram List

| # | Title | Roadmap Item | File |
|---|---|---|---|
| 1 | Shared `SessionBinding` Value Object | R-01 | [01-shared-session-binding.md](01-shared-session-binding.md) |
| 2 | Canonical `ValidateConsistency` Method | R-02 | [02-canonical-validate-consistency.md](02-canonical-validate-consistency.md) |
| 3 | Stronger State Machines / Enums | R-03 | [03-stronger-state-machines.md](03-stronger-state-machines.md) |
| 4 | Live Container vs Recovery Snapshot | R-04 | [04-live-container-vs-recovery-snapshot.md](04-live-container-vs-recovery-snapshot.md) |
| 5 | Identity-First Recovery Keys | R-05 | [05-identity-first-recovery-keys.md](05-identity-first-recovery-keys.md) |
| 6 | Scoped Update Guard / Staged Merge Model | R-06 | [06-scoped-update-guard.md](06-scoped-update-guard.md) |
| 7 | Reward Address Semantics | R-07 | [07-reward-address-semantics.md](07-reward-address-semantics.md) |
| 8 | Multi-Miner Collision Tests | R-08 | [08-multi-miner-collision-tests.md](08-multi-miner-collision-tests.md) |
| 9 | Canonical `CryptoContext` Accessor | R-09 | [09-canonical-crypto-context.md](09-canonical-crypto-context.md) |
| 10 | First-Mined-Block Acceptance Harness | R-10 | [10-first-mined-block-acceptance.md](10-first-mined-block-acceptance.md) |
| 11 | `SessionConflictResolver` | R-11 | [11-session-conflict-resolver.md](11-session-conflict-resolver.md) |
| 12 | Fast vs Full Validation Modes | R-12 | [12-fast-vs-full-validation.md](12-fast-vs-full-validation.md) |
| 13 | Per-Session Event Journal / Ring Buffer | R-13 | [13-per-session-event-journal.md](13-per-session-event-journal.md) |
| 14 | Packet-Ingress Preflight Gate | R-14 | [14-packet-ingress-preflight.md](14-packet-ingress-preflight.md) |
| 15 | Strong Semantic ID Wrapper Types | R-15 | [15-strong-semantic-id-types.md](15-strong-semantic-id-types.md) |

---

## How to Use These Diagrams

Each diagram file contains:

1. A **context block** explaining the current (before) state.
2. A **target block** explaining the desired (after) state.
3. A thick-box ASCII diagram showing the architecture change.
4. An **acceptance criteria** checklist.

The diagrams are designed for:
- Copy-paste into PR descriptions
- Screenshot/PNG conversion (use a monospace terminal font at 12–14px)
- Direct inclusion in code review comments

---

## Related Pages

- [Node Architecture Index](../../current/node/index.md)
- [Roadmap & Upgrade Path](../../current/node/roadmap-upgrade-path.md)
- [Test Strategy](../../current/node/test-strategy.md)
