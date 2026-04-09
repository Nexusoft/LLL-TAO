# Session Architecture — Option 4: Canonical Store + Materialized Views

## Overview

This document describes the **unified session architecture** implemented after the Option 4 redesign (PRs #529–#532). The architecture replaces the original 9 independent state stores with a single canonical store plus thin secondary indexes.

---

## Problem: 9 Stores, No Consistency Protocol

The original mining session management used **9 independent state stores**, each with its own lock, cleanup interval, and update logic:

```
┌─────────────────────────────────────────────────────────────────────┐
│  BEFORE: 9 Independent Stores (No Cross-Store Transactions)        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  StatelessMinerConnection::mapSessionKeys    [per-connection]       │
│  StatelessMinerManager::mapMiners            [address → context]    │
│  StatelessMinerManager::mapKeyToAddress       [hashKeyID → addr]    │
│  StatelessMinerManager::mapSessionToAddress   [sessionId → addr]    │
│  NodeSessionRegistry::m_mapByKey             [hashKeyID → entry]    │
│  NodeSessionRegistry::m_mapSessionToKey      [sessionId → keyID]   │
│  ActiveSessionBoard::m_mapSessions           [(sessionId,lane)]    │
│  SessionRecoveryManager::mapSessionsByKey    [hashKeyID → data]    │
│  SessionRecoveryManager::mapAddressToKey      [addr → hashKeyID]   │
│                                                                     │
│  Issues:                                                            │
│  • Each store updated independently with different locks            │
│  • Cleanup at different intervals (10m / 24h / 7d)                  │
│  • No transactional multi-store updates                             │
│  • TOCTOU windows between sequential map operations                 │
│  • Undefined lock ordering → deadlock risk                          │
│  • ActiveSessionBoard::Register() never called (dead code)          │
│  • Recovery state duplicated across stores                          │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Solution: Single Canonical Store

```
┌─────────────────────────────────────────────────────────────────────┐
│  AFTER: Option 4 — Canonical Store + Materialized Views            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│                    ┌─────────────────────────┐                      │
│                    │     SessionStore         │                      │
│                    │    (Singleton)           │                      │
│                    ├─────────────────────────┤                      │
│   PRIMARY MAP:     │  mapSessions             │                      │
│   (Source of       │  hashKeyID →             │                      │
│    Truth)          │    CanonicalSession       │                      │
│                    ├─────────────────────────┤                      │
│   SECONDARY        │  mapSessionIdToKey       │                      │
│   INDEXES:         │  mapAddressToKey          │                      │
│   (Materialized    │  mapGenesisToKey          │                      │
│    Views)          │  mapIPToKey               │                      │
│                    └──────────┬──────────────┘                      │
│                               │                                     │
│   ┌───────────────────────────┼──────────────────────────┐          │
│   │         Dual-Write Callers (Legacy Compatibility)    │          │
│   │                                                       │          │
│   │  StatelessMinerManager    NodeSessionRegistry         │          │
│   │  (active cache,           (identity + liveness,       │          │
│   │   connection-keyed)        key-keyed)                 │          │
│   └───────────────────────────────────────────────────────┘          │
│                                                                     │
│  REMOVED:                                                           │
│  ✗ ActiveSessionBoard        (health tracking → CanonicalSession)   │
│  ✗ SessionRecoveryManager    (full re-auth on disconnect)           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Components

### 1. CanonicalSession (`src/LLP/include/canonical_session.h`)

The **single record type** for a mining identity. One Falcon key → one `CanonicalSession`.

Merges fields from the former:
- `MiningContext` (identity, auth, channel, mining state)
- `NodeSessionEntry` (liveness flags per lane)
- `ActiveSessionEntry` (health: failed packets, disconnected flag)

```
CanonicalSession
├── Identity:      hashKeyID, nSessionId, hashGenesis, strUserName
├── Connection:    strAddress, strIP, nProtocolLane, fStatelessLive, fLegacyLive
├── Lifecycle:     nSessionState, fAuthenticated, nSessionStart, nLastActivity
├── Mining:        nChannel, nHeight, hashLastBlock, canonical_snap
├── Auth/Crypto:   vMinerPubKey, vChaChaKey, nFalconVersion, vDisposablePubKey
├── Reward:        hashRewardAddress, fRewardBound
├── Notifications: fSubscribedToNotifications, nSubscribedChannel
├── Keepalive:     nKeepaliveCount, nLastKeepaliveTime
├── Health:        nFailedPackets, nLastSuccessfulSend, fMarkedDisconnected
└── Protocol:      nProtocolVersion
```

**Key Methods:**
- `ToMiningContext()` — backward-compatible projection
- `FromMiningContext()` — construct from legacy type
- `GetSessionBinding()` / `GetCryptoContext()` — typed projections
- `IsExpired()` / `IsConsideredInactive()` / `AnyPortLive()` — queries

### 2. SessionStore (`src/LLP/include/session_store.h`)

The **unified session store** — singleton with a `ConcurrentHashMap<uint256_t, CanonicalSession>` primary map and 4 thin reverse indexes.

```
SessionStore (Singleton)
│
├── Register(session)           — insert/upsert + update all indexes
├── Remove(hashKeyID)           — remove + clean all indexes
│
├── LookupByKey(hashKeyID)      — O(1) primary lookup
├── LookupBySessionId(id)       — index hop: sessionId → keyID → session
├── LookupByAddress(addr)       — index hop: address → keyID → session
├── LookupByGenesis(genesis)    — index hop: genesis → keyID → session
├── LookupByIP(ip)              — index hop: IP → keyID → session
│
├── Transform(hashKeyID, fn)    — atomic read-modify-write
├── TransformBySessionId(id, fn)— index hop + transform
├── TransformByAddress(addr, fn)— index hop + transform
│
├── SweepExpired(timeout, now)  — remove expired sessions
│
├── RecordSendSuccess(keyID)    — reset failure counter
├── RecordSendFailure(keyID, n) — increment; mark disconnected at threshold
├── MarkDisconnected(keyID)     — manual disconnect flag
├── IsActive(keyID)             — !disconnected && AnyPortLive()
├── GetActiveSessionIdsForChannel(ch, lane) — push-notification query
│
└── Clear()                     — remove all (testing only)
```

### 3. StatelessMinerManager (`src/LLP/include/stateless_manager.h`)

**Active mining cache** — remains as the connection-keyed store for fast lookup by network address. Dual-writes to `SessionStore` on `UpdateMiner()` and `TransformMiner()`.

### 4. NodeSessionRegistry (`src/LLP/include/node_session_registry.h`)

**Canonical identity registry** — remains as the hashKeyID-keyed identity store with per-lane liveness tracking. Dual-writes to `SessionStore` on `RegisterOrRefresh()`.

---

## Data Flow

### Authentication Flow

```
Miner connects
    │
    ▼
MINER_AUTH_INIT  ──────────────────────────────────►  Generate nonce
    │                                                      │
    ▼                                                      ▼
MINER_AUTH_RESPONSE  ◄────────────────────────────  Verify Falcon signature
    │
    ├── Store 1: StatelessMinerConnection::mapSessionKeys[sessionId] = pubkey
    │
    ├── Store 5-6: NodeSessionRegistry::RegisterOrRefresh(hashKeyID, ...)
    │       └── Dual-write → SessionStore::Register(CanonicalSession)
    │
    └── Store 2-4: StatelessMinerManager::UpdateMiner(context)
            ├── mapMiners[address] = context
            ├── mapKeyToAddress[hashKeyID] = address
            ├── mapSessionToAddress[sessionId] = address
            └── Dual-write → SessionStore::Register(CanonicalSession)
```

### Push Notification Flow

```
New block found
    │
    ▼
MinerPushDispatcher::DispatchStatelessPush()
    │
    ▼
Server::NotifyChannelMiners(nChannel)
    │
    ▼
GetConnections()  ───►  For each live TCP connection:
    │                       │
    │                       ├── GetContext().fSubscribedToNotifications?
    │                       ├── GetContext().nSubscribedChannel == nChannel?
    │                       └── SendChannelNotification()
    │
    ▼
SessionStore provides:
    • GetActiveSessionIdsForChannel() — future use for index-based push
    • RecordSendSuccess/Failure() — health tracking per session
```

### Cleanup Flow

```
server.cpp Meter() — every CLEANUP_SWEEP_INTERVAL_SEC (configurable, default 600s)
    │
    ├── 1. AutoCooldownManager::Get().CleanupExpired()
    │       └── Rate limiting cooldown cleanup
    │
    ├── 2. NodeSessionRegistry::Get().SweepExpired(86400s)
    │       └── Remove sessions with no live ports for 24h
    │
    ├── 3. StatelessMinerManager::Get().CleanupInactive(86400s)
    │       └── Remove inactive miners via RemoveMiner() cascade
    │
    ├── 4. StatelessMinerManager::Get().PurgeInactiveMiners()
    │       └── Additional cache hygiene
    │
    └── 5. SessionStore::Get().SweepExpired(86400s)
            └── Remove expired sessions from canonical store + indexes
```

---

## Architectural Decisions

### Why Dual-Write Instead of Full Migration?

The legacy stores (`StatelessMinerManager`, `NodeSessionRegistry`) are deeply integrated with the connection lifecycle, packet processing, and template management. Full migration would require rewriting the entire mining connection layer.

**Current approach:** Dual-write ensures `SessionStore` stays in sync while legacy stores handle their existing responsibilities. This allows incremental migration: callers can be moved to `SessionStore` one at a time.

### Why hashKeyID as Canonical Key?

| Key Type | Pros | Cons |
|----------|------|------|
| **hashKeyID** (Falcon key hash) | Stable across reconnects, 1:1 with miner identity | Requires key authentication first |
| sessionId (lower 32-bit of key hash) | Fits wire protocol | 32-bit collision risk |
| address (IP:port) | Available pre-auth | Changes on NAT/reconnect |

`hashKeyID` is the only key that is **stable** (doesn't change on reconnect), **unique** (full 256-bit hash), and **identity-bound** (tied to Falcon key ownership).

### Why Remove SessionRecoveryManager?

Session recovery added complexity (7-day state retention, reconnect counters, cross-store propagation) for a use case (stateless reconnection) that conflicts with the security model. With full re-auth on disconnect:

- **Simpler**: No recovery state to maintain
- **Safer**: No stale crypto context reuse
- **Fewer stores**: 3 stores instead of 5 (primary) + 4 (indexes)

### Why Remove ActiveSessionBoard?

ActiveSessionBoard's `Register()` method was never called — the push path used TCP connection iteration, not the board. Its health tracking fields (`nFailedPackets`, `fMarkedDisconnected`) are now inline in `CanonicalSession`.

---

## Thread Safety

| Component | Lock Type | Granularity |
|-----------|-----------|-------------|
| `SessionStore::mapSessions` | `ConcurrentHashMap` (shared_mutex) | Per-map (single lock) |
| `SessionStore::mapSessionIdToKey` | `ConcurrentHashMap` (shared_mutex) | Per-map |
| `SessionStore::mapAddressToKey` | `ConcurrentHashMap` (shared_mutex) | Per-map |
| `SessionStore::mapGenesisToKey` | `ConcurrentHashMap` (shared_mutex) | Per-map |
| `SessionStore::mapIPToKey` | `ConcurrentHashMap` (shared_mutex) | Per-map |

**Atomic mutations** use `ConcurrentHashMap::Transform()` for single-map read-modify-write atomicity. Cross-map index updates happen after the primary `Transform()` completes — they are **eventually consistent** (brief window where indexes may lag the primary map).

---

## Test Coverage

Unit tests in `tests/unit/LLP/session_store_tests.cpp` cover:

| Category | Tests |
|----------|-------|
| CanonicalSession construction | Default constructor, parameterized constructor, DeriveIP |
| Identity queries | AnyPortLive, IsExpired, IsConsideredInactive |
| Projections | ToMiningContext, FromMiningContext, GetSessionBinding, GetCryptoContext |
| Registration | Register new, register update, re-register with address change |
| Index lookups | ByKey, BySessionId, ByAddress, ByGenesis, ByIP, missing returns nullopt |
| Remove | Remove cleans all indexes, remove non-existent returns false |
| Transform | Atomic mutation, index update on address change, TransformBySessionId, TransformByAddress |
| Aggregation | Count, CountLive, CountAuthenticated, GetAll, GetAllForChannel, ForEach |
| SweepExpired | Expired removal, index cleanup on sweep |
| Health tracking | RecordSendSuccess, RecordSendFailure threshold, MarkDisconnected, IsActive |
| Channel filtering | GetActiveSessionIdsForChannel with subscribed/unsubscribed/disconnected filters |
| Concurrency | Concurrent register, concurrent transform stress (100 threads) |

---

## File Reference

| File | Purpose |
|------|---------|
| `src/LLP/include/canonical_session.h` | CanonicalSession struct definition |
| `src/LLP/include/session_store.h` | SessionStore class declaration |
| `src/LLP/session_store.cpp` | SessionStore + CanonicalSession implementation |
| `src/LLP/include/node_session_registry.h` | NodeSessionRegistry (identity + liveness) |
| `src/LLP/node_session_registry.cpp` | NodeSessionRegistry implementation |
| `src/LLP/include/stateless_manager.h` | StatelessMinerManager (active cache) |
| `src/LLP/stateless_manager.cpp` | StatelessMinerManager implementation |
| `src/LLP/include/mining_timers.h` | Centralized timer constants |
| `tests/unit/LLP/session_store_tests.cpp` | Unit tests for SessionStore + CanonicalSession |
