# SIM-LINK Dual-Lane Architecture

## Overview

**SIM-LINK** is the dual-lane mining protocol architecture where a single authenticated miner session (`nSessionId`) is simultaneously active on both the legacy port (8323) and the stateless port (9323), sharing:

1. A single session-scoped `GetBlockRollingLimiter` budget (20/60s **total** across both lanes combined)
2. Session-scoped block template storage (so `SUBMIT_BLOCK` on either lane can resolve any template)
3. A single `SharedGetBlockHandler` implementation called by both connection types

---

## Problem Statement

Prior to SIM-LINK, the node had two independent GET_BLOCK codepaths:

| Port | Class | Rate Limiter | Block Storage |
|------|-------|--------------|---------------|
| 8323 | `Miner` | None (bypassed for stateless) | `mapBlocks` (raw ptr) + `mapBlockHashes` |
| 9323 | `StatelessMinerConnection` | Per-connection `m_getBlockRollingLimiter` | `mapBlocks` (TemplateMetadata + unique_ptr) |

This created three problems:

1. **Rate limits were per-connection, not per-session.** A miner with two connections (one legacy, one stateless) got 20 GET_BLOCK/min per connection — effectively 40/min total — bypassing the intended budget.

2. **`mapBlocks` was per-connection.** A template issued on port 9323 could not be resolved by a SUBMIT_BLOCK arriving on port 8323, even if both are the same authenticated session (same `nSessionId`).

3. **GET_BLOCK logic was duplicated.** `Miner::handle_get_block_stateless()` and `StatelessMinerConnection` GET_BLOCK handler were nearly identical but maintained separately.

---

## Architecture

### Session Rate Limiter

```
StatelessMinerManager::GetSessionRateLimiter(nSessionId)
  → returns shared_ptr<GetBlockRollingLimiter>
  → key: "session=N|combined"
  → budget: 20/60s total across ALL lanes for this session
```

The limiter is stored in `StatelessMinerManager::m_mapSessionLimiters` (keyed by session ID) and protected by `m_sessionLimiterMutex`. `shared_ptr` ensures the limiter object stays alive even after the map is released.

### Session Block Store

```
StatelessMinerManager::StoreSessionBlock(nSessionId, hashMerkleRoot, shared_ptr<Block>)
StatelessMinerManager::FindSessionBlock(nSessionId, hashMerkleRoot) → shared_ptr<Block>
StatelessMinerManager::PruneSessionBlocks(nSessionId)  // called on tip advance
```

Templates are stored as `shared_ptr<TAO::Ledger::Block>` copies (using Block's copy constructor) so both connection threads can safely reference the same template without ownership conflicts. Per-connection `mapBlocks` remains unchanged for backward compatibility.

### Shared Handler

```
LLP::SharedGetBlockHandler(GetBlockRequest) → GetBlockResult

GetBlockRequest {
    MiningContext context;               // lane, session, channel info
    uint32_t nSessionId;                 // for rate limiter lookup
    std::function<Block*()> fnCreateBlock; // per-connection new_block() callback
}

GetBlockResult {
    bool fSuccess;
    std::vector<uint8_t> vPayload;       // 228-byte BLOCK_DATA payload if success
    GetBlockPolicyReason eReason;        // failure reason if !fSuccess
    uint32_t nRetryAfterMs;              // retry hint if !fSuccess
    uint32_t nBlockChannel;              // for connection post-processing
    uint32_t nBlockHeight;               // for connection post-processing
    uint32_t nBlockBits;                 // for CanonicalChainState snapshot
}
```

**Handler steps:**
1. Session-scoped rate limit check (`GetSessionRateLimiter(nSessionId)->Allow("session=N|combined", ...)`)
2. `req.fnCreateBlock()` call (retry once on nullptr)
3. `StoreSessionBlock(nSessionId, hashMerkleRoot, copy_of_block)` for cross-lane resolution
4. Serialization of 228-byte BLOCK_DATA payload (`[0-3] nUnifiedHeight + [4-7] nChannelHeight + [8-11] nBits + [12-227] Block::Serialize()`)

---

## Architecture Invariants

| Invariant | Enforcement |
|-----------|-------------|
| "Never serve authenticated in-budget miners 0 block data" | `assert(!result.vPayload.empty())` in both callers + `SendGetBlockDataResponse` |
| 20/60s combined budget | Session limiter key `"session=N|combined"` shared across both lanes |
| Thread safety | `m_sessionLimiterMutex` + `m_sessionBlockMutex` in manager; `GetBlockRollingLimiter` internally locked |
| Backward compatibility | Per-connection `mapBlocks` unchanged; session map is additive |

---

## Files Changed

| File | Change |
|------|--------|
| `src/LLP/include/get_block_handler.h` | **NEW** — `GetBlockRequest`, `GetBlockResult`, `SharedGetBlockHandler()` |
| `src/LLP/get_block_handler.cpp` | **NEW** — shared handler implementation |
| `src/LLP/include/stateless_manager.h` | Added `GetSessionRateLimiter()`, `StoreSessionBlock()`, `FindSessionBlock()`, `PruneSessionBlocks()` + private storage members |
| `src/LLP/stateless_manager.cpp` | Implemented the four new session-scoped methods |
| `src/LLP/miner.cpp` | `handle_get_block_stateless()` delegates to `SharedGetBlockHandler` |
| `src/LLP/stateless_miner_connection.cpp` | GET_BLOCK handler delegates to `SharedGetBlockHandler`; `CheckRateLimit(GET_BLOCK)` uses session limiter; `SendChannelNotification()` calls `PruneSessionBlocks()` |
| `tests/unit/LLP/test_simlink_session_rate_limiter.cpp` | **NEW** — tests for session rate limiter and struct defaults |

---

## Design Decisions

### Why `shared_ptr<GetBlockRollingLimiter>` instead of a reference?

The limiter lives in `m_mapSessionLimiters` protected by `m_sessionLimiterMutex`. If we returned a raw reference, the caller would need to hold the mutex for the entire duration of the `Allow()` call, which could block the other lane. Using `shared_ptr` allows the mutex to be released immediately after lookup, and the limiter's own internal mutex protects concurrent `Allow()` calls.

### Why `shared_ptr<Block>` copies for session storage?

The per-connection `mapBlocks` uses raw pointers (legacy lane) or `unique_ptr` inside `TemplateMetadata` (stateless lane). These cannot be safely shared across threads. `shared_ptr<Block>` with a copy (using `Block`'s copy constructor) gives each domain independent ownership with shared lifetime.

### Why the stateless handler no longer calls `CheckRateLimit(GET_BLOCK)` before `SharedGetBlockHandler`?

`SharedGetBlockHandler` performs the session-scoped rate limit check as its first step. Calling `CheckRateLimit(GET_BLOCK)` before the handler would double-count the request against the session budget. `CheckRateLimit(GET_BLOCK)` has been updated to use the session limiter (for documentation completeness and potential future use), but is not invoked in the GET_BLOCK path.

### What about the legacy lane's rate limit?

The legacy lane (`Miner::handle_get_block_stateless()`) previously had no rate limit for the authenticated path. Under SIM-LINK, the session limiter inside `SharedGetBlockHandler` provides the first rate gate for the legacy lane too. This is a security improvement: a legacy-only miner is now subject to the same 20/60s session budget as a stateless-only miner.

---

## Known Limitations (Exploratory PR)

1. **Cross-lane SUBMIT_BLOCK resolution is additive-only.** The session block store holds copies, but SUBMIT_BLOCK handlers on each lane still only check their per-connection `mapBlocks`. Wiring `FindSessionBlock()` into the SUBMIT_BLOCK path is a follow-up task.

2. **Session limiter cleanup.** `m_mapSessionLimiters` grows with the number of unique sessions seen during the node's uptime. A periodic cleanup (tied to `CleanupExpiredSessions()`) should be added to prevent unbounded memory growth.

3. **Legacy lane session ID lookup.** The legacy handler uses `GetMinerContext(GetAddress().ToString())` to find the session ID. This lookup is per-connection-address; if the same miner connects from different IPs, the session IDs may not be correlated. This is a fundamental limitation of the legacy port's connection-scoped identity model.
