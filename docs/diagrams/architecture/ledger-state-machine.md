# TAO Register State Transitions

Diagrams showing the lifecycle and state transitions of Nexus TAO registers.

---

## Register Type Hierarchy

```mermaid
flowchart TD
    REG[Register Base] --> RO[READONLY 0x01<br/>Immutable data]
    REG --> AP[APPEND 0x02<br/>Append-only]
    REG --> RAW[RAW 0x03<br/>Mutable raw data]
    REG --> OBJ[OBJECT 0x04<br/>Enforced fields]
    REG --> SYS[SYSTEM 0x05<br/>System-only]

    OBJ --> ACC[ACCOUNT 0x12<br/>User accounts]
    OBJ --> TOK[TOKEN 0x13<br/>Token contracts]
    OBJ --> TRU[TRUST 0x14<br/>Trust/stake]
    OBJ --> NAM[NAME 0x15<br/>Name registrations]
    OBJ --> NS[NAMESPACE 0x16<br/>Namespace definitions]
    OBJ --> CRY[CRYPTO 0x17<br/>Cryptographic registers]
```

---

## Register Operation State Machine

```mermaid
stateDiagram-v2
    [*] --> Reserved: CREATE operation
    Reserved --> PreState: Begin Operation
    PreState --> Executing: Apply Operation
    Executing --> PostState: Operation Success
    Executing --> PreState: Operation Rollback
    PostState --> Committed: Commit to Ledger
    PostState --> PreState: Revert

    state PreState {
        [*] --> CaptureState
        CaptureState --> RecordChecksum
    }

    state PostState {
        [*] --> VerifyChecksum
        VerifyChecksum --> WriteState
    }
```

---

## Trust Register Lifecycle

```mermaid
sequenceDiagram
    participant U as User
    participant TAO as TAO Layer
    participant DB as TrustDB

    U->>TAO: Create Trust Account
    TAO->>DB: Write (stake=0, trust=0, balance=0)
    U->>TAO: Stake Tokens
    TAO->>DB: Update (stake=N, trust=0)
    Note over TAO: Staking period begins...
    loop Every Block
        TAO->>TAO: Calculate Trust Score
        TAO->>DB: Update (trust+=reward)
    end
    U->>TAO: Unstake
    TAO->>DB: Update (stake=0, balance+=stake)
```

---

## Register Operation Flow

```mermaid
flowchart LR
    OP[Operation] --> WR{Type?}
    WR -->|WRITE| W[Write to register]
    WR -->|APPEND| A[Append to register]
    WR -->|CREATE| C[Create new register]
    WR -->|TRANSFER| T[Transfer ownership]
    WR -->|CLAIM| CL[Claim transferred register]
    WR -->|DEBIT| D[Debit from account]
    WR -->|CREDIT| CR[Credit to account]

    W --> PRE[Capture PRESTATE 0x01]
    A --> PRE
    C --> PRE
    T --> PRE
    CL --> PRE
    D --> PRE
    CR --> PRE

    PRE --> EXEC[Execute]
    EXEC --> POST[Record POSTSTATE 0x02<br/>with checksum]
    POST --> COMMIT[Commit to Ledger]
```

---

## Cross-References

- [Consensus Validation](consensus-validation-flow.md)
- [State Machine Templates](../state-machine.md)
- Source: `src/TAO/Register/include/enum.h`
