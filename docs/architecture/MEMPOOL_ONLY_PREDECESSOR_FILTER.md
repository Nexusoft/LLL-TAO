# Mempool-Only Predecessor Filter (Option B)

**Status:** Implemented  
**Scope:** `src/TAO/Ledger/create.cpp::AddTransactions()` — single insertion point, all channels  
**Tests:** `tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp` (Catch2 tag `[filter_mempool_only_predecessor]`)

---

## 1. Problem Statement

When a block template is built, `AddTransactions()` selects sigchain transactions from the mempool. The mempool is a *volatile* set: between the moment a candidate tx is selected and the moment the block is signed and submitted, another node can broadcast a block that confirms (or supersedes) those mempool transactions.

Consider sigchain **S** with on-disk tip `T0` and mempool entries `T1` (chained on `T0`) and `T2` (chained on `T1`):

```
disk:    T0
mempool: T1 → T2
```

If `AddTransactions()` picks up only `T2` (e.g. `T1` was just accepted by another miner and removed from mempool, or only the tail was listed in this scan), the template carries a `vtx` entry whose `hashPrevTx == T1` — a hash that no longer exists anywhere we'll write to disk. When the block reaches `Accept`, `ValidateVtxSigchainConsistency()` walks each vtx's `hashPrevTx` against disk and rejects the block as `BLOCK_REJECTED`.

This race exists on **every** channel that calls `AddTransactions()`:

- `CHANNEL::STAKE` (0)
- `CHANNEL::PRIME` (1)
- `CHANNEL::HASH` (2)
- `CHANNEL::PRIVATE` (3)

The fix must therefore live in `AddTransactions()` itself, not in any per-channel call site.

---

## 2. Design — Option B

Drop any non-genesis tx whose `hashPrevTx` is **mempool-only** (present in mempool, absent from disk, and not earlier in this same candidate block).

**Invariants:**

| # | Rule | Rationale |
|---|---|---|
| I1 | `IsFirst()` genesis transactions are unconditionally kept | `hashPrevTx == 0`; no predecessor by design |
| I2 | Disk-confirmed predecessor → keep | Steady-state common case; cannot disappear |
| I3 | Mempool-only predecessor (not in block) → drop | Eliminates the race |
| I4 | In-block predecessor → keep | Same-sigchain T(n)+T(n+1) selected together; persisted atomically |
| I5 | Unknown predecessor (neither disk nor mempool) → drop | Dangling reference — never carry into a block |
| I6 | No channel parameter; identical behaviour on STAKE / PRIME / HASH / PRIVATE | Race exists everywhere; filter is correctness-positive everywhere |

---

## 3. Insertion Point

`src/TAO/Ledger/create.cpp::AddTransactions()` is the single shared helper invoked from every channel's `CreateBlock` path:

```
CreateBlock (PoW/Hash & Prime)  ─┐
CreateStakeBlock                 ─┼──► AddTransactions(block)
CreatePrivateBlock               ─┘            │
                                               ▼
                                  ┌──────────────────────────┐
                                  │  per-mempool-tx loop     │
                                  │  ├─ existing checks      │
                                  │  ├─ Option B gate (new)  │
                                  │  └─ block.vtx.push_back  │
                                  └──────────────────────────┘
```

One insertion point, four channels protected.

---

## 4. Algorithm

```
function AddTransactions(block):
    block.vtx.clear()
    vMempool ← mempool.List()
    setDependents ← {}
    setInBlock    ← {}                          # NEW — tracks accepted hashes

    for each hash in vMempool:
        tx ← mempool.Get(hash)
        # ... existing checks (coinbase/coinstake, dependents, timestamp,
        #     Verify, Connect, ReadLast(genesis)) ...

        # ── Option B mempool-only-predecessor gate ──
        if not tx.IsFirst():                    # I1: genesis exempt
            fInBlock ← hash ∈ setInBlock        # I4: in-block chaining preserved
            fOnDisk  ← LLD.Ledger.HasTx(tx.hashPrevTx, FLAGS.BLOCK)
            if not fInBlock and not fOnDisk:    # I3 + I5
                setDependents.insert(hash)
                continue

        block.vtx.push_back((TRITIUM, hash))    # I2 + I4: kept
        setInBlock.insert(hash)
```

`LLD::Ledger->HasTx(hashPrevTx, FLAGS::BLOCK)` is an O(1) keyspace lookup against the persisted ledger, distinct from the existing `ReadLast(hashGenesis, …)` which only confirms the sigchain has *some* disk tip without verifying *this* tx's specific predecessor.

---

## 5. Decision Flow

```
                ┌────────────────────────────────────┐
                │  candidate tx from mempool list    │
                └──────────────────┬─────────────────┘
                                   ▼
                       ┌──────────────────────┐
                       │  tx.IsFirst() ?      │
                       └─────┬────────┬───────┘
                        yes  │        │ no
                             ▼        ▼
                     ┌──────────┐   ┌──────────────────────────┐
                     │  KEEP    │   │ hashPrevTx ∈ setInBlock? │
                     │  (I1)    │   └─────┬──────────────┬─────┘
                     └──────────┘     yes │              │ no
                                          ▼              ▼
                                   ┌──────────┐   ┌──────────────────────────────┐
                                   │  KEEP    │   │ LLD.Ledger.HasTx(prev,BLOCK)?│
                                   │  (I4)    │   └─────┬──────────────────┬─────┘
                                   └──────────┘     yes │                  │ no
                                                        ▼                  ▼
                                                 ┌──────────┐       ┌──────────┐
                                                 │  KEEP    │       │  DROP    │
                                                 │  (I2)    │       │  (I3/I5) │
                                                 └──────────┘       └──────────┘
```

---

## 6. State of the World — Before vs After

### Before (race window)

```
 t0  miner-A picks up vMempool = [T1, T2]; T0 on disk
 t1  miner-B confirms block containing T1 → mempool drops T1
 t2  miner-A still has T1 by value (snapshot), pushes both into vtx
     (or, alt: miner-A picked only [T2] at t0 and never had T1)
 t3  miner-A signs block → submit → Accept walks vtx →
     ValidateVtxSigchainConsistency: T2.hashPrevTx == T1, T1 not on disk
     → BLOCK_REJECTED
```

### After (Option B gate active)

```
 t0  miner-A picks up vMempool = [T2]; T0 on disk; T1 mempool-only
 t1  AddTransactions: tx = T2
       IsFirst?            no
       setInBlock(prev)?   no   (T1 not earlier in this loop)
       HasTx(prev, BLOCK)? no   (T1 not on disk)
       → DROP T2 with "predecessor is mempool-only"
 t2  block.vtx = [] (or contains only safe entries)
 t3  block signed and accepted; T2 is included in the next round
     once T1 is persisted
```

### In-block chain preserved

```
 t0  vMempool = [T1, T2]; T0 on disk
 t1  AddTransactions, loop iter 1: tx = T1
       HasTx(T0, BLOCK) = true → KEEP; setInBlock = {T1}
 t2  loop iter 2: tx = T2
       setInBlock.count(T1) = 1 → KEEP; setInBlock = {T1, T2}
 t3  block.vtx = [T1, T2]; both persisted atomically by Accept
```

---

## 7. Why Channel-Agnostic

The race occurs whenever the mempool changes between *select* and *sign*. That window exists for:

- **STAKE** — stake minter wakes, builds template, computes proof of stake, signs.
- **PRIME / HASH** — miner pulls template, searches nonce space, returns solution.
- **PRIVATE** — private chain producer waits on `mempool.Size() > 0`, builds, signs.

There is no channel for which "carry a dangling `hashPrevTx`" is desirable behaviour. Placing the gate inside `AddTransactions()` — and exposing **no channel parameter** — makes the contract uniform and prevents future channel-specific drift.

---

## 7a. Why the bug is universal but the visibility is PoW-specific

The mempool-vs-template TOCTOU race exists on every channel that calls `AddTransactions()`. However, two properties of PoW make it the only channel where the race produces a visible `BLOCK_REJECTED`:

**1. Merkle immutability post-issue (PoW invariant).** Once a PoW template is handed to a remote miner, its merkle root is frozen: a rebuild would invalidate the miner's burned PoW work. Stake builds merkle deferred (post-solution, pre-sign) and can freely rebuild because `StakeHash()` does not include merkle (see `src/TAO/Ledger/stake_minter.cpp::ProcessBlock` — `BuildMerkleTree` runs after the hash-solution loop exits). Private similarly rebuilds in-process. Only PoW commits to a vtx selection that cannot be amended without invalidating the miner's work.

**2. Long template-to-submit window.** PoW miners hash for seconds-to-minutes per template. Stake's `HashBlock()` loop iterates in-process and `CheckStale()` aborts mid-loop on mempool/chain changes (`src/TAO/Ledger/stake_minter.cpp` — `CheckStale()` invoked every iteration of the hash loop). The TOCTOU window for stake is on the order of milliseconds; for PoW it is on the order of the actual hash time.

These two properties produce a same-bug-different-symptom situation:

- **Stake** with stale vtx → `CheckStale()` triggers → rebuild → never submitted.
- **PoW** with stale vtx → miner solves → submit → `ValidateVtxSigchainConsistency` rejects → wasted PoW work + miner credit loss.

Common amplifier: **burst blocks** (stake bursts or peer-mined block landing during template-build) compress the time between mempool snapshots, increasing the probability that a template built against snapshot N is signed against mempool state N+k. The filter is applied at `AddTransactions()` time inside `CreateBlock`, which is the template-cache key boundary in `MiningTemplateCacheTable`, so cached templates carry the filter's verdict for the full duration of their reuse window — the protection is single-shot at build, not per-reuse.

**Naming convention** — Some readers expect "merkle-immutability invariant" to be filed under PoW because that's where the symptom appears, but the *constraint* belongs to the protocol contract between the node (template issuer) and the remote miner (PoW solver): the merkle root the miner hashed against cannot change. Stake has no remote solver; private has no remote solver. PoW is the only channel that publishes a template across a process boundary, and is therefore the only channel where merkle immutability is load-bearing for correctness.

---

## 8. Test Matrix

`tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp` (Catch2, tag `[filter_mempool_only_predecessor]`) pins the contract independently of LLD/mempool wiring, using an inline simulator:

| # | Case | Invariant |
|---|---|---|
| 1 | `IsFirst()` genesis exempt | I1 |
| 2 | Disk-confirmed predecessor → keep | I2 |
| 3 | Mempool-only predecessor → drop | I3 |
| 4 | In-block chained predecessor → keep | I4 |
| 5 | Mixed input → drop bad, keep good, preserve order | I2 + I3 |
| 6 | Identical across STAKE / PRIME / HASH / PRIVATE | I6 |
| 7 | Empty candidate list → empty result (no-op) | — |
| 8 | Unknown predecessor → drop | I5 |

Run with:

```bash
make -f makefile.cli UNIT_TESTS=1 -j$(nproc)
./nexus "[filter_mempool_only_predecessor]"
```

---

## 9. Implementation Reference

- Production wiring: `src/TAO/Ledger/create.cpp::AddTransactions()` — the Option B gate is placed immediately after the existing `ReadLast(hashGenesis, …)` check and immediately before `block.vtx.push_back(...)`. The `setInBlock` set is updated on every push so that subsequent iterations can satisfy invariant I4.
- Disk classification API: `LLD::Ledger->HasTx(hashTx, FLAGS::BLOCK)` (`src/LLD/types/ledger.h`).
- Design contract & regression net: `tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp`.

## 10. Out of Scope

- Mempool admission ordering / topological sort (separate concern; mempool already returns hashes in arrival/dependency order).
- Producer transaction (`block.producer`) — handled separately in `CreateProducer` / `CreateBlock`; the producer's own `hashPrevTx` correctness is guarded by the singleflight cache-hit re-finalization path described in PR #598.
- Legacy (`TRANSACTION::LEGACY`) vtx — those have no sigchain `hashPrevTx` semantics; the gate only applies to TRITIUM tx in the first loop of `AddTransactions()`.
