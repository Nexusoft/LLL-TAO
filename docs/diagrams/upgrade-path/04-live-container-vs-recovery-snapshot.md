# [HISTORICAL PROPOSAL] Diagram 4 — Live Container vs Recovery Snapshot

> **⚠️ HISTORICAL:** This proposal references `SessionRecoveryManager` which was
> **removed** in PR #532.  Retained for design context only.

**Roadmap Item:** R-04  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

The live `MinerSessionContainer` object is also the object serialised and stored as the recovery snapshot.  This means the recovery store holds a reference to (or copy of) the same heavyweight object, including fields that are only relevant to the live connection.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Same Type Used for Live and Recovery                      ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  MinerSessionContainer (heavyweight)                         │   ║
║  │                                                              │   ║
║  │  Live-only fields:                    Recovery fields:       │   ║
║  │  • mutex (not serialisable)           • nSessionID           │   ║
║  │  • active TCP connection handle       • hashGenesis          │   ║
║  │  • template cache pointer             • strFalconKeyID       │   ║
║  │  • push notification subscribers     • vChacha20Key          │   ║
║  │                                       • vRewardHash          │   ║
║  │  Both sets of fields live in one object.                     │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  SessionRecoveryManager stores: MinerSessionContainer (full copy)   ║
║                                                                      ║
║  RISK: Recovery store holds live-only fields that waste memory.      ║
║        Serialisation code must skip live-only fields manually.       ║
║        New live-only fields are silently included in snapshots.      ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A thin, POD-like `RecoverableSnapshot` struct contains only the fields needed for recovery.  The live container derives a snapshot on demand; the recovery manager stores only snapshots.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Separate Live Container and Recovery Snapshot             ║
║                                                                      ║
║  ┌─────────────────────────────────────────────────────────────┐    ║
║  │  MinerSessionContainer (live, heavyweight)                  │    ║
║  │                                                             │    ║
║  │  All fields (live + recoverable)                            │    ║
║  │                                                             │    ║
║  │  RecoverableSnapshot ToSnapshot() const;  ←── derive        │    ║
║  │  static MinerSessionContainer             ←── reconstruct   │    ║
║  │    FromSnapshot(RecoverableSnapshot&&);                     │    ║
║  └──────────────────────┬──────────────────────────────────────┘    ║
║                         │ ToSnapshot()                              ║
║                         ▼                                          ║
║  ┌─────────────────────────────────────────────────────────────┐    ║
║  │  RecoverableSnapshot (lightweight, POD-serialisable)        │    ║
║  │                                                             │    ║
║  │  uint32_t            nSessionID;                            │    ║
║  │  uint256_t           hashGenesis;                           │    ║
║  │  char                szFalconKeyID[64];  // fixed-size      │    ║
║  │  uint8_t             chacha20Key[32];                       │    ║
║  │  uint8_t             rewardHash[32];                        │    ║
║  │  bool                fRewardBound;                          │    ║
║  │  uint8_t             eLane;                                 │    ║
║  │  int64_t             nLastActivity;                         │    ║
║  │                                                             │    ║
║  │  static_assert(std::is_trivially_copyable_v<this>);         │    ║
║  └─────────────────────────────────────────────────────────────┘    ║
║                         │                                          ║
║                         ▼                                          ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  SessionRecoveryManager                                      │   ║
║  │  std::unordered_map<std::string, RecoverableSnapshot>        │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  GAIN: Snapshots are POD → trivially serialisable, memcpy-safe,     ║
║        cross-architecture stable, and bounded in size.              ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `struct RecoverableSnapshot` defined with only recoverable fields
- [ ] `static_assert(std::is_trivially_copyable_v<RecoverableSnapshot>)`
- [ ] `MinerSessionContainer::ToSnapshot()` produces a `RecoverableSnapshot`
- [ ] `MinerSessionContainer::FromSnapshot()` reconstructs a live container
- [ ] `SessionRecoveryManager` stores `RecoverableSnapshot`, not the full container
- [ ] Cross-arch serialisation test (TC-5.1) uses `RecoverableSnapshot` byte layout
