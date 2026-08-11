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
| **SendStatelessTemplate()** | Captures chain state at push time; stores in `MiningContext`; logs staleness and drift diagnostics |
| **GET_BLOCK handler** | Captures chain state when building the 12-byte metadata prefix; stores in `MiningContext` |
| **SUBMIT_BLOCK pre-check gate** | Reads `context.canonical_snap` to cross-check template height and staleness (WARN-ONLY) |
| **Colin Mining Agent** | Reports `last_canonical_snap_age_ms` and emits `[WARN]` when snap is stale for active sessions |
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
    context = context.WithCanonicalSnap(canonicalSnap)  ← STORED
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
    context = context.WithCanonicalSnap(canonicalSnap)  ← STORED
    vPayload = 12-byte meta + vData       ← response built
    respond(BLOCK_DATA, vPayload)         ← reply to miner
```

---

## SUBMIT_BLOCK Pre-Check Gate

After ChaCha20 decryption and Disposable Falcon signature verification,
the SUBMIT_BLOCK handler performs a **canonical pre-check** using the
snapshot stored at GET_BLOCK / push time.

```
SUBMIT_BLOCK received
    │
    ├─ ChaCha20 decrypt
    │
    ├─ DisposableFalcon::VerifyWorkSubmission()  ← identity + integrity gate
    │
    ├─ find_block(hashMerkle)                    ← template must exist in mapBlocks
    │
    ├─ ── Canonical pre-check gate (WARN-ONLY) ──────────────────────────────
    │   context.canonical_snap.is_canonically_stale() → WARN if true
    │   template.nHeight != snap.canonical_unified_height → WARN if mismatch
    │   (node's validate_block() is the final authority — no rejection here)
    │
    ├─ sign_block(nNonce, hashMerkle)
    │
    ├─ ValidateMinedBlock(*pTritium)             ← node ledger is authoritative
    │
    └─ BLOCK_ACCEPTED / BLOCK_REJECTED
```

### Three-Tier Authority Model

1. **Canonical snapshot** (`context.canonical_snap`) — captures the chain state
   at the moment the template was issued.  Used for WARN-ONLY cross-checks:
   - Snapshot age > 30 s → miner may be working on an old template
   - Template height ≠ snapshot unified height → template may be from a different round

2. **DisposableFalcon signature** — proves miner identity and submission
   integrity.  The signature is verified and then **discarded** (never forwarded
   to the P2P network).  Failure → hard rejection.

3. **`validate_block()` + ledger** — node is the final authority on acceptance.
   A block that passes the canonical pre-check may still be rejected by
   `validate_block()` if e.g. difficulty is wrong or a conflicting block arrived.

### Code Example (SUBMIT_BLOCK handler)

```cpp
const CanonicalChainState& snap = context.canonical_snap;

if (snap.is_canonically_stale())
{
    debug::warning(FUNCTION, "SUBMIT_BLOCK pre-check: canonical snapshot stale (>30s) — proceeding with caution");
}

const uint32_t nTemplateHeight = it->second.pBlock ? it->second.pBlock->nHeight : 0;
if (nTemplateHeight > 0 && snap.canonical_unified_height > 0 &&
    nTemplateHeight != snap.canonical_unified_height)
{
    debug::warning(FUNCTION, "SUBMIT_BLOCK height mismatch: template height=", nTemplateHeight,
                   " canonical=", snap.canonical_unified_height,
                   " — may be stale template");
}
```

---

## MiningContext Integration

`CanonicalChainState` is stored directly in `LLP::MiningContext` as the
`canonical_snap` field.  This allows the SUBMIT_BLOCK handler to read the
snapshot that was active when the template was issued — without re-reading
chain state (which would reflect the *current* tip, not the tip the template
was built on).

```cpp
// In GET_BLOCK handler / SendStatelessTemplate():
context = context.WithCanonicalSnap(canonicalSnap);

// In SUBMIT_BLOCK handler:
const CanonicalChainState& snap = context.canonical_snap;
```

A default-constructed `MiningContext` has `canonical_snap.is_initialized() == false`,
which causes the pre-check gate to run in warn-only mode until the first template
is issued.

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

In `stateless_miner_connection.cpp`, `WithCanonicalSnap()` is called inside
`LOCK(MUTEX)` to ensure thread-safe context updates.

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
