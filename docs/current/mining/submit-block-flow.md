# SUBMIT_BLOCK Flow — Stateless Mining Protocol

**Status:** Active  
**Applies to:** Stateless Lane (port 9323), `StatelessMinerConnection`  
**Source:** `src/LLP/stateless_miner_connection.cpp`  
**Last Updated:** 2026-03-01

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
  │                              │      FAIL → BLOCK_REJECTED (unknown template)
  │                              │
  │                              ├─ 6. ── Canonical pre-check gate (WARN-ONLY) ──
  │                              │      context.canonical_snap.is_canonically_stale()
  │                              │        → WARN if >30s since last GET_BLOCK/push
  │                              │      template.nHeight ≠ snap.canonical_unified_height
  │                              │        → WARN if height mismatch (stale round)
  │                              │      (no rejection; node's validate_block is authoritative)
  │                              │
  │                              ├─ 7. sign_block(nNonce, hashMerkle)
  │                              │      FAIL → BLOCK_REJECTED
  │                              │
  │                              ├─ 8. hashPrevBlock staleness guard
  │                              │      block.hashPrevBlock == ChainState::hashBestChain
  │                              │      FAIL → BLOCK_REJECTED (0x01 STALE)
  │                              │
  │                              ├─ 9. SIM Link deduplication
  │                              │      ColinMiningAgent::check_and_record_submission()
  │                              │      FAIL → BLOCK_REJECTED (silent)
  │                              │
  │                              ├─ 10. ValidateMinedBlock(*pTritium)
  │                              │       checks nBits, ProofHash, channel rules
  │                              │       FAIL → BLOCK_REJECTED
  │                              │
  │                              ├─ 11. AcceptMinedBlock(*pTritium)
  │                              │       ledger acceptance + P2P broadcast
  │                              │       FAIL → BLOCK_REJECTED
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

## Wire Protocol

### SUBMIT_BLOCK Packet (Miner → Node)

```
[ChaCha20 ciphertext]
    └── decrypted ──►
        Full-block format:  [216-byte block][timestamp 8B LE][sig_len 2B LE][Falcon signature]
        Legacy format:      [merkle 64B][nonce 8B LE][timestamp 8B LE][sig_len 2B LE][Falcon signature]
```

### BLOCK_ACCEPTED / BLOCK_REJECTED (Node → Miner)

```
BLOCK_ACCEPTED (0xD082):  0 bytes payload
BLOCK_REJECTED (0xD083):  1 byte reason code
    0x01 = STALE
    0x08 = Stale Falcon timestamp
    0x0B = ChaCha20 decryption failure
    0x0C = Falcon signature verification failed
    0x0F = Packet too small
    0xFF = Internal error
```

---

## Related Documents

- [`canonical-chain-state.md`](canonical-chain-state.md) — `CanonicalChainState` struct reference
- [`stateless-protocol.md`](stateless-protocol.md) — Full stateless mining protocol
- [`push-refresh-loop.md`](push-refresh-loop.md) — Template push and refresh loop
