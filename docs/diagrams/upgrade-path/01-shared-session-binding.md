# Diagram 1 — Shared `SessionBinding` Value Object

**Roadmap Item:** R-01  
**Priority:** 1 (Correctness Critical)

---

## Context (Before)

Session identity is compared by repeating raw field comparisons at every call site.  A single off-by-one or field-swap bug can cause a session to be accepted when it should be rejected, or vice versa.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Scattered Identity Comparison                            ║
║                                                                      ║
║  MINER_AUTH handler:                                                 ║
║    if (snapshot.nSessionID  == incoming.nSessionID  &&              ║
║        snapshot.hashGenesis == incoming.hashGenesis &&              ║
║        snapshot.strFalconKeyID == incoming.strFalconKeyID)          ║
║      → merge (correct if fields match)                              ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║    if (container.nSessionID == packetSessionID &&                   ║
║        container.hashGenesis == expectedGenesis)                    ║
║      → allow submit (NOTE: Falcon key not checked here!)            ║
║                                                                      ║
║  ValidateConsistency():                                              ║
║    checks nSessionID != 0                                            ║
║    checks hashGenesis != null                                        ║
║    checks strFalconKeyID != ""                                       ║
║      → three separate checks, no shared abstraction                 ║
║                                                                      ║
║  RISK: Any call site that forgets one field allows impersonation.   ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A single immutable `SessionBinding` value object encapsulates the three-field identity tuple.  All comparison logic lives in `SessionBinding::Matches()`.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Shared SessionBinding Value Object                        ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  struct SessionBinding {                                     │   ║
║  │    const uint32_t     nSessionID;                            │   ║
║  │    const uint256_t    hashGenesis;                           │   ║
║  │    const std::string  strFalconKeyID;                        │   ║
║  │                                                              │   ║
║  │    bool Matches(const SessionBinding& other) const;          │   ║
║  │    bool IsValid() const;   // all three fields populated     │   ║
║  │    std::string Summary() const;  // for logging              │   ║
║  │  };                                                          │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                          │                                          ║
║          ┌───────────────┼───────────────┐                          ║
║          ▼               ▼               ▼                          ║
║  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐     ║
║  │ MinerSession│  │ Recovery    │  │ StatelessMiner          │     ║
║  │ Container   │  │ Manager     │  │ Connection handlers     │     ║
║  │             │  │             │  │                         │     ║
║  │ .binding()  │  │ .binding()  │  │ binding.Matches(other)  │     ║
║  │ ──────────  │  │ ──────────  │  │ → single call, all three│     ║
║  │ SessionBind │  │ SessionBind │  │   fields always checked │     ║
║  └─────────────┘  └─────────────┘  └─────────────────────────┘     ║
║                                                                      ║
║  GAIN: One definition, one comparison path, zero field-skip bugs.   ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `struct SessionBinding` defined in `src/LLP/include/session_recovery.h`
- [ ] `MinerSessionContainer::binding()` returns a `SessionBinding`
- [ ] `RecoverySnapshot::binding()` returns a `SessionBinding`
- [ ] All call sites that compared raw fields replaced with `binding.Matches()`
- [ ] `grep -r 'hashGenesis ==' src/LLP/` returns zero hits outside `session_binding.cpp`
