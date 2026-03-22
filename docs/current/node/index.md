# Node Architecture — Section Index

**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-10

This section documents the stateless mining node's internal architecture, session-container model, recovery mechanics, refactor roadmap, and test strategy.  It is the authoritative reference for the C++ refactor series that began with **PR #361** (session containerisation) and continues through the items listed in [Roadmap & Upgrade Path](roadmap-upgrade-path.md).

---

## Pages in This Section

| Document | Purpose |
|---|---|
| **[Session Container Architecture](session-container-architecture.md)** | Per-miner authoritative container model, indexing, and ownership rules |
| **[Recovery Merge Model](recovery-merge-model.md)** | How the node recovers, validates, and merges session state after reconnects |
| **[Roadmap & Upgrade Path](roadmap-upgrade-path.md)** | Remaining refactor items and their priority ordering |
| **[Test Strategy](test-strategy.md)** | First-block acceptance harness, multi-miner collision, and preflight tests |
| **[RISC-V Overview](riscv/index.md)** | RISC-V build targets, portability notes, and node deployment guide |

---

## Quick-Reference Architecture Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                  Stateless Mining Node (LLL-TAO)                │
│                                                                 │
│  ┌──────────────────┐     ┌──────────────────────────────────┐  │
│  │  LLP Listener    │────▶│  StatelessMinerConnection        │  │
│  │  port 9323       │     │  (per-TCP-connection handler)    │  │
│  └──────────────────┘     └──────────────┬───────────────────┘  │
│                                          │                      │
│                                          ▼                      │
│                          ┌───────────────────────────────────┐  │
│                          │  MinerSessionContainer            │  │
│                          │  (authoritative per-session blob) │  │
│                          │                                   │  │
│                          │  • SessionID (u32)                │  │
│                          │  • GenesisHash (uint256)          │  │
│                          │  • Falcon key fingerprint         │  │
│                          │  • ChaCha20 session key bytes     │  │
│                          │  • Reward hash (32 bytes)         │  │
│                          │  • Lane (stateless / legacy)      │  │
│                          │  • fAuthenticated flag            │  │
│                          └──────────────┬────────────────────┘  │
│                                         │                       │
│            ┌────────────────────────────┤                       │
│            │                            │                       │
│            ▼                            ▼                       │
│  ┌──────────────────┐    ┌─────────────────────────────────┐    │
│  │ NodeSessionRegistry│  │  SessionRecoveryManager         │    │
│  │ (live index)     │    │  (recovery snapshot store)      │    │
│  │                  │    │                                 │    │
│  │ session_id→ptr   │    │  address→container snapshot     │    │
│  │ falcon_id→ptr    │    │  falcon_id→container snapshot   │    │
│  └──────────────────┘    └─────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Key Design Invariants

1. **One authoritative container** — `MinerSessionContainer` is the single source of truth for all per-miner session state.
2. **Indexes reference, not copy** — `NodeSessionRegistry` indexes hold pointers/handles to the same container object; they never own independent mutable copies.
3. **Recovery validates before merging** — Recovered state is passed through `ValidateConsistency()` before it replaces the live container.
4. **Identity-first recovery** — Falcon key fingerprint is the primary recovery key; remote address is secondary.
5. **Reward binding travels with the session** — `rewardHash` is persisted inside the recoverable snapshot; it is never stored separately.

---

## Related Sections

- [Diagrams — Upgrade-Path Series](../../diagrams/upgrade-path/README.md)
- [Mining Server Architecture](../mining/mining-server.md)
- [Falcon Verification](../authentication/falcon-verification.md)
- [RISC-V Design](../../riscv-design.md)
