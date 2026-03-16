# Unified Tip Anchoring vs Channel Heights

**Status:** Canonical Reference (Active)  
**Applies to:** Stateless Mining Protocol, Legacy Tritium Protocol  
**Last Updated:** 2026-02-23

---

## Overview

Nexus mining involves two distinct height concepts that serve different purposes:

| Concept | Variable | Purpose |
|---|---|---|
| **Unified best-chain tip** | `hashBestChain` / `tStateBest.nHeight` | Identifies which block all new blocks must extend |
| **Channel target height** | `stateChannel.nChannelHeight + 1` | Determines the correct `nHeight` value inside a PoW block template |

These are **not interchangeable**. Conflating them is the root cause of historical false-staleness rejections and misleading documentation.

---

## 1. The Unified Best Chain Tip

### What it is

`TAO::Ledger::ChainState::hashBestChain` is the 256-bit hash of the current chain-tip block — the latest accepted block across **all channels** (Stake, Prime, Hash).

`TAO::Ledger::ChainState::tStateBest` is the corresponding `BlockState` object, which stores:
- `nHeight` — unified (all-channel) height
- `GetHash()` — equivalent to `hashBestChain`

### Why it matters for mining

Every new block — regardless of channel — must have:

```
pBlock->hashPrevBlock == ChainState::hashBestChain
```

This is enforced by `Block::Accept()`. A block that points to a superseded tip is **stale** and will be rejected, even if its channel height is numerically correct.

### The StakeMinter analogy

`StakeMinter` stores a snapshot of the tip hash when it begins hashing:

```cpp
uint512_t hashLastBlock = ChainState::hashBestChain.load();
```

It restarts hashing whenever the live tip diverges from this snapshot:

```cpp
if(hashLastBlock != ChainState::hashBestChain.load())
    RestartHashing(); // tip_moved
```

PoW miners must apply the same logic: **refresh the template whenever the unified tip moves**, even if their channel has not produced a new block.

---

## 2. Channel Target Height

### What it is

Each PoW channel (Prime = 1, Hash = 2) has its own monotonically increasing block counter: `BlockState::nChannelHeight`.

`CreateBlockForStatelessMining` obtains the last known state for the miner's channel and sets:

```cpp
// stateChannel is the last BlockState for this channel
pBlock->nHeight = stateChannel.nChannelHeight + 1;   // channel target
```

This value is the **channel target height** — it tells `Block::Accept()` what `nChannelHeight` value to expect for the next block in this channel's sequence.

> **Important:** `pBlock->nHeight` in a stateless mining template represents the channel target height, NOT the unified blockchain height. Do not confuse this with `ChainState::tStateBest.nHeight`.

### When channel height matters

Channel height is used exclusively by `Block::Accept()` to prevent sequence gaps within a channel:

```cpp
// Inside Block::Accept() — channel validation
if(statePrev.nChannelHeight + 1 != pBlock->nHeight)
    return error("channel height mismatch");
```

If two blocks are produced for the same channel simultaneously, only one can satisfy this check. The other is rejected as `channel_advanced`.

---

## 3. Two Staleness Axes

A mining template can become stale for two independent reasons:

### Axis 1: `tip_moved`

The unified best-chain tip advanced (any channel produced a block), so `hashBestChain` changed.

- **Detection:** `pBlock->hashPrevBlock != ChainState::hashBestChain.load()`
- **Effect:** Any submitted block with the old `hashPrevBlock` is rejected, regardless of channel height.
- **Correct action:** Discard template and request a fresh one immediately.
- **Recommended log label:** `tip_moved`

```
[INFO] template stale: tip_moved prev=00ab...cd best=00ef...gh
```

### Axis 2: `channel_advanced`

A new block appeared in this miner's specific channel, incrementing `nChannelHeight`.

- **Detection:** `stateChannel.nChannelHeight + 1 != pBlock->nHeight`
- **Effect:** Submitted block fails channel-height validation in `Block::Accept()`.
- **Correct action:** Discard template and request a fresh one.
- **Recommended log label:** `channel_advanced`

```
[INFO] template stale: channel_advanced ch_target=2302665 ch_current=2302666
```

### Relationship between the two axes

`channel_advanced` implies `tip_moved` (a new block always moves the unified tip).  
`tip_moved` does **not** imply `channel_advanced` (a Hash block moves the tip but does not advance the Prime channel height).

Therefore, miners must refresh on `tip_moved` even when `channel_advanced` is false.

---

## 4. Push Payload Semantics (12 bytes)

Both protocol lanes (8-bit port 8323 and 16-bit port 9323) deliver a 12-byte push notification payload:

```
Offset  Size  Field            Meaning
------  ----  ---------------  ----------------------------------------
0-3     4     nUnifiedHeight   Unified chain height at time of push
4-7     4     nChannelHeight   Current height of the notified channel
8-11    4     nBits            Next difficulty target for that channel
```

### What the payload can express

- `nUnifiedHeight` tells the miner the current global chain height, enabling **tip movement detection** without fetching a new template first.
- `nChannelHeight` tells the miner the current channel-specific height, enabling **`channel_advanced` detection**.
- `nBits` provides difficulty context without requiring a separate round-trip.

### What the payload cannot express

- The payload does **not** include `hashBestChain`. Miners must fetch a new template to obtain `hashPrevBlock`, which is the authoritative anchor.
- The payload does **not** guarantee the new template is already available; there may be a small propagation delay between the push and `GET_BLOCK` being served.
- Authenticated `GET_BLOCK` handling no longer uses ambiguous empty `BLOCK_DATA`; when no template can be served, the node sends an explicit control outcome with reason/retry guidance.

### Recommended miner response to a push notification

```
1. Receive push (tip_moved detected via nUnifiedHeight change)
2. Send GET_BLOCK immediately
3. Replace current template with the response
4. Verify pBlock->hashPrevBlock == expected hashBestChain
5. Begin mining on new template
```

---

## 5. Standardised Terminology

All documentation in this repository uses the following terms:

| Term | Meaning |
|---|---|
| `unified_current` | Current value of `ChainState::tStateBest.nHeight` |
| `unified_next` | `unified_current + 1` (not used in templates; informational only) |
| `channel_current` | `stateChannel.nChannelHeight` (last mined block in this channel) |
| `channel_target` | `channel_current + 1` (the value placed in `pBlock->nHeight`) |
| `template.prev_hash` | `pBlock->hashPrevBlock` (must equal `best_hash` at submission) |
| `best_hash` | `ChainState::hashBestChain` at the moment of block acceptance |

---

## 6. Cross-References

- [Mining Protocol Lanes](../../protocol/mining-protocol.md) — opcode tables, payload builder
- [Stateless Protocol (Node)](stateless-protocol.md) — full node-side implementation
- [Push Notification Flow](../../diagrams/push-notification-flow.md) — sequence diagrams
- [Mining Debug Cheat Sheet](../../onboarding/cheat-sheets/mining-debug.md) — troubleshooting `stale_tip` vs `channel_advanced`
- Archive: [GET_ROUND Protocol](../../archive/GET_ROUND_PROTOCOL.md) — historical polling protocol (deprecated)
- Archive: [Channel Height Discrepancy](../../archive/CHANNEL_HEIGHT_DISCREPANCY.md) — +3 block historical anomaly
- Source: `src/TAO/Ledger/include/chainstate.h` — `hashBestChain`, `tStateBest`
- Source: `src/TAO/Staking/stakepool.h` / `src/TAO/Staking/stakeminter.h` — `hashLastBlock` pattern
- Source: `src/LLP/stateless_miner_connection.cpp` — `CreateBlockForStatelessMining`
