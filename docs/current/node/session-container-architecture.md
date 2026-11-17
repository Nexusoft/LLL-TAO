# Session Container Architecture

**Section:** Node Architecture  
**Version:** LLL-TAO 5.1.0+ (post-PR #361)  
**Last Updated:** 2026-03-10

---

## Overview

Before PR #361 the stateless mining node scattered miner session state across several independent locations:

- a session record in `SessionRecoveryManager` keyed by remote address
- live decrypt/auth state held directly on `StatelessMinerConnection`
- reward binding stored separately from the session record

This split-state model made it impossible to guarantee that the key used to decrypt `MINER_SET_REWARD` was the same key that had been bound during `MINER_AUTH`.  It also made recovery fragile: a reconnecting miner could land in a partially-restored state.

PR #361 introduced **`MinerSessionContainer`** as the single authoritative blob for all per-miner session state.

---

## MinerSessionContainer — Fields and Ownership

```
┌─────────────────────────────────────────────────────────────────────────┐
│  MinerSessionContainer                                                  │
│  (src/LLP/include/session_recovery.h)                                   │
│                                                                         │
│  Identity fields                                                        │
│  ─────────────────────────────────────────────────────────────────────  │
│  uint32_t          nSessionID        session negotiated at MINER_AUTH   │
│  uint256_t         hashGenesis       Falcon key owner's genesis hash    │
│  std::string       strFalconKeyID    fingerprint of verified public key │
│  std::string       strRemoteAddress  IP[:port] at time of auth          │
│                                                                         │
│  Crypto fields                                                          │
│  ─────────────────────────────────────────────────────────────────────  │
│  std::vector<uint8_t> vChacha20Key   32-byte ChaCha20 session key       │
│  std::vector<uint8_t> vChacha20Hash  SHA-256 fingerprint of that key    │
│                                                                         │
│  Reward binding                                                         │
│  ─────────────────────────────────────────────────────────────────────  │
│  std::vector<uint8_t> vRewardHash    32-byte decoded reward address     │
│  bool                 fRewardBound   true once MINER_SET_REWARD accepted│
│                                                                         │
│  Operational flags                                                      │
│  ─────────────────────────────────────────────────────────────────────  │
│  bool               fAuthenticated   true after Falcon verify passes    │
│  ProtocolLane       eLane            STATELESS or LEGACY                │
│  int64_t            nLastActivity    unix timestamp, updated on traffic │
└─────────────────────────────────────────────────────────────────────────┘
```

### Ownership Rule

> **`MinerSessionContainer` is always owned by the connection handler (`StatelessMinerConnection`).  
> Every other subsystem that needs session data must receive a `const&` reference or a shared pointer — it must never copy fields out into its own mutable state.**

---

## NodeSessionRegistry — Indexes Without Ownership

`NodeSessionRegistry` maintains two indexes into the live set of containers:

```
┌───────────────────────────────────────────────────────────────┐
│  NodeSessionRegistry                                          │
│  (src/LLP/include/node_session_registry.h)                    │
│                                                               │
│  std::unordered_map<uint32_t,     ContainerHandle>  bySession │
│  std::unordered_map<std::string,  ContainerHandle>  byFalcon  │
│                                                               │
│  ContainerHandle = weak_ptr<MinerSessionContainer>            │
│                    (avoids dangling reference on disconnect)  │
└───────────────────────────────────────────────────────────────┘
```

Both maps point to **the same heap object**.  Updating the container through one index is immediately visible through the other.

---

## StatelessMinerConnection Lifecycle

```
  TCP connect
      │
      ▼
  ┌──────────────────────────────────────────┐
  │  StatelessMinerConnection::ProcessPacket │
  │                                          │
  │  MINER_AUTH                              │
  │    1. Verify Falcon signature            │
  │    2. Create MinerSessionContainer       │◀── authoritative object born here
  │    3. Register in NodeSessionRegistry    │
  │    4. Write MINER_AUTH_RESPONSE          │
  │                                          │
  │  MINER_SET_REWARD                        │
  │    1. Look up container via session ID   │
  │    2. Decrypt with container.vChacha20Key│◀── uses same key as auth
  │    3. Set container.vRewardHash          │
  │    4. Set container.fRewardBound = true  │
  │                                          │
  │  SUBMIT_BLOCK                            │
  │    1. Look up container via session ID   │
  │    2. Decrypt with container.vChacha20Key│
  │    3. Validate via container invariants  │
  │    4. Apply reward from container        │
  │                                          │
  │  TCP disconnect                          │
  │    1. Snapshot container → recovery mgr  │
  │    2. Remove from NodeSessionRegistry    │
  │    3. Container destroyed                │
  └──────────────────────────────────────────┘
```

---

## ValidateConsistency

Every path that touches security-relevant state must call `ValidateConsistency()` and fail-safe on mismatch:

```cpp
// Pseudocode — see actual impl in session_recovery.cpp
bool MinerSessionContainer::ValidateConsistency(std::string& outReason) const {
    if (nSessionID == 0)           { outReason = "zero session ID";      return false; }
    if (hashGenesis.IsNull())      { outReason = "null genesis hash";    return false; }
    if (strFalconKeyID.empty())    { outReason = "empty Falcon key ID";  return false; }
    if (vChacha20Key.size() != 32) { outReason = "bad ChaCha20 key len"; return false; }
    if (fRewardBound && vRewardHash.size() != 32)
                                   { outReason = "bad reward hash len";  return false; }
    return true;
}
```

Call sites (current and planned):

| Call site | Action on failure |
|---|---|
| End of `MINER_AUTH` handler | Reject auth, send `AUTH_FAILED` |
| Start of `MINER_SET_REWARD` handler | Reject packet |
| Start of `SUBMIT_BLOCK` handler | Reject submission |
| Recovery restore path | Discard recovered snapshot |

---

## Multi-Miner Isolation

Each TCP connection creates its own `MinerSessionContainer`.  The `NodeSessionRegistry` ensures:

- A session ID collision between two miners is detected immediately (duplicate key insert fails).
- A Falcon key ID collision (same owner, two connections) is handled by the newer connection superseding the older one after the older one's snapshot is retired.
- Remote-address collisions (NAT) are disambiguated by Falcon key ID during recovery.

---

## Related Pages

- [Recovery Merge Model](recovery-merge-model.md)
- [Roadmap & Upgrade Path](roadmap-upgrade-path.md)
- [Diagrams: Shared SessionBinding (Diagram 1)](../../diagrams/upgrade-path/01-shared-session-binding.md)
- [Diagrams: Canonical ValidateConsistency (Diagram 2)](../../diagrams/upgrade-path/02-canonical-validate-consistency.md)
- [Diagrams: Canonical CryptoContext (Diagram 9)](../../diagrams/upgrade-path/09-canonical-crypto-context.md)
