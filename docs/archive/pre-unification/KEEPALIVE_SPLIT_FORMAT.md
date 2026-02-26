# ARCHIVED: Pre-Unification Keepalive Split Format

**Status: ARCHIVED — superseded by PR #301 (LLL-TAO) and PR #216 (NexusMiner)**  
**Archive Date: 2026-02-26**

This document describes the split keepalive format that existed BEFORE unification.
It is kept for historical reference only. Do NOT use this as implementation guidance.

## What Existed Before PR #301

### Legacy path (port 8323) — BuildBestCurrentResponse() — 28 bytes

```
[0-3]   session_id     LE uint32
[4-7]   unified_height BE uint32
[8-11]  prime_height   BE uint32
[12-15] hash_height    BE uint32
[16-19] stake_height   BE uint32
[20-23] nBits          BE uint32  ← removed in unification (miner reads from template)
[24-27] hashBestChain_prefix (4 raw bytes) ← replaced by hash_tip_lo32
```

### Stateless path (port 9323) — KeepAliveV2AckFrame::Serialize() — 32 bytes (pre-PR #301)

```
[0-3]   sequence            BE uint32  ← renamed to session_id (LE) in PR #301
[4-7]   hashPrevBlock_lo32  BE uint32
[8-11]  unified_height      BE uint32
[12-15] hash_tip_lo32       BE uint32
[16-19] prime_height        BE uint32
[20-23] hash_height         BE uint32
[24-27] stake_height        BE uint32
[28-31] fork_score          BE uint32
```

### Miner-side components deleted in NexusMiner PR #216

- `KeepaliveTelemetrySnapshot` struct
- `KeepaliveTelemetryStore` class
- `keepalive_telemetry.hpp` (entire file)
- `OnKeepaliveAck()` method on HeightTracker
- `OnLegacyKeepalive()` method on HeightTracker
- `KEEPALIVE_ACK` and `LEGACY_KEEPALIVE` UpdateSource enum values
- `m_last_keepalive_fork_score` member on Solo
- `get_keepalive_fork_score()` accessor on Solo
- `ForkScoreSource` callback type on ColinAgent
- `m_fork_score_source` member on ColinAgent
