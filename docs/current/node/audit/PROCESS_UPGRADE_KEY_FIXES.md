# Process Upgrade Series — Key Fixes

**Audience:** reviewers, operators, and agents picking up recovery work  
**Scope:** ledger `Process()` / peer-best recovery / missing-tx escalation / BESTCHAIN coordination from roughly PR **#656** through the **near-tip orphan-pool exclusion**  
**Living audit:** [NODE_AUDIT_2026-08-10.md](NODE_AUDIT_2026-08-10.md) · [FOOT_GUNS.md](FOOT_GUNS.md) · [RECCES.md](RECCES.md)  
**Diagrams:** [process-upgrade-series.md](../../../diagrams/audit/process-upgrade-series.md) · [recovery-coordinator-upgrade.md](../../../diagrams/audit/recovery-coordinator-upgrade.md) · [mempool-recovery-coupling.md](../../../diagrams/audit/mempool-recovery-coupling.md)

---

## 0. Why this series exists

Several independent defects could leave a synced node **connected to peers but permanently behind** (or burning DataThread budget) until restart:

1. Missing-tx retry counters that never cleared → permanent incomplete silence  
2. Far peer tips that only logged “block-not-yet-received” → no LIST fetch  
3. Double LIST / window replace after recovery already queued work  
4. BESTCHAIN notify path that bypassed the coordinator throttle  
5. Near-tip spam suppression that could skip the only useful recovery path for orphans  

This document highlights the **most important** fixes and the invariants they established. Prefer the diagrams for flow; use FOOT_GUNS when changing nearby code.

---

## 1. Architecture at a glance

```
                    ┌─────────────────────────────────────────┐
                    │         Recovery coordinator            │
                    │   AttemptPeerBestChainRecovery          │
                    │   PeerBestRecoveryResult enum           │
                    └───────────────────┬─────────────────────┘
                                        │
              ┌─────────────────────────┼─────────────────────────┐
              │                         │                         │
              ▼                         ▼                         ▼
     Missing-tx escalate      BESTCHAIN notify           (future) manual
     RequestMissingTx…        RequestBestChain…          ForceLocalChainResync
              │                         │
              └────────────┬────────────┘
                           ▼
              LIST+TRANSACTIONS / orphan walk / activate
                           │
                           ▼
                      Process(block)
                  ACCEPTED → orphan BFS drain
                  INCOMPLETE → mapLastMissing + fanout
```

Full decision trees: [process-upgrade-series.md](../../../diagrams/audit/process-upgrade-series.md).

---

## 2. Key fixes (ordered by impact)

### KF-1 — Erase `mapLastMissing` on retry limit (not leave at 51+)

| | |
|--|--|
| **Problem** | After `MAX_MISSING_TRANSACTIONS_RETRIES`, code cleared `vMissing` / `hashMissing` but left the counter above the limit. Every later arrival immediately re-hit the guard → **permanent silence** on that block while the local miner extended a dead fork. |
| **Fix** | **Erase** the map entry and escalate (branch recovery / peer-best). Same pattern on primary path and orphan-drain. |
| **PRs / docs** | #656 lineage · [MISSING_TX_FORK_WEDGE_BUG.md](../../../archive/MISSING_TX_FORK_WEDGE_BUG.md) · FOOT_GUNS FG-06 |
| **Invariant** | Retry-limit is a **signal to escalate**, not a permanent blacklist by itself (terminal blacklist is a separate, explicit structure). |

```
  mapLastMissing[h] > MAX
        │
        ├─ BEFORE: leave counter → forever INCOMPLETE no-op
        └─ AFTER:  erase(h) + escalate via coordinator / LIST
```

---

### KF-2 — A1 far-tip recovery actually fetches (#690)

| | |
|--|--|
| **Problem** | `AttemptPeerBestChainRecovery` logged `block-not-yet-received` when the peer tip was not on disk and not in the orphan pool — **no network fetch**. Large gaps sat until mempool UNKNOWN budgets expired (~40 min) or restart. |
| **Fix** | Not-on-disk / not-orphan and orphan-gap share one throttled locator `LIST` + `SPECIFIER::TRANSACTIONS` path (optional one-peer fanout). Throttle key = deepest missing ancestor. |
| **PRs / docs** | #690 · recovery-coordinator-upgrade “Before/After” · FOOT_GUNS FG-01 (specifier) |
| **Invariant** | Post-sync recovery LIST uses **`SPECIFIER::TRANSACTIONS`**, never `SYNC` (unsolicited sync reject). |

```
  peer_best not on disk
        │
        ├─ connectable orphan → Remove + Process(ancestor) → PROGRESS?
        └─ else → ShouldSendBranchSyncRequest(ancestor)?
                     yes → LIST+TRANSACTIONS+LOCATOR → FETCH_QUEUED
                     no  → FETCH_THROTTLED
```

---

### KF-3 — A1b single LIST / result enum (#691)

| | |
|--|--|
| **Problem** | Callers always sent a second locator LIST after recovery, replacing `TxResponseWindow` and defeating the 3s throttle. |
| **Fix** | Orchestrators (`RequestMissingTxBranchRecovery`, later `RequestBestChainBranchRecovery`) switch on `PeerBestRecoveryResult`: only **`SKIPPED`** may fallback-LIST. |
| **PRs / docs** | #691 · FOOT_GUNS FG-05 |
| **Invariant** | `FETCH_QUEUED` and `FETCH_THROTTLED` both **suppress** fallback. |

---

### KF-4 — BESTCHAIN joins the coordinator (TIP-01 / TIP-02, #694)

| | |
|--|--|
| **Problem** | Missing-tx path was hardened; BESTCHAIN notify still had an unthrottled / partial path → residual double-LIST asymmetry. |
| **Fix** | `RequestBestChainBranchRecovery` always routes eligible tips through `AttemptPeerBestChainRecovery` and applies the same fallback rules. |
| **PRs / docs** | #694 · RECCES R1 |
| **Invariant** | One coordinator box owns throttle + fanout for both arrows. |

---

### KF-5 — Near-tip race skip is inventory-owned only

| | |
|--|--|
| **Problem** | After #694, every subscribed peer emitted PEER_BEST_RECOVERY + LIST + fanout for the normal tip-advance race (BLOCK + BESTCHAIN + BESTHEIGHT ordering with stale height). |
| **Fix (step 1)** | Skip coordinator when unknown tip is within `BESTCHAIN_NEAR_TIP_HEIGHT_SLACK` (1) of local height. |
| **Fix (step 2)** | Skip **only if** the NOTIFY handler already queued a matching **BLOCK inventory GET** (`fMatchingBlockInventoryGet`). Sync() / relay without BLOCK still recovers. |
| **Invariant** | Near-tip skip is a **race optimization**, not a gap detector. |

```
  equal / +1 height unknown tip
        │
        ├─ matching BLOCK GET queued?  no  → recover
        └─ yes → inventory owns race → SKIP LIST spam
```

---

### KF-6 — Exclude orphan-pool tips from the near-tip shortcut (this change)

| | |
|--|--|
| **Problem** | A tip already in `mapOrphans` could still match the inventory-owned shortcut. The subsequent BLOCK GET is a **duplicate**: `Process()` returns `ORPHAN` immediately when `mapOrphans.Contains(hash)`. The useful path is the coordinator walkback / missing-branch LIST. If the original recovery request was lost, near-tip skip left the node stuck. |
| **Fix** | Under `PROCESSING_MUTEX`, if `mapOrphans.Contains(hashPeerBest)`, **do not** take the near-tip shortcut — always enter `AttemptPeerBestChainRecovery`. |
| **Tests** | `BESTCHAIN recovery skips near-tip unknown race` — orphan-seeded near-tip still queues LIST; throttle key = orphan `hashPrevBlock`. |
| **Foot gun** | FOOT_GUNS **FG-20** |
| **Invariant** | Inventory shortcut requires: not on disk, **not in orphan pool**, peer at/ahead, height delta ≤ slack, matching BLOCK GET. |

```
  tip in mapOrphans + near-tip + matching GET
        │
        ├─ BEFORE: SKIP → GET → Process ORPHAN no-op → stall if LIST lost
        └─ AFTER:  coordinator walkback / gap LIST (ancestor throttle key)
```

---

### KF-7 — Soft incomplete + rate-limit semantics

| | |
|--|--|
| **Problem** | Missing txs are not `Check()` failures; rate-limit early returns with `hashMissing == 0` were misread by LLP as “retry limit exceeded” and triggered full escalation storms. |
| **Fix** | Rate-limit path sets `hashMissing = hashBlock` and returns `INCOMPLETE` so LLP takes the cheap per-tx re-request branch. Seed `mapLastMissingProcessTime` when the counter is first set to 1. |
| **PRs / docs** | #666 · FOOT_GUNS FG-07 / FG-08 |
| **Invariant** | Soft throttle ≠ escalation; only explicit erase/limit/blacklist paths escalate. |

---

### KF-8 — Never force full PrimeCheck on bulk IBD

| | |
|--|--|
| **Problem** | Primary `Process()` calling `Check(true)` / `fForceProof=true` disables the Synchronizing() fast path → multi-day sync. |
| **Fix** | Primary + orphan-drain use `Check()` default (`fForceProof=false`). Targeted recovery may still force proof after repeated rejects. |
| **PRs / docs** | #674 lineage · FOOT_GUNS FG-02 · source-guard unit test |
| **Invariant** | Bulk ingestion stays fast-path capable. |

---

### KF-9 — Mempool three-state classifier + Option C DAG

| | |
|--|--|
| **Problem** | Binary fork classifier treated “ancestor not found” as absolute invalid → permanent eviction of correct-chain data during sync gaps. Conflict cascade ERROR/NOTIFY doom-loops. |
| **Fix** | `DEFERRED_LOCAL_STATE` / `UNKNOWN` / `INVALID_ABSOLUTE`; UNKNOWN retry budget + GET TRANSACTION; conflict DAG parks dependents on roots only. |
| **PRs / docs** | #664 / #688 / #689 · mempool-recovery-coupling · FORK_RECOVERY_KNOWLEDGE_BASE |
| **Invariant** | Mempool clock must not expire good data before chain fetch can land (two-clock coupling). |

---

### KF-10 — Supporting hygiene (non-Process but recovery-adjacent)

| Fix | Why it matters for recovery |
|-----|----------------------------|
| DDoS defaults restored after #667 silent merge | High-volume GET BLOCK (+50 rSCORE) must not self-ban legitimate sync peers |
| AddressManager non-throwing `Get` | Avoid TOCTOU `out_of_range` → disconnect healthy peers mid-recovery |
| Ledger unit harness `EnsureUnitTestEnvironment` | `[ledger]` suite must boot before Catch tag filters (#690) |
| Orphan connectable must `mapOrphans.Remove` before `Process` | Presence in pool short-circuits to ORPHAN (FG-03) |

---

## 3. Specifier discipline (cross-cutting)

```
  Sync() / LASTINDEX while syncing     →  SPECIFIER::SYNC        OK
  BESTCHAIN / A1 / missing-tx / orphan →  SPECIFIER::TRANSACTIONS REQUIRED
  client mode recovery                 →  SPECIFIER::CLIENT
```

Breaking this produces `unsolicited sync block` drops and forced disconnects during fork recovery (FG-01).

---

## 4. Test map (regression anchors)

| Tag / case theme | Guards |
|------------------|--------|
| `[a1]` far-tip LIST on-wire | KF-2 |
| missing-tx single LIST / throttle | KF-3 |
| BESTCHAIN single LIST / throttle | KF-4 |
| `[near-tip]` inventory GET gate + orphan exclusion | KF-5, KF-6 |
| mapLastMissing erase / blacklist / rate-limit | KF-1, KF-7 |
| PrimeCheck source guard | KF-8 |
| orphan walkback / pool FIFO | KF-2, FG-03 |

Primary harness file: `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp`.

```bash
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
./nexus "[ledger][process][a1]"
./nexus "[ledger][process][bestchain]"
./nexus "[near-tip]"
```

---

## 5. Residual / next (not closed by this series)

| Item | Notes |
|------|--------|
| TIP-03 production soak | Log signatures for A1 / BESTCHAIN in the field |
| TIP-04 `ForceLocalChainResync` | Spec-only manual escape after UNKNOWN budget |
| TIP-06/07/08 | UNKNOWN observability; template flush coupling; unified admissibility |
| TIP-18 | Residual audit of #667 silent-merge surface |

Chooser scoreboard: [NODE_AUDIT_2026-08-10.md](NODE_AUDIT_2026-08-10.md).

---

## 6. Operator-facing log signatures

| Signature | Meaning |
|-----------|---------|
| `=== PEER_BEST_RECOVERY ===` … `locator-branch-sync-request` | A1/coordinator active fetch |
| `=== PEER_BEST_RECOVERED ===` | Orphan walkback or heavier activation advanced tip |
| `BESTCHAIN near-tip race; deferring recovery to block inventory` | Inventory-owned skip (should be rare in logs at level 2) |
| `BESTCHAIN differs; requesting branch` | Fallback LIST after SKIPPED |
| missing-tx retry / branch escalation warnings | KF-1 escalation path |

If near-tip skip dominates while the node stays one tip behind with orphans present, verify KF-6 (orphan exclusion) is deployed.
