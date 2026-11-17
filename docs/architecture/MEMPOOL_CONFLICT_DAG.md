# Mempool Conflict DAG (Option C)

**Status:** Implemented  
**Scope:** `src/TAO/Ledger/mempool.{h,cpp}` — conflict admission, parking, reconciliation  
**Tests:** `tests/unit/TAO/Ledger/mempool_conflict_dag.cpp` (`[mempool_conflict_dag]`)

---

## 1. Problem — mempool-driven node/miner liveness failure

A handful of sticky conflict **roots** in RAM (`mapConflicts`) plus normal peer tx
gossip produced a multi-second `ERROR: Accept : CONFLICT: prev tx CONFLICTED`
storm. Descendants of those roots were cascaded into `mapConflicts`, ERROR-logged,
and NOTIFY-relayed.

This was **not** merely a logging or RAM-growth symptom. The cascade put the
node and its attached miner into a **functional liveness doom loop**:

| Observed symptom | What it meant operationally |
|------------------|-----------------------------|
| Intermittent `BESTCHAIN` / `ActivateCandidateBestChain connect=1` | Tip could still tick forward sometimes — conflicts never gate `SetBest` — so the outage looked "soft" |
| Repeated block-check failures | Producer / Accept paths kept seeing conflicted or stranded sigchain state |
| Producer sequencing failures | Local create/sequence work could not advance cleanly past conflicted prev tips |
| Orphan map growth | Incomplete / unconnectable blocks piled up while the conflict tree churned |
| Miner remaining on stale work | Mining template / push path did not recover to fresh tip work |
| Recovery only after **both** node and miner restart | Wiping pure-mempool RAM state (`mapConflicts` and dependents) broke the loop; LLD was not corrupt |

So: chain height was not permanently frozen, but **useful forward progress for
block production and mining stalled in a self-reinforcing loop** until process
restart. Treat the incident as a **mempool-driven node/miner liveness failure**,
not as "BESTCHAIN kept advancing, so the cascade does not wedge operation."

Long-lived `STRANDED_STATE_DETECTED` (DEFERRED_LOCAL_STATE) is a separate,
intentional retain-and-retry path. It can coexist with tip movement, but it is
**not** a license to understate the cascade outage above — the cascade is what
turned sticky roots into a live production/mining stall.

### Diagram A — Before fix: cascade liveness doom loop

```
  Peers                    Node mempool                         Miner
    │                           │                                  │
    │  gossip descendant txs    │                                  │
    │──────────────────────────►│                                  │
    │                           │  sticky ROOT still in            │
    │                           │  mapConflicts                    │
    │                           ▼                                  │
    │                    ┌──────────────────────┐                  │
    │                    │ Accept(descendant)   │                  │
    │                    │ prev is CONFLICTED   │                  │
    │                    │ → ERROR log          │                  │
    │                    │ → insert mapConflicts│  (cascade)       │
    │                    │ → NOTIFY relay       │──────► peers     │
    │                    └──────────┬───────────┘                  │
    │                               │                              │
    │                               ▼                              │
    │                    block Check / producer                    │
    │                    sequencing failures                       │
    │                    orphan growth                             │
    │                               │                              │
    │                               │  intermittent BESTCHAIN      │
    │                               │  (connect=1 still possible)  │
    │                               │                              │
    │                               │   stale / no fresh work      │
    │                               └─────────────────────────────►│
    │                                                              │
    │                         ✖ useful mining stalled ✖            │
    │                         ✖ loop until node+miner restart ✖    │
```

```
                    mapConflicts BEFORE Option C
  ─────────────────────────────────────────────────────────
   ROOT (sticky tip disagreement)
     ├─ child   ← also inserted as "conflict", ERROR+NOTIFY
     │    ├─ grandchild  ← cascaded again
     │    └─ …
     └─ sibling tail     ← cascaded again

  Result: ERROR storm + relay amplification + producer/miner stall
          (tip may still advance intermittently — liveness, not LLD wedge)
```

---

## 2. Design — Option C

```
                    ┌─────────────────────────────┐
                    │ mapConflicts (ROOTS only)   │
                    │  + mapConflictRootByGenesis │
                    └─────────────┬───────────────┘
                                  │ hashPrevTx points at root
                                  ▼
                    ┌─────────────────────────────┐
                    │ mapConflictDependents       │
                    │  (keyed by parent hash)     │
                    │ mapConflictDependentsByIndex│
                    └─────────────────────────────┘
```

| Rule | Behavior |
|------|----------|
| R1 | Only **direct tip disagreements** become roots (CLAIMED prev, `hashPrevTx != ReadLast`, duplicate genesis) |
| R2 | Children of a conflict node **park as dependents** — no ERROR, no NOTIFY relay, not inserted into `mapConflicts` |
| R3 | One dependent slot per parent; earliest `nSequence` wins |
| R4 | Eviction / INVALID_ABSOLUTE drops the **whole tree** (`DropConflictTree`) |
| R5 | `mapConflictRootByGenesis` tracks earliest root per genesis for O(1) diagnostics |
| R6 | `Conflicts()` = root count only (BESTCHAIN health signal). `ConflictDependents()` = parked tail size |
| R7 | On resolve (disk tip matches root prev), re-Accept root then `ProcessConflictDependents` |

### Diagram B — After fix: root-only store + dependent parking

```
  Peers                    Node mempool                         Miner
    │                           │                                  │
    │  gossip descendant txs    │                                  │
    │──────────────────────────►│                                  │
    │                           │  sticky ROOT in mapConflicts     │
    │                           ▼                                  │
    │                    ┌──────────────────────┐                  │
    │                    │ Accept(descendant)   │                  │
    │                    │ IsConflictNode(prev) │                  │
    │                    │ → ParkConflictDependent                 │
    │                    │   (no ERROR, no NOTIFY,                 │
    │                    │    not a new root)   │                  │
    │                    └──────────┬───────────┘                  │
    │                               │                              │
    │                               ▼                              │
    │                    Conflicts() stays small                   │
    │                    ConflictDependents() holds tails          │
    │                    Check()/resolve drains tree               │
    │                               │                              │
    │                               │  BESTCHAIN + fresh templates │
    │                               └─────────────────────────────►│
    │                                                              │
    │                         ✔ mining stays on live work          │
```

```
                    mapConflicts AFTER Option C
  ─────────────────────────────────────────────────────────
   ROOT only  ──park──► dependent (one slot / parent)
                           └──park──► next dependent …

  Evict root        → DropConflictTree (whole chain gone)
  Resolve root      → re-Accept root → ProcessConflictDependents
```

---

## 3. Accept path (non-first tx)

```
HasTx(prev, MEMPOOL)?
  no  → ORPHAN (unchanged)
  yes →
    mapClaimed(prev)?     → AddConflictRoot + optional NOTIFY   [ROOT]
    IsConflictNode(prev)?
      prev on disk        → clear stale DAG markers, ProcessConflictDependents(prev), fall through
      else                → ParkConflictDependent               [DEPENDENT]
    ReadLast != hashPrev  → AddConflictRoot + optional NOTIFY   [ROOT]
    else                  → Verify/Connect as live mempool tx
```

Soft failures (orphan / root conflict / parked dependent / DEFERRED_LOCAL_STATE)
do **not** set `mapRejected`. LLP only ban-scores `mempool.Rejected()`.

### Diagram C — Accept decision flow

```
                    non-first tx arrives
                            │
                            ▼
                   HasTx(prev, MEMPOOL)?
                      │           │
                     no          yes
                      │           │
                      ▼           ▼
                   ORPHAN    mapClaimed(prev)? ──yes──► AddConflictRoot [ROOT]
                                  │ no
                                  ▼
                           IsConflictNode(prev)?
                            │ yes            │ no
                            ▼                ▼
                     prev on disk?     ReadLast != hashPrevTx?
                      │ yes    │ no     │ yes              │ no
                      ▼        ▼        ▼                  ▼
              clear stale   ParkDep   AddConflictRoot   Verify/Connect
              + ProcessDeps [DEP]     [ROOT]            (live mempool)
```

---

## 4. Observability

BESTCHAIN log (non-sync):

```
=== BESTCHAIN === height=… mempool_conflicts=N mempool_conflict_deps=M mempool_size=S
```

- `mempool_conflicts=N mempool_size=0` with a moving tip ⇒ stranded **roots** still present; tip movement alone does **not** prove producer/miner liveness is healthy.
- Growing `mempool_conflict_deps` with stable `mempool_conflicts` ⇒ peers offering tails of sticky roots; Check() eviction / resolve should drain them.
- Correlate with block-check failures, producer sequencing errors, orphan growth, and miner stale-work symptoms before declaring the node healthy.
- System API: `conflicts` + `conflict_deps`.

---

## 5. Relationship to prior options

| Option | What | Status |
|--------|------|--------|
| A | Stop CONFLICTED-prev cascade; soft-fail ban score; Check `continue` | Kept (folded into C) |
| B | Remove conflicts from `Has()` | Not chosen — dependents must stay "known" so they are not mis-classified as missing orphans |
| **C** | Per-genesis root-only store + dependent index | **This document** |
| D/E/F | Log-only / wipe-on-BESTCHAIN / no-relay-only | Rejected as incomplete — would leave the liveness loop intact |

---

## 6. Out of scope

- Legacy `mapLegacyConflicts` shape (unchanged).
- Changing DEFERRED_LOCAL_STATE / UNKNOWN retry budgets.
- Sigchain producer sequencing in `CreateTransaction` (separate NSEQ work).

---

## 7. Living audit links (2026-08-10)

Post-#688/#689/#690/#691 scoreboard, residual TIPs (including retry-map cap
fairness), and mempool↔recovery coupling diagram:

- [`docs/current/node/audit/NODE_AUDIT_2026-08-10.md`](../current/node/audit/NODE_AUDIT_2026-08-10.md)
- [`docs/diagrams/audit/mempool-recovery-coupling.md`](../diagrams/audit/mempool-recovery-coupling.md)
