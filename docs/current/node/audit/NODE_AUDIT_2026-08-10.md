# NODE Audit — Post PR #690 / #691 (2026-08-10)

**Branch baseline:** `NODE` @ `3c3f468` (includes #691)  
**Prior baseline:** `7caf17d` (#689 Option-C DAG retry clear)  
**Scope:** P2P recovery coordinator, missing-tx escalation, mempool conflict DAG, DDoS defaults, mining-session coordinator foot-guns, upstream isolation readiness  
**Method:** Code-backed recces only (file:line). Archive docs retained as history; this page is the living post-merge scoreboard.  
**Chooser format:** Every open item is a **TIP-xx** you can pick as the next scoped PR.

---

## 0. Executive scoreboard

| Area | Status after #690/#691 (+ follow-ons) | Residual risk |
|------|--------------------------------------|---------------|
| A1 far-tip silent no-op | **FIXED** (#690) | Needs production soak |
| A1 double LIST / window replace | **FIXED** (#691) | — |
| BESTCHAIN → coordinator | **FIXED** (#694 TIP-01/02) | Near-tip inventory + orphan gates |
| Near-tip orphan-pool skip hole | **FIXED** (FG-20 / KF-6) | Duplicate GET is ORPHAN no-op without coordinator |
| Ledger harness `[ledger]` segfault | **FIXED** (#690 harness) | Remaining assertion hygiene debt |
| Mempool Option C DAG | **LANDED** (#688/#689) | UNKNOWN budget expiry still hard |
| DDoS defaults (`-ddos`/`-rscore`) | **RESTORED** (#680 lineage) | Operator override foot-gun remains |
| AddressManager TOCTOU | **FIXED** (#678) | Throwing `Get()` marked `[[deprecated]]` (TIP-17) |
| `ForceLocalChainResync` | **SPEC ONLY** | Highest-risk future work |
| Upstream Colin isolation package | **NOT STARTED** | See TIP-12 + draft checklist |

**Series narrative + diagrams:** [PROCESS_UPGRADE_KEY_FIXES.md](PROCESS_UPGRADE_KEY_FIXES.md) · [process-upgrade-series.md](../../../diagrams/audit/process-upgrade-series.md)

```
                    NODE tip (post #691)
   ────────────────────────────────────────────────────────────
    mempool classifier     missing-tx retries     peer-best recovery
    DEFERRED/UNKNOWN/ABS   mapLastMissing erase   far-tip LIST+TXNS
         │                       │                        │
         └──────────┬────────────┴────────────┬───────────┘
                    ▼                         ▼
            Conflict DAG (C)        RequestMissingTxBranchRecovery
            roots+dependents        PeerBestRecoveryResult enum
                    │                         │
                    └──────────┬──────────────┘
                               ▼
                    production soak + TIP chooser
```

---

## 1. What just landed (closed findings)

### A1 — Far-tip recovery no longer a silent no-op — PR #690

**Was:** `AttemptPeerBestChainRecovery()` logged `action=block-not-yet-received` and returned when peer tip was not on disk and not orphaned — large gaps never started block fetch.

**Now:** Not-on-disk / not-orphan and orphan-gap share one throttled locator `LIST` + `SPECIFIER::TRANSACTIONS` path, optional one-peer fanout, throttle key = `hashDeepestAncestor` (defaults to peer tip when no orphans).

**Evidence:** `src/TAO/Ledger/process.cpp` ~659–794; `PeerBestRecoveryResult` in `include/process.h` ~336–350.

```
peer_best not on disk
        │
        ├─ connectable orphan ancestor → Process(ancestor) → PROGRESS?
        └─ else (far tip OR orphan gap)
                → ShouldSendBranchSyncRequest(hashDeepestAncestor)?
                     no  → FETCH_THROTTLED
                     yes → LIST + TRANSACTIONS + LOCATOR → peer_best
                           (+ optional fanout peer)
                           → FETCH_QUEUED | SKIPPED
```

### A1b — Missing-tx escalation no longer double-LIST — PR #691

**Was:** Escalation caller always sent its own locator `LIST` after recovery, replacing `TxResponseWindow` and defeating the 3s throttle.

**Now:** `RequestMissingTxBranchRecovery()` orchestrates on `PeerBestRecoveryResult`:

| Result | Fallback LIST? |
|--------|----------------|
| `PROGRESS` | No |
| `FETCH_QUEUED` | No |
| `FETCH_THROTTLED` | No |
| `SKIPPED` | Yes (also throttle-gated) |

**Evidence:** `process.cpp` `RequestMissingTxBranchRecovery` ~839–930; `tritium.cpp` missing-tx escalation ~3113–3125.

### Harness — `[ledger]` no longer segfaults from skipped bootstrap — PR #690

`CATCH_CONFIG_RUNNER` + `EnsureUnitTestEnvironment()` outside Catch tag filters (`tests/unit/main.cpp`).  
**Still open:** assertion hygiene failures inside individual ledger cases (TIP-05).

### Dead code removed — PR #690

`ValidationThreadPool` + standalone `auto_cooldown_test.cpp` removed from tree/makefile.

---

## 2. TIP list — choose next target

Pick **one TIP per PR**. Prefer S/M before L. Severity: P0 wedge/data-loss · P1 liveness · P2 correctness/DoS · P3 hygiene · P4 cleanup.

### P1 — Liveness / recovery

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-01** | Throttle BESTCHAIN branch LIST | S | **FIXED** — `RequestBestChainBranchRecovery` gates fallback LIST with `ShouldSendBranchSyncRequest` / result enum (same contract as #691). | `process.cpp`, `tritium.cpp` BESTCHAIN |
| **TIP-02** | Route BESTCHAIN unknown-tip through coordinator | S–M | **FIXED** — BESTCHAIN always calls coordinator via `RequestBestChainBranchRecovery` (fanout + shared enum). | `process.cpp`, `tritium.cpp` |
| **TIP-03** | Production soak checklist for A1 | S (ops) | Code-fixed; wedge family was multi-hour. Need log signatures + testnet script before declaring closed in the field. | ops runbook (this audit §5) |
| **TIP-04** | `ForceLocalChainResync` manual RPC (spec §5) | L | After UNKNOWN budget (~40 min) expires on a true local-mined dead fork, node still cannot adopt peer chain without restart/`-revertblocks`. Spec already written. | new `fork_recovery.cpp`, `checkforkrecovery` lineage |

### P1/P2 — Mempool / producer liveness

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-06** | UNKNOWN-budget observability + operator action | S | When `MAX_UNKNOWN_ANCESTOR_RETRIES` (80 × 30s) expires, force-evict is correct for ABSOLUTE-shaped cases but opaque for true gaps still downloading. Surface `checkforkrecovery`-style depth + “still fetching?” in logs/API. | `mempool.cpp`, RPC |
| **TIP-07** | Conflict-DAG resolve → mining template flush coupling | M | Option C stops cascade doom-loop, but producer/miner stale-work recovery still depends on BESTCHAIN/template push timing. Explicit “root resolved → flush templates” bridge reduces restart-pair recovery. | `mempool.cpp`, miner push path |
| **TIP-08** | Consolidate fork-admissibility decision point | L | Classifier + recovery + missing-tx escalation still decide independently. Spec §4 in knowledge base. Only after TIP-03 soak. | `process.cpp`, `mempool.cpp` |
| **TIP-19** | Conflict retry maps: prefer LRU/partial eviction over wholesale clear | M | When `mapConflictRetries` / `mapUnknownAncestorRetries` / `BoundConflictDAG()` hit `MAX_CONFLICTS_MAP_ENTRIES` (10000), **all** retry counters clear. A genesis at 15/20 or 70/80 resets to 0 and can retain forever under continuous DAG churn — extends beyond documented ~10/~40 min budgets. | `mempool.cpp` `BoundConflictDAG` ~80–107; retry bumps ~1311–1387 |

### P2 — Protocol / DoS / sync foot-guns

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-09** | Guard `Check(true)` / `fForceProof` regressions | S | Inline comments already document multi-day sync regression if primary `Process()` uses `Check(true)`. Add a unit/source guard test (partially present in missing_tx tests) + CI comment linter optional. | `process.cpp` ~1311–1325, `tritium.cpp` CheckInternal |
| **TIP-10** | SPECIFIER::SYNC inventory freeze | S | Post-sync LIST must stay on `TRANSACTIONS`. Add a single “recovery LIST specifier matrix” unit test covering BESTCHAIN / A1 / missing-tx / Sync(). | `tritium.cpp`, tests |
| **TIP-11** | DDoS operator foot-gun docs + safe profile | S | Defaults restored (`-ddos=false`, `-rscore=2000`), but enabling `-ddos` with low rscore still self-bans sync peers (`GET BLOCK` +50 rSCORE). Publish recommended profiles. | `global.cpp`, docs |

### P2/P3 — Mining session coordinator

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-13** | R-01 `SessionBinding` value object | M | Roadmap Priority-1; identity compares still ad-hoc in places. | session registry / miner |
| **TIP-14** | R-02 call-site audit refresh | S | Code now gates SUBMIT_BLOCK + MINER_SET_REWARD with `ValidateConsistency()` on stateless path; roadmap still says “missing”. Re-verify legacy lane + update roadmap status. | `stateless_miner*.cpp`, `miner.cpp`, roadmap |
| **TIP-15** | R-03 / R-06 state machine + scoped update guard | M–L | Boolean flag soup still admits illegal transitions under reconnect races. Diagrams already exist under `docs/diagrams/upgrade-path/`. | session container |
| **TIP-16** | Stale SUBMIT_BLOCK anti-doom-loop soak | S–M | Option A hardening exists; needs multi-miner collision harness (R-08) before upstream. | miner submit path, tests |

### P3 — Test / hygiene / cleanup

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-05** | Ledger unit assertion hygiene pass | M | **PARTIAL** — disk genesis Fix-1 arm in `CreateTransaction` + create_transaction sim; remaining 3 FAIL: `validate_vtx` MEMPOOL sim, 2× `mempool_check_orphan_fix` Primitive exception. | `create.cpp`, `tests/unit/TAO/Ledger/*` |
| **TIP-17** | Remove or `[[deprecated]]` throwing `AddressManager::Get()` | S | **FIXED** — single-arg `Get()` marked `[[deprecated]]`; live paths use non-throwing overload. | `manager.{h,cpp}` |
| **TIP-18** | PR #667 residual surface sample | M | ~553 clean-merge files never fully audited. Sample next tranche: permissions already done; next = time server / lookup / API DDoS / sync finalization edges. | LLP global surface |

### P1 — Upstream isolation (meta)

| ID | Title | Size | Why now | Primary files |
|----|-------|------|---------|---------------|
| **TIP-12** | Upstream PR isolation checklist (draft issue) | S (process) | Colin will want a diffed, isolatable patch against `testnet` or `merging`, not a NODE monopr. See [UPSTREAM_PR_CHECKLIST.md](UPSTREAM_PR_CHECKLIST.md). | process + git |

---

## 3. Recommended chooser order (default queue)

If you want a default sequence without reading every row:

1. **TIP-03** — soak A1 on testnet (no code, validates #690/#691)  
2. **TIP-01** — throttle BESTCHAIN LIST (small, closes residual double-path)  
3. **TIP-02** — unify BESTCHAIN with coordinator (completes A1 symmetry)  
4. **TIP-05** — ledger test hygiene (protects future PRs)  
5. **TIP-14** — refresh R-02 roadmap vs code  
6. **TIP-06** — UNKNOWN budget operator visibility  
7. **TIP-19** — retry-map cap eviction fairness (if soak shows budget extension)  
8. **TIP-12** — cut upstream isolation package when ready to talk to Colin  
9. **TIP-04** — only after soak proves remaining wedge is true local-dead-fork 

```
   soak (03) ──► BESTCHAIN throttle (01) ──► coordinator unify (02)
                      │
                      ▼
              ledger hygiene (05) ──► upstream package (12)
                      │
                      ▼
              UNKNOWN ops (06) ──► ForceLocalChainResync (04) [gate]
```

---

## 4. Closed vs open map (A-series continuity)

| Legacy ID | Title | Status |
|-----------|-------|--------|
| A1 silent far-tip no-op | Peer tip not on disk / not orphan → no fetch | **Closed #690** |
| A1b double LIST | Escalation always LIST after recovery | **Closed #691** |
| TIP-01/02 BESTCHAIN coordinator | Chatty BESTCHAIN unthrottled / dual path | **Closed #694** |
| Near-tip inventory race skip | Equal/+1 spam LIST during tip advance | **Closed** (inventory GET gate) |
| Near-tip orphan-pool hole | Orphan tip treated as inventory race → stall | **Closed FG-20 / KF-6** |
| A2 missing-tx permanent wedge | `mapLastMissing` left at 51+ | **Closed earlier** (erase + escalate) |
| A3 UNKNOWN vs INVALID_ABSOLUTE | Sync gap mis-evict | **Closed #664** |
| A4 TLS / SSL admissibility | PORT_SSL disabled / redesign | **Still deferred** (global.cpp TODO) |
| E.3 DDoS defaults | `-ddos`/`-rscore` regression from #667 | **Closed #680 lineage** |
| Finding #2 AddressManager TOCTOU | Has+Get race | **Closed #678** |
| Option C conflict DAG | Cascade liveness doom-loop | **Closed #688/#689** |
| ForceLocalChainResync | Evict local dead fork | **Open (TIP-04)** |

---

## 5. Field soak signatures (TIP-03)

After deploying NODE post-#691, grep logs for:

**Healthy A1 far-tip path**
```
=== PEER_BEST_RECOVERY === ... not_on_disk=true in_orphan_pool=no action=locator-branch-sync-request
```
(must **not** end the story at `action=block-not-yet-received` with no subsequent LIST)

**Healthy missing-tx single-LIST**
```
missing-tx retry limit reached ... escalating to branch recovery
```
followed by **one** LIST window open, not two rapid window replacements on the same peer.

**Mempool still stranded (not necessarily a bug)**
```
STRANDED_STATE_DETECTED
mempool_conflicts=N mempool_conflict_deps=M
```
while BESTCHAIN height still moves → roots present; correlate with miner stale work before paging.

**Bad old wedge (should be gone)**
```
action=block-not-yet-received
```
as a terminal state while peer_height − local_height stays large for hours **and** no locator LIST is observed.

---

## 6. Related living docs

| Doc | Role |
|-----|------|
| [RECCES.md](RECCES.md) | File:line reconnaissance notes (updated) |
| [FOOT_GUNS.md](FOOT_GUNS.md) | Trial-and-error traps (inline-comment backed) |
| [UPSTREAM_PR_CHECKLIST.md](UPSTREAM_PR_CHECKLIST.md) | Draft checklist for Colin / testnet|merging |
| [FORK_RECOVERY_KNOWLEDGE_BASE.md](../../../archive/FORK_RECOVERY_KNOWLEDGE_BASE.md) | Historical wedge archive (retained) |
| [MEMPOOL_CONFLICT_DAG.md](../../../architecture/MEMPOOL_CONFLICT_DAG.md) | Option C design |
| [MISSING_TX_FORK_WEDGE_BUG.md](../../../archive/MISSING_TX_FORK_WEDGE_BUG.md) | Missing-tx wedge archive |
| [CONNECTION_ACCEPTANCE_AND_DDOS_DEFENSE.md](../../../architecture/CONNECTION_ACCEPTANCE_AND_DDOS_DEFENSE.md) | DDoS/permissions |
| Diagrams: [recovery-coordinator-upgrade.md](../../../diagrams/audit/recovery-coordinator-upgrade.md) | Before/after coordinator |
| Diagrams: [mempool-recovery-coupling.md](../../../diagrams/audit/mempool-recovery-coupling.md) | Mempool ↔ recovery |

---

## 7. Explicit non-goals this audit

- Deleting archive documents (historical record stays).
- Implementing TIP-04 automatic rollback in this pass.
- Full line-by-line re-audit of all ~553 PR #667 clean-merge files.
- Changing DDoS defaults again.
- Touching `Check(true)` primary path.
