# Diagram — Mempool ↔ Recovery Coupling (Coordinator view)

**Status:** Classifier + Option C on NODE; ForceLocalChainResync still spec-only  
**Code:** `Mempool::Check` / `ComputeForkDivergence` / conflict DAG; `AttemptPeerBestChainRecovery`  
**Docs:** `MEMPOOL_CONFLICT_DAG.md`, `FORK_RECOVERY_KNOWLEDGE_BASE.md`

---

## Two clocks that must both succeed

```
   ┌─────────────────────────────────────────────────────────────┐
   │                     NODE liveness                           │
   │                                                             │
   │   Mempool clock                          Chain clock        │
   │   ─────────────                          ───────────        │
   │   conflict roots in RAM                  hashBestChain      │
   │   DEFERRED / UNKNOWN budgets             orphan pool        │
   │   30s Check() sweep                      peer LIST fetch    │
   │                                                             │
   │   If mempool clock expires BEFORE        If chain clock     │
   │   chain fetch lands → evict good data    never starts →     │
   │   (pre-#664 binary classifier)           UNKNOWN burns      │
   │                                          (pre-#690 A1)      │
   └─────────────────────────────────────────────────────────────┘
```

---

## After #664 + #688 + #690 — intended coupling

```
   peer txs / producer
          │
          ▼
   Accept ──► root? ──yes──► mapConflicts (ROOT)
          │                      │
          │                      ├── dependents park (no ERROR/NOTIFY)
          │                      ▼
          │                 Check() every 30s / on block connect
          │                      │
          │         ┌────────────┼────────────────┐
          │         ▼            ▼                ▼
          │     DEFERRED      UNKNOWN        INVALID_ABSOLUTE
          │     retry≤20      retry≤80         DropConflictTree
          │         │            │
          │         │            ├── GET TRANSACTION (gap fill)
          │         │            │
          │         │            └── needs blocks on disk
          │         │                       │
          │         │                       ▼
          │         │            AttemptPeerBestChainRecovery
          │         │            LIST+TRANSACTIONS (A1)
          │         │                       │
          │         ▼                       ▼
          │     resolve root ◄──── branch connects / SetBest
          │         │
          │         ▼
          │   ProcessConflictDependents → re-Accept tails
          │         │
          │         ▼
          │   [TIP-07] template flush / miner push  ← still soft-coupled
          ▼
   live mempool
```

---

## Option C storage shape

```
   mapConflicts (ROOTS only)          mapConflictDependents
   ┌─────────────────────┐            ┌──────────────────────┐
   │ genesis G → root R  │──park──►   │ parent R → child C1  │
   └─────────────────────┘            │ parent C1→ child C2  │
            │                         └──────────────────────┘
            ▼
   mapConflictRootByGenesis[G] = R     (O(1) diagnostics)
```

---

## Still missing: ForceLocalChainResync (TIP-04)

```
   UNKNOWN budget exhausted
   AND peer_height - local_height large
   AND conflict genesis is OUR miner sigchain
            │
            ▼
   ┌─────────────────────────────────────────┐
   │  ForceLocalChainResync (NOT BUILT)      │
   │  -allowlocalchainresync default OFF     │
   │  multi-peer corroboration               │
   │  bounded disconnect via SetBest path    │
   │  re-admit surviving sigchain txs        │
   └─────────────────────────────────────────┘
            │
            ▼ today without it
   operator restart / manual -revertblocks
```

---

## Cap foot-gun (TIP-19)

```
   continuous conflict churn
            │
            ▼
   map* size >= 10000 ──clear() ALL retries──► budgets reset
            │
            ▼
   documented 10/40 min ceilings become soft
```

---

## Acceptance coupling checklist

- [x] UNKNOWN ≠ INVALID_ABSOLUTE (#664)
- [x] Roots-only cascade stop (#688)
- [x] Stale DAG marker clears genesis retries (#689)
- [x] Far-tip fetch can start before UNKNOWN expires (#690)
- [x] Escalation does not thrash LIST windows (#691)
- [ ] Resolve → miner template flush explicit (TIP-07)
- [ ] Retry maps fair eviction (TIP-19)
- [ ] Manual/auto local chain resync (TIP-04, gated)
