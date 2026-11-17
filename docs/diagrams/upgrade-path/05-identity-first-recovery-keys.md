# [HISTORICAL PROPOSAL] Diagram 5 — Identity-First Recovery Keys

> **⚠️ HISTORICAL:** This proposal references `SessionRecoveryManager` which was
> **removed** in PR #532.  Retained for design context only.

**Roadmap Item:** R-05  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

The recovery manager uses remote address as a fallback key when no Falcon-key-ID hit is found.  For stateless-lane miners this can cause incorrect recovery in multi-miner / NAT scenarios.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Address as Fallback Recovery Key                          ║
║                                                                      ║
║  SessionRecoveryManager lookup:                                      ║
║                                                                      ║
║    Step 1: lookup by strFalconKeyID  → HIT → use snapshot           ║
║                                      → MISS ↓                       ║
║    Step 2: lookup by strRemoteAddress → HIT → use snapshot          ║
║                                         → MISS → fresh container    ║
║                                                                      ║
║  Problem scenario (NAT / shared IP):                                 ║
║                                                                      ║
║  Miner A (genesis=G_A) disconnects from IP 203.0.113.1              ║
║  Miner B (genesis=G_B) connects    from IP 203.0.113.1              ║
║                                                                      ║
║  Step 2 hits Miner A's snapshot for Miner B's connection.           ║
║  Cross-validation catches the genesis mismatch → snapshot discarded.║
║  BUT: the address lookup itself consumed the stored snapshot,        ║
║  so Miner A cannot recover if Miner A reconnects immediately after. ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

For stateless-lane connections, Falcon key ID is the only recovery key.  Address-based fallback is restricted to legacy-lane connections and documented as a known weakness.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Identity-First Recovery (Stateless Lane)                  ║
║                                                                      ║
║  ┌────────────────────────────────────────────────────────────┐      ║
║  │  SessionRecoveryManager lookup (stateless lane)            │      ║
║  │                                                            │      ║
║  │  Step 1: lookup by strFalconKeyID → HIT → use snapshot     │      ║
║  │                                    → MISS → fresh container│      ║
║  │                                                            │      ║
║  │  Address lookup: DISABLED for stateless lane               │      ║
║  └────────────────────────────────────────────────────────────┘      ║
║                                                                      ║
║  ┌────────────────────────────────────────────────────────────┐      ║
║  │  SessionRecoveryManager lookup (legacy lane)               │      ║
║  │                                                            │      ║
║  │  Step 1: lookup by strFalconKeyID → HIT → use snapshot     │      ║
║  │                                    → MISS ↓                │      ║
║  │  Step 2: lookup by strRemoteAddress → HIT → cross-validate │      ║
║  │    (Legacy protocol requires address-based recovery for    │      ║
║  │     miners that do not send Falcon identity on reconnect)  │      ║
║  └────────────────────────────────────────────────────────────┘      ║
║                                                                      ║
║  Configuration flag:                                                  ║
║    recovery.require_falcon_identity = true                           ║
║    → disables address fallback on both lanes                        ║
║                                                                      ║
║  GAIN: Stateless miners always recover by cryptographic identity.    ║
║        NAT/shared-IP collisions cannot cause cross-miner recovery.   ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `SessionRecoveryManager` lookup path checks `eLane` before attempting address fallback
- [ ] Stateless-lane lookup: no address fallback attempted
- [ ] Legacy-lane lookup: address fallback retained with cross-validation
- [ ] `recovery.require_falcon_identity` config flag disables address fallback on both lanes
- [ ] Unit test: two miners with same IP but different Falcon keys do not share recovery state
