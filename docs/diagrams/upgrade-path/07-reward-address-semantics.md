# Diagram 7 — Reward Address Semantics

**Roadmap Item:** R-07  
**Priority:** 2 (Safety & Robustness)

---

## Context (Before)

Two representations of the reward address exist in the system:
- `rewardAddressString` — the human-readable Nexus address string (sent by the miner)
- `vRewardHash` — the 32-byte decoded form (stored and applied by the node)

The decode step is performed inline in the `MINER_SET_REWARD` handler with no assertion that the decoded bytes can round-trip back to the original string.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Implicit Reward Address Decode (No Round-Trip Check)      ║
║                                                                      ║
║  MINER_SET_REWARD handler:                                           ║
║                                                                      ║
║  1. Decrypt packet → 32 raw bytes (reward hash)                     ║
║  2. Store bytes in container.vRewardHash                             ║
║  3. Set container.fRewardBound = true                                ║
║                                                                      ║
║  No string form is stored. If the miner encrypted the wrong bytes   ║
║  (e.g. session ID instead of reward hash), the node stores and      ║
║  later applies those wrong bytes to the coinbase silently.          ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║    applies container.vRewardHash to coinbase                         ║
║    → no verification that vRewardHash decodes to a valid address    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

The node validates the decoded reward hash by attempting to re-encode it as a Nexus address and logging both forms.  An optional strict mode rejects hashes that do not round-trip to a valid address.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Explicit Reward Address Validation                        ║
║                                                                      ║
║  MINER_SET_REWARD handler (after decrypt):                           ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  raw_bytes = decrypt(packet, container.vChacha20Key)         │   ║
║  │                                                              │   ║
║  │  if (raw_bytes.size() != 32)                                 │   ║
║  │    → reject ("reward hash must be 32 bytes")                 │   ║
║  │                                                              │   ║
║  │  std::string decoded_addr = EncodeRewardAddress(raw_bytes);  │   ║
║  │                                                              │   ║
║  │  if (decoded_addr.empty())                                   │   ║
║  │    → reject ("reward bytes do not decode to valid address")  │   ║
║  │    (only in strict mode; log and continue in lenient mode)   │   ║
║  │                                                              │   ║
║  │  LOG: "[REWARD] bound: addr=%s hash=%s"                      │   ║
║  │                                                              │   ║
║  │  container.vRewardHash    = raw_bytes;                       │   ║
║  │  container.strRewardAddr  = decoded_addr;  ← NEW field       │   ║
║  │  container.fRewardBound   = true;                            │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║    assert(container.vRewardHash.size() == 32)                        ║
║    assert(!container.strRewardAddr.empty())                          ║
║    apply container.vRewardHash to coinbase                           ║
║                                                                      ║
║  GAIN: Wrong-bytes bugs produce a visible log warning, not silent    ║
║        coinbase corruption. Strict mode blocks mining entirely.      ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `MINER_SET_REWARD` handler validates `raw_bytes.size() == 32`
- [ ] `MINER_SET_REWARD` handler calls `EncodeRewardAddress()` and logs both forms
- [ ] `container.strRewardAddr` field added to `MinerSessionContainer`
- [ ] `SUBMIT_BLOCK` handler asserts non-empty `strRewardAddr`
- [ ] Config flag `mining.reward_strict_validation = true` enables hard rejection
- [ ] Unit test: send 31 bytes → rejected; 32 invalid bytes → warned/rejected; 32 valid bytes → accepted
