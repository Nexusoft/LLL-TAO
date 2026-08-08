# Mempool Conflict DAG (Option C)

**Status:** Implemented  
**Scope:** `src/TAO/Ledger/mempool.{h,cpp}` — conflict admission, parking, reconciliation  
**Tests:** `tests/unit/TAO/Ledger/mempool_conflict_dag.cpp` (`[mempool_conflict_dag]`)

---

## 1. Problem

A handful of sticky conflict **roots** in RAM (`mapConflicts`) plus normal peer tx
gossip produced a multi-second `ERROR: Accept : CONFLICT: prev tx CONFLICTED`
storm. Descendants of those roots were cascaded into `mapConflicts`, ERROR-logged,
and NOTIFY-relayed. Best chain kept advancing (`ActivateCandidateBestChain
connect=1`) because conflicts never gate `SetBest`. A full node restart wiped
`mapConflicts` and "fixed" the symptom — pure mempool RAM state, not LLD
corruption.

Long-lived `STRANDED_STATE_DETECTED` (DEFERRED_LOCAL_STATE) is a separate,
intentional retain-and-retry path and does **not** wedge chain operation the way
the cascade storm did.

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

---

## 4. Observability

BESTCHAIN log (non-sync):

```
=== BESTCHAIN === height=… mempool_conflicts=N mempool_conflict_deps=M mempool_size=S
```

- `mempool_conflicts=N mempool_size=0` with healthy tip ⇒ stranded **roots**, not a fork wedge.
- Growing `mempool_conflict_deps` with stable `mempool_conflicts` ⇒ peers offering tails of sticky roots; Check() eviction / resolve should drain them.
- System API: `conflicts` + `conflict_deps`.

---

## 5. Relationship to prior options

| Option | What | Status |
|--------|------|--------|
| A | Stop CONFLICTED-prev cascade; soft-fail ban score; Check `continue` | Kept (folded into C) |
| B | Remove conflicts from `Has()` | Not chosen — dependents must stay "known" so they are not mis-classified as missing orphans |
| **C** | Per-genesis root-only store + dependent index | **This document** |
| D/E/F | Log-only / wipe-on-BESTCHAIN / no-relay-only | Rejected as incomplete |

---

## 6. Out of scope

- Legacy `mapLegacyConflicts` shape (unchanged).
- Changing DEFERRED_LOCAL_STATE / UNKNOWN retry budgets.
- Sigchain producer sequencing in `CreateTransaction` (separate NSEQ work).
