# Diagram 13 — Per-Session Event Journal / Ring Buffer

**Roadmap Item:** R-13  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

When a mining session failure occurs (e.g. invalid ChaCha20 tag, block rejected, reward mismatch), diagnosing the root cause requires sifting through the global node log and mentally reconstructing the event sequence for the specific miner session.  There is no per-session event history.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Global Log Only                                           ║
║                                                                      ║
║  Global node log (interleaved from all miners):                      ║
║                                                                      ║
║  14:01:03 [AUTH]   Miner A authenticated (session=0x1A2B)           ║
║  14:01:04 [AUTH]   Miner B authenticated (session=0x3C4D)           ║
║  14:01:05 [REWARD] Miner A reward bound                             ║
║  14:01:06 [SUBMIT] Miner B submit rejected (invalid tag)            ║
║  14:01:07 [REWARD] Miner B reward bind attempt                      ║
║  14:01:08 [KEEPAL] Miner A keepalive                                ║
║  14:01:09 [SUBMIT] Miner B submit rejected (stale block)            ║
║                                                                      ║
║  Problem: To debug Miner B's failures, must filter/search log       ║
║           manually. Earlier events for Miner B may have scrolled    ║
║           off or been compressed.                                    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

Each `MinerSessionContainer` holds a fixed-size ring buffer of recent events.  On any failure, the node dumps the ring buffer for the affected session inline — no log search required.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Per-Session Event Journal                                 ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  struct SessionEvent {                                       │   ║
║  │    int64_t     nTimestamp;                                   │   ║
║  │    EventType   eType;      // AUTH, REWARD, KEEPALIVE, etc.  │   ║
║  │    std::string strDetail;  // event-specific detail string   │   ║
║  │  };                                                          │   ║
║  │                                                              │   ║
║  │  MinerSessionContainer:                                      │   ║
║  │    SessionEventJournal<32> journal;  // ring buffer, 32 slots│   ║
║  │    journal.Append(eType, detail);   // O(1), no alloc        │   ║
║  │    journal.DumpTo(std::ostream&);   // oldest-first dump     │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  Events recorded:                                                    ║
║    AUTH          — Falcon verify pass/fail                           ║
║    REWARD_BIND   — reward hash accepted/rejected                     ║
║    KEEPALIVE     — keepalive received                                ║
║    SUBMIT        — block submitted (pass/fail + reason)              ║
║    RECOVERY      — recovery merge attempt (pass/fail)                ║
║    RECONNECT     — TCP reconnect detected                            ║
║    CONSISTENCY   — ValidateConsistency result                        ║
║                                                                      ║
║  On failure, dump format:                                            ║
║                                                                      ║
║  [SESSION JOURNAL] session=0x3C4D falcon=sha256:cd34...             ║
║    [0] 14:01:04 AUTH       Falcon verified, genesis=9f3a...         ║
║    [1] 14:01:07 REWARD     bind attempted, raw_len=32               ║
║    [2] 14:01:07 REWARD     REJECTED: tag mismatch                   ║
║    [3] 14:01:09 SUBMIT     REJECTED: stale hashPrevBlock            ║
║                                                                      ║
║  GAIN: Root cause visible immediately on first failure, no log dig.  ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `SessionEventJournal<N>` template class implemented (no heap alloc per event)
- [ ] `MinerSessionContainer::journal` field added (32-slot ring buffer)
- [ ] All listed event types recorded at correct call sites
- [ ] `DumpTo(std::ostream&)` outputs oldest-first with timestamps
- [ ] On `BLOCK_REJECTED`, node logs journal dump automatically
- [ ] On `ValidateConsistency()` failure, node logs journal dump automatically
- [ ] Journal adds < 1µs overhead per event on RISC-V hardware
