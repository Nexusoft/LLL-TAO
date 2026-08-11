# Diagram 14 — Packet-Ingress Preflight Gate

**Roadmap Item:** R-14  
**Priority:** 2 (Safety & Robustness)

---

## Context (Before)

Each opcode handler performs its own session-state and container validation inline.  There is no shared pre-handler gate.  If a handler forgets to check one condition (e.g. `fAuthenticated`), the omission is invisible until a bug manifests.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Per-Handler Ad-Hoc Guards                                 ║
║                                                                      ║
║  ProcessPacket(opcode, payload):                                     ║
║    switch(opcode) {                                                  ║
║      case MINER_SET_REWARD:                                          ║
║        if (!container) return reject;     ← guard A                 ║
║        if (!fAuthenticated) return reject; ← guard B                ║
║        // process...                                                 ║
║                                                                      ║
║      case SUBMIT_BLOCK:                                              ║
║        if (!container) return reject;     ← guard A (repeated)      ║
║        // NOTE: fAuthenticated check missing!  ← bug                ║
║        if (!fRewardBound) return reject;  ← guard C                 ║
║        // process...                                                 ║
║    }                                                                 ║
║                                                                      ║
║  RISK: Guard logic duplicated across handlers.                       ║
║        Each new handler must manually implement guards.              ║
║        One omission = security gap.                                  ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A `PreflightCheck(opcode, container)` function is called before every handler.  It centralises all session-state and consistency checks.  Handler code never re-checks these conditions.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Centralised Packet-Ingress Preflight Gate                 ║
║                                                                      ║
║  ProcessPacket(opcode, payload):                                     ║
║                                                                      ║
║    ┌────────────────────────────────────────────────────────────┐    ║
║    │  PreflightResult r = PreflightCheck(opcode, container);   │    ║
║    │                                                            │    ║
║    │  Checks (in order):                                        │    ║
║    │  1. Container exists for this session ID                   │    ║
║    │  2. container.ValidateConsistency() == true                │    ║
║    │  3. container.eState >= RequiredState(opcode)              │    ║
║    │     • MINER_SET_REWARD requires AUTHENTICATED              │    ║
║    │     • SET_CHANNEL     requires REWARD_BOUND                │    ║
║    │     • GET_BLOCK       requires CHANNEL_SET                 │    ║
║    │     • SUBMIT_BLOCK    requires MINING                      │    ║
║    │  4. Opcode-specific invariants (e.g. reward not yet bound  │    ║
║    │     for a second MINER_SET_REWARD)                         │    ║
║    │                                                            │    ║
║    │  if (!r.ok) {                                              │    ║
║    │    LOG("[PREFLIGHT] rejected opcode %04X: %s", op, r.why); │    ║
║    │    journal.Append(PREFLIGHT_FAIL, r.why);                  │    ║
║    │    return SendReject(opcode, r.errorCode);                 │    ║
║    │  }                                                          │    ║
║    └────────────────────────────────────────────────────────────┘    ║
║                                                                      ║
║    switch(opcode) {                                                  ║
║      case MINER_SET_REWARD:                                          ║
║        // container is guaranteed valid, authenticated, not bound   ║
║        // no state checks needed here                               ║
║        ...                                                           ║
║                                                                      ║
║      case SUBMIT_BLOCK:                                              ║
║        // container is guaranteed valid, in MINING state            ║
║        // reward is guaranteed bound                                 ║
║        ...                                                           ║
║    }                                                                 ║
║                                                                      ║
║  GAIN: Handler body code is free of guard boilerplate.               ║
║        New handlers get all guards for free.                         ║
║        Test cases can inject any opcode in any state and verify     ║
║        preflight behaviour without entering handler logic.           ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `PreflightCheck(opcode, container)` function implemented in `src/LLP/stateless_miner_connection.cpp`
- [ ] `ProcessPacket` calls `PreflightCheck` before every `switch` case
- [ ] `RequiredState(opcode)` table populated for all mining opcodes
- [ ] Handler bodies do not duplicate state checks that preflight already covers
- [ ] Unit tests (TC-4.1 – TC-4.4) all pass
- [ ] Preflight rejection logged with opcode + reason + session journal dump
