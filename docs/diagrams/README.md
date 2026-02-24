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
