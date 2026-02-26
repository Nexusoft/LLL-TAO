# Unified Keepalive Protocol — NexusMiner + LLL-TAO

**Status:** Current (as of 2026-02-26)  
**PRs:** LLL-TAO #299–302, NexusMiner #214–216  
**The Missing Piece Fixed:** `stake_height` tracking (was always 0 on stateless path before PR #299)

---

## Overview

The Nexus miner maintains session state with the node via periodic keepalive exchanges.
Before unification, the legacy port (8323) and stateless port (9323) sent **different packet formats**,
causing the miner's `HeightTracker` to have incomplete state — most critically, `stake_height` was
never populated on the stateless path.

After unification, **both ports send the same 32-byte packet** parsed by the same struct,
feeding the same `HeightTracker::OnKeepaliveResponse()` method, making `HeightTracker` the
single source of truth for all chain heights including stake.

---

## Diagram 1 — Unified Wire Format

```
┌─────────────────────────────────────────────────────────────────────────┐
│             UNIFIED KEEPALIVE ACK  (32 bytes, both ports)               │
├──────────┬────────────────────┬────────────┬───────────────────────────┤
│ Bytes    │ Field              │ Encoding   │ Notes                     │
├──────────┼────────────────────┼────────────┼───────────────────────────┤
│ [0-3]    │ session_id         │ LE uint32  │ Session validation        │
│ [4-7]    │ hashPrevBlock_lo32 │ BE uint32  │ Echo of miner fork canary │
│          │                    │            │ (0 on legacy path)        │
│ [8-11]   │ unified_height     │ BE uint32  │ Node's chain height       │
│ [12-15]  │ hash_tip_lo32      │ BE uint32  │ lo32 of hashBestChain     │
│          │                    │            │ (fork cross-check)        │
│ [16-19]  │ prime_height       │ BE uint32  │ Prime channel height      │
│ [20-23]  │ hash_height        │ BE uint32  │ Hash channel height       │
│ [24-27]  │ stake_height       │ BE uint32  │ ★ THE MISSING PIECE       │
│          │                    │            │ was ALWAYS 0 before PR#299│
│ [28-31]  │ fork_score         │ BE uint32  │ 0=healthy, >0=divergence  │
│          │                    │            │ (0 on legacy path)        │
└──────────┴────────────────────┴────────────┴───────────────────────────┘

  Note: nBits NOT included — miner reads difficulty from:
    1. 228-byte template push (CreateBlockForStatelessMining bakes nBits in)
    2. 12-byte GET_BLOCK / GET_ROUND response
```

---

## Diagram 2 — Node-Side Keepalive Flow (both ports)

```mermaid
flowchart TD
    A([Miner sends keepalive]) --> B{Which port?}

    B -->|Port 8323 legacy SESSION_KEEPALIVE| C[ProcessSessionKeepalive]
    B -->|Port 9323 stateless KEEPALIVE_V2| D[ProcessKeepaliveV2]

    C --> E[Load ChainState::tStateBest]
    D --> F[Parse 8-byte KeepAliveV2Frame\nsequence + hashPrevBlock_lo32]
    F --> E

    E --> G[GetLastState channel 1 → prime_height]
    G --> H[GetLastState channel 2 → hash_height]
    H --> I[GetLastState channel 0 → stake_height ★]
    I --> J[hashBestChain → hash_tip_lo32]

    J --> K{Which path?}
    K -->|Legacy| L[hashPrevBlock_lo32 = 0\nfork_score = 0]
    K -->|Stateless| M[hashPrevBlock_lo32 = echo from frame\nfork_score = hash_tip_lo32 != frame.hashPrevBlock_lo32 ? 1 : 0]

    L --> N[BuildUnifiedResponse\nsession_id, 0, unified, hash_tip_lo32,\nprime, hash, stake, 0]
    M --> O[BuildUnifiedResponse\nsession_id, hashPrevBlock_lo32,\nunified, hash_tip_lo32,\nprime, hash, stake, fork_score]

    N --> P([Send 32-byte response])
    O --> P
```

---

## Diagram 3 — Miner-Side HeightTracker Data Flow

```mermaid
flowchart LR
    subgraph NODE ["LLL-TAO Node"]
        direction TB
        CS[ChainState::tStateBest]
        BA[BLOCK_AVAILABLE push\n228-byte template]
        KA[Keepalive ACK\n32-byte unified]
    end

    subgraph MINER ["NexusMiner"]
        direction TB
        subgraph HT ["HeightTracker (single source of truth)"]
            UH[unified_height]
            PH[prime_height]
            HH[hash_height]
            SH[stake_height ★]
            CH[channel_height]
            HTL[hash_tip_lo32]
            FS[fork_score]
            PFS[peak_fork_score\ncanary]
            HPB[hashPrevBlock\nfull 1024-bit]
        end

        subgraph COLIN ["ColinAgent (reads snapshot)"]
            CR[emit_report:\nunified prime hash stake\nfork canary warning]
        end

        subgraph SOLO ["Solo"]
            SM[m_last_keepalive_prevhash_lo32]
            FD[IsForkDetected:\nhash_tip_lo32 != myPrevHash\nOR fork_score > 0]
        end
    end

    BA -->|OnTemplateReceived\nchannel + channel_height| HT
    BA -->|UpdateWithHashPrevBlock\nfull hash| HT
    KA -->|KeepAliveV2AckFrame::Parse\nOnKeepaliveResponse| HT
    HT -->|GetSnapshot| COLIN
    HT -->|GetSnapshot| SOLO
    SM -->|myHashPrevBlock_lo32| FD
    HTL -->|snap.hash_tip_lo32| FD
    FS -->|snap.fork_score| FD
```

---

## Diagram 4 — ColinAgent Diagnostic Report Structure

```mermaid
flowchart TD
    A([ColinAgent::run_diagnostics\nevery 60s]) --> B[Collect stats::Global]
    B --> C[Build warnings list\nretries / degraded mode / template age]
    C --> D[emit_report]

    D --> E[Print timestamp header]
    E --> F[assess_primary_lane\nstateless:9323 HEALTHY/DEGRADED/DOWN]
    F --> G[assess_secondary_lane\nlegacy:8323 HEALTHY/DEGRADED/DOWN]
    G --> H[Blocks: accepted / rejected / retries]

    H --> I{m_height_tracker set?}
    I -->|YES| J[GetSnapshot\nlog unified prime hash stake channel_height]
    I -->|NO| K[skip height section]

    J --> L{peak_fork_score > 0?}
    L -->|YES| M[WARN: FORK CANARY active\npeak_fork_score + current fork_score]
    L -->|NO| N[continue]
    M --> N
    K --> N

    N --> O{m_ping_source set?}
    O -->|YES| P[Log PING_DIAG section:\nseq, prime_pushes, hash_pushes,\nblocks_submitted/accepted/rejected,\nhealth_flags decoded]
    O -->|NO| Q[skip ping section]

    P --> R[Print footer separator]
    Q --> R
    R --> S([schedule_next after interval_s])
```

---

## Diagram 5 — The Missing Piece: Stake Height Tracking Timeline

```
TIME ──────────────────────────────────────────────────────────────────────►

BEFORE PR #299–302 (broken state):
  Legacy port (8323):
    Session keepalive → BuildBestCurrentResponse (28b) → stake_height ✅ present
    But: miner parsed into KeepaliveTelemetrySnapshot (not HeightTracker!)
    Result: HeightTracker::stake_height ALWAYS = 0 ❌

  Stateless port (9323):
    KEEPALIVE_V2_ACK → KeepAliveV2AckFrame (32b with sequence) → stake_height ✅ present
    But: OnKeepaliveAck() did NOT update stake_height (field not in old call signature)
    Result: HeightTracker::stake_height ALWAYS = 0 ❌

  Colin report:
    stake=0 always — useless for diagnostics ❌

═══════════════════════════════════════════════════════════════════════════════

AFTER PR #299–302 / #214–216 (fixed state):
  Both ports:
    Keepalive → BuildUnifiedResponse (32b, session_id LE) → stake_height ✅
    → KeepAliveV2AckFrame::Parse() → OnKeepaliveResponse()
    → HeightTracker::stake_height = actual stake height ✅

  Colin report:
    Heights │ unified=6001 prime=451 hash=801 stake=999  (channel_height=801) ✅

  Fork detection:
    IsForkDetected(myPrevHash_lo32):
      hash_tip_lo32 (from ACK) != myPrevHash_lo32  →  fork!
      OR fork_score (from ACK) > 0                 →  fork!

  Peak fork canary:
    peak_fork_score = max(all fork_scores seen since start)
    is_fork_active() = peak_fork_score > 0
    Once triggered, NEVER resets — persistent warning in Colin report ✅
```

---

## Component Reference Table

| Component | File | Role |
|-----------|------|------|
| `BuildUnifiedResponse()` | `src/LLP/include/keepalive_v2.h` | NODE: builds 32-byte response for both ports |
| `ProcessSessionKeepalive()` | `src/LLP/stateless_miner.cpp` | NODE: legacy port keepalive handler |
| `ProcessKeepaliveV2()` | `src/LLP/stateless_miner.cpp` | NODE: stateless port keepalive handler |
| `KeepAliveV2AckFrame` (NODE) | `src/LLP/include/keepalive_v2.h` | NODE: struct + Serialize() |
| `KeepAliveV2AckFrame` (MINER) | `src/LLP/include/colin_ping_protocol.h` | MINER: struct + Parse() + IsForkDetected() |
| `HeightTracker::OnKeepaliveResponse()` | `src/protocol/src/protocol/height_tracker.cpp` | MINER: unified update for all 6 fields |
| `HeightTracker::Snapshot` | `src/protocol/inc/protocol/height_tracker.hpp` | MINER: snapshot struct (unified_height, prime_height, hash_height, **stake_height**, hash_tip_lo32, fork_score, peak_fork_score) |
| `ColinAgent::emit_report()` | `src/colin_agent.cpp` | MINER: reads HeightTracker snapshot, logs all heights + fork canary |
| `Solo::process_messages()` | `src/protocol/src/protocol/solo.cpp` | MINER: routes both keepalive paths → OnKeepaliveResponse() |

---

## What Was Deleted

| Deleted Component | Reason |
|-------------------|--------|
| `BuildBestCurrentResponse()` | Replaced by `BuildUnifiedResponse()` |
| `sequence` field in stateless ACK | Replaced by `session_id` (LE, consistent with legacy) |
| `nBits` in keepalive response | Miner reads from template / GET_BLOCK |
| `hashBestChain_prefix` (4 raw bytes) | Replaced by `hash_tip_lo32` |
| `OnKeepaliveAck()` | Replaced by `OnKeepaliveResponse()` |
| `OnLegacyKeepalive()` | Replaced by `OnKeepaliveResponse()` |
| `UpdateSource::KEEPALIVE_ACK` | Replaced by `UpdateSource::KEEPALIVE` |
| `UpdateSource::LEGACY_KEEPALIVE` | Replaced by `UpdateSource::KEEPALIVE` |
| `KeepaliveTelemetrySnapshot` | Replaced by `HeightTracker::Snapshot` |
| `KeepaliveTelemetryStore` | Replaced by `HeightTracker` |
| `keepalive_telemetry.hpp` | Entire file deleted |
| `ForkScoreSource` callback on ColinAgent | Colin reads directly from HeightTracker |
| `m_last_keepalive_fork_score` on Solo | Replaced by `HeightTracker::Snapshot::fork_score` |
| `static_assert(PAYLOAD_SIZE != 28, ...)` | No longer needed |

---

## Version History

| Version | Date | Change |
|---------|------|--------|
| 1.0 | 2026-02-26 | Initial unified protocol document (post PR #299–302, #214–216) |
