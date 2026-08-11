# Diagram 8 — Multi-Miner Collision Tests

**Roadmap Item:** R-08  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

The refactored session container model has been tested with single-miner flows.  No automated tests verify that two or more concurrent miners cannot bleed state into each other's containers.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Single-Miner Test Coverage                                ║
║                                                                      ║
║  Existing test files:                                                ║
║    tests/unit/LLP/session_recovery.cpp         (single miner)       ║
║    tests/unit/LLP/node_session_registry_tests  (single miner)       ║
║    tests/unit/LLP/node_integration_hardening   (single miner)       ║
║                                                                      ║
║  Not tested:                                                         ║
║    Two miners simultaneously connected                               ║
║    Two miners with same IP but different genesis hashes              ║
║    Two miners where Miner B's reward is Miner A's session ID         ║
║    1000-miner scale (lock contention, memory leak)                   ║
║                                                                      ║
║  RISK: A container ownership or index bug only manifests with        ║
║        multiple concurrent miners — invisible in single-miner tests. ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A dedicated multi-miner collision test suite verifies all meaningful concurrent-miner scenarios.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Multi-Miner Collision Test Suite                          ║
║                                                                      ║
║  File: tests/unit/LLP/multi_miner_collision_tests.cpp               ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  TC-2.1  Two Miners, Different Genesis Hashes               │   ║
║  │                                                              │   ║
║  │  Miner A ──AUTH(G_A)──▶ Node                                 │   ║
║  │  Miner B ──AUTH(G_B)──▶ Node                                 │   ║
║  │                                                              │   ║
║  │  Assert: registry.size() == 2                                │   ║
║  │  Assert: containerA.vRewardHash ≠ containerB.vRewardHash     │   ║
║  │  Assert: SUBMIT from A uses containerA reward only           │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  TC-2.2  Same IP, Different Falcon Keys (NAT)               │   ║
║  │                                                              │   ║
║  │  Miner A (IP=X, falcon=F_A) ──▶ Node                        │   ║
║  │  Miner B (IP=X, falcon=F_B) ──▶ Node                        │   ║
║  │                                                              │   ║
║  │  Assert: both sessions active simultaneously                 │   ║
║  │  Assert: recovery lookup for F_A returns containerA only     │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  TC-2.3  Same Falcon Key, Two Connections (Reconnect)        │   ║
║  │                                                              │   ║
║  │  Miner connects (session_id=S1) → sets reward                │   ║
║  │  Miner disconnects                                           │   ║
║  │  Miner reconnects (session_id=S2) → recovers reward          │   ║
║  │                                                              │   ║
║  │  Assert: S2 ≠ S1                                             │   ║
║  │  Assert: containerNew.fRewardBound == true                   │   ║
║  │  Assert: containerNew.vRewardHash == originalRewardHash      │   ║
║  │  Assert: old session ID removed from registry                │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  TC-2.4  1000-Miner Scale Test                               │   ║
║  │                                                              │   ║
║  │  Launch 1000 miners (unique Falcon keys) sequentially        │   ║
║  │  Each authenticates and sets reward                          │   ║
║  │  All disconnect                                              │   ║
║  │                                                              │   ║
║  │  Assert: peak registry size == 1000                          │   ║
║  │  Assert: after disconnect registry.size() == 0               │   ║
║  │  Assert: no deadlock (completes in < 30s)                    │   ║
║  │  Assert: no memory leak (valgrind / AddressSanitizer)        │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `tests/unit/LLP/multi_miner_collision_tests.cpp` created
- [ ] TC-2.1 through TC-2.4 all pass in CI
- [ ] 1000-miner test completes under address sanitizer with no errors
- [ ] No race condition detected by ThreadSanitizer on TC-2.1 / TC-2.2
