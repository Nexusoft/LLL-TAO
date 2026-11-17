# Diagram 6 — Scoped Update Guard / Staged Merge Model

**Roadmap Item:** R-06  
**Priority:** 2 (Safety & Robustness)

---

## Context (Before)

Multi-field container updates (e.g. during recovery merge or reward binding) are performed as sequential assignments.  If an exception or early return occurs mid-sequence, the container is left in a partially-updated, inconsistent state.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Sequential Assignment (No Rollback)                       ║
║                                                                      ║
║  MergeContext(const MinerSessionContainer& incoming) {               ║
║    // Step 1: update genesis                                         ║
║    this->hashGenesis = incoming.hashGenesis;   ← written             ║
║                                                                      ║
║    // Step 2: update key (may throw if vChacha20Key is huge)        ║
║    this->vChacha20Key = incoming.vChacha20Key; ← may throw           ║
║                                                                      ║
║    // Step 3: update reward                                          ║
║    this->vRewardHash  = incoming.vRewardHash;  ← never reached       ║
║    this->fRewardBound = incoming.fRewardBound; ← never reached       ║
║  }                                                                   ║
║                                                                      ║
║  If Step 2 throws:                                                   ║
║    hashGenesis updated ✅                                            ║
║    vChacha20Key NOT updated ❌                                        ║
║    vRewardHash  NOT updated ❌                                        ║
║                                                                      ║
║  Container is now inconsistent. ValidateConsistency() may pass       ║
║  because genesis is non-null — but the key is stale.                ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A `ScopedContainerUpdate` RAII guard stages all field changes and atomically commits them only when explicitly told to, rolling back on scope exit otherwise.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — ScopedContainerUpdate (Staged Merge)                      ║
║                                                                      ║
║  MergeContext(const MinerSessionContainer& incoming) {               ║
║                                                                      ║
║    ScopedContainerUpdate guard(*this);  // acquires write lock       ║
║                                        // copies 'this' → staging   ║
║                                                                      ║
║    guard.staging().hashGenesis  = incoming.hashGenesis;              ║
║    guard.staging().vChacha20Key = incoming.vChacha20Key; // may throw║
║    guard.staging().vRewardHash  = incoming.vRewardHash;              ║
║    guard.staging().fRewardBound = incoming.fRewardBound;             ║
║                                                                      ║
║    std::string reason;                                               ║
║    if (!guard.staging().ValidateConsistency(reason))                 ║
║      return false;  // scope exit → rollback, live untouched         ║
║                                                                      ║
║    guard.Commit();  // atomic swap: staging → live                   ║
║  }  // scope exit → releases write lock                              ║
║                                                                      ║
║  ┌────────────────────────────────────────────────────────────┐      ║
║  │  ScopedContainerUpdate lifecycle                           │      ║
║  │                                                            │      ║
║  │  Construction  → lock acquired, staging = copy of live     │      ║
║  │  .staging()    → mutable reference to staging copy         │      ║
║  │  .Commit()     → ValidateConsistency + swap + mark done    │      ║
║  │  Destructor    → if !committed: discard staging (rollback)  │      ║
║  │                  release lock                              │      ║
║  └────────────────────────────────────────────────────────────┘      ║
║                                                                      ║
║  GAIN: Container is always valid or untouched — never partially      ║
║        updated. Exception safety without try/catch at call sites.    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `class ScopedContainerUpdate` implemented in `src/LLP/include/session_recovery.h`
- [ ] `MergeContext` uses `ScopedContainerUpdate`
- [ ] `MINER_SET_REWARD` handler uses `ScopedContainerUpdate` for reward field writes
- [ ] Unit test: simulate throw mid-merge → live container unchanged
- [ ] Unit test: successful merge → all fields updated atomically
