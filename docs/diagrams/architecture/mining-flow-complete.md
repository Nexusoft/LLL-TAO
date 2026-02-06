# End-to-End Mining Lifecycle

Complete sequence diagram showing the full mining lifecycle from authentication through block submission and new block notification.

---

## Stateless Tritium Mining Protocol Flow

```mermaid
sequenceDiagram
    participant M as Miner
    participant LL as LLP Server
    participant MC as MiningContext
    participant BC as Blockchain
    participant MM as Mempool

    M->>LL: MINER_AUTH (Falcon)
    LL->>MC: CreateSession
    MC->>LL: SessionID
    LL->>M: AUTH_RESULT
    M->>LL: SET_CHANNEL (Prime/Hash)
    LL->>MC: SetChannel
    M->>LL: MINER_READY
    LL->>BC: GetLatestBlock
    BC->>LL: BlockTemplate
    LL->>M: PRIME_BLOCK_AVAILABLE [12B]
    M->>LL: GET_BLOCK
    LL->>M: BLOCK_DATA [228B]
    Note over M: Mining...
    M->>LL: SUBMIT_BLOCK
    LL->>BC: ValidateBlock
    BC->>MM: AddToMempool
    BC->>LL: ACCEPTED
    LL->>M: BLOCK_ACCEPTED
    BC->>LL: NewBlockEvent
    LL->>M: PRIME_BLOCK_AVAILABLE [12B]
```

---

## Mining Thread Model

```mermaid
flowchart TD
    MS[Mining Server<br/>LLP::Server] --> TP[Thread Pool<br/>default: 4 workers]
    MS --> CM[Connection Manager]
    MS --> SC[Session Cache<br/>24h default]
    MS --> TG[Template Generator]
    MS --> PNB[Push Notification<br/>Broadcaster]

    TP --> W1[Worker Thread 1<br/>Packet Processing]
    TP --> W2[Worker Thread 2<br/>Block Validation]
    TP --> W3[Worker Thread 3<br/>Response Handling]
    TP --> W4[Worker Thread N<br/>...]

    CM --> PC[Per-Connection State]
    PC --> PP[Packet Parser]
    PC --> AH[Auth Handler]
    PC --> TD[Template Delivery]
    PC --> BV[Block Validation]
    PC --> CE[ChaCha20 Encryption]
```

---

## Mining Channel Selection

```mermaid
flowchart LR
    subgraph "Channel Types"
        CH0[Channel 0<br/>Proof of Stake]
        CH1[Channel 1<br/>Prime Mining]
        CH2[Channel 2<br/>Hash Mining]
    end

    M[Miner] -->|SET_CHANNEL| CH1
    M -->|SET_CHANNEL| CH2
    Note1[PoS channel is<br/>staking only] -.-> CH0
```

---

## Push Notification Payload (12 bytes)

Both Legacy and Stateless lanes use the same format:

```
Offset   Field              Type        Description
──────   ─────              ────        ───────────
[0-3]    nUnifiedHeight     uint32 BE   Blockchain height
[4-7]    nChannelHeight     uint32 BE   Channel-specific height
[8-11]   nBits              uint32 BE   Difficulty target
```

---

## Cross-References

- [Push Notification Flow](../push-notification-flow.md)
- [Mining Protocol](../../protocol/mining-protocol.md)
- [Opcodes Reference](../../reference/opcodes-reference.md)
- [Mining Server](../../current/mining/mining-server.md)
