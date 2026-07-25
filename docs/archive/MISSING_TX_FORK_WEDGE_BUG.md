# Missing-Transaction Retry Limit Permanent Fork Wedge — Root Cause Analysis & Fix

## Overview

A bug in `Process()` in `src/TAO/Ledger/process.cpp` caused a node that got stuck on a
locally-mined stale fork to be **permanently wedged** there until manually restarted.  The node
kept 12+ connected peers throughout but never converged onto the peers' heavier chain.

The bug is present in the same code path both for the top-level `Process()` missing-transaction
handling **and** for the orphan-block draining loop that runs after a block is accepted.

---

## Observed Symptoms

1. Node mines a block extending its own local chain tip (e.g. height 6789705,
   `hashPrevBlock=24fb86a0ae0d2752044b`).
2. Peers repeatedly advertise a different, heavier `BESTCHAIN` (e.g. peer height 6789708):
   ```
   Tritium Node : 23.240.35.30 ACTION::NOTIFY: BESTCHAIN differs; requesting branch
       84b025d6249d65f32d68 known=no peer_height=6789708 local_height=6789704
   ```
3. The node requests the branch, but incoming blocks from that branch are missing transactions:
   ```
   NOTICE: Process : missing 14 transactions
   ProcessPacket : missing tx 01fb9768b27de2387590
   ...
   ```
4. After 50 re-request retries (`MAX_MISSING_TRANSACTIONS_RETRIES = 50`) the retry limit fires:
   ```cpp
   if(mapLastMissing[hashBlock] > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
   {
       block.vMissing.clear();
       block.hashMissing = 0;
   }
   ```
   The block is silently dropped — **but the `mapLastMissing[hashBlock]` entry is left at 51+**.
5. On every subsequent BESTCHAIN notify the same block arrives again.  Because
   `mapLastMissing[hashBlock]` is already > 50, the code increments to 52, immediately sees
   52 > 50, and clears vMissing again — the block is **permanently silenced with zero recovery
   action**.
6. Meanwhile the node's own miner continues to extend the stale fork one block at a time since
   the local best-chain pointer never advances.
7. **Only a manual restart fixes the wedge** — the restart rebuilds mempool/tx state from disk
   cleanly, allowing the correct chain to be accepted.

---

## Root Cause

### Primary path (`Process()` top-level missing-tx block)

```cpp
// BEFORE (buggy)
if(mapLastMissing[hashBlock] > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
{
    block.vMissing.clear();   // stop re-requesting transactions
    block.hashMissing = 0;    // signals LLP layer to skip the normal re-request
    // mapLastMissing[hashBlock] stays at 51 !
}
```

Once `mapLastMissing[hashBlock]` exceeds the limit it is never erased.  Every future arrival of
this block immediately hits the `> 50` guard again, increments to 52/53/…, and is dropped with no
recovery action.  The permanent wedge follows.

### Orphan-drain loop (same bug pattern)

The analogous `if(mapLastMissing[hashOrphan] > MAX_MISSING_TRANSACTIONS_RETRIES)` block inside
the orphan-drain BFS loop had the identical defect.

### LLP recovery path (`src/LLP/tritium.cpp`)

When `block.hashMissing == 0` after `Process()` returns `INCOMPLETE`, the only action taken was a
log line:

```cpp
else
    debug::log(2, FUNCTION, "retry limit reached or no missing-block hash; skipping re-request");
```

No recovery action — no branch re-request, no `AttemptPeerBestChainRecovery` attempt, nothing.

---

## Fix

### `src/TAO/Ledger/process.cpp` (both paths)

When the retry limit is exceeded, the `mapLastMissing` entry is now **erased** instead of being
left at 51+.  This resets the retry cycle so the next arrival of the same block starts fresh with
a count of 1 (another 50 re-request attempts), rather than being permanently silenced.  A
`debug::warning` is also emitted so operators can identify the escalation in node logs.

```cpp
// AFTER (fixed)
if(mapLastMissing[hashBlock] > LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES)
{
    debug::warning(FUNCTION,
        "missing-tx retry limit exceeded for block ", hashBlock.SubString(),
        " height=", block.nHeight,
        "; resetting retry counter and escalating to branch recovery");

    mapLastMissing.erase(hashBlock);   // ← KEY FIX: reset so future arrivals get fresh retries
    block.vMissing.clear();
    block.hashMissing = 0;             // signal LLP layer to escalate to branch recovery
}
```

Same change applied to the orphan-drain loop for the `hashOrphan` case.

### `src/LLP/tritium.cpp` (escalated recovery)

When `block.hashMissing == 0` (retry limit was hit and reset), the LLP handler now performs three
recovery actions instead of silently giving up:

1. **`AttemptPeerBestChainRecovery`** — if the peer's advertised `hashBestChain` happens to be on
   disk already (e.g. from a parallel sync path), attempt a bounded ancestry-based chain
   activation.  No-op if the block is not yet on disk, but cheap to try.

2. **Full branch re-request via `LIST / LOCATOR`** — pushes a fresh `ACTION::LIST` with a block
   locator anchored at the current local best-chain hash and the peer's best-chain hash as the
   target.  This delivers all diverged blocks in order from the common ancestor, allowing proper
   sigchain state to build up and bypassing the stale local mempool.

3. **Full block + inline transactions from a random peer** — picks a different random connection
   via `TRITIUM_SERVER->RandomConnection()` and sends `ACTION::GET SPECIFIER::TRANSACTIONS
   TYPES::BLOCK <hashBlock>`.  A peer whose disk/mempool state is unaffected by the local fork may
   be able to deliver the block's transactions in a form that passes local validation.

---

## Flow Diagram

```
BESTCHAIN notify (known=no)
        │
        ▼
LIST / LOCATOR request sent to peer
        │
        ▼
Branch blocks received → Process() called
        │
        ├── vMissing empty → normal Accept() path
        │
        └── vMissing non-empty (missing txs)
                │
                ├── counter ≤ MAX_MISSING_TRANSACTIONS_RETRIES
                │       │
                │       └── SPECIFIER::TRANSACTIONS re-request to random peer
                │           (repeat up to 50 times)
                │
                └── counter > MAX_MISSING_TRANSACTIONS_RETRIES  ← BUG WAS HERE
                        │
                        │   OLD: counter stays at 51+, silent drop forever
                        │
                        │   NEW: counter ERASED, hashMissing = 0
                        │
                        ▼
                   LLP escalation handler (hashMissing == 0)
                        │
                        ├── AttemptPeerBestChainRecovery(hashBestChain)
                        │     (no-op if block not on disk yet)
                        │
                        ├── LIST/LOCATOR full branch re-request
                        │     (delivers blocks in order from ancestor)
                        │
                        └── GET SPECIFIER::TRANSACTIONS BLOCK from random peer
                              (different disk/mempool state may succeed)
```

---

## Impact

Any post-genesis node that:
- Had peers on a heavier fork,
- Received branch blocks whose transactions couldn't be validated against the local (stale) chain
  state, and
- Had those 50 re-request retries exhaust without success

would be permanently wedged until restarted.  The stuck node continued mining on its stale fork
and never synced with the network.

---

## Follow-up: repeating branch-recovery loop (post-fix)

After the wedge fix landed, operators observed a different failure mode: the same block could
re-enter every cycle with the same missing transaction hashes, exhaust retries, and trigger branch
recovery again indefinitely. This avoided a permanent silent wedge, but still caused no forward
progress and repeated network traffic.

### Follow-up root cause

The first fix escalated by re-requesting branch/block data, but did not fan out **individual missing
transaction hash** lookups to multiple peers. If those transactions were unavailable from the peers
that kept re-serving the block (expired/evicted mempool state), the same block could loop forever.

### Follow-up remediation

1. **Per-transaction multi-peer fanout in `src/LLP/tritium.cpp`**  
   In the `hashMissing == 0` branch-recovery path, the node now spreads hashes in `block.vMissing`
   across a small set of distinct connected peers (round-robin) via
   `ACTION::GET TYPES::TRANSACTION <hash>` (using legacy specifier where required), in addition to
   the existing branch/block re-requests.

2. **Escalation-cycle cap in `src/TAO/Ledger/process.cpp` / `include/process.h`**  
   A new `mapMissingBranchEscalations` counter tracks full branch-recovery cycles per block hash
   (incremented each time missing-tx retries are exhausted and `mapLastMissing` is erased while
   `hashMissing` is set to 0). The missing-hash list is preserved for LLP fanout requests.
   Once this exceeds `MAX_BRANCH_RECOVERY_ESCALATIONS` (3), LLP suppresses repeating branch-recovery
   traffic for that block and emits an explicit operator-facing warning that manual intervention
   (peer refresh/resync) is required.

3. **Tests (`tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp`)**  
   Coverage now verifies escalation-cycle counting and cap behavior in addition to the original
   retry-counter erase/reset assertions.

---

## Regression Tests

Two assertions were updated and one new test case was added in
`tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp`:

1. **Updated**: the existing "Missing transactions yield a soft INCOMPLETE, never a REJECT" test
   now verifies that `mapLastMissing.count(hashBlock) == 0` after the retry limit is exceeded
   (the entry must be **erased**, not left at 51+), and that a subsequent `Process()` call starts
   a fresh cycle (count == 1).

2. **New**: `"Orphan-drain retry-limit erases counter and signals branch recovery"` — exercises the
   orphan-drain loop's analogous fix by pre-filling `mapLastMissing[hashOrphan]` to exactly
   `MAX_MISSING_TRANSACTIONS_RETRIES`, processing the parent block (which triggers the drain), and
   verifying that the entry is erased and `block.hashMissing == 0` after the over-limit call.

Run with:

```bash
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
./nexus "[ledger][process]"
```

---

## Follow-up 3: orphan-purge staleness, throttle-key collision, and true walkback recovery

A later review pass (PR #660-series) found four additional gaps left open by the follow-up work
above.

### 1. Incomplete purge on orphan-pool flush (`process.cpp`)

`nConsecutiveOrphans >= 10000` cleared `mapOrphans` inline, but left `setUnrecoverableBlocks`,
`mapLastMissingProcessTime`, `mapLastMissing`, `mapMissingBranchEscalations`, and
`mapLastOrphanRequest` untouched. A block blacklisted against the now-discarded orphan graph
stayed permanently blacklisted even after the purge. `PurgeOrphanRecoveryState(pszReason)` now
owns `PROCESSING_MUTEX` itself and clears all six structures atomically; `tritium.cpp` calls it
instead of reaching into ledger globals directly.

### 2. `mapLastOrphanRequest` key-namespace collision (`process.cpp`)

The throttle map was written with two different key types: the LLP capped-path wrote
`hashBlock` (the stuck block itself), while the orphan-drain BFS loop wrote and erased
`hashPrevBlock` (the missing ancestor). `erase(hashParent)` in the drain loop could therefore
never clear entries the LLP path had written. `ShouldSendBranchSyncRequest(hashAncestor)`
replaces the old helper and is keyed canonically on the missing ancestor's `hashPrevBlock`
everywhere it is called — including the peer-best-chain gap path, which had briefly regressed to
keying on the branch **tip** (`hashPeerBest`) instead of the deepest walked ancestor.

### 3. Blacklist self-healing (`process.cpp`)

The terminal blacklist guard (`setUnrecoverableBlocks.count(hashBlock)`) originally returned
`PROCESS::IGNORED` with no `hashMissing` set. `IGNORED` is never inspected by the LLP layer, so
the throttled `ShouldSendBranchSyncRequest()` capped-path branch — reachable only via
`PROCESS::INCOMPLETE` — fired exactly once, on the arrival that *first* inserted the blacklist
entry. Every later arrival of the same block returned `IGNORED` and went silent forever, making
the `ORPHAN_REQUEST_THROTTLE_SECONDS` throttle moot (there was never a second call to throttle).
The guard now returns `INCOMPLETE` with `hashMissing = 0` on every arrival — still skipping
`Check()` and the escalation counter entirely — so the LLP capped-path branch, and therefore
`ShouldSendBranchSyncRequest()`, stays reachable for the life of the blacklist entry.

### 4. `AttemptPeerBestChainRecovery()` walks the graph instead of only logging (`process.cpp`)

Previously computed `fInOrphanPool`, logged it, and always returned `false` — deferring the
actual recovery to a caller that never existed. It now walks the orphan graph backwards from
`hashPeerBest` via `hashPrevBlock` (depth-capped at `MAX_BLOCK_ORPHANS`, with a visited-set guard
against a crafted parent cycle) to find the deepest ancestor already on disk. If found, it
releases `PROCESSING_MUTEX` and feeds that ancestor through `Process()`, letting the existing BFS
drain connect the chain forward. If there is a genuine gap, it issues the same throttled,
locator-anchored `LIST` as item 2. A `thread_local` re-entrancy guard makes explicit the one-way
invariant that nothing inside `Process()` may call back into this function on the same thread
(the mutex is non-recursive).

### The SPECIFIER protocol gotcha

> **This section was updated after the regression described in follow-up 4 below.** The
> original wording advised using `SPECIFIER::SYNC` for recovery paths; that was incorrect
> and caused the production failure documented there.

The gap-path `LIST` request must carry `SPECIFIER::TRANSACTIONS` (or `SPECIFIER::CLIENT`
in `-client` mode) — **not** `SPECIFIER::SYNC` and not a plain/legacy specifier — so the
peer pushes inline transaction bodies then the block tagged `SPECIFIER::TRITIUM`, which
the receiver accepts unconditionally regardless of sync state.  A locator request sent
without any specifier silently degrades to header-only blocks.

---

## Follow-up 4 — `SPECIFIER::SYNC` regression in fork-recovery paths (PR #66x)

### Summary

After initial synchronization, `SPECIFIER::SYNC` on `ACTION::LIST` silently fails:
the receiving node's handler rejects every such response as "unsolicited" and forces
a disconnect.  Follow-ups 1–3 changed the recovery-path `LIST` calls to use
`SPECIFIER::SYNC`, which is correct during initial sync but wrong once
`fSynchronized == true`.  The regression was invisible until a live fork occurred on an
already-synced operator node.

### Operator-visible diagnostic signature

```
ACTION::NOTIFY: BESTCHAIN differs; requesting branch <hash>
               known=no peer_height=N local_height=M
DROPPED: ProcessPacket : unsolicted sync block
Tritium Node : <ip> Outgoing Disconnected (Force)
```

The 286–362 ms gap between "requesting branch" and "DROPPED" is the round-trip time to
the peer; the dropped packet is the peer's answer to *our own* request.

### `ACTION::LIST` specifier semantics and sync-state coupling

The specifier byte is optional on the wire but load-bearing for content.  The
`ACTION::LIST` parser peeks the first byte and only consumes it if it matches a known
specifier.  Omitting it does not desync the stream or trigger `debug::drop` — but
`nSpecifier` defaults to `0`, and `PushBlock()` then returns **bare block headers with
no transactions**.  This was the original fork-wedge defect.

#### Specifier table

| Specifier | What the peer returns | Receiver-side gate |
|---|---|---|
| `SPECIFIER::SYNC` | `SyncBlock` with inline `vtx` | **Rejected if `nCurrentSession != nSyncSession \|\| fSynchronized`** — initial sync only |
| `SPECIFIER::TRANSACTIONS` | transactions pushed individually, then block tagged `SPECIFIER::TRITIUM` | **No sync-state gate** — correct for post-sync recovery |
| `SPECIFIER::CLIENT` | `ClientBlock` | `-client` mode only |
| none / `0` | bare header, no transactions | Never correct for recovery |

#### The sync-state coupling rule

> **The correct specifier depends on whether the requesting node is still synchronizing.**
>
> - Sync-time paths (`TritiumNode::Sync()`, `ACTION::NOTIFY → TYPES::LASTINDEX` handler): use `SPECIFIER::SYNC`.
> - Post-sync recovery paths (all `ACTION::LIST` branch-recovery calls after `fSynchronized == true`): must use `SPECIFIER::TRANSACTIONS`.

Getting this wrong causes silent peer force-disconnects that only manifest during a fork
on an already-synced node.

#### Why the failure mode hid so long

`Sync()` and the `LASTINDEX` handler always used the correct specifier, so normal
operation exercised the right code path every time.  Only fork recovery — which runs
rarely and only on an already-synced node — used the wrong one.  First the specifier was
omitted entirely (headers only, no transactions); the over-correction to `SYNC` then
caused `DROPPED: ProcessPacket : unsolicted sync block` plus `Outgoing Disconnected
(Force)`.

### Changed call sites

| File | Location | Old specifier | New specifier |
|---|---|---|---|
| `src/LLP/tritium.cpp` | `ACTION::NOTIFY → TYPES::BESTCHAIN` handler | `SYNC` | `TRANSACTIONS` |
| `src/LLP/tritium.cpp` | Capped-path throttled LIST (`IsMissingBranchRecoveryCapped`) | `SYNC` | `TRANSACTIONS` |
| `src/LLP/tritium.cpp` | Retry-limit-exhausted branch-recovery LIST | `SYNC` | `TRANSACTIONS` |
| `src/TAO/Ledger/process.cpp` | `AttemptPeerBestChainRecovery()` gap-path LIST | `SYNC` | `TRANSACTIONS` |
| `src/TAO/Ledger/process.cpp` | Orphan-insert locator LIST | `SYNC` | `TRANSACTIONS` |
| `src/TAO/Ledger/process.cpp` | `RandomConnection()` fallback LIST | `SYNC` | `TRANSACTIONS` |

### Unchanged call sites (correct as-is)

| File | Location | Specifier | Rationale |
|---|---|---|---|
| `src/LLP/tritium.cpp` | `TritiumNode::Sync()` | `SYNC` | Runs during initial sync; `fSynchronized == false` |
| `src/LLP/tritium.cpp` | `ACTION::NOTIFY → TYPES::LASTINDEX` handler | `SYNC` | Only fires while `nCurrentSession == nSyncSession` |

### Related files (follow-up 4)

| File | Change |
|---|---|
| `src/LLP/tritium.cpp` | BESTCHAIN handler, capped LIST, retry-limit LIST: `SYNC` → `TRANSACTIONS` |
| `src/TAO/Ledger/process.cpp` | Three recovery-path LIST calls: `SYNC` → `TRANSACTIONS`; stale comments updated |
| `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` | Post-sync specifier assertions; unsolicited-sync guard test |
| `docs/archive/MISSING_TX_FORK_WEDGE_BUG.md` | This section |

---

### Related files (follow-up 3)

| File | Change |
|------|--------|
| `src/TAO/Ledger/process.cpp` | `PurgeOrphanRecoveryState()`; `ShouldSendBranchSyncRequest()` key unification; blacklist guard reports `INCOMPLETE`/`hashMissing=0` instead of silent `IGNORED`; `AttemptPeerBestChainRecovery()` walkback with cycle detection and re-entrancy guard; restored `-checkpoints` gate |
| `src/LLP/tritium.cpp` | Calls `PurgeOrphanRecoveryState()` instead of clearing `mapOrphans` directly; capped-path `LIST` keyed via `ShouldSendBranchSyncRequest(block.hashPrevBlock)` |
| `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` | Regression coverage for all of the above |

---

## Related Files


| File | Change |
|------|--------|
| `src/TAO/Ledger/process.cpp` | Erase `mapLastMissing` on retry-limit exceeded (both primary and orphan-drain paths); add `debug::warning` |
| `src/LLP/tritium.cpp` | Add branch recovery actions when `hashMissing == 0` |
| `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` | Update existing test; add orphan-drain retry-limit test |
| `docs/archive/MISSING_TX_FORK_WEDGE_BUG.md` | This document |
