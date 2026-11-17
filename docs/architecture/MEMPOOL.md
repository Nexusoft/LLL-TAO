# Mempool semantics

**Status:** Current on NODE5  
**Scope:** `src/TAO/Ledger/types/mempool.h`, `src/TAO/Ledger/mempool.cpp`  
**Related:** [conflict DAG](MEMPOOL_CONFLICT_DAG.md), [mempool-only predecessor filter](MEMPOOL_ONLY_PREDECESSOR_FILTER.md), [BuildAndAccept](BUILD_CPP.md)

This document is the normative description of Tritium mempool admission, overlay ownership, and removal. Later agents must follow these rules. Do not infer that `Remove()` undoes register state, and do not infer that `Accept()` returning true means the transaction is in `mapLedger`.

---

## 1. What the mempool is

The mempool is the set of transactions that have been connected into the in-memory register overlay (`LLD` `FLAGS::MEMPOOL`) but are not yet committed in a block.

`Mempool::MUTEX` is a `std::recursive_mutex`. Every public method that touches pool maps takes it. Callers may also hold it across a multi-step accept / index / rollback sequence. Nested calls on the same thread are legal. A non-recursive lock must not be substituted.

Lock order when a caller also mutates SessionDB indexes:

1. `TAO::Ledger::mempool.MUTEX`
2. `TAO::API::Transaction::IndexLock(hashGenesis)`

Never acquire `mempool.MUTEX` while already holding `IndexLock`. `Indexing::BuildIndexes()` releases `IndexLock` before `BroadcastUnconfirmed()`, because that path calls `Accept()`. `BroadcastUnconfirmed()` takes the pool lock, then `IndexLock`, across the session read and `Broadcast()` write, and drops `IndexLock` before `Accept()`.

---

## 2. Maps and what membership means

| Map | Key | Meaning |
|-----|-----|---------|
| `mapLedger` | tx hash | Live pool. `Connect(FLAGS::MEMPOOL)` has been committed. This is the only set `InPool()` reports. |
| `mapClaimed` | `hashPrevTx` → child hash | The live child that claims that predecessor. One slot. Used to walk a dependent tail. |
| `mapOrphans` / `mapOrphansByIndex` | missing prev / tx hash | Predecessor was not on disk and not in the live pool. Not connected. |
| `mapConflicts` | tx hash | Conflict **roots** only. Descendants are not inserted here. |
| `mapConflictDependents` | parent hash | At most one parked child per parent. Not connected, not error-relayed. |
| `mapConflictDependentsByIndex` | tx hash | Same parked dependents, keyed by their own hash. |
| `mapRejected` | tx hash | Hard reject. A later `Accept()` of the same hash fails closed. |
| `mapLegacy` / `mapLegacyConflicts` | tx hash | Legacy transactions. Not the Tritium sigchain tail. |

`Has(hash)` is broader than `InPool(hash)`. `Has` is true for live, legacy, conflict-root, parked-dependent, and orphan entries. `InPool` is true only for `mapLedger`.

```mermaid
flowchart LR
    subgraph live [Live overlay]
        mapLedger
        mapClaimed
    end
    subgraph waiting [Not connected]
        mapOrphans
        mapConflicts
        mapConflictDependents
    end
    subgraph dead [Do not retry blindly]
        mapRejected
    end
    Accept -->|prev missing| mapOrphans
    Accept -->|tip disagreement| mapConflicts
    Accept -->|parent is a conflict node| mapConflictDependents
    Accept -->|Connect committed| mapLedger
    mapLedger -->|non-genesis| mapClaimed
    Accept -->|Verify or Connect hard-fail| mapRejected
```

---

## 3. Accept: connect is committed before the map insert

`Mempool::Accept()` holds `MUTEX` for the whole call, including `ProcessOrphans()` and `ProcessConflictDependents()`.

For a Tritium transaction that is eligible for the live pool, the success path is:

1. `LLD::TxnBegin(FLAGS::MEMPOOL)`
2. `tx.Connect(FLAGS::MEMPOOL)` — writes the register overlay
3. On connect failure: `TxnAbort(FLAGS::MEMPOOL)`. `DEFERRED_LOCAL_STATE` is **not** inserted into `mapRejected`. Other failures are.
4. `LLD::TxnCommit(FLAGS::MEMPOOL)` — the overlay is now durable for this process
5. `mapLedger[hashTx] = tx`
6. If not genesis: `mapClaimed[tx.hashPrevTx] = hashTx`
7. `ProcessOrphans(hashTx)` — may `Accept` one or more descendants, each of which repeats steps 1–6
8. `ProcessConflictDependents(hashTx)` — may admit a parked tail the same way
9. Relay `NOTIFY` / `TRANSACTION` when this node created the transaction (`pnode == nullptr`)

```mermaid
sequenceDiagram
    participant Caller
    participant Pool as Mempool::Accept
    participant LLD as MEMPOOL overlay
    participant Maps as mapLedger / mapClaimed

    Caller->>Pool: Accept(tx)
    Pool->>LLD: TxnBegin(FLAGS::MEMPOOL)
    Pool->>LLD: tx.Connect(FLAGS::MEMPOOL)
    alt Connect fails
        Pool->>LLD: TxnAbort(FLAGS::MEMPOOL)
        Pool-->>Caller: false (overlay unchanged)
    else Connect succeeds
        Pool->>LLD: TxnCommit(FLAGS::MEMPOOL)
        Note over LLD: Overlay effects are live<br/>before the pool maps change
        Pool->>Maps: insert mapLedger, then mapClaimed
        Pool->>Pool: ProcessOrphans + conflict-dependent drain
        Note over Pool,Maps: Descendants may now be in mapLedger<br/>with their own committed overlay
        Pool-->>Caller: true
    end
```

### Invariants

- **I1.** A transaction in `mapLedger` has already had `Connect(FLAGS::MEMPOOL)` committed. Erasing the map entry does not undo that connect.
- **I2.** `Remove(hash)` only erases pool maps (`mapLedger`, `mapClaimed`, orphans, conflicts, rejected, legacy). It does not call `Disconnect`.
- **I3.** To drop a live transaction, the caller must `Disconnect(FLAGS::MEMPOOL)` under `TxnBegin(FLAGS::MEMPOOL, INSTANCES::MEMORY)`, `TxnCommit` that memory transaction, and only then `Remove`. If disconnect or commit fails, `TxnAbort` and leave the map entry in place. Exception: `Mempool::Check()` orphan eviction does not leave the entry in place. After `TxnAbort`, it calls `Remove(hashTx)` to force-evict a stuck orphan whose `Disconnect` throws or returns false. That `Remove` is not a successful rollback, and no other caller may copy it.
- **I4.** `Accept` of a parent can admit descendants before it returns. Rolling back only the parent leaves those descendants in `mapLedger` with a live predecessor claim. Walk `ClaimedDescendants` and disconnect **nearest child last** (reverse iteration), then the parent, in one MEMPOOL memory transaction.
- **I5.** `Accept` can return true without inserting `mapLedger` (orphan predecessor, or a path that parked the tx). Callers that index or broadcast a "live" tx must check `InPool(hash)`, not the boolean alone.
- **I6.** `mapClaimed` has one child per predecessor. `ClaimedDescendants` walks that chain, nearest first, and stops on a cycle, a missing `mapLedger` entry, or a guard past `mapLedger.size()`.
- **I7.** Conflict roots and parked dependents are not in the live overlay. Do not `Disconnect` them as if they had been connected. Do not cascade descendants into `mapConflicts`. See [MEMPOOL_CONFLICT_DAG.md](MEMPOOL_CONFLICT_DAG.md).

```mermaid
flowchart TD
    A[Caller wants the tx gone] --> B{InPool?}
    B -->|no| C[Remove is enough<br/>no overlay was committed]
    B -->|yes| D[TxnBegin FLAGS::MEMPOOL MEMORY]
    D --> E[Disconnect descendants<br/>farthest child first]
    E --> F[Disconnect the parent]
    F --> G{all Disconnect calls true?}
    G -->|no| H[TxnAbort<br/>leave mapLedger entries]
    G -->|yes| I{TxnCommit?}
    I -->|no| H
    I -->|yes| J[Remove descendants then parent]
```

The flowchart is the rule for every caller except `Mempool::Check()`. `Check()` holds `MUTEX`, decides the per-genesis chain is stale, **then** takes `Transaction::IndexLock(genesis)`, opens `TxnBegin(FLAGS::MEMPOOL, INSTANCES::MEMORY)`, and disconnects in reverse sequence order. It is an exception to I3: if `Disconnect` throws or returns false, it aborts the memory transaction and then explicitly calls `Remove(hashTx)` to force-evict that stuck orphan so the sweep cannot loop on it forever. It does not leave the map entry in place. A disconnect that returns true is erased only after that successful disconnect, and the memory transaction is committed only if it was not aborted.

The contract probe before that decision is a scoped `TransactionGuard` for `FLAGS::SANITIZE`. It ends, and therefore aborts, before `TxnBegin(FLAGS::MEMPOOL)`. A throw from `Sanitize()` releases the sanitize slot instead of leaving it claimed across the disconnect transaction. The disconnect transaction is unchanged: `Disconnect` exceptions are caught and aborted explicitly so the stuck orphan can be force-evicted. A throw from `Delete()` after `Remove` is not wrapped in a guard, because aborting then would restore overlay effects for entries already erased from the pool maps.

`IndexLock` on this path starts after the stale-chain decision (`hashPrevTx !=` ledger last, or a failed contract sanitize). `Indexing::IndexSigchain` does not take `mempool.MUTEX`, so it can finish indexing a confirmed tail in that gap. The following `Delete()` rewrites the predecessor's `hashNextTx` and `indexing.last` and can clobber that tail, even though the lock is held during the delete. `Notifications::SanitizeUnconfirmed` re-reads the tail after acquiring `IndexLock` and skips the delete if the tail changed. `Check()` does not. Taking `IndexLock` before the decision would hold the stripe across contract sanitization for every live genesis. That is intentionally not done. The lock still closes the narrower Remove-to-Delete gap: once held, `IndexSigchain` cannot advance the genesis between `Remove` and `Delete`. This is window W2 in [BUILD_CPP.md](BUILD_CPP.md).

---

## 4. Orphans versus conflicts

```mermaid
flowchart TD
    Start[Accept tx] --> Prev{hashPrevTx on disk<br/>or in mapLedger?}
    Prev -->|missing| Orphan[mapOrphans<br/>not connected]
    Prev -->|present| Seq{sequence / claim<br/>matches live tip?}
    Seq -->|another tx already claimed prev<br/>or prev is not the tip| Root[mapConflicts root<br/>not connected]
    Seq -->|parent is a conflict node| Dep[mapConflictDependents<br/>not connected]
    Seq -->|yes| Live[Connect + mapLedger]
    Orphan -->|parent later accepted| Proc[ProcessOrphans re-enters Accept]
    Dep -->|parent later live| Drain[ProcessConflictDependents re-enters Accept]
```

`ProcessOrphans` runs **before `Accept` returns**. Any code that treats "just accepted" as a single transaction is wrong if a dependent was waiting.

---

## 5. What not to do

- Do not call `Remove` on an `InPool` transaction without a successful MEMPOOL disconnect+commit.
- Do not call `ProcessOrphans` or `Accept` while a physical `FLAGS::BLOCK` transaction owns the shared `pMemory` overlay. `TritiumBlock::Accept()` commits and releases that slot before `ProcessOrphans(hashProducer)`, and only when disk `ReadLast` for the producer genesis is the producer.
- Do not disconnect a parent while a `mapClaimed` child is still connected.
- Do not hold `Transaction::IndexLock` and then call `Accept` / `Remove` (those take `mempool.MUTEX`). Take the pool lock first.
- Do not treat `Check()`'s `IndexLock` as covering the stale-chain decision. The lock starts after that decision (window W2 in [BUILD_CPP.md](BUILD_CPP.md)). Do not close that window by taking `IndexLock` before `MUTEX`, or by taking `MUTEX` inside `IndexSigchain`.
- Do not treat `Has(hash) == true` as "safe to index into SessionDB". Only `InPool` means the live overlay entry exists.
- Do not insert conflict descendants into `mapConflicts`. Park them.
- Do not add a `DEFERRED_LOCAL_STATE` connect failure to `mapRejected`. That blacklist would turn a stale height into a permanent wedge.

---

## 6. File map

| Symbol | File |
|--------|------|
| `Mempool::Accept` | `src/TAO/Ledger/mempool.cpp` |
| `Mempool::ProcessOrphans` | `src/TAO/Ledger/mempool.cpp` |
| `Mempool::InPool` / `Has` / `ClaimedDescendants` | `src/TAO/Ledger/mempool.cpp` |
| `Mempool::Remove` | `src/TAO/Ledger/mempool.cpp` |
| `Mempool::Check` | `src/TAO/Ledger/mempool.cpp` |
| Map declarations | `src/TAO/Ledger/types/mempool.h` |
