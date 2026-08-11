# PR #375 — Fix DEGRADED MODE never recovers: SESSION\_STATUS proactive push + PeekSession

**Status: STABLE ✅**  
**Merged:** 2026-03-13  
**Related PRs:** [#370](#370-related), [#372](#372-related), [#373](#373-related), [#374](#374-related)

---

## Problem Statement

**"0 for 10" DEGRADED MODE recovery failure.**

When a NexusMiner lost its block template or its push-notification subscription stalled,
it set `MINER_DEGRADED` (bit 0 of `status_flags`) in every subsequent `SESSION_STATUS`
request.  The LLL-TAO node echoed the flag back in `SESSION_STATUS_ACK` — and did
nothing else.  The miner remained permanently stuck in DEGRADED MODE, mining zero blocks.

Four previous PRs (#370, #372, #373, #374) all merged without resolving the root cause.
PR #375 identifies four distinct bugs and fixes each one.

---

## Root Causes (4 Bugs)

### BUG 1 — SESSION\_STATUS handler is PASSIVE
The handler parsed the request, built the ACK, echoed `MINER_DEGRADED`, and returned
`true`.  There was no code path that checked the `MINER_DEGRADED` bit and acted on it.
Both the stateless port (0xD0DB, `stateless_miner_connection.cpp`) and the legacy port
(0xDB, `miner.cpp`) were affected.

### BUG 2 — Template DRIFT (two-step re-arm missing)
`SendChannelNotification()` consumed `m_force_next_push` (set it `false`) **and** reset
`m_last_template_push_time` to "now".  Without re-arming the flag before calling
`SendStatelessTemplate()`, the template push function saw `elapsed ≈ 0 ms` and was
throttled by `TEMPLATE_PUSH_MIN_INTERVAL_MS`.  Net result: channel notification sent,
`BLOCK_DATA` template never followed.

### BUG 3 — ColinAgent reconnect slot exhaustion
`ColinMiningAgent::emit_report()` called `RecoverSessionByIdentity()` — a **consuming**
function that increments `nReconnectCount` — every 60 seconds.  With
`DEFAULT_MAX_RECONNECTS = 10`, after 10 report cycles (600 seconds) all reconnect slots
were exhausted.  When a real `MINER_READY` recovery then fired, the session had already
been evicted.  This is the literal "0 for 10" pattern observed in logs.

### BUG 4 — MINER\_READY race window (write-after-send)
The `MINER_READY` / `STATELESS_MINER_READY` handler sent the template **before** calling
`SaveSession()`.  If the connection dropped in the race window between the send and the
save, the recovery cache retained the old channel height.  The next `MINER_READY`
handler's push was throttled because it saw the cached height as current → miner stayed
DEGRADED.

---

## Files Changed

| File | Change |
|------|--------|
| `src/LLP/stateless_miner_connection.cpp` | SESSION\_STATUS handler: detect `MINER_DEGRADED`, arm `m_force_next_push`, call `SendChannelNotification()`, re-arm, call `SendStatelessTemplate()`. Added TWO-STEP RE-ARM comment block. |
| `src/LLP/miner.cpp` | Legacy-port SESSION\_STATUS handler: same `MINER_DEGRADED` detection + force-push logic. Added matching comment referencing the two-step re-arm invariant. |
| `src/LLP/include/session_recovery.h` | Added `PeekSession(hashKeyID)` public API — read-only, never increments `nReconnectCount`. |
| `src/LLP/session_recovery.cpp` | Implemented `PeekSession()` using `mapSessionsByKey.Get()` without any mutation. Added diagnostic comment block. |
| `src/LLP/colin_mining_agent.cpp` | Switched `emit_report()` from `RecoverSessionByIdentity()` to `PeekSession()`. Added call-site comment explaining the consuming-vs-read-only distinction. |

---

## Post-Merge Status

**STABLE ✅**

| Scenario | Result |
|----------|--------|
| DEGRADED → SESSION\_STATUS → Template Push → MINING | ✅ passes |
| ColinAgent 10+ report cycles → `RecoverSession()` still succeeds | ✅ passes |
| MINER\_READY race-window → correct height in recovery cache | ✅ passes |

---

## Test Coverage Added

- DEGRADED recovery path exercises the two-step re-arm sequence end-to-end
- `PeekSession()` unit test: 1000 read-only calls → `nReconnectCount` remains zero
- `MINER_READY` write-ahead: drop connection after `SaveSession()` → subsequent recovery delivers correct template height

---

## Related PRs

| PR | Description |
|----|-------------|
| #370 | Preceding work — issue persisted after merge |
| #372 | Preceding work — issue persisted after merge |
| #373 | Preceding work — issue persisted after merge |
| #374 | Preceding work — issue persisted after merge |
| **#375** | **This PR — root-cause fix** |

---

## See Also

- [Technical Error Report & Protocol Flow Diagrams](../../current/mining/degraded-mode-recovery.md)
- [Stateless Mining Protocol](../../current/mining/stateless-protocol.md)
- [Session Recovery Architecture](../../current/node/session-container-architecture.md)
