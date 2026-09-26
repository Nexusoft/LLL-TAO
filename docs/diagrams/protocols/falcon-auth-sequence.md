# Falcon Authentication Sequence

Full Falcon handshake diagram showing the post-quantum authentication flow between miners and nodes.

---

## Complete Falcon Handshake

```mermaid
sequenceDiagram
    participant M as Miner
    participant N as Node
    participant SC as Session Cache
    participant BC as Blockchain

    Note over M: Generate Falcon-512/1024<br/>key pair (disposable)

    M->>N: MINER_AUTH
    Note right of M: Falcon public key<br/>(optionally ChaCha20 encrypted)<br/>+ Tritium GenesisHash<br/>+ Timestamp + MinerID

    N->>N: Validate Timestamp<br/>(replay protection)

    alt ChaCha20 Encrypted
        N->>N: Decrypt public key<br/>with shared secret
    end

    N->>BC: Verify GenesisHash<br/>ownership
    BC->>N: Ownership confirmed

    N->>SC: Create Session Entry
    Note over SC: Store: SessionID,<br/>Falcon PubKey,<br/>GenesisHash, Timestamp

    N->>M: AUTH_RESULT (success)

    Note over M,N: Session established<br/>24h-7d lifetime
```

---

## Dual Signature Architecture

```mermaid
flowchart TD
    subgraph "Disposable Signature (Session Auth)"
        DS[Falcon-512: 809 bytes<br/>Falcon-1024: 1577 bytes]
        DS --> USAGE1[Used in: MINER_AUTH]
        DS --> USAGE2[Used in: SUBMIT_BLOCK]
        DS --> LIFE[Lifetime: 24h - 7d]
        DS --> ROT[Rotatable / Revocable]
    end

    subgraph "Physical Signature (Blockchain Proof)"
        PS[Optional: physicalsigner=1]
        PS --> PERM[Permanent blockchain record]
        PS --> AFTER[Follows Disposable in SUBMIT_BLOCK]
        PS --> CHAIN[Links to on-chain identity]
    end

    SUBMIT[SUBMIT_BLOCK Payload] --> DS
    SUBMIT --> PS
```

---

## Session Cache Management

```mermaid
flowchart TD
    NEW[New Auth Request] --> CACHE{Cache Full?<br/>500 entries max}
    CACHE -->|No| ADD[Add to Cache]
    CACHE -->|Yes| DDOS[DDOS Protection<br/>Reject connection]

    ADD --> ACTIVE[Active Session]
    ACTIVE --> PING{Keep-Alive Ping<br/>every 24h?}
    PING -->|Yes| ACTIVE
    PING -->|No| CHECK{Connection Type?}
    CHECK -->|Remote| PURGE7[Purge after 7 days]
    CHECK -->|Localhost| PURGE30[Purge after 30 days]
```

---

## Authentication Modes

```mermaid
flowchart LR
    subgraph "Private Solo Mining (Localhost)"
        L1[Simplified validation]
        L2[Extended cache timeout: 30 days]
        L3[ChaCha20 optional<br/>plaintext allowed]
        L4[Direct reward payout]
    end

    subgraph "Public Pool Mining (Internet)"
        R1[Mandatory TLS 1.3<br/>ChaCha20-Poly1305-SHA256]
        R2[Required Falcon encryption]
        R3[GenesisHash validation]
        R4[Session auth with auto-purge]
    end
```

---

## Cross-References

- [ChaCha20 Session Lifecycle](chacha20-session-lifecycle.md)
- [Mining Flow](../architecture/mining-flow-complete.md)
- [Falcon Verification](../../current/authentication/falcon-verification.md)
- Source: `src/LLP/include/falcon_handshake.h`
