# Block Validation Pipeline

Diagrams showing how incoming blocks are validated before acceptance into the blockchain.

---

## Complete Block Validation Flow

```mermaid
flowchart TD
    RB[Receive Block] --> SV[Size Validation<br/>Check block size limits]
    SV -->|Pass| TV[Timestamp Validation<br/>Check time range]
    SV -->|Fail| REJ1[BLOCK_REJECTED]
    TV -->|Pass| VV[Version Validation<br/>Check version timelocks]
    TV -->|Fail| REJ2[BLOCK_REJECTED]
    VV -->|Pass| PV[Producer Validation<br/>Verify producer tx exists]
    VV -->|Fail| REJ3[BLOCK_REJECTED]

    PV -->|Pass| CV[Channel Validation]
    PV -->|Fail| REJ4[BLOCK_REJECTED]

    CV --> CH0{Channel?}
    CH0 -->|PoS| STV[Stake Validation<br/>Verify stake claims]
    CH0 -->|Prime| PRV[Prime Validation<br/>Verify prime chain]
    CH0 -->|Hash| HV[Hash Validation<br/>Verify hash target]

    STV --> MV[Merkle Root Validation<br/>Build and verify merkle tree]
    PRV --> MV
    HV --> MV

    MV -->|Pass| TXV[Transaction Verification<br/>Validate all transactions]
    MV -->|Fail| REJ5[BLOCK_REJECTED]
    TXV -->|Pass| DUP[Duplicate Check<br/>Check for conflicts]
    TXV -->|Fail| REJ6[BLOCK_REJECTED]
    DUP -->|Pass| ACC[Accept Block<br/>Commit state changes]
    DUP -->|Fail| REJ7[BLOCK_REJECTED]
    ACC --> NB[Broadcast NEW_BLOCK<br/>Notify connected miners]

    style ACC fill:#2d6,stroke:#333
    style REJ1 fill:#d33,stroke:#333,color:#fff
    style REJ2 fill:#d33,stroke:#333,color:#fff
    style REJ3 fill:#d33,stroke:#333,color:#fff
    style REJ4 fill:#d33,stroke:#333,color:#fff
    style REJ5 fill:#d33,stroke:#333,color:#fff
    style REJ6 fill:#d33,stroke:#333,color:#fff
    style REJ7 fill:#d33,stroke:#333,color:#fff
```

---

## Consensus Channels

```mermaid
flowchart LR
    subgraph "Proof of Stake (Channel 0)"
        S1[Verify Trust Score] --> S2[Check Stake Amount]
        S2 --> S3[Validate Coinstake TX]
    end

    subgraph "Prime Mining (Channel 1)"
        P1[Verify Prime Chain] --> P2[Check Cluster Size]
        P2 --> P3[Validate Difficulty]
    end

    subgraph "Hash Mining (Channel 2)"
        H1[Compute Block Hash] --> H2[Compare to Target]
        H2 --> H3[Validate Nonce]
    end
```

---

## Transaction Verification Detail

```mermaid
flowchart TD
    TX[Transaction] --> TYPE{Type?}
    TYPE -->|Tritium| TRI[Tritium TX Verify]
    TYPE -->|Legacy| LEG[Legacy TX Verify]
    TYPE -->|Coinbase| CB[Coinbase Verify]
    TYPE -->|Coinstake| CS[Coinstake Verify]

    TRI --> SIG[Signature Check]
    LEG --> SIG
    CB --> REWARD[Reward Amount Check]
    CS --> STAKE[Stake Rules Check]

    SIG --> FINAL[Add to Verified Set]
    REWARD --> FINAL
    STAKE --> FINAL
```

---

## Cross-References

- [Mining Flow](mining-flow-complete.md)
- [Ledger State Machine](ledger-state-machine.md)
- [Opcodes Reference](../../reference/opcodes-reference.md)
- Source: `src/TAO/Ledger/tritium.cpp`
