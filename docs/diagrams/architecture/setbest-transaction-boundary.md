# RC13 Diagrams: `SetBest()` Transaction Boundary

Companion diagrams for
[`docs/release/rc13-transactional-chain-transition-fixes.md`](../../release/rc13-transactional-chain-transition-fixes.md),
covering the four interlocking fixes shipped in
[nexus-node-v5.1.0-localhost-prime-cpu-rc13](https://github.com/NamecoinGithub/LLL-TAO/releases/tag/nexus-node-v5.1.0-localhost-prime-cpu-rc13-linux-x86_64)
(PRs [#651](https://github.com/NamecoinGithub/LLL-TAO/pull/651),
[#652](https://github.com/NamecoinGithub/LLL-TAO/pull/652),
[#653](https://github.com/NamecoinGithub/LLL-TAO/pull/653),
[#654](https://github.com/NamecoinGithub/LLL-TAO/pull/654)) and referenced from
[Nexusoft/LLL-TAO#254](https://github.com/Nexusoft/LLL-TAO/issues/254).

---

## 1. Bug chain overview

```mermaid
flowchart TD
    A["PR #651\nOrdering bug:\nmempool/ChainState mutated\nbefore disk commit"] -->|"fix exposes"| B["PR #652\nSilent-void bug:\nTxnCommit() couldn't report\npartial per-DB failure"]
    B -->|"fix exposes"| C["PR #653\nCaller-dependency bug:\nSetBest() assumed caller\nalready opened a transaction"]
    C -->|"combines with #652 fix"| D["PR #654\nFalse-positive bug:\nEvery best-chain block logged\n5 false TxnCommit errors"]

    style A fill:#f66,color:#fff
    style B fill:#f66,color:#fff
    style C fill:#f66,color:#fff
    style D fill:#f66,color:#fff
```

---

## 2. PR #651 — Before/After: mempool mutation timing

### Before (bug): mempool/ChainState mutated before any commit

```mermaid
sequenceDiagram
    participant Caller as Accept() / ActivateCandidateBestChain()
    participant SB as SetBest()
    participant Disk as Disk (Ledger/Trust/etc DBs)
    participant Mem as Mempool
    participant CS as ChainState (in-memory atomics)

    Caller->>Disk: TxnBegin()
    Caller->>SB: SetBest()
    SB->>Disk: disconnect old-tip blocks
    SB->>Mem: mempool.Accept() (resurrect) ⚠ before any commit
    SB->>Disk: connect new-tip blocks
    SB->>Mem: mempool.Remove() (confirm) ⚠ before any commit
    SB->>CS: publish new best-chain atomics ⚠ before any commit
    SB-->>Caller: return true
    Caller->>Disk: TxnCommit() -- can still fail here!
    Note over Disk,CS: If TxnCommit() fails NOW, mempool and<br/>ChainState are already wrong. No rollback path.
```

### After (fixed): three strict phases, commit gates everything else

```mermaid
sequenceDiagram
    participant Caller as Accept() / ActivateCandidateBestChain()
    participant SB as SetBest()
    participant Disk as Disk (Ledger/Trust/etc DBs)
    participant Mem as Mempool
    participant CS as ChainState (in-memory atomics)

    Caller->>Disk: TxnBegin()
    Caller->>SB: SetBest()
    rect rgb(255,235,235)
    Note over SB,Disk: Phase 1 — Disk only
    SB->>Disk: disconnect old-tip blocks
    SB->>Disk: connect new-tip blocks
    alt any disk write fails
        SB->>Disk: TxnAbort()
        SB-->>Caller: return false (nothing else touched)
    end
    end
    rect rgb(235,255,235)
    Note over SB,Disk: Phase 2 — Internal commit gate
    SB->>Disk: LLD::TxnCommit() (internal, bool result)
    alt commit fails
        SB-->>Caller: return false (mempool/ChainState untouched)
    end
    end
    rect rgb(235,235,255)
    Note over SB,CS: Phase 3 — Post-commit only
    SB->>Mem: mempool.Accept() (resurrect)
    SB->>Mem: mempool.Remove() (confirm)
    SB->>CS: publish new best-chain atomics
    end
    SB-->>Caller: return true
    Caller->>Disk: TxnCommit() -- now a harmless no-op
```

---

## 3. PR #652 — `TxnCommit()` return-value aggregation

```mermaid
flowchart TD
    START["LLD::TxnCommit(nFlags, nInstances)"] --> GUARD{"nFlags ==\nMINER or SANITIZE?"}
    GUARD -->|yes| RETTRUE1["return true\n(intentional guard, not a failure)"]
    GUARD -->|no| MEMCOMMIT["MemoryCommit() for\nContract/Register/Ledger"]
    MEMCOMMIT --> MEMGUARD{"nFlags ==\nMEMPOOL?"}
    MEMGUARD -->|yes| RETTRUE2["return true\n(in-memory only, no disk txn)"]
    MEMGUARD -->|no| CKPT["TxnCheckpoint() for all\nselected instances"]
    CKPT --> AGG["fAllSucceeded = true"]
    AGG --> L1{"Logical->TxnCommit()"}
    L1 -->|false| ERR1["debug::error(); fAllSucceeded = false"] --> C1
    L1 -->|true| C1{"Contract->TxnCommit()"}
    C1 -->|false| ERR2["debug::error(); fAllSucceeded = false"] --> R1
    C1 -->|true| R1{"Register->TxnCommit()"}
    R1 -->|false| ERR3["debug::error(); fAllSucceeded = false"] --> LD1
    R1 -->|true| LD1{"Ledger->TxnCommit()"}
    LD1 -->|false| ERR4["debug::error(); fAllSucceeded = false"] --> CL1
    LD1 -->|true| CL1{"Client->TxnCommit()"}
    CL1 -->|false/true| TR1{"Trust->TxnCommit()"}
    TR1 -->|false/true| LG1{"Legacy->TxnCommit()"}
    LG1 -->|false/true| RELEASE["TxnRelease() for all instances\n(runs unconditionally)"]
    RELEASE --> RESULT["return fAllSucceeded"]

    style ERR1 fill:#f66,color:#fff
    style ERR2 fill:#f66,color:#fff
    style ERR3 fill:#f66,color:#fff
    style ERR4 fill:#f66,color:#fff
```

**Key point:** every instance is attempted regardless of an earlier failure (no
short-circuit) — a partial-failure scenario (e.g. Trust DB fails, Ledger DB succeeds)
now returns `false` overall instead of being silently swallowed, and no instance is
skipped just because an earlier one failed.

---

## 4. PR #653 — Self-contained transaction ownership

```mermaid
stateDiagram-v2
    [*] --> CheckOwnership: SetBest() called

    CheckOwnership --> CallerOwns: HasOpenTransaction() == true\n(caller already did TxnBegin())
    CheckOwnership --> SelfOwns: HasOpenTransaction() == false\n(fOwnedTxn = true)

    SelfOwns --> TxnBeginSelf: SetBest() calls TxnBegin() itself

    CallerOwns --> DiskPhase
    TxnBeginSelf --> DiskPhase

    DiskPhase --> Success: all disk writes succeed
    DiskPhase --> Failure: any disk write fails

    Failure --> AbortAlways: TxnAbort() called\nunconditionally
    AbortAlways --> CallerOwnedAbort: if CallerOwns,\nrolls back caller's\nbuffered writes too
    AbortAlways --> SelfOwnedAbort: if SelfOwns,\nrolls back only\nSetBest()'s own writes
    CallerOwnedAbort --> ReturnFalse: return false\n(caller's own TxnAbort()\nbecomes safe no-op)
    SelfOwnedAbort --> ReturnFalse

    Success --> InternalCommit: LLD::TxnCommit()\n(nulls pTransaction)
    InternalCommit --> PostCommit: mempool + ChainState mutations
    PostCommit --> ReturnTrue: return true

    ReturnFalse --> [*]
    ReturnTrue --> [*]
```

**Why `SetBest()` can't just always call `TxnBegin()`:** `SectorDatabase::TxnBegin()`
deletes any existing `pTransaction` before creating a new one. If a caller like
`TritiumBlock::Accept()` already buffered vtx/producer writes in an open transaction,
an unconditional `TxnBegin()` inside `SetBest()` would silently discard them. The
`fOwnedTxn` check exists specifically to avoid that.

---

## 5. PR #654 — Case A vs Case B: why the outer `TxnCommit()` needs a guard

```mermaid
flowchart TD
    ACC["Accept() opens TxnBegin(),\nbuffers vtx/producer writes,\ncalls Index() → ... → SetBest()"] --> BRANCH{"Did this block become\nthe new best chain?"}

    BRANCH -->|"Case A: yes"| SB_RAN["SetBest() ran its full\nsuccess path (PR #651):\ncommits internally,\nnulls pTransaction (PR #653)"]
    SB_RAN --> HOT_A{"HasOpenTransaction()?"}
    HOT_A -->|false| SKIP["Outer TxnCommit() SKIPPED\n(nothing left to commit)"]
    SKIP --> TRUE_A["Accept() returns true ✅"]

    BRANCH -->|"Case B: no\n(side-chain / not-yet-winning block)"| NO_SB["SetBest() not reached,\nor returned early without\ntouching the transaction"]
    NO_SB --> HOT_B{"HasOpenTransaction()?"}
    HOT_B -->|true| REAL["Outer TxnCommit() runs for real"]
    REAL --> CHECK{"TxnCommit()\nsucceeds?"}
    CHECK -->|yes| TRUE_B["Accept() returns true ✅"]
    CHECK -->|no| FALSE_B["Accept() returns false\n(genuine error) ❌"]

    style SKIP fill:#9f9
    style TRUE_A fill:#9f9
    style TRUE_B fill:#9f9
    style FALSE_B fill:#f66,color:#fff
```

### Before PR #654 (bug): Case A treated as a failure

```
if(!LLD::TxnCommit())
    return debug::error(FUNCTION, "disk transaction commit failed for block acceptance");
```
Every Case A block (the common, successful path!) hit this because
`TxnCommit()` correctly returns `false` for "nothing to commit" — but the code
couldn't tell that apart from a real failure. Result: 5 error logs + `false` return on
every accepted best-chain block.

### After PR #654 (fixed): guarded with `HasOpenTransaction()`

```
if(LLD::HasOpenTransaction() && !LLD::TxnCommit())
    return debug::error(FUNCTION, "disk transaction commit failed for block acceptance");
```
Now Case A (`HasOpenTransaction() == false`) short-circuits past the check entirely,
and only Case B's real commit failures are treated as errors.

---

## 6. End-to-end timeline across all four PRs (single block acceptance)

```mermaid
sequenceDiagram
    participant Net as Network/Miner
    participant Accept as TritiumBlock::Accept()
    participant Idx as Index() / ActivateCandidateBestChain()
    participant SB as BlockState::SetBest()
    participant LLD as LLD::TxnCommit / HasOpenTransaction
    participant Disk as Consensus DBs
    participant Mem as Mempool
    participant CS as ChainState

    Net->>Accept: submit/relay block
    Accept->>Disk: TxnBegin() [outer transaction]
    Accept->>Disk: buffer vtx/producer writes
    Accept->>Idx: Index()
    Idx->>SB: SetBest() [reached because block extends/wins tip]

    SB->>LLD: HasOpenTransaction()? → true (Accept() owns it)
    Note over SB: fOwnedTxn = false, does NOT call TxnBegin() (PR #653)

    SB->>Disk: disconnect/connect (Phase 1)
    alt disk phase fails
        SB->>Disk: TxnAbort() [rolls back Accept()'s buffered writes too]
        SB-->>Idx: return false
    else disk phase succeeds
        SB->>LLD: TxnCommit() [internal, aggregated bool] (PR #652)
        alt internal commit fails
            SB-->>Idx: return false (mempool/ChainState untouched)
        else internal commit succeeds
            SB->>Mem: resurrect/remove (Phase 3, post-commit)
            SB->>CS: publish new tip atomics (Phase 3)
            SB-->>Idx: return true
        end
    end

    Idx-->>Accept: propagate result
    Accept->>LLD: HasOpenTransaction()? → false (SetBest() already committed)
    Note over Accept,LLD: Outer TxnCommit() SKIPPED (PR #654) — no false-positive error
    Accept-->>Net: Accept() returns true, block fully committed
```

---

## Related documents

- [`docs/release/rc13-transactional-chain-transition-fixes.md`](../../release/rc13-transactional-chain-transition-fixes.md) — full teaching write-up
- [Nexusoft/LLL-TAO#254](https://github.com/Nexusoft/LLL-TAO/issues/254)
