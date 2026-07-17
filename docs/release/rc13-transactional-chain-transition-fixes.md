# RC13 Deep Dive: The Transactional Chain-Transition Bug Chain

**Release:** [`nexus-node-v5.1.0-localhost-prime-cpu-rc13-linux-x86_64`](https://github.com/NamecoinGithub/LLL-TAO/releases/tag/nexus-node-v5.1.0-localhost-prime-cpu-rc13-linux-x86_64)
**Marked by the maintainer as:** REQUIRED UPDATE
**Related issue:** [Nexusoft/LLL-TAO#254](https://github.com/Nexusoft/LLL-TAO/issues/254) — *Part 1: Local Stake/Local Mined NODE Autocorrect for Accepted but Losing Network Consensus Blocks (PR #640) and Part 2: Stake Block Burst Cooldown Proposal*
**Pull requests fixed in this RC:** [#651](https://github.com/NamecoinGithub/LLL-TAO/pull/651) · [#652](https://github.com/NamecoinGithub/LLL-TAO/pull/652) · [#653](https://github.com/NamecoinGithub/LLL-TAO/pull/653) · [#654](https://github.com/NamecoinGithub/LLL-TAO/pull/654)

---

## Why this document exists

RC13 fixes a **chain of four interlocking bugs** in `BlockState::SetBest()` — the single
function responsible for moving the node's canonical chain tip from one block to the
next. This is the most safety-critical code path in the ledger: a bug here can silently
diverge disk state from mempool state, or make the node log false errors on *every*
accepted block.

Each of the four PRs fixed a **real** bug, but each fix also **exposed a new bug** in the
layer beneath it — a textbook example of how patching a symptom can reveal the next
layer of the underlying defect, and why regression tests must exercise real code paths,
not simulations. This document walks through the bug chain in the order it was
discovered and fixed, so the failure modes and the reasoning behind each fix are clear.

A companion diagram file with Mermaid sequence/state diagrams is at
[`docs/diagrams/architecture/setbest-transaction-boundary.md`](../diagrams/architecture/setbest-transaction-boundary.md).

---

## Background: what `SetBest()` does and why atomicity matters

`BlockState::SetBest()` (in `src/TAO/Ledger/state.cpp`) is called whenever a new block
might become the best (canonical) chain tip — from `TritiumBlock::Accept()`,
`LegacyBlock::Accept()`, `ActivateCandidateBestChain()`, and the checkpoint/rewind paths
in `chainstate.cpp`. It performs three categories of mutation:

1. **Disk phase** — disconnects blocks from the old tip and connects blocks up to the
   new tip (register/contract/ledger/trust/legacy database writes).
2. **Mempool phase** — resurrects transactions from disconnected blocks back into the
   mempool, and removes transactions from the mempool that are now confirmed on the new
   tip.
3. **In-memory `ChainState` phase** — publishes the new best-chain hash/height atomics
   that every other subsystem (RPC, mining templates, LLP broadcast) reads.

These three phases **must** be atomic relative to a disk-write failure: if the disk
phase fails partway through, phases 2 and 3 must never run, and any partial disk writes
must be rolled back via `LLD::TxnAbort()`. If they aren't atomic, the node can end up
with mempool or in-memory state that disagrees with what's actually on disk — the root
cause of the historical "Doom Loop" defect referenced in PR #651.

```
Doom Loop shape (generic pattern, not specific to this RC):
  1. Disk write partially fails/aborts
  2. In-memory or mempool state already mutated as if it succeeded
  3. Node believes it's at tip X, disk says tip Y
  4. Next block validation references the wrong "last" state
  5. Node produces/accepts an invalid transition, or gets stuck re-processing
  6. Restart doesn't fix it because disk state itself is inconsistent
```

---

## Bug 1 (PR #651): Mempool/`ChainState` mutations happened *before* the disk commit

### The defect

Every caller of `SetBest()` wraps the call in the standard transaction pattern:

```cpp
LLD::TxnBegin(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
if(!state.SetBest())
    LLD::TxnAbort(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
else
    LLD::TxnCommit(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
```

This looks safe — but `SetBest()` itself, **before returning**, was already:

- Calling `mempool.Accept()` to resurrect transactions from disconnected blocks,
  interleaved *between* the disconnect and connect disk-write loops (i.e. mid disk
  phase, long before any `TxnCommit()`),
- Calling `mempool.Remove()` for newly-confirmed transactions immediately after the
  connect loop, still before `TxnCommit()`,
- Publishing the new `ChainState` atomics right after that.

So if the *outer* `TxnCommit()` (or an internal disk write) later failed, the mempool and
`ChainState` had **already been mutated** as if the transition succeeded. There was no
way to roll that back — the classic Doom-Loop shape.

There was also a standalone bug at **call site #4** in `chainstate.cpp` (the
hardcoded-checkpoint revert path): it called `TxnCommit()` **unconditionally**, without
even checking whether `SetBest()` had returned `true` or `false`:

```cpp
// Before (bug) — TxnCommit always reached, even on failure
LLD::TxnBegin();
stateAncestor.SetBest();   // return value ignored!
LLD::TxnCommit();          // commits partial/failed writes
```

### The fix

`state.cpp`'s `SetBest()` was restructured into three strict, ordered phases:

1. **Disk phase** — disconnect/connect loop only. Returns `false` immediately on any
   failure (nothing else has been touched yet).
2. **Internal commit** — `LLD::TxnCommit(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS)` runs
   *inside* `SetBest()`, only after every disk write in phase 1 has succeeded.
3. **Post-commit phase** — *only now* does `mempool.Accept()` (resurrect),
   `mempool.Remove()`, `ChainState` atomic publish, `WriteBestChain`, broadcast, and
   miner notification run.

And `chainstate.cpp` call site #4 now checks the return value before committing:

```cpp
// After — check return value, abort on failure
LLD::TxnBegin();
if(!stateAncestor.SetBest())
{
    debug::error(FUNCTION, "failed to revert to hardcoded ancestor checkpoint");
    LLD::TxnAbort();
}
else
    LLD::TxnCommit();
```

Because `SetBest()` now commits internally on success, the caller's own subsequent
`TxnCommit()` becomes a harmless no-op — the transaction object is already null.

### Why this matters for a learner

The lesson: **wrapping a function call in a transaction pattern at the call site is not
enough if the function itself does side effects that aren't gated on that transaction's
outcome.** The transaction boundary has to be enforced at the point where the
irreversible, non-disk side effects (mempool, in-memory caches) actually happen — not
just at the call site.

---

## Bug 2 (PR #652): `TxnCommit()` couldn't fail — it returned `void`

### The defect

PR #651 made `SetBest()`'s internal `LLD::TxnCommit()` the gate for whether mempool/
`ChainState` mutations run. But `LLD::TxnCommit()` was declared `void`:

```cpp
// Before — silent discard of every per-DB result
void TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
{
    ...
    Ledger->TxnCommit();   // return value dropped
    Trust->TxnCommit();    // return value dropped
    ...
}
```

If, say, the Trust DB's keychain write failed while the Ledger DB succeeded, that
failure was **completely invisible**. `SetBest()` would proceed to resurrect mempool
transactions and publish `ChainState` atomics as though the disk transition had fully
succeeded — even though one of the seven underlying databases was left in a
partially-committed state.

### The fix

`LLD::TxnCommit()` now returns `bool`, and aggregates the per-instance result without
short-circuiting (every instance is still attempted, so state doesn't diverge further
than necessary even when one instance fails):

```cpp
// After — aggregated, non-short-circuiting
bool TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
{
    bool fAllSucceeded = true;
    if(Logical  && ...) { if(!Logical->TxnCommit())  { debug::error(...); fAllSucceeded = false; } }
    if(Contract && ...) { if(!Contract->TxnCommit()) { debug::error(...); fAllSucceeded = false; } }
    if(Register && ...) { if(!Register->TxnCommit()) { debug::error(...); fAllSucceeded = false; } }
    if(Ledger   && ...) { if(!Ledger->TxnCommit())   { debug::error(...); fAllSucceeded = false; } }
    if(Client   && ...) { if(!Client->TxnCommit())   { debug::error(...); fAllSucceeded = false; } }
    if(Trust    && ...) { if(!Trust->TxnCommit())    { debug::error(...); fAllSucceeded = false; } }
    if(Legacy   && ...) { if(!Legacy->TxnCommit())   { debug::error(...); fAllSucceeded = false; } }
    return fAllSucceeded;
}
```

Two flag values are intentional early-return successes, not failures:
`FLAGS::MINER`/`FLAGS::SANITIZE` return `true` immediately (these are guard flags used
by callers that deliberately want to skip a real commit), and `FLAGS::MEMPOOL` returns
`true` after only the in-memory commits (mempool-mode doesn't touch the on-disk
transaction/checkpoint machinery). `TxnRelease()` still runs unconditionally afterward
to clean up checkpoint markers, regardless of the aggregated result.

Every call site that could reach `SetBest()` — `state.cpp` itself, `process.cpp`'s
`ActivateCandidateBestChain()`, `tritium.cpp`'s `TritiumBlock::Accept()`,
`legacy.cpp`'s `LegacyBlock::Accept()`, and both revert paths in `chainstate.cpp` — was
updated to check this new return value.

### Why this matters for a learner

The lesson: **a function that can fail must be able to report failure.** A `void`
return type on an operation with multiple independent sub-steps (seven separate
database instances here) hides exactly the partial-failure case that atomicity code is
supposed to protect against. Changing `void` → `bool` is a small type change with a
large safety implication — and it can't be effective until every caller actually checks
the new value, which is why the PR touched five files at all the affected call sites.

---

## Bug 3 (PR #653): `SetBest()` still depended entirely on the *caller* opening a transaction

### The defect

After PR #651 and #652, `SetBest()` correctly gated mempool/`ChainState` mutation on a
successful **internal** `TxnCommit()`. But `SetBest()` itself never called
`TxnBegin()` — it assumed a transaction was already open, because every *known* caller
happened to open one first. This is an implicit contract, not an enforced one: any
future caller that forgot `TxnBegin()` before calling `SetBest()` would silently hit
`TxnCommit()` with no active transaction and get an unhelpful, silent no-op.

Making `SetBest()` unconditionally call `TxnBegin()` itself was **not** safe, because
`SectorDatabase::TxnBegin()` discards any existing transaction:

```cpp
// SectorDatabase::TxnBegin() — destroys any existing pTransaction
delete pTransaction;
pTransaction = new SectorTransaction();
```

Callers like `TritiumBlock::Accept()` open a transaction *before* calling `SetBest()`
and use it to buffer their own vtx/producer writes. An unconditional `TxnBegin()` inside
`SetBest()` would silently **destroy those already-buffered writes**.

### The fix — self-containment without destroying caller state

Two new LLD primitives were added:

- `SectorDatabase<>::HasTransaction()` — returns whether `pTransaction != nullptr`
  (thread-safe, under the existing transaction mutex).
- `LLD::HasOpenTransaction(nFlags, nInstances)` — fans this out across the
  Ledger/Contract/Register/Trust/Legacy instances relevant to a consensus commit;
  returns `false` unconditionally for `MEMPOOL`/`MINER`/`SANITIZE` modes.

`SetBest()` now conditionally opens its own transaction, only if the caller hasn't
already opened one:

```cpp
const bool fOwnedTxn = !LLD::HasOpenTransaction(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
if(fOwnedTxn)
    LLD::TxnBegin(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
```

And every early-return failure path inside `SetBest()` now unconditionally calls
`TxnAbort()`:

```cpp
LLD::TxnAbort(FLAGS::BLOCK, LLD::INSTANCES::CONSENSUS);
return debug::error(FUNCTION, ...);
```

This is safe in both cases:
- If `SetBest()` owns the transaction (`fOwnedTxn == true`), this abort rolls back its
  own disk writes.
- If a caller already owns the transaction, this abort rolls back **everything buffered
  so far**, including the caller's own vtx/producer writes — which is the correct
  behavior, since the whole point of wrapping `Accept()` + `SetBest()` in one
  transaction is that they succeed or fail together. The caller's own subsequent
  `TxnAbort()` call becomes a safe no-op, because `pTransaction` is already null.

### Why this matters for a learner

The lesson: **"assume the caller does X" is not the same as "the callee is safe if the
caller doesn't do X."** Self-containment has to be added carefully when the naive
version (unconditionally opening your own transaction) would itself destroy state a
caller is relying on. The fix here is a good example of a **conditional ownership**
pattern: check whether a resource already exists before creating your own, but always
clean up (abort) unconditionally on failure so the outcome is correct regardless of who
owns it.

---

## Bug 4 (PR #654): The Bug 2 + Bug 3 fixes combined to create false-positive errors on *every* block

### The defect — an interaction bug between three already-merged fixes

This is the most subtle bug in the chain, because **neither PR #652 nor PR #653
introduced a defect in isolation** — the defect only appeared when both were combined:

1. PR #651 made `SetBest()` commit its own transaction internally on success.
2. PR #652 made `TxnCommit()` return `bool`, and made every caller check it — including
   the *outer* `TxnCommit()` in `TritiumBlock::Accept()` / `LegacyBlock::Accept()` /
   `ActivateCandidateBestChain()`, which runs *after* `SetBest()` returns.
3. PR #653 made `SetBest()`'s internal commit null out `pTransaction` for **all**
   consensus DB instances on success.

Put together: on the (very common!) path where a block **does** become the new best
chain, `SetBest()` commits and nulls the transaction internally. Control returns to
`Accept()`, which then calls its own outer `TxnCommit()` — but there's no transaction
left to commit. Post-#652, `TxnCommit()` correctly returns `false` for "no open
transaction." Post-#651-shape code, that `false` was being treated as a hard error:

```cpp
// Before (fires on every best-chain block — false positive!)
if(!LLD::TxnCommit())
    return debug::error(FUNCTION, "disk transaction commit failed for block acceptance");
```

The result: **every single best-chain block acceptance** logged five `TxnCommit` error
messages (one per DB instance check inside the aggregation) and made `Accept()` return
`false` — despite the block being fully and correctly committed to disk. The chain
itself was not corrupted (the block really was committed), but a caller returning
`false` on every successful acceptance has real risk for anything downstream that keys
behavior off that return value (peer scoring, retry logic, reorg handling).

### The fix — distinguish "nothing to commit" from "commit failed"

Each outer `TxnCommit()` call that follows a potential `SetBest()` path is now guarded
with `LLD::HasOpenTransaction()`:

```cpp
// After — distinguishes "already committed by SetBest()" from "a real failure"
if(LLD::HasOpenTransaction() && !LLD::TxnCommit())
    return debug::error(FUNCTION, "disk transaction commit failed for block acceptance");
```

This distinguishes two cases:

- **Case A — block became the new best chain.** `SetBest()` already committed and
  nulled the transaction. `HasOpenTransaction()` is `false`, so the outer
  `TxnCommit()` call is skipped entirely. `Accept()` returns `true`, correctly.
- **Case B — block was accepted but did *not* become the best chain** (e.g. it's a
  side-chain block that doesn't win consensus yet). The transaction opened by
  `Accept()` is still open (`SetBest()` was never reached, or returned early without
  touching it). `HasOpenTransaction()` is `true`, so `TxnCommit()` runs for real, and a
  failure here is a genuine error.

The same guard was applied at all four affected call sites:
`TritiumBlock::Accept()`, `LegacyBlock::Accept()`,
`ActivateCandidateBestChain()` (`fTransaction=true` path), and both revert paths in
`chainstate.cpp`.

### Why this matters for a learner

The lesson: **each of PRs #651–#653 was individually correct, but the interaction
between "callee now sometimes owns and completes the whole transaction" (#651) and
"caller now must check its own commit result" (#652) created a new class of false
positive that neither PR's author could have seen by reviewing that PR alone.** This is
why sequences of small, well-reviewed, individually-correct changes still need
integration testing against real end-to-end behavior (in this case: "log output on
every accepted block," not just the two changed functions) — and it is exactly the kind
of defect that a full regression run (rather than per-PR unit tests only) is designed to
catch.

---

## The bug chain at a glance

| # | PR | Symptom before fix | Root cause | Fix |
|---|-----|--------------------|------------|-----|
| 1 | [#651](https://github.com/NamecoinGithub/LLL-TAO/pull/651) | Mempool/`ChainState` could mutate even if the disk phase later failed; checkpoint-revert committed unconditionally | Side effects not gated on any commit boundary | Reordered `SetBest()` into disk → internal commit → post-commit phases; checked `SetBest()` return before `TxnCommit()` at call site #4 |
| 2 | [#652](https://github.com/NamecoinGithub/LLL-TAO/pull/652) | A partial per-DB commit failure (e.g. Trust DB) was invisible | `LLD::TxnCommit()` was `void` | Changed to `bool`, aggregated non-short-circuiting per-instance results, updated all callers to check it |
| 3 | [#653](https://github.com/NamecoinGithub/LLL-TAO/pull/653) | A future caller forgetting `TxnBegin()` before `SetBest()` would silently no-op | `SetBest()` had no self-contained transaction boundary | Added `HasTransaction()`/`HasOpenTransaction()`; `SetBest()` conditionally opens its own transaction only if none is open, and unconditionally aborts on any failure path |
| 4 | [#654](https://github.com/NamecoinGithub/LLL-TAO/pull/654) | Every best-chain block acceptance logged 5 false `TxnCommit` errors and returned `false` | Interaction between #651 (callee commits &amp; nulls transaction) and #652 (caller now checks its own commit result) | Guard outer `TxnCommit()` calls with `HasOpenTransaction()` to distinguish "already committed internally" from "a real failure" |

```
PR #651  ──fixes──▶  ordering bug
             │
             └─exposes──▶  PR #652 ──fixes──▶  silent-void bug
                              │
                              └─exposes──▶  PR #653 ──fixes──▶  caller-dependency bug
                                               │
                                               └─combines with #652──▶  PR #654 ──fixes──▶  false-positive bug
```

See [`docs/diagrams/architecture/setbest-transaction-boundary.md`](../diagrams/architecture/setbest-transaction-boundary.md)
for sequence diagrams of the before/after behavior for each bug, and a state diagram of
the transaction ownership model introduced in PR #653/#654.

---

## Regression test coverage added across the four PRs

All tests live in `tests/unit/TAO/Ledger/setbest_txn_ordering.cpp`:

1. **Call site #4 abort-on-failure** (PR #651) — `TxnCommit` never reached when
   `SetBest()` fails; `TxnAbort` called instead.
2. **Mempool ordering** (PR #651) — resurrect/remove only fire after `TxnCommit`;
   untouched when the disk phase fails.
3. **`ChainState` atomics ordering** (PR #651) — only published post-`TxnCommit`;
   unchanged when the disk phase fails.
4. **`TxnCommit()` no-transaction case** (PR #652) — returns `false` when no active
   transaction exists.
5. **`TxnCommit()` all-succeed case** (PR #652) — returns `true` when all selected
   instances have active transactions.
6. **`TxnCommit()` partial-failure case** (PR #652) — returns `false` (aggregated) when
   one instance fails, and confirms no short-circuit by verifying the *other* instance's
   transaction still committed.
7. **`TxnCommit()` MINER/SANITIZE case** (PR #652) — returns `true` for these guard
   flags.
8. **Real-code Test 4** (PR #653) — real `SetBest()` on a block with a missing vtx
   transaction: asserts `HasIndex()` is `false` (rolled back by `TxnAbort`), `ChainState`
   atomics unchanged, no transaction left open.
9. **Real-code Test 5** (PR #653) — mirrors the real call-site #4 pattern
   (`TxnBegin()` → `SetBest()` → outer `TxnAbort()`); confirms the outer abort is a safe
   no-op after the self-containment fix.
10. **Real-code Test 6** (PR #653) — `mempool.Size()` is identical before/after a
    failing `SetBest()` call, confirming mempool is never touched when the disk phase
    fails.
11. **Case A / Case B guard** (PR #654, Tests 8–9) — Test 8 asserts
    `HasOpenTransaction()` is `false` and a subsequent `TxnCommit()` correctly returns
    `false` (expected, not an error) after `SetBest()` succeeds; Test 9 asserts
    `HasOpenTransaction()` is `true` and `TxnCommit()` succeeds when `SetBest()` was
    never reached.

Notably, PR #653 explicitly replaced *simulation-only* tests (which re-implemented the
expected ordering as inline fake logic without exercising real code) with tests that
call the real `BlockState::SetBest()` / `LLD::TxnCommit()` APIs, using a
`RealCodeLedgerGuard` / `ChainStateGuard` RAII pattern to isolate global state between
tests. This is itself a good practice to carry forward: **a regression test that only
re-implements the expected behavior in the test file does not catch a bug in the real
implementation.**

---

## Relationship to issue #254

[Issue #254](https://github.com/Nexusoft/LLL-TAO/issues/254) requests two things:

- **Part 1** — "Local Stake/Local Mined NODE Autocorrect for Accepted but Losing Network
  Consensus Blocks" (referencing PR #640).
- **Part 2** — A stake-block burst cooldown proposal to reduce orphan/stale-tip stalls
  during rapid stake-block bursts (observed pattern: ~5 blocks in 10 seconds).

The issue's "Useful Recent Stability PRs" list explicitly includes #651–#654 (this RC13
bug chain) alongside #640 and #649. The connection: issue #254's Part 1 autocorrect
logic and Part 2's burst-cooldown proposal both depend on `SetBest()` behaving
atomically and correctly reporting success/failure — an autocorrect mechanism that
"accepts a losing block, then needs to revert" is exactly the reorg/revert code path
that PRs #651–#654 hardened. A burst-cooldown mechanism that needs to know "did the
last stake block's chain transition actually complete" also depends on `SetBest()` (and
`Accept()`) returning an accurate, non-false-positive result — which is precisely what
PR #654 fixed. RC13 is a **prerequisite hardening step** for building issue #254's
proposals on top of a `SetBest()` that is provably atomic and provably accurate about
its own success/failure, rather than building burst-recovery logic on top of the
false-positive-error and mempool/disk-divergence risks described above.

---

## Further reading

- [`docs/diagrams/architecture/setbest-transaction-boundary.md`](../diagrams/architecture/setbest-transaction-boundary.md) — Mermaid diagrams for this bug chain
- `src/TAO/Ledger/state.cpp` — `BlockState::SetBest()`
- `src/LLD/global.cpp` / `src/LLD/include/global.h` — `LLD::TxnCommit()`, `LLD::HasOpenTransaction()`
- `tests/unit/TAO/Ledger/setbest_txn_ordering.cpp` — regression tests
- [Nexusoft/LLL-TAO#254](https://github.com/Nexusoft/LLL-TAO/issues/254)
