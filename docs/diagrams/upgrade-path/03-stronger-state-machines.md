# Diagram 3 — Stronger State Machines / Enums

**Roadmap Item:** R-03  
**Priority:** 2 (Safety & Robustness)

---

## Context (Before)

Session state is tracked by a collection of independent boolean flags.  Any combination of flags is technically reachable, including many that are nonsensical (e.g. `fRewardBound=true` with `fAuthenticated=false`).

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Boolean Flag Soup                                         ║
║                                                                      ║
║  bool fAuthenticated = false;                                        ║
║  bool fRewardBound   = false;                                        ║
║  bool fChannelSet    = false;                                        ║
║                                                                      ║
║  Legal combinations: 2³ = 8 possible flag states                    ║
║  Valid combinations: 4 ordered states (CONNECTED → … → MINING)      ║
║  Invalid combinations: 4 nonsensical states — reachable by bugs      ║
║                                                                      ║
║  Handler guard pattern:                                              ║
║    if (!fAuthenticated) return reject;       (repeated everywhere)  ║
║    if (!fRewardBound)   return reject;       (some handlers forget) ║
║    if (!fChannelSet)    return reject;       (some handlers forget) ║
║                                                                      ║
║  RISK: Any handler that omits a flag check accepts state it          ║
║        should not, silently.                                         ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A `MinerSessionState` enum class replaces the flags.  Each handler declares the minimum state it requires; invalid transitions are impossible by construction.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — MinerSessionState Enum with Transition Guards             ║
║                                                                      ║
║  enum class MinerSessionState : uint8_t {                           ║
║    CONNECTED     = 0,   // TCP open, no auth yet                    ║
║    AUTHENTICATED = 1,   // Falcon verified                          ║
║    REWARD_BOUND  = 2,   // MINER_SET_REWARD accepted                ║
║    CHANNEL_SET   = 3,   // SET_CHANNEL accepted                     ║
║    MINING        = 4,   // GET_BLOCK issued, actively mining        ║
║  };                                                                  ║
║                                                                      ║
║  State machine:                                                      ║
║                                                                      ║
║  ┌───────────┐   AUTH    ┌───────────────┐  SET_REWARD              ║
║  │ CONNECTED │──────────▶│ AUTHENTICATED │────────────────┐         ║
║  └───────────┘           └───────────────┘                ▼         ║
║                                                  ┌──────────────┐   ║
║                                                  │ REWARD_BOUND │   ║
║                                                  └──────┬───────┘   ║
║                                                         │ SET_CHAN  ║
║                                                         ▼           ║
║                                                  ┌──────────────┐   ║
║                                                  │ CHANNEL_SET  │   ║
║                                                  └──────┬───────┘   ║
║                                                         │ GET_BLOCK ║
║                                                         ▼           ║
║                                                  ┌──────────────┐   ║
║                                                  │   MINING     │   ║
║                                                  └──────────────┘   ║
║                                                                      ║
║  Handler requires:                                                   ║
║    SUBMIT_BLOCK  → requires state >= MINING                         ║
║    GET_BLOCK     → requires state >= CHANNEL_SET                    ║
║    SET_CHANNEL   → requires state >= REWARD_BOUND                   ║
║    SET_REWARD    → requires state >= AUTHENTICATED                  ║
║                                                                      ║
║  Illegal transitions → compile-time assertion, not runtime check    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `enum class MinerSessionState` defined in `src/LLP/include/session_recovery.h`
- [ ] `MinerSessionContainer::eState` replaces `fAuthenticated`, `fRewardBound`, `fChannelSet`
- [ ] Each handler checks `eState >= RequiredState` instead of individual flags
- [ ] Invalid transition attempt logs `[STATE_MACHINE] illegal transition` with current state
- [ ] Unit test: attempt `SUBMIT_BLOCK` in each state < `MINING` → rejected
