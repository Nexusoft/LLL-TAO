# Diagram 15 — Strong Semantic ID Wrapper Types

**Roadmap Item:** R-15  
**Priority:** 4 (Future Architecture)

---

## Context (Before)

Session IDs, Falcon key IDs, and genesis hashes are represented as primitive types (`uint32_t`, `std::string`, `uint256_t`).  At any call site that takes multiple ID parameters, it is easy to pass them in the wrong order — the compiler will not catch it.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Raw Primitive IDs                                         ║
║                                                                      ║
║  Function signature:                                                 ║
║    bool LookupSession(uint32_t session_id,                          ║
║                       const std::string& falcon_id,                 ║
║                       const uint256_t& genesis);                    ║
║                                                                      ║
║  Call site:                                                          ║
║    // BUG: genesis passed where falcon_id expected                  ║
║    LookupSession(session_id, genesis_hex_str, genesis_hash);        ║
║                          ↑                                          ║
║                  compiles fine; wrong at runtime                    ║
║                                                                      ║
║  Log format:                                                         ║
║    "session=%u genesis=%s falcon=%s"                                 ║
║    // Which string is which? Easy to swap in format string.         ║
║                                                                      ║
║  RISK: Transposed IDs produce silent correctness bugs.               ║
║        Not caught by compiler; not caught by simple unit tests.      ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

Named wrapper types (`SessionId`, `FalconKeyId`, `GenesisHash`) prevent transposition.  Overloaded comparison and formatting operators provide type-safe logging and lookup.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Strong Semantic ID Wrapper Types                          ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  // src/LLP/include/session_types.h                          │   ║
║  │                                                              │   ║
║  │  struct SessionId {                                          │   ║
║  │    uint32_t value;                                           │   ║
║  │    explicit SessionId(uint32_t v) : value(v) {}              │   ║
║  │    bool operator==(const SessionId&) const = default;        │   ║
║  │    std::string ToString() const;                             │   ║
║  │  };                                                          │   ║
║  │                                                              │   ║
║  │  struct FalconKeyId {                                        │   ║
║  │    std::string value;    // "sha256:<hex>"                   │   ║
║  │    explicit FalconKeyId(std::string v) : value(std::move(v)){}│   ║
║  │    bool operator==(const FalconKeyId&) const = default;      │   ║
║  │    std::string ToString() const;                             │   ║
║  │  };                                                          │   ║
║  │                                                              │   ║
║  │  struct GenesisHash {                                        │   ║
║  │    uint256_t value;                                          │   ║
║  │    explicit GenesisHash(uint256_t v) : value(v) {}           │   ║
║  │    bool operator==(const GenesisHash&) const = default;      │   ║
║  │    std::string ToString() const;  // LE hex                  │   ║
║  │  };                                                          │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  Usage:                                                              ║
║                                                                      ║
║  // Function signature — types are self-documenting                  ║
║  bool LookupSession(SessionId sid,                                   ║
║                     FalconKeyId fid,                                 ║
║                     GenesisHash gen);                                ║
║                                                                      ║
║  // Call site — transposition caught at compile time                 ║
║  LookupSession(sid, genesis_str, genesis_hash);   // COMPILE ERROR  ║
║                                                                      ║
║  LookupSession(sid, fid, gen);                    // OK              ║
║                                                                      ║
║  // Log format — ToString() disambiguates                            ║
║  LOG("session=%s genesis=%s falcon=%s",                              ║
║      sid.ToString(), gen.ToString(), fid.ToString());                ║
║                                                                      ║
║  GAIN: ID transposition bugs caught at compile time, not runtime.    ║
║        Log strings are self-labelled. Refactors are type-checked.    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Adoption Strategy

1. Define wrapper types in `src/LLP/include/session_types.h`.
2. Update `MinerSessionContainer` to use wrapper types for `nSessionID`, `strFalconKeyID`, `hashGenesis`.
3. Update `NodeSessionRegistry` index maps to use `SessionId` and `FalconKeyId` as keys.
4. Update function signatures one subsystem at a time (`SessionRecoveryManager` → `StatelessMinerConnection` → handler code).
5. Use `static_assert(sizeof(SessionId) == sizeof(uint32_t))` to ensure no size regression.

---

## Acceptance Criteria

- [ ] `session_types.h` created with `SessionId`, `FalconKeyId`, `GenesisHash`
- [ ] `MinerSessionContainer` uses all three wrapper types
- [ ] `NodeSessionRegistry` index maps use `SessionId` and `FalconKeyId` as key types
- [ ] Transposition unit test: passing wrong type to `LookupSession` fails to compile
- [ ] No `reinterpret_cast` or `static_cast` used to bypass wrapper types
- [ ] `ToString()` output for each type is consistent with existing log format
