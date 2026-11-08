# SUBMIT_BLOCK Flow — Stateless Mining Protocol

**Status:** Active  
**Applies to:** Stateless Lane (port 9323), `StatelessMinerConnection`  
**Source:** `src/LLP/stateless_miner_connection.cpp`  
**Last Updated:** 2026-07-09

---

## Overview

This document describes the complete flow from when a miner sends a
`SUBMIT_BLOCK` packet to when the node responds with `BLOCK_ACCEPTED` or
`BLOCK_REJECTED`.

The flow enforces a **three-tier authority model**:

| Tier | Gate | Outcome on failure |
|------|------|--------------------|
| 1 | DisposableFalcon signature | Hard rejection |
| 2 | Canonical pre-check gate | WARN-only, continue |
| 3 | `validate_block()` + ledger | Hard rejection |

---

## Sequence Diagram

```
MINER                          NODE  (StatelessMinerConnection)
  │                              │
  │── SUBMIT_BLOCK ─────────────►│
  │   (ChaCha20 encrypted)       │
  │                              │
  │                              ├─ 1. ChaCha20 decrypt
  │                              │      vChaChaKey from MiningContext
  │                              │      → decryptedData
  │                              │
  │                              ├─ 2. Detect format
  │                              │      Full-block format: [block_data][timestamp][sig_len][sig]
  │                              │      Legacy format:     [merkle][nonce][timestamp][sig_len][sig]
  │                              │
  │                              ├─ 3. DisposableFalcon::VerifyWorkSubmission()
  │                              │      vPubKey from MiningContext
  │                              │      → hashMerkle, nonce extracted
  │                              │      FAIL → BLOCK_REJECTED (0x0C)
  │                              │
  │                              ├─ 4. Timestamp freshness check
  │                              │      |now - timestamp| ≤ 30s
  │                              │      FAIL → BLOCK_REJECTED (0x08)
  │                              │
  │                              ├─ 5. find_block(hashMerkle)
  │                              │      Template must exist in mapBlocks
  │                              │      FAIL → BLOCK_REJECTED (0x09 unknown template)
  │                              │
  │                              ├─ 6. ── Canonical pre-check gate (WARN-ONLY) ──
  │                              │      context.canonical_snap.is_canonically_stale()
  │                              │        → WARN if >30s since last GET_BLOCK/push
  │                              │      template.nHeight ≠ snap.canonical_unified_height
  │                              │        → WARN if height mismatch (stale round)
  │                              │      (no rejection; node's validate_block is authoritative)
  │                              │
  │                              ├─ 7. sign_block(nNonce, hashMerkle)
  │                              │      FAIL → BLOCK_REJECTED (0x16 preparation failed)
  │                              │
  │                              ├─ 8. Shared staleness pre-check
  │                              │      ValidateSubmitBlockStaleness()
  │                              │      checks hashPrevBlock, committed vtx,
  │                              │      producer sigchain, vtx sigchain,
  │                              │      and merkle immutability
  │                              │      FAIL → BLOCK_REJECTED (0x01 STALE)
  │                              │
  │                              ├─ 9. Coinbase contract size guard
  │                              │      FAIL → BLOCK_REJECTED (0x0B)
  │                              │
  │                              ├─ 10. ValidateMinedBlock(*pTritium)
  │                              │       checks nBits, ProofHash, channel rules
  │                              │       Check(fForceProof=true): full PoW/PoS proof
  │                              │       verified unconditionally (ignores Synchronizing())
  │                              │       FAIL → BLOCK_REJECTED (0x0E, 0x01 or 0x11)
  │                              │
  │                              ├─ 11. AcceptMinedBlock(*pTritium)
  │                              │       ledger acceptance + P2P broadcast
  │                              │       FAIL → BLOCK_REJECTED (0x0E, 0x01, 0x04 or 0x12)
  │                              │
  │◄── BLOCK_ACCEPTED ───────────┤  (or BLOCK_REJECTED with reason byte)
  │
```

---

## Step 6: Canonical Pre-Check Gate — Detail

The canonical pre-check gate was introduced in PR #323 as part of the node-side
finalization of the canonical chain state separation.

### What It Checks

```cpp
const CanonicalChainState& snap = context.canonical_snap;

// Check 1: Snapshot age
if (snap.is_canonically_stale())
    debug::warning(FUNCTION, "SUBMIT_BLOCK pre-check: canonical snapshot stale (>30s)...");

// Check 2: Height cross-check
const uint32_t nTemplateHeight = it->second.pBlock->nHeight;
if (nTemplateHeight > 0 && snap.canonical_unified_height > 0 &&
    nTemplateHeight != snap.canonical_unified_height)
    debug::warning(FUNCTION, "SUBMIT_BLOCK height mismatch: template=", nTemplateHeight,
                   " canonical=", snap.canonical_unified_height);
```

### Why WARN-Only?

The node's `validate_block()` is the authoritative gate.  The canonical
pre-check exists to give **early diagnostic signal** — before `validate_block()`
runs — so that:

1. Operators can detect miners submitting stale templates
2. Colin's periodic report can flag sessions with stale snapshots

A miner submitting a perfectly valid block that happens to have a stale
snapshot (e.g., the snapshot was captured 31 seconds ago but the block is
still at the correct height) should **not** be rejected by the pre-check.

### When It Fires

| Scenario | Which warning fires |
|----------|---------------------|
| Miner never called GET_BLOCK (push-only) and >30s elapsed | Staleness warning |
| Network partition caused miner to retry without refresh | Staleness warning |
| Miner solved a template from a previous round | Both warnings |
| `canonical_snap` is default-constructed (no GET_BLOCK yet) | Staleness warning only |

---

## Step 8: Shared Staleness Pre-Check — Detail

After `sign_block()` finalizes the candidate and freezes the merkle root, both
mining lanes call `ValidateSubmitBlockStaleness()`.

The helper is detection-only and rejects stale work before the expensive
`ValidateMinedBlock()` / `AcceptMinedBlock()` path. It emits a stable
`[SUBMIT_STALE]` diagnostic with lane, reason code, channel, unified height,
merkle root, `hashPrevBlock`, and current `hashBestChain`.

Reason codes:

| Code | Meaning |
|------|---------|
| `HASH_PREV_BLOCK` | Template extends an old chain tip. |
| `VTX_ALREADY_COMMITTED` | A transaction in `vtx` is already committed by another block. |
| `PRODUCER_SIGCHAIN` | Producer transaction no longer follows the authoritative sigchain predecessor. |
| `VTX_SIGCHAIN` | A `vtx` transaction is stale or malformed relative to the connect-time sigchain anchor. |
| `MERKLE_MUTATED` | A pre-validation step mutated `hashMerkleRoot`, invalidating the proof. |

Any staleness-rooted rejection invalidates the stale cached template and forces
a fresh template push so the miner does not wait for its own poll/backoff loop.

---

## Wire Protocol

### SUBMIT_BLOCK Packet (Miner → Node)

```
[ChaCha20 ciphertext]
    └── decrypted ──►
        Full-block format (Hash channel):  [216-byte block][timestamp 8B LE][sig_len 2B LE][Falcon signature]
        Full-block format (Prime channel): [216-byte block][vOffsets N bytes][timestamp 8B LE][sig_len 2B LE][Falcon signature]
        Compact legacy wrapper:            [merkle 64B][nonce 8B LE][timestamp 8B LE][sig_len 2B LE][Falcon signature]
```

The Falcon-wrapped full-block payload grammar is shared by both mining lanes. Only the
outer packet framing differs:

- Legacy lane: 1-byte opcode header
- Stateless lane: 2-byte opcode header

### BLOCK_ACCEPTED / BLOCK_REJECTED (Node → Miner)

```
BLOCK_ACCEPTED (0xD0C8):  0 bytes payload
BLOCK_REJECTED (0xD0C9):  [1 byte reason code] for SUBMIT_BLOCK
    0x01 = stale precheck or tip race
    0x03 = legacy Falcon-wrapped verification failure
    0x04 = duplicate block
    0x06 = session invalid / missing channel (also legacy missing key)
    0x07 = reward not bound (legacy)
    0x08 = stale Falcon timestamp (stateless); general enum also calls this RATE_LIMITED
    0x09 = unknown template
    0x0B = malformed producer OR ChaCha20 decrypt failure
    0x0C = encryption required OR Falcon-wrapped verification failure
    0x0D = missing Falcon session key (stateless)
    0x0E = mining paused or work epoch revoked
    0x0F = packet too small
    0x11 = other mined-block validation failure
    0x12 = other ledger acceptance failure
    0x13 = rate limited
    0x14 = session inconsistent
    0x15 = packet too large
    0x16 = candidate preparation failed (multiple possible causes)
    0x17 = unexpected block type
    0x18 = malformed submission / parse failure (legacy)
    0xFF = internal error
```

The detailed reason remains in node logs. `Check()` failures are not labeled
invalid PoW: that check also performs other consensus validation. Historical
shared enum assignments `0x02` INVALID_POW, `0x05` FORK and `0x0A`
LOCAL_TEMPLATE_REJECT remain reserved; these submit handlers no longer emit
them. Older nodes can send empty rejections. An unauthenticated preflight
instead sends an authentication result, and oversized declared frames can be
disconnected at header validation before any rejection. The legacy lane retains
its historical empty `ORPHAN_BLOCK` opcode for a stale previous hash. The same
`BLOCK_REJECTED` opcode also carries an eight-byte GET_BLOCK control response,
not a submit reason. Consumers must tolerate unknown codes; `0x08`, `0x0B`,
and `0x0C` remain historically ambiguous.

---

## Related Documents

- [`canonical-chain-state.md`](canonical-chain-state.md) — `CanonicalChainState` struct reference
- [`stateless-protocol.md`](stateless-protocol.md) — Full stateless mining protocol
- [`push-refresh-loop.md`](push-refresh-loop.md) — Template push and refresh loop
