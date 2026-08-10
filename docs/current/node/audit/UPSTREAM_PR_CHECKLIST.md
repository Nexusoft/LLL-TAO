# Draft Checklist — Upstream PR (testnet / merging isolation)

**Status:** Rough draft for a future GitHub Issue / PR against upstream Colin  
**NODE context:** Post #690/#691 recovery + mempool series  
**Audience:** NamecoinGithub NODE maintainers preparing an isolatable patch for `Nexusoft/LLL-TAO` (`testnet` or `merging`)  
**Related TIP:** TIP-12 in [NODE_AUDIT_2026-08-10.md](NODE_AUDIT_2026-08-10.md)

---

## Why this exists

Upstream will almost certainly **not** want a NODE monopr. Colin isolates changes first. A workable path is:

1. Diff NODE → extract a **minimal vertical slice**  
2. Open against upstream **`testnet`** (preferred) or a dedicated branch off **`merging`**  
3. Keep NODE archive/docs out of the upstream diff unless Colin asks  
4. Prove behavior with unit tests that do not require full NODE mining stack  

```
   NODE (NamecoinGithub)                    Upstream (Nexusoft)
   ─────────────────────                    ───────────────────
   many coupled PRs (#662…#691)             wants isolatable commits
           │                                         ▲
           │  git diff / cherry-pick series          │
           ▼                                         │
   "slice branch" ── rebase onto testnet/merging ────┘
           │
           ▼
   checklist below (this doc)
```

---

## 0. Pre-flight (do before cutting the branch)

- [ ] Freeze NODE tip SHA and tag it (`node-upstream-slice-YYYYMMDD`)
- [ ] List commits in the slice with one-line intent (see §1 suggested slices)
- [ ] Confirm each commit builds alone on NODE (`make -f makefile.cli -j$(nproc)`)
- [ ] Confirm unit tests for the slice: `make -f makefile.cli UNIT_TESTS=1 -j$(nproc)` + targeted tags
- [ ] Identify files that are NODE-only (stateless miner extras, archive docs, RISC-V experiments) — **exclude by default**
- [ ] Read FG-01…FG-16 in [FOOT_GUNS.md](FOOT_GUNS.md) so the slice does not “clean up” hard-won guards
- [ ] Decide target: **`testnet`** (safer) vs **`merging`** (closer to release train)

---

## 1. Suggested isolatable slices (pick one per upstream PR)

### Slice A — Post-sync SPECIFIER discipline (smallest political surface)

**Theme:** Never send `SPECIFIER::SYNC` on post-sync recovery LIST.  
**Likely ancestry:** #662 family  
**Core files (typical):**
- `src/LLP/tritium.cpp` (BESTCHAIN / missing-tx LIST specifiers)
- `src/TAO/Ledger/process.cpp` (recovery LIST)
- tests asserting specifier constants  

**Upstream pitch:** “Fixes unsolicited sync block disconnects during fork recovery on already-synced nodes.”

**Out of scope for Slice A:** mempool classifier, Option C DAG, mining session code.

---

### Slice B — Missing-tx wedge + escalation (medium)

**Theme:** Erase `mapLastMissing` on limit; escalate; rate-limit hashMissing semantics.  
**Likely ancestry:** MISSING_TX doc + #666 + escalation work  
**Core files:**
- `src/TAO/Ledger/process.cpp` / `include/process.h`
- `src/LLP/tritium.cpp` INCOMPLETE handler
- `src/LLP/include/tx_response_window.h` (+ open/rollback call sites) if windows are required for safe LIST
- `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` (trim NODE-only asserts)

**Upstream pitch:** “Prevents permanent incomplete-block silence that wedges a node on its own fork.”

**Depends on:** Slice A specifier rules if recovery LIST is included.

---

### Slice C — Peer-best far-tip fetch + single-LIST coordinator (this audit’s head)

**Theme:** A1 active fetch + A1b `PeerBestRecoveryResult` / `RequestMissingTxBranchRecovery`.  
**Ancestry:** #690 + #691  
**Core files:**
- `src/TAO/Ledger/include/process.h` (`PeerBestRecoveryResult`, declarations)
- `src/TAO/Ledger/process.cpp` (`AttemptPeerBestChainRecovery`, coordinator)
- `src/LLP/tritium.cpp` (call site only — missing-tx escalation)
- A1 unit tests (socketpair path) — may need upstream Catch harness adaptation
- **Optional follow-up commit:** BESTCHAIN throttle (TIP-01/02) once soaked on NODE

**Upstream pitch:** “Peer tips hundreds of blocks ahead no longer log-and-return; throttled locator LIST starts recovery without duplicate windows.”

**Depends on:** Slice A (specifier); ideally Slice B (escalation caller).

**Diagram for PR body:** [recovery-coordinator-upgrade.md](../../../diagrams/audit/recovery-coordinator-upgrade.md)

---

### Slice D — Mempool three-state classifier (no Option C)

**Theme:** UNKNOWN vs INVALID_ABSOLUTE only (#664).  
**Core files:**
- `src/TAO/Ledger/mempool.cpp` / `types/mempool.h` (classifier + UNKNOWN budget)
- classifier unit tests if portable

**Upstream pitch:** “Stop permanently evicting correct-chain txs during sync gaps.”

**Out of scope:** Option C dependent parking (Slice E).

---

### Slice E — Option C conflict DAG (largest mempool surface)

**Theme:** Root-only conflicts + dependent parking + BoundConflictDAG + #689 retry clear.  
**Ancestry:** #688 / #689  
**Core files:**
- `mempool.cpp` / `mempool.h`
- `tests/unit/TAO/Ledger/mempool_conflict_dag.cpp`
- living doc optional: `docs/architecture/MEMPOOL_CONFLICT_DAG.md` (Colin may drop)

**Upstream pitch:** “Stops mempool cascade liveness doom-loop (ERROR/NOTIFY storm) while tip still advances.”

**Depends on:** Slice D classifier semantics.  
**Diagram:** [mempool-recovery-coupling.md](../../../diagrams/audit/mempool-recovery-coupling.md)

---

### Slice F — DDoS defaults + permissions (ops safety)

**Theme:** Restore TRITIUM `-ddos=false` / `-rscore=2000`; testnet permissions bypass fix.  
**Ancestry:** #680 / #681 family  
**Core files:** `src/LLP/global.cpp`, `permissions.cpp`, docs optional  
**Caution:** Upstream may have intentionally changed defaults — **diff carefully** and justify with sync self-ban evidence, do not force NODE prefs silently.

---

### Slice G — Do **not** send upstream yet

| Item | Why hold |
|------|----------|
| `ForceLocalChainResync` | Spec only; high risk; needs NODE soak + manual RPC first (TIP-04) |
| Full stateless mining coordinator (R-01…R-15) | Large NODE-specific surface; separate product track |
| Archive historical markdown dumps | NODE learning archive; Colin can delete — default exclude |
| Ledger harness `EnsureUnitTestEnvironment` | Useful, but couple only if upstream tests need it; may be its own tiny PR |
| TIP-19 retry LRU | Nice-to-have after soak proves budget extension |

---

## 2. Diff hygiene rules (hard)

1. **One concern per upstream PR** — do not bundle mining + mempool + DDoS.  
2. **No drive-by refactors** in touched files.  
3. **Preserve foot-gun comments** in the slice (FG catalog). Upstream reviewers need them.  
4. **Tests travel with behavior** — if harness cannot run upstream, include a minimal Catch case or document manual testnet steps.  
5. **Docs:** prefer a short PR-body diagram over shipping the entire `docs/archive/`.  
6. **Config defaults:** call out any default change in its own commit with before/after table.  
7. **No secrets, no mainnet checkpoints edits** unless explicitly required and reviewed.  
8. **Rebase, don’t merge NODE into upstream branch** — keeps history isolatable.

---

## 3. Mechanical recipe (diffed merge / cherry-pick)

```bash
# 0) Record NODE tip
git fetch origin NODE
NODE_SHA=$(git rev-parse origin/NODE)

# 1) Fetch upstream (adjust remote URL)
git remote add nexusoft https://github.com/Nexusoft/LLL-TAO.git 2>/dev/null || true
git fetch nexusoft testnet merging

# 2) Cut isolation branch from upstream target
git checkout -b upstream/slice-C-peer-best-recovery nexusoft/testnet

# 3) Cherry-pick only the slice commits (oldest→newest), resolve conflicts narrowly
# git cherry-pick <sha1> <sha2> ...

# 4) Or path-limited diff apply (when history is too entangled):
# git diff <upstream-base> $NODE_SHA -- src/TAO/Ledger/process.cpp src/TAO/Ledger/include/process.h \
#   src/LLP/tritium.cpp tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp > /tmp/slice-C.patch
# git apply --3way /tmp/slice-C.patch

# 5) Build + tests on upstream tree
make -f makefile.cli clean
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
./nexus "[ledger][a1]"   # or whatever tags survive the port

# 6) Open PR against nexusoft testnet (not NODE)
```

**Conflict expectation:** `process.cpp`, `tritium.cpp`, `mempool.cpp` are hot files since PR #667-style merges — budget real review time.

---

## 4. Testnet validation checklist (runtime)

Run a private or public testnet node built from the slice branch:

- [ ] Fresh sync from height 0 completes (PrimeCheck fast-path intact — FG-02)
- [ ] After sync, force a BESTCHAIN divergence (peer ahead): observe **one** locator LIST with TRANSACTIONS, not SYNC
- [ ] Far tip not on disk: log contains `action=locator-branch-sync-request` (not terminal `block-not-yet-received`)
- [ ] Missing-tx exhaustion: single LIST window; no rapid double OpenTxResponseWindow on same peer
- [ ] With `-ddos=0` default: long sync peer not banned; if testing `-ddos=1`, use high `-rscore`
- [ ] Mempool slice only: induce CONFLICTED-prev cascade; confirm dependents park without ERROR storm
- [ ] Miner attached (if slice includes mining): no DEGRADED doom-loop on stale submit
- [ ] `checkforkrecovery <genesis>` still read-only / safe

Log greps: see soak signatures in NODE audit §5.

---

## 5. PR body template (for Colin)

```markdown
## Summary
<one paragraph: symptom → root cause → minimal fix>

## Isolation
- Base: nexusoft/<testnet|merging> @ <sha>
- Slice: <A|B|C|D|E|F>
- NODE provenance: NamecoinGithub/LLL-TAO NODE @ <sha> (PRs #…)
- Explicitly excluded: <archive docs, mining extras, …>

## Behavior change
- Before: …
- After: …

## Specifier / foot-gun notes
- Post-sync LIST uses TRANSACTIONS (not SYNC)
- <other FG-xx that apply>

## Diagram
<paste from docs/diagrams/audit/… or ASCII>

## Tests
- [ ] unit: …
- [ ] testnet manual: …

## Risk
- Rollback plan: revert this PR
- Feature flag: <-peerbestchainrecovery / none>
```

---

## 6. Reviewer focus questions (attach to issue)

1. Does any path still emit `SPECIFIER::SYNC` after `fSynchronized==true`?  
2. Can `AttemptPeerBestChainRecovery` re-enter itself holding `PROCESSING_MUTEX`?  
3. Is throttle key always the missing ancestor?  
4. Does fallback LIST fire on `FETCH_THROTTLED`? (must be no)  
5. Are mempool retry budgets still meaningful under map-cap clear? (TIP-19)  
6. Any default config change? Justified?  
7. Does the slice compile without NODE-only headers?

---

## 7. Draft GitHub Issue title/body (copy)

**Title:** `[upstream] Isolation checklist: peer-best recovery + mempool slices for testnet/merging`

**Body:**  
Point to this file path in NODE:  
`docs/current/node/audit/UPSTREAM_PR_CHECKLIST.md`  

Include scoreboard from NODE audit §0 and ask Colin which slice (A–F) to lead with.

---

## 8. Exit criteria for “ready to open upstream”

- [ ] Slice builds on clean upstream base  
- [ ] Targeted unit tests green on that base  
- [ ] Testnet soak §4 checked for that slice  
- [ ] PR body uses template §5  
- [ ] NODE archive not required to understand the patch (comments in code suffice)  
- [ ] Owner assigned for conflict resolution with concurrent upstream commits  
