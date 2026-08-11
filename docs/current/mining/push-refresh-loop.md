# Push-Refresh Loop — Mining Template Delivery

**Status:** Active (event-driven model since PR #278)  
**Date:** 2026-02-24

---

## Overview

With PR #278 merged, **every unified-tip advance** triggers
`BlockState::SetBest()` → `NotifyChannelMiners()` → `SendChannelNotification()`
+ `SendStatelessTemplate()` for **both** PoW channels, regardless of which
channel mined the block.

Templates are now **event-driven**.  The old 600-second age-out assumption is
gone.  Every new block (any channel) triggers a fresh push, with a 1-second
throttle guard to absorb fork-resolution bursts.

---

## Diagram 1 — Unified Tip Push Loop (happy path)

```diagram
Block found (any channel)
       │
       ▼
BlockState::SetBest()
       │
       ▼
NotifyChannelMiners(PRIME) ─────────────────────────────────────────────────┐
NotifyChannelMiners(HASH)  ─────────────────────────────────────────────────┤
       │                                                                     │
       ▼                                                                     ▼
StatelessMinerConnection::SendChannelNotification()     Miner::SendChannelNotification()
       │   [pushThrottle < 1s? drop]                          [pushThrottle < 1s? drop]
       │   LOG: [BLOCK CREATE] hashPrevBlock = X               LOG: [BLOCK CREATE] hashPrevBlock = X
       ▼                                                                     │
SendStatelessTemplate()  ───────────────────────────────────────────────────┘
       │   LOG: [BLOCK CREATE] hashPrevBlock = X (hashBestChain at block creation)
       ▼
Miner receives 0xD081 (228 bytes)
       │
       ▼
read_template() → [TEMPLATE ANCHOR] hashPrevBlock = XYZ...
       │           (compare with node's [BLOCK CREATE] log for cross-verification)
       ▼
Workers resume hashing on new tip ✅
```

---

## Diagram 2 — 6.5 s GET_BLOCK Rate Limiter (miner-side, legacy fallback only)

```diagram
Miner GET_BLOCK request path
       │
       ├─ elapsed < 6500 ms?  YES ─→ return empty (rate limited, silently drop)
       │
       └─ elapsed ≥ 6500 ms?  NO  ─→ transmit GET_BLOCK
                                            │
                                            ▼
                               m_last_get_block_time = now
                               (cooldown expires after 1 s — NOT reset on serve)
```

---

## Diagram 3 — AutoCoolDown (1 s) Rate-Limit Floor

```diagram
Solution found → pre-submission staleness pass → SUBMIT_BLOCK
       │
  BLOCK_REJECTED (STALE)? ────────── YES ──────────────────────────────┐
       │                                                                 │
       NO                                              cooldown Ready() after 1 s
       │                                               (NOT reset on serve —
       ▼                                               retry every 1 s during recovery)
BLOCK_ACCEPTED → request new template via push (node sends 0xD081 automatically)
```

---

## Diagram 4 — Template Staleness Decision Tree (miner)

```diagram
Template received (hashPrevBlock = H, nHeight = N)
       │
       ▼
New block push arrives (0xD081 / notification)
       │
       ├─ hashPrevBlock changed?  YES ─→ discard old template, feed new one ✅
       │
       └─ hashPrevBlock same?      NO  ─→ keep mining (tip unchanged, safe to continue)

Fallback (if no push within 1 s):
       │
       ▼
AutoCoolDown.Ready()?  YES ─→ send GET_BLOCK → receive fresh template
       │
       NO  ─→ continue mining (unlikely — node always pushes on tip advance)
```

---

## Diagram 5 — Node-Side Push Throttle Guard

```diagram
SetBest() fires (rapid fork-resolution burst: 5 events in 100 ms)
       │
   ┌───┴───────────────────────────────────────────────────┐
   │ Event 1 (t=0 ms)    │ Event 2 (t=20 ms) │ Events 3-5 │
   │ m_last_push=⊘       │ elapsed=20 ms     │ elapsed<    │
   │ → SEND template ✅   │ < 1000 ms         │  1000 ms   │
   │                      │ → THROTTLED ⛔    │ → THROTTLED │
   └──────────────────────┴───────────────────┴─────────────┘
       │
After 1000 ms settle time: next tip advance sends fresh template ✅
```

---

## Diagram 6 — Re-subscription Bypass (doom-loop prevention)

```diagram
STATELESS_MINER_READY received
       │
       ▼
m_force_next_push = true   (set under MUTEX before SendChannelNotification / SendStatelessTemplate)
       │
       ▼
SendChannelNotification() / SendStatelessTemplate()
       │
       ├─ m_force_next_push == true? YES ─→ m_force_next_push=false → SEND template ✅
       │                                     (throttle bypassed — re-subscription response)
       └─ m_force_next_push == false ─→ normal 1s throttle check applies
```

---

## Implementation Reference

| Constant | Value | File |
|---|---|---|
| `TEMPLATE_PUSH_MIN_INTERVAL_MS` | 1 000 ms | `src/LLP/include/mining_constants.h` |
| `GET_BLOCK_COOLDOWN_SECONDS` | 1 s | `src/LLP/include/mining_constants.h` |

| Class | Key members | File |
|---|---|---|
| `StatelessMinerConnection` | `m_last_template_push_time`, `m_force_next_push`, `m_get_block_cooldown` | `src/LLP/types/stateless_miner_connection.h` |
| `Miner` | `m_last_template_push_time`, `m_force_next_push`, `m_get_block_cooldown` | `src/LLP/types/miner.h` |

### `hashPrevBlock` Cross-Verification

`hashPrevBlock` in every mining template is set from `hashBestChain` at block
creation time.  The node logs the value at `debug::level 0` (visible in default
verbosity) in **all four** template-creation paths:

| Path | Log tag | Level |
|---|---|---|
| `StatelessMinerConnection::SendStatelessTemplate()` | `[BLOCK CREATE] hashPrevBlock = X (template anchor baked in, unified height N)` | 0 |
| `StatelessMinerConnection::SendChannelNotification()` | `[BLOCK CREATE] hashPrevBlock = X (template anchor embedded in push notification, unified height N)` | 0 |
| `StatelessMinerConnection` GET_BLOCK handler | `[BLOCK CREATE] hashPrevBlock = X (template anchor baked in, unified height N)` | 0 |
| `Miner::handle_get_block_stateless()` | `[BLOCK CREATE] hashPrevBlock = X (template anchor baked in, unified height N)` | 0 |
| `Miner::SendChannelNotification()` | `[BLOCK CREATE] hashPrevBlock = X (template anchor embedded in push notification, unified height N)` | 0 |

The miner's `[TEMPLATE ANCHOR] hashPrevBlock = X` log can be directly compared
with the node's `[BLOCK CREATE] hashPrevBlock = X` log to confirm alignment.
A full MSB-first hex value is also logged at `debug::level 2` for byte-level
cross-reference against the miner's raw deserialization logs.

See also: [stateless-protocol.md](stateless-protocol.md) §Push Throttle and AutoCoolDown
