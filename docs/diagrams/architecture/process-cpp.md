# Process.cpp

`src/TAO/Ledger/process.cpp` accepts network blocks and activates a heavier candidate through `ActivateCandidateBestChain()`. That function is the only production caller that opens a physical `FLAGS::BLOCK` transaction and then calls `SetBest()`.

`Process()` catches exceptions from that path and marks the block `REJECTED`. The activation transaction must not stay owned when that catch returns, or the process-wide `PhysicalSlot` stays claimed by the processing thread.

## Candidate activation transaction

```mermaid
flowchart TD
    A["ActivateCandidateBestChain(fTransaction)"] --> B{"fTransaction?"}
    B -->|no| C["SetBest() may open its own journal"]
    B -->|yes| D["TransactionGuard begins FLAGS::BLOCK / CONSENSUS"]
    D --> E{"Begin owned?"}
    E -->|no| F["Return false. Slot unchanged"]
    E -->|yes| G["SetBest()"]
    G --> H{"SetBest throws?"}
    H -->|yes| I["Guard aborts while this thread still owns PhysicalSlot"]
    I --> J["Process() catch marks REJECTED"]
    H -->|no| K{"SetBest returned false?"}
    K -->|yes| L["Guard aborts if this thread still owns the slot, then return false"]
    K -->|no| M{"Journal still open?"}
    M -->|yes| N["TxnCommit. Failure leaves the guard armed"]
    M -->|no| O["SetBest already committed and released the slot"]
    N --> P["Release the guard only after success"]
    O --> P
    P --> Q["Return true"]
```

`SetBest()` treats an open journal owned by this thread as an outer transaction and commits it internally on success. On failure, it leaves an outer transaction open for this caller's guard to abort; when `SetBest()` opened its own journal, its self-owned guard aborts it. On exception, either guard aborts only a transaction it still owns.

Sibling documents: [State.cpp](state-cpp.md), [Tritium.cpp](tritium-cpp.md), [LLD global transactions](lld-global-transactions.md).
