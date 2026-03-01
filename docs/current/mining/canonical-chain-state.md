# CanonicalChainState — Chain Snapshot for Mining Templates

**Status:** Active  
**Applies to:** Stateless Block Utility, Colin Mining Agent  
**Header:** `src/LLP/include/canonical_chain_state.h`  
**Last Updated:** 2026-03-01

---

## Overview

`CanonicalChainState` is a lightweight, point-in-time snapshot of the node's
canonical blockchain state.  It captures the exact heights, difficulty, and
previous-block hash that the node considers authoritative **at the moment a
block template is created or pushed**.

The struct lives in `LLP::` alongside `TemplateMetadata`, `HeightInfo`, and
`ColinMiningAgent` — all of which participate in the template-serving pipeline.

---

## Purpose

| Consumer | How it uses the snapshot |
|---|---|
| **SendStatelessTemplate()** | Captures chain state at push time; logs staleness and drift diagnostics |
| **GET_BLOCK handler** | Captures chain state when building the 12-byte metadata prefix; detects drift |
| **Colin Mining Agent** | Can embed the snapshot in PingFrame diagnostics to report chain freshness |
| **Template validation** | `is_canonically_stale()` rejects snapshots older than 30 s |
| **Fork detection** | `height_drift_from_canonical()` reveals positive drift (advance) or negative drift (reorg) |

---

## Fields

```
┌──────────────────────────────────────────────────────────────┐
│                    CanonicalChainState                        │
├──────────────────────────────┬───────────────────────────────┤
│ canonical_unified_height     │ uint32_t   — all-channel tip  │
│ canonical_channel_height     │ uint32_t   — channel-specific │
│ canonical_difficulty_nbits   │ uint32_t   — compact nBits    │
│ canonical_channel_target     │ uint32_t   — channel target   │
│ canonical_hash_prev_block    │ uint1024_t — prev block hash  │
│ canonical_received_at        │ steady_clock::time_point      │
├──────────────────────────────┴───────────────────────────────┤
│ CANONICAL_STALE_SECONDS = 30                                 │
└──────────────────────────────────────────────────────────────┘
```

---

## Methods

### `is_initialized()`

Returns `true` when `canonical_unified_height != 0`.  A default-constructed
snapshot reports `false` because the genesis block has height 0 and is never
a valid chain tip in production.

### `is_canonically_stale()`

Returns `true` when either:
1. The snapshot was never populated (`!is_initialized()`), **or**
2. More than `CANONICAL_STALE_SECONDS` (30 s) have elapsed since `canonical_received_at`.

The 30 s threshold is short enough to catch stale chain views during normal
mining (block times ~50 s) but long enough to tolerate network jitter.

### `height_drift_from_canonical()`

```
drift = ChainState::tStateBest.nHeight − canonical_unified_height
```

| Drift | Meaning |
|-------|---------|
| `> 0` | Chain has advanced past the snapshot (new blocks arrived) |
| `= 0` | Chain is at the same height — snapshot is current |
| `< 0` | Chain has rolled back — fork / reorg detected |

### `from_chain_state()` (static factory)

```cpp
CanonicalChainState snap = CanonicalChainState::from_chain_state(
    stateBest, stateChannel, nDifficulty);
```

Builds a fully-populated snapshot from a `BlockState` pair and difficulty.
Sets `canonical_received_at` to `steady_clock::now()`.

---

## Integration Points

### SendStatelessTemplate() — Push Path

```
    stateBest = tStateBest.load()         ← atomic snapshot
    stateChannel = GetLastState(channel)  ← walk-back ~3 blocks
    nDifficulty = GetNextTargetRequired() ← channel retarget
    ┌──────────────────────────────────┐
    │ canonicalSnap = from_chain_state │  ← SNAPSHOT CAPTURED
    └──────────────────────────────────┘
    pBlock = new_block()                  ← template creation
    notification = 12-byte meta + 216-byte block
    respond(notification)                 ← push to miner
```

### GET_BLOCK handler — Pull Path

```
    pBlock = new_block()                  ← template creation
    vData = pBlock->Serialize()           ← 216-byte serialization
    stateChannelMeta = GetLastState()     ← channel state
    nBitsMeta = pBlock->nBits             ← difficulty
    ┌──────────────────────────────────┐
    │ canonicalSnap = from_chain_state │  ← SNAPSHOT CAPTURED
    └──────────────────────────────────┘
    vPayload = 12-byte meta + vData       ← response built
    respond(BLOCK_DATA, vPayload)         ← reply to miner
```

---

## Relationship to Other State Types

```
CanonicalChainState         TemplateMetadata           HeightInfo
────────────────────        ─────────────────          ───────────
Node chain snapshot         Per-template snapshot      Per-channel view
  at push/GET time            at creation time            from ChannelStateManager
                                                         
┌─ unified_height           ┌─ nHeight (unified)       ┌─ nUnifiedHeight
├─ channel_height           ├─ nChannelHeight          ├─ nChannelHeight
├─ difficulty_nbits         ├─ hashBestChainAtCreation ├─ nNextUnifiedHeight
├─ hash_prev_block          ├─ hashMerkleRoot          ├─ hashCurrentBlock
└─ received_at              └─ nCreationTime           └─ fForkDetected

Purpose:                    Purpose:                   Purpose:
  "What does the chain        "When was this template     "Full diagnostic view
   look like RIGHT NOW?"       born, and is it stale?"     of a channel's state"
```

---

## Thread Safety

`CanonicalChainState` is plain data — no internal mutex.  Callers sharing
a snapshot across threads must protect it externally, just as
`TemplateMetadata` is protected by the connection's `MUTEX`.

The `from_chain_state()` factory takes already-loaded `BlockState` objects
(atomic loads happen before the call).

---

## Example Usage

```cpp
#include <LLP/include/canonical_chain_state.h>

// Capture a snapshot
TAO::Ledger::BlockState stateBest = TAO::Ledger::ChainState::tStateBest.load();
TAO::Ledger::BlockState stateChannel = stateBest;
TAO::Ledger::GetLastState(stateChannel, nChannel);
uint32_t nDifficulty = TAO::Ledger::GetNextTargetRequired(stateBest, nChannel);

LLP::CanonicalChainState snap = LLP::CanonicalChainState::from_chain_state(
    stateBest, stateChannel, nDifficulty);

// Check freshness
if (snap.is_canonically_stale())
    RefreshSnapshot();

// Monitor drift
int32_t drift = snap.height_drift_from_canonical();
if (drift < 0)
    debug::warning("Chain rolled back ", -drift, " blocks since snapshot");
```
