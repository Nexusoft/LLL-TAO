# Recces — NODE Post #690 / #691

Living reconnaissance notes. Prefer this over session memory.  
**Baseline:** `NODE` @ `3c3f468` (2026-08-10).  
**Companion:** [NODE_AUDIT_2026-08-10.md](NODE_AUDIT_2026-08-10.md)

---

## R0 — How to re-verify quickly

```bash
# Recovery coordinator
grep -n "PeerBestRecoveryResult\|RequestMissingTxBranchRecovery\|pfBranchSyncQueued" \
  src/TAO/Ledger/include/process.h src/TAO/Ledger/process.cpp src/LLP/tritium.cpp

# Far-tip must NOT early-return only on !fInOrphanPool
grep -n "in_orphan_pool=\|locator-branch-sync-request\|block-not-yet-received" \
  src/TAO/Ledger/process.cpp

# SPECIFIER discipline
grep -n "SPECIFIER::SYNC\|SPECIFIER::TRANSACTIONS" src/LLP/tritium.cpp src/TAO/Ledger/process.cpp

# DDoS defaults
grep -n 'GetBoolArg.*-ddos\|GetArg.*-rscore' src/LLP/global.cpp

# Harness
grep -n "EnsureUnitTestEnvironment\|CATCH_CONFIG_RUNNER" tests/unit/main.cpp
```

---

## R1 — Peer-best recovery coordinator (post A1)

### Symbols

| Symbol | File | Notes |
|--------|------|-------|
| `PeerBestRecoveryResult` | `process.h` ~344–350 | `SKIPPED / PROGRESS / FETCH_QUEUED / FETCH_THROTTLED` |
| `AttemptPeerBestChainRecovery(...)` | `process.cpp` ~520–836 | Optional `pfBranchSyncQueued`; returns enum |
| `RequestMissingTxBranchRecovery(...)` | `process.cpp` ~839+ | Missing-tx orchestration; suppresses fallback LIST on QUEUED/THROTTLED/PROGRESS |
| `ShouldSendBranchSyncRequest(hashAncestor)` | `process.cpp` ~965+ | Owns `PROCESSING_MUTEX`; key = missing ancestor, **not** tip |
| `ORPHAN_REQUEST_THROTTLE_SECONDS` | `process.h` ~180 | **3 seconds** |
| `MAX_BRANCH_RECOVERY_ESCALATIONS` | `process.h` ~164 | **3** |

### Control flow (current)

```
AttemptPeerBestChainRecovery(peer_best)
  guard re-entrancy (thread_local) → SKIPPED if nested
  disabled (-peerbestchainrecovery=0) → SKIPPED
  peer_best on disk?
    yes → heavier? FindCommonAncestor → ActivateCandidateBestChain → PROGRESS|SKIPPED
    no  → orphan walkback (visited-set, MAX_BLOCK_ORPHANS)
            connectable? mapOrphans.Remove + Process → PROGRESS|SKIPPED
            else LIST+TRANSACTIONS(locator→peer_best) throttled
                 fanout 2nd peer if distinct
                 → FETCH_QUEUED | FETCH_THROTTLED | SKIPPED
```

### Call sites

| Site | File:area | Passes pnode? | Uses coordinator? |
|------|-----------|---------------|-------------------|
| Missing-tx escalation | `tritium.cpp` ~3113 | yes (`this`) | **Yes** `RequestMissingTxBranchRecovery` |
| BESTCHAIN notify | `tritium.cpp` BESTCHAIN | `RequestBestChainBranchRecovery` (pnode + result enum) | **Yes** TIP-01/02 |
| Process internal | `process.cpp` other LIST sites | varies | local |

### Residual recce (open)

1. ~~**BESTCHAIN unthrottled LIST**~~ — closed via `RequestBestChainBranchRecovery` (TIP-01/02).
2. ~~**BESTCHAIN `!fKnownBest`**~~ — always enters coordinator with notifying peer (TIP-02).
3. Fanout peer selection is `RandomConnection()` — may pick disconnected; missing-tx fanout elsewhere requires `Connected()` (memory claims; verify if hardening needed).

---

## R2 — Missing-tx soft fail + escalation

### Invariants (still true)

- Missing txs are `PROCESS::INCOMPLETE`, not `Check()` failure.
- `mapLastMissing[hash]` erased when retries exceed `MAX_MISSING_TRANSACTIONS_RETRIES` (not left at 51+).
- Rate-limit early return sets `block.hashMissing = hashBlock` so LLP takes cheap per-tx path (not hashMissing==0 escalation).
- `mapLastMissingProcessTime` seeded when counter first set to 1.

### Escalation ladder (post #691)

```
INCOMPLETE + retries exhausted
  → erase mapLastMissing
  → RequestMissingTxBranchRecovery(peer_best, hashBlock, ...)
       1) AttemptPeerBestChainRecovery → may LIST
       2) fallback LIST only if result==SKIPPED
  → random peer GET BLOCK+TRANSACTIONS
  → fanout missing tx hashes across Connected() peers
```

### Tests

- `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` — soft-fail, A1 far-tip on-wire (socketpair), single-LIST coordination, throttle.

---

## R3 — Mempool conflict DAG (Option C)

### State

| Piece | Location | Value |
|-------|----------|-------|
| Roots only | `mapConflicts` | max 10000 |
| Dependents | `mapConflictDependents` (+ByIndex) | max 10000 |
| Per-genesis root index | `mapConflictRootByGenesis` | O(1) |
| DEFERRED retries | `mapConflictRetries` | max 20 (~10 min @ 30s) |
| UNKNOWN retries | `mapUnknownAncestorRetries` | max 80 (~40 min @ 30s) |
| Sweep interval | `CONFLICTS_SWEEP_INTERVAL_SECONDS` | 30s (LLP GENERIC) |
| Bound helper | `BoundConflictDAG()` | wholesale clear at cap |
| Stale marker clear | #689 | clears genesis retry state |

### Classifier (still 3-way)

```
ComputeForkDivergence
  disk last matches → Resolved
  ancestor on main  → DEFERRED_LOCAL_STATE
  ancestor missing  → UNKNOWN  (GET TRANSACTION budget)
  ancestor off-chain→ INVALID_ABSOLUTE (DropConflictTree)
```

### Docs

- Living: `docs/architecture/MEMPOOL_CONFLICT_DAG.md`
- Archive history: `docs/archive/FORK_RECOVERY_KNOWLEDGE_BASE.md` (do not delete)

### Residual

- No automatic local-chain eviction (`ForceLocalChainResync` still spec-only).
- Resolve → mining template flush coupling not explicit (TIP-07).

---

## R4 — DDoS / permissions / AddressManager

| Item | Current | Evidence |
|------|---------|----------|
| TRITIUM `-ddos` default | **false** | `global.cpp` ~227 |
| TRITIUM `-rscore` default | **2000** | `global.cpp` ~237 |
| API `-apiddos` default | **true** | `global.cpp` ~286 |
| Ban check | `rSCORE.Score() > DDOS_rSCORE` | `data.cpp` ~611 |
| GET BLOCK cost | +50 rSCORE (approx; see tritium sites) | high-volume sync foot-gun if -ddos on |
| AddressManager safe Get | `bool Get(addr, TrustAddress&)` | used by tritium ADDRESS + system initialize |
| Throwing Get | `[[deprecated]]` | `manager.cpp` ~210 `at()` — unused live path (TIP-17) |

---

## R5 — Sync / PrimeCheck foot-gun

Primary `Process()` path: `block.Check()` with **fForceProof=false** (`process.cpp` ~1311–1325).  
Orphan-drain BFS: also false (`~1654+`).  
Targeted recovery after N Check failures: intentional `Check(true)`.

`TritiumBlock::CheckInternal`: `fForceProof \|\| !Synchronizing()` gates `VerifyWork` / PrimeCheck.

**Regression signature:** multi-day IBD if someone “helps” by forcing proof on the bulk path.

---

## R6 — Unit harness

| Item | Status |
|------|--------|
| `CATCH_CONFIG_RUNNER` + one-shot env | Present (`tests/unit/main.cpp`) |
| LLD Logical + Sessions created | Yes (avoids RefreshEvents nullptr) |
| Single `LLP::Initialize()` | Yes (no double API thread terminate) |
| `[ledger]` segfault from skipped `[args]` | Fixed |
| Remaining ledger assertion failures | Open TIP-05 |

Build/run:
```bash
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
./nexus "[ledger][process]"
./nexus "[ledger][a1]"
./nexus "[mempool_conflict_dag]"
./nexus "[cleanup]"
```

---

## R7 — Mining session coordinator (stateless)

| Concern | Recce note |
|---------|------------|
| `ValidateConsistency` | Present on MINER_SET_REWARD (`stateless_miner.cpp` ~2002) and SUBMIT_BLOCK (stateless_miner_connection ~1779; legacy miner path ~3108). Roadmap R-02 status text is stale → TIP-14. |
| Session container | Authoritative blob model documented under `docs/current/node/session-container-architecture.md` |
| Upgrade diagrams | `docs/diagrams/upgrade-path/01`–`15` still valid targets |
| Stale submit anti-doom-loop | Option A comments in miner headers; soak + multi-miner tests still thin |

---

## R8 — SPECIFIER matrix (do not regress)

| Path | Specifier | When |
|------|-----------|------|
| `TritiumNode::Sync()` initial LIST | `SYNC` | Only while establishing sync session |
| LASTINDEX continuation LIST during sync | `SYNC` | Sync peer only |
| BESTCHAIN post-sync branch request | `TRANSACTIONS` | Already-synced receiver |
| A1 / orphan-gap recovery LIST | `TRANSACTIONS` | Post-sync fork recovery |
| Missing-tx escalation LIST | `TRANSACTIONS` | Post-sync |
| Client mode variants | `CLIENT` | `fClient` |

**Foot-gun:** `SYNC` after `fSynchronized==true` → “unsolicited sync block” → force disconnect.

---

## R9 — Archive map (retained on purpose)

| Archive doc | Why kept |
|-------------|----------|
| `FORK_RECOVERY_KNOWLEDGE_BASE.md` | Full AttemptForkRecovery autopsy + ForceLocalChainResync spec |
| `MISSING_TX_FORK_WEDGE_BUG.md` | mapLastMissing permanent wedge |
| `NSEQ_DIAG_MEMPOOL_*` | Producer sequence / hashLast bugs |
| PR summaries under `docs/archive/pr-summaries/` | How we got here |

Upstream Colin may drop archive copies in a cherry-pick; **NODE keeps them**.

---

## R10 — Change log for this recce pack

| Date | What |
|------|------|
| 2026-08-10 | Rewritten against post-#690/#691 NODE tip; A1/A1b closed; TIP list opened |
