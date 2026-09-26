# PR #697 Review Hardening

These diagrams document the three follow-up invariants applied after review of
the transaction, checkpoint, and orphan-recovery changes.

## Ordered checkpoint publication

```mermaid
flowchart LR
    C1["Connect block 1"] --> S1["Stage predecessor 0"]
    S1 --> C2["Connect block 2"]
    C2 --> S2["Stage predecessor 1"]
    S2 --> COMMIT["Durable transaction commit"]
    COMMIT --> H1["Evaluate predecessor 0"]
    H1 --> H2["Evaluate predecessor 1"]
    H2 --> TIP["Publish best tip"]
```

Every connected predecessor is evaluated after commit in ascending order. A
newer, immature candidate therefore cannot suppress an earlier matured one.

## Accepted root with incomplete descendant

```mermaid
flowchart TD
    ROOT["Connectable root accepted"] --> BFS["Breadth-first orphan drain"]
    BFS --> MISS["Descendant incomplete"]
    MISS --> STATUS["Status = ACCEPTED | INCOMPLETE<br/>hashMissing = descendant"]
    STATUS --> CLEAR["Erase descendant throttle"]
    CLEAR --> PROGRESS["Return PROGRESS"]
    PROGRESS --> RETRY["Prompt descendant redelivery allowed"]
```

The throttle key comes from `block.hashMissing`; using the accepted root's hash
would leave the incomplete descendant rate-limited.

## Exception-safe mempool cleanup

```mermaid
flowchart TD
    BEGIN["TransactionGuard begins MEMPOOL scope"] --> WORK["Disconnect + remove API indexes"]
    WORK -->|success| COMMIT["TxnCommit"]
    WORK -->|failure return| ABORT["Explicit TxnAbort"]
    WORK -->|exception| DTOR["Guard destructor"]
    DTOR --> ABORT
    COMMIT --> DONE["Scope exit; abort is a no-op"]
```

All exits release the shared transaction coordinator, including exceptions
thrown after `Disconnect()`.
