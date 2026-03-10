# Diagram 9 — Canonical `CryptoContext` Accessor

**Roadmap Item:** R-09  
**Priority:** 1 (Correctness Critical)

---

## Context (Before)

The critical bug identified in NexusMiner PR #373 was caused by `Solo` using a stale local copy of `m_chacha20_session_key` for encrypting `MINER_SET_REWARD` instead of the authoritative session key.  On the node side, the same risk exists: code that decrypts packets may read `m_chacha20_session_key` directly from a member field rather than from the authoritative container, especially after a recovery merge or reconnect.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Direct Field Access (Stale Key Risk)                      ║
║                                                                      ║
║  StatelessMinerConnection:                                           ║
║    uint32_t             m_nSessionID;       // local copy           ║
║    std::vector<uint8_t> m_chacha20Key;      // local copy           ║
║    MinerSessionContainer* m_pContainer;     // authoritative        ║
║                                                                      ║
║  MINER_SET_REWARD handler:                                           ║
║    auto result = chacha20_decrypt(packet, m_chacha20Key, ...);      ║
║                                       ↑                             ║
║                              local copy — may be stale              ║
║                              after reconnect or merge               ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║    auto result = chacha20_decrypt(packet, m_chacha20Key, ...);      ║
║                                       ↑                             ║
║                              same problem                           ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A `SessionCryptoContext` value object is fetched atomically from the container under a read lock.  All encrypt/decrypt operations use this value object.  Direct member field access is removed.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Canonical CryptoContext Accessor                          ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  struct SessionCryptoContext {                               │   ║
║  │    uint32_t             nSessionID;                          │   ║
║  │    uint256_t            hashGenesis;                         │   ║
║  │    std::vector<uint8_t> vChacha20Key;   // 32 bytes          │   ║
║  │    std::string          strKeyFingerprint;                   │   ║
║  │                                                              │   ║
║  │    bool IsValid() const;                                     │   ║
║  │  };                                                          │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  MinerSessionContainer:                                              ║
║    SessionCryptoContext GetCryptoContext() const;                    ║
║    // acquires read lock, copies fields, releases lock              ║
║                                                                      ║
║  Handler usage:                                                      ║
║                                                                      ║
║  MINER_SET_REWARD handler:                                           ║
║    auto ctx = m_pContainer->GetCryptoContext();                      ║
║    if (!ctx.IsValid()) return reject("invalid crypto context");      ║
║    auto result = chacha20_decrypt(packet, ctx.vChacha20Key, ...);   ║
║                                          ↑                          ║
║                                 always authoritative                ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║    auto ctx = m_pContainer->GetCryptoContext();                      ║
║    auto result = chacha20_decrypt(packet, ctx.vChacha20Key, ...);   ║
║                                                                      ║
║  LOG format:                                                         ║
║    "crypto_ctx: session=%u key_fp=%s genesis=%s"                     ║
║                                                                      ║
║  GAIN: ChaCha20 tag mismatch class of bug is structurally           ║
║        impossible — decrypt always uses the live authoritative key.  ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `struct SessionCryptoContext` defined in `src/LLP/include/session_recovery.h`
- [ ] `MinerSessionContainer::GetCryptoContext()` implemented (thread-safe, under read lock)
- [ ] `MINER_SET_REWARD` handler uses `GetCryptoContext()` for decrypt key
- [ ] `SUBMIT_BLOCK` handler uses `GetCryptoContext()` for decrypt key
- [ ] `grep -r 'm_chacha20Key\b' src/LLP/` returns zero hits outside container implementation
- [ ] Integration test: force a recovery merge mid-session; subsequent `SUBMIT_BLOCK` uses new key
