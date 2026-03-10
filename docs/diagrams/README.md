# Diagram Templates & Architecture Diagrams

Diagram templates for PR descriptions and comprehensive Mermaid architecture diagrams.

## 🎮 Interactive Training App

**[index.html](index.html)** — Self-contained single-file interactive training application for Nexus mining staleness detection logic.

- 12 step-by-step scenarios covering every staleness guard in both the **Stateless lane (port 9323)** and **Legacy lane (port 8323)**
- Inline SVG flowcharts — no external images or CDN dependencies
- Dark theme, keyboard navigation, real-time score tracking
- Open directly in any browser — no server required

### Keyboard shortcuts

| Key | Action |
|-----|--------|
| `←` / `→` | Previous / next scenario |
| `A` – `D` | Select answer option |
| `R` | Restart quiz |

### Scenario list

| # | Title | Lane |
|---|-------|------|
| 1 | Fresh Template — All Checks Pass | Stateless |
| 2 | Age Timeout — 600 s Safety Net | Stateless |
| 3 | Invalid Timestamp — Zero nCreationTime | Stateless |
| 4 | Zero Channel Height — Uninitialized Metadata | Stateless |
| 5 | Channel Height Mismatch — Another Miner Found a Block | Stateless |
| 6 | GetLastState Failure — Safe Stale | Stateless |
| 7 | Legacy Lane — Height Change Detection | Legacy |
| 8 | Legacy Lane — Same-Height Reorg Detection | Legacy |
| 9 | Universal Tip Push — Any Channel Notifies All Miners | Both |
| 10 | Push Notification Payload Format | Both |
| 11 | Submission Guard — hashPrevBlock vs hashBestChain | Both |
| 12 | Template Height Semantics — Channel vs Unified Height | Stateless |

---

## ASCII Templates

1. **[architecture-boxes.md](architecture-boxes.md)** - Component relationships (box diagrams)
2. **[flow-chart.md](flow-chart.md)** - Decision flows and conditional logic
3. **[state-machine.md](state-machine.md)** - State transitions and lifecycle
4. **[data-pipeline.md](data-pipeline.md)** - Data flow and transformations
5. **[push-notification-flow.md](push-notification-flow.md)** - Push notification sequence (Legacy vs Stateless Tritium Protocol)

## Architecture Diagrams

6. **[architecture/mining-flow-complete.md](architecture/mining-flow-complete.md)** - End-to-end mining lifecycle
7. **[architecture/consensus-validation-flow.md](architecture/consensus-validation-flow.md)** - Block validation pipeline
8. **[architecture/ledger-state-machine.md](architecture/ledger-state-machine.md)** - TAO register state transitions
9. **[architecture/trust-network-topology.md](architecture/trust-network-topology.md)** - Peer reputation system

## Protocol Diagrams

10. **[protocols/llp-packet-anatomy.md](protocols/llp-packet-anatomy.md)** - Wire format visualization
11. **[protocols/falcon-auth-sequence.md](protocols/falcon-auth-sequence.md)** - Full Falcon handshake
12. **[protocols/chacha20-session-lifecycle.md](protocols/chacha20-session-lifecycle.md)** - Encryption lifecycle

## AI-Human Collaboration

13. **[ai-collaboration/ai-human-simlink.md](ai-collaboration/ai-human-simlink.md)** - AI-Human symbiotic strengths
14. **[ai-collaboration/learning-pathways.md](ai-collaboration/learning-pathways.md)** - How to use AI to master this codebase
15. **[ai-collaboration/cheat-sheets/common-tasks.md](ai-collaboration/cheat-sheets/common-tasks.md)** - AI prompts for frequent operations

## Upgrade-Path Diagrams (Refactor Series)

15 thick-box ASCII upgrade-path diagrams for the stateless mining node C++ refactor series.  Screenshot-friendly, suitable for PNG conversion.

**[upgrade-path/README.md](upgrade-path/README.md)** — Index of all 15 diagrams

| # | Title | File |
|---|---|---|
| 1 | Shared `SessionBinding` Value Object | [01](upgrade-path/01-shared-session-binding.md) |
| 2 | Canonical `ValidateConsistency` Method | [02](upgrade-path/02-canonical-validate-consistency.md) |
| 3 | Stronger State Machines / Enums | [03](upgrade-path/03-stronger-state-machines.md) |
| 4 | Live Container vs Recovery Snapshot | [04](upgrade-path/04-live-container-vs-recovery-snapshot.md) |
| 5 | Identity-First Recovery Keys | [05](upgrade-path/05-identity-first-recovery-keys.md) |
| 6 | Scoped Update Guard / Staged Merge | [06](upgrade-path/06-scoped-update-guard.md) |
| 7 | Reward Address Semantics | [07](upgrade-path/07-reward-address-semantics.md) |
| 8 | Multi-Miner Collision Tests | [08](upgrade-path/08-multi-miner-collision-tests.md) |
| 9 | Canonical `CryptoContext` Accessor | [09](upgrade-path/09-canonical-crypto-context.md) |
| 10 | First-Mined-Block Acceptance Harness | [10](upgrade-path/10-first-mined-block-acceptance.md) |
| 11 | `SessionConflictResolver` | [11](upgrade-path/11-session-conflict-resolver.md) |
| 12 | Fast vs Full Validation Modes | [12](upgrade-path/12-fast-vs-full-validation.md) |
| 13 | Per-Session Event Journal / Ring Buffer | [13](upgrade-path/13-per-session-event-journal.md) |
| 14 | Packet-Ingress Preflight Gate | [14](upgrade-path/14-packet-ingress-preflight.md) |
| 15 | Strong Semantic ID Wrapper Types | [15](upgrade-path/15-strong-semantic-id-types.md) |

## Why Use Diagrams?

- **Character efficient:** 50-200 chars vs 500-1,000+ for code examples
- **Universal rendering:** Mermaid renders on GitHub, ASCII works everywhere
- **Clear communication:** Architecture > code snippets
- **Saves budget:** Use 70-90% fewer characters than code

## Usage

1. Copy template from relevant file
2. Customize component/state names
3. Paste into PR description or documentation
4. Total PR description must stay under 3,000 chars

---

**Tip:** Always prefer diagrams over code examples in PR descriptions!
