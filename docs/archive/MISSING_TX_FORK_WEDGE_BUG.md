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
   In the `hashMissing == 0` branch-recovery path, the node now requests each hash in `block.vMissing`
   from multiple distinct connected peers via `ACTION::GET TYPES::TRANSACTION <hash>` (using legacy
   specifier where required), in addition to the existing branch/block re-requests.

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

## Related Files

| File | Change |
|------|--------|
| `src/TAO/Ledger/process.cpp` | Erase `mapLastMissing` on retry-limit exceeded (both primary and orphan-drain paths); add `debug::warning` |
| `src/LLP/tritium.cpp` | Add branch recovery actions when `hashMissing == 0` |
| `tests/unit/TAO/Ledger/missing_tx_soft_fail.cpp` | Update existing test; add orphan-drain retry-limit test |
| `docs/archive/MISSING_TX_FORK_WEDGE_BUG.md` | This document |
