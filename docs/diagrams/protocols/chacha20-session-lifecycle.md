# ChaCha20 Session Lifecycle

Diagrams showing the ChaCha20 encryption lifecycle used in Falcon public key exchange and session security.

---

## ChaCha20 Encryption Flow

```mermaid
sequenceDiagram
    participant M as Miner
    participant N as Node

    Note over M,N: Key Exchange Phase
    M->>M: Generate Falcon key pair
    M->>M: Encrypt Falcon PubKey<br/>with ChaCha20 shared secret

    M->>N: MINER_AUTH<br/>[Encrypted Falcon PubKey]

    N->>N: Derive ChaCha20 key<br/>from shared secret
    N->>N: Decrypt Falcon PubKey
    N->>N: Store PubKey in session

    N->>M: AUTH_RESULT

    Note over M,N: Encrypted Session Active
    Note over M,N: All subsequent packets<br/>use session encryption
```

---

## Session Encryption Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Plaintext: Connection opened

    Plaintext --> KeyExchange: MINER_AUTH sent
    KeyExchange --> Encrypted: ChaCha20 key derived
    Encrypted --> Active: AUTH_RESULT received

    Active --> Active: Encrypted packets
    Active --> Rekey: Session rotation
    Rekey --> Active: New key established

    Active --> Expired: Timeout (24h-7d)
    Expired --> [*]: Session purged

    state Active {
        [*] --> SendEncrypted
        SendEncrypted --> ReceiveDecrypt
        ReceiveDecrypt --> SendEncrypted
    }
```

---

## Encryption Requirements by Mode

```mermaid
flowchart TD
    CONN[New Connection] --> TYPE{Connection<br/>Type?}

    TYPE -->|Localhost| LOCAL[Local Mining]
    TYPE -->|Remote| REMOTE[Public Mining]

    LOCAL --> LOCALENC{ChaCha20?}
    LOCALENC -->|Optional| PLAIN[Plaintext Allowed]
    LOCALENC -->|Enabled| ENC1[ChaCha20 Encrypted]

    REMOTE --> REMENC[Mandatory Encryption]
    REMENC --> TLS[TLS 1.3]
    REMENC --> CHACHA[ChaCha20-Poly1305-SHA256]

    PLAIN --> SESSION[Session Established]
    ENC1 --> SESSION
    TLS --> SESSION
    CHACHA --> SESSION
```

---

## ChaCha20 Configuration

```bash
# Require encryption for all miners
# Default: false for localhost, true for remote
-falconhandshake.requireencryption=1

# Cipher suite for public connections
# TLS 1.3 with ChaCha20-Poly1305-SHA256
```

---

## Cross-References

- [Falcon Auth Sequence](falcon-auth-sequence.md)
- [LLP Packet Anatomy](llp-packet-anatomy.md)
- [Falcon Verification](../../current/authentication/falcon-verification.md)
