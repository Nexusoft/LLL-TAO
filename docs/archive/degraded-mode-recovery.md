# Degraded Mode Recovery

**Version:** Post-PR #375  
**Last Updated:** 2026-03-13  
**Component:** SESSION\_STATUS handler — Stateless Port (0xD0DB) + Legacy Port (0xDB)

---

## Section 1 — Overview

**DEGRADED MODE** is a miner-side failure state.  The miner sets `MINER_DEGRADED`
(bit 0 of `status_flags`) in a `SESSION_STATUS` request when it has lost its block
template **or** its push-notification subscription has stalled and it is no longer
receiving new work.

Prior to PR #375, the LLL-TAO node simply echoed the `MINER_DEGRADED` flag back in
`SESSION_STATUS_ACK` and did nothing else — leaving the miner permanently stuck in
DEGRADED MODE with zero new work delivered.  The fix (PR #375) makes the handler
*proactive*: on receipt of a `SESSION_STATUS` with `MINER_DEGRADED` set, the node
immediately forces a channel notification + fresh template push so the miner can
resume mining within one protocol exchange.

---

## Section 2 — Technical Error Report

```
╔══════════════════════════════════════════════════════════════════════════════════╗
║             TECHNICAL ERROR REPORT — DEGRADED MODE RECOVERY FAILURE             ║
║                       LLL-TAO Stateless Mining Protocol                         ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  Report Date       : 2026-03-13                                                 ║
║  Severity          : CRITICAL — miner permanently halted, 0 blocks mined        ║
║  Component         : SESSION_STATUS handler (stateless + legacy ports)          ║
║  Affected Files    : src/LLP/stateless_miner_connection.cpp                     ║
║                      src/LLP/miner.cpp                                          ║
║                      src/LLP/colin_mining_agent.cpp                             ║
║                      src/LLP/include/session_recovery.h                         ║
║                      src/LLP/session_recovery.cpp                               ║
║  PRs That Preceded : #370 #372 #373 #374 (all merged, issue persisted)          ║
║  Fix PR            : #375                                                       ║
║  Status After Fix  : ✅ STABLE — DEGRADED MODE exits within 1 SESSION_STATUS   ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  ROOT CAUSES                                                                    ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  BUG 1  SESSION_STATUS handler is PASSIVE — MINER_DEGRADED echoed, not acted    ║
║         on. No template push, no channel notification, no m_force_next_push.    ║
║         Both stateless port (0xD0DB) and legacy port (0xDB) affected.           ║
║                                                                                 ║
║  BUG 2  Template DRIFT — SendChannelNotification() consumes m_force_next_push   ║
║         and resets m_last_template_push_time. Without re-arming the flag,       ║
║         SendStatelessTemplate() sees elapsed=0ms and is throttled.              ║
║         Net result: push notification sent, BLOCK_DATA template never follows.  ║
║                                                                                 ║
║  BUG 3  ColinMiningAgent::emit_report() calls RecoverSessionByIdentity()        ║
║         (consuming!) every 60 seconds. DEFAULT_MAX_RECONNECTS=10 means          ║
║         10 report cycles exhaust all reconnect slots — real recovery is          ║
║         evicted before it can fire. This is the "0 for 10" pattern.             ║
║                                                                                 ║
║  BUG 4  MINER_READY handler sends template BEFORE SaveSession(). If the         ║
║         connection drops in the race window, the recovery cache retains the     ║
║         old channel height → next MINER_READY skips the push (throttled).       ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  FIXES APPLIED (PR #375)                                                        ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  FIX 1  SESSION_STATUS handler: after ACK, detect MINER_DEGRADED and arm        ║
║         m_force_next_push → SendChannelNotification() → re-arm → Template push  ║
║                                                                                 ║
║  FIX 2  Two-step re-arm pattern documented and enforced with comment block      ║
║                                                                                 ║
║  FIX 3  New PeekSession() public API on SessionRecoveryManager — read-only,     ║
║         never increments nReconnectCount. ColinAgent switched to PeekSession()  ║
║                                                                                 ║
║  FIX 4  MINER_READY / STATELESS_MINER_READY: reordered to SaveSession() FIRST, ║
║         then SendStatelessTemplate() — write-ahead pattern                      ║
╠══════════════════════════════════════════════════════════════════════════════════╣
║  POST-FIX STATUS   : STABLE ✅                                                  ║
║  Tested Scenarios  : DEGRADED → SESSION_STATUS → Template Push → MINING         ║
║                      ColinAgent 10+ report cycles → session NOT exhausted       ║
║                      MINER_READY race-window → correct height in recovery cache  ║
╚══════════════════════════════════════════════════════════════════════════════════╝
```

---

## Section 3 — MSCII Protocol Flow Diagrams

### Diagram A — Before Fix: DEGRADED MODE Doom Loop

```
┌─────────────────────────────────────────────────────────────────┐
│              DEGRADED MODE DOOM LOOP (Pre-PR #375)              │
└─────────────────────────────────────────────────────────────────┘

  NexusMiner                                          LLL-TAO Node
      │                                                     │
      │  [enters DEGRADED: template lost / push stalled]    │
      │                                                     │
      │──── SESSION_STATUS (MINER_DEGRADED flag set) ──────►│
      │                                                     │
      │                          ┌─────────────────────┐   │
      │                          │ Parse request        │   │
      │                          │ Build ACK            │   │
      │                          │ Echo MINER_DEGRADED  │   │
      │                          │ return true ← ← ← ← │◄──┤ ⚠️ BUG 1
      │                          └─────────────────────┘   │
      │                                                     │
      │◄─── SESSION_STATUS_ACK (echo: MINER_DEGRADED) ─────│
      │                                                     │
      │  [still DEGRADED — no template received]            │
      │  [waits degraded_poll_interval, then...]            │
      │                                                     │
      │──── SESSION_STATUS (MINER_DEGRADED flag set) ──────►│  ← repeats forever
      │                          ...                        │
      │                          ...                        │
      ╳  MINING HALTED PERMANENTLY  ╳
```

---

### Diagram B — After Fix: DEGRADED MODE Recovery Path

```
┌─────────────────────────────────────────────────────────────────┐
│           DEGRADED MODE RECOVERY FLOW (Post-PR #375)            │
└─────────────────────────────────────────────────────────────────┘

  NexusMiner                                          LLL-TAO Node
      │                                                     │
      │  [enters DEGRADED: template lost / push stalled]    │
      │                                                     │
      │──── SESSION_STATUS (MINER_DEGRADED=1) ─────────────►│
      │                                                     │
      │                    ┌──────────────────────────────┐ │
      │                    │ 1. Parse request              │ │
      │                    │ 2. Build lane-health ACK      │ │
      │                    │ 3. Send SESSION_STATUS_ACK    │ │
      │                    │ 4. Check MINER_DEGRADED bit ✓ │ │
      │                    │ 5. m_force_next_push = true   │ │
      │                    │ 6. SendChannelNotification()  │ │  (step A)
      │                    │ 7. m_force_next_push = true   │ │  ← re-arm (BUG 2 fix)
      │                    │ 8. SendStatelessTemplate()    │ │  (step B)
      │                    └──────────────────────────────┘ │
      │                                                     │
      │◄─── SESSION_STATUS_ACK ─────────────────────────────│
      │◄─── CHANNEL_NOTIFICATION (PRIME/HASH_AVAILABLE) ────│  (step A arrives)
      │◄─── BLOCK_DATA (fresh template) ────────────────────│  (step B arrives)
      │                                                     │
      │  [clears DEGRADED flag]                             │
      │  [resumes mining with fresh template]               │
      │                                                     │
      ✓  MINING RESUMED
```

---

### Diagram C — ColinAgent PeekSession vs RecoverSessionByIdentity (Bug 3)

```
┌─────────────────────────────────────────────────────────────────┐
│        ColinAgent Reconnect Slot Exhaustion — Bug 3 Fix         │
└─────────────────────────────────────────────────────────────────┘

  BEFORE (RecoverSessionByIdentity — consuming):

  ┌──────────────┐   every 60s    ┌───────────────────────────────┐
  │ ColinAgent   │───────────────►│ RecoverSessionByIdentity()    │
  │ emit_report()│                │   → PreviewRecoverableSession │
  └──────────────┘                │   → RecoverSession()          │
                                  │       nReconnectCount++       │ ← BUG
                                  └───────────────────────────────┘

  After 10 report cycles (600 seconds):
  nReconnectCount == DEFAULT_MAX_RECONNECTS (10)
  → Session EVICTED from recovery cache
  → Real MINER_READY recovery: "No recoverable session" → FAILS ✗


  AFTER (PeekSession — read-only):

  ┌──────────────┐   every 60s    ┌───────────────────────────────┐
  │ ColinAgent   │───────────────►│ PeekSession()                 │
  │ emit_report()│                │   → mapSessionsByKey.Get()    │
  └──────────────┘                │   (no nReconnectCount touch)  │ ✓
                                  └───────────────────────────────┘

  After 1000 report cycles:
  nReconnectCount unchanged
  → Real MINER_READY recovery: session available → SUCCEEDS ✓
```

---

### Diagram D — MINER\_READY Write-Ahead Fix (Bug 4)

```
┌─────────────────────────────────────────────────────────────────┐
│          MINER_READY Handler Order — Bug 4 Fix                  │
└─────────────────────────────────────────────────────────────────┘

  BEFORE (Race Window):                 AFTER (Write-Ahead):

  1. compute nChannelHeight             1. compute nChannelHeight
  2. SendStatelessTemplate()  ← sends   2. context.WithLastTemplateChannelHeight()
  3. context.WithHeight(...)            3. SaveSession(context)  ← persisted first
  4. SaveSession(context)               4. SendStatelessTemplate()  ← then send

  ⚠️  If connection drops between       ✓  Session always has correct height
      steps 2 and 4:                       before template is sent.
      cache = OLD height                   No race window.
      next recovery = throttled push
      miner stays DEGRADED
```

---

### Diagram E — Full SESSION\_STATUS State Machine (Post-Fix)

```
┌─────────────────────────────────────────────────────────────────┐
│         SESSION_STATUS Full State Machine (Post-PR #375)        │
│              Stateless Port (0xD0DB) + Legacy Port (0xDB)       │
└─────────────────────────────────────────────────────────────────┘

                   ┌──────────────────┐
                   │  Receive         │
                   │  SESSION_STATUS  │
                   └────────┬─────────┘
                            │
                   ┌────────▼─────────┐
                   │  Parse payload   │
                   │  (8 bytes)       │◄── FAIL ──► send zero ACK, return
                   └────────┬─────────┘
                            │
                   ┌────────▼─────────┐
                   │  Validate        │
                   │  session_id      │◄── MISMATCH ──► send corrective ACK, return
                   └────────┬─────────┘
                            │
                   ┌────────▼─────────┐
                   │  Build lane      │
                   │  health flags    │
                   │  + uptime        │
                   └────────┬─────────┘
                            │
                   ┌────────▼─────────┐
                   │  Send            │
                   │  SESSION_STATUS  │
                   │  _ACK (16 bytes) │
                   └────────┬─────────┘
                            │
              ┌─────────────▼──────────────┐
              │  MINER_DEGRADED flag set?   │
              └──────┬──────────┬──────────┘
                   NO│          │YES
                     │   ┌──────▼───────────────────┐
                     │   │  fAuthenticated &&        │
                     │   │  channel ∈ {1,2}?         │
                     │   └──────┬──────────┬─────────┘
                     │        NO│          │YES
                     │          │  ┌───────▼───────────────────┐
                     │          │  │  m_force_next_push = true  │
                     │          │  │  SendChannelNotification() │
                     │          │  │  m_force_next_push = true  │ ← re-arm
                     │          │  │  SendStatelessTemplate()   │
                     │          │  │  log: "✓ recovery push"    │
                     │          │  └───────────────────────────┘
                     │          │
                     └──────────┴──────► return true
```

---

## Section 4 — Recovery Validation Checklist

Operators can run through the following checklist after upgrading to PR #375 or later:

- [ ] Miner enters DEGRADED → sends `SESSION_STATUS` → receives `BLOCK_DATA` within 1 exchange
- [ ] ColinAgent runs 10+ report cycles → `RecoverSession()` still succeeds afterward
- [ ] Node restarted while miner in DEGRADED → `MINER_READY` fires → template delivered → height in recovery cache matches channel tip
- [ ] Log line `✓ Degraded-recovery template pushed` appears for each DEGRADED recovery
- [ ] Log line `PeekSession` (level 2) appears in ColinAgent report cycles (NOT "SESSION RECOVERY RESTORE")

---

## Section 5 — Cross-References

- [Stateless Mining Protocol](stateless-protocol.md)
- [Push Refresh Loop](push-refresh-loop.md)
- [Failover State Machine](../../diagrams/failover/failover-state-machine.md)
- [Degraded Mode Recovery Flow Diagrams](../../diagrams/mining/degraded-mode-recovery-flow.md)
- `src/LLP/include/session_status.h`
- `src/LLP/session_recovery.h`
