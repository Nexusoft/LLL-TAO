# LedgerDB hash-keyed block reads

`LedgerDB::ReadBlock(hash, BlockState&)` and `LedgerDB::ReadBlock(hash, atomic&)` both check the loaded record before publishing it to the caller. A mismatch returns false and leaves the caller's output unchanged.

```mermaid
flowchart TD
    A["ReadBlock(hash)"] --> B{"config::fClient?"}
    B -->|yes| C["ClientDB::ReadBlock into ClientBlock"]
    B -->|no| D["Sector read into BlockState"]
    C --> E{"GetHash() == hash?"}
    D --> E
    E -->|no| F["Return false. Do not assign or store"]
    E -->|yes| G["Assign BlockState or atomic.store"]
```

Client mode uses `ClientDB` and still applies the same identity check. The non-client path uses the ledger sector record. Callers no longer have to repeat this comparison for these two hash-keyed reads.

```mermaid
sequenceDiagram
    participant Caller
    participant LedgerDB
    participant Temporary as Temporary BlockState
    Caller->>LedgerDB: ReadBlock(hash, atomicState)
    LedgerDB->>Temporary: Read record by hash
    Temporary-->>LedgerDB: Loaded block
    LedgerDB->>LedgerDB: Compare block.GetHash() with hash
    alt hashes differ
        LedgerDB-->>Caller: Return false; atomicState unchanged
    else hashes match
        LedgerDB->>Caller: atomicState.store(Temporary)
        LedgerDB-->>Caller: Return true
    end
```

Production bucket counts must match the on-disk keychain. `LLD::Initialize` opens client-mode `LedgerDB` with 77773 buckets and non-client `LedgerDB` with `256 * 256 * 64`. `ClientDB` is separate and is opened with 1000000 buckets. A test that creates `ClientDB` must use that production count, and it must restore or erase only the record it wrote.
