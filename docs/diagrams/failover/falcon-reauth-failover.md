# Falcon Re-Authentication on Failover

> **⚠️ Partial Update (2026-04-11):** `SessionRecoveryManager` and `fFreshAuth` were
> **removed** in PR #532.  On failover, miners perform a full fresh Falcon handshake
> (as shown below) — this behavior is unchanged.  The `MarkFreshAuth()` tracking
> call and `SessionRecoveryData` no longer exist.

Mermaid sequence diagram showing the Falcon post-quantum re-authentication flow when a miner fails over from the primary node to the failover node.

> **Critical**: Session IDs are **NOT portable** between nodes. Each node issues its own session ID and derives an independent ChaCha20-Poly1305 encryption key. On failover, the miner must perform a completely fresh `MINER_AUTH_INIT` handshake — no session ID reuse is permitted.

```mermaid
sequenceDiagram
    participant M  as NexusMiner
    participant PN as Primary_Node
    participant FN as Failover_Node

    Note over M,PN: Phase 1 — Successful authentication on primary node

    M->>PN: TCP connect (port 9323)
    PN-->>M: TCP ACK

    M->>PN: MINER_AUTH_INIT\n(Falcon-1024 public key, version)
    PN-->>M: MINER_AUTH_CHALLENGE\n(random 32-byte nonce)

    M->>PN: MINER_AUTH_RESPONSE\n(Falcon signature over nonce)
    PN-->>M: MINER_AUTH_RESULT (success)\n(session_id = X, chacha20_key derived from X)

    Note over M,PN: Mining proceeds — session_id=X, encrypted channel active

    M->>PN: GET_BLOCK (encrypted, session_id=X)
    PN-->>M: BLOCK_DATA (encrypted, session_id=X)

    Note over M,PN: Primary node becomes unreachable

    PN--xM: connection lost / timeout

    Note over M: retry_connect() fails N times\nDualConnectionManager.set_failover_active(true, failover_ip)\nSession X is ABANDONED — not sent to failover node

    Note over M,FN: Phase 2 — Fresh Falcon handshake on failover node\n(NO reuse of session_id=X or its ChaCha20 key)

    M->>FN: TCP connect (port 9323)
    FN-->>M: TCP ACK

    M->>FN: MINER_AUTH_INIT\n(Falcon-1024 public key, version)\n[fresh handshake — no session_id field]
    FN-->>M: MINER_AUTH_CHALLENGE\n(NEW random 32-byte nonce)

    M->>FN: MINER_AUTH_RESPONSE\n(Falcon signature over NEW nonce)
    FN-->>M: MINER_AUTH_RESULT (success)\n(session_id = Y, NEW chacha20_key derived from Y)

    Note over M,FN: Y ≠ X — completely independent session on failover node

    M->>FN: GET_BLOCK (encrypted, session_id=Y)
    FN-->>M: BLOCK_DATA (encrypted, session_id=Y)

    Note over M: DualConnectionManager.reset_fail_count()\nSIM Link re-established when LEGACY lane also connects
```

## Why Session IDs Are Not Portable

| Property | Explanation |
|----------|-------------|
| **Node-local nonce** | The `MINER_AUTH_CHALLENGE` nonce is generated fresh by each node. A signature valid for node A's nonce is cryptographically unrelated to node B's nonce. |
| **Key derivation** | ChaCha20-Poly1305 keys are derived from the session ID, which encodes the node-local challenge. A key derived on node A cannot decrypt node B's packets. |
| **Replay prevention** | Accepting a session ID from another node would break replay-attack prevention: a MITM could forward captured session traffic from the primary to the failover. |
| **Fresh session_id = Y** | After failover, the miner operates with `session_id=Y` exclusively. The old `session_id=X` is discarded and never transmitted to the failover node. |

## `fFreshAuth` Flag [REMOVED]

~~After a successful failover handshake, `SessionRecoveryManager::MarkFreshAuth(strAddress)` sets the `fFreshAuth` flag in `SessionRecoveryData`.~~

**Removed in PR #532:** `SessionRecoveryManager` and `fFreshAuth` no longer exist.  On failover, miners simply perform a full Falcon re-authentication handshake.  The CanonicalSession in SessionStore tracks session state without a separate recovery mechanism.
