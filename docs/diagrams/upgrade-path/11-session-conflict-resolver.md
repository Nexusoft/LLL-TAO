# Diagram 11 — `SessionConflictResolver`

**Roadmap Item:** R-11  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

If two recovery snapshots exist with the same Falcon key ID (e.g. because of a server-side bug or an unusual reconnect sequence), the recovery manager performs a lookup and returns whichever snapshot it happens to find first.  There is no deterministic conflict resolution and no conflict log.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Non-Deterministic Snapshot Collision                      ║
║                                                                      ║
║  SessionRecoveryManager internal map:                                ║
║    "sha256:ab12" → snapshot_X  (from connection #1, old)            ║
║    "sha256:ab12" → snapshot_X  (DUPLICATE — same key)               ║
║                                                                      ║
║  In std::unordered_map: second insert silently overwrites.          ║
║  In std::multimap: lookup returns arbitrary entry.                   ║
║                                                                      ║
║  Neither choice is documented or tested.                             ║
║                                                                      ║
║  RISK: Miner reconnects and gets the wrong (older) snapshot          ║
║        restored → reward binding from a different session.           ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A `SessionConflictResolver` component detects duplicate Falcon key IDs, logs the conflict with full details, and applies a documented deterministic resolution rule.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — SessionConflictResolver                                   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  SessionConflictResolver                                     │   ║
║  │  (src/LLP/include/session_conflict_resolver.h)               │   ║
║  │                                                              │   ║
║  │  Triggered when:                                             │   ║
║  │    StoreSnapshot(key, newSnapshot) finds key already exists  │   ║
║  │                                                              │   ║
║  │  Resolution steps:                                           │   ║
║  │                                                              │   ║
║  │  1. Log both snapshots:                                      │   ║
║  │     "[CONFLICT] duplicate Falcon key: %s"                    │   ║
║  │     "  existing: session=%u last_activity=%lld reward=%s"    │   ║
║  │     "  incoming: session=%u last_activity=%lld reward=%s"    │   ║
║  │                                                              │   ║
║  │  2. Apply resolution rule:                                   │   ║
║  │     if incoming.nLastActivity > existing.nLastActivity       │   ║
║  │       → replace existing with incoming (newer wins)          │   ║
║  │     else                                                      │   ║
║  │       → keep existing, discard incoming                      │   ║
║  │                                                              │   ║
║  │  3. Log resolution outcome:                                  │   ║
║  │     "[CONFLICT] resolved: kept %s, discarded %s"             │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  Alternative: Freeze-on-conflict mode                        │   ║
║  │                                                              │   ║
║  │  Config: recovery.conflict_mode = freeze                     │   ║
║  │  → Both snapshots quarantined                                │   ║
║  │  → Falcon key ID marked as conflicted                        │   ║
║  │  → Next reconnect with that key starts fresh                 │   ║
║  │  → Miner must re-bind reward                                 │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  GAIN: Conflicts are visible in logs and resolved deterministically. ║
║        No silent state corruption from duplicate snapshots.          ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `SessionConflictResolver` class implemented in `src/LLP/`
- [ ] `StoreSnapshot` calls resolver when key collision detected
- [ ] Conflict log lines emitted with both snapshot summaries
- [ ] Resolution rule: newer `nLastActivity` wins
- [ ] Config flag `recovery.conflict_mode` supports `newer_wins` (default) and `freeze`
- [ ] Unit test (TC-3.4): two snapshots with same Falcon key → deterministic resolution
