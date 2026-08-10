# Foot Guns — NODE Trial-and-Error Catalog

Things that **look correct** but break under specific conditions.  
Most were discovered the hard way and are now guarded by inline comments and/or archive docs.  
**Do not “simplify” these without re-reading the cited comment blocks.**

Companion: [NODE_AUDIT_2026-08-10.md](NODE_AUDIT_2026-08-10.md) · [RECCES.md](RECCES.md)

---

## FG-01 — `SPECIFIER::SYNC` after sync completes

**Symptom:** `unsolicited sync block` drops, `DISCONNECT::FORCE`, fork recovery never converges.  
**Trap:** Copy-paste Sync() LIST into BESTCHAIN / missing-tx / peer-best recovery.  
**Rule:** Post-sync recovery LIST uses `SPECIFIER::TRANSACTIONS` (or `CLIENT` in client mode).  
**Where documented in code:** `process.cpp` A1 LIST block; `tritium.cpp` BESTCHAIN and missing-tx comments.  
**Archive:** PR #662 lineage; FORK / MISSING_TX docs.

```
Sync() / LASTINDEX (syncing)  →  SPECIFIER::SYNC     OK
BESTCHAIN / A1 / missing-tx   →  SPECIFIER::TRANSACTIONS   REQUIRED
```

---

## FG-02 — Force `Check(true)` / `fForceProof` on bulk IBD

**Symptom:** Sync goes from hours → **days** (full PrimeCheck/VerifyWork every block).  
**Trap:** “Make validation stricter during sync” on the primary `Process()` path.  
**Rule:** Primary + orphan-drain use `Check()` / `fForceProof=false`. Targeted recovery after repeated rejects may use `Check(true)`.  
**Code:** `process.cpp` ~1311–1325, ~1654+; `tritium.cpp` `CheckInternal` gate `fForceProof || !Synchronizing()`.  
**Arch:** `docs/architecture/BLOCK_PRODUCTION_FLOW.md` regression notes; PR #674.

---

## FG-03 — Leave connectable orphan in `mapOrphans` then `Process()` it

**Symptom:** Recovery “feeds” ancestor but nothing happens.  
**Trap:** Clone orphan and Process without `mapOrphans.Remove(hash)`.  
**Rule:** `Process()` treats presence in `mapOrphans` as known ORPHAN and returns before Check/Accept.  
**Code:** `AttemptPeerBestChainRecovery` walkback comment ~608–616.

---

## FG-04 — Throttle key = peer tip instead of missing ancestor

**Symptom:** Orphan drain `erase(hashParent)` never clears throttle entries; requests stick or double-fire under two namespaces.  
**Trap:** Key `ShouldSendBranchSyncRequest` on `hashPeerBest` after an orphan walk.  
**Rule:** Canonical key is **missing ancestor** (`hashPrevBlock` / `hashDeepestAncestor`). Default to peer tip only when no walk occurred.  
**Code:** `process.cpp` throttle comments ~688–694; `process.h` ShouldSendBranchSyncRequest docs.

---

## FG-05 — Treat throttle denial as “fallback LIST is fine”

**Symptom:** Every escalation bypasses the 3s throttle with a second LIST; `TxResponseWindow` replaced.  
**Trap:** `if (!fBranchSyncQueued) fallback_LIST()` without distinguishing `FETCH_THROTTLED`.  
**Rule:** Orchestrate on `PeerBestRecoveryResult`; THROTTLED and QUEUED both suppress fallback.  
**Code:** `RequestMissingTxBranchRecovery` switch ~853–878; fixed in #691.

---

## FG-06 — `mapLastMissing` left above retry limit

**Symptom:** Permanent incomplete-block silence; local miner extends dead fork; only restart helps.  
**Trap:** Clear `vMissing` / `hashMissing` but leave counter at 51+.  
**Rule:** **Erase** the map entry on limit; escalate via branch recovery.  
**Archive:** `docs/archive/MISSING_TX_FORK_WEDGE_BUG.md`.

---

## FG-07 — Rate-limit early return with `hashMissing == 0`

**Symptom:** LLP interprets soft throttle as retry-limit escalation (wrong branch).  
**Trap:** Early-return INCOMPLETE without setting `hashMissing = hashBlock`.  
**Rule:** Rate-limit path must set `hashMissing` so LLP takes cheap per-tx re-request.  
**Code:** `process.cpp` rate-limit block; PR #666.

---

## FG-08 — Seed `mapLastMissing` without seeding process-time

**Symptom:** Second rapid arrival after first miss is not throttled.  
**Trap:** Only check `mapLastMissingProcessTime` when entry already exists, but never write it on first miss.  
**Rule:** Seed process-time when counter first set to 1 (primary + orphan-drain).

---

## FG-09 — Binary fork classifier (no UNKNOWN)

**Symptom:** Correct-chain mempool data permanently evicted during large sync gap; node stuck on own mined tip.  
**Trap:** `!fAncestorFound` ⇒ INVALID_ABSOLUTE.  
**Rule:** Three states — DEFERRED_LOCAL_STATE / UNKNOWN / INVALID_ABSOLUTE.  
**Archive:** FORK_RECOVERY_KNOWLEDGE_BASE §3; PR #664.

---

## FG-10 — Cascade conflict descendants into `mapConflicts`

**Symptom:** ERROR storm, NOTIFY amplification, producer/miner liveness doom-loop; tip may still tick.  
**Trap:** Treat every CONFLICTED-prev Accept as a new root.  
**Rule:** Option C — roots only; park dependents; no ERROR/NOTIFY on park.  
**Doc:** `docs/architecture/MEMPOOL_CONFLICT_DAG.md`; PR #688/#689.

---

## FG-11 — Enable `-ddos` with low `-rscore` during sync

**Symptom:** Self-ban of legitimate high-volume peers (`GET BLOCK` costs rSCORE).  
**Trap:** “Turn on DDoS protection” without raising rscore / understanding moving average.  
**Defaults (restored):** `-ddos=false`, `-rscore=2000` on TRITIUM.  
**Doc:** `CONNECTION_ACCEPTANCE_AND_DDOS_DEFENSE.md`; PR #667 silent merge regression + #680 fix.

---

## FG-12 — `Has()` then throwing `Get()` on AddressManager

**Symptom:** Rare `std::out_of_range` → connection drop via generic exception handler.  
**Trap:** TOCTOU between Has and `at()`.  
**Rule:** Use `bool Get(addr, TrustAddress&)`. Do not revive throwing single-arg Get at call sites.  
**Code:** `manager.h/cpp`; PR #678.

---

## FG-13 — Unit bootstrap only inside `[args]` TEST_CASE

**Symptom:** `./nexus "[ledger]"` segfaults (LLD/ChainState null).  
**Trap:** Catch tag filters skip setup case.  
**Rule:** `CATCH_CONFIG_RUNNER` + `EnsureUnitTestEnvironment()` in `main` before Catch::Session::run.  
**Code:** `tests/unit/main.cpp`; fixed #690.

---

## FG-14 — Double `LLP::Initialize()` / `API::Initialize()` in tests

**Symptom:** `std::terminate` from assigning over joinable `std::thread` members.  
**Trap:** Call API init again “to be safe” after LLP::Initialize (which already inits API).  
**Rule:** One Initialize; shutdown on suite exit.

---

## FG-15 — Far-tip recovery that only logs

**Symptom:** Hours stuck while mempool UNKNOWN budget burns; miner extends local tip.  
**Trap:** `not_on_disk && !orphan → return` without LIST.  
**Rule:** Active fetch path (A1) — locator LIST + TRANSACTIONS.  
**Fixed:** #690. BESTCHAIN asymmetry closed via RequestBestChainBranchRecovery (TIP-01/02).

---

## FG-16 — `AttemptForkRecovery` nostalgia

**Symptom:** Design thrash restoring unsafe auto-rollback.  
**Fact:** Feature added, removed, reverted, then permanently replaced by classifier. It **never** rolled back to off-chain/not-found ancestors. True “adopt peer chain / disconnect local blocks” is **new** work (`ForceLocalChainResync` spec), not a restore.  
**Archive:** FORK_RECOVERY_KNOWLEDGE_BASE §1 and §5.

---

## FG-17 — Mining session flag soup without ValidateConsistency

**Symptom:** SUBMIT_BLOCK / reward bind on partially recovered session; DEGRADED loops.  
**Trap:** Check only `fAuthenticated` bool.  
**Rule:** `ValidateConsistency()` at security boundaries; prefer roadmap R-01/R-03 long-term.  
**Diagrams:** `docs/diagrams/upgrade-path/02`, `03`, `06`.

---

## FG-19 — Wholesale clear of conflict retry maps at cap

**Symptom:** Documented DEFERRED (~10 min) / UNKNOWN (~40 min) budgets never expire under continuous conflict churn.  
**Trap:** `BoundConflictDAG()` and per-map `size() >= MAX` paths `clear()` **all** retry counters when any map hits 10000.  
**Rule:** Caps are intentional DoS guards, but fairness matters — prefer evicting cold genesis entries or partial trim before zeroing hot counters mid-budget.  
**Code:** `mempool.cpp` `BoundConflictDAG` ~80–107; `mapConflictRetries`/`mapUnknownAncestorRetries` bumps ~1311–1387.  
**TIP:** TIP-19.

---

## FG-18 — Silent archive deletion during “cleanup” PRs

**Symptom:** Future agents re-derive multi-week root causes from scratch.  
**Rule:** Archive is historical on purpose. Prefer status strikethroughs over deletion. Upstream may drop files in their merge; NODE keeps them.

---

## Quick “before you touch X” index

| If you touch… | Read first |
|---------------|------------|
| `AttemptPeerBestChainRecovery` | FG-01,03,04,05,15 + RECCES R1 |
| `Process()` missing-tx | FG-06,07,08 + MISSING_TX archive |
| `Mempool::Check` / Accept conflicts | FG-09,10 + MEMPOOL_CONFLICT_DAG |
| `block.Check` / VerifyWork | FG-02 |
| DDoS defaults / permissions | FG-11 + CONNECTION_ACCEPTANCE doc |
| AddressManager | FG-12 |
| Unit main / ledger tests | FG-13,14 |
| Fork rollback ideas | FG-16 + knowledge base §5 |
| Miner session / submit | FG-17 + upgrade-path diagrams |
