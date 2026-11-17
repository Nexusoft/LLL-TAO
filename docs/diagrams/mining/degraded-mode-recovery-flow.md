# Degraded Mode Recovery — Protocol Flow Diagrams

> **⚠️ Partial Update (2026-04-11):** Diagrams C and D reference
> `SessionRecoveryManager`, `PeekSession()`, `RecoverSessionByIdentity()`,
> `nReconnectCount`, and `SaveSession()` — all **removed** in PRs #530–#532.
> The SESSION_STATUS handler fixes (Diagrams A/B/E) remain accurate.

Version: Post-PR #375  
Last Updated: 2026-03-13

---

## Diagram A — Before Fix: DEGRADED MODE Doom Loop

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

## Diagram B — After Fix: DEGRADED MODE Recovery Path

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

## Diagram C — ColinAgent PeekSession vs RecoverSessionByIdentity (Bug 3)

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

## Diagram D — MINER\_READY Write-Ahead Fix (Bug 4)

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

## Diagram E — Full SESSION\_STATUS State Machine (Post-Fix)

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

*See also: [degraded-mode-recovery.md](../../current/mining/degraded-mode-recovery.md) for the full Technical Error Report and recovery validation checklist.*
