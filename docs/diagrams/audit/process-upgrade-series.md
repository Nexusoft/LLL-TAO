# Diagram — Process / Recovery Upgrade Series (#656 → near-tip orphan fix)

**Status:** Living architecture map for the ledger `Process()` and peer-best recovery stack  
**Code:** `src/TAO/Ledger/process.cpp`, `src/TAO/Ledger/include/process.h`, `src/LLP/tritium.cpp`  
**Companions:** [recovery-coordinator-upgrade.md](recovery-coordinator-upgrade.md) · [mempool-recovery-coupling.md](mempool-recovery-coupling.md) · [key-fixes document](../../current/node/audit/PROCESS_UPGRADE_KEY_FIXES.md)

---

## 1. End-to-end control plane (current)

```
                         peer NOTIFY / incomplete block / BESTCHAIN
                                        │
          ┌─────────────────────────────┼─────────────────────────────┐
          ▼                             ▼                             ▼
   TYPES::BLOCK body          missing-tx INCOMPLETE            ACTION::NOTIFY
   (inventory GET path)       (mapLastMissing path)            BESTCHAIN (+ BLOCK?)
          │                             │                             │
          ▼                             ▼                             ▼
      Process(block)          RequestMissingTxBranch      RequestBestChainBranch
          │                   Recovery                    Recovery
          │                             │                             │
          │                             └────────────┬────────────────┘
          │                                          ▼
          │                          AttemptPeerBestChainRecovery
          │                                          │
          │               ┌──────────────────────────┼──────────────────────────┐
          │               ▼                          ▼                          ▼
          │          on disk                   orphan pool                 far tip / gap
          │       heavier activate           walkback + Remove            LIST+TRANSACTIONS
          │               │                  + Process(ancestor)          + TxResponseWindow
          │               │                          │                    + optional fanout
          │               ▼                          ▼                          │
          │           PROGRESS                  PROGRESS /                 FETCH_QUEUED /
          │                                     SKIPPED                    FETCH_THROTTLED
          │                                          │                          │
          ▼                                          └──────────┬───────────────┘
   ACCEPTED / ORPHAN /                                          │
   INCOMPLETE / DUPLICATE /                          PeerBestRecoveryResult
   REJECTED / IGNORED                                           │
                                                     fallback LIST only if
                                                     result == SKIPPED
                                                     (A1b / TIP-01 contract)
```

---

## 2. Timeline of the series (what each PR closed)

```
  #656  mapLastMissing erase on retry-limit   ──► stop permanent incomplete silence
   │
  #657/#658  branch-escalation cap + fanout   ──► bound recovery loops; skip dead peers
   │
  #659/#666  terminal blacklist + rate-limit  ──► budget protection; hashMissing!=0 on throttle
   │
  #664/#688/#689  mempool 3-state + Option C  ──► UNKNOWN gap vs INVALID_ABSOLUTE; DAG roots
   │
  #690  A1 far-tip LIST (no silent no-op)     ──► not-on-disk/not-orphan starts fetch
   │
  #691  A1b single LIST / result enum         ──► no double LIST after QUEUED/THROTTLED
   │
  #694  TIP-01/02 BESTCHAIN → coordinator     ──► same throttle + fanout as missing-tx
   │
  near-tip race skip (post-#694)              ──► equal/+1 height + matching BLOCK GET
   │                                              skips spam LIST during tip advance
   │
  inventory GET gate                          ──► skip only when BLOCK GET actually queued
   │                                              (Sync/relay without BLOCK still recovers)
   │
  orphan-pool exclusion (this change)         ──► tips in mapOrphans never take shortcut;
                                                  coordinator walkback / gap LIST still run
```

---

## 3. BESTCHAIN near-tip decision card (post orphan exclusion)

```
  RequestBestChainBranchRecovery(hash, height, …, fMatchingBlockInventoryGet)

                    hash on disk?
                   /            \
                 yes             no
                  │               │
                  │               ├─ in mapOrphans?  (LOCK PROCESSING_MUTEX)
                  │               │     yes ──────────────────────────────┐
                  │               │     no                                │
                  │               │      │                                │
                  │               │      ▼                                │
                  │               │  peer at/ahead AND                    │
                  │               │  delta ≤ NEAR_TIP_SLACK(1) AND        │
                  │               │  fMatchingBlockInventoryGet?          │
                  │               │     yes → SKIP (inventory owns race)  │
                  │               │     no  ──────────────┐               │
                  │               │                       │               │
                  └───────────────┴───────────────────────┴───────────────┘
                                          │
                                          ▼
                          AttemptPeerBestChainRecovery (coordinator)
```

**Why orphan exclusion matters**

```
  tip already in mapOrphans
           │
           ├─ shortcut SKIP  → later BLOCK GET arrives
           │                      Process() sees mapOrphans.Contains
           │                      → ORPHAN return (no walkback, no LIST)
           │                      → ★ recovery stall if original LIST was lost ★
           │
           └─ no shortcut   → coordinator walkback
                                 connectable ancestor → Remove + Process
                                 orphan gap          → LIST+TRANSACTIONS
                                                      (throttle key = deepest ancestor)
```

---

## 4. Process() state machine (ingress)

```
  Process(block)
       │
       ├─ setUnrecoverableBlocks? → IGNORED (+ refill vMissing from cache)
       │
       ├─ mapLastMissing rate-limit hit?
       │     yes → INCOMPLETE, hashMissing = hashBlock  « FG-07 »
       │
       ├─ LLD HasBlock(hash)? → DUPLICATE (+ purge recovery maps)
       │
       ├─ mapOrphans.Contains(hash)? → ORPHAN return  « duplicate body »
       │
       ├─ !HasBlock(prev)?
       │     → ORPHAN insert; maybe throttled ancestor LIST
       │
       ├─ Check()  [fForceProof=false on primary]  « FG-02 / PrimeCheck »
       │     missing txs → INCOMPLETE + mapLastMissing++
       │        over limit → erase counter + escalate  « FG-06 »
       │
       └─ Accept() → ACCEPTED → orphan BFS drain
```

---

## 5. Result enum contract (shared by missing-tx and BESTCHAIN)

```
  ┌──────────────────┬────────────────────────────────────────────┬─────────────┐
  │ Result           │ Meaning                                    │ Fallback    │
  ├──────────────────┼────────────────────────────────────────────┼─────────────┤
  │ SKIPPED          │ disabled / same tip / no peer / no action  │ allowed*    │
  │ PROGRESS         │ local best advanced                        │ suppressed  │
  │ FETCH_QUEUED     │ primary locator LIST on the wire           │ suppressed  │
  │ FETCH_THROTTLED  │ would fetch; 3s ancestor throttle hit      │ suppressed  │
  └──────────────────┴────────────────────────────────────────────┴─────────────┘
   * still gated by ShouldSendBranchSyncRequest + height / sync checks
```

---

## 6. Related surfaces (not Process, but coupled)

| Surface | Role in the series |
|---------|-------------------|
| `Mempool::Check` 3-state classifier | Holds correct-chain txs during gap (`UNKNOWN`) instead of hard-evict |
| Conflict DAG (Option C) | Parks dependents; stops ERROR/NOTIFY doom-loop |
| `TxResponseWindow` | Tracks LIST/GET TRANSACTIONS budget per peer |
| DDoS defaults (`-ddos`/`-rscore`) | Restored after #667 silent merge; high-volume GET BLOCK must not self-ban |
| AddressManager non-throwing `Get` | Avoid TOCTOU disconnect of healthy peers |

See [mempool-recovery-coupling.md](mempool-recovery-coupling.md) for the two-clock picture.
