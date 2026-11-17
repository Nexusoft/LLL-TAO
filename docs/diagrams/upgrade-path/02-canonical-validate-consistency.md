# Diagram 2 — Canonical `ValidateConsistency` Method

**Roadmap Item:** R-02  
**Priority:** 1 (Correctness Critical)

---

## Context (Before)

`ValidateConsistency()` exists (added in PR #363) but is only called at two of the four required call sites.  Two critical gaps remain: the `MINER_SET_REWARD` handler and the `SUBMIT_BLOCK` handler do not call it.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Partial ValidateConsistency Coverage                     ║
║                                                                      ║
║  ┌─────────────────────────────────────────────────────────────┐    ║
║  │ ValidateConsistency() exists in MinerSessionContainer       │    ║
║  └───────────────────────────┬─────────────────────────────────┘    ║
║                              │                                      ║
║          ┌───────────────────┼───────────────────────┐              ║
║          ▼                   ▼                        ▼              ║
║  ┌───────────────┐   ┌────────────────┐   ┌──────────────────────┐  ║
║  │ MINER_AUTH    │   │ MINER_SET_     │   │ SUBMIT_BLOCK         │  ║
║  │ handler       │   │ REWARD handler │   │ handler              │  ║
║  │               │   │                │   │                      │  ║
║  │ ✅ Calls      │   │ ❌ NOT called  │   │ ❌ NOT called        │  ║
║  │ ValidateCon.. │   │                │   │                      │  ║
║  └───────────────┘   └────────────────┘   └──────────────────────┘  ║
║                                                                      ║
║          ▼                                                           ║
║  ┌────────────────────────────────┐                                  ║
║  │ Recovery merge path            │                                  ║
║  │ ✅ Calls ValidateConsistency() │                                  ║
║  └────────────────────────────────┘                                  ║
║                                                                      ║
║  RISK: A corrupted container can reach SUBMIT_BLOCK handler.         ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

`ValidateConsistency()` is called at all four security boundaries.  Each call site fails safe with a distinct rejection reason logged.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — ValidateConsistency at All Security Boundaries            ║
║                                                                      ║
║  ┌────────────────────────────────────────────────────────────────┐  ║
║  │  ValidateConsistency() — called at every security boundary    │  ║
║  └─────────────────────────────────────────────────────────────┬──┘  ║
║                                                                │     ║
║   ┌────────────────┐  ┌─────────────────┐  ┌────────────────┐  │     ║
║   │ MINER_AUTH     │  │ MINER_SET_REWARD│  │ SUBMIT_BLOCK   │  │     ║
║   │                │  │                 │  │                │  │     ║
║   │ ✅ Call #1     │  │ ✅ Call #2 NEW  │  │ ✅ Call #3 NEW │  │     ║
║   │ Fail → AUTH_   │  │ Fail → packet   │  │ Fail → BLOCK_  │  │     ║
║   │ FAILED         │  │ dropped         │  │ REJECTED       │  │     ║
║   └────────────────┘  └─────────────────┘  └────────────────┘  │     ║
║                                                                │     ║
║   ┌──────────────────────────────────────────────────────────┐  │     ║
║   │ Recovery merge path                                      │  │     ║
║   │ ✅ Call #4 (existing)                                    │◀─┘     ║
║   │ Fail → snapshot discarded, fresh container used          │        ║
║   └──────────────────────────────────────────────────────────┘        ║
║                                                                       ║
║  GAIN: No path from connect to accepted block without full validation.║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `ValidateConsistency()` called at end of `MINER_AUTH` handler ✅
- [ ] `ValidateConsistency()` called at start of `MINER_SET_REWARD` handler
- [ ] `ValidateConsistency()` called at start of `SUBMIT_BLOCK` handler
- [ ] `ValidateConsistency()` called before recovery merge ✅
- [ ] Integration test: deliberate field corruption triggers rejection at each call site
