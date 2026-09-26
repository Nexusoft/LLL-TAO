# [ARCHIVED] Failover Re-Authentication Flow

> **⚠️ ARCHIVED:** This document references `SessionRecoveryManager`,
> `SessionRecoveryData`, and `fFreshAuth` — all of which were **removed** in
> PRs #530–#532.  Session recovery is no longer supported; miners perform a
> full Falcon re-authentication on reconnect.  Retained for historical context.
>
> **Current documentation:** See [Recent Changes Summary](../current/mining/recent-changes-summary.md)
> and [Known Risks and Limitations](../current/mining/known-risks-and-limitations.md).

## Overview

When a NexusMiner client switches from its primary mining node to a failover node
(e.g., `primary:9323` → `failover:9323`), the failover node has **no knowledge of
the miner's previous session**. The `SessionRecoveryManager` on the failover node
contains no entry for this miner's key ID or IP address. Therefore, the miner must
perform a **complete fresh Falcon authentication handshake** from scratch.

This document describes the full flow and how the node detects and reports it.

---

## Why a Full Re-Authentication Is Required

The node-side `SessionRecoveryManager` stores sessions keyed by `hashKeyID`
(the lower 32 bits of the Falcon public key hash). When a miner connects to a
**different node** (failover), that node's session store is empty for this miner:

- `RecoverSessionByAddress()` → returns `std::nullopt`
- `RecoverSessionByKeyId()` → returns `std::nullopt`

No session can be resumed. The miner's `MiningContext` starts fresh:
- `fAuthenticated = false`
- `nSessionId = 0`
- No ChaCha20 key

The miner **must** re-authenticate before it can receive block templates.

---

## Step-by-Step Failover Flow

```
NexusMiner (client)              Failover Node (server)
────────────────────             ──────────────────────

retry_connect() → failover IP

TCP connect ─────────────────────► StatelessMinerConnection::Event(CONNECT)
                                       RecoverSessionByAddress() → nullopt
                                       FailoverConnectionTracker::RecordConnection(IP)
                                       [logged: "Potential failover connection"]

MINER_AUTH_INIT ─────────────────► ProcessMinerAuthInit()
                                       Generates MINER_AUTH_CHALLENGE nonce

◄──────────────── MINER_AUTH_CHALLENGE

MINER_AUTH_RESPONSE ─────────────► ProcessFalconResponse()
(signed challenge + pubkey)            Verifies Falcon signature
                                       hashKeyID = lower 32 bits of pubkey hash
                                       nSessionId = static_cast<uint32_t>(hashKeyID.Get64(0))
                                       fAuthenticated = true

                                       if FailoverConnectionTracker::IsFailover(IP):
                                           ChannelStateManager::NotifyFailoverConnection()
                                           FailoverConnectionTracker::ClearConnection(IP)

◄──────────────── MINER_AUTH_RESULT (success + nSessionId)

                                       ChaCha20 key derived from hashGenesis
                                       SessionRecoveryManager::SaveSession() (fresh session)

STATELESS_MINER_READY ───────────► Subscribed to channel notifications
                                       Templates pushed on new blocks

[Mining resumes on failover node]
```

---

## Node-Side Detection Components

### `FailoverConnectionTracker` (singleton)

Located in `src/LLP/include/failover_connection_tracker.h` and
`src/LLP/failover_connection_tracker.cpp`.

- **`RecordConnection(strAddr)`** — Called in `Event(EVENTS::CONNECT)` when
  `RecoverSessionByAddress()` returns no session for a non-localhost IP.
- **`IsFailover(strAddr)`** — Returns `true` if this IP has a pending failover flag.
  Checked after `MINER_AUTH_RESPONSE` succeeds.
- **`ClearConnection(strAddr)`** — Clears the flag after successful authentication.
- **`GetTotalFailoverCount()`** — Monotonically increasing counter of all failover
  auth events since process start.

### `ChannelStateManager::NotifyFailoverConnection()`

Located in `src/LLP/channel_state_manager.cpp`.

Increments an internal `m_nFailoverConnections` atomic counter and logs the event
with the miner's `hashKeyID` and IP address. Used for operational visibility in the
Colin mining agent report.

---

## Colin Mining Agent Report

The Colin report now shows a **Miner Session State** section replacing the
previous DualConnectionManager-based vague tracking:

```
╠════════════════════════════════════════════════════════════════════╣
║  MINER SESSION STATE:                                              ║
║    Session 0xABCD1234 | addr=192.168.1.10 | Falcon auth OK |      ║
║      channel=Prime (1) | fresh_auth=false                         ║
║    Failover connections since boot: 3                              ║
╠════════════════════════════════════════════════════════════════════╣
```

- **`Session 0xXXXX`** — The `nSessionId` derived from the miner's Falcon pubkey hash.
- **`addr=...`** — The miner's IP address.
- **`Falcon auth OK`** / **`unauthenticated`** — Authentication status.
- **`channel=Prime/Hash (N)`** — Mining channel.
- **`fresh_auth=true/false`** — Whether this session arrived via failover
  (no prior session on this node) vs. normal connection/recovery.
- **`Failover connections since boot: N`** — Total failover auth events.

---

## Design Constraints

### No Session Resumption Across Nodes

Session IDs are **node-local**. The `SessionRecoveryManager` on each node only
stores sessions that were authenticated on that specific node. When a miner
connects to a failover node, it cannot resume its old session — it must authenticate
fresh. This is a fundamental security property: session keys are not synchronized
between nodes.

### Existing Authentication Path Is Unchanged

The `ProcessMinerAuthInit()` → `ProcessFalconResponse()` path in `stateless_miner.cpp`
already handles fresh authentication correctly. The new failover detection code adds
only **observation and reporting** around this existing path — no behavioral changes.

### Localhost Connections Are Excluded

Connections from `127.0.0.1` or `::1` are not recorded as potential failovers since
they are typically local test or management connections, not miner failovers.

---

## `SessionRecoveryData::fFreshAuth` Flag

The `SessionRecoveryData` struct (in `session_recovery.h`) includes a `fFreshAuth`
boolean field. When `SessionRecoveryManager::SaveSession()` is called after a
fresh Falcon handshake on a failover node, this flag can be set to `true` to
preserve the information that this was a fresh (not recovered) authentication.
The flag defaults to `false` for normal recovered sessions.
