# Tritium.cpp

Two Tritium translation units open physical transactions that must not outlive an exception.

- `src/TAO/Ledger/tritium.cpp` — `TritiumBlock::Accept()` writes the block and may activate it through `SetBest()`.
- `src/LLP/tritium.cpp` — the client `TYPES::MERKLE` handler indexes a merkle transaction on a long-lived network thread.

## Accept and merkle indexing

```mermaid
flowchart TD
    A["TritiumBlock::Accept()"] --> B["TransactionGuard begins FLAGS::BLOCK / CONSENSUS"]
    B --> C{"Begin owned?"}
    C -->|no| D["Return false"]
    C -->|yes| E["WriteTx, then Index / SetBest"]
    E --> F{"Throw or error return before Release?"}
    F -->|yes| G["Destructor aborts if this thread still owns PhysicalSlot"]
    F -->|no| H["Commit if SetBest left the journal open"]
    H --> I["Release the guard"]
    I --> J["Process producer orphans only after the slot is free"]

    K["LLP TYPES::MERKLE"] --> L["TransactionGuard begins FLAGS::BLOCK / MERKLE"]
    L --> M{"Begin owned?"}
    M -->|no| N["Drop or error. Slot unchanged"]
    M -->|yes| O["WriteTx, IndexBlock, IndexDependant or Connect"]
    O --> P{"Throw?"}
    P -->|yes| Q["Destructor aborts MERKLE journals and PhysicalSlot"]
    P -->|no| R{"TxnCommit succeeds?"}
    R -->|yes| S["Release guard and acknowledge"]
    R -->|no| T["Reject request without releasing guard"]
```

`Accept()` must not process producer orphans while the block guard is armed. MEMPOOL and BLOCK share `pMemory`, so `TxnBegin(FLAGS::MEMPOOL)` is rejected until `Release()`. The LLP merkle handler is the same rule on `INSTANCES::MERKLE`: `IndexDependant` or a database call can throw after begin, and a persistent network thread would otherwise keep `PhysicalSlot` forever.

Both MERKLE handlers reject a request when `TxnCommit()` reports failure and do not log it as accepted. The guard is released only after a successful commit; LLD releases its physical owner as part of commit cleanup even when one of the selected database commits fails.

The lookup server uses the same MERKLE guard in `src/LLP/lookup.cpp`.

Sibling documents: [Process.cpp](process-cpp.md), [State.cpp](state-cpp.md), [LLD global transactions](lld-global-transactions.md).
